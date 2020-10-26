#pragma once
#include <cstdint>
#include <vector>
#include "wsa.h"

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

namespace Attribute
{
	enum {
		xor_mapped_address = 0x0020,
		software = 0x8022
	};
}

struct StunMessageHeader
{
	uint16_t messageType;
	uint16_t length;
	uint8_t cookie[4] = {0x21, 0x12, 0xa4, 0x42};
	uint32_t transactionId[3];
};

EXCEPT(StunException)

std::string ipToStr(uint32_t ip);

void getTransactionId(uint32_t* transactionId);

Addr stun(Socket& s, const Addr& stunServer);