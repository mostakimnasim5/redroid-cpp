/**
 * @file DeviceProfile.h
 * @brief Complete Android Device Profile for Virtual Device Emulation
 * @version 3.0.0
 * 
 * Professional-grade device profile generation for Android devices.
 * Includes all device properties required for realistic device spoofing.
 * 
 * Categories:
 * - Device IDs: IMEI, IMEI2, Serial, Android ID, GSF ID, Advertising ID
 * - MAC Addresses: WiFi, Bluetooth, Ethernet
 * - SIM: ICCID, IMSI
 * - Hardware: CPU, GPU, RAM, Battery
 * - Build: Fingerprint, Bootloader, Build ID, Security Patch
 * - Verified Boot: State (green), VBMeta digest
 * - Network: Hostname, TCP buffers, DNS
 * - GPS: Latitude, Longitude, Altitude, Accuracy
 * - Sensors: Accelerometer, Gyro, Mag, Baro, Light, Proximity
 * - Security: KNOX ID, SELinux, Hardware attestation
 * 
 * Copyright (c) 2024. Licensed for authorized testing purposes only.
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <array>
#include <cstdint>
#include <chrono>

namespace RedroidCPP {

// ============================================================================
// Device Identity
// ============================================================================

/**
 * @brief Device Identity - IMEI, Serial, Android ID, GSF ID, Advertising ID
 */
struct DeviceIdentity {
    // IMEI (International Mobile Equipment Identity)
    std::string imei;                      // 15 digits, Luhn validated
    std::string imei2;                     // For dual SIM devices
    
    // Serial Number
    std::string serialNumber;               // Manufacturer-specific format
    
    // Android System Identifiers
    std::string androidId;                 // 16 hex characters
    std::string gsfId;                     // Google Services Framework ID
    std::string advertisingId;             // Google Advertising ID (AAID)
    
    // Generate all identifiers
    void generate(const std::string& tac, const std::string& manufacturer);
    
    // Validate all identifiers
    bool validate() const;
    
    // Get as map
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// MAC Addresses
// ============================================================================

/**
 * @brief MAC Addresses - WiFi, Bluetooth, Ethernet
 */
struct MACAddresses {
    std::string wifiMac;                   // WiFi MAC address (xx:xx:xx:xx:xx:xx)
    std::string bluetoothMac;              // Bluetooth MAC address
    std::string ethernetMac;               // Ethernet MAC address
    
    std::string wifiInterface;             // e.g., "wlan0"
    std::string ethernetInterface;         // e.g., "eth0"
    
    // Generate MAC addresses with valid OUI
    void generate(const std::string& manufacturer);
    
    // Validate MAC format
    bool isValid() const;
    
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// SIM Configuration
// ============================================================================

/**
 * @brief SIM Card Information
 */
struct SIMConfiguration {
    // ICCID (Integrated Circuit Card Identifier)
    std::string iccid1;                   // 20 digits, SIM slot 1
    std::string iccid2;                   // 20 digits, SIM slot 2 (dual SIM)
    
    // IMSI (International Mobile Subscriber Identity)
    std::string imsi1;                    // 15 digits, SIM slot 1
    std::string imsi2;                    // 15 digits, SIM slot 2
    
    // Carrier Information
    std::string carrierName;              // e.g., "T-Mobile", "AT&T"
    std::string carrierCountry;           // ISO country code, e.g., "US"
    std::string mcc;                      // Mobile Country Code (3 digits)
    std::string mnc;                      // Mobile Network Code (2-3 digits)
    
    // SIM State
    bool isMultiSIM;                      // true if dual SIM
    int32_t simCount;                     // 1 or 2
    std::string simState;                 // "READY", "ABSENT"
    
    // Network Type
    std::string networkType;              // "LTE", "5G", "NR"
    std::string phoneType;                // "GSM", "CDMA", "LTE"
    
    void generate();
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Hardware Configuration
// ============================================================================

/**
 * @brief CPU/Processor Information
 */
struct CPUInformation {
    std::string architecture;              // "arm64-v8a"
    std::string cpuAbiList;               // "arm64-v8a, armeabi-v7a"
    std::string cpuAbi2List;             // empty
    
    std::string processor;                 // "ARM Implementer 88 -> Qualcomm"
    std::string hardware;                  // "qcom"
    std::string board;                    // "taro", "kalama", "lito"
    
    std::string modelName;               // "Snapdragon 8 Gen 3"
    std::string modelShort;               // "SM8650"
    
    uint32_t coreCount;                  // Total cores
    uint32_t coreCountBig;               // Performance cores
    uint32_t coreCountMid;               // Medium cores
    uint32_t coreCountLittle;            // Efficiency cores
    
