/**
 * @file DeviceProfile.h
 * @brief Complete Android Device Profile
 * @version 2.0.0
 * 
 * Contains all device properties required for realistic Android device emulation.
 * This is used for anti-detection and device spoofing purposes.
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

namespace RedroidCPP {

// ============================================================================
// Forward Declarations
// ============================================================================

class IMEIValidator;
class SerialGenerator;
class MACGenerator;

// ============================================================================
// Version Information
// ============================================================================

/**
 * @brief Android version information
 */
struct AndroidVersionInfo {
    std::string codename;           // "UpsideDownCake", "Tiramisu", etc.
    std::string versionNumber;      // "15", "14", "13", etc.
    std::string apiLevel;           // "35", "34", "33", etc.
    std::string releaseDate;       // "2024-08"
    std::string securityPatch;      // "2024-06-01"
    std::string buildType;          // "userdebug", "user", "eng"
    std::string buildTags;         // "release-keys", "dev-keys"
    
    static AndroidVersionInfo getLatest();
    static std::vector<AndroidVersionInfo> getAllSupported();
};

// ============================================================================
// Display Configuration
// ============================================================================

/**
 * @brief Display/Hardware configuration
 */
struct DisplayConfig {
    uint32_t widthPixels;           // e.g., 1440
    uint32_t heightPixels;         // e.g., 3120
    uint32_t densityDPI;           // e.g., 560
    uint32_t densityValue;         // e.g., 3.5
    uint32_t fps;                  // e.g., 120
    uint32_t vsync;               // e.g., 120
    std::string hdrCapabilities;   // "HDR10, DolbyVision"
    std::string wideColorGamut;     // "sRGB, Display P3"
    
    // Calculated values
    uint32_t densityBucket() const;
    uint32_t smallestWidth() const;
    
    static DisplayConfig getDefault();
    static DisplayConfig getForDeviceClass(const std::string& deviceClass);
};

// ============================================================================
// Hardware Configuration
// ============================================================================

/**
 * @brief CPU/Processor information
 */
struct CPUInfo {
    std::string architecture;      // "arm64-v8a", "armeabi-v7a"
    std::string cpuAbiList;         // "arm64-v8a, armeabi-v7a"
    std::string cpuAbi2List;       // ""
    std::string processor;          // "ARM Implementer 88 -> Qualcomm"
    std::string hardware;           // "qcom"
    std::string board;             // "taro", "kalama", "lito"
    std::string modelName;          // "Snapdragon 8 Gen 3"
    
    // Physical properties
    uint32_t coreCount;            // 8
    uint32_t coreCountBig;         // 1
    uint32_t coreCountMid;         // 3
    uint32_t coreCountLittle;       // 4
    
    // Frequencies (in Hz)
    uint64_t cpuMaxFreq;           // 3050000000
    uint64_t cpuMinFreq;           // 300000000
    
    // BogoMIPS
    uint32_t bogoMips;             // 192.0
    
    // Features
    bool hasAES() const;
    bool hasNEON() const;
    bool hasVFP() const;
};

/**
 * @brief GPU/Graphics information
 */
struct GPUInfo {
    std::string renderer;           // "Adreno (TM) 750"
    std::string vendor;            // "Qualcomm"
    std::string version;           // "OpenGL ES 3.2 V@0533.0"
    std::string extensions;        // List of OpenGL extensions
    
    // Vulkan
    std::string vulkanVersion;     // "1.1.279"
    std::string vulkanConformance;  // "1.3.2"
};

/**
 * @brief Memory configuration
 */
struct MemoryConfig {
    uint64_t totalRAM;              // in bytes, e.g., 12884901888 (12GB)
    uint64_t totalRAMMB;           // in MB
    uint64_t availableRAM;          // in bytes
    uint64_t lowMemoryThreshold;    // in bytes
    uint64_t threshold;             // in bytes
    
    bool isLowRamDevice;           // false
    uint32_t hardwarePageSize;     // 4096
    
    // Memory features
    bool largeHeapEnabled;        // true
    bool lowRamDevice(); const;
};

/**
 * @brief Battery configuration
 */
struct BatteryConfig {
    std::string status;            // "good"
    std::string health;            // "health" -> "good"
    std::string plugType;          // "usb", "ac", "wireless"
    int32_t level;                 // 100
    int32_t temperature;           // in deciCelsius, e.g., 250 (25.0°C)
    int32_t voltage;               // in mV, e.g., 4200
    int32_t currentNow;             // in mA
    bool present;                  // true
    std::string technology;         // "Li-ion"
    int32_t capacity;              // in mAh, e.g., 4500
};

