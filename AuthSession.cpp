#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "AuthCodes.h"

#include "TCPSocketManager.h"

#pragma pack(push, 1)

struct PacketAuthLoginGatherInfo
{
    uint8_t		id;
    uint8_t		error;
    uint16_t	size;

    uint8_t		versionMajor;
    uint8_t		versionMinor;
    uint8_t		versionRevision;

    uint8_t		usernameSize;
    uint8_t		username[1];
};
static_assert(sizeof(PacketAuthLoginGatherInfo) == (1 + 1 + 2 + 1 + 1 + 1 + 1 + 1), "PacketAuthLoginGatherInfo size assert failed!");
#define	MAX_USERNAME_LENGTH 16-1 // 1 is already in packet username[1] 
#define MAX_ACCEPTED_GATHER_INFO_SIZE (sizeof(PacketAuthLoginGatherInfo) + MAX_USERNAME_LENGTH) // 16 is username length
#define PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE 4 // this represent the fixed portion of this packet, which needs to be read to at least identify the packet

struct PacketAuthLoginGatherInfoResponse
{
    uint8_t		id;
    uint8_t		error;
    uint16_t	size;
};
static_assert(sizeof(PacketAuthLoginGatherInfoResponse) == (1 + 1 + 2), "PacketAuthLoginGatherInfoResponse size assert failed!");
#define MAX_ACCEPTED_GATHER_INFO_RESPONSE_SIZE (sizeof(PacketAuthLoginGatherInfoResponse)) // 16 is username length
#define PACKET_AUTH_LOGIN_GATHER_INFO_RESPONSE_INITIAL_SIZE 4 // this represent the fixed portion of this packet, which needs to be read to at least identify the packet



#pragma pack(pop)


std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
{
	std::unordered_map<uint8_t, AuthHandler> handlers;

    handlers[PCKTID_AUTH_LOGIN_GATHER_INFO] = { AuthStatus::STATUS_GATHER_INFO, PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE, &HandleAuthLoginGatherInfoPacket};

	return handlers;
}
std::unordered_map<uint8_t, AuthHandler> const Handlers = AuthSession::InitHandlers();


void AuthSession::ReadCallback()
{
	LOG_OK("AuthSession ReadCallback");

    NetworkMessage& packet = inBuffer;

    while (packet.GetActiveSize())
    {
        uint8_t cmd = packet.GetReadPointer()[0]; // read first byte

        auto it = Handlers.find(cmd);
        if (it == Handlers.end())
        {
            // Discard packet, nothing we should handle
            packet.Clear();
            break;
        }

        // Check if the current cmd matches our state
        if (status != it->second.status)
        {
            Shutdown();
            Close();
            return;
        }

        // Check if the passed packet sizes matches the handler's one, otherwise we're not ready to process this yet
        uint16_t size = uint16_t(it->second.packetSize);
        if (packet.GetActiveSize() < size)
            break;

        // If it's a variable-sized packet, we need to ensure size
        if (cmd == PCKTID_AUTH_LOGIN_GATHER_INFO)
        {
            PacketAuthLoginGatherInfo* pcktData = reinterpret_cast<PacketAuthLoginGatherInfo*>(packet.GetReadPointer());
            size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

            // Check for size
            if (size > MAX_ACCEPTED_GATHER_INFO_SIZE)
            {
                Shutdown();
                Close();
                return;
            }
        }

        // At this point, ensure the read size matches the whole packet size
        if (packet.GetActiveSize() < size)
            break;  // probably a short receive

        // Call the Handler's function and ensure it returns true
        if (!(*this.*it->second.handler)())
        {
            Close();
            return;
        }

        packet.ReadCompleted(size); // Flag the read as completed, the while will look for remaining packets
    }
}

bool AuthSession::HandleAuthLoginGatherInfoPacket()
{
    PacketAuthLoginGatherInfo* pcktData = reinterpret_cast<PacketAuthLoginGatherInfo*>(inBuffer.GetReadPointer());

    std::string login((char const*)pcktData->username, pcktData->usernameSize);

    LOG_OK("Handling AuthLoginInfo for user:" + login);

    // Here we would perform checks such as account exists, banned, suspended, IP locked, region locked, etc.
    // Just check if the username is active 
    bool usernameInUse = TCPSocketManager::UsernameIsActive(login);

    // Reply to the client
    Packet packet;

    packet << uint8_t(AuthPacketIDs::PCKTID_AUTH_LOGIN_GATHER_INFO);
    
    if (usernameInUse)
    {
        LOG_INFO("User tried to login with an username already in use.");
        packet << uint8_t(AuthResults::AUTH_FAILED_USERNAME_IN_USE);
    }
    else
    {
        packet << uint8_t(AuthResults::AUTH_SUCCESS);
        
        TCPSocketManager::RegisterUsername(login, this);
        username = login;
    }

    packet << uint16_t(sizeof(PacketAuthLoginGatherInfoResponse) - PACKET_AUTH_LOGIN_GATHER_INFO_RESPONSE_INITIAL_SIZE);

    NetworkMessage m(packet);
    QueuePacket(m);

    Send();

    return true;
}
