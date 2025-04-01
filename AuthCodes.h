#ifndef AUTH_CODES_H
#define AUTH_CODES_H


//--------------------------------------------------------------------------------------------
// Commands to identify packets, they are the first byte of the packets
//--------------------------------------------------------------------------------------------
enum AuthCommands
{
	CMD_LOGON_CHALLENGE			= 0x00,
	CMD_LOGON_PROOF				= 0x01,
	CMD_SERVER_LIST				= 0x02
};

//--------------------------------------------------------------------------------------------
// Results to send as payload to tell the client what happened as a result of the command 
//--------------------------------------------------------------------------------------------
enum AuthResults
{
	AUTH_SUCCESS						= 0x00,
	AUTH_FAILED_UNKNOWN_ACCOUNT			= 0x01,
	AUTH_FAILED_ACCOUNT_BANNED			= 0x02,
	AUTH_FAILED_WRONG_PASSWORD			= 0x03,
	AUTH_FAILED_WRONG_CLIENT_VERSION	= 0x04
};


#endif
