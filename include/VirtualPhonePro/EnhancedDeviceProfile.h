/**
 * @file EnhancedDeviceProfile.h
 * @brief Enhanced Device Profile for Banking App Testing
 * @version 2.0.0
 * 
 * Comprehensive device profile with ALL fields needed for 100% realistic
 * banking and security app testing without detection.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_ENHANCED_DEVICE_PROFILE_H
#define VIRTUALPHONEPRO_ENHANCED_DEVICE_PROFILE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>

namespace VirtualPhonePro {

// ========================================================================
// Enhanced Device Identity
// ========================================================================

struct EnhancedIdentity {
    // Device IDs
    QString imei;                    // 15 digits
    QString imei2;                   // For dual SIM
    QString meid;                   // For CDMA
    QString serialNumber;             // Manufacturer-specific
    QString androidId;               // 16 hex chars
    QString gsfId;                  // Google Services Framework
    QString advertisingId;          // AAID
    QString wlanMacAddress;          // WiFi MAC
    
    // SIM Information
    QString iccid;                  // SIM card ID
    QString imsi;                   // Subscriber ID
    QString simOperator;             // MCC + MNC
    QString simOperatorName;        // Carrier name
    QString simCountry;              // SIM country
    
    // Generate all
    void generateAll(const QString& manufacturer);
    bool validateAll() const;
};

// ========================================================================
// Enhanced Build Information  
// ========================================================================

struct EnhancedBuildInfo {
    // Brand & Manufacturer
    QString brand;                  // e.g., "samsung"
    QString manufacturer;          // e.g., "samsung electronics"
    QString device;                // e.g., "dm3q"
    QString product;               // e.g., "dm3q"
    QString board;                  // e.g., "dm3q"
    QString hardware;              // e.g., "qcom"
    
    // Model
    QString model;                // e.g., "SM-S928B"
    QString deviceName;            // e.g., "Galaxy S24 Ultra"
    
    // Build
    QString fingerprint;          // Full fingerprint
    QString bootloader;            // e.g., "S928BXXU1AXXX"
    QString buildId;              // e.g., "UP1A.231005.007"
    QString buildType;            // "user" or "userdebug"
    QString buildTags;             // "release-keys"
    QString buildDescription;      // Build description
    
    // Version
    int androidVersion;           // e.g., 14
    int sdkVersion;               // e.g., 34
    int securityPatchLevel;       // e.g., 2024-01-01
    
    // Dates
    QString buildDate;           // Build date
    QString buildTime;            // Build time
    
    // Generate
    void generateForDevice(const QString& manufacturer, const QString& model);
};

// ========================================================================
// Hardware Information
// ========================================================================

struct EnhancedHardwareInfo {
    // CPU
    QString cpuArchitecture;       // "arm64-v8a"
    QStringList cpuAbiList;        // Supported ABIs
    QString processor;              // e.g., "ARM Implementer 88"
    QString hardware;              // e.g., "qcom"
    QString board;                 // e.g., "taro"
    
    // CPU Cores
    uint32_t coreCount;           // Total cores
    uint32_t coreCountBig;         // Performance cores
    uint32_t coreCountMid;         // Medium cores
    uint32_t coreCountLittle;      // Efficiency cores
    
    // CPU Frequencies
    uint64_t cpuMaxFreq;          // Max frequency in Hz
    uint64_t cpuMinFreq;          // Min frequency in Hz
    double bogoMips;               // BogoMIPS
    
    // Features
    bool hasAES;                  // AES-NI
    bool hasNEON;                  // NEON SIMD
    bool hasVFPv4;                // VFPv4
    bool hasARMv8;                 // ARMv8 support
    
    // GPU
    QString gpuRenderer;           // e.g., "Adreno (TM) 750"
    QString gpuVendor;             // e.g., "Qualcomm"
    QString gpuVersion;           // OpenGL ES version
    QString vulkanVersion;        // Vulkan version
    
    // Memory
    quint64 totalRAM;             // Total RAM in bytes
    quint64 totalRAMMB;           // Total RAM in MB
    quint64 heapSize;             // Max heap size
    quint64 largeHeapSize;         // Large heap size
    
    // Generate
    void generateForDevice(const QString& manufacturer);
};

// ========================================================================
// Network Information
// ========================================================================

struct EnhancedNetworkInfo {
    // Hostname
    QString hostname;              // e.g., "android-dm3q"
    
    // MAC Addresses
    QString wifiMac;              // WiFi MAC
    QString bluetoothMac;         // Bluetooth MAC
    QString ethernetMac;           // Ethernet MAC
    
    // IP
    QString ipAddress;            // Device IP
    QString subnetMask;           // e.g., "255.255.255.0"
    QString gateway;              // Gateway IP
    QString dns1;                 // Primary DNS
    QString dns2;                 // Secondary DNS
    
    // Network Type
    QString networkType;          // "LTE", "5G", "WIFI"
    QString mobileCountryCode;     // MCC
    QString mobileNetworkCode;     // MNC
    
    // Generate
    void generateAll();
};

// ========================================================================
// Battery Information
// ========================================================================

struct EnhancedBatteryInfo {
    int level;                    // 0-100
    int health;                  // 1=Good, 2=Overheat, etc.
    int temperature;              // Temperature in deciCelsius
    int voltage;                 // Voltage in mV
    int status;                   // 2=Charging, etc.
    QString technology;          // e.g., "Li-ion"
    bool isCharging;             // true if plugged in
    bool isAc;                   // AC charging
    bool isUsb;                 // USB charging
    bool isWireless;            // Wireless charging
    
    // Generate
    void generateAll();
};

// ========================================================================
// Sensor Information
// ========================================================================

struct EnhancedSensorInfo {
    // Accelerometer
    struct SensorData {
        QString name;
        QString vendor;
        QString version;
        float resolution;
        float maxRange;
        float power;
        int minDelay;
    };
    
    SensorData accelerometer;
    SensorData gyroscope;
    SensorData magnetometer;
    SensorData proximity;
    SensorData light;
    SensorData pressure;
    SensorData stepCounter;
    SensorData heartRate;
    
    // Generate
    void generateAll(const QString& manufacturer);
};

// ========================================================================
// Security Information
// ========================================================================

struct EnhancedSecurityInfo {
    // SELinux
    QString selinuxMode;          // "Enforcing" or "Permissive"
    
    // Verified Boot
    QString verifiedBootState;     // "green", "yellow", "orange", "red"
    QString verifiedBootLocked;   // "locked" or "unlocked"
    QString verifiedBootKeyHash; // VBMeta digest
    
    // Keymaster
    int keymasterVersion;         // e.g., 4
    QString keymasterSecurityLevel; // "SOFTWARE", "TRUSTED_ENVIRONMENT", "STRONGBOX"
    
    // StrongBox
    bool hasStrongbox;            // StrongBox available
    int strongboxVersion;         // StrongBox version
    
    // Gatekeeper
    int gatekeeperVersion;        // Gatekeeper version
    QString gatekeeperSecurityLevel;
    
    // Hardware
    bool hasHardwareAttestation;  // Hardware attestation
    bool hasSecureHardware;       // Secure hardware
    
    // Crypto
    QString cryptoSupported;      // "AES-256-GCM"
    bool hardwareBackedKeys;       // Hardware-backed keys
    
    // Root/Unlock
    bool isRooted;                // false for banking
    QString rootAccess;           // "0" = no root
    bool oemUnlockEnabled;         // false for banking
    
    // Generate
    void generateAll();
};

// ========================================================================
// Display Information
// ========================================================================

struct EnhancedDisplayInfo {
    uint32_t widthPixels;
    uint32_t heightPixels;
    uint32_t densityDpi;
    float densityValue;
    uint32_t refreshRate;
    
    QString hdrCapabilities;        // HDR support
    QString wideColorGamut;       // WCG support
    
    uint32_t densityBucket;      // MDPI, HDPI, XHDPI, etc.
    uint32_t smallestWidth;      // sw<N>dp
    
    // Generate
    void generateForDevice(const QString& deviceClass);
};

// ========================================================================
// Camera Information
// ========================================================================

struct EnhancedCameraInfo {
    QString frontCamera;          // Front camera model
    QString backCamera;           // Back camera model
    
    int frontResolution;          // MP
    int backResolution;           // MP
    
    bool hasFlash;               // LED flash
    bool hasAutofocus;           // Autofocus
    bool hasOIS;                 // Optical Image Stabilization
    
    // Generate
    void generateAll(const QString& manufacturer);
};

// ========================================================================
// Bluetooth Information
// ========================================================================

struct EnhancedBluetoothInfo {
    QString name;                 // Bluetooth device name
    QString address;              // Bluetooth MAC
    QString version;             // BT version (5.0, 5.1, etc.)
    bool isEnabled;               // BT enabled
    
    // Generate
    void generateAll(const QString& manufacturer);
};

// ========================================================================
// NFC Information
// ========================================================================

struct EnhancedNFCInfo {
    bool hasNFC;                  // NFC available
    bool isNFCEnabled;            // NFC enabled
    QString nfcController;       // NFC chip
    QString nfcVersion;          // NFC version
    
    // Generate
    void generateAll(const QString& manufacturer);
};

// ========================================================================
// Complete Enhanced Device Profile
// ========================================================================

class EnhancedDeviceProfile {
public:
    // Metadata
    QString id;                 // UUID
    QString name;                // Profile name
    QString manufacturer;        // e.g., "Samsung"
    QString deviceClass;          // e.g., "flagship"
    QString createdAt;
    
    // All components
    EnhancedIdentity identity;
    EnhancedBuildInfo build;
    EnhancedHardwareInfo hardware;
    EnhancedNetworkInfo network;
    EnhancedBatteryInfo battery;
    EnhancedSensorInfo sensors;
    EnhancedSecurityInfo security;
    EnhancedDisplayInfo display;
    EnhancedCameraInfo camera;
    EnhancedBluetoothInfo bluetooth;
    EnhancedNFCInfo nfc;
    
    // Runtime
    int adbPort;
    int vncPort;
    
public:
    EnhancedDeviceProfile();
    
    // Factory methods
    static EnhancedDeviceProfile createForBanking(const QString& manufacturer);
    static EnhancedDeviceProfile createForTesting(const QString& manufacturer);
    
    // Generate complete profile
    void generateComplete(const QString& manufacturer, const QString& model);
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Validation
    bool isValidForBanking() const;
    QStringList getValidationErrors() const;
    
    // Export all properties as ADB commands
    QStringList getAdbCommands() const;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_ENHANCED_DEVICE_PROFILE_H