    uint64_t cpuMaxFreq;                 // Max frequency in Hz
    uint64_t cpuMinFreq;                 // Min frequency in Hz
    
    double bogoMips;                      // BogoMIPS value
    
    // CPU Features
    bool hasAES;                         // AES-NI support
    bool hasNEON;                        // NEON SIMD support
    bool hasVFP;                         // VFP support
    bool hasARMv8;                       // ARMv8 support
    
    void generate(const std::string& deviceClass);
    std::map<std::string, std::string> toMap() const;
};

/**
 * @brief GPU/Graphics Information
 */
struct GPUInformation {
    std::string renderer;                  // "Adreno (TM) 750"
    std::string vendor;                   // "Qualcomm"
    std::string version;                 // "OpenGL ES 3.2 V@0533.0"
    
    // Vulkan
    std::string vulkanVersion;           // "1.1.279"
    std::string vulkanConformance;       // "1.3.2"
    
    // OpenGL ES
    std::string glEsVersion;             // "3.2"
    std::string glEsRenderer;             // Full renderer string
    
    // Supported Extensions
    std::vector<std::string> extensions; // List of GL extensions
    
    void generate(const std::string& gpuModel);
    std::map<std::string, std::string> toMap() const;
};

/**
 * @brief Memory Configuration
 */
struct MemoryInformation {
    uint64_t totalRAM;                   // Total RAM in bytes
    uint64_t totalRAMMB;                 // Total RAM in MB
    uint64_t totalRAMGB;                // Total RAM in GB
    uint64_t availableRAM;               // Available RAM in bytes
    
    uint64_t lowMemoryThreshold;         // Low memory threshold
    uint64_t threshold;                  // Memory threshold
    
    bool isLowRamDevice;                // false for flagship
    uint32_t hardwarePageSize;           // Page size (4096)
    
    // Large Heap
    bool largeHeapEnabled;              // true
    int64_t largeHeapSize;             // Max heap size
    
    void generate();
    std::map<std::string, std::string> toMap() const;
};

/**
 * @brief Battery Configuration
 */
struct BatteryInformation {
    std::string status;                  // "good", "charging", "discharging"
    std::string health;                  // "health", "good"
    std::string plugType;               // "usb", "ac", "wireless"
    
    int32_t level;                     // Battery level (0-100)
    int32_t temperature;                // Temperature in deciCelsius (250 = 25.0°C)
    int32_t voltage;                    // Voltage in mV
    int32_t currentNow;                 // Current in mA
    int32_t currentAvg;                 // Average current
    
    bool present;                       // Battery present
    std::string technology;             // "Li-ion", "Li-poly"
    
    int32_t capacity;                  // Capacity in mAh
    int32_t batteryScale;              // Scale (100)
    
    // Battery Stats
    int32_t chargeCounter;             // Charge counter in uAh
    int32_t energyCounter;             // Energy counter in nWh
    
    void generate();
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Build Information
// ============================================================================

/**
 * @brief Build Information - Fingerprint, Bootloader, Build ID, Security Patch
 */
struct BuildInformation {
    // Core Build Properties
    std::string fingerprint;              // Full build fingerprint
    std::string bootloader;              // Bootloader version
    std::string radioVersion;            // Baseband/Radio version
    
    std::string buildId;                 // Build ID (e.g., "UP1A.231005.007")
    std::string buildDisplay;           // Display ID
    std::string buildType;              // "user", "userdebug", "eng"
    std::string buildTags;              // "release-keys", "dev-keys"
    
    // Version Information
    std::string buildVersionRelease;     // Android version (e.g., "14")
    std::string buildVersionCodename;   // Codename (e.g., "REL")
    std::string buildVersionSDK;        // API Level (e.g., "34")
    std::string buildVersionSecurityPatch; // Security patch level
    
    // Device Properties
    std::string buildBrand;             // Brand (e.g., "samsung")
    std::string buildManufacturer;      // Manufacturer (e.g., "samsung")
    std::string buildDevice;            // Device codename (e.g., "z3s")
    std::string buildProduct;           // Product name
    std::string buildHardware;          // Hardware name
    
    // Build Metadata
    std::string buildModel;            // Model (e.g., "SM-S928B")
    std::string buildName;              // Build name
    
    // Time
    std::string buildDate;              // Build date string
    std::string buildDateUTC;           // UTC build date
    int64_t buildTime;                 // Unix timestamp
    
