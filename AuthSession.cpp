#include "AuthSession.h"
#include "NECROAuth.h"
#include "AuthCodes.h"
#include "TCPSocketManager.h"

#include <random>
#include <sstream>
#include <iomanip>

#pragma pack(push, 1)

// -------------------------------------------------------------------------------------------------------
// When defining packets: 
// 1) S prefix means (for)Server, so it's a packet that the server will receive and the client will send
// 2) C prefix means (for)Client, so it's a packet that the client will receive and the server will send
// -------------------------------------------------------------------------------------------------------

struct SPacketAuthLoginGatherInfo
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
static_assert(sizeof(SPacketAuthLoginGatherInfo) == (1 + 1 + 2 + 1 + 1 + 1 + 1 + 1), "SPacketAuthLoginGatherInfo size assert failed!");
#define	MAX_USERNAME_LENGTH 16-1 // 1 is already in packet username[1] 
#define S_MAX_ACCEPTED_GATHER_INFO_SIZE (sizeof(SPacketAuthLoginGatherInfo) + MAX_USERNAME_LENGTH) // 16 is username length
#define S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE 4 // this represent the fixed portion of this packet, which needs to be read to at least identify the packet


struct CPacketAuthLoginGatherInfo
{
    uint8_t		id;
    uint8_t		error;
};
static_assert(sizeof(CPacketAuthLoginGatherInfo) == (1 + 1), "CPacketAuthLoginGatherInfo size assert failed!");

// Login Proof doesn't exist for now, this is just a dull packet
struct SPacketAuthLoginProof
{
    uint8_t		id;
    uint8_t		error;

    uint32_t    clientsIVRandomPrefix;
};
static_assert(sizeof(SPacketAuthLoginProof) == (1 + 1 + 4), "SPacketAuthLoginProof size assert failed!");


// Login Proof doesn't exist for now, this is just a dull packet
struct CPacketAuthLoginProof
{
    uint8_t		id;
    uint8_t		error;
    uint16_t    size;

    uint8_t     sessionKey[AES_128_KEY_SIZE];
};
static_assert(sizeof(CPacketAuthLoginProof) == (1 + 1 + 2 + AES_128_KEY_SIZE), "CPacketAuthLoginProof size assert failed!");
#define C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE 4 // this represent the fixed portion of this packet, which needs to be read to at least identify the packet



#pragma pack(pop)


