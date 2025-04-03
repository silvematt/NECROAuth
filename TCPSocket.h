#ifndef TCP_SOCKET_H
#define TCP_SOCEKT_H

#ifdef _WIN32
#include "WinSock2.h"
typedef SOCKET sock_t;
#else
typedef int sock_t;
#endif

#include <memory>
#include <cstdint>
#include <queue>

#include "SocketAddress.h"
#include "NetworkMessage.h"

#include "SocketUtility.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"


#define READ_BLOCK_SIZE 4096

const int TCP_LISTEN_DEFUALT_BACKLOG = SOMAXCONN;

enum SocketAddressesFamily
{
	INET = AF_INET,
	INET6 = AF_INET6
};

//-------------------------------------------------------
// Defines a TCP Socket object.
//-------------------------------------------------------
class TCPSocket
{
protected: 
	friend class SocketAddress;
	sock_t m_socket;

	SocketAddress remoteAddress;
	uint16_t remotePort;

	// Read/Write buffers
	NetworkMessage inBuffer;
	std::queue<NetworkMessage> outQueue;

	bool closed = false;

public:
	TCPSocket(SocketAddressesFamily family);
	TCPSocket(sock_t inSocket);

	~TCPSocket();

	virtual void OnConnectedCallback() {};
	virtual void ReadCallback() {};
	virtual void SendCallback() {};

	bool						IsOpen();

	int							Bind(const SocketAddress& addr);
	int							Listen(int backlog = TCP_LISTEN_DEFUALT_BACKLOG);

	// Templated Accept
	template<typename T = TCPSocket>
	std::shared_ptr<T> Accept(SocketAddress& addr)
	{
		static_assert(std::is_base_of<TCPSocket, T>::value || std::is_same<TCPSocket, T>::value, "T must be TCPSocket or derived from it");

		int addrLen = addr.GetSize();
		sock_t inSocket = accept(m_socket, &addr.m_addr, &addrLen);

		if (inSocket != INVALID_SOCKET)
		{
			return std::make_shared<T>(inSocket); // Assumes T has a constructor taking sock_t
		}
		else
		{
			if (!SocketUtility::ErrorIsWouldBlock())
			{
				LOG_ERROR(std::string("Error during TCPSocket::Accept()"));
			}
			return nullptr;
		}
	}

	int							Connect(const SocketAddress& addr);
	
	void						QueuePacket(NetworkMessage& pckt);
	int							Send();
	int							Receive();

	int							Shutdown();
	int							Close();

	sock_t						GetSocketFD() { return m_socket; };

	int							SetBlockingEnabled(bool blocking);
	int							SetSocketOption(int lvl, int optName, const char* optVal, int optLen);
};

#endif
