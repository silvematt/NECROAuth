#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include "TCPSocket.h"

// Status of the current socket
enum AuthStatus
{
    STATUS_GATEHR_INFO = 0,
    STATUS_LOGIN_ATTEMPT,
    STATUS_AUTHED,
    STATUS_CLOSED
};

//----------------------------------------------------------------------------------------------------
// AuthSession is the extension of the base TCPSocket class, that defines the methods and
// functionality that defines the exchange of messages with the connected client on the other end
//----------------------------------------------------------------------------------------------------
class AuthSession : public TCPSocket
{
public:
    AuthSession(sock_t socket) : TCPSocket(socket), status(STATUS_GATEHR_INFO) {}

    AuthStatus status;
    
    void ReadCallback() override;
};

#endif
