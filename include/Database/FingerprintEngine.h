/**
 * @file FingerprintEngine.h
 * @brief Seed-Based Deterministic Fingerprint Generation Engine
 * @version 2.0.0
 * 
 * Generates unique, non-colliding, deterministic hardware fingerprints
 * from a single UUID seed. Each profile is 100% unique.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_FINGERPRINT_ENGINE_H
#define VIRTUALPHONEPRO_FINGERPRINT_ENGINE_H

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>
#include <QMutex>

namespace VirtualPhonePro {

/**
 * @brief Complete fingerprint set for one profile
 */
struct DeviceFingerprint {
    // Identity
    QString uuid;                    // Primary seed (UUID v4)
    QString profileId;               // Derived profile ID
    
    // IMEI (15 digits with Luhn check digit)
    QString imei1;                  // Primary SIM
    QString imei2;                  // Secondary SIM (dual-SIM)
    
    // Android Identity
    QString androidId;              // 16-char hex
    QString gsfId;                  // Google Services Framework ID (10 digits)
    QString serialNumber;           // Manufacturer serial
    QString bootloaderSerial;       // Bootloader serial
    
    // MAC Addresses
    QString wifiMac;               // XX:XX:XX:XX:XX:XX
    QString bluetoothMac;          // XX:XX:XX:XX:XX:XX
    QString ethernetMac;           // XX:XX:XX:XX:XX:XX
    
    // SIM Cards
    QString iccid1;                 // ICCID SIM 1 (20 digits)
    QString iccid2;                 // ICCID SIM 2
    QString imsi1;                  // IMSI SIM 1 (15 digits)
    QString imsi2;                  // IMSI SIM 2
    
    // Hardware Hashes
    QString hardwareId;              // Hardware platform hash
    QString deviceKey;              // Device encryption key hash
    QString vendorId;               // OEM vendor ID
    
    // Build Fingerprint Components
    QString fingerprint;            // Complete android.os.Build fingerprint
    QString buildId;               // Build ID (e.g., UP1A.231005.007)
    QString bootloader;            // Bootloader version
    QString radioVersion;           // Baseband/Radio version
    
    // Secure Random Data
    QString secureId;              // Secure random identifier
    QString authToken;             // Authentication token
    QString googleServicesKey;      // GMS API key hash
    
    // TAC Database
    QString tac;                   // Type Allocation Code (8 digits)
    QString modelCode;             // Internal model code
    
    // Metadata
    qint64 generatedAt;
    QString algorithmVersion;
    quint32 checksum;
};

/**
 * @brief Fingerprint Generation Configuration
 */
struct FingerprintConfig {
    QString manufacturer;
    QString model;
    QString brand;
    QString device;
    QString product;
    
    int androidVersion;           // e.g., 14
    int sdkVersion;              // e.g., 34
    QString securityPatch;        // e.g., "2024-01-01"
    QString buildType;            // "user" or "userdebug"
    
    QString country;              // Device country
    QString language;             // Device language
    QString carrier;              // Default carrier
    
    bool isDualSim;
    bool hasGoogleServices;
};

/**
 * @brief FingerprintEngine - Deterministic Seed-Based Generator
 * 
 * Uses cryptographic hash functions to generate unique fingerprints
 * from a single UUID seed, ensuring 100% collision-free profiles.
 */
class FingerprintEngine {
public:
    static FingerprintEngine& instance();
    
    // =========================================================================
    // Generation
    // =========================================================================
    
    /**
     * @brief Generate complete fingerprint from seed
     * @param seed UUID v4 seed for deterministic generation
     * @param config Device configuration
     * @return Complete unique fingerprint
     */
    DeviceFingerprint generateFingerprint(const QString& seed, const FingerprintConfig& config);
    
    /**
     * @brief Generate from seed string
     */
    DeviceFingerprint generateFromSeed(const QString& seedString);
    
    /**
     * @brief Regenerate existing fingerprint with same seed
     * @param seed Original seed
     * @param config Same config used originally
     * @return Same fingerprint (deterministic)
     */
    DeviceFingerprint regenerateFingerprint(const QString& seed, const FingerprintConfig& config);
    
    // =========================================================================
    // Validation
    // =========================================================================
    
    /**
     * @brief Validate IMEI with Luhn algorithm
     */
    bool validateIMEI(const QString& imei) const;
    
    /**
     * @brief Validate IMSI format
     */
    bool validateIMSI(const QString& imsi) const;
    
    /**
     * @brief Validate ICCID format
     */
    bool validateICCID(const QString& iccid) const;
    
    /**
     * @brief Check for fingerprint collision
     */
    bool checkCollision(const DeviceFingerprint& fp) const;
    
    // =========================================================================
    // Serialization
    // =========================================================================
    
    QJsonObject toJson(const DeviceFingerprint& fp) const;
    DeviceFingerprint fromJson(const QJsonObject& json) const;
    QByteArray toBinary(const DeviceFingerprint& fp) const;
    DeviceFingerprint fromBinary(const QByteArray& data) const;
    
    // =========================================================================
    // Utilities
    // =========================================================================
    
    QString generateSeed();
    quint32 calculateChecksum(const DeviceFingerprint& fp) const;
    QString hashToHex(const QByteArray& data) const;
    
private:
    FingerprintEngine();
    Q_DISABLE_COPY(FingerprintEngine)
    
    // Internal generation methods
    QString deriveFromSeed(const QString& seed, const QString& purpose, int length);
    QByteArray hmacSha256(const QString& key, const QString& data) const;
    QString generateIMEI(const QString& seed, const QString& tac);
    QString generateIMEI2(const QString& seed);
    QString generateIMSI(const QString& seed, const QString& mcc, const QString& mnc);
    QString generateICCID(const QString& seed, const QString& mcc);
    QString generateMAC(const QString& seed, const QString& prefix);
    QString generateSerial(const QString& seed, const QString& manufacturer);
    QString generateAndroidId(const QString& seed);
    QString generateGsFId(const QString& seed);
    QString generateFingerprintString(const DeviceFingerprint& fp, const FingerprintConfig& config);
    QString calculateLuhnCheckDigit(const QString& base) const;
    
    // Collision database
    QMap<QString, quint32> m_imeiIndex;
    QMap<QString, quint32> m_androidIdIndex;
    QMap<QString, quint32> m_serialIndex;
    QMap<QString, quint32> m_wifiMacIndex;
    QMutex m_mutex;
    
    static constexpr const char* ALGORITHM_VERSION = "2.0.0";
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_FINGERPRINT_ENGINE_H
