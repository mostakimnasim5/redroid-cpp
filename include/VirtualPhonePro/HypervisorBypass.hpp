#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace AntiDetect {

enum class HypervisorType {
    NONE,
    QEMU,
    GENUMERATION,
    VIRTUALBOX,
    VMWARE,
    HYPER_V,
    KVM,
    INTEL_VTX,
    AMD_V
};

struct TimingProfile {
    double baseLatencyNs;
    double cacheHitLatencyNs;
    double cacheMissLatencyNs;
    double contextSwitchNs;
    double syscallLatencyNs;
    double pageFaultNs;
};

struct HypervisorResult {
    bool success;
    std::string message;
    std::string error;
    std::map<std::string, std::string> details;
};

class HypervisorBypass {
public:
    static HypervisorBypass& getInstance();
    
    HypervisorBypass();
    ~HypervisorBypass();
    
    bool initialize();
    bool isInitialized() const;
    void shutdown();
    
    // Detection Bypass
    HypervisorResult enableBypass();
    HypervisorResult disableBypass();
    HypervisorResult setDeviceAsRealHardware();
    
    // CPU Timing Normalization
    HypervisorResult enableTimingNormalization();
    HypervisorResult disableTimingNormalization();
    HypervisorResult setTimingProfile(const TimingProfile& profile);
    HypervisorResult useNaturalTiming();
    
    // Hardware Info Spoofing
    HypervisorResult spoofCPUInfo(const std::string& cpuModel, int cores, int threads);
    HypervisorResult spoofGPUInfo(const std::string& gpuModel);
    HypervisorResult spoofKernelVersion(const std::string& version);
    
    // Virtualization Detection Bypass
    HypervisorResult disableVMDetection();
    HypervisorResult hideHypervisorIndicators();
    HypervisorResult setCPUModel(const std::string& model);
    HypervisorResult setCPUFeatures(const std::vector<std::string>& features);
    
    // QEMU Detection Bypass
    HypervisorResult bypassQEMUDetection();
    HypervisorResult setQEMUIndicators(bool isQEMU);
    
    // VMware/VirtualBox Detection Bypass
    HypervisorResult bypassVMDetection();
    HypervisorResult hideVMArtifacts();
    
    // ARM Simulation
    HypervisorResult enableARMSimulation();
    HypervisorResult disableARMSimulation();
    HypervisorResult setARMProperties();
    
    // Cache Timing Attack Prevention
    HypervisorResult enableCacheTimingProtection();
    HypervisorResult disableCacheTimingProtection();
    HypervisorResult addCacheNoise(double level);
    
    // Status
    HypervisorResult getStatus();
    std::map<std::string, std::string> getDetailedStatus();
    HypervisorType detectCurrentHypervisor();
    
private:
    void applyAllBypasses();
    void removeAllBypasses();
    std::string getDefaultCPUModel();
    int getDefaultCPUCores();
    
    bool m_initialized;
    bool m_bypassEnabled;
    bool m_timingNormalizationEnabled;
    bool m_cacheProtectionEnabled;
    bool m_armSimulationEnabled;
    
    HypervisorType m_currentHypervisor;
    TimingProfile m_timingProfile;
    std::string m_spoofedCPUModel;
    int m_spoofedCPUCores;
    
    std::vector<std::string> m_originalProperties;
    std::map<std::string, std::string> m_modifiedProperties;
};

}