    void generate(const std::string& manufacturer,
                 const std::string& model,
                 const std::string& codename,
                 const std::string& androidVersion);
    
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Verified Boot
// ============================================================================

/**
 * @brief Verified Boot State
 */
struct VerifiedBootInformation {
    // Boot State
    std::string verifiedBootState;      // "green", "yellow", "orange", "red"
    std::string verifiedBootLocked;      // "true", "false"
    std::string verifiedBootHardlocked; // "true", "false"
    
    // Verification
    std::string verificationBootEnabled; // "true"
    std::string verificationBoot;       // "true"
    
    // VBMeta Digest
    std::string vbmetaDigest;           // VBMeta digest (64 char hex)
    std::string vbmetaVersion;          // VBMeta version
    
    // Device Lock Status
    std::string oemLockState;          // "locked", "unlocked"
    std::string oemUnlockAllowed;       // "true", "false"
    
    void generate();
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Network Configuration
// ============================================================================

/**
 * @brief Network Configuration - Hostname, TCP buffers, DNS
 */
struct NetworkConfiguration {
    // Hostname
    std::string hostname;               // Device hostname
    std::string dhcpHostname;          // DHCP hostname
    
    // IP Addresses
    std::string ipAddress;             // Device IP
    std::string netmask;               // Netmask
    std::string gateway;               // Gateway
    std::string dns1;                  // Primary DNS
    std::string dns2;                  // Secondary DNS
    
    // TCP Buffers
    std::string tcpRmemMin;            // TCP receive buffer min
    std::string tcpRmemDefault;        // TCP receive buffer default
    std::string tcpRmemMax;             // TCP receive buffer max
    std::string tcpWmemMin;            // TCP send buffer min
    std::string tcpWmemDefault;         // TCP send buffer default
    std::string tcpWmemMax;            // TCP send buffer max
    std::string tcpCongestion;         // TCP congestion algorithm
    std::string tcpCongestionDefault;  // Default congestion type
    
    // DNS Configuration
    std::string dnsSearchDomains;      // Search domains
    std::vector<std::string> dnsServers; // DNS server list
    
    // Network Interface
    std::string networkInterface;       // Primary network interface
    std::string macAddress;           // MAC for this interface
    
    // Domain
    std::string domain;               // Domain name
    std::string domainLookup;          // Domain lookup setting
    
    void generate(const std::string& deviceName);
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// GPS Configuration
// ============================================================================

/**
 * @brief GPS/Location Configuration
 */
struct GPSConfiguration {
    // Location
    double latitude;                    // GPS latitude
    double longitude;                   // GPS longitude
    double altitude;                    // Altitude in meters
    double accuracy;                    // Accuracy in meters
    double altitudeAccuracy;            // Altitude accuracy
    double speed;                      // Speed in m/s
    double bearing;                    // Bearing in degrees
    
    // GPS Info
    std::string gpsVersion;            // "1.0"
    std::string gpsSize;               // GPS data size
    std::string gpsFlags;              // GPS flags
    
    // Supported Constellations
    bool supportsGPS;                  // GPS (USA)
    bool supportsGLONASS;              // GLONASS (Russia)
    bool supportsBEIDOU;              // BeiDou (China)
    bool supportsGALILEO;              // Galileo (Europe)
    bool supportsQZSS;                 // QZSS (Japan)
    
    // GPS Properties
    std::string gpsAccuracy;           // Accuracy setting
    std::string gpsSensitivity;       // GPS sensitivity
    std::string gpsVendor;             // GPS vendor
    std::string gpsModel;             // GPS model
    
    // Geofencing
    std::string lastLocation;         // Last known location
    std::string locationMode;         // "high_accuracy", "battery_saving", "device_only"
    
    void generate();
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Sensors Configuration
// ============================================================================

/**
 * @brief Sensor Configuration
 */
struct SensorsConfiguration {
    // Sensor List
    std::vector<std::string> availableSensors;
    
    // Accelerometer
    struct Accelerometer {
        std::string name;               // "STMicro Accelerometer"
        std::string vendor;             // "STMicroelectronics"
        std::string version;           // "1"
        std::string type;              // "1" (TYPE_ACCELEROMETER)
        std::string maxRange;           // "19.613300"
        std::string resolution;         // "0.001907"
        std::string power;             // "0.130000"
        std::string maxDelay;          // "200000"
        std::string minDelay;          // "5000"
        
        // Current values
        float x, y, z;                  // Current acceleration
    } accelerometer;
    
    // Gyroscope
    struct Gyroscope {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "4"
        std::string maxRange;          // "4000.000000"
        std::string resolution;       // "0.001220"
        std::string power;             // "0.260000"
        
