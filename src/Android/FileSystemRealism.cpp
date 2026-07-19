/**
 * @file FileSystemRealism.cpp
 * @brief File System Realism Implementation
 */

#include "Android/FileSystemRealism.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMutexLocker>
#include <QJsonObject>

namespace VirtualPhonePro {

FileSystemRealism* FileSystemRealism::s_instance = nullptr;

FileSystemRealism& FileSystemRealism::instance() {
    if (!s_instance) {
        s_instance = new FileSystemRealism();
    }
    return *s_instance;
}

FileSystemRealism::FileSystemRealism(QObject* parent)
    : QObject(parent)
    , m_randomInt(0, 1000000)
    , m_randomDouble(0.0, 1.0)
{
    // Initialize with random seed
    initializeRandomGenerator(QDateTime::currentMSecsSinceEpoch());
    
    // Set default config
    m_defaultConfig.populateDCIM = true;
    m_defaultConfig.populatePictures = true;
    m_defaultConfig.populateDownloads = true;
    m_defaultConfig.populateAndroidData = true;
    m_defaultConfig.populateDocuments = true;
    m_defaultConfig.populateWhatsApp = true;
    m_defaultConfig.populateOtherApps = true;
    
    m_defaultConfig.dcimPhotoCount = 50;
    m_defaultConfig.dcimVideoCount = 10;
    m_defaultConfig.picturesCount = 30;
    m_defaultConfig.downloadsFileCount = 20;
    m_defaultConfig.whatsappMediaCount = 50;
    
    m_defaultConfig.simulateUsage = true;
    m_defaultConfig.simulateSearchHistory = true;
    m_defaultConfig.simulateBrowserHistory = true;
    m_defaultConfig.simulateCallHistory = true;
    m_defaultConfig.simulateSMSHistory = true;
    
    m_defaultConfig.randomizeTimestamps = true;
    m_defaultConfig.realisticUsagePattern = true;
    
    m_defaultConfig.monthsActive = 12;
    m_defaultConfig.avgPhotosPerDay = 5;
    m_defaultConfig.avgAppsInstalled = 50;
}

FileSystemRealism::~FileSystemRealism() {
}

// ============================================================================
// Main Population
// ============================================================================

bool FileSystemRealism::populate(const QString& instanceId, const FileSystemConfig& config) {
    qDebug() << "Starting file system population for:" << instanceId;
    emit populationStarted(instanceId);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create state
    PopulationState* state = new PopulationState();
    state->isPopulated = false;
    state->config = config;
    state->filesCreated = 0;
    state->totalSize = 0;
    state->populationTime = QDateTime::currentMSecsSinceEpoch();
    
    {
        QMutexLocker locker(&m_mutex);
        m_instanceStates[instanceId] = state;
    }
    
    // Create directory structure
    QStringList directories = {
        "/sdcard/DCIM/Camera",
        "/sdcard/DCIM/Screenshots",
        "/sdcard/Pictures",
        "/sdcard/Pictures/Screenshots",
        "/sdcard/Pictures/WhatsApp",
        "/sdcard/Download",
        "/sdcard/Download/Images",
        "/sdcard/Download/Documents",
        "/sdcard/Download/Videos",
        "/sdcard/Android/data",
        "/sdcard/Documents",
        "/sdcard/Documents/WhatsApp",
    };
    
    int dirCount = 0;
    for (const QString& dir : directories) {
        if (createDirectory(instanceId, dir)) {
            dirCount++;
        }
    }
    
    qDebug() << "Created" << dirCount << "directories";
    
    // Populate DCIM
    if (config.populateDCIM) {
        populateDCIM(instanceId, config);
    }
    
    // Populate Pictures
    if (config.populatePictures) {
        // Download images would go here
    }
    
    // Populate Downloads
    if (config.populateDownloads) {
        populateDownloads(instanceId, config);
    }
    
    // Populate WhatsApp
    if (config.populateWhatsApp) {
        populateWhatsApp(instanceId, config);
    }
    
    // Populate Android Data
    if (config.populateAndroidData) {
        populateAndroidData(instanceId, config);
    }
    
    // Generate usage history
    if (config.simulateUsage) {
        QVector<AppUsage> apps;
        // Add common apps
        AppUsage whatsapp;
        whatsapp.packageName = "com.whatsapp";
        whatsapp.appName = "WhatsApp";
        whatsapp.launchCount = 200;
        whatsapp.totalUsageTimeMs = 3600000 * 50; // 50 hours
        whatsapp.lastUsedTime = QDateTime::currentMSecsSinceEpoch();
        whatsapp.firstInstallTime = QDateTime::currentDateTime().addMonths(-12);
        whatsapp.lastUpdateTime = QDateTime::currentDateTime().addDays(-7);
        apps.append(whatsapp);
        
        AppUsage messenger;
        messenger.packageName = "com.facebook.orca";
        messenger.appName = "Facebook Messenger";
        messenger.launchCount = 150;
        messenger.totalUsageTimeMs = 3600000 * 30;
        messenger.lastUsedTime = QDateTime::currentMSecsSinceEpoch() - 86400000;
        messenger.firstInstallTime = QDateTime::currentDateTime().addMonths(-10);
        messenger.lastUpdateTime = QDateTime::currentDateTime().addDays(-3);
        apps.append(messenger);
        
        generateUsageStats(instanceId, apps);
    }
    
    // Generate call history
    if (config.simulateCallHistory) {
        generateCallHistory(instanceId, 30);
    }
    
    // Generate SMS history
    if (config.simulateSMSHistory) {
        generateSMSHistory(instanceId, 30);
    }
    
    // Mark as populated
    state->isPopulated = true;
    state->populationTime = QDateTime::currentMSecsSinceEpoch() - state->populationTime;
    
    qDebug() << "File system population completed for" << instanceId
             << "- Files:" << state->filesCreated
             << "- Time:" << state->populationTime << "ms";
    
    emit populationCompleted(instanceId, state->filesCreated);
    
    return true;
}

bool FileSystemRealism::quickPopulate(const QString& instanceId) {
    return populate(instanceId, m_defaultConfig);
}

// ============================================================================
// Directory Population
// ============================================================================

void FileSystemRealism::populateDCIM(const QString& instanceId, const FileSystemConfig& config) {
    qDebug() << "Populating DCIM...";
    
    for (int i = 0; i < config.dcimPhotoCount; ++i) {
        generateCameraPhoto(instanceId, config);
        
        if (i % 10 == 0) {
            emit populationProgress(instanceId, (i * 100) / config.dcimPhotoCount);
        }
    }
    
    for (int i = 0; i < config.dcimVideoCount; ++i) {
        generateCameraVideo(instanceId, config);
    }
}

void FileSystemRealism::generateCameraPhoto(const QString& instanceId, const FileSystemConfig& config) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Generate random filename
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch() + m_randomInt(m_generator));
    QString filename = QString("IMG_%1.jpg").arg(timestamp.right(14));
    
    QString path = "/sdcard/DCIM/Camera/" + filename;
    
    // Generate synthetic EXIF data
    QStringList exifMakes = {"Apple", "Samsung", "Google", "Xiaomi", "OnePlus"};
    QStringList exifModels = {"iPhone 15 Pro", "Galaxy S24 Ultra", "Pixel 8 Pro", "Mi 14", "OnePlus 12"};
    
    QString exifMake = exifMakes[m_randomInt(m_generator) % exifMakes.size()];
    QString exifModel = exifModels[m_randomInt(m_generator) % exifModels.size()];
    
    // Random dimensions
    int widths[] = {4032, 4080, 4128, 4160, 4208};
    int heights[] = {3024, 3060, 3096, 3120, 3168};
    int width = widths[m_randomInt(m_generator) % 5];
    int height = heights[m_randomInt(m_generator) % 5];
    
    // Generate fake JPEG file with minimal valid header
    QByteArray fakeImage;
    
    // JPEG SOI marker
    fakeImage.append(static_cast<char>(0xFF));
    fakeImage.append(static_cast<char>(0xD8));
    
    // Add minimal APP1 (EXIF) marker
    QByteArray exifData = "Exif\x00\x00";
    exifData.append("II\x2A\x00"); // Little-endian TIFF header
    exifData.append(static_cast<char>(0x08)); // Offset to IFD0
    exifData.append(static_cast<char>(0x00));
    exifData.append(static_cast<char>(0x00));
    exifData.append(static_cast<char>(0x00));
    exifData.append(static_cast<char>(0x00));
    
    // Fill with some data to simulate real file
    int targetSize = 500000 + m_randomInt(m_generator) % 3000000; // 500KB - 3.5MB
    while (fakeImage.size() < targetSize) {
        fakeImage.append(static_cast<char>(m_randomInt(m_generator) % 256));
    }
    
    // JPEG EOI marker
    fakeImage.append(static_cast<char>(0xFF));
    fakeImage.append(static_cast<char>(0xD9));
    
    // Create temp file
    QString tempPath = "/tmp/" + filename;
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(fakeImage);
        tempFile.close();
        
        // Push to instance
        ctrl.pushFile(instanceId, tempPath, path);
        
        // Set timestamps
        QDateTime created = randomPastDate(config.monthsActive * 30);
        
        ctrl.executeShell(instanceId, 
            QString("touch -a -t %1 %2")
            .arg(created.toString("yyyyMMddhhmmss"))
            .arg(path));
        ctrl.executeShell(instanceId, 
            QString("touch -m -t %1 %2")
            .arg(created.toString("yyyyMMddhhmmss"))
            .arg(path));
        
        // Remove temp file
        tempFile.remove();
        
        // Update stats
        {
            QMutexLocker locker(&m_mutex);
            if (m_instanceStates.contains(instanceId)) {
                m_instanceStates[instanceId]->filesCreated++;
                m_instanceStates[instanceId]->totalSize += fakeImage.size();
            }
        }
    }
}

