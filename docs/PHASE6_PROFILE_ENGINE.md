# VirtualPhonePro - Phase 6: Device Profiles & ProfileEngine

## Overview

Phase 6 implements the device profile system that generates and manages realistic device identities for anti-detection.

## Profile System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      ProfileEngine                               │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                  Profile Database                          │ │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │ │
│  │  │ Samsung │ │ Google  │ │ Xiaomi  │ │ OnePlus │          │ │
│  │  │ Profile │ │ Profile │ │ Profile │ │ Profile │          │ │
│  │  │  Pool   │ │  Pool   │ │  Pool   │ │  Pool   │          │ │
│  │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘          │ │
│  │       └──────────┬┴──────────┬┴──────────┬┘                │ │
│  │                  │           │           │                  │ │
│  │            ┌────┴───────────┴───────────┴────┐            │ │
│  │            │     TAC Database Lookup         │            │ │
│  │            │   (Real Device Identifiers)    │            │ │
│  │            └─────────────────────────────────┘            │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                  Profile Generator                         │ │
│  │                                                            │ │
│  │  • IMEI Generator (Luhn validated)                        │ │
│  │  • MAC Generator (Valid OUI)                              │ │
│  │  • Serial Generator (Manufacturer format)                │ │
│  │  • Fingerprint Generator                                 │ │
│  │  • Sensor Data Generator                                 │ │
│  │  • GPS Location Generator                                │ │
│  │                                                            │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Profile JSON Schema

```json
{
    "id": "uuid-v4",
    "name": "Samsung Galaxy S24 Ultra",
    "manufacturer": "Samsung",
    "version": "1.0.0",
    
    "identity": {
        "imei": "358751090123456",
        "imei2": "358751090123457",
        "serialNumber": "R5CW80XXXXXX",
        "androidId": "a1b2c3d4e5f6g7h8",
        "gsfId": "1234567890",
        "advertisingId": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    },
    
    "build": {
        "brand": "samsung",
        "manufacturer": "samsung",
        "model": "SM-S928B",
        "device": "dm3q",
        "product": "dm3q",
        "board": "dm3q",
        "hardware": "qcom",
        "fingerprint": "samsung/dm3q/dm3q:14/UP1A.231005.007/xxx:userdebug/release-keys",
        "bootloader": "S928BXXU1AXXX",
        "buildId": "UP1A.231005.007",
        "buildType": "userdebug",
        "securityPatch": "2024-01-01",
        "androidVersion": "14",
        "sdkVersion": "34"
    },
    
    "network": {
        "wifiMac": "8C:71:F8:XX:XX:XX",
        "bluetoothMac": "8C:71:F8:XX:XX:XX",
        "ethernetMac": "00:1A:11:XX:XX:XX",
        "hostname": "android-dm3q"
    },
    
    "sim": {
        "iccid": "8961012345678901234",
        "imsi": "310260123456789",
        "carrier": "T-Mobile",
        "country": "US",
        "mcc": "310",
        "mnc": "260"
    },
    
    "gps": {
        "latitude": 37.7749295,
        "longitude": -122.4194155,
        "altitude": 10.0,
        "accuracy": 5.0,
        "provider": "gps"
    },
    
    "sensors": {
        "accelerometer": {
            "name": "LSM6DSO Accelerometer",
            "vendor": "STMicroelectronics",
            "version": 1,
            "resolution": 0.001196,
            "maxRange": 78.4532
        },
        "gyroscope": {
            "name": "LSM6DSO Gyroscope",
            "vendor": "STMicroelectronics",
            "version": 1,
            "resolution": 0.001221,
            "maxRange": 34.906586
        },
        "magneticField": {
            "name": "AK09918 Magnetometer",
            "vendor": "Asahi Kasei",
            "version": 1,
            "resolution": 0.15,
            "maxRange": 4912.0
        }
    },
    
    "hardware": {
        "cpu": {
            "architecture": "arm64-v8a",
            "processor": "ARM Implementer 88",
            "hardware": "qcom",
            "modelName": "Snapdragon 8 Gen 3",
            "coreCount": 8,
            "frequencies": [300000, 2400000]
        },
        "gpu": {
            "renderer": "Adreno (TM) 750",
            "vendor": "Qualcomm",
            "openGlVersion": "3.2",
            "vulkanVersion": "1.1.269"
        },
        "memory": {
            "totalRam": 12288000000,
            "totalRamGB": 12,
            "heapSize": 512000000,
            "largeHeapSize": 4096000000
        }
    },
    
    "security": {
        "selinux": "Enforcing",
        "verifiedBoot": "green",
        "verifiedBootLocked": true,
        "keymasterVersion": "4.1",
        "strongboxAvailable": true,
        "hasHardwareAttestation": true
    },
    
    "metadata": {
        "createdAt": "2024-01-15T10:30:00Z",
        "modifiedAt": "2024-01-15T10:30:00Z",
        "createdBy": "system",
        "tags": ["high-end", "flagship", "5g"]
    }
}
```

