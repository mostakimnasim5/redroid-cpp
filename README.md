# RedroidCPP - Professional Android Emulator Manager

<div align="center">

![Version](https://img.shields.io/badge/Version-2.0.0-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-green)
![License](https://img.shields.io/badge/License-Apache--2.0-orange)
![Platform](https://img.shields.io/badge/Platform-Linux-yellow)

**Professional-grade C++ application for managing virtual Android devices with realistic device spoofing capabilities.**

*Built for Banking App Testing, Security Testing, and Anti-Detection Verification*

</div>

---

## 🎯 Features

### Device Profile Generation
- **IMEI Generation** with valid Luhn check digits
- **Serial Number Generation** based on manufacturer-specific formats
- **MAC Address Generation** with valid OUI prefixes
- **Android ID & GSF ID Generation**
- **Complete Build Fingerprint Generation**

### Supported Manufacturers
| Manufacturer | Models | TAC Codes |
|-------------|--------|-----------|
| Samsung | Galaxy S24/S23/Z Fold/Flip, A-series | 35875109, 35776608 |
| Google | Pixel 8/7/6/5/Fold | 35746608, 35441008 |
| Xiaomi | Xiaomi 14/13, Redmi, POCO | 86917102, 86100208 |
| OnePlus | OnePlus 12/11/10T, Nord | 45890508 |
| OPPO | Find X7/X6, Reno | 86536703 |
| Vivo | X100/X90, V30 | 86538903 |
| Huawei | Mate 60, P60, Mate X5 | 86799304 |
| Motorola | Edge 50/40/30 | 35899405 |
| Sony | Xperia 1/5/10 | 35885607 |
| ASUS | ROG Phone, Zenfone | 35892008 |
| Realme | GT 5 Pro, C67 | 86936203 |
| Nokia | G42, X30 | 35918108 |

### Device Properties Generated
- ✅ IMEI / IMEI2 (Dual SIM)
- ✅ Serial Number
- ✅ Android ID
- ✅ GSF ID
- ✅ WiFi MAC Address
- ✅ Bluetooth MAC Address
- ✅ Build Fingerprint
- ✅ Bootloader Version
- ✅ Radio/Baseband Version
- ✅ Display Resolution/DPI/FPS
- ✅ CPU/GPU Information
- ✅ Memory Configuration
- ✅ SIM/Operator Information
- ✅ Security Patch Level
- ✅ SELinux Status

## 📋 Requirements

- **OS:** Linux (Ubuntu 20.04+, Debian 11+)
- **Compiler:** GCC 9+ or Clang 10+
- **CMake:** 3.16+
- **Docker:** 20.10+ (for container management)
- **ADB:** For device control

## 🔧 Installation

```bash
# Clone the repository
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp

# Build with CMake
mkdir build && cd build
cmake ..
make

# Or compile directly
g++ -std=c++17 -O2 -Wall -o redroid-cli src/main.cpp

# Install
sudo make install
```

## 🚀 Usage

### Command Line Interface

```bash
# Show help
./redroid-cli help

# Create a device
./redroid-cli create -m Samsung
./redroid-cli create -m Google --name "My Pixel"
./redroid-cli create -m Xiaomi -a 14.0.0

# Generate profile only (without saving)
./redroid-cli profile -m OnePlus

# List all devices
./redroid-cli list

# Show device information
./redroid-cli info <device-id>

# Delete a device
./redroid-cli delete <device-id>

# Validate IMEI
./redroid-cli validate 358751090123456

# Show system status
./redroid-cli status

# List supported manufacturers
./redroid-cli manufacturers
```

### Output Example

```
═══════════════════════════════════════════════════════════
                    DEVICE INFORMATION
═══════════════════════════════════════════════════════════

[ BASIC INFO ]
  ID:           dev_95fd3485
  Name:         Test-Samsung
  Manufacturer: Samsung
  Model:        Galaxy S23+
  Status:       created
  Port:         5555

[ DEVICE IDENTITY ]
  IMEI:         357766095455908
  IMEI2:        357766097696426
  Serial:       BMDQUD9FYVI7
  Android ID:   d3e71ae65cfc3d12
  GSF ID:       4323468124

[ NETWORK ]
  WiFi MAC:     8C:71:F8:AB:CD:EF
  Bluetooth:    8C:71:F8:12:34:56

[ BUILD ]
  Fingerprint:   Samsung/Samsung/z3:14/UP1A.4289383/...
  Bootloader:    Mz36995
  Security Patch: 2024-11-01
  Android:      14 (UpsideDownCake)

[ DISPLAY ]
  Resolution:   1440x3120
  DPI:          560
  FPS:          120
```

## 📁 Project Structure

```
redroid-cpp/
├── include/
│   ├── Data/
│   │   └── TACDatabase.h        # TAC code database for IMEI generation
│   └── Core/
│       ├── DeviceProfile.h       # Complete device profile structures
│       └── DeviceManager.h       # Singleton device manager
├── src/
│   ├── main.cpp                 # CLI application
│   ├── Data/
│   │   └── TACDatabase.cpp      # TAC database implementation
│   └── Core/
│       ├── DeviceProfile.cpp     # Device profile implementation
│       └── DeviceManager.cpp     # Device manager implementation
├── tests/                       # Unit tests
├── docs/                        # Documentation
├── CMakeLists.txt              # CMake build configuration
└── README.md                   # This file
```

## 🔒 Security & Compliance

This software is designed for **legitimate testing purposes only**:

- ✅ **Banking App Testing** - Test banking applications in isolated environments
- ✅ **Security Research** - Verify app security measures
- ✅ **Anti-Detection Testing** - Test if apps can detect emulated devices
- ✅ **QA Testing** - Quality assurance for Android applications

### ⚠️ Disclaimer

> **This software is provided for authorized testing and development purposes only.** 
> Users are responsible for ensuring compliance with applicable laws and terms of service.
> The developers assume no liability for misuse of this software.

## 📊 Technical Details

### IMEI Generation Algorithm

The IMEI is generated using the **Luhn algorithm** for check digit validation:

```
TAC (8 digits) + SNR (6 digits) + Check Digit (1 digit) = 15 digits
```

### TAC Database

Type Allocation Codes (TAC) are validated codes assigned to device models by the GSMA.
Our database includes real TAC codes for major manufacturers to ensure authenticity.

### Device Fingerprint Structure

```
{manufacturer}/{brand}/{device}:{version}/{build_id}/{build_id}:
{type}/{tags}
```

Example:
```
Samsung/Samsung/z3:14/UP1A.231005.007/UP1A.231005.007:user/release-keys
```

## 🤝 Contributing

Contributions are welcome! Please read our contributing guidelines before submitting PRs.

## 📄 License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.

## 👨‍💻 Author

**RedroidCPP Development Team**

---

<div align="center">

**Made with ❤️ for the Android testing community**

</div>
