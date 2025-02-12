#ifndef NECROAUTH_H
#define NECROAUTH_H

#include "ConsoleLogger.h"
#include "FileLogger.h"

class NECROAuth
{
private:
	// Status
	bool isRunning;

	ConsoleLogger cLogger;
	FileLogger fLogger;

public:
	ConsoleLogger& GetConsoleLogger();
	FileLogger& GetFileLogger();

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

#endif
