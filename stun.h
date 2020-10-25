#pragma once
#include <cstdint>
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
			binding = 1,
			xor_mapped_address = 0x0020,
			software = 0x8022
		};
	}
}

struct StunMessageHeader
{
	uint16_t messageType;
	uint16_t length;
	uint8_t cookie[4] = {0x21, 0x12, 0xa4, 0x42};
	uint16_t transactionId[3];
};

void stun(Socket& s);