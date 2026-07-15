/**
 * @file PersistentIdentityManager.cpp
 * @brief Persistent Identity Manager Implementation
 */

#include "VirtualPhonePro/PersistentIdentityManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>

namespace VirtualPhonePro {

PersistentIdentityManager* PersistentIdentityManager::s_instance = nullptr;

PersistentIdentityManager& PersistentIdentityManager::instance() {
    if (!s_instance) {
        s_instance = new PersistentIdentityManager();
    }
    return *s_instance;
}

PersistentIdentityManager::PersistentIdentityManager() {
}

PersistentIdentityManager::~PersistentIdentityManager() {
    // Persist all identities on destruction
    for (auto it = m_identities.begin(); it != m_identities.end(); ++it) {
        persistToFile(it.key());
    }
}

bool PersistentIdentityManager::initialize(const QString& instanceId) {
    qDebug() << "Initializing PersistentIdentityManager for:" << instanceId;
    
    // Generate all identities if not exist
    if (!m_identities.contains(instanceId)) {
        generateAllIdentities(instanceId);
    }
    
    // Initialize factory reset config
    if (!m_factoryResetConfigs.contains(instanceId)) {
        FactoryResetConfig frConfig;
        frConfig.resetCount = 0;
        frConfig.lastResetTime = QDateTime::currentDateTime();
        frConfig.firstBootDate = QDateTime::currentDateTime();
        frConfig.daysSinceFactoryReset = 0;
        frConfig.isFirstBoot = true;
        m_factoryResetConfigs[instanceId] = frConfig;
    }
    
    // Apply all identities
    return applyAllIdentities(instanceId);
}

bool PersistentIdentityManager::generateAllIdentities(const QString& instanceId) {
    QMap<IdentityType, IdentityEntry> identities;
    QDateTime now = QDateTime::currentDateTime();
    
    // Android ID (16 hex characters)
    IdentityEntry androidId;
    androidId.type = IdentityType::ANDROID_ID;
    androidId.value = generateAndroidId();
    androidId.hashedValue = generateHash(androidId.value);
    androidId.createdAt = now;
    androidId.lastUsed = now;
    androidId.isActive = true;
    androidId.isPersistent = true;
    identities[IdentityType::ANDROID_ID] = androidId;
    
    // GSF ID (Google Services Framework ID)
    IdentityEntry gsfId;
    gsfId.type = IdentityType::GSF_ID;
    gsfId.value = generateGSFId();
    gsfId.hashedValue = generateHash(gsfId.value);
    gsfId.createdAt = now;
    gsfId.lastUsed = now;
    gsfId.isActive = true;
    gsfId.isPersistent = true;
    identities[IdentityType::GSF_ID] = gsfId;
    
    // Google Advertising ID
    IdentityEntry gaid;
    gaid.type = IdentityType::GOOGLE_ADVERTISING_ID;
    gaid.value = generateGAID();
    gaid.hashedValue = generateHash(gaid.value);
    gaid.createdAt = now;
    gaid.lastUsed = now;
    gaid.isActive = true;
    gaid.isPersistent = false;  // Can be reset
    identities[IdentityType::GOOGLE_ADVERTISING_ID] = gaid;
    
    // Device Qualification ID
    IdentityEntry dqId;
    dqId.type = IdentityType::DEVICE_QUALIFICATION_ID;
    dqId.value = generateDeviceQualificationId();
    dqId.hashedValue = generateHash(dqId.value);
    dqId.createdAt = now;
    dqId.lastUsed = now;
    dqId.isActive = true;
    dqId.isPersistent = true;
    identities[IdentityType::DEVICE_QUALIFICATION_ID] = dqId;
    
    // Android Device ID
    IdentityEntry deviceId;
    deviceId.type = IdentityType::ANDROID_DEVICE_ID;
    deviceId.value = generateRandomString(16).toUpper();
    deviceId.hashedValue = generateHash(deviceId.value);
    deviceId.createdAt = now;
    deviceId.lastUsed = now;
    deviceId.isActive = true;
    deviceId.isPersistent = true;
    identities[IdentityType::ANDROID_DEVICE_ID] = deviceId;
    
    // Boot Token
    IdentityEntry bootToken;
    bootToken.type = IdentityType::BOOT_TOKEN;
    bootToken.value = generateBootToken();
    bootToken.hashedValue = generateHash(bootToken.value);
    bootToken.createdAt = now;
    bootToken.lastUsed = now;
    bootToken.isActive = true;
    bootToken.isPersistent = true;
    identities[IdentityType::BOOT_TOKEN] = bootToken;
    
    m_identities[instanceId] = identities;
    
    // Initialize default credential
    DeviceCredential cred;
    cred.credentialType = "password";
    cred.credentialHash = "";
    cred.failedAttempts = 0;
    cred.isSet = false;
    cred.isBiometricEnabled = false;
    cred.isStrongBiometricEnabled = false;
    m_credentials[instanceId] = cred;
    
    // Initialize backup config
    BackupConfig backup;
    backup.isBackupEnabled = true;
    backup.isAutoBackupEnabled = true;
    m_backupConfigs[instanceId] = backup;
    
    return true;
}

bool PersistentIdentityManager::applyAllIdentities(const QString& instanceId) {
    if (!m_identities.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    const QMap<IdentityType, IdentityEntry>& identities = m_identities[instanceId];
    
    // Apply Android ID
    if (identities.contains(IdentityType::ANDROID_ID)) {
        const QString& androidId = identities[IdentityType::ANDROID_ID].value;
        ctrl.executeShell(instanceId, QString("settings put secure android_id %1").arg(androidId));
        ctrl.executeShell(instanceId, QString("setprop ro.setupwizard.android_id %1").arg(androidId));
    }
    
    // Apply GSF ID
    if (identities.contains(IdentityType::GSF_ID)) {
        const QString& gsfId = identities[IdentityType::GSF_ID].value;
        ctrl.executeShell(instanceId, QString("content insert --uri content://com.google.settings/partner --bind name:s --bind value:s%1").arg(gsfId));
    }
    
    qDebug() << "Applied all persistent identities for:" << instanceId;
    
    return true;
}

QString PersistentIdentityManager::getAndroidId(const QString& instanceId) const {
    if (m_identities.contains(instanceId) && m_identities[instanceId].contains(IdentityType::ANDROID_ID)) {
        return m_identities[instanceId][IdentityType::ANDROID_ID].value;
    }
    return QString();
}

bool PersistentIdentityManager::setAndroidId(const QString& instanceId, const QString& androidId) {
    if (!m_identities.contains(instanceId)) {
        return false;
    }
    
    m_identities[instanceId][IdentityType::ANDROID_ID].value = androidId;
    m_identities[instanceId][IdentityType::ANDROID_ID].hashedValue = generateHash(androidId);
    m_identities[instanceId][IdentityType::ANDROID_ID].lastUsed = QDateTime::currentDateTime();
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put secure android_id %1").arg(androidId));
    
    return true;
}

QString PersistentIdentityManager::generateAndroidId() {
    // Android ID is 16 hex characters
    return generateRandomString(16);
}

QString PersistentIdentityManager::getGSFId(const QString& instanceId) const {
    if (m_identities.contains(instanceId) && m_identities[instanceId].contains(IdentityType::GSF_ID)) {
        return m_identities[instanceId][IdentityType::GSF_ID].value;
    }
    return QString();
}

bool PersistentIdentityManager::setGSFId(const QString& instanceId, const QString& gsfId) {
    if (!m_identities.contains(instanceId)) {
        return false;
    }
    
    m_identities[instanceId][IdentityType::GSF_ID].value = gsfId;
    m_identities[instanceId][IdentityType::GSF_ID].hashedValue = generateHash(gsfId);
    m_identities[instanceId][IdentityType::GSF_ID].lastUsed = QDateTime::currentDateTime();
    
    return true;
}

QString PersistentIdentityManager::generateGSFId() {
    // GSF ID format: long numeric string
    // Example: 3515388264682749854
    return QString::number(generateRandomNumber(1000000000000000ULL, 9999999999999999ULL));
}

QString PersistentIdentityManager::getGAID(const QString& instanceId) const {
    if (m_identities.contains(instanceId) && m_identities[instanceId].contains(IdentityType::GOOGLE_ADVERTISING_ID)) {
        return m_identities[instanceId][IdentityType::GOOGLE_ADVERTISING_ID].value;
    }
    return QString();
}

QString PersistentIdentityManager::generateGAID() {
    // GAID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    // First and last segments are 8 chars, middle three are 4 chars
    QString part1 = generateRandomString(8);
    QString part2 = generateRandomString(4);
    QString part3 = generateRandomString(4);
    QString part4 = generateRandomString(4);
    QString part5 = generateRandomString(12);
    
    return QString("%1-%2-%3-%4-%5").arg(part1, part2, part3, part4, part5);
}

bool PersistentIdentityManager::resetGAID(const QString& instanceId) {
    if (!m_identities.contains(instanceId)) {
        return false;
    }
    
    m_identities[instanceId][IdentityType::GOOGLE_ADVERTISING_ID].value = generateGAID();
    m_identities[instanceId][IdentityType::GOOGLE_ADVERTISING_ID].lastUsed = QDateTime::currentDateTime();
    
    return true;
}

QString PersistentIdentityManager::getDeviceQualificationId(const QString& instanceId) const {
    if (m_identities.contains(instanceId) && m_identities[instanceId].contains(IdentityType::DEVICE_QUALIFICATION_ID)) {
        return m_identities[instanceId][IdentityType::DEVICE_QUALIFICATION_ID].value;
    }
    return QString();
}

QString PersistentIdentityManager::generateDeviceQualificationId() {
    // Device qualification ID - long alphanumeric
    return "DQ" + generateRandomString(32);
}

QString PersistentIdentityManager::getBootToken(const QString& instanceId) const {
    if (m_identities.contains(instanceId) && m_identities[instanceId].contains(IdentityType::BOOT_TOKEN)) {
        return m_identities[instanceId][IdentityType::BOOT_TOKEN].value;
    }
    return QString();
}

QString PersistentIdentityManager::generateBootToken() {
    // Boot token - 64 character hex
    return generateRandomString(64);
}

DeviceCredential PersistentIdentityManager::getDeviceCredential(const QString& instanceId) const {
    if (m_credentials.contains(instanceId)) {
        return m_credentials[instanceId];
    }
    
    DeviceCredential defaultCred;
    defaultCred.credentialType = "none";
    defaultCred.failedAttempts = 0;
    defaultCred.isSet = false;
    defaultCred.isBiometricEnabled = false;
    return defaultCred;
}

bool PersistentIdentityManager::setLockCredential(const QString& instanceId, const QString& credentialType, const QString& credential) {
    DeviceCredential& cred = m_credentials[instanceId];
    
    cred.credentialType = credentialType;
    cred.credentialHash = generateHash(credential);
    cred.credentialSalt = generateRandomString(16);
    cred.isSet = true;
    cred.failedAttempts = 0;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (credentialType == "password" || credentialType == "pin") {
        ctrl.executeShell(instanceId, QString("locksmith reset_password %1").arg(credential));
    }
    
    return true;
}

bool PersistentIdentityManager::enableBiometric(const QString& instanceId, bool strong) {
    DeviceCredential& cred = m_credentials[instanceId];
    
    cred.isBiometricEnabled = true;
    cred.isStrongBiometricEnabled = strong;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure biometric_enabled 1");
    if (strong) {
        ctrl.executeShell(instanceId, "settings put secure strong_biometric_enabled 1");
    }
    
    return true;
}

int PersistentIdentityManager::getFailedAttempts(const QString& instanceId) const {
    if (m_credentials.contains(instanceId)) {
        return m_credentials[instanceId].failedAttempts;
    }
    return 0;
}

FactoryResetConfig PersistentIdentityManager::getFactoryResetConfig(const QString& instanceId) const {
    FactoryResetConfig defaultConfig;
    defaultConfig.resetCount = 0;
    defaultConfig.isFirstBoot = true;
    
    if (m_factoryResetConfigs.contains(instanceId)) {
        return m_factoryResetConfigs[instanceId];
    }
    return defaultConfig;
}

bool PersistentIdentityManager::setFactoryResetConfig(const QString& instanceId, const FactoryResetConfig& config) {
    m_factoryResetConfigs[instanceId] = config;
    return true;
}

bool PersistentIdentityManager::simulateFactoryReset(const QString& instanceId) {
    // Save previous IDs
    FactoryResetConfig config;
    if (m_factoryResetConfigs.contains(instanceId)) {
        config = m_factoryResetConfigs[instanceId];
        config.previousAndroidId = getAndroidId(instanceId);
        config.previousGSFId = getGSFId(instanceId);
    }
    
    // Update reset count
    config.resetCount++;
    config.lastResetTime = QDateTime::currentDateTime();
    config.daysSinceFactoryReset = 0;
    config.isFirstBoot = true;
    
    m_factoryResetConfigs[instanceId] = config;
    
    // Generate new identities
    setAndroidId(instanceId, generateAndroidId());
    setGSFId(instanceId, generateGSFId());
    resetGAID(instanceId);
    
    // Reset credentials
    m_credentials[instanceId] = DeviceCredential();
    m_credentials[instanceId].failedAttempts = 0;
    
    // Apply new identities
    applyAllIdentities(instanceId);
    
    qDebug() << "Simulated factory reset for:" << instanceId << "- Reset count:" << config.resetCount;
    
    return true;
}

bool PersistentIdentityManager::isFirstBootAfterReset(const QString& instanceId) const {
    if (m_factoryResetConfigs.contains(instanceId)) {
        return m_factoryResetConfigs[instanceId].isFirstBoot;
    }
    return false;
}

BackupConfig PersistentIdentityManager::getBackupConfig(const QString& instanceId) const {
    BackupConfig defaultConfig;
    defaultConfig.isBackupEnabled = true;
    defaultConfig.isAutoBackupEnabled = true;
    
    if (m_backupConfigs.contains(instanceId)) {
        return m_backupConfigs[instanceId];
    }
    return defaultConfig;
}

bool PersistentIdentityManager::configureBackup(const QString& instanceId, const BackupConfig& config) {
    m_backupConfigs[instanceId] = config;
    return true;
}

bool PersistentIdentityManager::linkBackupAccount(const QString& instanceId, const QString& email) {
    BackupConfig& config = m_backupConfigs[instanceId];
    config.backupAccount = email;
    config.backupToken = generateRandomString(64);
    config.isBackupEnabled = true;
    
    return true;
}

bool PersistentIdentityManager::persistToFile(const QString& instanceId) {
    QJsonObject data = exportIdentities(instanceId);
    QJsonDocument doc(data);
    
    QString filePath = QString("/data/local/tmp/vpp_identity/%1/identities.json").arg(instanceId);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }
    
    return false;
}

bool PersistentIdentityManager::loadFromFile(const QString& instanceId) {
    QString filePath = QString("/data/local/tmp/vpp_identity/%1/identities.json").arg(instanceId);
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            return importIdentities(instanceId, doc.object());
        }
    }
    
    return false;
}