        float x, y, z;                  // Current rotation
    } gyroscope;
    
    // Magnetic Field
    struct MagneticField {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "2"
        std::string maxRange;          // "49.999992"
        std::string resolution;        // "0.001500"
        std::string power;             // "0.250000"
        
        float x, y, z;                  // Current magnetic field
    } magneticField;
    
    // Barometer (Pressure)
    struct Barometer {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "6"
        std::string maxRange;          // "1100.000000"
        std::string resolution;        // "0.001000"
        std::string power;             // "0.001400"
        
        float pressure;                  // Current pressure in hPa
    } barometer;
    
    // Light Sensor
    struct LightSensor {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "5"
        std::string maxRange;          // "4300.000000"
        std::string resolution;        // "1.000000"
        std::string power;             // "0.150000"
        
        float lux;                      // Current illuminance
    } light;
    
    // Proximity Sensor
    struct ProximitySensor {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "3"
        std::string maxRange;          // "5.000000"
        std::string resolution;        // "1.000000"
        std::string power;             // "0.180000"
        
        float distance;                  // Current distance in cm
    } proximity;
    
    // Step Counter/Detector
    struct StepSensor {
        std::string name;
        std::string vendor;
        std::string version;
        std::string type;              // "19" (STEP_COUNTER), "18" (STEP_DETECTOR)
        
        int64_t steps;                  // Total step count
        bool stepDetected;              // Step detected
    } stepCounter;
    
    // Haptic Feedback
    struct Haptic {
        std::string name;
        int32_t maxAmplitude;          // 255
        int32_t defaultAmplitude;      // 128
    } haptic;
    
    void generate(const std::string& deviceClass);
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Security Configuration
// ============================================================================

/**
 * @brief Security Configuration - KNOX, SELinux, Hardware Attestation
 */
struct SecurityConfiguration {
    // KNOX (Samsung)
    std::string knoxVersion;           // KNOX version (e.g., "3.9")
    std::string knoxId;               // Samsung KNOX ID (32 char hex)
    std::string knoxGuardVersion;      // KNOX Guard version
    
    // Samsung-specific
    std::string odeMode;              // ODE mode
    std::string secureStorageEnabled; // Secure storage status
    std::string teeVersion;           // Trusted Execution Environment version
    
    // SELinux (Security-Enhanced Linux)
    std::string selinuxStatus;        // "Enforcing", "Permissive"
    std::string selinuxEnforcing;     // "Enforcing", "Permissive"
    std::string selinuxMode;          // "Enforcing", "Permissive"
    
    // Hardware Attestation
    std::string attestationEnabled;   // "true"
    std::string attestationStatus;    // "true"
    std::string keymasterVersion;    // Keymaster version (e.g., "4.1")
    std::string keymasterSecurityLevel; // "SOFTWARE"
    
    // Gatekeeper
    std::string gatekeeperVersion;   // Gatekeeper version
    std::string gatekeeperSecurityLevel; // "SOFTWARE"
    
    // Strongbox
    bool hasStrongbox;               // Strongbox available
    std::string strongboxVersion;     // Strongbox version
    std::string strongboxSecurityLevel; // "STRONGBOX"
    
    // Trusty (Trusted OS)
    std::string trustyVersion;       // Trusty TEE version
    
    // Verified Boot Key
    std::string verifiedBootKeyHash;  // Hash of verified boot key
    std::string roBootloader;        // Read-only bootloader state
    
    // Hardware-backed keys
    std::string hardwareBackedKeys;   // "true"
    std::string cryptoSupported;      // "true"
    
    // SafetyNet/Play Integrity
    std::string safetyNetEnabled;    // SafetyNet enabled
    std::string playIntegrityToken;   // Play Integrity token
    
    void generate(const std::string& manufacturer);
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Android Version
// ============================================================================

/**
 * @brief Android Version Information
 */
struct AndroidVersion {
    std::string codename;              // "UpsideDownCake"
    std::string versionNumber;         // "15"
    std::string apiLevel;             // "35"
    std::string releaseDate;          // "2024-10"
    std::string securityPatch;         // "2024-11-01"
    std::string buildType;            // "user"
    std::string buildTags;            // "release-keys"
    
    static AndroidVersion getLatest();
    static AndroidVersion fromString(const std::string& version);
};

// ============================================================================
// Display Configuration
// ============================================================================

/**
 * @brief Display Configuration
 */
struct DisplayConfiguration {
    uint32_t widthPixels;
    uint32_t heightPixels;
    uint32_t densityDPI;
    float densityValue;
    uint32_t fps;
    uint32_t vsync;
    
