#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>
#include <chrono>

namespace VirtualPhonePro {

struct AdvancedSpoofingResult {
    bool success;
    std::string category;
    std::string property;
    std::string originalValue;
    std::string newValue;
    std::string error;
    std::string message;
};

struct DeviceIDConfig {
    std::string androidId;
    std::string deviceId;
    std::string serialNumber;
    std::string buildId;
};

struct HardwareConfig {
    std::string cpuModel;
    std::string cpuAbi;
    std::string cpuHardware;
    int processorCount;
    long totalMemory;
};

struct SensorConfig {
    bool accelerometer;
    bool gyroscope;
    bool magnetometer;
    bool proximity;
    bool light;
    
    float accelerometerX;
    float accelerometerY;
    float accelerometerZ;
    
    float gyroscopeX;
    float gyroscopeY;
    float gyroscopeZ;
    
    float magnetometerX;
    float magnetometerY;
    float magnetometerZ;
};

struct WebRTCConfig {
    std::string localIp;
    std::string publicIp;
    std::vector<std::string> iceServers;
    bool enableProxy;
};

class AdvancedSpoofing {
public:
    AdvancedSpoofing();
    ~AdvancedSpoofing();
    
    bool initialize();
    bool isInitialized() const;
    
    // Device ID Spoofing
    AdvancedSpoofingResult spoofAndroidId(const std::string& androidId);
    AdvancedSpoofingResult spoofDeviceId(const std::string& deviceId);
    AdvancedSpoofingResult spoofSerialNumber(const std::string& serial);
    AdvancedSpoofingResult spoofBuildId(const std::string& buildId);
    
    // Hardware Spoofing
    AdvancedSpoofingResult spoofCPUModel(const std::string& cpuModel);
    AdvancedSpoofingResult spoofCPUAbi(const std::string& abi);
    AdvancedSpoofingResult spoofProcessorCount(int count);
    AdvancedSpoofingResult spoofTotalMemory(long memoryMB);
    
    // GPU Spoofing
    AdvancedSpoofingResult spoofGPURenderer(const std::string& renderer);
    AdvancedSpoofingResult spoofGPUVendor(const std::string& vendor);
    AdvancedSpoofingResult spoofOpenGLVersion(const std::string& version);
    AdvancedSpoofingResult spoofVulkanVersion(const std::string& version);
    
    // Sensor Spoofing
    AdvancedSpoofingResult spoofAccelerometer(float x, float y, float z);
    AdvancedSpoofingResult spoofGyroscope(float x, float y, float z);
    AdvancedSpoofingResult spoofMagnetometer(float x, float y, float z);
    AdvancedSpoofingResult spoofProximity(bool present);
    AdvancedSpoofingResult spoofLightSensor(float lux);
    
    AdvancedSpoofingResult enableSensorSpoofing();
    AdvancedSpoofingResult disableSensorSpoofing();
    
    // User-Agent Spoofing
    AdvancedSpoofingResult spoofUserAgent(const std::string& userAgent);
    std::string generateRandomUserAgent(const std::string& browser = "Chrome", const std::string& os = "Android");
    
    // WebRTC Spoofing
    AdvancedSpoofingResult spoofWebRTCLocalIP(const std::string& ip);
    AdvancedSpoofingResult spoofWebRTCPublicIP(const std::string& ip);
    AdvancedSpoofingResult enableWebRTCProxy();
    AdvancedSpoofingResult disableWebRTCProxy();
    
    // Widevine & DRM
    AdvancedSpoofingResult spoofWidevineLevel(int level);
    AdvancedSpoofingResult spoofHDCPLevel(const std::string& level);
    AdvancedSpoofingResult enableDRMEmulation();
    AdvancedSpoofingResult disableDRMEmulation();
    
    // SafetyNet & Play Integrity
    AdvancedSpoofingResult spoofSafetyNetResponse(const std::map<std::string, std::string>& response);
    AdvancedSpoofingResult spoofPlayIntegrityResult(const std::string& nonce, const std::string& result);
    AdvancedSpoofingResult enableBasicIntegrity();
    AdvancedSpoofingResult enableDeviceIntegrity();
    AdvancedSpoofingResult enableNoCtsMismatch();
    
    // Tracker Blocking
    AdvancedSpoofingResult blockTracker(const std::string& trackerDomain);
    AdvancedSpoofingResult unblockTracker(const std::string& trackerDomain);
    std::vector<std::string> getBlockedTrackers();
    AdvancedSpoofingResult loadBlocklist(const std::string& filepath);
    
    // Utility
    std::string generateRandomAndroidId();
    std::string generateRandomSerial();
    std::string generateRandomDeviceId();
    std::string generateRandomBuildFingerprint();
    
    bool resetAll();
    bool resetCategory(const std::string& category);
    std::map<std::string, std::string> getCurrentSpoofState();

private:
    bool applySpoof(const std::string& property, const std::string& value);
    std::string getCurrentValue(const std::string& property);
    void backupOriginalValue(const std::string& property, const std::string& value);
    
    std::string generateRandomHex(int length);
    std::string generateRandomAlphanumeric(int length);
    std::string formatSerialNumber();
    std::string formatAndroidId();
    
    bool m_initialized;
    bool m_sensorSpoofingEnabled;
    bool m_webRTCPProxyEnabled;
    bool m_drmEmulationEnabled;
    
    std::map<std::string, std::string> m_originalValues;
    std::map<std::string, std::string> m_currentValues;
    std::map<std::string, std::string> m_blockedTrackers;
    
    std::vector<std::string> m_appliedSpoofs;
    std::vector<std::string> m_trustedTrackers;
    
    static const std::vector<std::string> DEVICE_ID_PROPERTIES;
    static const std::vector<std::string> HARDWARE_PROPERTIES;
    static const std::vector<std::string> GPU_PROPERTIES;
    static const std::vector<std::string> WEBRTC_PROPERTIES;
    static const std::vector<std::string> DRM_PROPERTIES;
    static const std::vector<std::string> TRACKER_DOMAINS;
};

}
