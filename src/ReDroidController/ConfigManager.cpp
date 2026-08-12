/**
 * @file ConfigManager.cpp
 * @brief Configuration Manager Implementation
 */

#include "VirtualPhonePro/ConfigManager.hpp"
#include <QDebug>
#include <QCoreApplication>

namespace VirtualPhonePro {

ConfigManager& ConfigManager::instance() {
    static ConfigManager s_instance;
    return s_instance;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    loadConfig();
}

ConfigManager::~ConfigManager() {}

QString ConfigManager::getConfigDir() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir;
}

QString ConfigManager::getConfigFilePath() const {
    return getConfigDir() + "/config.json";
}

QString ConfigManager::getFirebaseBaseUrl() const {
    return QString("https://firestore.googleapis.com/v1/projects/%1/databases/(default)/documents")
        .arg(getFirebaseProjectId());
}

bool ConfigManager::loadConfig() {
    QString configPath = getConfigFilePath();
    QFile file(configPath);
    
    // If config file doesn't exist, create default
    if (!file.exists()) {
        qDebug() << "[ConfigManager] Config file not found, creating default";
        return createDefaultConfig();
    }
    
    // Load existing config
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ConfigManager] Failed to open config file:" << configPath;
        return createDefaultConfig();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[ConfigManager] JSON parse error:" << error.errorString();
        return createDefaultConfig();
    }
    
    m_config = doc.object();
    qDebug() << "[ConfigManager] Config loaded from:" << configPath;
    
    emit configLoaded();
    return true;
}

bool ConfigManager::saveConfig() {
    QString configPath = getConfigFilePath();
    QFile file(configPath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ConfigManager] Failed to save config file:" << configPath;
        return false;
    }
    
    QJsonDocument doc(m_config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "[ConfigManager] Config saved to:" << configPath;
    
    emit configSaved();
    return true;
}

bool ConfigManager::createDefaultConfig() {
    // Create default config with Firebase project ID
    m_config = QJsonObject{
        {"firebase", QJsonObject{
            {"projectId", "redroid-d8110"},
            {"apiKey", "AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA"}
        }},
        {"api", QJsonObject{
            {"serverPort", 8080},
            {"apiKey", "YOUR_API_KEY"}
        }},
        {"docker", QJsonObject{
            {"path", "docker"},
            {"host", ""},
            {"timeout", 30}
        }},
        {"adb", QJsonObject{
            {"path", "adb"},
            {"portStart", 5555},
            {"autoConnect", true}
        }}
    };
    
    // Save default config
    saveConfig();
    
    qDebug() << "[ConfigManager] Default config created at:" << getConfigFilePath();
    return true;
}

bool ConfigManager::resetToDefaults() {
    m_config = QJsonObject();
    return createDefaultConfig();
}

bool ConfigManager::hasFirebaseConfig() const {
    QString projectId = getFirebaseProjectId();
    QString apiKey = getFirebaseApiKey();
    
    // Check if project ID is valid and not empty
    // API key can be optional for some endpoints
    if (projectId.isEmpty() || 
        projectId == "YOUR_PROJECT_ID" ||
        projectId == "your-firebase-project-id") {
        return false;
    }
    
    if (apiKey.isEmpty() ||
        apiKey == "your-firebase-api-key" ||
        apiKey == "YOUR_API_KEY") {
        return false;
    }
    
    return true;
}

void ConfigManager::setConfig(const QJsonObject& config) {
    m_config = config;
    saveConfig();
}

} // namespace VirtualPhonePro
