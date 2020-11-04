#include "cproto.h"

size_t std::hash<DistrId>::operator()(const DistrId& did) const
{
	return *(uint64_t*)&did.bytes[0] + *(uint64_t*)&did.bytes[8];
}

uint64_t generateTransId()
{
	union {
		uint64_t res;
		uint8_t bytes[8];
	};
	for (uint8_t i = 0; i < 8; i++)
	{
		bytes[i] = random() & 0xFF;
	}
	return res;
}

DistrId generateDistrId()
{
	DistrId id;
	for (uint8_t i = 0; i < sizeof(id); i++)
	{
		id.bytes[i] = random() & 0xFF;
	}
	return id;
}

void MessageProcessor::distributeMessage(Addr from, std::vector<uint8_t> msg)
{
	std::vector<std::thread> threads;

	{
		std::lock_guard lg(conns_m);
		for (const auto& conn : conns)
		{
			if (conn != from)
			{
				threads.push_back(std::thread([&](){
					try
					{
						send(conn, ((CHeader*)&msg[0])->msgType, &msg[sizeof(CHeader)], msg.size() - sizeof(CHeader));
					}
					catch (...) {}
				}));
			}
		}
	}

	for (auto& thr : threads)
	{
		thr.join();
	}
}

void MessageProcessor::idCleanerLoop()
{
	try
	{
		using namespace std::chrono_literals;
		std::unique_lock ul(msgs_m);
		while (msgs_cv.wait_for(ul, 2min) == std::cv_status::timeout)
		{
			for (auto i = recentMsgs.begin(); i != recentMsgs.end(); )
			{
				if (difftime(time(nullptr), i->second) > 120.0)
				{
					i = recentMsgs.erase(i);
				}
				else
				{
					i++;
				}
			}
			for (auto i = recentDistrMsgs.begin(); i != recentDistrMsgs.end(); )
			{
				if (difftime(time(nullptr), i->second) > 120.0)
				{
					i = recentDistrMsgs.erase(i);
				}
				else
				{
					i++;
				}
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
				"Error in ID cleaning thread",
				MB_ICONERROR | MB_TASKMODAL
			);
			throw;
		}
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error in ID cleaning thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	catch (...)
	{
		MessageBoxW(nullptr, L"Unknown error", L"Error in ID cleaning thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
}

void MessageProcessor::distributorJoinerLoop()
{
	try
	{
		while (true)
		{
			std::thread* currThr = nullptr;
			{
				std::unique_lock ul(distributors_m);
				while (!distributors.size())
				{
					if (closing) return;
					distributors_cv.wait(ul);
				}
				currThr = &distributors[0];
			}
			currThr->join();
			{
				std::lock_guard lg(distributors_m);
				distributors.erase(distributors.begin());
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
				"Error in distribution thread joiner thread",
				MB_ICONERROR | MB_TASKMODAL
			);
			throw;
		}
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error in distribution thread joiner thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
	catch (...)
	{
		MessageBoxW(nullptr, L"Unknown error", L"Error in distribution thread joiner thread", MB_ICONERROR | MB_TASKMODAL);
		throw;
	}
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
			
			printf("Received:\n\tTransID: %zu\n", ch.transId);
			
			if (ch.msgType == MsgType::recv)
			{
				printf("\tType: recv\n");
				std::lock_guard lg(cvs_m);
				if (cvs.count(ch.transId))
				{
					cvs.at(ch.transId).notify_all();
				}
				s.popDatagram();
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

			{
				std::lock_guard lg(msgs_m);
				if (recentMsgs.count(ch.transId))
				{
					if (difftime(time(nullptr), recentMsgs.at(ch.transId)) > 120.0)
					{
						printf("\t(Already received, but old)\n");
						recentMsgs.erase(ch.transId);
					}
					else
					{
						printf("\t(Already received)\n");
						s.popDatagram();
						continue;
					}
				}
				recentMsgs.emplace(ch.transId, time(nullptr));
			}

			std::vector<uint8_t> data(ch.msgSize);
			s.recv(&data[0], data.size());

			switch (ch.msgType)
			{
				case MsgType::text:
				{
					printf("\tType: text\n");

					const DistrId& distrId = ((TextHeader*)&data[sizeof(CHeader)])->distrId;
					printf("\tDistrID: %zu-%zu\n", *(uint64_t*)&distrId.bytes[0], *(uint64_t*)&distrId.bytes[8]);
					{
						std::lock_guard lg(msgs_m);
						if (recentDistrMsgs.count(distrId))
						{
							if (difftime(time(nullptr), recentDistrMsgs.at(distrId)) > 120.0)
							{
								printf("\t(Already received, but old)\n");
								recentDistrMsgs.erase(distrId);
							}
							else
							{
								printf("\t(Already received)\n");
								break;
							}
						}
						recentDistrMsgs.emplace(distrId, time(nullptr));
					}

					printf("\tText: %s\n", &data[sizeof(CHeader) + sizeof(TextHeader)]);

					std::lock_guard lg(distributors_m);
					distributors.push_back(std::thread(
						&MessageProcessor::distributeMessage,
						this,
						sender,
						std::move(data)
					));
				}
				break;
				case MsgType::conn:
				{
					printf("\tType: conn\n");

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

	printf(
		"Sending:\n\tTransID: %zu\n\tType: %c%c%c%c\n",
		ch.transId,
		msgType & 0xFF,
		(msgType >> 8) & 0xFF,
		(msgType >> 16) & 0xFF,
		(msgType >> 24) & 0xFF
	);

	for (char i = 0; i < 3; i++)
	{
		s.send(addr, &msg[0], sizeof(CHeader) + size);
		if (cvs.at(ch.transId).wait_for(ul, 500ms) == std::cv_status::no_timeout)
		{
			cvs.erase(ch.transId);
			return;
		}
		puts("\t(Resending)");
	}
	puts("\t(Giving up)");
	cvs.erase(ch.transId);
	throw NotRespondingException("peer is not responding");
}

void MessageProcessor::sendMessage(const std::string& message)
{
	std::vector<uint8_t> msg(sizeof(TextHeader) + message.size());

	memcpy(&msg[sizeof(TextHeader)], message.data(), message.size());
	
	DistrId& distrId = ((TextHeader*)&msg[0])->distrId;
regenId:
	distrId = generateDistrId();
	if (recentDistrMsgs.count(distrId))
	{
		if (difftime(time(nullptr), recentDistrMsgs.at(distrId)) > 120.0)
		{
			recentDistrMsgs.erase(distrId);
		}
		else goto regenId;
	}

	std::lock_guard lg(distributors_m);
	distributors.push_back(std::thread([this](std::vector<uint8_t> msg){
		std::vector<std::thread> threads;

		{
			std::lock_guard lg(conns_m);
			for (const auto& conn : conns)
			{
				threads.push_back(std::thread([&](){
					try
					{
						send(conn, MsgType::text, &msg[0], msg.size());
					}
					catch (...) {}
				}));
			}
		}

		for (auto& thr : threads)
		{
			thr.join();
		}
	}, std::move(msg)));
}

MessageProcessor::MessageProcessor(bool ipv4, short port)
	: s(ipv4),
	  closing(false),
	  idCleaner(&MessageProcessor::idCleanerLoop, this),
	  distributorJoiner(&MessageProcessor::distributorJoinerLoop, this)
{
	s.bind(port);
	
	Name myName = addrToName(stun(nameToAddr({"74.125.200.127", 19302})));
	printf("IP: %s\nPort: %d\n", myName.ip.c_str(), myName.port);

	std::mutex receiverReadyM;
	std::condition_variable receiverReadyCV;
	std::unique_lock<std::mutex> receiverReadyUL(receiverReadyM);
	std::thread t(&MessageProcessor::receiverLoop, this, &receiverReadyM, &receiverReadyCV);
	t.swap(receiver);
	receiverReadyCV.wait(receiverReadyUL);
}

MessageProcessor::~MessageProcessor()
{
	closing = true;
	std::lock_guard lg(closeMutex);
	s.close();
	
	receiver.join();
	
	{
		std::lock_guard lg2(distributors_m);
		distributors_cv.notify_all();
	}
	distributorJoiner.join();

	{
		std::lock_guard lg3(msgs_m);
		msgs_cv.notify_all();
	}
	idCleaner.join();
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