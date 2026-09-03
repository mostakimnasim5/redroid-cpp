/**
 * @file ConfigManager.h
 * @brief Configuration Manager - Handles loading/saving app config
 * @version 2.0.0
 * 
 * SECURITY: Sensitive values (API keys) are loaded from config file
 * instead of being hardcoded in source code.
 */

#ifndef VIRTUALPHONEPRO_CONFIG_MANAGER_HPP
#define VIRTUALPHONEPRO_CONFIG_MANAGER_HPP

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QtGlobal>

namespace VirtualPhonePro {

/**
 * @brief Configuration Manager Singleton
 * 
 * Manages application configuration including:
 * - Firebase credentials (API keys)
 * - Docker/ADB paths
 * - Server settings
 * 
 * Config file location: %APPDATA%/RedroidCPP/config.json
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();
    
    // Config file paths
    QString getConfigFilePath() const;
    QString getConfigDir() const;
    
    // Firebase config
    // The web API key is public by design (it only identifies the project;
    // access is controlled by Firestore rules), so it ships compiled-in like
    // in the Mainadmin web panel. End users never configure anything.
    // Environment variables, then the config file, override these defaults.
    static QString defaultFirebaseProjectId() { return QStringLiteral("redroid-d8110"); }
    static QString defaultFirebaseApiKey() { return QStringLiteral("AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA"); }

    QString getFirebaseProjectId() const {
        const QString env = qEnvironmentVariable("REDROID_FB_PROJECT_ID");
        if (!env.isEmpty()) return env;
        const QString cfg = m_config["firebase"]["projectId"].toString();
        return cfg.isEmpty() ? defaultFirebaseProjectId() : cfg;
    }
    QString getFirebaseApiKey() const {
        const QString env = qEnvironmentVariable("REDROID_FB_API_KEY");
        if (!env.isEmpty()) return env;
        const QString cfg = m_config["firebase"]["apiKey"].toString();
        return cfg.isEmpty() ? defaultFirebaseApiKey() : cfg;
    }
    QString getFirebaseBaseUrl() const;

    // Writes firebase.projectId/apiKey to the config file and returns the
    // result of saveConfig(). Environment variables still take precedence
    // over these values at read time.
    bool setFirebaseConfig(const QString& projectId, const QString& apiKey);

    // API config
    int getServerPort() const { return m_config["api"]["serverPort"].toInt(8080); }
    QString getApiKey() const { return m_config["api"]["apiKey"].toString(); }
    
    // Docker config
    QString getDockerPath() const { return m_config["docker"]["path"].toString("docker"); }
    QString getDockerHost() const { return m_config["docker"]["host"].toString(); }
    int getDockerTimeout() const { return m_config["docker"]["timeout"].toInt(30); }
    
    // ADB config
    QString getAdbPath() const { return m_config["adb"]["path"].toString("adb"); }
    int getAdbPortStart() const { return m_config["adb"]["portStart"].toInt(5555); }
    bool getAutoAdbConnect() const { return m_config["adb"]["autoConnect"].toBool(true); }
    
    // Config management
    bool loadConfig();
    bool saveConfig();
    bool resetToDefaults();
    bool hasFirebaseConfig() const;
    
    // Direct access to full config
    QJsonObject getConfig() const { return m_config; }
    void setConfig(const QJsonObject& config);
    
signals:
    void configLoaded();
    void configSaved();

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();
    Q_DISABLE_COPY(ConfigManager)
    
    bool createDefaultConfig();
    
    QJsonObject m_config;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_CONFIG_MANAGER_HPP
