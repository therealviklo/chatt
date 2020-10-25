#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"

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
			Socket s;
			s.send({argv[2], (unsigned short)std::stoul(argv[3])}, "testyy");
		}
		else if (strcmp(argv[1], "reciever") == 0)
		{
			if (argc != 2) throw std::runtime_error("invalid number of command-line arguments");
			Socket s;
			s.bind(0);
			auto name = s.getName();
			printf("IP: %s\nPort: %hu\n", name.ip.c_str(), name.port);

			Name sender;
			printf("Recieved: %s\n", s.recv(256, &sender).c_str());
			printf("From: %s:%hu", sender.ip.c_str(), sender.port);
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