QJsonObject PersistentIdentityManager::exportIdentities(const QString& instanceId) const {
    QJsonObject data;
    
    // Export identities
    QJsonArray identities;
    if (m_identities.contains(instanceId)) {
        for (auto it = m_identities[instanceId].begin(); it != m_identities[instanceId].end(); ++it) {
            QJsonObject entry;
            entry["type"] = static_cast<int>(it.value().type);
            entry["value"] = it.value().value;
            entry["hashedValue"] = it.value().hashedValue;
            entry["createdAt"] = it.value().createdAt.toString(Qt::ISODate);
            entry["lastUsed"] = it.value().lastUsed.toString(Qt::ISODate);
            entry["isActive"] = it.value().isActive;
            entry["isPersistent"] = it.value().isPersistent;
            identities.append(entry);
        }
    }
    data["identities"] = identities;
    
    // Export factory reset config
    if (m_factoryResetConfigs.contains(instanceId)) {
        const FactoryResetConfig& fr = m_factoryResetConfigs[instanceId];
        QJsonObject frConfig;
        frConfig["resetCount"] = fr.resetCount;
        frConfig["lastResetTime"] = fr.lastResetTime.toString(Qt::ISODate);
        frConfig["firstBootDate"] = fr.firstBootDate.toString(Qt::ISODate);
        frConfig["previousAndroidId"] = fr.previousAndroidId;
        frConfig["previousGSFId"] = fr.previousGSFId;
        frConfig["isFirstBoot"] = fr.isFirstBoot;
        data["factoryReset"] = frConfig;
    }
    
    // Export backup config
    if (m_backupConfigs.contains(instanceId)) {
        const BackupConfig& backup = m_backupConfigs[instanceId];
        QJsonObject backupConfig;
        backupConfig["backupAccount"] = backup.backupAccount;
        backupConfig["backupToken"] = backup.backupToken;
        backupConfig["isBackupEnabled"] = backup.isBackupEnabled;
        backupConfig["isAutoBackupEnabled"] = backup.isAutoBackupEnabled;
        backupConfig["lastBackupTime"] = backup.lastBackupTime.toString(Qt::ISODate);
        data["backup"] = backupConfig;
    }
    
    return data;
}

