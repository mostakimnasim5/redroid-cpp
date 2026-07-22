/**
 * @file DeviceIntegrityManager.h
 * @brief Device Integrity Manager
 * @version 3.0.0
 * 
 * Provides comprehensive device integrity simulation:
 * - Basic integrity checks
 * - Hardware attestation
 * - Verified boot state
 * - System integrity
 * - SELinux status
 * - Gatekeeper status
 */

#pragma once

#ifndef VIRTUALPHONEPRO_DEVICE_INTEGRITY_MANAGER_H
#define VIRTUALPHONEPRO_DEVICE_INTEGRITY_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include <QStringList>

namespace VirtualPhonePro {

// Integrity Check Level
enum class IntegrityLevel {
    BASIC,
    BASIC_HARDWARE,
    CERTIFIED,
    VERIFIED_BOOT
};

// Verified Boot State
enum class VerifiedBootState {
    GREEN,
    YELLOW,
    ORANGE,
    RED,
    UNKNOWN
};

// Device Lock State
enum class DeviceLockState {
    UNLOCKED,
    LOCKED,
    LOCKED_CREDENTIAL,
    LOCKED_BIOMETRIC,
    PASSWORD_ONLY
};

// Integrity Check Result
struct IntegrityCheckResult {
    bool passed;
    QString checkName;
    QString errorCode;
    QString errorMessage;
    QDateTime checkTime;
    int errorCodeNumeric;
};

// Complete Integrity State
struct DeviceIntegrityState {
    QString instanceId;
    
    // Integrity Level
    IntegrityLevel integrityLevel;
    
    // Verified Boot
    VerifiedBootState verifiedBootState;
    bool isVerifiedBootEnabled;
    bool isVerifiedBootLocked;
    QString verifiedBootStateString;
    QString bootVerificationStatus;
    
    // Basic Integrity
    bool isBasicIntegrity;
    bool isCtsProfileMatch;
    bool isBasicIntegritySyscallCheck;
    bool isBasicIntegrityMockLocation;
    bool isBasicIntegrityGmsBanned;
    bool isBasicIntegrityPassed;
    
    // Security Patch
    bool isSecurityPatchCurrent;
    QString securityPatchLevel;
    QDateTime lastSecurityUpdate;
    
    // System Integrity
    bool isSystemIntegrityEnabled;
    bool isDebuggerDetected;
    bool isADBEnabled;
    bool isRooted;
    bool isRootDetectionBypassed;
    bool isEmulatorDetected;
    bool isEmulatorDetectionBypassed;
    
    // SELinux
    bool isSELinuxEnforcing;
    bool isSELinuxPermissive;
    QString selinuxStatus;
    
    // Gatekeeper
    bool isGatekeeperEnabled;
    bool isGatekeeperLocked;
    bool isGatekeeperTimeoutValid;
    int gatekeeperTimeoutSeconds;
    
    // Device Lock
    DeviceLockState lockState;
    bool isPasswordSet;
    bool isBiometricEnabled;
    bool isStrongBiometricEnabled;
    int maxFailedAttempts;
    int currentFailedAttempts;
    
    // Additional Checks
    bool isHardwareBackedKeyStore;
    bool isSecureHardwarePresent;
    bool isScreenLockSet;
    bool isEncryptionEnabled;
    bool isEncryptionSupported;
    bool isFileBasedEncryptionEnabled;
    bool isAdiantumEnabled;
    
    // Last check
    QDateTime lastIntegrityCheck;
    int totalChecksPassed;
    int totalChecksFailed;
};

class DeviceIntegrityManager {
public:
    static DeviceIntegrityManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure device integrity
     */
    bool configure(const QString& instanceId, const DeviceIntegrityState& config);
    
    /**
     * @brief Configure for certification level
     */
    bool configureForLevel(const QString& instanceId, IntegrityLevel level);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Verified Boot
    // =========================================================================
    
    /**
     * @brief Set verified boot state
     */
    bool setVerifiedBootState(const QString& instanceId, VerifiedBootState state);
    
    /**
     * @brief Get verified boot state
     */
    VerifiedBootState getVerifiedBootState(const QString& instanceId) const;
    
    /**
     * @brief Enable verified boot
     */
    bool enableVerifiedBoot(const QString& instanceId);
    
    /**
     * @brief Disable verified boot
     */
    bool disableVerifiedBoot(const QString& instanceId);
    
    // =========================================================================
    // Basic Integrity
    // =========================================================================
    
    /**
     * @brief Set basic integrity status
     */
    bool setBasicIntegrity(const QString& instanceId, bool integrity);
    
    /**
     * @brief Set CTS profile match
     */
    bool setCtsProfileMatch(const QString& instanceId, bool match);
    