// ============================================================================
// Build Information
// ============================================================================

/**
 * @brief Android Build information
 */
struct BuildInfo {
    std::string fingerprint;
    std::string bootloader;
    std::string radioVersion;       // Baseband version
    std::string buildId;            // Build ID like "UP1A.231005.007"
    std::string buildDisplay;       // "UP1A.231005.007"
    std::string buildType;          // "user"
    std::string buildTags;          // "release-keys"
    std::string buildVersion;       // "14"
    std::string buildVersionRelease; // "14"
    std::string buildVersionSDK;     // "34"
    std::string buildSecurityPatch;  // "2024-06-01"
    std::string buildBoard;         // Platform/board name
    std::string buildDevice;        // Device codename
    std::string buildProduct;       // Product name
    std::string buildHardware;      // Hardware name
    std::string buildBrand;         // Brand
    std::string buildManufacturer;  // Manufacturer
    std::string buildModel;        // Model
    std::string buildName;         // Build name
    
    // Time information
    long buildTime;                // Unix timestamp
    std::string buildDate;         // "Wed Jan 10 00:00:00 UTC 2024"
    
    // Generate complete fingerprint
    std::string generateFingerprint(const std::string& manufacturer,
                                      const std::string& brand,
                                      const std::string& device,
                                      const std::string& model,
                                      const std::string& version,
                                      const std::string& buildId) const;
};

// ============================================================================
// Network Configuration
// ============================================================================

/**
 * @brief Network/MAC address information
 */
struct NetworkConfig {
    // WiFi MAC Address
    std::string wifiMAC;           // "8C:71:F8:AB:CD:EF"
    std::string wifiInterface;     // "wlan0"
    std::string wifiAddress;       // IP address
    
    // Bluetooth MAC Address
    std::string bluetoothMAC;      // "8C:71:F8:AB:CD:EF"
    
    // Ethernet
    std::string ethernetMAC;      // "00:00:00:00:00:00"
    std::string ethernetInterface; // "eth0"
    
    // Generate MAC with valid OUI
    static std::string generateMAC(const std::string& oui);
    static bool isValidOUI(const std::string& oui);
    
    // Get OUI database
    static std::string getOUIForManufacturer(const std::string& manufacturer);
};

// ============================================================================
// SIM/Operator Information
// ============================================================================

/**
 * @brief SIM card information
 */
struct SIMInfo {
    std::string iccid;             // ICCID
    std::string imsi;              // IMSI
    std::string carrierName;       // "T-Mobile", "AT&T"
    std::string carrierCountry;     // "US", "BD"
    std::string mcc;               // Mobile Country Code
    std::string mnc;              // Mobile Network Code
    bool isMultiSIM;              // true/false
    int32_t simCount;             // 1 or 2
    std::string simState;          // "READY", "ABSENT"
    std::string networkType;       // "LTE", "5G", "NR"
    std::string phoneType;         // "GSM", "CDMA"
    
    // SIM slots
    std::string simSlot1ICCID;
    std::string simSlot2ICCID;
    
    static std::vector<SIMInfo> generateSimInfo(int count = 1);
};

// ============================================================================
// Telephony Information
// ============================================================================

/**
 * @brief Telephony/Radio information
 */
struct TelephonyInfo {
    std::string deviceId;          // IMEI
    std::string deviceId2;         // IMEI2 (for dual SIM)
    std::string subscriberId;      // IMSI
    std::string subscriberId2;     // IMSI2
    std::string serialNumber;      // Serial number
    std::string androidId;        // Android ID
    std::string gsfId;           // Google Services Framework ID
    
    // Voice
    std::string voiceMailNumber;
    std::string voiceMailTag;
    
    // Phone number (if available)
    std::string line1Number;
    std::string line1NumberTag;
};

// ============================================================================
// Sensor Configuration
// ============================================================================

/**
 * @brief Sensor information
 */
struct SensorInfo {
    std::vector<std::string> sensors;
    