bool PersistentIdentityManager::importIdentities(const QString& instanceId, const QJsonObject& data) {
    // Import identities
    if (data.contains("identities")) {
        QJsonArray identities = data["identities"].toArray();
        for (const QJsonValue& val : identities) {
            QJsonObject entry = val.toObject();
            IdentityEntry ie;
            ie.type = static_cast<IdentityType>(entry["type"].toInt());
            ie.value = entry["value"].toString();
            ie.hashedValue = entry["hashedValue"].toString();
            ie.createdAt = QDateTime::fromString(entry["createdAt"].toString(), Qt::ISODate);
            ie.lastUsed = QDateTime::fromString(entry["lastUsed"].toString(), Qt::ISODate);
            ie.isActive = entry["isActive"].toBool();
            ie.isPersistent = entry["isPersistent"].toBool();
            m_identities[instanceId][ie.type] = ie;
        }
    }
    
    // Import factory reset config
    if (data.contains("factoryReset")) {
        QJsonObject frConfig = data["factoryReset"].toObject();
        FactoryResetConfig fr;
        fr.resetCount = frConfig["resetCount"].toInt();
        fr.lastResetTime = QDateTime::fromString(frConfig["lastResetTime"].toString(), Qt::ISODate);
        fr.firstBootDate = QDateTime::fromString(frConfig["firstBootDate"].toString(), Qt::ISODate);
        fr.previousAndroidId = frConfig["previousAndroidId"].toString();
        fr.previousGSFId = frConfig["previousGSFId"].toString();
        fr.isFirstBoot = frConfig["isFirstBoot"].toBool();
        m_factoryResetConfigs[instanceId] = fr;
    }
    
    // Import backup config
    if (data.contains("backup")) {
        QJsonObject backupConfig = data["backup"].toObject();
        BackupConfig backup;
        backup.backupAccount = backupConfig["backupAccount"].toString();
        backup.backupToken = backupConfig["backupToken"].toString();
        backup.isBackupEnabled = backupConfig["isBackupEnabled"].toBool();
        backup.isAutoBackupEnabled = backupConfig["isAutoBackupEnabled"].toBool();
        backup.lastBackupTime = QDateTime::fromString(backupConfig["lastBackupTime"].toString(), Qt::ISODate);
        m_backupConfigs[instanceId] = backup;
    }
    
    return true;
}

