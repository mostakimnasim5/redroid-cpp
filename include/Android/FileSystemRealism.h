/**
 * @file FileSystemRealism.h
 * @brief File System Realism - Synthetic File Population
 * @version 2.0.0
 * 
 * Pre-populates Android virtual SD card with realistic files and metadata
 * to simulate a real human-used phone.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_FILE_SYSTEM_REALISM_H
#define VIRTUALPHONEPRO_FILE_SYSTEM_REALISM_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QDateTime>
#include <QMutex>

namespace VirtualPhonePro {

// File entry for synthetic population
struct SyntheticFile {
    QString path;                // Full path
    QString name;               // File name
    QString mimeType;            // MIME type
    qint64 size;                // File size in bytes
    QDateTime createdAt;        // Creation timestamp
    QDateTime modifiedAt;      // Modification timestamp
    QDateTime accessedAt;      // Last access timestamp
    bool isDirectory;
    
    // For images
    QString exifMake;           // Camera manufacturer
    QString exifModel;          // Camera model
    QString exifDateTime;       // Date taken
    int exifWidth;
    int exifHeight;
    
    // For videos
    int videoDuration;          // Duration in seconds
    
    // For documents
    QString documentTitle;
    QString documentAuthor;
    
    // Content hash
    QString contentHash;
};

// Configuration for file system population
struct FileSystemConfig {
    // Population settings
    bool populateDCIM;
    bool populatePictures;
    bool populateDownloads;
    bool populateAndroidData;
    bool populateDocuments;
    bool populateWhatsApp;
    bool populateOtherApps;
    
    // File counts
    int dcimPhotoCount;
    int dcimVideoCount;
    int picturesCount;
    int downloadsFileCount;
    int whatsappMediaCount;
    
    // Usage simulation
    bool simulateUsage;         // Add app usage history
    bool simulateSearchHistory;
    bool simulateBrowserHistory;
    bool simulateCallHistory;
    bool simulateSMSHistory;
    
    // Time distribution
    bool randomizeTimestamps;  // Random dates over past year
    bool realisticUsagePattern; // More files in recent months
    
    // Device usage profile
    int monthsActive;          // How many months the device has been used
    int avgPhotosPerDay;
    int avgAppsInstalled;
};

// App to simulate usage for
struct AppUsage {
    QString packageName;
    QString appName;
    int launchCount;
    qint64 totalUsageTimeMs;
    qint64 lastUsedTime;
    QDateTime firstInstallTime;
    QDateTime lastUpdateTime;
};

class FileSystemRealism : public QObject {
    Q_OBJECT

public:
    static FileSystemRealism& instance();
    
    // =========================================================================
    // Population
    // =========================================================================
    
    /**
     * @brief Populate file system for instance
     * @param instanceId Target instance
     * @param config Configuration
     * @return true if successful
     */
    bool populate(const QString& instanceId, const FileSystemConfig& config);
    
    /**
     * @brief Quick populate with defaults
     */
    bool quickPopulate(const QString& instanceId);
    
    // =========================================================================
    // Selective Population
    // =========================================================================
    
    /**
     * @brief Populate specific directory
     */
    bool populateDirectory(const QString& instanceId, const QString& directory, const FileSystemConfig& config);
    
    /**
     * @brief Add individual file
     */
    bool addFile(const QString& instanceId, const SyntheticFile& file);
    
    /**
     * @brief Add app data
     */
    bool addAppData(const QString& instanceId, const QString& packageName, int dataSize);
    
    // =========================================================================
    // Usage History
    // =========================================================================
    
    /**
     * @brief Generate usage history
     */
    bool generateUsageHistory(const QString& instanceId, const QVector<AppUsage>& apps);
    
    /**
     * @brief Generate call history
     */
    bool generateCallHistory(const QString& instanceId, int days);
    
    /**
     * @brief Generate SMS history
     */
    bool generateSMSHistory(const QString& instanceId, int days);
    
    /**
     * @brief Generate browser history
     */
    bool generateBrowserHistory(const QString& instanceId, int days);
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set configuration
     */
    void setConfig(const FileSystemConfig& config);
    
    /**
     * @brief Get current config
     */
    FileSystemConfig getConfig() const;
    
    /**
     * @brief Get default config
     */
    FileSystemConfig getDefaultConfig() const;
    
    // =========================================================================
    // Status
    // =========================================================================
    
    /**
     * @brief Check if populated
     */
    bool isPopulated(const QString& instanceId) const;
    
    /**
     * @brief Get population stats
     */
    QJsonObject getStats(const QString& instanceId) const;

signals:
    void populationStarted(const QString& instanceId);
    void populationProgress(const QString& instanceId, int percent);
    void populationCompleted(const QString& instanceId, int filesCreated);
    void error(const QString& instanceId, const QString& message);

private:
    FileSystemRealism(QObject* parent = nullptr);
    ~FileSystemRealism();
    Q_DISABLE_COPY(FileSystemRealism)
    
    // Internal methods
    void initializeRandomGenerator(qint64 seed);
    QString generateRandomString(int length);
    QDateTime randomPastDate(int maxDaysAgo);
    QString generateContentHash(const QString& path);
    
    // Population helpers
    bool createDirectory(const QString& instanceId, const QString& path);
    bool createFile(const QString& instanceId, const SyntheticFile& file);
    bool setFileTimestamps(const QString& instanceId, const QString& path, const SyntheticFile& file);
    
    // DCIM population
    void populateDCIM(const QString& instanceId, const FileSystemConfig& config);
    void generateCameraPhoto(const QString& instanceId, const FileSystemConfig& config);
    void generateCameraVideo(const QString& instanceId, const FileSystemConfig& config);
    
    // Downloads population
    void populateDownloads(const QString& instanceId, const FileSystemConfig& config);
    
    // WhatsApp population
    void populateWhatsApp(const QString& instanceId, const FileSystemConfig& config);
    
    // Android data population
    void populateAndroidData(const QString& instanceId, const FileSystemConfig& config);
    
    // App usage generation
    void generateUsageStats(const QString& instanceId, const QVector<AppUsage>& apps);
    
    // Instance state
    struct PopulationState {
        bool isPopulated;
        FileSystemConfig config;
        int filesCreated;
        qint64 totalSize;
        qint64 populationTime;
    };
    
    QMap<QString, PopulationState*> m_instanceStates;
    FileSystemConfig m_defaultConfig;
    mutable QMutex m_mutex;
    
    // Random generation
    std::mt19937 m_generator;
    std::uniform_int_distribution<int> m_randomInt;
    std::uniform_real_distribution<double> m_randomDouble;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_FILE_SYSTEM_REALISM_H
