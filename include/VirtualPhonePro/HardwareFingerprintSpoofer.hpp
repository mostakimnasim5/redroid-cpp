#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace AntiDetect {

struct HardwareFingerprint {
    std::string cpuModel;
    std::string cpuHardware;
    std::string cpuVariant;
    int cpuCores;
    int cpuThreads;
    
    std::string gpuRenderer;
    std::string gpuVendor;
    std::string gpuVersion;
    
    std::string deviceModel;
    std::string deviceManufacturer;
    std::string deviceBrand;
    std::string deviceBoard;
    std::string deviceHardware;
    
    std::string bootloaderVersion;
    std::string radioVersion;
    std::string kernelVersion;
    
    std::string dmiSystemVendor;
    std::string dmiSystemProduct;
    std::string dmiBoardVendor;
    std::string dmiBoardProduct;
    
    bool isValidated;
};

struct SpoofResult {
    bool success;
    std::string message;
    std::string error;
    std::map<std::string, std::string> details;
};

class HardwareFingerprintSpoofer {
public:
    static HardwareFingerprintSpoofer& getInstance();
    
    HardwareFingerprintSpoofer();
    ~HardwareFingerprintSpoofer();
    
    bool initialize();
    bool isInitialized() const;
    void shutdown();
    
    // Core Spoofing
    SpoofResult enableAllSpoofing();
    SpoofResult disableAllSpoofing();
    
    // CPU Fingerprint Spoofing
    SpoofResult spoofCPUInfo(const std::string& cpuModel, int cores, int threads);
    SpoofResult spoofCPUHardware(const std::string& hardware);
    SpoofResult spoofCPUVariant(const std::string& variant);
    SpoofResult setExynos2100Profile();
    SpoofResult setSnapdragon888Profile();
    SpoofResult setSnapdragon8Gen1Profile();
    SpoofResult setDimensity9000Profile();
    
    // GPU Fingerprint Spoofing
    SpoofResult spoofGPUInfo(const std::string& gpuModel);
    SpoofResult setMaliG78Profile();
    SpoofResult setAdreno660Profile();
    SpoofResult setAdreno730Profile();
    SpoofResult setMaliG710Profile();
    
    // Device Fingerprint Spoofing
    SpoofResult spoofDeviceInfo(const std::string& manufacturer, 
                                const std::string& model,
                                const std::string& brand);
    SpoofResult setSamsungGalaxyS21Profile();
    SpoofResult setSamsungGalaxyS22Profile();
    SpoofResult setGooglePixel6Profile();
    SpoofResult setGooglePixel7Profile();
    SpoofResult setXiaomi12Profile();
    SpoofResult setOnePlus10Profile();
    
    // Bootloader & Radio Spoofing
    SpoofResult spoofBootloaderVersion(const std::string& version);
    SpoofResult spoofRadioVersion(const std::string& version);
    SpoofResult spoofKernelVersion(const std::string& version);
    
    // DMI/SMBIOS Spoofing (for emulator detection)
    SpoofResult spoofDMIInfo(const std::string& vendor,
                            const std::string& product,
                            const std::string& boardVendor,
                            const std::string& boardProduct);
    SpoofResult setRealHardwareDMI();
    
    // Battery & Hardware Features
    SpoofResult spoofBatteryInfo(int level, const std::string& status, 
                                const std::string& health);
    SpoofResult spoofHardwareFeatures();
    SpoofResult spoofSupportedABIs(const std::vector<std::string>& abis);
    
    // Fingerprint & Face ID
    SpoofResult spoofBiometricInfo();
    SpoofResult hideBiometricEnrollment();
    
    // USB & Connectivity
    SpoofResult spoofUSBInfo();
    SpoofResult spoofBluetoothInfo();
    SpoofResult spoofWifiInfo();
    
    // Build Fingerprint
    SpoofResult spoofBuildFingerprint(const std::string& fingerprint);
    SpoofResult generateSamsungFingerprint();
    SpoofResult generateGoogleFingerprint();
    SpoofResult generateXiaomiFingerprint();
    
    // Validation
    SpoofResult validateSpoofing();
    bool isSpoofingActive() const;
    HardwareFingerprint getCurrentSpoofedFingerprint();
    
    // Status
    std::map<std::string, std::string> getDetailedStatus();
    SpoofResult getStatus();

private:
    void applyCPUChanges(const HardwareFingerprint& fp);
    void applyGPUChanges(const HardwareFingerprint& fp);
    void applyDeviceChanges(const HardwareFingerprint& fp);
    void applyDMIChanges(const HardwareFingerprint& fp);
    void restoreOriginalValues();
    
    std::string generateRandomHex(int length);
    std::string generateBuildFingerprint(const std::string& brand,
                                        const std::string& device,
                                        const std::string& model);
    
    bool m_initialized;
    bool m_spoofingActive;
    HardwareFingerprint m_currentSpoof;
    HardwareFingerprint m_originalSpoof;
    
    std::map<std::string, std::string> m_spoofedProperties;
    std::map<std::string, std::string> m_originalProperties;
};

}
