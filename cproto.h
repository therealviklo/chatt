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
		succ = 0x63637573,
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
	
	std::thread receiver;

	void receiverLoop();
public:
	MessageProcessor(bool ipv4);
	~MessageProcessor();

	MessageProcessor(const MessageProcessor&) = delete;
	MessageProcessor& operator=(const MessageProcessor&) = delete;

	inline Addr stun(const Addr& stunServer) { return ::stun(s, stunServer); }

	void send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size);
	std::vector<uint8_t> recv();
};