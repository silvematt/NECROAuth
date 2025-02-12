#include "TCPSocket.h"
#include "SocketUtility.h"

#include <iostream>
#include <string>

#include "ConsoleLogger.h"
#include "FileLogger.h"

TCPSocket::TCPSocket(SocketAddressesFamily family)
{
	m_socket = socket(family, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == INVALID_SOCKET)
	{
		LOG_ERROR(std::string("Error 1 during TCPSocket::Create()"));
		LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
	}
}

TCPSocket::TCPSocket(sock_t inSocket)
{
	m_socket = inSocket;

	if (m_socket == INVALID_SOCKET)
	{
		LOG_ERROR(std::string("Error 2 during TCPSocket::Create()"));
		LOG_ERROR(std::to_string(SocketUtility::GetLastError()));
	}
}

TCPSocket::~TCPSocket()
{
	closesocket(m_socket);
}

int TCPSocket::Bind(const SocketAddress& addr)
{
	int err = bind(m_socket, &addr.m_addr, addr.GetSize());

	if (err != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Bind() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
}

int TCPSocket::Listen(int backlog)
{
	int err = listen(m_socket, backlog);

	if (err != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Listen() [" + std::to_string(SocketUtility::GetLastError()) + "]"));
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
}

std::shared_ptr<TCPSocket> TCPSocket::Accept(SocketAddress& addr)
{
	int addrLen = addr.GetSize();
	sock_t inSocket = accept(m_socket, &addr.m_addr, &addrLen);

	if (inSocket != INVALID_SOCKET)
		return std::make_shared<TCPSocket>(inSocket);
	else
	{
		LOG_ERROR(std::string("Error during TCPSocket::Accept()"));
		return nullptr;
	}
}

int TCPSocket::Connect(const SocketAddress& addr)
{
	int err = connect(m_socket, &addr.m_addr, addr.GetSize());

	if (err != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Connect() [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
}

int TCPSocket::Send(const void* inData, int inLen)
{
	int bytesSent = send(m_socket, static_cast<const char*>(inData), inLen, 0);

	if (bytesSent < 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Send() [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return bytesSent;
}

int TCPSocket::Receive(void* inBuffer, int inLen)
{
	int bytesReceived = recv(m_socket, static_cast<char*>(inBuffer), inLen, 0);

	if (bytesReceived < 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Receive() [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return bytesReceived;
}

int TCPSocket::SetBlockingEnabled(bool blocking)
{
#ifdef _WIN32
	u_long mode = blocking ? 0 : 1;
	int result = ioctlsocket(m_socket, FIONBIO, &mode);

	if (result != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
#else
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
	{
		LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

	result = fcntl(fd, F_SETFL, flags);

	if (result != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::SetBlockingEnabled(" + std::to_string(blocking) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
#endif
}

int TCPSocket::SetSocketOption(int lvl, int optName, const char* optVal, int optLen)
{
	int optResult = setsockopt(m_socket, lvl, optName, optVal, optLen);

	if (optResult != 0)
	{
		LOG_ERROR(std::string("Error during TCPSocket::Receive(" + std::to_string(optName) + ") [") + std::to_string(SocketUtility::GetLastError()) + "]");
		return SocketUtility::GetLastError();
	}

	return SOCKET_UTILITY_NO_ERROR;
}
