/**
 * @file Logger.cpp
 * @brief Logger Implementation
 */

#include "VirtualPhonePro/Logger.hpp"
#include <thread>

namespace VirtualPhonePro {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_minLevel(LogLevel::DEBUG)
    , m_outputStream(&std::cout)
    , m_enableTimestamp(true)
    , m_enableThreadId(false)
    , m_enableColors(true)
{
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::getThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::getColor(LogLevel level) {
    if (!m_enableColors) return "";
    
    switch (level) {
        case LogLevel::DEBUG:    return "\033[36m";   // Cyan
        case LogLevel::INFO:     return "\033[32m";   // Green
        case LogLevel::WARNING:  return "\033[33m";   // Yellow
        case LogLevel::ERROR:    return "\033[31m";   // Red
        case LogLevel::CRITICAL: return "\033[35m";   // Magenta
        default:                 return "\033[0m";
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    std::stringstream ss;
    
    if (m_enableTimestamp) {
        ss << "[" << getCurrentTimestamp() << "] ";
    }
    
    ss << "[" << levelToString(level) << "] ";
    
    if (m_enableThreadId) {
        ss << "[Thread-" << getThreadId() << "] ";
    }
    
    ss << message;
    
    if (m_enableColors) {
        ss << "\033[0m"; // Reset color
    }
    
    return ss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string formatted = formatMessage(level, message);
    
    if (m_outputStream) {
        *m_outputStream << formatted << std::endl;
        m_outputStream->flush();
    }
    
    // Notify callbacks
    for (const auto& callback : m_callbacks) {
        callback(level, message, formatted);
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

void Logger::setOutputStream(std::ostream& stream) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outputStream = &stream;
}

void Logger::enableTimestamp(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enableTimestamp = enable;
}

void Logger::enableThreadId(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enableThreadId = enable;
}

void Logger::enableColors(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enableColors = enable;
}

void Logger::addCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back(callback);
}

} // namespace VirtualPhonePro
