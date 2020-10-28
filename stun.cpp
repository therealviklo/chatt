#include "stun.h"

std::string ipToStr(uint32_t ip)
{
	std::stringstream ss;
	ss << ((ip >> 24) & 0xFF);
	ss << '.';
	ss << ((ip >> 16) & 0xFF);
	ss << '.';
	ss << ((ip >> 8) & 0xFF);
	ss << '.';
	ss << (ip & 0xFF);
	return ss.str();
}

void getTransactionId(uint8_t* transactionId)
{
	for (uint8_t i = 0; i < 12; i++)
	{
		transactionId[i] += rand() & 0xFF;
	}
}

void xorData(void* dest, const void* other, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		((uint8_t*)dest)[i] ^= ((uint8_t*)other)[i];
	}
}

Addr stun(Socket& s, const Addr& stunServer)
{
	Addr addr{};

	uint8_t transactionId[12]{};
	getTransactionId(transactionId);

	struct {
		StunMessageHeader smh{};
	} req;
	req.smh.messageType = htons(MessageType::Method::binding | MessageType::Class::request);
	req.smh.length = sizeof(req) - sizeof(req.smh);
	memcpy(req.smh.transactionId, transactionId, sizeof(uint32_t[3]));

	StunMessageHeader smh;
	DWORD timeout = 500;
	while (true)
	{
		s.setTimeout(timeout);
		Defer tod([&](){
			s.setTimeout(0);
		});

		s.send(stunServer, &req, sizeof(req));

		Addr responseAddr;
		try
		{
			s.peek(&smh, sizeof(smh), &responseAddr);
		}
		catch (const WSAException& e)
		{
			if (e.errCode == 10060)
			{
				timeout *= 2;
				continue;
			}
			throw;
		}
		if (memcmp(smh.transactionId, transactionId, sizeof(uint32_t[3])) || stunServer != responseAddr)
		{
			s.popDatagram();
			continue;
		}

		break;
	}

	if (ntohs(smh.messageType) != (MessageType::Method::binding | MessageType::Class::success))
		throw StunException("STUN request failed");

	const unsigned short bufSize = ntohs(smh.length) + 20;
	
	std::vector<uint8_t> buf(bufSize);
	s.setTimeout(0);
	s.recv(&buf[0], bufSize);

	uint8_t* pos = &buf[20];
	uint8_t* const end = &buf[bufSize];
	auto readU8 = [&]() -> uint8_t {
		const uint8_t byte = *pos;
		pos += 1;
		return byte;
	};
	auto readU16 = [&]() -> uint16_t {
		const uint16_t dbyte = ntohs(*reinterpret_cast<uint16_t*>(pos));
		pos += 2;
		return dbyte;
	};
	auto readU16N = [&]() -> uint16_t {
		const uint16_t dbyte = *reinterpret_cast<uint16_t*>(pos);
		pos += 2;
		return dbyte;
	};
	auto readU32 = [&]() -> uint32_t {
		const uint32_t qbyte = ntohl(*reinterpret_cast<uint32_t*>(pos));
		pos += 4;
		return qbyte;
	};
	auto readU32N = [&]() -> uint32_t {
		const uint32_t qbyte = *reinterpret_cast<uint32_t*>(pos);
		pos += 4;
		return qbyte;
	};
	while (pos < end)
	{
		const auto attr = readU16();
		const auto size = readU16();
		uint8_t* const attrEnd = pos + (size / 4 + 1) * 4;
		switch (attr)
		{
			case Attribute::xor_mapped_address:
			{
				readU8();
				const auto family = readU8();
				if (family == 1)
				{
					addr.family = AF_INET;
					
					addr.ipv4.sin_port = readU16N();
					addr.ipv4.sin_port ^= 0x1221;
					
					addr.ipv4.sin_addr.S_un.S_addr = readU32N();
					addr.ipv4.sin_addr.S_un.S_addr ^= 0x42a41221;
				}
				else if (family == 2)
				{
					addr.family = AF_INET6;
					
					addr.ipv6.sin6_port = readU16N();
					addr.ipv6.sin6_port ^= 0x1221;
					
					addr.ipv6.sin6_addr.u.Word[0] = readU16N();
					addr.ipv6.sin6_addr.u.Word[1] = readU16N();
					addr.ipv6.sin6_addr.u.Word[2] = readU16N();
					addr.ipv6.sin6_addr.u.Word[3] = readU16N();
					addr.ipv6.sin6_addr.u.Word[4] = readU16N();
					addr.ipv6.sin6_addr.u.Word[5] = readU16N();
					addr.ipv6.sin6_addr.u.Word[6] = readU16N();
					addr.ipv6.sin6_addr.u.Word[7] = readU16N();
					xorData(&addr.ipv6.sin6_addr, &req.smh.cookie, 4);
					xorData(&addr.ipv6.sin6_addr.u.Byte[4], &transactionId, 12);
				}
				else throw StunException("unknown address family");
			}
			break;
		}
		pos = attrEnd;
	}

	return addr;
}