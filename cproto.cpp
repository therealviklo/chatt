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

void MessageProcessor::receiverLoop()
{
	try
	{
		std::unique_lock<std::mutex> receiverUL(recvM);
		{
			std::unique_lock<std::mutex> receiverReadyUL(receiverReadyM);
			receiverReadyCV.notify_all();
		}
		while (true)
		{
			CHeader ch;
			try
			{
				puts("MP: Peeking...");
				s.peek(&ch, sizeof(ch));
			}
			catch (const Socket::ConnectionClosed& e)
			{
				puts("MP: Exception! (A)");
				s.popDatagram();
				continue;
			}
			catch (const Socket::Exception& e)
			{
				puts("MP: Exception! (B)");
				s.popDatagram();
				continue;
			}

			if (ch.c != 'C')
			{
				puts("MP: Strange message");
				s.popDatagram();
				continue;
			}

			if (ch.msgType == MsgType::succ)
			{
				puts("MP: Success");
				std::lock_guard lg(cvs_m);
				if (cvs.count(ch.transId))
				{
					cvs.at(ch.transId).notify_all();
				}
				s.popDatagram();
				continue;
			}

			puts("MP: Not for me");
			recvCV.wait(receiverUL);
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

MessageProcessor::MessageProcessor(bool ipv4)
	: s(ipv4),
	  receiverReadyUL(receiverReadyM),
	  receiver(&MessageProcessor::receiverLoop, this)
{
	receiverReadyCV.wait(receiverReadyUL);
	receiverReadyUL.unlock();
}

MessageProcessor::~MessageProcessor()
{
	try
	{
		s.close();
		receiver.join();
	}
	catch (...) {}
	puts("MP destructor");
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
	puts("Throwing...");
	throw NotRespondingException("peer is not responding");
}

std::vector<uint8_t> MessageProcessor::recv(Addr* sender)
{
	std::vector<uint8_t> buffer;
	while (true)
	{
		std::unique_lock<std::mutex> receiverUL(recvM);
		Defer d([&](){
			recvCV.notify_all();
		});

		CHeader ch;
		puts("PH: Peeking...");
		s.peek(&ch, sizeof(ch), sender);

		if (ch.c != 'C')
		{
			puts("PH: Not for me (A)");
			continue;
		}
		if (ch.msgType == MsgType::succ)
		{
			puts("PH: Not for me (B)");
			continue;
		}

		buffer.resize(ch.msgSize);
		try
		{
			puts("PH: Receiving ...");
			s.recv(&buffer[0], buffer.size());
		}
		catch (const Socket::ConnectionClosed& e)
		{
			puts("PH: Exception! (C)");
			continue;
		}
		catch (const Socket::Exception& e)
		{
			puts("PH: Exception! (D)");
			continue;
		}

		if (recentMsgs.count(ch.transId))
		{
			if (difftime(time(nullptr), recentMsgs.at(ch.transId)) > 300)
			{
				recentMsgs.erase(ch.transId);
			}
			else
			{
				puts("PH: Already received");
				continue;
			}
		}
		recentMsgs.insert({ch.transId, time(nullptr)});

		CHeader succMsg{};
		succMsg.transId = ch.transId;
		succMsg.msgType = MsgType::succ;
		succMsg.msgSize = sizeof(CHeader);
		try
		{
			s.send(*sender, &succMsg, sizeof(succMsg));
		}
		catch (const WSAException& e) {}

		puts("PH: Breaking ...");
		break;
	}
	return buffer;
}

void ProtocolHandler::receiverLoop()
{
	try
	{
		while (true)
		{
			Addr sender;
			puts("PH: Starting recv procedure ...");
			std::vector<uint8_t> msg = mp.recv(&sender);
			const CHeader& ch = *(CHeader*)&msg[0];
			puts("PH: recv procedure over");
			switch (ch.msgType)
			{
				case MsgType::conn:
				{
					puts("PH: conn");
					std::lock_guard lg(conns_m);
					if (std::find(conns.begin(), conns.end(), sender) == conns.end())
						conns.push_back(sender);
					Name name = addrToName(sender);
					printf("Connected:\n\tIP: %s\n\tPort: %hu\n", name.ip.c_str(), name.port);
				}
				break;
			}
			puts("PH: Looping ...");
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
			puts(ss.str().c_str());
			MessageBoxA(
				nullptr,
				ss.str().c_str(),
				"Error in protocol handler thread",
				MB_ICONERROR | MB_TASKMODAL
			);
			throw;
		}
	}
	catch (const std::exception& e)
	{
		puts(e.what());
		MessageBoxA(nullptr, e.what(), "Error in protocol handler thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	catch (...)
	{
		puts("Unknown error");
		MessageBoxW(nullptr, L"Unknown error", L"Error in protocol handler thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	puts("PH exiting");
}

ProtocolHandler::ProtocolHandler(bool ipv4)
	: mp(ipv4),
	  receiver(&ProtocolHandler::receiverLoop, this) {}

ProtocolHandler::~ProtocolHandler()
{
	puts("PH destructor");
	mp.s.close();
	receiver.join();
}

void ProtocolHandler::connect(const Addr& addr)
{
	std::unique_lock ul(conns_m);
	if (std::find(conns.begin(), conns.end(), addr) == conns.end())
	{
		ul.unlock();

		mp.send(addr, MsgType::conn, nullptr, 0);

		ul.lock();
		if (std::find(conns.begin(), conns.end(), addr) == conns.end())
			conns.push_back(addr);
	}
}