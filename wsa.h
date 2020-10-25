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

Name addrToName(const sockaddr_in& addr);
sockaddr_in nameToAddr(const Name& name);

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
public:
	Socket();

	Name getName();

	void bind(unsigned short port);
	void setTimeout(DWORD time);

	void send(const Name& name, const std::string& msg);
	void send(const Name& name, const void* data, int size);

	std::string recv(int max, Name* name = nullptr);
	void recv(void* buffer, int size, Name* name = nullptr);
	
	void peek(void* buffer, int size, Name* name = nullptr);
};