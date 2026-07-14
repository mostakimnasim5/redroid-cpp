# ReDroidCPP - Professional Android Emulator Controller

<div align="center">

![Version](https://img.shields.io/badge/Version-3.0.0-blue)
![C++](https://img.shields.io/badge/C++-17-green)
![Qt](https://img.shields.io/badge/Qt-6.5+-purple)
![License](https://img.shields.io/badge/License-Testing--Only-red)

**Enterprise-grade Android emulator controller with advanced anti-detection features**

</div>

---

## 🎯 Overview

ReDroidCPP is a professional-grade C++/Qt6 application for managing ReDroid (Real Docker) Android containers. It provides advanced device spoofing, multi-instance management, and comprehensive anti-detection features for banking app testing and security research.

## ✨ Features

### Core Features

| Feature | Description |
|---------|-------------|
| **Multi-Instance Management** | Run multiple Android instances simultaneously |
| **Device Profile Generation** | Samsung, Google Pixel, Xiaomi, Huawei, OnePlus, etc. |
| **Docker Integration** | Auto-start containers with ReDroid |
| **Qt6 GUI** | Modern Windows application |
| **REST API** | HTTP API for automation |

### Anti-Detection Features

| Feature | Description |
|---------|-------------|
| **SafetyNet/Play Integrity** | Hardware attestation spoofing |
| **Banking App Support** | Root/Hook/Emulator detection bypass |
| **Behavioral Analysis** | Human typing patterns, touch simulation |
| **TLS Fingerprinting** | JA3/JA4 SSL fingerprint spoofing |
| **Hardware Emulation** | CPU, GPU, Battery, Thermal simulation |
| **OEM Deep Spoofing** | Samsung Knox, Huawei HMS, Xiaomi MIUI |

## 📊 Project Statistics

| Metric | Value |
|--------|-------|
| **Source Files** | 39 |
| **Total Lines** | 15,657 |
| **Modules** | 18 |
| **Documentation** | Complete |

## 🏗️ Architecture

```
ReDroidCPP/
├── include/VirtualPhonePro/         # Public headers
│   ├── ReDroidController.h         # Main controller
│   ├── DeviceProfile.h             # Device profiles
│   ├── BankingAppSpoofer.h         # Banking app bypass
│   ├── AdvancedAntiDetection.hpp    # Anti-detection
│   ├── TLSFingerprint.hpp           # SSL fingerprinting
│   └── HyperRealisticTouchEmulator.hpp
│
├── src/
│   ├── ReDroidController/          # Implementation
│   │   ├── ReDroidController.cpp
│   │   ├── BankingAppSpoofer.cpp
│   │   ├── AdvancedAntiDetection.cpp
│   │   └── ...
│   ├── main.cpp                    # CLI entry
│   ├── mainwindow.cpp              # Qt6 GUI
│   └── AutoStartDialog.cpp         # Docker auto-start
│
├── docker/                         # Docker configs
├── profiles/                       # Device profiles
└── CMakeLists.txt                 # Build system
```

## 🔒 Anti-Detection Module Tree

```
Anti-Detection System
│
├── 🔐 Identity Spoofing
│   ├── Unique Device IDs (IMEI, Serial, Android ID)
│   ├── MAC Addresses (WiFi, Bluetooth)
│   ├── SIM Cards (ICCID, IMSI)
│   └── Advertising IDs
│
├── 🛡️ Security Bypass
│   ├── SafetyNet Attestation
│   ├── Play Integrity API
│   ├── Root Detection Bypass
│   ├── Hook Detection Bypass
│   └── SSL Pinning Bypass
│
├── 🌐 Network Spoofing
│   ├── TLS Fingerprinting (JA3/JA4)
│   ├── DNS Configuration
│   └── Proxy Support
│
├── 📱 Hardware Emulation
│   ├── CPU Simulation (8-core)
│   ├── GPU (Adreno 750)
│   ├── Battery State
│   └── Thermal Management
│
├── 👆 Touch Simulation
│   ├── Tap/Double Tap
│   ├── Swipe (All directions)
│   ├── Pinch-to-Zoom
│   └── Pressure Sensitivity
│
└── 🏭 OEM Deep Spoofing
    ├── Samsung Knox
    ├── Huawei HMS
    ├── Xiaomi MIUI
    └── Qualcomm QSEE
```

## 📦 Supported Devices

| Manufacturer | Models |
|-------------|--------|
| Samsung | Galaxy S24 Ultra, S23, A-series |
| Google | Pixel 8, 7, 6 series |
| Xiaomi | Mi 13, Redmi Note series |
| Huawei | P60, Mate series |
| OnePlus | 11, 10 series |
| Custom | Any Android device |

## 🚀 Installation

### Prerequisites

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ desktop development
- **Qt 6.5+** with MSVC 2022 64-bit
- **CMake 3.20+**
- **Docker Desktop** (for container support)

### Quick Start

```bash
# Clone the repository
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp

# Build for Windows
.\build-windows.bat

# Run the application
.\build\Release\virtualphonepro-qt.exe
```

## 🛠️ Building

### Windows (CMake)

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_QT6_GUI=ON
cmake --build . --config Release
```

### Windows (Qt Creator)

1. Open Qt Creator
2. File → Open File or Project
3. Select `CMakeLists.txt`
4. Configure kit: Desktop Qt 6.5.x MSVC2022 64bit
5. Build and Run

### Linux

```bash
mkdir build && cd build
cmake .. -DBUILD_QT6_GUI=ON
make -j$(nproc)
```

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [BUILD_WINDOWS.md](BUILD_WINDOWS.md) | Windows build instructions |
| [BUILD_GUIDE.md](BUILD_GUIDE.md) | General build guide |

## 🚀 Usage

### GUI Mode

```bash
# Launch Qt6 GUI
virtualphonepro-qt.exe
```

### CLI Mode

```bash
# Create instance
redroid-cli create --manufacturer samsung --model "SM-S928B"

# List instances
redroid-cli list

# Apply banking profile
redroid-cli spoof --instance 0 --profile banking

# Start instance
redroid-cli start --instance 0
```

## 📋 Requirements

| Component | Version | Required |
|-----------|---------|----------|
| C++ Standard | C++17 | Yes |
| CMake | 3.20+ | Yes |
| Qt6 | 6.5+ | Yes (for GUI) |
| OpenSSL | 1.1+ | Optional |
| Docker | 20.10+ | Optional |

## ⚖️ Disclaimer

> **This software is for authorized testing purposes only.**
> - Banking App Testing
> - Security Research
> - Anti-Detection Verification
> - QA Testing

## 📄 License

Licensed for authorized testing purposes only.

---

**Version 3.0.0** - Complete Professional Implementation
