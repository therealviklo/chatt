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
		mp.stun(nameToAddr({"74.125.200.127", 19302}));
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