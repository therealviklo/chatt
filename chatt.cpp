#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"
#include "stun.h"
#include "cproto.h"
#include "gui.h"

// Huvudfilen

int main()
{
	try // Fånga alla körtidsfel så att man kan visa en ruta med felet
	{
		wsaHandler.initialise();

		puts("c = connect, s = send, q = quit");
		MessageProcessor mp(true, 0);
		while (true)
		{
			printf("> ");
			char c = getchar();
			getchar();
			switch (c)
			{
				case 'c':
				{
					try
					{
						printf("IP: ");
						char ip[48];
						if (!fgets(ip, 48, stdin)) throw std::runtime_error("IP too long");
						printf("Port: ");
						char port[10];
						if (!fgets(port, 10, stdin)) throw std::runtime_error("port too long");
						unsigned short portI = std::strtoul(port, nullptr, 10);
						mp.connect(nameToAddr({ip, portI}));
					}
					catch (...)
					{
						puts("Kunde inte ansluta");
					}
				}
				break;
				case 's':
				{
					printf("Message: ");
					char msg[256];
					if (!fgets(msg, 256, stdin)) throw std::runtime_error("message too long");
					mp.sendMessage(msg);
				}
				break;
				case 'q': return EXIT_SUCCESS;
			}
		}
	}
	catch (const WSAException& e) // WSAException har en felkod som man kan skriva ut
	{
		std::stringstream ss;
		ss << e.what();
		ss << "\r\nError code: ";
		ss << e.errCode;
		fputs(("Error: " + ss.str()).c_str(), stderr);
		return EXIT_FAILURE;
	}
	catch (const std::exception& e) // Andra fel som ärver från std::exception
	{
		fputs(("Error: " + (std::string)e.what()).c_str(), stderr);
		return EXIT_FAILURE;
	}
	catch (...) // För fel som inte ärver från std::exception (som de borde göra)
	{
		fputs("Error: unknown error", stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}