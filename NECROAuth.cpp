#include "NECROAuth.h"

#include "SocketUtility.h"
#include "TCPSocket.h"
#include <memory>

NECROAuth server;

int NECROAuth::Init()
{
	isRunning = false;

	LOG_OK("Booting up NECROAuth...");

	SocketUtility::Initialize();

	return 0;
}

void NECROAuth::Update()
{
	isRunning = true;
	LOG_OK("NECROAuth is running...");

	// DEBUG: Just a quick test
	SocketAddress localAddr(AF_INET, INADDR_ANY, 61531);
	TCPSocket listenerSocket(SocketAddressesFamily::INET);

	int flag = 1;
	listenerSocket.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	listenerSocket.Bind(localAddr);

	listenerSocket.Listen();

	std::shared_ptr<TCPSocket> inSock;

	// Engine Loop
	while (isRunning)
	{
		// Loop
		SocketAddress otherAddress;
		if (inSock = listenerSocket.Accept(otherAddress))
		{
			LOG_INFO("Somebody just connected!");
		}
	}

	Shutdown();
}

void NECROAuth::Stop()
{
	LOG_OK("Stopping NECROAuth...");

	isRunning = false;
}

int NECROAuth::Shutdown()
{
	// Shutdown

	LOG_OK("Shut down of the NECROAuth completed.");
	return 0;
}
