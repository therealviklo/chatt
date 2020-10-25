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

void Socket::setTimeout(DWORD time)
{
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time)) == SOCKET_ERROR)
		throw WSAException("failed to set socket timeout");
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
	int received;
	if (name)
	{
		sockaddr_in sa{};
		int size = sizeof(sa);
		received = ::recvfrom(s, &buf[0], max, 0, (sockaddr*)&sa, &size);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
		*name = addrToName(sa);
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

void Socket::recv(void* buffer, int size, Name* name)
{
	int received;
	if (name)
	{
		sockaddr_in sa{};
		int size = sizeof(sa);
		received = ::recvfrom(s, (char*)buffer, size, 0, (sockaddr*)&sa, &size);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
		*name = addrToName(sa);
	}
	else
	{
		received = ::recvfrom(s, (char*)buffer, size, 0, nullptr, nullptr);
		if (received == SOCKET_ERROR) throw WSAException("failed to receive data");
	}
	if (received == 0) throw ConnectionClosed("connection closed gracefully");
	if (received != size) throw Exception("did not receive correct amount of data");
}

void Socket::peek(void* buffer, int size, Name* name)
{
	int received;
	if (name)
	{
		sockaddr_in sa{};
		int size = sizeof(sa);
		received = ::recvfrom(s, (char*)buffer, size, MSG_PEEK, (sockaddr*)&sa, &size);
		if (received == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != 10040)
				throw WSAException("failed to receive data", errCode);
		}
		*name = addrToName(sa);
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