## Components

### 6.1 ProfileEngine

```cpp
// src/core/ProfileEngine.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include "DeviceProfile.h"

namespace VirtualPhonePro {

class ProfileEngine {
public:
    static ProfileEngine& getInstance();
    
    // Profile Management
    std::vector<DeviceProfile> getAllProfiles();
    std::optional<DeviceProfile> getProfile(const std::string& id);
    std::optional<DeviceProfile> getProfileByName(const std::string& name);
    bool saveProfile(const DeviceProfile& profile);
    bool deleteProfile(const std::string& id);
    bool profileExists(const std::string& id);
    
    // Built-in Profiles
    std::vector<DeviceProfile> getBuiltInProfiles();
    void loadBuiltInProfiles();
    
    // Profile Generation
    DeviceProfile generateProfile(const std::string& manufacturer,
                                    const std::string& model = "");
    
    DeviceProfile generateRandomProfile();
    
    // Profile Validation
    bool validateProfile(const DeviceProfile& profile);
    std::vector<std::string> getProfileErrors(const DeviceProfile& profile);
    
    // Import/Export
    bool importProfile(const std::string& filePath);
    bool exportProfile(const std::string& profileId, 
                      const std::string& filePath);
    
    // Categories
    std::vector<DeviceProfile> getProfilesByManufacturer(const std::string& mfr);
    std::vector<DeviceProfile> getProfilesByTag(const std::string& tag);
    std::vector<DeviceProfile> searchProfiles(const std::string& query);
    
    // TAC Database
    bool isValidTAC(const std::string& tac);
    std::string getManufacturerFromTAC(const std::string& tac);
    
private:
    ProfileEngine();
    void initialize();
    void loadProfilesFromDisk();
    void saveProfilesToDisk();
    
    std::string m_profilesDir;
    std::map<std::string, DeviceProfile> m_profiles;
    std::map<std::string, std::string> m_manufacturerProfiles;  // First profile per manufacturer
};
```

### 6.2 Profile Generator

```cpp
// src/core/ProfileGenerator.h
#pragma once

#include <string>
#include <random>
#include "DeviceProfile.h"

namespace VirtualPhonePro {

class ProfileGenerator {
public:
    static ProfileGenerator& getInstance();
    
    // Full profile generation
    DeviceProfile generate(const std::string& manufacturer,
                          const std::string& model = "");
    
    // Individual generators
    std::string generateIMEI(const std::string& tac);
    std::string generateIMEI2(const std::string& tac);
    std::string generateSerial(const std::string& manufacturer);
    std::string generateAndroidId();
    std::string generateGSFId();
    std::string generateAdvertisingId();
    
    // MAC Address generation
    std::string generateWifiMAC(const std::string& manufacturer);
    std::string generateBluetoothMAC(const std::string& manufacturer);
    std::string generateEthernetMAC();
    
    // SIM generation
    std::string generateICCID();
    std::string generateIMSI(const std::string& mcc, const std::string& mnc);
    
    // Fingerprint generation
    std::string generateFingerprint(const DeviceProfile& profile);
    
    // GPS coordinates
    double generateLatitude();
    double generateLongitude();
    
    // Sensor data
    SensorCalibration generateSensorCalibration(const std::string& sensorType);
    
private:
    int calculateLuhnCheckDigit(const std::string& base);
    bool validateLuhn(const std::string& number);
    std::string getOUIForManufacturer(const std::string& manufacturer);
    
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<int> m_dist;
};

} // namespace VirtualPhonePro
```

### 6.3 TAC Database

```cpp
// src/core/TACDatabase.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace VirtualPhonePro {

struct TACEntry {
    std::string tac;                // 8-digit TAC
    std::string brand;              // Brand name
    std::string modelName;          // Marketing model name
    std::string internalName;       // Internal codename
    std::string deviceClass;       // "Smartphone", "Tablet"
    std::string launchYear;         // "2024"
    std::string launchMonth;        // "01"
    std::string gpuModel;           // "Adreno 750"
    std::string cpuModel;           // "Snapdragon 8 Gen 3"
};

class TACDatabase {
public:
    static TACDatabase& getInstance();
    
    std::optional<TACEntry> getByTAC(const std::string& tac);
    std::vector<TACEntry> getByManufacturer(const std::string& manufacturer);
    std::vector<TACEntry> getByBrand(const std::string& brand);
    std::vector<TACEntry> getByDeviceClass(const std::string& deviceClass);
    std::optional<TACEntry> getRandomForManufacturer(const std::string& manufacturer);
    std::optional<TACEntry> getRandom();
    
    std::vector<std::string> getManufacturers();
    size_t size() const { return m_entries.size(); }
    
private:
    TACDatabase();
    void initialize();
    
    std::map<std::string, TACEntry> m_entries;
    std::map<std::string, std::vector<std::string>> m_manufacturerTACs;
    std::map<std::string, std::vector<std::string>> m_brandTACs;
};

} // namespace VirtualPhonePro
```

