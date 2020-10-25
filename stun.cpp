#include "stun.h"

void getTransactionId(uint32_t* transactionId)
{
	for (uint8_t i = 0; i < 3; i++)
	{
		for (uint8_t j = 0; j < 4; j++)
		{
			transactionId[i] += ((rand() & 0xFF) << (j * 8));
		}
	}
}

void stun(Socket& s)
{
	uint32_t transactionId[3]{};
	getTransactionId(transactionId);

	struct {
		StunMessageHeader smh{};
		// struct {
		// 	uint16_t type = Attribute::software;
		// 	uint16_t len = 4;
		// 	char value[4] = {'c', 'h', 'a', 't'};
		// } softwareAttribute;
	} req;
	req.smh.messageType = htons(MessageType::Method::binding | MessageType::Class::request);
	req.smh.length = sizeof(req) - sizeof(req.smh);
	memcpy(req.smh.transactionId, transactionId, sizeof(uint32_t[3]));

	DWORD timeout = 500;
	while (true)
	{
		s.setTimeout(timeout);

		s.send({"203.183.172.196", 3478}, &req, sizeof(req));

		StunMessageHeader smh;
		try
		{
			s.peek(&smh, sizeof(smh));
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
		if (memcmp(smh.transactionId, transactionId, sizeof(uint32_t[3])))
			throw StunException("server responded with invalid transaction ID");

		const unsigned short bufSize = ntohs(smh.length) + 20;
		
		printf( "Message type: %hu\n"
				"Message size: %hu\n", ntohs(smh.messageType), bufSize);
		
		std::vector<char> buf(bufSize);
		s.setTimeout(0);
		s.recv(&buf[0], bufSize);

		break;
	}
}