std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
{
	std::unordered_map<uint8_t, AuthHandler> handlers;

    handlers[PCKTID_AUTH_LOGIN_GATHER_INFO] = { AuthStatus::STATUS_GATHER_INFO, S_PACKET_AUTH_LOGIN_GATHER_INFO_INITIAL_SIZE, &HandleAuthLoginGatherInfoPacket};
    handlers[PCKTID_AUTH_LOGIN_ATTEMPT]     = { AuthStatus::STATUS_LOGIN_ATTEMPT, sizeof(SPacketAuthLoginProof), &HandleAuthLoginProofPacket};

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
            LOG_WARNING("Status mismatch for user: " + data.username + ". Status is: '" + std::to_string(status) + "' but should have been '" + std::to_string(it->second.status) + "'. Closing the connection.");

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
            SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(packet.GetReadPointer());
            size += pcktData->size; // we've read the handler's defined packetSize, so this is safe. Attempt to read the remainder of the packet

            // Check for size
            if (size > S_MAX_ACCEPTED_GATHER_INFO_SIZE)
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
    SPacketAuthLoginGatherInfo* pcktData = reinterpret_cast<SPacketAuthLoginGatherInfo*>(inBuffer.GetReadPointer());

    std::string login((char const*)pcktData->username, pcktData->usernameSize);

    LOG_OK("Handling AuthLoginInfo for user:" + login);

    // Here we would perform checks such as account exists, banned, suspended, IP locked, region locked, etc.
    // Just check if the username is active 
    bool usernameInUse = TCPSocketManager::UsernameIsActive(login);

    // Reply to the client
    Packet packet;

    packet << uint8_t(AuthPacketIDs::PCKTID_AUTH_LOGIN_GATHER_INFO);

    // Check the DB, see if user exists
    LoginDatabase& db = server.GetDirectDB();
    mysqlx::SqlStatement s = db.Prepare(LoginDatabaseStatements::LOGIN_SEL_ACCOUNT_ID_BY_NAME);
    s.bind(login);

    /*
    * EXAMPLE OF WORKER THREAD USAGE
    auto& w = server.GetDBWorker();

    for (int i = 0; i < 100000; i++)
    {
        // MAKE SURE STATEMENT IS PREPARED FROM THE WORKER DB!!!
        mysqlx::SqlStatement s = server.GetDBWorker().Prepare(LoginDatabaseStatements::LOGIN_SEL_ACCOUNT_ID_BY_NAME);
        s.bind(login);

        w.Enqueue(std::move(s));
    }
    */

    mysqlx::SqlResult result = db.Execute(s);
    mysqlx::Row row = result.fetchOne();


    if (!row)
    {
        LOG_INFO("User tried to login with an username that doesn't exist.");
        packet << uint8_t(AuthResults::AUTH_FAILED_UNKNOWN_ACCOUNT);
    }
    else
    {
        // Check client version with server's client version
        if (pcktData->versionMajor == CLIENT_VERSION_MAJOR && pcktData->versionMinor == CLIENT_VERSION_MINOR && pcktData->versionRevision == CLIENT_VERSION_REVISION)
        {
            packet << uint8_t(AuthResults::AUTH_SUCCESS);

            TCPSocketManager::RegisterUsername(login, this);
            data.username = login;
            status = STATUS_LOGIN_ATTEMPT;

            // Done, will wait for client's proof packet
        }
        else
        {
            LOG_INFO("User tried to login with an invalid client version.");
            packet << uint8_t(AuthResults::AUTH_FAILED_WRONG_CLIENT_VERSION);
        }
    }

    NetworkMessage m(packet);

    /* Encryption example
    int res = m.AESEncrypt(data.sessionKey.data(), data.iv, nullptr, 0);
    if (res < 0)
        return false;
    */

    QueuePacket(m);

    //Send(); packets are sent by checking POLLOUT events in the authSockets, and we check for POLLOUT events only if there are packets written in the outQueue

    return true;
}

bool AuthSession::HandleAuthLoginProofPacket()
{
    SPacketAuthLoginProof* pcktData = reinterpret_cast<SPacketAuthLoginProof*>(inBuffer.GetReadPointer());

    LOG_OK("Handling AuthLoginProof for user:" + data.username);

    // Reply to the client
    Packet packet;

    packet << uint8_t(AuthPacketIDs::PCKTID_AUTH_LOGIN_ATTEMPT);

    /*
    if (pcktData->key != 242) old dull check, here as an example of sending an error message
    {
        LOG_INFO("User tried to send proof with a wrong key.");
        packet << uint8_t(LoginProofResults::LOGIN_FAILED);

        packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE - AES_128_KEY_SIZE); // Adjust the size appropriately
    }
    */

    packet << uint8_t(LoginProofResults::LOGIN_SUCCESS);

    packet << uint16_t(sizeof(CPacketAuthLoginProof) - C_PACKET_AUTH_LOGIN_PROOF_INITIAL_SIZE); // Adjust the size appropriately, here we send the key

    // Calculate this side's IV, making sure it's different from the client's
    while(pcktData->clientsIVRandomPrefix == data.iv.prefix)
        data.iv.RandomizePrefix();

    data.iv.ResetCounter();

    LOG_INFO("Client's IV Random Prefix: " + std::to_string(pcktData->clientsIVRandomPrefix) + " | Server's IV Random Prefix: " + std::to_string(data.iv.prefix));

    // Calculate a random session key
    data.sessionKey = NECROAES::GenerateSessionKey();
        

    // Convert sessionKey to hex string in order to print it
    std::ostringstream sessionStrStream;
    for (int i = 0; i < AES_128_KEY_SIZE; ++i)
    {
        sessionStrStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data.sessionKey[i]);
    }
    std::string sessionStr = sessionStrStream.str();

    LOG_DEBUG("Session key for user " + data.username + " is: " + sessionStr);

    // Write session key to packet
    for (int i = 0; i < AES_128_KEY_SIZE; ++i)
    {
        packet << data.sessionKey[i];
    }

    NetworkMessage m(packet);
    QueuePacket(m);

    //Send(); packets are sent by checking POLLOUT events in the authSockets, and we check for POLLOUT events only if there are packets written in the outQueue

    return true;
}
