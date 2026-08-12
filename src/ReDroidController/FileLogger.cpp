/**
 * @file FileLogger.cpp
 * @brief Qt-based persistent file logger
 * @version 2.0.0
 */

#include "VirtualPhonePro/FileLogger.hpp"
#include <QDebug>
#include <QFileInfo>
#include <QThread>

namespace VirtualPhonePro {

FileLogger& FileLogger::instance() {
    static FileLogger s_instance;
    return s_instance;
}

FileLogger::FileLogger()
    : m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_minLevel(LogLevel::INFO)
    , m_logToFile(true)
    , m_logToConsole(true)
    , m_maxFileSize(10 * 1024 * 1024) // 10MB
    , m_maxBackupFiles(5)
    , m_currentFileSize(0)
{
    // Set default log path
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    logDir += "/logs";
    QDir().mkpath(logDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    m_logFilePath = QString("%1/RedroidCPP_%2.log").arg(logDir).arg(timestamp);
    
    openLogFile();
    
    // Log startup
    info("FileLogger", "FileLogger initialized");
    info("FileLogger", QString("Log file: %1").arg(m_logFilePath));
}

FileLogger::~FileLogger() {
    closeLogFile();
}

void FileLogger::log(LogLevel level, const QString& module, const QString& message) {
    if (level < m_minLevel) return;
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString formattedMessage = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(logLevelToString(level))
        .arg(module)
        .arg(message);
    
    // Write to file (thread-safe)
    if (m_logToFile) {
        QMutexLocker locker(&m_fileMutex);
        writeToFile(formattedMessage);
    }
    
    // Write to console
    if (m_logToConsole) {
        QString consoleMsg = QString("[%1] %2").arg(module).arg(message);
        switch (level) {
            case LogLevel::DEBUG:
                qDebug().noquote() << consoleMsg;
                break;
            case LogLevel::INFO:
                qDebug().noquote() << consoleMsg;
                break;
            case LogLevel::WARNING:
                qWarning().noquote() << consoleMsg;
                break;
            case LogLevel::ERROR:
            case LogLevel::CRITICAL:
                qCritical().noquote() << consoleMsg;
                break;
        }
    }
    
    // Emit signal for UI
    emit logMessageReceived(formattedMessage, level);
}

void FileLogger::debug(const QString& module, const QString& message) {
    log(LogLevel::DEBUG, module, message);
}

void FileLogger::info(const QString& module, const QString& message) {
    log(LogLevel::INFO, module, message);
}

void FileLogger::warning(const QString& module, const QString& message) {
    log(LogLevel::WARNING, module, message);
}

void FileLogger::error(const QString& module, const QString& message) {
    log(LogLevel::ERROR, module, message);
}

void FileLogger::critical(const QString& module, const QString& message) {
    log(LogLevel::CRITICAL, module, message);
}

void FileLogger::writeToFile(const QString& message) {
    if (!m_logFile || !m_logStream) return;
    
    // Check file size and rotate if needed
    if (m_currentFileSize >= m_maxFileSize) {
        m_logStream->flush();
        closeLogFile();
        rotateLogFile();
        openLogFile();
    }
    
    *m_logStream << message << "\n";
    m_logStream->flush();
    m_currentFileSize += message.length() + 1;
}

void FileLogger::openLogFile() {
    if (m_logFile) return;
    
    m_logFile = new QFile(m_logFilePath);
    
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream = new QTextStream(m_logFile);
        m_currentFileSize = m_logFile->size();
    } else {
        qWarning() << "Failed to open log file:" << m_logFilePath;
        delete m_logFile;
        m_logFile = nullptr;
    }
}

void FileLogger::closeLogFile() {
    if (m_logStream) {
        m_logStream->flush();
        delete m_logStream;
        m_logStream = nullptr;
    }
    
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

bool FileLogger::rotateLogFile() {
    closeLogFile();
    
    QString basePath = m_logFilePath;
    int dotIndex = basePath.lastIndexOf('.');
    if (dotIndex > 0) {
        basePath = basePath.left(dotIndex);
    }
    
    // Remove oldest backup
    QString oldestBackup = QString("%1_%2.log").arg(basePath).arg(m_maxBackupFiles);
    if (QFile::exists(oldestBackup)) {
        QFile::remove(oldestBackup);
    }
    
    // Shift existing backups
    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString oldName = QString("%1_%2.log").arg(basePath).arg(i);
        QString newName = QString("%1_%2.log").arg(basePath).arg(i + 1);
        if (QFile::exists(oldName)) {
            QFile::rename(oldName, newName);
        }
    }
    
    // Rename current to 1.log
    if (QFile::exists(m_logFilePath)) {
        QString backupName = QString("%1_1.log").arg(basePath);
        QFile::rename(m_logFilePath, backupName);
    }
    
    return true;
}

void FileLogger::clearOldLogs() {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    logDir += "/logs";
    
    QDir dir(logDir);
    QStringList oldLogs = dir.entryList({"*_*.log"}, QDir::Files);
    
    int maxAgeDays = 7;
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-maxAgeDays);
    
    for (const QString& file : oldLogs) {
        QString fullPath = dir.filePath(file);
        QFileInfo info(fullPath);
        if (info.lastModified() < cutoff) {
            QFile::remove(fullPath);
        }
    }
}

QString FileLogger::getRecentLogs(int lines) const {
    QString result;
    
    if (!QFile::exists(m_logFilePath)) {
        return "No log file found.";
    }
    
    QFile file(m_logFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "Failed to read log file.";
    }
    
    QStringList allLines;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        allLines.append(stream.readLine());
    }
    file.close();
    
    int start = qMax(0, allLines.size() - lines);
    for (int i = start; i < allLines.size(); ++i) {
        result += allLines[i] + "\n";
    }
    
    return result;
}

void FileLogger::setLogLevel(LogLevel level) {
    m_minLevel = level;
    info("FileLogger", QString("Log level set to: %1").arg(logLevelToString(level)));
}

void FileLogger::setLogToFile(bool enabled) {
    m_logToFile = enabled;
}

void FileLogger::setLogToConsole(bool enabled) {
    m_logToConsole = enabled;
}

void FileLogger::setMaxFileSize(qint64 maxBytes) {
    m_maxFileSize = maxBytes;
}

void FileLogger::setMaxBackupFiles(int count) {
    m_maxBackupFiles = count;
}

QString FileLogger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace VirtualPhonePro
