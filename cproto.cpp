#include "cproto.h"

uint64_t generateTransId()
{
	union {
		uint64_t res;
		uint8_t bytes[8];
	};
	for (uint8_t i = 0; i < 8; i++)
	{
		bytes[i] = rand() & 0xFF;
	}
	return res;
}

void MessageProcessor::receiverLoop(std::mutex* receiverReadyM, std::condition_variable* receiverReadyCV)
{
	try
	{
		std::unique_lock<std::mutex> receiverUL(recvM);
		{
			std::unique_lock<std::mutex> receiverReadyUL(*receiverReadyM);
			receiverReadyCV->notify_all();
		}
		while (true)
		{
			Addr sender;
			CHeader ch;
			try
			{
				s.peek(&ch, sizeof(ch), &sender);
			}
			catch (const WSAException& e)
			{
				std::unique_lock closeLock(closeMutex, std::defer_lock);
				if (!closeLock.try_lock()) return;
				throw;
			}
			catch (const Socket::ConnectionClosed& e)
			{
				s.popDatagram();
				continue;
			}
			catch (const Socket::Exception& e)
			{
				s.popDatagram();
				continue;
			}

			std::unique_lock closeLock(closeMutex, std::defer_lock);
			if (!closeLock.try_lock()) return;

			if (ch.c != 'C')
			{
				puts("MP: Strange message");
				s.popDatagram();
				continue;
			}
			
			if (ch.msgType == MsgType::recv)
			{
				std::lock_guard lg(cvs_m);
				if (cvs.count(ch.transId))
				{
					cvs.at(ch.transId).notify_all();
				}
				continue;
			}

			CHeader succMsg{};
			succMsg.transId = ch.transId;
			succMsg.msgType = MsgType::recv;
			succMsg.msgSize = sizeof(CHeader);
			try
			{
				s.send(sender, &succMsg, sizeof(succMsg));
			}
			catch (const WSAException& e) {}

			std::vector<uint8_t> data(ch.msgSize);
			s.recv(&data[0], data.size());

			switch (ch.msgType)
			{
				break;
				case MsgType::conn:
				{
					std::lock_guard lg(conns_m);
					if (std::find(conns.begin(), conns.end(), sender) == conns.end())
						conns.push_back(sender);
					Name name = addrToName(sender);
					printf("Connected:\n\tIP: %s\n\tPort: %hu\n", name.ip.c_str(), name.port);
				}
				break;
			}
		}
	}
	catch (const WSAException& e)
	{
		if (e.errCode != 10004)
		{
			std::stringstream ss;
			ss << e.what();
			ss << "\r\nError code: ";
			ss << e.errCode;
			MessageBoxA(
				nullptr,
				ss.str().c_str(),
				"Error in message processing thread",
				MB_ICONERROR | MB_TASKMODAL
			);
			throw;
		}
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error in message processing thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	catch (...)
	{
		MessageBoxW(nullptr, L"Unknown error", L"Error in message processing thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	puts("MP exiting");
}

void MessageProcessor::send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size)
{
	using namespace std::chrono_literals;

	std::vector<uint8_t> msg(sizeof(CHeader) + size);
	CHeader& ch = *(CHeader*)&msg[0];
	ch.c = 'C';
	ch.msgType = msgType;
	ch.msgSize = sizeof(CHeader) + size;
	memcpy(&msg[sizeof(CHeader)], data, size);

regenId:	
	ch.transId = generateTransId();
	
	std::unique_lock<std::mutex> ul(cvs_m);
	if (cvs.count(ch.transId)) goto regenId;
	cvs[ch.transId]; // Ser konstigt ut men det här kan inserta.

	for (char i = 0; i < 3; i++)
	{
		s.send(addr, &msg[0], sizeof(CHeader) + size);
		if (cvs.at(ch.transId).wait_for(ul, 500ms) == std::cv_status::no_timeout)
		{
			cvs.erase(ch.transId);
			return;
		}
	}
	cvs.erase(ch.transId);
	throw NotRespondingException("peer is not responding");
}

MessageProcessor::MessageProcessor(bool ipv4)
	: s(ipv4)
{
	s.bind(0);

	std::mutex receiverReadyM;
	std::condition_variable receiverReadyCV;
	std::unique_lock<std::mutex> receiverReadyUL(receiverReadyM);
	std::thread t(&MessageProcessor::receiverLoop, this, &receiverReadyM, &receiverReadyCV);
	t.swap(receiver);
	receiverReadyCV.wait(receiverReadyUL);
}

MessageProcessor::~MessageProcessor()
{
	std::lock_guard lg(closeMutex);
	s.close();
	receiver.join();
}

void MessageProcessor::connect(const Addr& addr)
{
	std::unique_lock ul(conns_m);
	if (std::find(conns.begin(), conns.end(), addr) == conns.end())
	{
		ul.unlock();

		send(addr, MsgType::conn, nullptr, 0);

		ul.lock();
		if (std::find(conns.begin(), conns.end(), addr) == conns.end())
			conns.push_back(addr);
	}
}