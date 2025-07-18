#include "TCPSocketManager.h"

#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include "NECROServer.h"
#include "DBRequest.h"

#include <unordered_map>

namespace NECRO
{
namespace Auth
{
	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	TCPSocketManager::TCPSocketManager(SocketAddressesFamily _family) : m_listener(_family), m_wakeListen(_family), m_wakeWrite(_family)
	{
		uint16_t inPort = 61531;
		SocketAddress localAddr(AF_INET, INADDR_ANY, inPort);
		int flag = 1;

		m_listener.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		m_listener.SetSocketOption(SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));

		m_listener.Bind(localAddr);
		m_listener.SetBlockingEnabled(false);
		m_listener.Listen();

		// Initialize the pollfds vector
		pollfd pfd;
		pfd.fd = m_listener.GetSocketFD();
		pfd.events = POLLIN;
		pfd.revents = 0;
		m_poll_fds.push_back(pfd);

		//pollfd wakefd = SetupWakeup();
		//m_poll_fds.push_back(wakefd);
	}

	pollfd TCPSocketManager::SetupWakeup()
	{
		// Make the wake up read listen
		int flag = 1;

		m_wakeListen.Bind(SocketAddress(AF_INET, htonl(INADDR_LOOPBACK), 0));
		m_wakeListen.Listen(1);

		sockaddr_in wakeReadAddr;
		int len = sizeof(wakeReadAddr);
		if (getsockname(m_wakeListen.GetSocketFD(), (sockaddr*)&wakeReadAddr, &len) == SOCKET_ERROR)
		{
			LOG_ERROR("getsockname() failed with error: {}", WSAGetLastError());
			throw std::runtime_error("Failed to get socket address.");
		}
		
		// Connect on the Write side
		m_wakeWrite.Connect(SocketAddress(*reinterpret_cast<sockaddr*>(&wakeReadAddr)));

		m_wakeWrite.SetBlockingEnabled(false);
		m_wakeWrite.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

		// Accept the connection on the read side
		SocketAddress dummy;
		sock_t accepted = m_wakeListen.AcceptSys();
		m_wakeRead = std::make_unique<TCPSocket>(accepted);

		m_wakeListen.SetBlockingEnabled(false);
		m_wakeListen.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

		pollfd wakefd;
		wakefd.fd = m_wakeRead->GetSocketFD();
		wakefd.events = POLLIN;
		wakefd.revents = 0;

		return wakefd;
	}

