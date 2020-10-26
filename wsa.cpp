#include "wsa.h"

WSAHandler wsaHandler;

Name addrToName(const Addr& addr)
{
	char buf[47];
	if (addr.family == AF_INET)
	{
		RtlIpv4AddressToStringA(&addr.ipv4.sin_addr, buf);
		return {buf, ntohs(addr.ipv4.sin_port)};
	}
	else if (addr.family == AF_INET6)
	{
		RtlIpv6AddressToStringA(&addr.ipv6.sin6_addr, buf);
		return {buf, ntohs(addr.ipv6.sin6_port)};
	}
	else
	{
		throw AddressException("address is neither IPv4 nor IPv6");
	}
}

Addr nameToAddr(const Name& name)
{
	Addr sa{};
	if (name.ip.find_first_of("."))
	{
		sa.ipv4.sin_family = AF_INET;
		sa.ipv4.sin_port = htons(name.port);
		const char* end;
		if (RtlIpv4StringToAddressA(name.ip.c_str(), FALSE, &end, &sa.ipv4.sin_addr))
			throw AddressException("failed to convert IP address");
	}
	else
	{
		sa.ipv6.sin6_family = AF_INET6;
		sa.ipv6.sin6_port = htons(name.port);
		const char* end;
		if (RtlIpv6StringToAddressA(name.ip.c_str(), &end, &sa.ipv6.sin6_addr))
			throw AddressException("failed to convert IP address");	
	}
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

Socket::Socket(bool ipv4)
	: ipv4(ipv4)
{
	s = socket(ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (!s) throw WSAException("failed to create socket");
}

Addr Socket::getAddr()
{
	Addr sa;
	int len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len) == SOCKET_ERROR)
		throw WSAException("failed to get socket name");
	return sa;
}

void Socket::bind(unsigned short port)
{
	if (ipv4)
	{
		sockaddr_in sa{};
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = INADDR_ANY;
		if (::bind(s, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
			throw WSAException("failed to bind socket");
	}
	else
	{
		sockaddr_in6 sa{};
		sa.sin6_family = AF_INET6;
		sa.sin6_port = htons(port);
		sa.sin6_addr = in6addr_any;
		if (::bind(s, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
			throw WSAException("failed to bind socket");
	}
}

void Socket::setTimeout(DWORD time)
{
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time)) == SOCKET_ERROR)
		throw WSAException("failed to set socket timeout");
}

void Socket::send(const Addr& addr, const std::string& msg)
{
	if (sendto(
		s,
		msg.data(),
		msg.size(),
		0,
		(const sockaddr*)&addr,
		addr.family == AF_INET ? sizeof(addr.ipv4) : sizeof(addr.ipv6)
	) == SOCKET_ERROR)
		throw WSAException("failed to send data");
}

void Socket::send(const Addr& addr, const void* data, int size)
{
	if (sendto(
		s,
		(const char*)data,
		size,
		0,
		(const sockaddr*)&addr,
		addr.family == AF_INET ? sizeof(addr.ipv4) : sizeof(addr.ipv6)
	) == SOCKET_ERROR)
		throw WSAException("failed to send data");
}

std::string Socket::recv(int max, Addr* addr)
{
	std::string buf;
	buf.resize(max);
	int received;
	if (addr)
	{
		int size = sizeof(Addr);
		received = ::recvfrom(s, &buf[0], max, 0, (sockaddr*)addr, &size);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
	}
	else
	{
		received = ::recvfrom(s, &buf[0], max, 0, nullptr, nullptr);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
	}
	if (received == 0) return "";
	buf.resize(received);
	return std::move(buf);
}

void Socket::recv(void* buffer, int size, Addr* addr)
{
	int received;
	if (addr)
	{
		int size = sizeof(Addr);
		received = ::recvfrom(s, (char*)buffer, size, 0, (sockaddr*)addr, &size);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
	}
	else
	{
		received = ::recvfrom(s, (char*)buffer, size, 0, nullptr, nullptr);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
	}
	if (received == 0) throw ConnectionClosed("connection closed gracefully");
	if (received != size) throw Exception("did not receive correct amount of data");
}

void Socket::peek(void* buffer, int size, Addr* addr)
{
	int received;
	if (addr)
	{
		int size = sizeof(Addr);
		received = ::recvfrom(s, (char*)buffer, size, MSG_PEEK, (sockaddr*)addr, &size);
		if (received == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != 10040)
				throw WSAException("failed to receive data", errCode);
		}
	}
	else
	{
		received = ::recvfrom(s, (char*)buffer, size, MSG_PEEK, nullptr, nullptr);
		if (received == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != 10040)
				throw WSAException("failed to receive data", errCode);
		}
	}
	if (received == 0) throw ConnectionClosed("connection closed gracefully");
	if (received > 0 && received < size) throw Exception("did not receive correct amount of data");
}