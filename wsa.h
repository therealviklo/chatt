#pragma once
#include "win.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winternl.h>
#include <mstcpip.h>
#include <ip2string.h>
#include <string>
#include <sstream>
#include "utils.h"

EXCEPT(AddressException)

struct Name
{
	std::string ip;
	unsigned short port;
};

union Addr
{
	sockaddr_in6 ipv6;
	sockaddr_in ipv4;
	ADDRESS_FAMILY family;

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

Name addrToName(const Addr& addr);
Addr nameToAddr(const Name& name);

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

	void initialise();
};
extern WSAHandler wsaHandler;

struct WSAException final : public std::runtime_error
{
	int errCode;
	WSAException(const char* msg, int errCode)
		: std::runtime_error(msg),
		  errCode(errCode) {}
	WSAException(const char* msg)
		: std::runtime_error(msg),
		  errCode(WSAGetLastError()) {}
};

class Socket
{
public:
	EXCEPT(ConnectionClosed)
	EXCEPT(Exception)
private:
	UHandle<SOCKET, closesocket, INVALID_SOCKET> s;
	bool ipv4;
public:
	Socket(bool ipv4);

	Addr getAddr();

	void bind(unsigned short port);
	void setTimeout(DWORD time);

	void send(const Addr& addr, const std::string& msg);
	void send(const Addr& addr, const void* data, int size);

	std::string recv(int max, Addr* addr = nullptr);
	void recv(void* buffer, int size, Addr* addr = nullptr);
	
	void peek(void* buffer, int size, Addr* addr = nullptr);
};