    // Accelerometer
    std::string accelerometerName;    // "STMicro accelerometer"
    std::string accelerometerVendor;   // "STMicroelectronics"
    std::string accelerometerVersion; // "1"
    std::string accelerometerType;     // "1" (TYPE_ACCELEROMETER)
    float accelerometerMaxRange;        // "19.613300"
    float accelerometerResolution;      // "0.001907"
    float accelerometerPower;          // "0.130000"
    
    // Gyroscope
    std::string gyroscopeName;
    std::string gyroscopeVendor;
    
    // Magnetic Field
    std::string magneticName;
    std::string magneticVendor;
    
    // Proximity
    std::string proximityName;
    std::string proximityVendor;
    
    // Light
    std::string lightName;
    std::string lightVendor;
    
    // Pressure
    std::string pressureName;
    std::string pressureVendor;
    
    // Steps
    std::string stepCounterName;
    std::string stepDetectorName;
    
    // Haptic
    std::string hapticName;
    int32_t hapticMaxAmplitude;
    
    static SensorInfo getDefault();
    static SensorInfo getForDeviceClass(const std::string& deviceClass);
};

// ============================================================================
// Camera Configuration
// ============================================================================

/**
 * @brief Camera information
 */
struct CameraInfo {
    // Back Camera
    std::string backCameraMake;     // "Samsung"
    std::string backCameraModel;     // "S5KHP2"
    std::string backCameraLensMake;  // "Samsung"
    std::string backCameraLensModel; // "Samsung S5KHP2"
    std::string backCameraLensFocalLength; // "5.67mm"
    float backCameraAperture;       // 1.75
    int32_t backCameraOrientation;   // 90
    int32_t backCameraSupportedModes;
    std::string backCameraCapabilities; // "auto-exposure-modes, auto-whitebalance"
    
    // Front Camera
    std::string frontCameraMake;
    std::string frontCameraModel;
    std::string frontCameraLensMake;
    std::string frontCameraLensModel;
    float frontCameraAperture;
    int32_t frontCameraOrientation;
    
    // Flash
    bool hasFlash;                 // true
    std::string flashTemperature;   // "5500K"
    
    // Supported sizes
    std::vector<std::string> backCameraSizes;
    std::vector<std::string> frontCameraSizes;
    
    static CameraInfo getDefault();
};

// ============================================================================
// Audio Configuration
// ============================================================================

/**
 * @brief Audio/Sound configuration
 */
struct AudioInfo {
    std::string audioDevice;         // "audio primary boot"
    std::string audioEffect;          // "audio offload"
    std::string audioOffloadBitWidth; // "24"
    std::string audioOffloadChannels;  // "2"
    std::string audioOffloadSampleRate; // "48000"
    
    // Speaker
    std::string speakerImpedance;    // "4 Ohm"
    std::vector<std::string> supportedAudioFormats;
    std::vector<std::string> supportedAudioSources;
    
    // Microphones
    int32_t microphoneCount;
    std::string microphoneNames;
    
    static AudioInfo getDefault();
};

// ============================================================================
// Location/GPS Configuration
// ============================================================================

/**
 * @brief GPS/Location configuration
 */
struct GPSInfo {
    std::string gpsVersion;         // "1.0"
    std::string gpsSize;            // "1040384"
    std::string gpsFlags;           // "1"
    
    // Supported protocols
    bool supportsGPS;
    bool supportsGLONASS;
    bool supportsBEIDOU;
    bool supportsGALILEO;
    bool supportsQZSS;
    
    std::string gpsAccuracy;        // "2.0"
    std::string gpsSensitivity;     // "0.0"
    std::string gpsVendor;          // "Qualcomm"
    
    static GPSInfo getDefault();
};

// ============================================================================
// Biometric Configuration
// ============================================================================

/**
 * @brief Biometric/Fingerprint configuration
 */
struct BiometricInfo {
    bool hasFingerprint;           // true
    std::string fingerprintVendor;  // "Qualcomm"
    std::string fingerprintModel;   // "QTI Fingerprint Sensor"
    std::string fingerprintVersion; // "1.0"
    
    bool hasFaceUnlock;            // true/false
    std::string faceUnlockVendor;
    
    bool hasIrisScanner;           // false
    std::string irisScannerVendor;
    
    static BiometricInfo getDefault();
};

// ============================================================================
// Security/Boot Configuration
// ============================================================================

/**
 * @brief Boot and security configuration
 */
struct SecurityInfo {
    std::string verifiedBootState;   // "green"
    std::string verifiedBootLocked; // "true"
    std::string verifiedBootHardlocked; // "true"
    std::string verificationBootEnabled; // "true"
    std::string verificationBoot;    // "true"
    
