#ifndef NECRO_LOGGER_H
#define NECRO_LOGGER_H

#include <iostream>
#include <string>
#include <mutex>

#include "Utility.h"

//---------------------------------------------------------------------------
// Simple abstract Logger class, used by Console/File Loggers implementations
//---------------------------------------------------------------------------
class Logger
{
public:
    enum LogLevel
    {
        LOG_LEVEL_INFO = 0,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_OKSTATUS,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_CRITICAL
    };

protected:
    std::mutex lMutex;
    std::string GetLogLevelStr(LogLevel level);

public:
    virtual void Log(const std::string& message, LogLevel level, const char* file, int line) = 0;

#define cLog ConsoleLogger::Instance()
#define fLog FileLogger::Instance()

    // LOG uses the default Loggers instances, cLog and fLog (consoleLog, fileLog), by default logging on the console will also log on the file
#define LOG(logger, level, message) (logger)->Log(message, level, __FILE__, __LINE__)
#define LOG_INFO(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_INFO, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_INFO, message)
#define LOG_OK(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message)
#define LOG_DEBUG(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_DEBUG, message)
#define LOG_WARNING(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_WARNING, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_WARNING, message)
#define LOG_ERROR(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_ERROR, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_ERROR, message)
#define LOG_CRITICAL(message) LOG(cLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message); LOG(fLog, Logger::LogLevel::LOG_LEVEL_CRITICAL, message)

// S(PECIFIC) Log, allows to call log on a specific Logger object, may be useful for Daily loggers
#define SLOG(logger, level, message) (logger).Log(message, level, __FILE__, __LINE__)
#define SLOG_INFO(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_INFO, message)
#define SLOG_OK(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_OKSTATUS, message)
#define SLOG_DEBUG(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_DEBUG, message)
#define SLOG_WARNING(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_WARNING, message)
#define SLOG_ERROR(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_ERROR, message)
#define SLOG_CRITICAL(logger, message) LOG(logger, Logger::LogLevel::LOG_LEVEL_CRITICAL, message)
};

#endif