QMap<IdentityType, IdentityEntry> PersistentIdentityManager::getAllIdentities(const QString& instanceId) const {
    if (m_identities.contains(instanceId)) {
        return m_identities[instanceId];
    }
    return QMap<IdentityType, IdentityEntry>();
}

QJsonObject PersistentIdentityManager::getIdentityJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_identities.contains(instanceId)) {
        const QMap<IdentityType, IdentityEntry>& identities = m_identities[instanceId];
        
        for (auto it = identities.begin(); it != identities.end(); ++it) {
            QString key;
            switch (it.key()) {
                case IdentityType::ANDROID_ID: key = "androidId"; break;
                case IdentityType::GSF_ID: key = "gsfId"; break;
                case IdentityType::GOOGLE_ADVERTISING_ID: key = "gaid"; break;
                case IdentityType::DEVICE_QUALIFICATION_ID: key = "deviceQualificationId"; break;
                case IdentityType::ANDROID_DEVICE_ID: key = "androidDeviceId"; break;
                case IdentityType::BOOT_TOKEN: key = "bootToken"; break;
                default: key = "unknown"; break;
            }
            
            QJsonObject entry;
            entry["value"] = it.value().value;
            entry["isPersistent"] = it.value().isPersistent;
            entry["lastUsed"] = it.value().lastUsed.toString(Qt::ISODate);
            json[key] = entry;
        }
    }
    
    return json;
}

bool PersistentIdentityManager::reset(const QString& instanceId) {
    m_identities.remove(instanceId);
    m_credentials.remove(instanceId);
    m_factoryResetConfigs.remove(instanceId);
    m_backupConfigs.remove(instanceId);
    
    return true;
}

QString PersistentIdentityManager::generateHash(const QString& input) {
    QByteArray data = input.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

QString PersistentIdentityManager::generateRandomString(int length) {
    const QString chars = "0123456789abcdef";
    QString result;
    QRandomGenerator* gen = QRandomGenerator::global();
    
    for (int i = 0; i < length; i++) {
        int index = gen->bounded(chars.length());
        result.append(chars[index]);
    }
    
    return result;
}

quint64 PersistentIdentityManager::generateRandomNumber(quint64 min, quint64 max) {
    return QRandomGenerator::global()->bounded(min, max + 1);
}

} // namespace VirtualPhonePro
