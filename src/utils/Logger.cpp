#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
}

void Logger::log(LogLevel level, const std::string& message)
{
    const char* levelName = "[INFO]";
    switch (level)
    {
        case LogLevel::WARNING: levelName = "[WARN]"; break;
        case LogLevel::ERROR: levelName = "[ERROR]"; break;
        case LogLevel::DEBUG: levelName = "[DEBUG]"; break;
        default: break;
    }

    std::cout << timestamp() << levelName << message << std::endl;
}

void Logger::info(const std::string& message)
{
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::ERROR, message);
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::DEBUG, message);
}