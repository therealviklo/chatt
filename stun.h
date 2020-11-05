#pragma once
#include <cstdint>
#include <vector>
#include "wsa.h"
#include "random.h"

// Funktioner och typer för att kommunicera med STUN-servrar.

/* Värden som representerar meddelandetyper. Typen har två
   komponenter: "Class" och "Method".*/
namespace MessageType
{
	namespace Class
	{
		enum {
			request = 0b000000000,
			indication = 0b000010000,
			success = 0b100000000,
			error = 0b100010000
		};
	}
	namespace Method
	{
		enum {
			binding = 1
		};
	}

	namespace Mask
	{
		constexpr const uint16_t Class = 0b100010000;
		constexpr const uint16_t Method = 0b0011111011101111;
	}
}

/* Värden som representerar attribut-id:n. "Attribut" är olika bitar information
   som ett meddelande kan innehålla. */
namespace Attribute
{
	enum {
		xor_mapped_address = 0x0020,
		software = 0x8022
	};
}


// Alla STUN-meddelanden har en sådan här header.
struct StunMessageHeader
{
	uint16_t messageType;
	uint16_t length;
	uint8_t cookie[4] = {0x21, 0x12, 0xa4, 0x42};
	uint8_t transactionId[12];
};

// Exception för om något går fel.
EXCEPT(StunException)

// (Finns inte redan en sådan här i wsa.h?)
std::string ipToStr(uint32_t ip);

/* Slumpar fram ett transaktions-id. (Notera att den inte returnerar id:t utan att
   den tar en pekare till början av där id:t ska läggas.) */
void getTransactionId(uint8_t* transactionId);
// XOR:ar två bitar data. 
void xorData(void* dest, const void* other, size_t size);

// Kontaktar en STUN-server och frågar den om vem man är.
Addr stun(Socket& s, const Addr& stunServer);