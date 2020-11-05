#pragma once
#include "win.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <sstream>
#include "utils.h"

// Funktioner och typer som används för göra saker med Winsock.

/* "Name" var ett jättedåligt namnval men det här är alltså en sträng
   med en ip-address och en short med ett portnummer. Denna struct
   finns för att Addr har ip-addressen och porten i ett lite konstigt format. */
struct Name
{
	std::string ip;
	unsigned short port;
};

/* sockaddr_in är typen som används av Winsock för IPv4 och sockaddr_in6 är typen som
   används för IPv6. Addr är en union som kan användas för addresser av båda typerna.
   Både sockaddr_in och sockaddr_in6 har en medlem som är om addressen är IPv4 eller
   IPv6, och denna medlem ligger på samma ställe i båda typerna, vilket betyder att
   man kan använda Addr:s medlem family för att kolla om addressen är en IPv4-address
   eller en IPv6-address. (Den är AF_INET för IPv4 och AF_INET6 för IPv6.) */
union Addr
{
	sockaddr_in6 ipv6;
	sockaddr_in ipv4;
	ADDRESS_FAMILY family;

	// En ==-operator som gör så att man kan jämföra variabler av typen Addr med ==.
	bool operator==(const Addr& o) const noexcept
	{
		if (family == o.family)
		{
			if (family == AF_INET)
			{
				return	ipv4.sin_port == o.ipv4.sin_port &&
						ipv4.sin_addr.S_un.S_addr == o.ipv4.sin_addr.S_un.S_addr;
			}
			else
			{
				return	ipv6.sin6_port == o.ipv6.sin6_port &&
						memcmp(&ipv6.sin6_addr, &o.ipv6.sin6_addr, sizeof(ipv6.sin6_addr));
			}
		}
		return false;
	}
	inline bool operator!=(const Addr& o) const noexcept
	{
		return !operator==(o);
	}
};

// En exception som kastas av addrToName och nameToAddr när något går fel.
EXCEPT(AddressException)

// Konverterar en Addr till en Name
Name addrToName(const Addr& addr);
// Konverterar en Name till en Addr
Addr nameToAddr(const Name& name);

/* En klass som avinitierar Winsock när den förstörs (vilket är när
   programmet avslutas).*/
class WSAHandler
{
public:
	EXCEPT(InitFail)
private:
	bool initialised;
	WSADATA wsaData;
public:
	WSAHandler() noexcept : initialised(false) {}
	~WSAHandler();

	// Kör denna för att initera
	void initialise();
};
// Instansen (Den är egentligen i wsa.cpp vilket är varför den är extern.)
extern WSAHandler wsaHandler;

/* En exception för fel från Winsock. Till skillnad från de flesta exceptions i detta
   program så har den inte bara ett felmeddelande utan även en felkod. Används mest
   av klassen Socket. */
struct WSAException final : public std::runtime_error
{
	int errCode;
	// Tar felkoden som en parameter.
	WSAException(const char* msg, int errCode)
		: std::runtime_error(msg),
		  errCode(errCode) {}
	// Hämtar automatiskt felkoden med WSAGetLastError().
	WSAException(const char* msg)
		: std::runtime_error(msg),
		  errCode(WSAGetLastError()) {}
};

// En klass som representerar en (UDP-)socket. Den har funktioner för att skicka/ta emot/etc. data.
class Socket
{
public:
	/* Kastas av vissa funktioner om anslutningen bryts (vilket jag inte tror kommer hända då det här
	   är en UDP-socket. */
	EXCEPT(ConnectionClosed)
	// För fel där det inte skulle passa att kasta en WSAException.
	EXCEPT(Exception)
private:
	UHandle<SOCKET, closesocket, INVALID_SOCKET> s;
	// Om socketen använder IPv4 (true) eller IPv6 (false).
	bool ipv4;
public:
	Socket(bool ipv4);

	// Returnerar den address (med port) som bands med bind().
	Addr getAddr();

	// "Binder" socketen till en port (0 för "Välj åt mig").
	void bind(unsigned short port);
	// Ändrar timeouten för recv till time. (Däremot verkar timeouts inte fungera så bra)
	void setTimeout(DWORD time);

	// Skickar en std::string till en Addr.
	void send(const Addr& addr, const std::string& msg);
	// Skickar visst mycket data till en Addr.
	void send(const Addr& addr, const void* data, int size);

	/* Lyssna efter inkommande datagram. Läser högst max byte och datan läggs i en std::string.
	   Addr-variabeln som pekas till av addr fylls med addressen som datagrammet skickades ifrån. 
	   Sätt addr till nullptr (som är default) för att skita i addressen. */
	std::string recv(int max, Addr* addr = nullptr);
	/* Lyssna efter inkommande datagram. Läser EXAKT size byte och datan läggs dit buffer pekar.
	   Addr-variabeln som pekas till av addr fylls med addressen som datagrammet skickades ifrån. 
	   Sätt addr till nullptr (som är default) för att skita i addressen. */
	void recv(void* buffer, int size, Addr* addr = nullptr);
	
	/* Som recv men den tar inte bort datagrammet från kön (så nästa gång recv eller peek körs så
	   får man samma meddelande). Den klagar inte heller om buffern är för liten. (Meddelandet 
	   cuttas helt enkelt av.) Däremot klagar den om meddelandet är för litet. */
	void peek(void* buffer, int size, Addr* addr = nullptr);
	// Tar bort nästa datagram från kön utan att returnera det.
	void popDatagram();

	/* Stänger socketen. (Det här görs automatiskt om Socket:en förstörs men denna funktion låter en 
	   göra det utan att behöva förstöra instansen av klassen.) */
	void close();
};