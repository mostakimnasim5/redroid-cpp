#pragma once

#include <string>
#include <map>
#include <vector>

namespace AntiDetect {

enum class SafetyNetIntegrityLevel {
    UNKNOWN = 0,
    MEETS_BASIC_INTEGRITY = 1,
    MEETS_DEVICE_INTEGRITY = 2,
    MEETS_STRONG_INTEGRITY = 3,
    MEETS_LEGACY_DEVICE_INTEGRITY = 3,  // Legacy name
    CERTIFIED = 4
};

enum class BootState {
    UNKNOWN,
    GREEN,
    ORANGE,
    RED
};

struct SafetyNetIntegrityToken {
    bool isValid;
    std::string basicIntegrity;
    std::string ctsProfileMatch;
    std::string basicIntegrityNegotiveAdvise;
    std::string basicIntegrityErrMsg;
    std::string deviceIntegrity;
    std::string accountConsistency;
    std::string accountDefense;
    
    // Device attributes
    bool secureFolder;
    bool KnoxDetected;
    bool DebugDetected;
    bool EmulatorDetected;
    bool RootDetected;
    bool VirtualizationDetected;
    bool screenLockEnabled;
    bool usesStorageEncryption;
    bool usesBootloader;
    bool isHardwareAffestedRoot;
    bool isSafetyNetEnabled;
    
    // Additional checks
    bool platformKnownVersion;
    bool basicIntegrityPast;
    bool deviceCertificate;
};

struct SafetyNetResult {
    bool success;
    std::string message;
    std::string error;
    SafetyNetIntegrityToken token;
    std::map<std::string, std::string> details;
};

class SafetyNetAdvancedBypass {
public:
    static SafetyNetAdvancedBypass& getInstance();
    
    SafetyNetAdvancedBypass();
    ~SafetyNetAdvancedBypass();
    
    bool initialize();
    bool isInitialized() const;
    void shutdown();
    
    // === FULL BYPASS SEQUENCE ===
    SafetyNetResult performFullBypass();
    
    // === CORE BYPASS FUNCTIONS ===
    
    // Root Detection Bypass
    SafetyNetResult bypassRootDetection();
    SafetyNetResult hideRootBinary();
    SafetyNetResult hideSUBinary();
    SafetyNetResult hideMagisk();
    SafetyNetResult hideSuperSU();
    SafetyNetResult installRootCloak();
    SafetyNetResult useMagiskHide();
    
    // Boot Integrity
    SafetyNetResult setVerifiedBootState(BootState state);
    SafetyNetResult setGreenBootState();
    SafetyNetResult setOrangeBootState();
    SafetyNetResult setRedBootState();
    SafetyNetResult spoofTCPOptions();
    SafetyNetResult bypassVerifiedBoot();
    SafetyNetResult setBootloaderLocked();
    
    // SELinux & Security
    SafetyNetResult enforceSELinux();
    SafetyNetResult setSELinuxEnforcing();
    SafetyNetResult enableDMVerity();
    SafetyNetResult disableDebugFlags();
    SafetyNetResult hideDebugSymbols();
    
    // System Properties
    SafetyNetResult hideSystemProperties();
    SafetyNetResult spoofBuildTags();
    SafetyNetResult setReleaseKeys();
    SafetyNetResult spoofSecurityPatch();
    SafetyNetResult setLatestSecurityPatch();
    
    // Play Services Detection
    SafetyNetResult bypassPlayServicesChecks();
    SafetyNetResult hideGMS();
    SafetyNetResult spoofGMSVersion();
    SafetyNetResult disableSafetyNet();
    SafetyNetResult hookSafetyNetAPI();
    
    // Hardware Attestation
    SafetyNetResult prepareHardwareAttestation();
    SafetyNetResult setHardwareAttestationKey();
    SafetyNetResult generateAttestationCertificate();
    SafetyNetResult setKeystoreFlags();
    
    // API Level Spoofing
    SafetyNetResult spoofAPILevel(int level);
    SafetyNetResult setAPILevel33();
    SafetyNetResult setAPILevel34();
    SafetyNetResult spoofBuildVersion();
    
    // Integrity Token Generation
    SafetyNetResult generateIntegrityToken(SafetyNetIntegrityLevel level);
    SafetyNetResult setBasicIntegrity();
    SafetyNetResult setCertifiedIntegrity();
    SafetyNetResult setStrongIntegrity();
    SafetyNetResult setCTSCProfileMatch();
    SafetyNetResult setDeviceIntegrity(const std::string& level);
    
    // Response Hooking
    SafetyNetResult hookIntegrityAPI();
    SafetyNetResult hookPlayIntegrityAPI();
    SafetyNetResult setMockResponse(const std::string& api, const std::string& response);
    
    // File System Checks
    SafetyNetResult hideSystemMounts();
    SafetyNetResult hideSystemBinaries();
    SafetyNetResult checkForDangerousApps();
    SafetyNetResult hideXposed();
    SafetyNetResult hideFrida();
    
    // Battery & Power
    SafetyNetResult setBatteryHealth(const std::string& health);
    SafetyNetResult setBatteryStatus(const std::string& status);
    SafetyNetResult disablePowerSaving();
    
    // Memory & Process
    SafetyNetResult checkMemoryTampering();
    SafetyNetResult hideDebuggableProcess();
    SafetyNetResult secureMemoryAllocation();
    
    // Device State
    SafetyNetResult setDeviceSecure();
    SafetyNetResult setScreenLocked();
    SafetyNetResult setEncryptionEnabled();
    SafetyNetResult setKeyguardSecure();
    
    // Pre-built Profiles
    SafetyNetResult applySamsungProfile();
    SafetyNetResult applyGoogleProfile();
    SafetyNetResult applyStockROMProfile();
    SafetyNetResult applyMinimalProfile();
    
    // Validation
    SafetyNetResult validateAllChecks();
    bool isBypassActive() const;
    SafetyNetIntegrityToken getCurrentToken();
    
    // Status
    std::map<std::string, std::string> getDetailedStatus();
    SafetyNetResult getStatus();

private:
    void prepareBypassEnvironment();
    void applyIntegrityToken();
    void restoreSystemState();
    
    std::string generateNonce();
    std::string signAttestation();
    std::string encryptPayload();
    
    bool m_initialized;
    bool m_bypassActive;
    SafetyNetIntegrityToken m_currentToken;
    
    std::map<std::string, std::string> m_modifiedProperties;
    std::map<std::string, std::string> m_backupValues;
};

}
