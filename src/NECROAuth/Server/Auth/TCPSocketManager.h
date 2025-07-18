#ifndef TCP_SOCKET_MANAGER
#define TCP_SOCKET_MANAGER

#include "AuthSession.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <unordered_map>

namespace NECRO
{
namespace Auth
{
	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	class TCPSocketManager
	{
	public:
		// Construct the socket manager
		TCPSocketManager(SocketAddressesFamily _family);


	protected:
		// Underlying listener socket
		TCPSocket m_listener;

		// Connections container
		std::vector<std::shared_ptr<AuthSession>> m_list;
		
		// m_listener has index 0
		// m_WakeRead has index 1
		// Clients go from 2 to n
		std::vector<pollfd> m_poll_fds; 

		// WakeUp socket loop
		TCPSocket					m_wakeListen;
		std::unique_ptr<TCPSocket>	m_wakeRead;
		TCPSocket					m_wakeWrite;

		pollfd SetupWakeup();

	public:
		int Poll();
		void WakeUp();

	};

}
}

#endif
