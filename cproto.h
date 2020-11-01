#pragma once
#include <cstdint>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <chrono>
#include <queue>
#include "wsa.h"
#include "stun.h"
#include "random.h"

namespace MsgType
{
	enum {
		recv = 0x76636572,
		conn = 0x6e6e6f63,
		text = 0x74786574
	};
}

struct DistrId
{
	uint8_t bytes[16];

	constexpr bool operator==(const DistrId& o) const noexcept
	{
		for (unsigned i = 0; i < sizeof(bytes); i++)
		{
			if (bytes[i] != o.bytes[i]) return false;
		}
		return true;
	}
};
template <>
struct std::hash<DistrId>
{
	size_t operator()(const DistrId& did) const;
};

struct CHeader
{
	uint8_t c = 'C';
	uint64_t transId;
	uint32_t msgType;
	uint64_t msgSize;
};

struct TextHeader
{
	DistrId distrId;
};

uint64_t generateTransId();
DistrId generateDistrId();

class MessageProcessor
{
public:
	EXCEPT(NotRespondingException)
private:
	Socket s;

	std::mutex cvs_m;
	std::unordered_map<uint64_t, std::condition_variable> cvs;

	std::unordered_map<uint64_t, time_t> recentMsgs;
	std::unordered_map<DistrId, time_t> recentDistrMsgs;

	std::mutex recvM;
	std::condition_variable recvCV;

	std::mutex conns_m;
	std::vector<Addr> conns;

	std::mutex distributors_m;
	std::condition_variable distributors_cv;
	std::vector<std::thread> distributors;
	std::thread distributorJoiner;
	
	bool closing;
	std::mutex closeMutex;

	std::thread receiver;

	void distributeMessage(Addr from, std::vector<uint8_t> msg);

	void distributorJoinerLoop();
	void receiverLoop(std::mutex* receiverReadyM, std::condition_variable* receiverReadyCV);
public:
	MessageProcessor(bool ipv4);
	~MessageProcessor();

	MessageProcessor(const MessageProcessor&) = delete;
	MessageProcessor& operator=(const MessageProcessor&) = delete;
	
	inline Addr stun(const Addr& stunServer) { return ::stun(s, stunServer); }

	void send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size);
	void sendMessage(const std::string& message);

	void connect(const Addr& addr);
};