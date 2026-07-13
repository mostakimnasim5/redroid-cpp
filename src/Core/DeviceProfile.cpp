/**
 * @file DeviceProfile.cpp
 * @brief Implementation of DeviceProfile
 */

#include "DeviceProfile.h"
#include "TACDatabase.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace RedroidCPP {

// ============================================================================
// Utility Functions
// ============================================================================

namespace {

std::string generateRandomHex(size_t length) {
    static const char hexChars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, 15);
    
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += hexChars[dis(gen)];
    }
    return result;
}

std::string generateRandomNumeric(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, 9);
    
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += std::to_string(dis(gen));
    }
    return result;
}

std::string generateRandomAlphaNumeric(size_t length) {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, 35);
    
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

int calculateLuhnCheckDigit(const std::string& baseIMEI) {
    int sum = 0;
    bool alternate = true;
    
    for (int i = static_cast<int>(baseIMEI.length()) - 1; i >= 0; --i) {
        int n = baseIMEI[i] - '0';
        
        if (alternate) {
            n *= 2;
            if (n > 9) {
                n = (n % 10) + 1;
            }
        }
        
        sum += n;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

std::string formatMAC(const std::string& mac) {
    std::string result;
    for (size_t i = 0; i < mac.length(); ++i) {
        if (i > 0 && i % 2 == 0) {
            result += ':';
        }
        result += std::toupper(mac[i]);
    }
    return result;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string formatHex(int value, int width) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// AndroidVersionInfo Implementation
// ============================================================================

AndroidVersionInfo AndroidVersionInfo::getLatest() {
    return getAllSupported().front();
}

std::vector<AndroidVersionInfo> AndroidVersionInfo::getAllSupported() {
    return {
        {
            "VanillaIceCream",
            "16",
            "36",
            "2025-03",
            "2025-06-01",
            "user",
            "release-keys"
        },
        {
            "UpsideDownCake",
            "15",
            "35",
            "2024-10",
            "2024-11-01",
            "user",
            "release-keys"
        },
        {
            "Tiramisu",
            "14",
            "34",
            "2023-08",
            "2024-06-01",
            "user",
            "release-keys"
        },
        {
            "SnowCone",
            "13",
            "33",
            "2022-08",
            "2024-02-01",
            "user",
            "release-keys"
        },
        {
            "Red Velvet Cake",
            "12",
            "31",
            "2021-05",
            "2023-05-01",
            "user",
            "release-keys"
        }
    };
}

// ============================================================================
// DisplayConfig Implementation
// ============================================================================

uint32_t DisplayConfig::densityBucket() const {
    if (densityDPI <= 120) return 120;
    if (densityDPI <= 160) return 160;
    if (densityDPI <= 240) return 240;
    if (densityDPI <= 320) return 320;
    if (densityDPI <= 480) return 480;
    return 640;
}

uint32_t DisplayConfig::smallestWidth() const {
    return std::min(widthPixels, heightPixels) / (densityDPI / 160);
}

DisplayConfig DisplayConfig::getDefault() {
    DisplayConfig config;
    config.widthPixels = 1440;
    config.heightPixels = 3120;
    config.densityDPI = 560;
    config.densityValue = 3.5f;
    config.fps = 120;
    config.vsync = 120;
    config.hdrCapabilities = "HDR10, HDR10+, DolbyVision, HLG";
    config.wideColorGamut = "sRGB, Display P3, Adobe RGB";
    return config;
}

DisplayConfig DisplayConfig::getForDeviceClass(const std::string& deviceClass) {
    DisplayConfig config;
    
    if (deviceClass == "High-End") {
        config.widthPixels = 1440;
        config.heightPixels = 3120;
        config.densityDPI = 560;
        config.fps = 120;
    }
    else if (deviceClass == "Mid-Range") {
        config.widthPixels = 1080;
        config.heightPixels = 2400;
        config.densityDPI = 400;
        config.fps = 90;
    }
    else if (deviceClass == "Compact") {
        config.widthPixels = 1080;
        config.heightPixels = 2340;
        config.densityDPI = 420;
        config.fps = 90;
    }
    else if (deviceClass == "Gaming") {
        config.widthPixels = 1080;
        config.heightPixels = 2400;
        config.densityDPI = 400;
        config.fps = 144;
    }
    else if (deviceClass == "Tablet") {
        config.widthPixels = 2560;
        config.heightPixels = 1600;
        config.densityDPI = 320;
        config.fps = 60;
    }
    else {
        config = getDefault();
    }
    
    return config;
}

// ============================================================================
// CPUInfo Implementation
// ============================================================================

bool CPUInfo::hasAES() const { return true; }
bool CPUInfo::hasNEON() const { return true; }
bool CPUInfo::hasVFP() const { return true; }

// ============================================================================
// MemoryConfig Implementation
// ============================================================================

bool MemoryConfig::lowRamDevice() const {
    return isLowRamDevice;
}

// ============================================================================
// BuildInfo Implementation
// ============================================================================

std::string BuildInfo::generateFingerprint(const std::string& manufacturer,
                                           const std::string& brand,
                                           const std::string& device,
                                           const std::string& model,
                                           const std::string& version,
                                           const std::string& buildId) const {
    std::ostringstream oss;
    oss << manufacturer << "/" << brand << "/" << device << ":"
        << version << "/" << buildId << "/" << buildId << ":"
        << buildType << "/" << buildTags;
    return oss.str();
}

// ============================================================================
// NetworkConfig Implementation
// ============================================================================

std::string NetworkConfig::generateMAC(const std::string& oui) {
    std::string mac = oui;
    mac += generateRandomHex(6);
    return formatMAC(mac);
}

bool NetworkConfig::isValidOUI(const std::string& oui) {
    if (oui.length() != 8 && oui.length() != 17) return false;
    
    // Remove colons if present
    std::string cleanOui = oui;
    cleanOui.erase(std::remove(cleanOui.begin(), cleanOui.end(), ':'), cleanOui.end());
    
    if (cleanOui.length() != 6) return false;
    
    for (char c : cleanOui) {
        if (!std::isxdigit(c)) return false;
    }
    
    return true;
}

std::string NetworkConfig::getOUIForManufacturer(const std::string& manufacturer) {
    static const std::map<std::string, std::string> ouiMap = {
        {"Samsung", "8C:71:F8"},
        {"Google", "94:EB:2C"},
        {"Xiaomi", "64:09:80"},
        {"OnePlus", "F8:39:6B"},
        {"Sony", "AC:2B:6E"},
        {"OPPO", "88:C9:D0"},
        {"Vivo", "2C:33:61"},
        {"Huawei", "20:F3:A3"},
        {"Motorola", "A4:39:A6"},
        {"Realme", "88C9:D0"},
        {"ASUS", "04:D3:C2"},
        {"Nokia", "001727"}
    };
    
    auto it = ouiMap.find(manufacturer);
    if (it != ouiMap.end()) {
        return it->second;
    }
    
    return "00:00:00"; // Default
}

// ============================================================================
// SIMInfo Implementation
// ============================================================================

std::vector<SIMInfo> SIMInfo::generateSimInfo(int count) {
    std::vector<SIMInfo> sims;
    
    static const std::vector<std::tuple<std::string, std::string, std::string, std::string>> carriers = {
        {"T-Mobile", "US", "310", "260"},
        {"AT&T", "US", "310", "410"},
        {"Verizon", "US", "311", "480"},
        {"Vodafone", "UK", "234", "15"},
        {"O2", "UK", "234", "10"},
        {"Orange", "FR", "208", "01"},
        {"Deutsche Telekom", "DE", "262", "01"},
        {"Grameenphone", "BD", "470", "01"},
        {"Robi", "BD", "470", "04"},
        {"Banglalink", "BD", "470", "07"},
        {"Telia", "SE", "240", "01"},
        {"Telenor", "NO", "242", "02"},
        {"Swisscom", "CH", "228", "01"}
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, carriers.size() - 1);
    
    for (int i = 0; i < count; ++i) {
        SIMInfo sim;
        auto carrier = carriers[dis(gen)];
        
        sim.carrierName = std::get<0>(carrier);
        sim.carrierCountry = std::get<1>(carrier);
        sim.mcc = std::get<2>(carrier);
        sim.mnc = std::get<3>(carrier);
        sim.isMultiSIM = (count > 1);
        sim.simCount = count;
        sim.simState = "READY";
        sim.networkType = "LTE";
        sim.phoneType = "GSM";
        
        // Generate ICCID (20 digits)
        std::string iccid = "89" + sim.mcc + sim.mnc + generateRandomNumeric(13);
        sim.iccid = iccid;
        
        // Generate IMSI (15 digits)
        std::string imsi = sim.mcc + sim.mnc + generateRandomNumeric(9);
        sim.imsi = imsi;
        
        // Slot-specific
        if (i == 0) {
            sim.simSlot1ICCID = sim.iccid;
        } else {
            sim.simSlot2ICCID = sim.iccid;
        }
        
        sims.push_back(sim);
    }
    
    return sims;
}

// ============================================================================
// SensorInfo Implementation
// ============================================================================

SensorInfo SensorInfo::getDefault() {
    SensorInfo info;
    
    info.sensors = {
        "Accelerometer Sensor",
        "Gyroscope Sensor",
        "Magnetic Field Sensor",
        "Proximity Sensor",
        "Light Sensor",
        "Pressure Sensor",
        "Step Counter Sensor",
        "Step Detector Sensor",
        "Haptic Feedback Consumer"
    };
    
    info.accelerometerName = "STMicro Accelerometer";
    info.accelerometerVendor = "STMicroelectronics";
    info.accelerometerVersion = "1";
    info.accelerometerType = "1";
    info.accelerometerMaxRange = 19.6133f;
    info.accelerometerResolution = 0.001907f;
    info.accelerometerPower = 0.130f;
    
    info.gyroscopeName = "STMicro Gyroscope";
    info.gyroscopeVendor = "STMicroelectronics";
    
    info.magneticName = "STMicro Magnetic Field";
    info.magneticVendor = "STMicroelectronics";
    
    info.proximityName = "STMicro Proximity";
    info.proximityVendor = "STMicroelectronics";
    
    info.lightName = "STMicro Light Sensor";
    info.lightVendor = "STMicroelectronics";
    
    info.pressureName = "BMP280 Pressure";
    info.pressureVendor = "Bosch";
    
    info.stepCounterName = "Step Counter";
    info.stepDetectorName = "Step Detector";
    
    info.hapticName = "Haptic Feedback";
    info.hapticMaxAmplitude = 255;
    
    return info;
}

SensorInfo SensorInfo::getForDeviceClass(const std::string& deviceClass) {
    SensorInfo info = getDefault();
    
    if (deviceClass == "High-End" || deviceClass == "Gaming") {
        // Premium sensors
        info.accelerometerMaxRange = 39.2266f;
        info.accelerometerResolution = 0.0012f;
    }
    else if (deviceClass == "Budget" || deviceClass == "Mid-Range") {
        // Basic sensors
        info.accelerometerMaxRange = 19.6133f;
        info.accelerometerResolution = 0.0024f;
    }
    
    return info;
}

// ============================================================================
// CameraInfo Implementation
// ============================================================================

CameraInfo CameraInfo::getDefault() {
    CameraInfo info;
    
    info.backCameraMake = "Samsung";
    info.backCameraModel = "S5KHP2";
    info.backCameraLensMake = "Samsung";
    info.backCameraLensModel = "Samsung S5KHP2";
    info.backCameraLensFocalLength = "5.67mm";
    info.backCameraAperture = 1.75f;
    info.backCameraOrientation = 90;
    info.backCameraSupportedModes = 1;
    info.backCameraCapabilities = "auto-exposure-modes, auto-whitebalance";
    
    info.frontCameraMake = "Samsung";
    info.frontCameraModel = "S5K3LU";
    info.frontCameraLensMake = "Samsung";
    info.frontCameraLensModel = "Samsung S5K3LU";
    info.frontCameraAperture = 2.2f;
    info.frontCameraOrientation = 270;
    
    info.hasFlash = true;
    info.flashTemperature = "5500K";
    
    info.backCameraSizes = {"4000x3000", "3840x2160", "1920x1080"};
    info.frontCameraSizes = {"3264x2448", "1920x1080"};
    
    return info;
}

// ============================================================================
// AudioInfo Implementation
// ============================================================================

AudioInfo AudioInfo::getDefault() {
    AudioInfo info;
    
    info.audioDevice = "audio primary boot";
    info.audioEffect = "audio offload";
    info.audioOffloadBitWidth = "24";
    info.audioOffloadChannels = "2";
    info.audioOffloadSampleRate = "48000";
    
    info.speakerImpedance = "4 Ohm";
    
    info.supportedAudioFormats = {
        "AAC", "AMR-NB", "AMR-WB", "FLAC", "GSM-EFR", "GSM-HR",
        "MP3", "OGG", "PCM", "WMA", "eAAC+", "eAAC+"
    };
    
    info.supportedAudioSources = {
        "default", "mic", "camcorder", "voice_call",
        "voice_communication", "voice_downlink", "voice_uplink"
    };
    
    info.microphoneCount = 3;
    info.microphoneNames = "Bottom Mic, Top Mic, Back Mic";
    
    return info;
}

// ============================================================================
// GPSInfo Implementation
// ============================================================================

GPSInfo GPSInfo::getDefault() {
    GPSInfo info;
    
    info.gpsVersion = "1.0";
    info.gpsSize = "1040384";
    info.gpsFlags = "1";
    
    info.supportsGPS = true;
    info.supportsGLONASS = true;
    info.supportsBEIDOU = true;
    info.supportsGALILEO = true;
    info.supportsQZSS = true;
    
    info.gpsAccuracy = "2.0";
    info.gpsSensitivity = "0.0";
    info.gpsVendor = "Qualcomm";
    
    return info;
}

// ============================================================================
// BiometricInfo Implementation
// ============================================================================

BiometricInfo BiometricInfo::getDefault() {
    BiometricInfo info;
    
    info.hasFingerprint = true;
    info.fingerprintVendor = "Qualcomm";
    info.fingerprintModel = "QTI Fingerprint Sensor";
    info.fingerprintVersion = "1.0";
    
    info.hasFaceUnlock = true;
    info.faceUnlockVendor = "Qualcomm";
    
    info.hasIrisScanner = false;
    
    return info;
}

// ============================================================================
// SecurityInfo Implementation
// ============================================================================

SecurityInfo SecurityInfo::getDefault() {
    SecurityInfo info;
    
    info.verifiedBootState = "green";
    info.verifiedBootLocked = "true";
    info.verifiedBootHardlocked = "true";
    info.verificationBootEnabled = "true";
    info.verificationBoot = "true";
    
    info.selinuxEnforcing = "Enforcing";
    info.selinuxStatus = "Enforcing";
    
    info.trustyVersion = "1.0";
    info.keymasterVersion = "4.1";
    info.gatekeeperVersion = "1.0";
    
    info.hasStrongbox = true;
    info.hasRKP = false;
    
    return info;
}
