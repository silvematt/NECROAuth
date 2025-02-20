#include "NECROAuth.h"

#include "SocketUtility.h"
#include "TCPSocketManger.h"
#include "Packet.h"

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
	TCPSocketManager sockManager(SocketAddressesFamily::INET);

	// @DEBUG just for debugging
	Packet p;
	p.Print();
	uint8_t uint = 1;
	p.Append(&uint, sizeof(uint8_t));
	p.Print();
	p << std::string("hello") << " " << " world!";
	p.Print();

	NetworkMessage m(p.Size());
	m.Write(p.GetContent(), p.Size());

	// Server Loop
	while (isRunning)
	{
		int pollVal = sockManager.Poll();

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
