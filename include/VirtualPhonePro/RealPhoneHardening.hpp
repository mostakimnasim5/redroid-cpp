#pragma once

#include <string>
#include <vector>
#include <map>

namespace VirtualPhonePro {

struct HardeningConfig {
    bool hideRoot;
    bool hideMagisk;
    bool hideSU;
    bool hideXposed;
    bool hideSELinux;
    bool hideDebugFlags;
    bool enableDmVerity;
    bool enableVerifiedBoot;
    bool hideBuildType;
    bool hideUserDebug;
    bool hideTestKeys;
    bool enablePlayIntegrity;
    bool enableSafetyNet;
    bool hidePowerUserMode;
};

struct BatteryHardening {
    bool enabled;
    int batteryLevel;
    std::string chargingStatus;
    std::string chargingType;
    std::string temperature;
    std::string health;
    bool isCharging;
};

struct RadioHardening {
    bool enabled;
    std::string basebandVersion;
    std::string radioVersion;
    std::string kernelVersion;
    std::string cpuAbi;
};

struct KernelHardening {
    bool enabled;
    bool hideProcfs;
    bool spoofCpuTiming;
    bool addSensorNoise;
    std::string kernelVersion;
};

struct EmulatorBypass {
    bool enabled;
    bool bypassQEMU;
    bool bypassGenymotion;
    bool bypassBlueStacks;
    bool bypassNox;
    bool bypassLDPlayer;
    bool bypassVirtualBox;
    bool bypassWine;
};

struct CanvasSpoof {
    bool enabled;
    int spoofMode;  // 0=random, 1=realistic, 2=custom
    std::string customPattern;
};

struct WebGLSpoof {
    bool enabled;
    std::string renderer;
    std::string vendor;
    std::string version;
    std::string extensions;
};

struct AudioSpoof {
    bool enabled;
    std::string audioContext;
    std::string sampleRate;
};

class RealPhoneHardening {
public:
    static RealPhoneHardening& getInstance();
    
    bool initialize();
    bool shutdown();
    
    // Configuration
    void setConfig(const HardeningConfig& config);
    HardeningConfig getConfig() const;
    
    // Root Detection Bypass
    bool hideRoot();
    bool hideMagisk();
    bool hideSU();
    bool hideXposed();
    bool hideSelinuxStatus();
    bool hideDebugStatus();
    bool enableDmVerity();
    bool enableVerifiedBoot();
    bool hideBuildType();
    bool convertUserdebugToUser();
    bool hideTestKeys();
    
    // SafetyNet & Play Integrity
    bool enableSafetyNet();
    bool enablePlayIntegrity();
    bool mockSafetyNetResponse(bool basicIntegrity, bool ctsProfile);
    bool setPlayIntegrityResult(const std::string& nonce, const std::string& result);
    
    // Battery Hardening
    bool setBatteryState(int level, const std::string& status, const std::string& type);
    bool setBatteryTemperature(const std::string& temp);
    bool setBatteryHealth(const std::string& health);
    bool enableBatterySimulation();
    bool disableBatterySimulation();
    
    // Radio/Bandwidth Hardening
    bool spoofBaseband(const std::string& version);
    bool spoofRadioVersion(const std::string& version);
    bool spoofKernelVersion(const std::string& version);
    bool spoofCpuAbi(const std::string& abi);
    
    // Kernel Hardening
    bool hideProcfsContents();
    bool spoofCpuTiming();
    bool addSensorNoise();
    bool removeSensorNoise();
    bool hideLinuxSubsystem();
    
    // Emulator Detection Bypass
    bool bypassQEMUDetection();
    bool bypassGenymotionDetection();
    bool bypassBlueStacksDetection();
    bool bypassNoxDetection();
    bool bypassLDPlayerDetection();
    bool bypassVirtualBoxDetection();
    bool bypassAllEmulators();
    
    // Canvas Fingerprint Bypass
    bool enableCanvasSpoofing();
    bool disableCanvasSpoofing();
    bool setCanvasSpoofMode(int mode);
    bool setCustomCanvasPattern(const std::string& pattern);
    bool randomizeCanvasFingerprint();
    
    // WebGL Fingerprint Bypass
    bool spoofWebGLRenderer(const std::string& renderer);
    bool spoofWebGLVendor(const std::string& vendor);
    bool spoofWebGLVersion(const std::string& version);
    bool applyWebGLHardening();
    
    // Audio Fingerprint Bypass
    bool enableAudioSpoofing();
    bool disableAudioSpoofing();
    bool setAudioContext(const std::string& context);
    bool setSampleRate(const std::string& rate);
    
    // DNS Hardening
    bool setGoogleDNS();
    bool setCloudflareDNS();
    bool setCustomDNS(const std::string& dns1, const std::string& dns2);
    bool enablePrivateDNS();
    bool disablePrivateDNS();
    
    // Complete Apply
    bool applyAllHardening();
    bool applyEmulatorBypass();
    bool applyFingerprintBypass();
    
    // Reset
    bool resetAll();
    bool resetRootDetection();
    bool resetBattery();
    bool resetEmulatorBypass();
    bool resetFingerprint();
    
    // Status
    std::map<std::string, std::string> getHardeningStatus();
    bool isDeviceRealPhone();
    std::vector<std::string> getDetectionWarnings();

private:
    RealPhoneHardening();
    ~RealPhoneHardening();
    
    RealPhoneHardening(const RealPhoneHardening&) = delete;
    RealPhoneHardening& operator=(const RealPhoneHardening&) = delete;
    
    bool applyProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key);
    
    bool executeCommand(const std::string& command);
    
    HardeningConfig m_config;
    BatteryHardening m_batteryConfig;
    RadioHardening m_radioConfig;
    KernelHardening m_kernelConfig;
    EmulatorBypass m_emulatorConfig;
    CanvasSpoof m_canvasConfig;
    WebGLSpoof m_webglConfig;
    AudioSpoof m_audioConfig;
    
    bool m_initialized;
    bool m_hardeningApplied;
};

}