    /**
     * @brief Run integrity check
     */
    IntegrityCheckResult runIntegrityCheck(const QString& instanceId, const QString& checkName);
    
    /**
     * @brief Get all integrity check results
     */
    QList<IntegrityCheckResult> getAllCheckResults(const QString& instanceId);
    
    // =========================================================================
    // Security Patch
    // =========================================================================
    
    /**
     * @brief Set security patch level
     */
    bool setSecurityPatchLevel(const QString& instanceId, const QString& patchLevel);
    
    /**
     * @brief Get security patch level
     */
    QString getSecurityPatchLevel(const QString& instanceId) const;
    
    /**
     * @brief Check if patch is current
     */
    bool isSecurityPatchCurrent(const QString& instanceId);
    
    // =========================================================================
    // System Integrity
    // =========================================================================
    
    /**
     * @brief Enable system integrity
     */
    bool enableSystemIntegrity(const QString& instanceId);
    
    /**
     * @brief Disable system integrity
     */
    bool disableSystemIntegrity(const QString& instanceId);
    
    /**
     * @brief Set debugger detection
     */
    bool setDebuggerDetected(const QString& instanceId, bool detected);
    
    /**
     * @brief Set ADB state
     */
    bool setADBEnabled(const QString& instanceId, bool enabled);
    
    /**
     * @brief Set root status
     */
    bool setRootStatus(const QString& instanceId, bool isRooted, bool bypassed = false);
    
    /**
     * @brief Set emulator detection
     */
    bool setEmulatorDetection(const QString& instanceId, bool detected, bool bypassed = false);
    
    // =========================================================================
    // SELinux
    // =========================================================================
    
    /**
     * @brief Enable SELinux enforcing
     */
    bool enableSELinuxEnforcing(const QString& instanceId);
    
    /**
     * @brief Set SELinux permissive
     */
    bool setSELinuxPermissive(const QString& instanceId);
    
    /**
     * @brief Get SELinux status
     */
    QString getSELinuxStatus(const QString& instanceId) const;
    
    // =========================================================================
    // Gatekeeper
    // =========================================================================
    
    /**
     * @brief Enable gatekeeper
     */
    bool enableGatekeeper(const QString& instanceId);
    
    /**
     * @brief Lock gatekeeper
     */
    bool lockGatekeeper(const QString& instanceId);
    
    /**
     * @brief Set gatekeeper timeout
     */
    bool setGatekeeperTimeout(const QString& instanceId, int seconds);
    
    // =========================================================================
    // Device Lock
    // =========================================================================
    
    /**
     * @brief Set device lock state
     */
    bool setDeviceLockState(const QString& instanceId, DeviceLockState state);
    
    /**
     * @brief Get device lock state
     */
    DeviceLockState getDeviceLockState(const QString& instanceId) const;
    
    /**
     * @brief Enable biometric
     */
    bool enableBiometric(const QString& instanceId, bool strong);
    
    /**
     * @brief Disable biometric
     */
    bool disableBiometric(const QString& instanceId);
    
    /**
     * @brief Set password
     */
    bool setPassword(const QString& instanceId, const QString& password);
    
    // =========================================================================
    // Encryption
    // =========================================================================
    
    /**
     * @brief Enable file-based encryption
     */
    bool enableFileBasedEncryption(const QString& instanceId);
    
    /**
     * @brief Check encryption status
     */
    bool isEncryptionEnabled(const QString& instanceId) const;
    
    /**
     * @brief Enable full disk encryption
     */
    bool enableFullDiskEncryption(const QString& instanceId);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get complete integrity state
     */
    DeviceIntegrityState getIntegrityState(const QString& instanceId) const;
    
    /**
     * @brief Get integrity info as JSON
     */
    QJsonObject getIntegrityInfoJSON(const QString& instanceId) const;
    
    /**
     * @brief Check if device passes integrity
     */
    bool passesIntegrity(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
    /**
     * @brief Apply all integrity features
     */
    bool applyAllIntegrity(const QString& instanceId);
    
private:
    DeviceIntegrityManager();
    static DeviceIntegrityManager* s_instance;
    
    // Helper methods
    QString integrityLevelToString(IntegrityLevel level) const;
    QString bootStateToString(VerifiedBootState state) const;
    QString lockStateToString(DeviceLockState state) const;
    IntegrityLevel stringToIntegrityLevel(const QString& level) const;
    bool evaluateIntegrityChecks(const QString& instanceId);
    
    QMap<QString, DeviceIntegrityState> m_states;
    QMap<QString, QList<IntegrityCheckResult>> m_checkResults;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_DEVICE_INTEGRITY_MANAGER_H
