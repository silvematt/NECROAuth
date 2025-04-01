#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

//----------------------------------------------------------------------------------------------------
// Define packets structures
//----------------------------------------------------------------------------------------------------
enum AuthPacketIDs
{
    PCKTID_AUTH_LOGIN_GATHER_INFO = 0x00,
	PCKTID_AUTH_LOGIN_ATTEMPT = 0x01,
	PCKTID_AUTH_CONFIRM = 0x02
};

#pragma pack(push, 1)


struct PacketAuthLoginGatherInfo
{
	uint8_t		id;
	uint8_t		error;
	uint16_t	size;
	
	uint8_t		versionMajor;
	uint8_t		versionMinor;
	uint8_t		versionRevision;

	uint32_t	ip;

	uint8_t		usernameSize;
	uint8_t		username[1];
};
static_assert(sizeof(PacketAuthLoginGatherInfo) == (1 + 1 + 2 + 1 + 1 + 1 + 4 + 1 + 1), "PacketAuthLoginGatherInfo size assert failed!");
#define	MAX_USERNAME_LENGTH 16
#define MAX_ACCEPTED_GATHER_INFO_SIZE (sizeof(PacketAuthLoginGatherInfo) + MAX_USERNAME_LENGTH) // 16 is username length

#pragma pack(pop)


std::unordered_map<uint8_t, AuthHandler> AuthSession::InitHandlers()
{
	std::unordered_map<uint8_t, AuthHandler> handlers;

	// fill

	return handlers;
}
std::unordered_map<uint8_t, AuthHandler> const Handlers = AuthSession::InitHandlers();


void AuthSession::ReadCallback()
{
	LOG_OK("AuthSession ReadCallback");
}