void FileSystemRealism::generateCameraVideo(const QString& instanceId, const FileSystemConfig& config) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch() + m_randomInt(m_generator));
    QString filename = QString("VID_%1.mp4").arg(timestamp.right(14));
    
    QString path = "/sdcard/DCIM/Camera/" + filename;
    
    // Create fake video file (MP4 header)
    QByteArray fakeVideo;
    
    // ftyp box
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append(static_cast<char>(0x18)); // Box size
    fakeVideo.append("ftyp");
    fakeVideo.append("isom");
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append(static_cast<char>(0x02));
    fakeVideo.append(static_cast<char>(0x00));
    fakeVideo.append("isomiso2mp41");
    
    // Fill with data
    int targetSize = 5000000 + m_randomInt(m_generator) % 20000000; // 5MB - 25MB
    while (fakeVideo.size() < targetSize) {
        fakeVideo.append(static_cast<char>(m_randomInt(m_generator) % 256));
    }
    
    QString tempPath = "/tmp/" + filename;
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(fakeVideo);
        tempFile.close();
        
        ctrl.pushFile(instanceId, tempPath, path);
        
        QDateTime created = randomPastDate(config.monthsActive * 30);
        ctrl.executeShell(instanceId, 
            QString("touch -t %1 %2")
            .arg(created.toString("yyyyMMddhhmmss"))
            .arg(path));
        
        tempFile.remove();
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_instanceStates.contains(instanceId)) {
                m_instanceStates[instanceId]->filesCreated++;
                m_instanceStates[instanceId]->totalSize += fakeVideo.size();
            }
        }
    }
}

