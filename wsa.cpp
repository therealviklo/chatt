#include "wsa.h"

WSAHandler wsaHandler;

Name addrToName(const sockaddr_in& addr)
{
	char buf[47];
	RtlIpv4AddressToStringA(&addr.sin_addr, buf);
	return {buf, ntohs(addr.sin_port)};
}

sockaddr_in nameToAddr(const Name& name)
{
	sockaddr_in sa{};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(name.port);
	const char* end;
	if (RtlIpv4StringToAddressA(name.ip.c_str(), FALSE, &end, &sa.sin_addr))
		throw AddressException("failed to convert IP address");
	return sa;
}

WSAHandler::~WSAHandler()
{
	if (initialised) WSACleanup();
}

void WSAHandler::initialise()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		throw InitFail("failed to initialise Winsock");
	initialised = true;
}

Socket::Socket()
{
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!s) throw WSAException("failed to create socket");
}

Name Socket::getName()
{
	sockaddr_in sa;
	int len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len) == SOCKET_ERROR)
		throw WSAException("failed to get socket name");
	return addrToName(sa);
}

void Socket::bind(unsigned short port)
{
	sockaddr_in sa{};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;
	if (::bind(s, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw WSAException("failed to bind socket");
}

void Socket::send(const Name& name, const std::string& msg)
{
	sockaddr_in sa = nameToAddr(name);
	if (sendto(s, msg.data(), msg.size(), 0, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw WSAException("failed to send data");
}

void Socket::send(const Name& name, const void* data, int size)
{
	sockaddr_in sa = nameToAddr(name);
	if (sendto(s, (const char*)data, size, 0, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw WSAException("failed to send data");
}

std::string Socket::recv(int max, Name* name)
{
	std::string buf;
	buf.resize(max);
	int recieved;
	if (name)
	{
		sockaddr_in sa{};
		int size = sizeof(sa);
		recieved = ::recvfrom(s, &buf[0], max, 0, (sockaddr*)&sa, &size);
		if (recieved == SOCKET_ERROR) throw WSAException("failed to recieve data");
		*name = addrToName(sa);
	}
	else
	{
		recieved = ::recvfrom(s, &buf[0], max, 0, nullptr, nullptr);
		if (recieved == SOCKET_ERROR) throw WSAException("failed to recieve data");
	}
	if (recieved == 0) return "";
	buf.resize(recieved);
	return std::move(buf);
}

void Socket::recv(void* buffer, int size, Name* name)
{
	int recieved;
	if (name)
	{
		sockaddr_in sa{};
		int size = sizeof(sa);
		recieved = ::recvfrom(s, (char*)buffer, size, 0, (sockaddr*)&sa, &size);
		if (recieved == SOCKET_ERROR) throw WSAException("failed to recieve data");
		*name = addrToName(sa);
	}
	else
	{
		recieved = ::recvfrom(s, (char*)buffer, size, 0, nullptr, nullptr);
		if (recieved == SOCKET_ERROR) throw WSAException("failed to recieve data");
	}
	if (recieved == 0) throw ConnectionClosed("connection closed gracefully");
	if (recieved != size) throw Exception("did not recieve correct amount of data");
}