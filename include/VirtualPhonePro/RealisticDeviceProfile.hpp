/**
 * @file RealisticDeviceProfile.hpp
 * @brief 100% Realistic Device Profile Generator
 * @version 4.0.0
 * 
 * Generates COMPLETE realistic Android device profiles with ALL properties
 * that a real device would have. Makes detection 100% impossible.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_REALISTIC_DEVICE_PROFILE_H
#define VIRTUALPHONEPRO_REALISTIC_DEVICE_PROFILE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QDateTime>

namespace VirtualPhonePro {

// ========================================================================
// COMPLETE DEVICE IDENTITIES
// ========================================================================

struct CompleteDeviceIdentity {
    // Primary Identifiers
    QString imei;                    // 15 digits with Luhn check
    QString imei2;                  // Dual SIM support
    QString meid;                   // CDMA MEID (14 hex digits)
    QString serialNumber;            // Manufacturer-specific
    
    // Google/Android Identifiers
    QString androidId;              // 16 hex chars
    QString gsfId;                  // Google Services Framework ID
    QString advertisingId;           // Google Advertising ID (AAID)
    QString fireAdvertisingId;        // Amazon Fire advertising ID
    
    // Device Keys
    QString deviceKey;               // Crypto device key
    QString deviceCredential;        // Device credential hash
    QString androidVnci;            // VNC identifier
    
    // WLAN Driver ID
    QString wlanMac;                // WiFi MAC
    QString bluetoothMac;           // Bluetooth MAC
    QString ethernetMac;            // Ethernet MAC
    QString nfcMac;                // NFC MAC
    
    // SIM/eSIM
    QString iccid1;                 // Primary SIM ICCID
    QString iccid2;                // Secondary SIM ICCID
    QString imsi1;                 // Primary IMSI
    QString imsi2;                 // Secondary IMSI
    
    // OEM Identifiers
    QString KnoxId;                 // Samsung KNOX ID
    QString KnoxVersion;            // Samsung KNOX version
    QString spBleDeviceId;         // Samsung BLE device ID
    QString samsungAccountId;       // Samsung account
    
    // Apple-like Identifiers (for variety)
    QString oemSpecificId;          // OEM-specific identifier
    QString secureId;               // Secure device ID
    QString vendorId;               // Vendor-specific ID
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE CARRIER CONFIGURATION
// ========================================================================

struct CompleteCarrierConfig {
    // Primary SIM
    QString carrierName;            // e.g., "T-Mobile"
    QString carrierCountry;         // ISO country code
    QString carrierId;             // Internal carrier ID
    QString carrierType;            // postpaid, prepaid
    
    // MCC/MNC
    QString mcc;                   // Mobile Country Code (3 digits)
    QString mnc;                   // Mobile Network Code (2-3 digits)
    QString mncLength;             // MNC length
    
    // SIM Info
    QString simOperatorName;        // SIM operator
    QString simCountry;             // SIM country
    QString simSerialNumber;       // SIM serial (CN)
    
    // Network Type
    QString networkType;           // LTE, 5G, NR
    QString phoneType;             // GSM, CDMA, LTE
    QString dataNetworkType;        // e.g., "LTE_CA"
    
    // VoLTE/VoWiFi
    bool volteEnabled;              // Voice over LTE
    bool vowifiEnabled;             // Voice over WiFi
    bool wifiCallingEnabled;
    
    // Carrier-specific
    QString carrierSpecificKey;    // Carrier-specific key
    QString carrierSpecificValue;   // Carrier-specific value
    
    // IMS Configuration
    QString imsPrivateUri;         // IMS private URI
    QString imsPublicUri;          // IMS public URI
    QString mmtelImsi;             // MMTel IMSI
    
    // APN Settings
    QString apnName;               // Access Point Name
    QString apn;                   // APN URL
    QString mmsc;                  // MMS Proxy
    QString mmsProxy;              // MMS Proxy address
    int mmsPort;                   // MMS Port
    QString mccApn;               // APN MCC
    QString mncApn;                // APN MNC
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE HARDWARE SPECIFICATIONS
// ========================================================================

struct CompleteHardwareSpec {
    // CPU/Processor
    QString processor;             // e.g., "ARMv8 Processor"
    QString cpuAbi;               // Primary ABI (arm64-v8a)
    QString cpuAbi2;              // Secondary ABI (armeabi-v7a)
    QString hardware;              // Hardware name (qcom, exynos)
    QString board;                // Board name
    QString systemOnChip;         // SoC model
    
    // CPU Details
    int coreCount;                // Total cores (8)
    int bigCoreCount;             // Performance cores
    int midCoreCount;             // Medium cores
    int littleCoreCount;          // Efficiency cores
    int coreFreqMax;              // Max frequency (MHz)
    int coreFreqMin;              // Min frequency (MHz)
    int cpuGovernor;              // CPU governor
    
    // BogoMIPS
    double bogoMips;              // BogoMIPS value
    int maxCpuFreq;               // Max CPU frequency
    int minCpuFreq;               // Min CPU frequency
    
    // GPU
    QString gpuVendor;            // e.g., "Qualcomm"
    QString gpuRenderer;          // e.g., "Adreno (TM) 750"
    QString gpuVersion;           // GPU driver version
    QString gpuExtensions;         // GPU extensions
    
    // Vulkan
    QString vulkanVersion;        // Vulkan API version
    QString vulkanConformance;    // Conformance version
    
    // OpenGL ES
    QString glEsVersion;          // e.g., "3.2"
    QString glEsRenderer;         // Full renderer
    QString glVendor;             // GL Vendor
    
    // Memory
    quint64 totalRam;             // Total RAM in bytes
    quint64 totalRamMB;           // RAM in MB
    quint64 totalRamGB;           // RAM in GB
    quint64 heapSize;             // Max heap size
    quint64 largeHeapSize;       // Large heap size
    quint64 lowMemory;            // Low memory threshold
    int memoryPageSize;           // Page size
    
    // Storage
    quint64 internalStorage;      // Internal storage
    quint64 externalStorage;       // External storage
    QString sdCardId;            // SD Card identifier
    
    // Battery
    int batteryCapacity;          // mAh
    int batteryLevel;             // Current level %
    int batteryTemp;              // Temperature (Celsius * 10)
    int batteryVoltage;           // Voltage (mV)
    int batteryCurrent;            // Current (mA)
    QString batteryStatus;        // charging, discharging
    QString batteryHealth;        // health, overheat, dead
    QString batteryTechnology;    // Li-ion, Li-poly
    QString plugged;             // usb, ac, wireless
    
    // Display
    int displayWidth;             // Pixels
    int displayHeight;            // Pixels
    int densityDpi;              // DPI
    float density;                // Density
    int refreshRate;             // Hz
    int hdrRefreshRate;           // HDR refresh rate
    QString hdrCapabilities;      // HDR10, DolbyVision, etc.
    QString wideColorGamut;       // Color gamut
    QString colorMode;           // natural, saturated, cinema
    
    // Audio
    QString audioCodec;           // Audio codec
    QString audioOutput;         // Audio output devices
    QString audioEffect;          // Audio effects
    int speakerCount;             // Number of speakers
    
    // Camera
    QString frontCamera;          // Front camera specs
    QString backCamera;           // Back camera specs
    int cameraCount;              // Number of cameras
    QString cameraFeatures;        // Camera features
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE BUILD INFORMATION
// ========================================================================

struct CompleteBuildInfo {
    // Basic Build
    QString brand;                 // e.g., "samsung"
    QString manufacturer;          // e.g., "samsung"
    QString model;               // e.g., "SM-S928B"
    QString device;              // Codename (e.g., "dm3q")
    QString product;             // Product name
    QString board;               // Board name
    
    // Fingerprint
    QString fingerprint;          // Full Android fingerprint
    QString bootimageFingerprint; // Boot image fingerprint
    
    // Version Info
    QString androidVersion;       // e.g., "14"
    QString releaseVersion;       // Codename (UpsideDownCake)
    QString sdkVersion;          // API level (34)
    QString securityPatch;       // Security patch date
    QString buildDescription;     // Build description
    
    // Build IDs
    QString buildId;             // Build ID
    QString buildDisplay;         // Display ID
    QString buildType;            // user, userdebug, eng
    QString buildTags;            // release-keys, dev-keys
    
    // Bootloader/Radio
    QString bootloader;           // Bootloader version
    QString radioVersion;         // Baseband version
    QString baseband;            // Baseband
    
    // Time/Date
    QString buildDate;           // Build date
    int buildDateTime;          // Unix timestamp
    int buildTimeSeconds;        // Seconds since boot
    
    // Hardware Info
    QString hardwareRevision;     // Hardware revision
    QString deviceRevision;       // Device revision
    
    // First/Last API
    int firstApiLevel;           // First API level
    int targetApiLevel;          // Target API level
    
    // OTA
    QString otaUpdates;           // OTAcertificates
    QString otaStatus;           // OTA status
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE SECURITY CONFIGURATION
// ========================================================================

struct CompleteSecurityConfig {
    // SELinux
    QString selinuxStatus;        // Enforcing, Permissive
    QString selinuxEnforce;       // Enforcing, Permissive
    QString selinuxMode;          // Enforcing, Permissive
    
    // Verified Boot
    QString verifiedBootState;    // green, yellow, orange, red
    bool verifiedBootLocked;      // true/false
    QString verificationBoot;     // verification enabled
    QString roBootloader;         // Bootloader state
    
    // VBMeta
    QString vbmetaDigest;         // VBMeta digest
    QString vbmetaVersion;        // VBMeta version
    QString vbmetaAlgorithm;      // Algorithm used
    
    // Keymaster/Keystore
    int keymasterVersion;         // Keymaster version
    QString keymasterSecurityLevel; // Security level
    QString gatekeeperVersion;    // Gatekeeper version
    QString gatekeeperSecurityLevel;
    
    // StrongBox
    bool hasStrongbox;            // StrongBox available
    int strongboxVersion;         // StrongBox version
    QString strongboxSecurityLevel;
    
    // Trusty TEE
    QString trustyVersion;        // Trusty TEE version
    bool hasTrusty;               // Trusty available
    
    // Hardware Attestation
    QString attestationEnabled;    // Attestation enabled
    QString attestationStatus;     // Attestation status
    bool hasHardwareAttestation;   // Hardware attestation
    
    // DRM/Widevine
    QString widevineLevel;        // L1, L2, L3
    QString drmSecurityLevel;     // DRM security
    QString hdcpLevel;           // HDCP level
    
    // Encryption
    QString fileBasedEncryption;   // FBE supported
    QString fullDiskEncryption;   // FDE supported
    bool isEncrypted;             // Device encrypted
    
    // Samsung Knox
    QString knoxVersion;          // KNOX version
    QString knoxId;              // KNOX ID (32 hex)
    bool isKnoxDevice;            // Samsung Knox device
    QString odeMode;             // ODE mode
    QString secureStorageStatus;  // Secure storage status
    
    // Huawei
    QString huaweiDevice;         // Huawei device
    QString huaweiSecPlatform;   // Security platform
    
    // Xiaomi
    bool isMiuiDevice;           // Xiaomi MIUI device
    QString miuiVersion;         // MIUI version
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE SENSOR CALIBRATION
// ========================================================================

struct CompleteSensorCalibration {
    // Accelerometer
    struct SensorCal {
        QString name;
        QString vendor;
        int version;
        float resolution;
        float maxRange;
        int power;               // mA
        float minDelay;          // us
        QString type;            // Sensor type
        QString typeString;      // Type as string
    };
    
    SensorCal accelerometer;
    SensorCal gyroscope;
    SensorCal magnetometer;
    SensorCal barometer;
    SensorCal light;
    SensorCal proximity;
    SensorCal heartRate;
    SensorCal stepCounter;
    SensorCal gameRotation;
    SensorCal rotationVector;
    SensorCal gravity;
    SensorCal linearAcceleration;
    SensorCal orientation;
    
    // Fingerprint
    struct FingerprintCal {
        QString name;
        QString vendor;
        int version;
        QString type;           // UNDER_DISPLAY, REAR_MOUNTED, etc.
        QString strength;        // STRONG, WEAK
        int sensorId;
    };
    FingerprintCal fingerprint;
    
    // Face Recognition
    struct FaceCal {
        QString name;
        QString vendor;
        bool enrolled;
        int faceCount;
        bool supportsIris;
    };
    FaceCal faceRecognition;
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE NETWORK CONFIGURATION
// ========================================================================

struct CompleteNetworkConfig {
    // Hostname
    QString hostname;              // Device hostname
    QString dhcpHostname;         // DHCP hostname
    
    // IP Configuration
    QString ipAddress;            // Current IP
    QString wifiIpAddress;        // WiFi IP
    QString mobileIpAddress;      // Mobile data IP
    QString subnetMask;           // Subnet mask
    QString gateway;              // Gateway
    QString dns1;                // Primary DNS
    QString dns2;                // Secondary DNS
    
    // WiFi
    QString wifiSSID;            // Connected SSID
    QString wifiBSSID;           // Access point MAC
    QString wifiSignal;          // Signal strength
    int wifiLinkSpeed;           // Link speed (Mbps)
    int wifiFrequency;           // Frequency (MHz)
    QString wifiDriver;          // WiFi driver
    QString wifiChipset;        // WiFi chipset
    
    // Proxy
    QString proxyHost;           // Proxy host
    int proxyPort;               // Proxy port
    QString proxyExclusionList;  // Bypass list
    
    // VPN
    QString vpnInterface;         // VPN interface
    QString vpnAddress;          // VPN IP
    QString vpnGateway;          // VPN gateway
    
    // Network Scores
    int networkScore;            // Network quality score
    int connectivityScore;       // Connectivity score
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE LOCATION/GPS CONFIGURATION
// ========================================================================

struct CompleteLocationConfig {
    // GPS Settings
    bool gpsEnabled;              // GPS enabled
    QString gpsStatus;            // GPS status
    
    // Coordinates
    double latitude;              // GPS latitude
    double longitude;             // GPS longitude
    double altitude;             // Altitude (meters)
    float accuracy;              // Accuracy (meters)
    float speed;                 // Speed (m/s)
    float bearing;               // Bearing (degrees)
    
    // Time
    qint64 gpsTime;             // GPS timestamp
    int gpsWeek;                // GPS week number
    int gpsTimeOfWeek;          // Time of week (ms)
    
    // NMEA
    QString nmeaSentence;        // NMEA sentence
    QString nmeaDate;           // NMEA date
    
    // constellations
    bool gpsConstellation;        // GPS enabled
    bool glonassConstellation;    // GLONASS enabled
    bool beidouConstellation;     // BeiDou enabled
    bool galileoConstellation;   // Galileo enabled
    bool qzssConstellation;      // QZSS enabled
    
    // AGPS
    QString agpsServer;          // AGPS server
    QString suplServer;          // SUPL server
    int suplPort;                // SUPL port
    
    // Geofencing
    bool geofencingEnabled;       // Geofencing enabled
    int geofenceCount;           // Number of geofences
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE TIMING/CLOCK CONFIGURATION
// ========================================================================

struct CompleteTimingConfig {
    // System Time
    qint64 systemTime;           // Unix timestamp (ms)
    qint64 systemTimeNanos;      // System time in nanos
    qint64 elapsedRealtime;      // Ms since boot
    qint64 elapsedRealtimeNanos;  // Nanos since boot
    qint64 uptimeMillis;         // Uptime in ms
    
    // Boot Time
    qint64 bootCount;            // Boot count
    qint64 bootTime;             // Boot timestamp
    bool bootCompleted;          // Boot completed
    bool bootAnimation;          // Boot animation running
    
    // Wall Clock
    qint64 wallClockTime;        // Wall clock time
    int timeZoneOffset;          // Timezone offset (ms)
    QString timeZoneId;          // Timezone ID (e.g., "America/New_York")
    bool autoTime;                // Auto time enabled
    bool autoTimeZone;           // Auto timezone enabled
    
    // NITZ
    QString nitzTime;           // NITZ time string
    QString nitzTimeZone;        // NITZ timezone
    
    // Drift
    int timeDrift;               // Time drift (ms)
    int clockDrift;              // Clock drift
    
    QJsonObject toJson() const;
};

// ========================================================================
// COMPLETE DEVICE PROFILE
// ========================================================================

class RealisticDeviceProfile {
public:
    static RealisticDeviceProfile& instance();
    
    // Generate complete realistic profile
    QJsonObject generateCompleteProfile(
        const QString& manufacturer,
        const QString& model,
        const QString& androidVersion = "14"
    );
    
    // Generate specific device types
    QJsonObject generateSamsungS24Ultra();
    QJsonObject generateGooglePixel8Pro();
    QJsonObject generateXiaomi14Ultra();
    QJsonObject generateOnePlus12();
    QJsonObject generateHuaweiMate60();
    QJsonObject generateSamsungA54();
    QJsonObject generateRandomRealisticDevice();
    
    // Get all profiles for export
    QJsonObject getCompleteDeviceJson(const QString& manufacturer);
    
    // Validate completeness
    bool isProfileComplete(const QJsonObject& profile);
    QStringList getMissingFields(const QJsonObject& profile);
    
private:
    RealisticDeviceProfile();
    
    // Internal generators
    CompleteDeviceIdentity generateIdentity(const QString& manufacturer);
    CompleteCarrierConfig generateCarrier();
    CompleteHardwareSpec generateHardware(const QString& manufacturer);
    CompleteBuildInfo generateBuild(const QString& manufacturer, const QString& model, const QString& androidVersion);
    CompleteSecurityConfig generateSecurity(const QString& manufacturer);
    CompleteSensorCalibration generateSensors();
    CompleteNetworkConfig generateNetwork();
    CompleteLocationConfig generateLocation();
    CompleteTimingConfig generateTiming();
    
    // Helper methods
    QString generateFingerprint(const QString& manufacturer, const QString& model, const QString& androidVersion);
    int calculateLuhnDigit(const QString& base);
    QString generateHex(int length);
    QString generateNumeric(int length);
    double generateCoordinate(bool isLatitude);
    
    // Storage
    QVector<QString> m_generatedSerials;
    QVector<QString> m_generatedIMEIs;
    QVector<QString> m_generatedAndroidIds;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_REALISTIC_DEVICE_PROFILE_H