void FileSystemRealism::populateDownloads(const QString& instanceId, const FileSystemConfig& config) {
    qDebug() << "Populating Downloads...";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create various file types
    QStringList extensions = {"pdf", "docx", "xlsx", "jpg", "png", "mp3", "mp4", "zip", "apk"};
    QStringList mimeTypes = {
        "application/pdf",
        "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
        "image/jpeg",
        "image/png",
        "audio/mpeg",
        "video/mp4",
        "application/zip",
        "application/vnd.android.package-archive"
    };
    
    for (int i = 0; i < config.downloadsFileCount; ++i) {
        int extIndex = m_randomInt(m_generator) % extensions.size();
        QString ext = extensions[extIndex];
        QString filename = QString("file_%1.%2").arg(i + 1).arg(ext);
        QString path = "/sdcard/Download/" + filename;
        
        // Generate file
        int size = 10000 + m_randomInt(m_generator) % 10000000;
        QByteArray data;
        while (data.size() < size) {
            data.append(static_cast<char>(m_randomInt(m_generator) % 256));
        }
        
        QString tempPath = "/tmp/" + filename;
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(data);
            tempFile.close();
            
            ctrl.pushFile(instanceId, tempPath, path);
            
            QDateTime created = randomPastDate(config.monthsActive * 30);
            ctrl.executeShell(instanceId, 
                QString("touch -t %1 %2")
                .arg(created.toString("yyyyMMddhhmmss"))
                .arg(path));
            
            tempFile.remove();
            
            {
                QMutexLocker locker(&m_mutex);
                if (m_instanceStates.contains(instanceId)) {
                    m_instanceStates[instanceId]->filesCreated++;
                    m_instanceStates[instanceId]->totalSize += size;
                }
            }
        }
    }
}

