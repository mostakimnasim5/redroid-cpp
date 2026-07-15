/**
 * @file Logger.hpp
 * @brief Centralized Logging System
 * @version 3.0.0
 * 
 * Thread-safe singleton logger for the ReDroidCPP application.
 * Provides debug, info, warning, error logging levels.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_LOGGER_HPP
#define VIRTUALPHONEPRO_LOGGER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <memory>
#include <vector>
#include <functional>

namespace VirtualPhonePro {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    static Logger& getInstance();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Logging methods
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // Log with level
    void log(LogLevel level, const std::string& message);
    
    // Configuration
    void setLogLevel(LogLevel level);
    void setOutputStream(std::ostream& stream);
    void enableTimestamp(bool enable);
    void enableThreadId(bool enable);
    void enableColors(bool enable);
    
    // Callback for log events
    using LogCallback = std::function<void(LogLevel, const std::string&, const std::string&)>;
    void addCallback(LogCallback callback);
    
private:
    Logger();
    ~Logger() = default;
    
    std::string formatMessage(LogLevel level, const std::string& message);
    std::string getCurrentTimestamp();
    std::string getThreadId();
    std::string levelToString(LogLevel level);
    std::string getColor(LogLevel level);
    
    LogLevel m_minLevel;
    std::ostream* m_outputStream;
    bool m_enableTimestamp;
    bool m_enableThreadId;
    bool m_enableColors;
    std::mutex m_mutex;
    std::vector<LogCallback> m_callbacks;
};

// Convenience macros
#define LOG_DEBUG(msg) VirtualPhonePro::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) VirtualPhonePro::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) VirtualPhonePro::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) VirtualPhonePro::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) VirtualPhonePro::Logger::getInstance().critical(msg)

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_LOGGER_HPP
