/**
 * @file PersistentIdentityManager.h
 * @brief Persistent Identity Manager
 * @version 4.0.0
 * 
 * Manages persistent device identities that survive across:
 * - Device restarts
 * - Factory resets
 * - App reinstalls
 * - Docker container rebuilds
 */

#pragma once

#ifndef VIRTUALPHONEPRO_PERSISTENT_IDENTITY_MANAGER_H
#define VIRTUALPHONEPRO_PERSISTENT_IDENTITY_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include <QList>

namespace VirtualPhonePro {

// Persistent Identity Types
enum class IdentityType {
    ANDROID_ID,
    GSF_ID,
    GOOGLE_ADVERTISING_ID,
    DEVICE_QUALIFICATION_ID,
    ANDROID_DEVICE_ID,
    GOOGLE_SERVICES_HASH,
    BOOT_TOKEN,
    FINGERPRINT_ID
};

// Identity Entry
struct IdentityEntry {
    IdentityType type;
    QString value;
    QString hashedValue;
    QDateTime createdAt;
    QDateTime lastUsed;
    bool isActive;
    bool isPersistent;
};

// Device Credential
struct DeviceCredential {
    QString credentialType;  // "password", "pattern", "pin", "biometric"
    QString credentialHash;
    QString credentialSalt;
    int failedAttempts;
    QDateTime lastFailedAttempt;
    bool isSet;
    bool isBiometricEnabled;
    bool isStrongBiometricEnabled;
};

// Factory Reset Configuration
struct FactoryResetConfig {
    int resetCount;
    QDateTime lastResetTime;
    QDateTime firstBootDate;
    int daysSinceFactoryReset;
    QString previousAndroidId;
    QString previousGSFId;
    bool isFirstBoot;
};

// Backup Configuration
struct BackupConfig {
    QString backupAccount;
    QString backupToken;
    bool isBackupEnabled;
    bool isAutoBackupEnabled;
    QDateTime lastBackupTime;
    int backupSize;
};

class PersistentIdentityManager {
public:
    static PersistentIdentityManager& instance();
    
    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize identity manager
     */
    bool initialize(const QString& instanceId);
    
    /**
     * @brief Generate all identities
     */
    bool generateAllIdentities(const QString& instanceId);
    
    /**
     * @brief Apply all identities
     */
    bool applyAllIdentities(const QString& instanceId);
    
    // =========================================================================
    // Android ID
    // =========================================================================
    
    /**
     * @brief Get Android ID
     */
    QString getAndroidId(const QString& instanceId) const;
    
    /**
     * @brief Set Android ID
     */
    bool setAndroidId(const QString& instanceId, const QString& androidId);
    
    /**
     * @brief Generate Android ID
     */
    QString generateAndroidId();
    
    // =========================================================================
    // GSF ID
    // =========================================================================
    
    /**
     * @brief Get GSF ID
     */
    QString getGSFId(const QString& instanceId) const;
    
    /**
     * @brief Set GSF ID
     */
    bool setGSFId(const QString& instanceId, const QString& gsfId);
    
    /**
     * @brief Generate GSF ID
     */
    QString generateGSFId();
    
    // =========================================================================
    // Google Advertising ID
    // =========================================================================
    
    /**
     * @brief Get GAID
     */
    QString getGAID(const QString& instanceId) const;
    
    /**
     * @brief Generate GAID
     */
    QString generateGAID();
    
    /**
     * @brief Reset GAID
     */
    bool resetGAID(const QString& instanceId);
    
    // =========================================================================
    // Device Qualification ID
    // =========================================================================
    
    /**
     * @brief Get Device Qualification ID
     */
    QString getDeviceQualificationId(const QString& instanceId) const;
    
    /**
     * @brief Generate Device Qualification ID
     */
    QString generateDeviceQualificationId();
    
    // =========================================================================
    // Boot Token
    // =========================================================================
    
    /**
     * @brief Get Boot Token
     */
    QString getBootToken(const QString& instanceId) const;
    
    /**
     * @brief Generate Boot Token
     */
    QString generateBootToken();
    
    // =========================================================================
    // Device Credentials
    // =========================================================================
    
    /**
     * @brief Get device credential
     */
    DeviceCredential getDeviceCredential(const QString& instanceId) const;
    
    /**
     * @brief Set lock screen credential
     */
    bool setLockCredential(const QString& instanceId, const QString& credentialType, const QString& credential);
    
    /**
     * @brief Enable biometric
     */
    bool enableBiometric(const QString& instanceId, bool strong);
    
    /**
     * @brief Get failed attempts
     */
    int getFailedAttempts(const QString& instanceId) const;
    
    // =========================================================================
    // Factory Reset Simulation
    // =========================================================================
    
    /**
     * @brief Get factory reset config
     */
    FactoryResetConfig getFactoryResetConfig(const QString& instanceId) const;
    
    /**
     * @brief Set factory reset config
     */
    bool setFactoryResetConfig(const QString& instanceId, const FactoryResetConfig& config);
    
    /**
     * @brief Simulate factory reset
     */
    bool simulateFactoryReset(const QString& instanceId);
    
    /**
     * @brief Check if first boot after reset
     */
    bool isFirstBootAfterReset(const QString& instanceId) const;
    
    // =========================================================================
    // Backup Configuration
    // =========================================================================
    
    /**
     * @brief Get backup config
     */
    BackupConfig getBackupConfig(const QString& instanceId) const;
    
    /**
     * @brief Configure backup
     */
    bool configureBackup(const QString& instanceId, const BackupConfig& config);
    
    /**
     * @brief Link backup account
     */
    bool linkBackupAccount(const QString& instanceId, const QString& email);
    
    // =========================================================================
    // Persistence
    // =========================================================================
    
    /**
     * @brief Persist all identities to file
     */
    bool persistToFile(const QString& instanceId);
    
    /**
     * @brief Load identities from file
     */
    bool loadFromFile(const QString& instanceId);
    
    /**
     * @brief Export identities
     */
    QJsonObject exportIdentities(const QString& instanceId) const;
    
    /**
     * @brief Import identities
     */
    bool importIdentities(const QString& instanceId, const QJsonObject& data);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get all identities
     */
    QMap<IdentityType, IdentityEntry> getAllIdentities(const QString& instanceId) const;
    
    /**
     * @brief Get identity as JSON
     */
    QJsonObject getIdentityJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
private:
    PersistentIdentityManager();
    ~PersistentIdentityManager();
    
    static PersistentIdentityManager* s_instance;
    
    QString generateHash(const QString& input);
    QString generateRandomString(int length);
    quint64 generateRandomNumber(quint64 min, quint64 max);
    
    QMap<QString, QMap<IdentityType, IdentityEntry>> m_identities;
    QMap<QString, DeviceCredential> m_credentials;
    QMap<QString, FactoryResetConfig> m_factoryResetConfigs;
    QMap<QString, BackupConfig> m_backupConfigs;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PERSISTENT_IDENTITY_MANAGER_H
