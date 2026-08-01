/**
 * @file FileLogger.h
 * @brief Qt-based persistent file logger
 * @version 2.0.0
 */

#ifndef VIRTUALPHONEPRO_FILE_LOGGER_H
#define VIRTUALPHONEPRO_FILE_LOGGER_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

namespace VirtualPhonePro {

/**
 * @brief Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Thread-safe persistent file logger
 */
class FileLogger : public QObject {
    Q_OBJECT

public:
    static FileLogger& instance();
    
    void debug(const QString& module, const QString& message);
    void info(const QString& module, const QString& message);
    void warning(const QString& module, const QString& message);
    void error(const QString& module, const QString& message);
    void critical(const QString& module, const QString& message);
    
    void log(LogLevel level, const QString& module, const QString& message);
    
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);
    void setLogToConsole(bool enabled);
    void setMaxFileSize(qint64 maxBytes);
    void setMaxBackupFiles(int count);
    
    QString getLogFilePath() const { return m_logFilePath; }
    bool rotateLogFile();
    void clearOldLogs();
    QString getRecentLogs(int lines = 100) const;

signals:
    void logMessageReceived(const QString& message, LogLevel level);

private:
    explicit FileLogger();
    ~FileLogger();
    Q_DISABLE_COPY(FileLogger)
    
    void writeToFile(const QString& message);
    void openLogFile();
    void closeLogFile();
    static QString logLevelToString(LogLevel level);
    
    static FileLogger* s_instance;
    
    QFile* m_logFile;
    QTextStream* m_logStream;
    QString m_logFilePath;
    QMutex m_fileMutex;
    
    LogLevel m_minLevel;
    bool m_logToFile;
    bool m_logToConsole;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
    qint64 m_currentFileSize;
};

// Convenience macros
#define FILE_LOG_DEBUG(module, msg) VirtualPhonePro::FileLogger::instance().debug(module, msg)
#define FILE_LOG_INFO(module, msg) VirtualPhonePro::FileLogger::instance().info(module, msg)
#define FILE_LOG_WARNING(module, msg) VirtualPhonePro::FileLogger::instance().warning(module, msg)
#define FILE_LOG_ERROR(module, msg) VirtualPhonePro::FileLogger::instance().error(module, msg)
#define FILE_LOG_CRITICAL(module, msg) VirtualPhonePro::FileLogger::instance().critical(module, msg)

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_FILE_LOGGER_H
