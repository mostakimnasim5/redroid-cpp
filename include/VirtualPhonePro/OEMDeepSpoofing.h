/**
 * @file OEMDeepSpoofing.h
 * @brief OEM-Specific Deep Spoofing
 * @version 2.0.0
 * 
 * Provides deep OEM-specific spoofing for:
 * - Samsung Knox / Samsung Pay
 * - Huawei HMS / AppGallery
 * - Xiaomi MIUI / Mi Pay
 * - Google Mobile Services
 * - OEM-specific build properties
 */

#pragma once

#ifndef VIRTUALPHONEPRO_OEM_DEEP_SPOOFING_H
#define VIRTUALPHONEPRO_OEM_DEEP_SPOOFING_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// OEM Types
enum class OEMType {
    SAMSUNG,
    GOOGLE,
    XIAOMI,
    HUAWEI,
    OPPO,
    VIVO,
    ONEPLUS,
    REALME,
    GENERIC
};

// Samsung Knox State
struct KnoxConfig {
    bool isKnoxEnabled;
    bool isKnoxSupported;
    bool isKnoxActive;
    int knoxVersion;
    QString knoxId;
    QString knoxContainerId;
    bool isSecurityPolicyEnforced;
    bool isSamsungPaySupported;
    bool isSamsungPassSupported;
    QString knoxLicenseStatus;
};

// Huawei HMS Configuration
struct HMSConfig {
    bool isHMSSupported;
    bool isHMSCoreInstalled;
    bool isAppGalleryAvailable;
    bool isHMSEnabled;
    QString hmsVersion;
    QString hmsClientId;
    bool isSafetyDetAvailable;
    bool isIntegrityAvailable;
};

// Xiaomi MIUI Configuration
struct MIUIConfig {
    bool isMIUISupported = false;
    bool isMIUIEnhanced = false;
    QString miuiVersion;
    QString miuiBuildVersion;
    bool isMiPaySupported = false;
    bool isGameTurboEnabled = false;
    bool isBatterySaverPlus = false;
    QString antutuVersion;
    QString oemId;       // MIUI version name (e.g. "V14.0")
    QString oemBrand;    // MIUI build version (e.g. "14.0.24")
};

// Google Services Configuration
struct GMSConfig {
    bool isGMSInstalled;
    bool isPlayStoreInstalled;
    bool isGooglePlayServicesInstalled;
    bool isSafetyNetAvailable;
    bool isPlayIntegrityAvailable;
    bool isDeviceCertificationInstalled;
    bool isGoogleBackupSupported;
    QString gmsVersion;
    QString playServicesVersion;
    QString playStoreVersion;
    QString playServicesVersionCode;
    QString deviceCountryCode;
    QString carrierId;
};

// Complete OEM State
struct OEMState {
    OEMType type;
    
    // OEM Identification
    QString oemId;
    QString oemName;
    QString oemBrand;
    QString oemModel;
    
    // Individual configs
    KnoxConfig samsung;
    HMSConfig huawei;
    MIUIConfig xiaomi;
    GMSConfig gms;
    
    bool allOEMFeaturesEnabled;
};

class OEMDeepSpoofing {
public:
    static OEMDeepSpoofing& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure OEM for device type
     */
    bool configureForOEM(const QString& instanceId, OEMType type);
    
    /**
     * @brief Configure Samsung
     */
    bool configureSamsung(const QString& instanceId);
    
    /**
     * @brief Configure Google
     */
    bool configureGoogle(const QString& instanceId);
    
    /**
     * @brief Configure Xiaomi
     */
    bool configureXiaomi(const QString& instanceId);
    
    /**
     * @brief Configure Huawei
     */
    bool configureHuawei(const QString& instanceId);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Samsung Knox
    // =========================================================================
    
    /**
     * @brief Enable Knox
     */
    bool enableKnox(const QString& instanceId);
    
    /**
     * @brief Disable Knox
     */
    bool disableKnox(const QString& instanceId);
    
    /**
     * @brief Set Knox container
     */
    bool setKnoxContainer(const QString& instanceId, const QString& containerId);
    
    // =========================================================================
    // Huawei HMS
    // =========================================================================
    
    /**
     * @brief Enable HMS
     */
    bool enableHMS(const QString& instanceId);
    
    /**
     * @brief Set HMS Integrity
     */
    bool setHMSIntegrity(const QString& instanceId, bool valid);
    
    // =========================================================================
    // Xiaomi MIUI
    // =========================================================================
    
    /**
     * @brief Enable MIUI features
     */
    bool enableMIUI(const QString& instanceId);
    
    /**
     * @brief Enable Game Turbo
     */
    bool enableGameTurbo(const QString& instanceId, bool enabled);
    
    // =========================================================================
    // Google Services
    // =========================================================================
    
    /**
     * @brief Enable full GMS
     */
    bool enableFullGMS(const QString& instanceId);
    
    /**
     * @brief Set Google Play certification
     */
    bool setDeviceCertification(const QString& instanceId, bool certified);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get OEM state
     */
    OEMState getOEMState(const QString& instanceId) const;
    
    /**
     * @brief Get all OEM properties
     */
    QMap<QString, QString> getAllOEMProperties(const QString& instanceId);
    
    /**
     * @brief Apply all OEM spoofing
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Reset OEM
     */
    bool resetOEM(const QString& instanceId);
    
private:
    static OEMDeepSpoofing* s_instance;
    OEMDeepSpoofing();
    static OEMDeepSpoofing* s_instance;
    
    OEMState getDefaultsForType(OEMType type) const;
    QString oemTypeToString(OEMType type) const;
    
    QMap<QString, OEMState> m_oemStates;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_OEM_DEEP_SPOOFING_H
