#ifndef TCP_SOCKET_MANAGER
#define TCP_SOCEKT_MANAGER

#include "TCPSocket.h"

//-----------------------------------------------------------------------------------------------------
// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
//-----------------------------------------------------------------------------------------------------
class TCPSocketManager
{
protected:
	// Underlying listener socket
	TCPSocket listener;

	// Connections container
	std::vector<std::shared_ptr<TCPSocket>> list;

public:
	// Construct the socket manager
	TCPSocketManager(SocketAddressesFamily _family) : listener(_family)
	{
		uint16_t inPort = 61531;
		SocketAddress localAddr(AF_INET, INADDR_ANY, inPort);
		int flag = 1;

		listener.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		listener.SetSocketOption(IPPROTO_TCP, SO_REUSEADDR, (char*)&flag, sizeof(int));

		listener.Bind(localAddr);
		listener.SetBlockingEnabled(false);
		listener.Listen();
	}

	// Replace with Poll
	void Run()
	{
		SocketAddress otherAddress;
		if (std::shared_ptr<TCPSocket> inSock = listener.Accept(otherAddress))
		{
			LOG_INFO("Somebody just connected!");
			list.push_back(inSock);
		}

		for (int i = 0; i < list.size(); i++)
			list[i]->Receive();
	}
};

#endif
