#pragma once

#ifndef VIRTUALPHONEPRO_DEVICE_PROFILE_H
#define VIRTUALPHONEPRO_DEVICE_PROFILE_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QVariantMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

/**
 * @brief Device Identity Information
 * 
 * Contains all device identification data for spoofing
 */
struct DeviceIdentity {
    QString imei;              // 15-digit IMEI with Luhn check digit
    QString imei2;            // Second IMEI for dual-SIM devices
    QString serialNumber;      // Manufacturer-specific serial format
    QString androidId;         // 16-character hex Android ID
    QString gsfId;            // 10-digit Google Services Framework ID
    QString advertisingId;     // UUID-format advertising ID

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief MAC Addresses for network interfaces
 */
struct MACAddresses {
    QString wifiMac;           // WiFi MAC address (XX:XX:XX:XX:XX:XX)
    QString bluetoothMac;      // Bluetooth MAC address
    QString ethernetMac;       // Ethernet MAC address

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Build Information (Android Build Properties)
 */
struct BuildInfo {
    QString brand;             // e.g., "samsung"
    QString manufacturer;      // e.g., "Samsung Electronics"
    QString model;            // e.g., "SM-S928B"
    QString device;           // e.g., "dm3q" (codename)
    QString product;          // e.g., "dm3q"
    QString board;            // e.g., "dm3q"
    QString hardware;         // e.g., "qcom"
    
    QString fingerprint;       // Full Android fingerprint
    QString bootloader;       // e.g., "S928BXXU1AXXX"
    QString buildId;          // e.g., "UP1A.231005.007"
    QString buildType;         // e.g., "userdebug" or "user"
    QString securityPatch;     // e.g., "2024-01-01"
    
    int androidVersion;       // e.g., 14
    int sdkVersion;           // e.g., 34

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Network Configuration
 */
struct NetworkConfig {
    QString hostname;          // e.g., "android-dm3q"
    QString wifiMac;
    QString bluetoothMac;
    QString ethernetMac;
    
    QString ipAddress;         // e.g., "192.168.1.100"
    QString dns1;             // e.g., "8.8.8.8"
    QString dns2;             // e.g., "8.8.4.4"

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief SIM Card Configuration
 */
struct SIMConfig {
    QString iccid;            // 20-digit ICCID
    QString imsi;            // 15-digit IMSI
    QString carrier;          // e.g., "T-Mobile"
    QString country;          // e.g., "US"
    QString mcc;             // Mobile Country Code (3 digits)
    QString mnc;             // Mobile Network Code (2-3 digits)

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief GPS Location Configuration
 */
struct GPSConfig {
    double latitude;          // e.g., 37.7749
    double longitude;         // e.g., -122.4194
    double altitude;          // e.g., 10.0 (meters)
    float accuracy;           // e.g., 5.0 (meters)
    float speed;              // e.g., 0.0 (m/s)
    float bearing;            // e.g., 0.0 (degrees)
    QString provider;         // e.g., "gps" or "network"

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Sensor Calibration Data
 */
struct SensorCalibration {
    QString name;             // e.g., "LSM6DSO Accelerometer"
    QString vendor;          // e.g., "STMicroelectronics"
    int version;              // e.g., 1
    float resolution;         // e.g., 0.001196
    float maxRange;          // e.g., 78.4532

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Hardware Specifications
 */
struct HardwareInfo {
    QString cpuArchitecture;      // e.g., "arm64-v8a"
    QString processor;            // e.g., "ARM Implementer 88"
    QString hardware;             // e.g., "qcom"
    QString modelName;           // e.g., "Snapdragon 8 Gen 3"
    int coreCount;                // e.g., 8
    
    QString gpuRenderer;          // e.g., "Adreno (TM) 750"
    QString gpuVendor;            // e.g., "Qualcomm"
    QString openGlVersion;        // e.g., "3.2"
    QString vulkanVersion;        // e.g., "1.1.269"

    quint64 totalRam;             // e.g., 12288000000 (bytes)
    quint64 heapSize;             // e.g., 512000000 (bytes)
    quint64 largeHeapSize;         // e.g., 4096000000 (bytes)

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Security Configuration
 */
struct SecurityConfig {
    QString selinuxMode;         // "enforcing" or "permissive"
    QString verifiedBootState;    // "green", "yellow", "orange", "red"
    bool flashLocked;            // true = locked, false = unlocked
    int keymasterVersion;         // e.g., 4
    bool strongboxAvailable;      // StrongBox keystore available
    bool hasHardwareAttestation; // Hardware attestation support

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Complete Device Profile
 * 
 * Encapsulates all device identity and configuration data
 */
class DeviceProfile {
public:
    // Profile metadata
    QString id;                  // UUID v4
    QString name;                // e.g., "Samsung Galaxy S24 Ultra"
    QString manufacturer;        // e.g., "Samsung"
    QString version;             // Profile version
    QStringList tags;           // e.g., ["flagship", "5g"]
    
    // Profile data
    DeviceIdentity identity;
    MACAddresses mac;
    BuildInfo build;
    NetworkConfig network;
    SIMConfig sim;
    GPSConfig gps;
    HardwareInfo hardware;
    SecurityConfig security;
    
    // Metadata
    QString createdAt;
    QString modifiedAt;
    QString createdBy;

    // Runtime assignment
    int adbPort;                // ADB port (e.g., 5555)
    int vncPort;                // VNC port (e.g., 5900)
    int instanceIndex;          // Zero-based instance index

public:
    DeviceProfile();
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // File operations
    bool save(const QString& filePath) const;
    static DeviceProfile load(const QString& filePath);
    
    // Validation
    bool isValid() const;
    QStringList validationErrors() const;
    
    // Factory methods
    static DeviceProfile createSamsungS24Ultra();
    static DeviceProfile createGooglePixel8Pro();
    static DeviceProfile createRandom();
    
    // Property generation helpers
    static QString generateIMEI(const QString& tac);
    static QString generateSerial(const QString& manufacturer);
    static QString generateAndroidId();
    static QString generateMAC(const QString& oui = QString());
    static bool validateIMEI(const QString& imei);
};

/**
 * @brief Instance Runtime State
 */
enum class InstanceState {
    Stopped,
    Starting,
    Running,
    Paused,
    Error
};

inline InstanceState stateFromString(const QString& str) {
    if (str == "running" || str == "Up") return InstanceState::Running;
    if (str == "paused") return InstanceState::Paused;
    if (str == "starting") return InstanceState::Starting;
    if (str == "error") return InstanceState::Error;
    return InstanceState::Stopped;
}

inline QString stateToString(InstanceState state) {
    switch (state) {
        case InstanceState::Running: return "running";
        case InstanceState::Paused: return "paused";
        case InstanceState::Starting: return "starting";
        case InstanceState::Error: return "error";
        default: return "stopped";
    }
}

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_DEVICE_PROFILE_H