### 6.4 Built-in Profile Data

```cpp
// Samsung Profiles
namespace SamsungProfiles {
    const char* GALAXY_S24_ULTRA = R"({
        "name": "Samsung Galaxy S24 Ultra",
        "manufacturer": "Samsung",
        "tac": "35875109",
        "model": "SM-S928B",
        "internalName": "dm3q",
        "androidVersion": "14",
        "cpu": "Snapdragon 8 Gen 3",
        "gpu": "Adreno 750",
        "ram": "12GB"
    })";
    
    const char* GALAXY_S24_PLUS = R"({
        "name": "Samsung Galaxy S24+",
        "manufacturer": "Samsung",
        "tac": "35875108",
        "model": "SM-S926B",
        "internalName": "z3s",
        "androidVersion": "14",
        "cpu": "Exynos 2400",
        "gpu": "Xclipse 940",
        "ram": "12GB"
    })";
}

// Google Pixel Profiles
namespace GoogleProfiles {
    const char* PIXEL_8_PRO = R"({
        "name": "Google Pixel 8 Pro",
        "manufacturer": "Google",
        "tac": "35746608",
        "model": "GA04777",
        "internalName": "husky",
        "androidVersion": "14",
        "cpu": "Tensor G3",
        "gpu": "Immortalis-G715s MC10",
        "ram": "12GB"
    })";
    
    const char* PIXEL_8 = R"({
        "name": "Google Pixel 8",
        "manufacturer": "Google",
        "tac": "35746609",
        "model": "GA04809",
        "internalName": "shiba",
        "androidVersion": "14",
        "cpu": "Tensor G3",
        "gpu": "Mali-G715",
        "ram": "8GB"
    })";
}

// Xiaomi Profiles
namespace XiaomiProfiles {
    const char* XIAOMI_14_PRO = R"({
        "name": "Xiaomi 14 Pro",
        "manufacturer": "Xiaomi",
        "tac": "86917102",
        "model": "23127PN0CC",
        "internalName": "sm8550",
        "androidVersion": "14",
        "cpu": "Snapdragon 8 Gen 3",
        "gpu": "Adreno 750",
        "ram": "16GB"
    })";
}
```

## Profile Categories

| Category | Description | Examples |
|----------|-------------|----------|
| `flagship` | High-end flagship devices | Galaxy S24 Ultra, Pixel 8 Pro |
| `high-end` | Premium mid-range | Galaxy S24+, Xiaomi 14 |
| `mid-range` | Mid-range devices | Galaxy A54, Pixel 7a |
| `budget` | Budget devices | Galaxy A14, Redmi Note 12 |
| `gaming` | Gaming-focused | ROG Phone, Red Magic |
| `foldable` | Foldable devices | Galaxy Z Fold, Pixel Fold |

## Implementation Tasks

### Task 6.1: Profile Storage
- [ ] Implement JSON serialization
- [ ] Implement profile directory management
- [ ] Implement built-in profile loading
- [ ] Implement profile import/export

### Task 6.2: Profile Generation
- [ ] Implement IMEI generator with Luhn validation
- [ ] Implement MAC generator with valid OUI
- [ ] Implement fingerprint generator
- [ ] Implement sensor data generator

### Task 6.3: TAC Database
- [ ] Populate TAC database with real device codes
- [ ] Implement TAC lookup methods
- [ ] Implement manufacturer/model mapping

### Task 6.4: Validation
- [ ] Implement profile validation rules
- [ ] Implement error reporting
- [ ] Implement automatic correction

## Next Steps

After Phase 6, proceed to:
- [Phase 7: Network Isolation & Spoofing](./PHASE7_NETWORK_SPOOFING.md)
- [Phase 8: SafetyNet/Play Integrity](./PHASE8_SAFETYNET.md)
- [Phase 9: Sensor Spoofing](./PHASE9_SENSOR_SPOOFING.md)
- [Phase 10: Testing & Polish](./PHASE10_TESTING.md)

---

*VirtualPhonePro - Phase 6*
