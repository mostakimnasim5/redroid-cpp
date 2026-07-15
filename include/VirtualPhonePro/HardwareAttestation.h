/**
 * @file HardwareAttestation.h
 * @brief Hardware Attestation & Keystore Simulation
 * @version 2.0.0
 * 
 * Provides hardware-backed attestation and keystore simulation
 * for StrongBox Keymaster and TEE operations.
 * 
 * This is critical for banking apps that verify:
 * - Hardware-backed key storage
 * - Verified boot chain
 * - TEE/SE presence
 */

#pragma once

#ifndef VIRTUALPHONEPRO_HARDWARE_ATTESTATION_H
#define VIRTUALPHONEPRO_HARDWARE_ATTESTATION_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// Attestation key types
enum class AttestationKeyType {
    ATTESTATION,
    RSA_2048,
    RSA_3072,
    EC_P256,
    EC_P384
};

// Keymaster HAL version
enum class KeymasterVersion {
    KM_1_0,
    KM_2_0,
    KM_3_0,
    KM_4_0,
    KM_4_1,
    KM_STRONGBOX
};

// Verified boot state
enum class VerifiedBootState {
    VERIFIED,      // Green - Device is verified
    SELF_SIGNED,   // Yellow - Device has been modified
    UNVERIFIED,    // Orange - Device is unverified
    FAILED,        // Red - Device verification failed
    UNLOCKED       // Orange - Bootloader unlocked
};

// Bootloader lock state
enum class BootloaderState {
    LOCKED,
    UNLOCKED,
    RELOCKED
};

// DRM security level
enum class DRMLevel {
    L3,  // Software only
    L2,  // Hardware-backed
    L1   // Full hardware security (Widevine)
};

// Complete hardware security state
struct HardwareSecurityState {
    // Keymaster
    KeymasterVersion keymasterVersion;
    bool isStrongBox;
    bool isTEEPresent;
    bool isSEPresent;
    
    // Verified Boot
    VerifiedBootState verifiedBootState;
    BootloaderState bootloaderState;
    QString verifiedBootKey;
    QString verifiedBootHash;
    QString verifiedBootStateString;
    
    // Security Features
    bool isHardwareAttestationSupported;
    bool isDeviceLockEnabled;
    bool isSecureHardwarePresent;
    bool isEncryptionEnabled;
    bool isEncryptionSupported;
    
    // DRM
    DRMLevel drmLevel;
    QString widevineKeybox;
    bool isHDCPCompliant;
    int hdcpLevel;
    
    // TEE Info
    QString teeVendor;
    QString teeVersion;
    QString teePatchLevel;
    
    // Chipset Info
    QString socManufacturer;
    QString socModel;
    QString hardwareVendor;
    
    // Timestamps
    qint64 lastAttestationTime;
    qint64 bootVerifiedTime;
};

class HardwareAttestation {
public:
    static HardwareAttestation& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure hardware security for device model
     */
    bool configureForDevice(const QString& manufacturer, const QString& model);
    
    /**
     * @brief Set complete security state
     */
    bool setSecurityState(const QString& instanceId, const HardwareSecurityState& state);
    
    /**
     * @brief Get current security state
     */
    HardwareSecurityState getSecurityState(const QString& instanceId) const;
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Keymaster Operations
    // =========================================================================
    
    /**
     * @brief Enable StrongBox Keymaster (StrongBox Keystore)
     */
    bool enableStrongBox(const QString& instanceId);
    
    /**
     * @brief Enable TEE-based Keymaster
     */
    bool enableTEEKeymaster(const QString& instanceId);
    
    /**
     * @brief Set Keymaster version
     */
    bool setKeymasterVersion(const QString& instanceId, KeymasterVersion version);
    
    /**
     * @brief Generate attestation key
     */
    bool generateAttestationKey(const QString& instanceId, AttestationKeyType type);
    
    // =========================================================================
    // Verified Boot
    // =========================================================================
    
    /**
     * @brief Set verified boot state
     */
    bool setVerifiedBootState(const QString& instanceId, VerifiedBootState state);
    
    /**
     * @brief Set bootloader state
     */
    bool setBootloaderState(const QString& instanceId, BootloaderState state);
    
    /**
     * @brief Lock bootloader
     */
    bool lockBootloader(const QString& instanceId);
    
    /**
     * @brief Unlock bootloader
     */
    bool unlockBootloader(const QString& instanceId);
    
    // =========================================================================
    // DRM & Widevine
    // =========================================================================
    
    /**
     * @brief Set Widevine DRM level
     */
    bool setWidevineLevel(const QString& instanceId, DRMLevel level);
    
    /**
     * @brief Set HDCP compliance level
     */
    bool setHDCPLevel(const QString& instanceId, int level);
    
    /**
     * @brief Install Widevine keybox
     */
    bool installWidevineKeybox(const QString& instanceId);
    
    // =========================================================================
    // TEE Configuration
    // =========================================================================
    
    /**
     * @brief Configure TEE
     */
    bool configureTEE(const QString& instanceId, const QString& vendor, const QString& version);
    
    /**
     * @brief Set SE (Secure Element) present
     */
    bool setSecureElementPresent(const QString& instanceId, bool present);
    
    // =========================================================================
    // Hardware Properties
    // =========================================================================
    
    /**
     * @brief Get all security properties
     */
    QMap<QString, QString> getAllSecurityProperties(const QString& instanceId);
    
    /**
     * @brief Apply all security spoofing
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Verify attestation (simulated check)
     */
    QJsonObject verifyAttestation(const QString& instanceId);
    
    /**
     * @brief Reset to default secure state
     */
    bool resetSecurity(const QString& instanceId);
    
private:
    HardwareAttestation();
    
    QString keymasterVersionToString(KeymasterVersion version) const;
    QString verifiedBootStateToString(VerifiedBootState state) const;
    QString drmLevelToString(DRMLevel level) const;
    QString generateVerifiedBootHash(const QString& manufacturer, const QString& model);
    QString generateVerifiedBootKey();
    QString generateWidevineKeybox();
    
    HardwareSecurityState getDeviceDefaults(const QString& manufacturer, const QString& model);
    
    QMap<QString, HardwareSecurityState> m_securityStates;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_HARDWARE_ATTESTATION_H
