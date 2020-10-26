#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"
#include "stun.h"

int main(int argc, char* argv[])
{
	try
	{
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		wsaHandler.initialise();

		if (argc < 2) throw std::runtime_error("invalid number of command-line arguments");
		
		if (strcmp(argv[1], "sender") == 0)
		{
			if (argc != 4) throw std::runtime_error("invalid number of command-line arguments");
			Socket s(true);
			s.send(nameToAddr({argv[2], (unsigned short)std::stoul(argv[3])}), "testyy");
		}
		else if (strcmp(argv[1], "receiver") == 0)
		{
			if (argc != 2) throw std::runtime_error("invalid number of command-line arguments");
			Socket s(true);
			s.bind(0);
			
			Addr selfAddr = s.getAddr();
			Name selfName = addrToName(selfAddr);
			printf("Self:\n\tIP: %s\n\tPort: %hu\n", selfName.ip.c_str(), selfName.port);
			Addr stunAddr = stun(s, nameToAddr({"74.125.200.127", 19302}));
			Name stunName = addrToName(stunAddr);
			printf("STUN:\n\tIP: %s\n\tPort: %hu\n", stunName.ip.c_str(), stunName.port);

			Addr sender;
			printf("Received: %s\n", s.recv(256, &sender).c_str());
			Name senderName = addrToName(sender);
			printf("From: %s:%hu", senderName.ip.c_str(), senderName.port);
		}
		else
		{
			throw std::runtime_error("invalid command-line argument");
		}
	}
	catch (const WSAException& e)
	{
		fprintf(stderr, "Error: %s\nError code: %d\n", e.what(), e.errCode);
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		fputs("Error: unknown error", stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}