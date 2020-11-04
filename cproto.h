#pragma once
#include <cstdint>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <chrono>
#include <queue>
#include <condition_variable>
#include "wsa.h"
#include "stun.h"
#include "random.h"

// Funktioner och typer som används för att hantera kommunikation mellan peers. (Cproto = chat protocol)

/* Olika slags meddelanden i protokollet. Meddelandets typ representeras av fyra tecken, och dessa värden 
   är tecknen som ett tal (i omvänd ordning för att x86-64 använder little-endian). */
namespace MsgType
{
	enum {
		/* Används för att bekräfta att ett meddelande har mottagits. Skickas alltid
		   som svar på meddelanden, och transId:t ska vara samma som meddelandet som
		   togs emot. Ingen data utöver CHeadern. */
		recv = 0x76636572,
		// Används för att ansluta till en annan peer. Ingen data utöver CHeadern.
		conn = 0x6e6e6f63,
		/* Används för att skicka eller reläa ett meddelande. Utöver CHeadern ska det ingå 
		   en TextHeader och själva meddelandet. */
		text = 0x74786574
	};
}

/* Typen för id:t som används för att identifiera meddelanden som reläas. 
   Den är egentligen bara 16 byte men jag har även gjort en ==-operaor för
   att jämföra DistrId:n och en hashfunktion (under den). Hashfunktionen är
   för att man ska kunna använda typen i en std::unordered_map */
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

/* En struct som representerar strukturen i headern som alla meddelanden har.
   Man kan använda Socket:s andra recv-funktion för att skicka structs. */
struct CHeader
{
	/* Den första byten ska alltid vara Unicode/ASCII-koden för ett stort c.
	   Detta är för att det ska vara lättare att skilja meddelandena från andra
	   meddelanden. */
	uint8_t c = 'C';
	/* Transaktions-id:t. Detta slumpas fram varje gång man ska skicka ett meddelande
	   (förutom när det är ett recv-meddelande). */
	uint64_t transId;
	// Meddelandets typ.
	uint32_t msgType;
	// Storleken på meddelanden, INKLUSIVE CHeadern.
	uint64_t msgSize;
};

// Headern för "text"-meddelanden.
struct TextHeader
{
	// Ett framslumpat id som representerar chattmeddelandet.
	DistrId distrId;
};

// Slumpar fram ett transId.
uint64_t generateTransId();
// Slumpar fram ett DistrId.
DistrId generateDistrId();

class MessageProcessor
{
public:
	EXCEPT(NotRespondingException)
private:
	Socket s;

	std::mutex cvs_m;
	std::unordered_map<uint64_t, std::condition_variable> cvs;

	std::mutex msgs_m;
	std::condition_variable msgs_cv;
	std::unordered_map<uint64_t, time_t> recentMsgs;
	std::unordered_map<DistrId, time_t> recentDistrMsgs;

	std::mutex recvM;
	std::condition_variable recvCV;

	std::mutex conns_m;
	std::vector<Addr> conns;

	std::mutex distributors_m;
	std::condition_variable distributors_cv;
	std::vector<std::thread> distributors;
	
	bool closing;
	std::mutex closeMutex;

	std::thread idCleaner;
	std::thread distributorJoiner;
	std::thread receiver;

	void distributeMessage(Addr from, std::vector<uint8_t> msg);

	void idCleanerLoop();
	void distributorJoinerLoop();
	void receiverLoop(std::mutex* receiverReadyM, std::condition_variable* receiverReadyCV);
public:
	MessageProcessor(bool ipv4, short port);
	~MessageProcessor();

	MessageProcessor(const MessageProcessor&) = delete;
	MessageProcessor& operator=(const MessageProcessor&) = delete;
	
	inline Addr stun(const Addr& stunServer) { return ::stun(s, stunServer); }

	void send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size);
	void sendMessage(const std::string& message);

	void connect(const Addr& addr);
};
