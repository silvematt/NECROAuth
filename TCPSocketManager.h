#ifndef TCP_SOCKET_MANAGER
#define TCP_SOCEKT_MANAGER

#include "AuthSession.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <unordered_map>

//-----------------------------------------------------------------------------------------------------
// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
//-----------------------------------------------------------------------------------------------------
class TCPSocketManager
{
protected:
	// Underlying listener socket
	TCPSocket listener;

	// Connections container
	std::vector<std::shared_ptr<AuthSession>> list;

	std::vector<pollfd> poll_fds;

	// Map for username lookup (since we don't use databases yet)
	static std::unordered_map<std::string, AuthSession*> usernameMap;

public:
	// Construct the socket manager
	TCPSocketManager(SocketAddressesFamily _family);

	int Poll();

	static bool UsernameIsActive(const std::string& s);

};

#endif
