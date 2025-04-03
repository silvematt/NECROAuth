#include "NECROAuth.h"

#include "SocketUtility.h"
#include "TCPSocketManager.h"
#include "Packet.h"

#include <memory>

NECROAuth server;

int NECROAuth::Init()
{
	isRunning = false;

	LOG_OK("Booting up NECROAuth...");

	SocketUtility::Initialize();

	// Make TCPSocketManager
	sockManager = std::make_unique<TCPSocketManager>(SocketAddressesFamily::INET);

	return 0;
}

void NECROAuth::Update()
{
	isRunning = true;
	LOG_OK("NECROAuth is running...");

	// Server Loop
	while (isRunning)
	{
		int pollVal = sockManager->Poll();

		if (pollVal == -1)
			Stop();
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
