#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"
#include "stun.h"
#include "cproto.h"

int main(int argc, char* argv[])
{
	try
	{
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		wsaHandler.initialise();

		MessageProcessor mp(true);

		Name myName = addrToName(mp.stun(nameToAddr({"74.125.200.127", 19302})));
		printf("IP: %s\nPort: %d\n", myName.ip.c_str(), myName.port);

		printf("Connect to:\n\tIP: ");
		char ip[47];
		scanf("%s", ip);
		printf("\tPort: ");
		unsigned short port;
		scanf("%hu", &port);
		mp.connect(nameToAddr({ip, port}));
		// while (true);
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