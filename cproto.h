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

// Funktioner och typer som används för att hantera kommunikation mellan noder. (cproto = chat protocol)

/* Olika slags meddelanden i protokollet. Meddelandets typ representeras av fyra tecken, och dessa värden 
   är tecknen som ett tal (i omvänd ordning för att x86-64 använder little-endian). */
namespace MsgType
{
	enum {
		/* Används för att bekräfta att ett meddelande har mottagits. Skickas alltid
		   som svar på meddelanden, och transId:t ska vara samma som meddelandet som
		   togs emot. Ingen data utöver CHeadern. */
		recv = 0x76636572,
		// Används för att ansluta till en annan nod. Ingen data utöver CHeadern.
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

/* Det här är en klass som innehåller några trådar som hanterar meddelanden som kommer
   in från andra noder och meddelanden som ska skickas. Den innehåller även de variabler
   som de behöver. */
class MessageProcessor
{
public:
	EXCEPT(NotRespondingException)
private:
	// Socketen som används för att komunicera.
	Socket s;

	/* När man ska skicka ett meddelande lägger man in sitt transId här och sover med
	   std::condition_variablen vid det id:t. När receivertråden får ett recv-meddelande
	   som svar väcker den den tråd vid det id:t med hjälp av denna map. */
	std::mutex cvs_m; // Mutex för att synkronisera
	std::unordered_map<uint64_t, std::condition_variable> cvs;

	/* Två mappar som innehåller tiden när ett id senast användes. (Så att man kan veta om
	   ett meddelande är en dubblett.) */
	std::mutex msgs_m; // Mutexen används till båda mappar.
	std::condition_variable msgs_cv; // Används för att väcka och stoppa idCleaner när klassen ska förstöras.
	std::unordered_map<uint64_t, time_t> recentMsgs;
	std::unordered_map<DistrId, time_t> recentDistrMsgs;

	// En lista med alla andra noder som man vet är online.
	std::mutex conns_m;
	std::vector<Addr> conns;

	/* En lista med trådar som skickar ut ett meddelande som ska reläas. (De
	   måste vänta på svar så de är egna trådar.) */
	std::mutex distributors_m;
	/* Tråden som joinar dessa trådar sover när det inte finns några trådar att
	   joina och denna används för att väcka den igen. */
	std::condition_variable distributors_cv;
	std::vector<std::thread> distributors;
	
	// Dessa används för att meddela till vissa trådar att de ska avsluta.
	bool closing;
	std::mutex closeMutex;

	// Trådarna
	std::thread idCleaner; // Tar bort gamla id:n från recentMsgs och recentDistrMsgs.
	std::thread distributorJoiner; // Joinar trådari distributors.
	std::thread receiver; // Tar emot meddelanden.
	
	inline Addr stun(const Addr& stunServer) { return ::stun(s, stunServer); }

	// Används för att reläa ett meddelande som man har fått till alla andra.
	void distributeMessage(Addr from, std::vector<uint8_t> msg);

	void idCleanerLoop();
	void distributorJoinerLoop();
	void receiverLoop();
public:
	MessageProcessor(bool ipv4, short port);
	~MessageProcessor();

	// Ingen kopiering
	MessageProcessor(const MessageProcessor&) = delete;
	MessageProcessor& operator=(const MessageProcessor&) = delete;
	
	// Skicka ett meddelande till en viss address.
	void send(const Addr& addr, uint32_t msgType, const void* data, uint64_t size);
	// Skicka ett textmeddelande till alla andra.
	void sendMessage(const std::string& message);

	// Anslut till en annan nod.
	void connect(const Addr& addr);
};