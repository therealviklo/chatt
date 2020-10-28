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
		while (true)
		{
			CHeader ch;
			try
			{
				s.peek(&ch, sizeof(ch));
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

			if (ch.c != 'C')
			{
				s.popDatagram();
				continue;
			}

			if (ch.msgType == MsgType::succ)
			{
				std::lock_guard lg(cvs_m);
				if (cvs.count(ch.transId))
				{
					cvs.at(ch.transId).notify_all();
				}
				s.popDatagram();
				continue;
			}
		}
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error in receiving thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	catch (...)
	{
		MessageBoxW(nullptr, L"Unknown error", L"Error in receiving thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
}

MessageProcessor::MessageProcessor(bool ipv4)
	: s(ipv4),
	  receiver(&MessageProcessor::receiverLoop, this) {}

MessageProcessor::~MessageProcessor()
{
	try
	{
		s.close();
		receiver.join();
	}
	catch (...) {}
}

void MessageProcessor::send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size)
{
	using namespace std::chrono_literals;

	std::vector<uint8_t> msg(sizeof(CHeader) + size);
	CHeader& ch = *(CHeader*)&msg[0];
	ch.c = 'C';
	ch.msgType = msgType;
	ch.msgSize = htonll(sizeof(CHeader) + size);
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
	throw NotRespondingException("peer is not responding");
}

std::vector<uint8_t> MessageProcessor::recv()
{
	std::vector<uint8_t> buffer;
	while (true)
	{
		CHeader ch;
		Addr sender;
		try
		{
			s.peek(&ch, sizeof(ch), &sender);
		}
		catch (const Socket::ConnectionClosed& e)
		{
			continue;
		}
		catch (const Socket::Exception& e)
		{
			continue;
		}

		if (ch.c != 'C') continue;
		if (ch.msgType == MsgType::succ) continue;

		buffer.resize(ntohll(ch.msgSize));
		try
		{
			s.recv(&buffer[0], buffer.size());
		}
		catch (const Socket::ConnectionClosed& e)
		{
			continue;
		}
		catch (const Socket::Exception& e)
		{
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
				continue;
			}
		}
		recentMsgs.insert({ch.transId, time(nullptr)});

		CHeader succMsg{};
		succMsg.transId = ch.transId;
		succMsg.msgType = MsgType::succ;
		succMsg.msgSize = 0;
		try
		{
			s.send(sender, &succMsg, sizeof(succMsg));
		}
		catch (const WSAException& e) {}

		break;
	}
	return buffer;
}