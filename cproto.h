#pragma once
#include <cstdint>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include "wsa.h"
#include "stun.h"

namespace MsgType
{
	enum {
		recv = 0x76636572,
		conn = 0x6e6e6f63
	};
}

struct CHeader
{
	uint8_t c = 'C';
	uint64_t transId;
	uint32_t msgType;
	uint64_t msgSize;
};

uint64_t generateTransId();

class MessageProcessor
{
public:
	EXCEPT(NotRespondingException)
private:
	Socket s;

	std::mutex cvs_m;
	std::unordered_map<uint64_t, std::condition_variable> cvs;

	std::unordered_map<uint64_t, time_t> recentMsgs;

	std::mutex recvM;
	std::condition_variable recvCV;

	std::mutex conns_m;
	std::vector<Addr> conns;
	
	std::mutex closeMutex;

	std::thread receiver;

	// std::thread protocolHandler;

	void receiverLoop(std::mutex* receiverReadyM, std::condition_variable* receiverReadyCV);

	// std::vector<uint8_t> recv(Addr* sender);
	// void protocolHandlerLoop();
public:
	MessageProcessor(bool ipv4);
	~MessageProcessor();

	MessageProcessor(const MessageProcessor&) = delete;
	MessageProcessor& operator=(const MessageProcessor&) = delete;
	
	inline Addr stun(const Addr& stunServer) { return ::stun(s, stunServer); }

	void send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size);

	void connect(const Addr& addr);
};