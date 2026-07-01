#include "Logger.hpp"

#include <iostream>

void Logger::log(LogLevel level, const std::string& message)
{
    switch(level)
    {
        case LogLevel::INFO:
            std::cout << "[INFO] ";
            break;

        case LogLevel::WARNING:
            std::cout << "[WARNING] ";
            break;

        case LogLevel::ERROR:
            std::cout << "[ERROR] ";
            break;

        case LogLevel::DEBUG:
            std::cout << "[DEBUG] ";
            break;
    }

    std::cout << message << std::endl;
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