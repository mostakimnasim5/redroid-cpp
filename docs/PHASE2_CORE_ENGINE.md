# VirtualPhonePro - Phase 2: Core Engine Specification

## Overview

Phase 2 implements the core engine that manages multiple ReDroid instances with device spoofing capabilities.

## Components to Implement

### 2.1 InstanceManager

```cpp
// src/core/InstanceManager.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include "DeviceProfile.h"

namespace VirtualPhonePro {

enum class InstanceState {
    Stopped,
    Starting,
    Running,
    Paused,
    Error
};

struct InstanceInfo {
    std::string id;                    // Unique instance ID
    std::string name;                  // Display name
    int adbPort;                       // ADB port (5555, 5557, etc.)
    int vncPort;                        // VNC port (5900, 5902, etc.)
    std::string containerId;            // Docker container ID
    std::string containerIP;            // Docker internal IP
    InstanceState state;
    std::string profileId;              // Linked device profile
    uint64_t memoryLimit;               // Memory limit in MB
    time_t startedAt;
    time_t createdAt;
};

class InstanceManager {
public:
    static InstanceManager& getInstance();
    
    // Lifecycle
    std::string createInstance(const std::string& profileId);
    bool startInstance(const std::string& instanceId);
    bool stopInstance(const std::string& instanceId);
    bool restartInstance(const std::string& instanceId);
    bool deleteInstance(const std::string& instanceId);
    
    // State
    InstanceState getState(const std::string& instanceId);
    std::vector<InstanceInfo> listInstances();
    std::optional<InstanceInfo> getInstance(const std::string& instanceId);
    
    // Configuration
    bool setMemoryLimit(const std::string& instanceId, uint64_t mb);
    bool setProfile(const std::string& instanceId, const std::string& profileId);
    
private:
    InstanceManager() = default;
    int allocatePort();
    std::string generateInstanceId();
};

} // namespace VirtualPhonePro
```

### 2.2 DeviceSpoofer

```cpp
// src/core/DeviceSpoofer.h
#pragma once

#include <string>
#include <optional>
#include "DeviceProfile.h"

namespace VirtualPhonePro {

struct SpoofConfig {
    bool spoofIMEI = true;
    bool spoofMAC = true;
    bool spoofAndroidId = true;
    bool spoofGSF = true;
    bool spoofSerial = true;
    bool spoofFingerprint = true;
    bool spoofSensors = true;
    bool spoofNetwork = true;
    bool spoofGPS = true;
};

class DeviceSpoofer {
public:
    static DeviceSpoofer& getInstance();
    
    // Apply spoofing to running instance
    bool applySpoofing(const std::string& instanceId, 
                       const DeviceProfile& profile,
                       const SpoofConfig& config);
    
    // Verify spoofing is active
    bool verifySpoofing(const std::string& instanceId);
    
    // Individual spoofing methods
    bool spoofIMEI(const std::string& instanceId, const std::string& imei);
    bool spoofMAC(const std::string& instanceId, const std::string& mac);
    bool spoofSerial(const std::string& instanceId, const std::string& serial);
    bool spoofFingerprint(const std::string& instanceId, const DeviceProfile& profile);
    bool spoofGPS(const std::string& instanceId, double lat, double lon);
    
    // SafetyNet/Play Integrity
    bool spoofSafetyNet(const std::string& instanceId);
    bool spoofPlayIntegrity(const std::string& instanceId);
    
private:
    DeviceSpoofer() = default;
};

} // namespace VirtualPhonePro
```

### 2.3 DeviceProfile (Enhanced)

```cpp
// src/core/DeviceProfile.h
#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace VirtualPhonePro {

using json = nlohmann::json;

// Device Identity
struct DeviceIdentity {
    std::string imei;                  // 15 digits, Luhn validated
    std::string imei2;                 // Dual SIM
    std::string serialNumber;           // Manufacturer format
    std::string androidId;              // 16 hex chars
    std::string gsfId;                 // 10 digits
    std::string advertisingId;          // UUID format
};

// MAC Addresses
struct MACAddresses {
    std::string wifi;                  // XX:XX:XX:XX:XX:XX
    std::string bluetooth;
    std::string ethernet;
};

// Build Information
struct BuildInfo {
    std::string brand;                 // samsung
    std::string manufacturer;          // Samsung Electronics
    std::string model;                 // SM-S928B
    std::string device;                // dm3q
    std::string product;               // dm3q
    std::string fingerprint;           // Full Android fingerprint
    std::string bootloader;            // S928BXXU1AXXX
    std::string buildId;               // UP1A.231005.007
    std::string securityPatch;         // 2024-01-01
};

// Network
struct NetworkConfig {
    std::string hostname;              // android-xxxx
    std::string wifiMac;
    std::string ipAddress;
    std::string dns1;
    std::string dns2;
};

// SIM
struct SIMConfig {
    std::string iccid;                 // 20 digits
    std::string imsi;                 // 15 digits
    std::string carrier;               // T-Mobile
    std::string country;               // US
    std::string mcc;                   // 310
    std::string mnc;                   // 260
};

// GPS
struct GPSConfig {
    double latitude;
    double longitude;
    double altitude;
    float accuracy;
};

// Complete Profile
class DeviceProfile {
public:
    std::string id;
    std::string name;                  // "Samsung Galaxy S24 Ultra"
    std::string manufacturer;         // "Samsung"
    
    DeviceIdentity identity;
    MACAddresses mac;
    BuildInfo build;
    NetworkConfig network;
    SIMConfig sim;
    GPSConfig gps;
    
    // Metadata
    std::string createdAt;
    std::string modifiedAt;
    std::string createdBy;             // "system" or username
    
    // Serialization
    json toJson() const;
    static DeviceProfile fromJson(const json& j);
    bool save(const std::string& path) const;
    static std::optional<DeviceProfile> load(const std::string& path);
    
    // Generation
    static DeviceProfile generate(const std::string& manufacturer,
                                   const std::string& model = "");
};

// Profile Manager
class ProfileManager {
public:
    static ProfileManager& getInstance();
    
    std::vector<DeviceProfile> listProfiles();
    std::optional<DeviceProfile> getProfile(const std::string& id);
    bool saveProfile(const DeviceProfile& profile);
    bool deleteProfile(const std::string& id);
    
    // Built-in profiles
    std::vector<DeviceProfile> getBuiltInProfiles();
    
private:
    ProfileManager();
    std::string profilesDir;
};

} // namespace VirtualPhonePro
```

## Implementation Tasks

### Task 2.1: Instance Lifecycle
- [ ] Implement port allocation algorithm
- [ ] Implement Docker container creation
- [ ] Implement container start/stop/restart
- [ ] Implement instance state tracking
- [ ] Add event callbacks for state changes

### Task 2.2: Device Spoofing
- [ ] Implement IMEI spoofing with Luhn validation
- [ ] Implement MAC address spoofing
- [ ] Implement fingerprint generation
- [ ] Implement GPS spoofing
- [ ] Implement SafetyNet response spoofing
- [ ] Implement Play Integrity spoofing

### Task 2.3: Profile Management
- [ ] Implement profile JSON serialization
- [ ] Implement built-in profile library
- [ ] Implement profile import/export
- [ ] Implement profile validation

## Next Phase

[Phase 3: Docker Integration](./PHASE3_DOCKER_INTEGRATION.md)

---

*VirtualPhonePro - Phase 2*