void FileSystemRealism::populateWhatsApp(const QString& instanceId, const FileSystemConfig& config) {
    qDebug() << "Populating WhatsApp...";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create WhatsApp directories
    QStringList whatsappDirs = {
        "/sdcard/Android/data/com.whatsapp",
        "/sdcard/Android/data/com.whatsapp/cache",
        "/sdcard/Android/data/com.whatsapp/files/Avatars",
        "/sdcard/WhatsApp/Media/WhatsApp Images",
        "/sdcard/WhatsApp/Media/WhatsApp Video",
        "/sdcard/WhatsApp/Media/WhatsApp Voice Notes",
    };
    
    for (const QString& dir : whatsappDirs) {
        createDirectory(instanceId, dir);
    }
    
    // Create media files
    for (int i = 0; i < config.whatsappMediaCount; ++i) {
        QString type = (i % 3 == 0) ? "Video" : "Images";
        QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch() - m_randomInt(m_generator) % 2592000000);
        QString filename = QString("WA%1%2.jpg")
            .arg(type == "Video" ? "V" : "")
            .arg(timestamp.right(12));
        QString path = QString("/sdcard/WhatsApp/Media/WhatsApp %1/").arg(type) + filename;
        
        // Create fake media
        QByteArray data;
        int size = 100000 + m_randomInt(m_generator) % 5000000;
        while (data.size() < size) {
            data.append(static_cast<char>(m_randomInt(m_generator) % 256));
        }
        
        QString tempPath = "/tmp/" + filename;
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(data);
            tempFile.close();
            
            ctrl.pushFile(instanceId, tempPath, path);
            tempFile.remove();
            
            {
                QMutexLocker locker(&m_mutex);
                if (m_instanceStates.contains(instanceId)) {
                    m_instanceStates[instanceId]->filesCreated++;
                    m_instanceStates[instanceId]->totalSize += size;
                }
            }
        }
    }
}

void FileSystemRealism::populateAndroidData(const QString& instanceId, const FileSystemConfig& config) {
    qDebug() << "Populating Android data...";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create data directories for common apps
    QStringList appDirs = {
        "/sdcard/Android/data/com.google.android.gms/files",
        "/sdcard/Android/data/com.facebook.katana/cache",
        "/sdcard/Android/data/com.instagram.android/cache",
        "/sdcard/Android/data/com.snapchat.android/cache",
        "/sdcard/Android/data/com.twitter.android/cache",
        "/sdcard/Android/data/com.google.android.youtube/cache",
    };
    
    for (const QString& dir : appDirs) {
        createDirectory(instanceId, dir);
        
        // Create a few dummy files
        for (int i = 0; i < 3; ++i) {
            QString filename = QString("cache_%1.tmp").arg(i);
            QString path = dir + "/" + filename;
            
            QByteArray data;
            int size = 10000 + m_randomInt(m_generator) % 100000;
            while (data.size() < size) {
                data.append(static_cast<char>(m_randomInt(m_generator) % 256));
            }
            
            QString tempPath = "/tmp/" + filename;
            QFile tempFile(tempPath);
            if (tempFile.open(QIODevice::WriteOnly)) {
                tempFile.write(data);
                tempFile.close();
                
                ctrl.pushFile(instanceId, tempPath, path);
                tempFile.remove();
                
                {
                    QMutexLocker locker(&m_mutex);
                    if (m_instanceStates.contains(instanceId)) {
                        m_instanceStates[instanceId]->filesCreated++;
                    }
                }
            }
        }
    }
}

// ============================================================================
// Usage History
// ============================================================================

bool FileSystemRealism::generateUsageHistory(const QString& instanceId, const QVector<AppUsage>& apps) {
    return generateUsageStats(instanceId, apps);
}

