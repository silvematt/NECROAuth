#include "FileLogger.h"

#include <string>

FileLogger* FileLogger::Instance()
{
    static FileLogger instance;
    return &instance;
}

FileLogger::~FileLogger()
{
    if (logFile.is_open())
        logFile.close();
}

FileLogger::FileLogger()
{
    logFile.open(DEFAULT_LOG_FILE_NAME, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
}

FileLogger::FileLogger(const std::string& filePath)
{
    logFile.open(filePath, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        std::cerr << "Error while trying to open LogFile: " << DEFAULT_LOG_FILE_NAME << std::endl;
}


void FileLogger::Log(const std::string& message, Logger::LogLevel lvl, const char* file, int line)
{
    std::lock_guard<std::mutex> guard(lMutex);

    if (!logFile.is_open())
    {
        std::cerr << "Attempt to log to an unopened file." << std::endl;
        return;
    }

    // Get timestamp
    std::string nowTimestamp = Utility::time_stamp();

    // Get level
    std::string levelStr = GetLogLevelStr(lvl);

    // Format and write the log message
    logFile << "[" << nowTimestamp << "] " << "[" << levelStr << "] ";

    if (file != nullptr)
        logFile << "[" << file << ":" << line << "] ";

    logFile << message << std::endl;
}
