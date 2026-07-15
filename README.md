# ReDroidCPP - Professional Android Emulator Controller

<div align="center">

![Version](https://img.shields.io/badge/Version-5.0.0-blue)
![C++](https://img.shields.io/badge/C++-17-green)
![Qt](https://img.shields.io/badge/Qt-6.5+-purple)
![Platform](https://img.shields.io/badge/Platform-Windows-red)

**Enterprise-grade Android emulator controller with advanced anti-detection features**

</div>

---

## 🎯 Overview

ReDroidCPP is a professional-grade C++/Qt6 application for managing Android emulators on Windows. It works with **Android Studio Emulator** to provide advanced device spoofing and comprehensive anti-detection features for banking app testing and security research.

> 💡 **No Docker Required!** - This project uses Android Studio Emulator instead of Docker.

---

## 🚀 Quick Start (Windows)

### ১. Install Requirements
```
- Android Studio (with Virtual Device)
- Visual Studio 2022
- Qt6 (optional for GUI)
```

### ২. Start Android Studio Emulator
```
Android Studio → Tools → Device Manager → Start Pixel 6 Pro
```

### ৩. Build & Run
```powershell
.\build-win.bat Release
.\launch-gui.bat
```

### ৪. Apply Device Profile
```
GUI-তে Profile বেছে নিন → Apply Profile
```

## ✨ Features

### Core Features

| Feature | Description |
|---------|-------------|
| **Multi-Instance Management** | Run multiple Android instances simultaneously |
| **Device Profile Generation** | Samsung, Google Pixel, Xiaomi, Huawei, OnePlus, etc. |
| **Android Studio Emulator** | Works with standard Android Studio emulator |
| **Qt6 GUI** | Modern Windows application |
| **ADB Integration** | Direct ADB commands for device control |

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
│   ├── AndroidRealismEngine.hpp    # 100% realistic spoofing
│   ├── BankingAppSpoofer.h         # Banking app bypass
│   ├── HardwareAttestation.h        # Hardware attestation
│   ├── SafetyNetSpoofer.h          # SafetyNet/Play Integrity
│   └── ...
│
├── src/
│   ├── ReDroidController/          # Implementation
│   │   ├── ReDroidController.cpp
│   │   ├── AndroidRealismEngine.cpp
│   │   ├── BankingAppSpoofer.cpp
│   │   ├── SafetyNetSpoofer.cpp
│   │   └── ...
│   ├── main.cpp                    # CLI entry
│   ├── mainwindow.cpp              # Qt6 GUI
│   └── AutoStartDialog.cpp         # Emulator setup
│
├── profiles/                       # Device profiles
└── CMakeLists.txt                 # Build system
```

**Note:** Docker support has been removed. The project now uses Android Studio Emulator via ADB.

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
│   ├── SSL Pinning Bypass
│   └── Device Integrity Checks
│
├── 🌐 Network Spoofing
│   ├── TLS Fingerprinting (JA3/JA4)
│   ├── DNS Configuration
│   ├── Proxy Support
│   ├── Wi-Fi Calling Configuration
│   ├── VoLTE/VoWiFi Simulation
│   ├── Carrier Aggregation (CA)
│   └── Dual SIM Dual Standby
│
├── 📱 Hardware Emulation
│   ├── CPU Simulation (8-core)
│   ├── GPU (Adreno 750)
│   ├── Battery State
│   ├── Thermal Management
│   └── Power Profiles
│
├── 👆 Touch Simulation
│   ├── Tap/Double Tap
│   ├── Swipe (All directions)
│   ├── Pinch-to-Zoom
│   └── Pressure Sensitivity
│
├── 🏭 OEM Deep Spoofing
│   ├── Samsung Knox / Samsung Pay
│   ├── Huawei HMS / AppGallery
│   ├── Xiaomi MIUI / Mi Pay
│   └── Google Mobile Services
│
├── 📦 System App Simulation
│   ├── Carrier Bloatware (AT&T, Verizon, T-Mobile, etc.)
│   ├── OEM Pre-installed Apps
│   ├── App Hibernation Behavior
│   ├── Battery Optimization
│   └── Background Process Management
│
├── 🔑 SSL Certificate Management
│   ├── Root CA Certificates
│   ├── Intermediate CA Certificates
│   ├── OEM-Specific Certificates
│   └── Certificate Chain Validation
│
└── 📍 Device Services
    ├── Find My Device Status
    ├── Location Services
    ├── Device Health Indicators
    └── Remote Actions Simulation
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

## 🐳 Docker Android Emulator

This project includes a Docker-based Android emulator setup.

### Requirements

| Platform | Requirements |
|----------|-------------|
| **Windows** | Docker Desktop, VcXsrv, WSL2 |
| **Linux** | Docker, KVM (optional) |
| **macOS** | Docker Desktop |

### Quick Start (3 Steps)

#### Step 1: Open VcXsrv (Windows only)
```
Launch XLaunch from Start Menu
→ Select "Multiple windows"
→ Select "Start no client"
→ Check "Disable access control"
→ Finish
```

#### Step 2: Run the Emulator
```bash
# Double-click start.bat OR run in terminal:
cd docker
docker compose up --build
```

#### Step 3: Connect ADB
```bash
# Double-click adb-connect.bat OR run:
adb connect localhost:5555
```

### Common ADB Commands
```bash
# Connect to emulator
adb connect localhost:5555

# List devices
adb devices

# Open shell
adb shell

# Install APK
adb install app.apk

# Take screenshot
adb exec-out screencap -p > screenshot.png

# Reboot device
adb reboot
```

### Troubleshooting

| Error | Solution |
|-------|----------|
| "PANIC: Cannot find AVD system path" | Environment variables configured in docker-compose.yml |
| "ADB server not running" | Run `adb start-server` or use `adb-connect.bat` |
| Container exits immediately | Check logs with `docker compose logs` |
| Black screen / not visible | Ensure VcXsrv is running (Windows) |
| "DISPLAY not set" | Run `xhost +local:docker` (Linux) |

### Performance

| Feature | With HW Accel | Without |
|---------|--------------|---------|
| Boot Time | ~2-3 min | ~5-10 min |
| Uses | KVM/WHPX | Software |

### Project Structure
```
├── docker/
│   ├── Dockerfile         # Container image
│   ├── docker-compose.yml # Orchestration
│   └── entrypoint.sh      # Startup script
├── start.bat              # Windows launcher
├── adb-connect.bat        # ADB connection
└── README.md
```

---

**Version 3.0.0** - Complete Professional Implementation