    // SELinux
    std::string selinuxEnforcing;  // "Enforcing"
    std::string selinuxStatus;     // "Enforcing"
    
    // Trusty
    std::string trustyVersion;     // "1.0"
    
    // Keymaster
    std::string keymasterVersion;  // "4.1"
    
    // Gatekeeper
    std::string gatekeeperVersion; // "1.0"
    
    // Strongbox
    bool hasStrongbox;             // true
    
    // RKP (Samsung Knox, etc.)
    bool hasRKP;                  // true/false
    std::string rkpVersion;
    
    static SecurityInfo getDefault();
};

// ============================================================================
// Complete Device Profile
// ============================================================================

/**
 * @brief Complete device profile containing all properties
 */
class DeviceProfile {
public:
    // ========================================================================
    // Metadata
    // ========================================================================
    std::string profileId;              // Unique profile ID
    std::string profileName;            // Human-readable name
    std::string createdAt;             // Creation timestamp
    std::string modifiedAt;             // Last modified timestamp
    std::string manufacturer;           // Manufacturer name
    
    // ========================================================================
    // Identity Components
    // ========================================================================
    TelephonyInfo telephony;
    SIMInfo sim;
    NetworkConfig network;
    
    // ========================================================================
    // Hardware Components
    // ========================================================================
    CPUInfo cpu;
    GPUInfo gpu;
    MemoryConfig memory;
    BatteryConfig battery;
    DisplayConfig display;
    
    // ========================================================================
    // Build Components
    // ========================================================================
    BuildInfo build;
    AndroidVersionInfo androidVersion;
    
    // ========================================================================
    // Peripherals
    // ========================================================================
    SensorInfo sensors;
    CameraInfo camera;
    AudioInfo audio;
    GPSInfo gps;
    BiometricInfo biometric;
    SecurityInfo security;
    
    // ========================================================================
    // Methods
    // ========================================================================
    
    /**
     * @brief Default constructor
     */
    DeviceProfile();
    
    /**
     * @brief Create profile for specific manufacturer
     * @param manufacturer Manufacturer name
     */
    explicit DeviceProfile(const std::string& manufacturer);
    
    /**
     * @brief Copy constructor
     */
    DeviceProfile(const DeviceProfile& other);
    
    /**
     * @brief Assignment operator
     */
    DeviceProfile& operator=(const DeviceProfile& other);
    
    /**
     * @brief Generate all required identifiers
     */
    void generateIdentifiers();
    
    /**
     * @brief Get all properties as map for ADB commands
     * @return Map of property name to value
     */
    std::map<std::string, std::string> getAllProperties() const;
    
    /**
     * @brief Get properties grouped by category
     * @return Map of category to property map
     */
    std::map<std::string, std::map<std::string, std::string>> getPropertiesByCategory() const;
    
    /**
     * @brief Serialize to JSON string
     * @return JSON representation
     */
    std::string toJSON() const;
    
    /**
     * @brief Deserialize from JSON string
     * @param json JSON string
     * @return true if successful
     */
    bool fromJSON(const std::string& json);
    
    /**
     * @brief Save profile to file
     * @param filepath File path
     * @return true if successful
     */
    bool save(const std::string& filepath) const;
    
    /**
     * @brief Load profile from file
     * @param filepath File path
     * @return true if successful
     */
    bool load(const std::string& filepath);
    
    /**
     * @brief Print profile to stdout
     */
    void print() const;
    
    /**
     * @brief Validate profile consistency
     * @return true if valid
     */
    bool validate() const;
    
    /**
     * @brief Get profile summary
     * @return Summary string
     */
    std::string summary() const;

private:
    /**
     * @brief Initialize with manufacturer defaults
     */
    void initializeForManufacturer(const std::string& manufacturer);
    
    /**
     * @brief Generate TAC entry based on manufacturer
     */
    void generateTACEntry();
    
    /**
     * @brief Generate build fingerprint
     */
    void generateFingerprint();
    
    /**
     * @brief Generate serial number
     */
    void generateSerialNumber();
    
    /**
     * @brief Generate MAC addresses
     */
    void generateMACAddresses();
    
    /**
     * @brief Generate SIM information
     */
    void generateSIMInfo();
};

} // namespace RedroidCPP