	int TCPSocketManager::Poll()
	{
		static int timeout = 3000;	// wait forever until at least one socket has an event

		LOG_CRITICAL("WAKEN UP!!! ACQUIRING MUTEX....");
		// Execute the callbacks
		std::queue<DBRequest> requests = g_server.GetDBWorker().GetResponseQueue();
		LOG_CRITICAL("MUTEX ACQUIRED!!!");

		while (requests.size() > 0)
		{
			LOG_CRITICAL("SERVING A REQUEST!!!");
			DBRequest r = std::move(requests.front());
			requests.pop();
			r.m_callback(r.m_sqlRes);
		}

		LOG_CRITICAL("LEAVING!!!");

		LOG_DEBUG("Polling {}", m_list.size());

		int res = WSAPoll(m_poll_fds.data(), m_poll_fds.size(), timeout);

		// Check for errors
		if (res < 0)
		{
			LOG_ERROR("Could not Poll()");
			return -1;
		}

		// Check for timeout
		if (res == 0)
		{
			return 0;
		}

		// Check for the listener (index 0)
		if (m_poll_fds[0].revents != 0)
		{
			if (m_poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("Listener encountered an error.");
				return -1;
			}
			// If there's a reading event for the listener socket, it's a new connection
			else if (m_poll_fds[0].revents & POLLIN)
			{
				SocketAddress otherAddr;
				if (std::shared_ptr<AuthSession> inSock = m_listener.Accept<AuthSession>(otherAddr))
				{
					LOG_INFO("New connection! Setting up TLS and handshaking...");

					inSock->ServerTLSSetup("localhost");

					bool success = true;
					int ret = 0;
					// Perform the handshake
					while ((ret = SSL_accept(inSock->GetSSL())) != 1)
					{
						int err = SSL_get_error(inSock->GetSSL(), ret);

						// Keep trying
						if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
							continue;

						if (err == SSL_ERROR_WANT_CONNECT || err == SSL_ERROR_WANT_ACCEPT)
							continue;

						if (err == SSL_ERROR_ZERO_RETURN)
						{
							LOG_INFO("TLS connection closed by peer during handshake.");
							success = false;
							break;
						}

						if (err == SSL_ERROR_SYSCALL)
						{
							LOG_ERROR("System call error during TLS handshake. Ret: {}.", ret);
							success = false;
							break;
						}

						if (err == SSL_ERROR_SSL)
						{
							if (SSL_get_verify_result(inSock->GetSSL()) != X509_V_OK)
								LOG_ERROR("Verify error: {}\n", X509_verify_cert_error_string(SSL_get_verify_result(inSock->GetSSL())));
						}

						LOG_ERROR("TLSPerformHandshake failed!");
						success = false;
						break;
					}

					if (success)
					{
						LOG_OK("TLSPerformHandshake succeeded!");

						// Initialize status
						inSock->m_status = SocketStatus::GATHER_INFO;
						m_list.push_back(inSock); // save it in the active list

						// Add the new connection to the pfds
						pollfd newPfd;
						newPfd.fd = inSock->GetSocketFD();
						newPfd.events = POLLIN;
						newPfd.revents = 0;
						m_poll_fds.push_back(newPfd);

						// Set inSock PFD pointer with the one we've just created and put in the vector
						inSock->SetPfd(&m_poll_fds[m_poll_fds.size() - 1]);
					}
				}
			}
		}

		/*
		// Check for WakeUp
		if (m_poll_fds[1].revents != 0)
		{
			if(m_poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("WakeUp encountered an error.");
				return -1;
			}
			else if (m_poll_fds[1].revents & POLLIN)
			{
				// We gotten waken up by the DBWorker

				// Consume what was sent
				char buf[128];
				m_wakeRead->SysReceive(buf, sizeof(buf));

				LOG_CRITICAL("WAKEN UP!!! ACQUIRING MUTEX....");
				// Execute the callbacks
				std::queue<DBRequest> requests = g_server.GetDBWorker().GetResponseQueue();
				LOG_CRITICAL("MUTEX ACQUIRED!!!");

				while (requests.size() > 0)
				{
					LOG_CRITICAL("SERVING A REQUEST!!!");
					DBRequest r = std::move(requests.front());
					requests.pop();
					r.m_callback(r.m_sqlRes);
				}

				LOG_CRITICAL("LEAVING!!!");
			}
		}
		*/

		// Vector of indexes of invalid sockets we'll remove after the client check
		std::vector<int> toRemove;

		// Check for clients
		for (size_t i = 1; i < m_poll_fds.size(); i++)
		{
			if (m_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_INFO("Client socket error/disconnection detected. Removing it later.");
				toRemove.push_back(i); // i is the fds index, but in connection list it's i-2
			}
			else
			{
				// If the socket is writable AND we're looking for POLLOUT events as well (meaning there's something on the outQueue), send it!
				if (m_poll_fds[i].revents & POLLOUT)
				{
					int r = m_list[i - 1]->Send();

					// If send failed
					if (r < 0)
					{
						LOG_INFO("Client socket error/disconnection detected. Removing it later.");
						toRemove.push_back(i); // i is the fds index, but in connection list it's i-2
						continue;
					}
				}

				if (m_poll_fds[i].revents & POLLIN)
				{
					int r = m_list[i - 1]->Receive();

					// If receive failed, 
					if (r < 0)
					{
						LOG_INFO("Client socket error/disconnection detected. Removing it later.");
						toRemove.push_back(i); // i is the fds index, but in connection list it's i-2
						continue;
					}
				}
			}
		}

		/*
		if (toRemove.size() > 0)
		{
			// Remove socket from list and fds, in reverse order to avoid index shift
			std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
			for (int idx : toRemove)
			{
				LOG_DEBUG("Removing pollfd[{}] and list[{}].", idx, idx - 1);

				m_list[idx - 1]->Close(); // -2 because we have both 0 (listener) and 1 (loopback)

				m_poll_fds.erase(m_poll_fds.begin() + idx);
				m_list.erase(m_list.begin() + (idx - 1));
			}
		}
		*/

		return 0;
	}

	void TCPSocketManager::WakeUp()
	{
		/*
		LOG_CRITICAL("WAKING UP CALLED");
		char dummy = 0;
		m_wakeWrite.SysSend(&dummy, sizeof(dummy));
		*/
	}

}
}