    std::string hdrCapabilities;      // "HDR10, HDR10+, DolbyVision"
    std::string wideColorGamut;      // "sRGB, Display P3"
    
    // Calculated
    uint32_t densityBucket;
    uint32_t smallestWidth;
    
    void generate(const std::string& deviceClass);
    std::map<std::string, std::string> toMap() const;
};

// ============================================================================
// Complete Device Profile
// ============================================================================

/**
 * @brief Complete Device Profile containing all device properties
 */
class DeviceProfile {
public:
    // Metadata
    std::string profileId;
    std::string profileName;
    std::string createdAt;
    std::string modifiedAt;
    std::string manufacturer;
    std::string model;
    std::string codename;
    std::string deviceClass;
    
    // All Components
    DeviceIdentity identity;
    MACAddresses macAddresses;
    SIMConfiguration sim;
    CPUInformation cpu;
    GPUInformation gpu;
    MemoryInformation memory;
    BatteryInformation battery;
    DisplayConfiguration display;
    BuildInformation build;
    VerifiedBootInformation verifiedBoot;
    NetworkConfiguration network;
    GPSConfiguration gps;
    SensorsConfiguration sensors;
    SecurityConfiguration security;
    AndroidVersion androidVersion;
    
    // TAC Information
    std::string tac;
    std::string tacModel;
    std::string tacInternalName;
    
    // Constructors
    DeviceProfile();
    explicit DeviceProfile(const std::string& manufacturer);
    DeviceProfile(const DeviceProfile& other);
    
    // Assignment
    DeviceProfile& operator=(const DeviceProfile& other);
    
    // Generate complete profile
    void generate(const std::string& manufacturer,
                 const std::string& model = "",
                 const std::string& androidVersion = "");
    
    // Serialization
    std::string toJSON() const;
    bool fromJSON(const std::string& json);
    bool save(const std::string& filepath) const;
    bool load(const std::string& filepath);
    
    // Export
    std::map<std::string, std::string> getAllProperties() const;
    std::map<std::string, std::map<std::string, std::string>> getPropertiesByCategory() const;
    
    // Display
    void print() const;
    std::string summary() const;
    bool validate() const;

private:
    void initialize();
    std::string generateUniqueId();
    std::string getCurrentTimestamp();
};

// ============================================================================
// TAC Database
// ============================================================================

/**
 * @brief TAC Entry
 */
struct TACEntry {
    std::string tac;                   // 8-digit TAC
    std::string brand;                 // Brand name
    std::string modelName;             // Marketing model name
    std::string internalName;          // Internal codename
    std::string deviceType;           // "Smartphone", "Tablet"
    std::string deviceClass;          // "High-End", "Mid-Range"
    std::string launchYear;           // "2024"
    std::string launchMonth;          // "01"
};

/**
 * @brief TAC Database for IMEI generation
 */
class TACDatabase {
public:
    static TACDatabase& getInstance();
    
    std::optional<TACEntry> getByTAC(const std::string& tac) const;
    std::optional<TACEntry> getRandomForManufacturer(const std::string& manufacturer);
    std::vector<TACEntry> getByManufacturer(const std::string& manufacturer);
    std::vector<std::string> getManufacturers() const;
    std::optional<TACEntry> getRandom();
    
private:
    TACDatabase();
    void initializeDatabase();
    void addTAC(const std::string& tac, const std::string& brand,
                const std::string& model, const std::string& internal,
                const std::string& type, const std::string& cls,
                const std::string& year, const std::string& month);
    
    std::map<std::string, TACEntry> m_tacMap;
    std::map<std::string, std::vector<std::string>> m_manufacturerTACs;
};

// ============================================================================
// IMEI Generator Utility
// ============================================================================

class IMEIGenerator {
public:
    static std::string generate(const std::string& tac);
    static bool validate(const std::string& imei);
    static int calculateLuhnCheckDigit(const std::string& base);
};

// ============================================================================
// MAC Generator Utility
// ============================================================================

class MACGenerator {
public:
    static std::string generate(const std::string& oui);
    static std::string getOUIForManufacturer(const std::string& manufacturer);
};

// ============================================================================
// Operator/Carrier Database
// ============================================================================

struct CarrierInfo {
    std::string name;
    std::string country;
    std::string mcc;
    std::string mnc;
};

class CarrierDatabase {
public:
    static CarrierDatabase& getInstance();
    
    CarrierInfo getRandom();
    std::vector<CarrierInfo> getByCountry(const std::string& country);
    
private:
    CarrierDatabase();
    void initialize();
    
    std::vector<CarrierInfo> m_carriers;
};

} // namespace RedroidCPP