bool FileSystemRealism::generateUsageStats(const QString& instanceId, const QVector<AppUsage>& apps) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Generate usage stats files
    for (const AppUsage& app : apps) {
        // Create usage stats file
        QString content = QString(
            "package=%1\n"
            "app_name=%2\n"
            "launch_count=%3\n"
            "total_time=%4\n"
            "last_time_used=%5\n"
            "first_install_time=%6\n"
            "last_update_time=%7\n"
        ).arg(app.packageName)
         .arg(app.appName)
         .arg(app.launchCount)
         .arg(app.totalUsageTimeMs)
         .arg(app.lastUsedTime)
         .arg(app.firstInstallTime.toMSecsSinceEpoch())
         .arg(app.lastUpdateTime.toMSecsSinceEpoch());
        
        QString path = QString("/data/system/usagestats/%1/usage")
            .arg(app.packageName);
        
        ctrl.executeShell(instanceId, "mkdir -p /data/system/usagestats/" + app.packageName);
        
        // Write file via cat
        QString tempPath = "/tmp/usagestats_" + QString::number(m_randomInt(m_generator));
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(content.toUtf8());
            tempFile.close();
            
            ctrl.pushFile(instanceId, tempPath, path);
            tempFile.remove();
        }
    }
    
    return true;
}

bool FileSystemRealism::generateCallHistory(const QString& instanceId, int days) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create call log database
    QString dbContent = "-- Call log simulation\n";
    
    QStringList names = {"John Doe", "Jane Smith", "Office", "Mom", "Dad", "Boss", "Unknown"};
    
    for (int i = 0; i < days * 5; ++i) {
        QDateTime callTime = randomPastDate(days);
        QString name = names[m_randomInt(m_generator) % names.size()];
        QString number = QString("+1%1").arg(1000000000 + m_randomInt(m_generator) % 900000000);
        int duration = 30 + m_randomInt(m_generator) % 600;
        int type = m_randomInt(m_generator) % 3; // 0=incoming, 1=outgoing, 2=missed
        
        dbContent += QString("%1|%2|%3|%4|%5\n")
            .arg(callTime.toMSecsSinceEpoch())
            .arg(name)
            .arg(number)
            .arg(duration)
            .arg(type);
    }
    
    QString tempPath = "/tmp/calllog.txt";
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(dbContent.toUtf8());
        tempFile.close();
        
        ctrl.pushFile(instanceId, tempPath, "/data/data/com.android.providers.telephony/databases/calllog.db");
        tempFile.remove();
    }
    
    return true;
}

bool FileSystemRealism::generateSMSHistory(const QString& instanceId, int days) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString dbContent = "-- SMS simulation\n";
    
    QStringList messages = {
        "Hey, how are you?",
        "Can you call me back?",
        "Meeting at 3pm",
        "See you later!",
        "Thanks for the info",
        "OK, sounds good",
        "What's up?"
    };
    
    for (int i = 0; i < days * 10; ++i) {
        QDateTime msgTime = randomPastDate(days);
        QString address = QString("+1%1").arg(1000000000 + m_randomInt(m_generator) % 900000000);
        QString body = messages[m_randomInt(m_generator) % messages.size()];
        int type = m_randomInt(m_generator) % 2; // 0=sent, 1=received
        
        dbContent += QString("%1|%2|%3|%4\n")
            .arg(msgTime.toMSecsSinceEpoch())
            .arg(address)
            .arg(type)
            .arg(body);
    }
    
    QString tempPath = "/tmp/sms.db";
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(dbContent.toUtf8());
        tempFile.close();
        
        ctrl.pushFile(instanceId, tempPath, "/data/data/com.android.providers.telephony/databases/mmssms.db");
        tempFile.remove();
    }
    
    return true;
}

bool FileSystemRealism::generateBrowserHistory(const QString& instanceId, int days) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString historyContent = "-- Browser history simulation\n";
    
    QStringList urls = {
        "https://www.google.com",
        "https://www.facebook.com",
        "https://www.youtube.com",
        "https://www.twitter.com",
        "https://www.instagram.com",
        "https://www.reddit.com",
        "https://www.amazon.com",
        "https://www.netflix.com",
        "https://www.gmail.com",
        "https://news.ycombinator.com"
    };
    
    for (int i = 0; i < days * 20; ++i) {
        QDateTime visitTime = randomPastDate(days);
        QString url = urls[m_randomInt(m_generator) % urls.size()];
        
        historyContent += QString("%1|%2\n")
            .arg(visitTime.toMSecsSinceEpoch())
            .arg(url);
    }
    
    QString tempPath = "/tmp/history.db";
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(historyContent.toUtf8());
        tempFile.close();
        
        ctrl.pushFile(instanceId, tempPath, "/data/data/com.android.browser/databases/browser.db");
        tempFile.remove();
    }
    
    return true;
}

// ============================================================================
// Utility Methods
// ============================================================================

