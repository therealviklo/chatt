#include "wsa.h"

WSAHandler wsaHandler;

Name addrToName(const Addr& addr)
{
	std::stringstream ss;
	if (addr.family == AF_INET)
	{
		ss << (uint16_t)addr.ipv4.sin_addr.S_un.S_un_b.s_b1 << '.'
		   << (uint16_t)addr.ipv4.sin_addr.S_un.S_un_b.s_b2 << '.'
		   << (uint16_t)addr.ipv4.sin_addr.S_un.S_un_b.s_b3 << '.'
		   << (uint16_t)addr.ipv4.sin_addr.S_un.S_un_b.s_b4;
		return {ss.str(), ntohs(addr.ipv4.sin_port)};
	}
	else if (addr.family == AF_INET6)
	{
		ss << (uint16_t)addr.ipv6.sin6_addr.u.Byte[0] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[1] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[2] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[3] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[4] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[5] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[6] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[7] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[8] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[9] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[10] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[11] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[12] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[13] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[14] << ":"
		   << (uint16_t)addr.ipv6.sin6_addr.u.Byte[15] << ":";
		return {ss.str(), ntohs(addr.ipv6.sin6_port)};
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

		const char* currChar = name.ip.c_str();
		for (int i = 0; i < 4; i++)
		{
			reinterpret_cast<uint8_t*>(&sa.ipv4.sin_addr.S_un.S_un_b)[i] = strtol(currChar, (char**)&currChar, 10);
			if (*currChar == '.') currChar++;
		}
	}
	else
	{
		// TODO om det behövs
		throw AddressException("IPv6 not supported yet");
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
	return buf;
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
				throw WSAException("failed to peek data", errCode);
		}
	}
	else
	{
		received = ::recvfrom(s, (char*)buffer, size, MSG_PEEK, nullptr, nullptr);
		if (received == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != 10040)
				throw WSAException("failed to peek data", errCode);
		}
	}
	if (received == 0) throw ConnectionClosed("connection closed gracefully");
	if (received > 0 && received < size) throw Exception("did not receive correct amount of data");
}

void Socket::popDatagram()
{
	try
	{
		recv(1);
	}
	catch (const WSAException& e)
	{
		if (e.errCode != 10040) throw;
	}
}

void Socket::close()
{
	s = INVALID_SOCKET;
}