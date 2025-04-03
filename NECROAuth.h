#ifndef NECROAUTH_H
#define NECROAUTH_H

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "TCPSocketManager.h"

class NECROAuth
{
public:
	NECROAuth() :
		isRunning(false)
	{

	}

private:
	// Status
	bool isRunning;

	ConsoleLogger cLogger;
	FileLogger fLogger;
	std::unique_ptr<TCPSocketManager> sockManager;

public:
	ConsoleLogger& GetConsoleLogger();
	FileLogger& GetFileLogger();
	TCPSocketManager& GetSocketManager();

	int						Init();
	void					Update();
	void					Stop();
	int						Shutdown();
};

// Global access for the Server 
extern NECROAuth server;

// Inline functions
inline ConsoleLogger& NECROAuth::GetConsoleLogger()
{
	return cLogger;
}

inline FileLogger& NECROAuth::GetFileLogger()
{
	return fLogger;
}

inline TCPSocketManager& NECROAuth::GetSocketManager()
{
	return *sockManager.get();
}

#endif