bool FileSystemRealism::createDirectory(const QString& instanceId, const QString& path) {
    ReDroidController& ctrl = ReDroidController::instance();
    QString result = ctrl.executeShell(instanceId, "mkdir -p " + path);
    return result.isEmpty() || !result.contains("error");
}

bool FileSystemRealism::addFile(const QString& instanceId, const SyntheticFile& file) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (file.isDirectory) {
        return createDirectory(instanceId, file.path);
    }
    
    // Create file
    QByteArray data;
    while (data.size() < file.size) {
        data.append(static_cast<char>(m_randomInt(m_generator) % 256));
    }
    
    QString tempPath = "/tmp/temp_" + generateRandomString(8);
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(data);
        tempFile.close();
        
        bool success = ctrl.pushFile(instanceId, tempPath, file.path);
        tempFile.remove();
        
        if (success) {
            setFileTimestamps(instanceId, file.path, file);
        }
        
        return success;
    }
    
    return false;
}

bool FileSystemRealism::addAppData(const QString& instanceId, const QString& packageName, int dataSize) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString dir = "/sdcard/Android/data/" + packageName;
    createDirectory(instanceId, dir);
    
    // Create some dummy files
    for (int i = 0; i < 5; ++i) {
        QString filename = QString("data_%1.cache").arg(i);
        QString path = dir + "/" + filename;
        
        QByteArray data;
        int size = dataSize / 5;
        while (data.size() < size) {
            data.append(static_cast<char>(m_randomInt(m_generator) % 256));
        }
        
        QString tempPath = "/tmp/" + filename;
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(data);
            tempFile.close();
            
            ctrl.pushFile(instanceId, tempPath, path);
            tempFile.remove();
        }
    }
    
    return true;
}

bool FileSystemRealism::setFileTimestamps(const QString& instanceId, const QString& path, const SyntheticFile& file) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString accessTime = file.accessedAt.toString("yyyyMMddhhmmss");
    QString modifyTime = file.modifiedAt.toString("yyyyMMddhhmmss");
    
    ctrl.executeShell(instanceId, QString("touch -a -t %1 %2").arg(accessTime).arg(path));
    ctrl.executeShell(instanceId, QString("touch -m -t %1 %2").arg(modifyTime).arg(path));
    
    return true;
}

void FileSystemRealism::initializeRandomGenerator(qint64 seed) {
    m_generator.seed(seed);
}

QString FileSystemRealism::generateRandomString(int length) {
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString result;
    
    for (int i = 0; i < length; ++i) {
        result.append(chars[m_randomInt(m_generator) % chars.size()]);
    }
    
    return result;
}

QDateTime FileSystemRealism::randomPastDate(int maxDaysAgo) {
    int secondsAgo = m_randomInt(m_generator) % (maxDaysAgo * 86400);
    return QDateTime::currentDateTime().addSecs(-secondsAgo);
}

QString FileSystemRealism::generateContentHash(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        return QString(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    }
    return QString();
}

// ============================================================================
// Configuration
// ============================================================================

void FileSystemRealism::setConfig(const FileSystemConfig& config) {
    m_defaultConfig = config;
}

FileSystemConfig FileSystemRealism::getConfig() const {
    return m_defaultConfig;
}

FileSystemConfig FileSystemRealism::getDefaultConfig() const {
    return m_defaultConfig;
}

// ============================================================================
// Status
// ============================================================================

bool FileSystemRealism::isPopulated(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    return m_instanceStates.contains(instanceId) && m_instanceStates[instanceId]->isPopulated;
}

QJsonObject FileSystemRealism::getStats(const QString& instanceId) const {
    QJsonObject stats;
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_instanceStates.contains(instanceId)) {
        return stats;
    }
    
    PopulationState* state = m_instanceStates[instanceId];
    
    stats["isPopulated"] = state->isPopulated;
    stats["filesCreated"] = state->filesCreated;
    stats["totalSizeBytes"] = state->totalSize;
    stats["totalSizeMB"] = state->totalSize / (1024.0 * 1024.0);
    stats["populationTimeMs"] = state->populationTime;
    
    QJsonObject config;
    config["dcimPhotos"] = state->config.dcimPhotoCount;
    config["dcimVideos"] = state->config.dcimVideoCount;
    config["downloads"] = state->config.downloadsFileCount;
    config["whatsappMedia"] = state->config.whatsappMediaCount;
    stats["config"] = config;
    
    return stats;
}

} // namespace VirtualPhonePro
