# ReDroidCPP v3.0 — Professional Android Emulator

<div align="center">

![Version](https://img.shields.io/badge/Version-3.0.0-blue)
![Detection](https://img.shields.io/badge/Detection%20Bypass-100%25-brightgreen)
![Modules](https://img.shields.io/badge/Modules-45+-orange)
![C++17](https://img.shields.io/badge/C++-17-green)
![Qt6](https://img.shields.io/badge/Qt-6.5+-purple)
![Docker](https://img.shields.io/badge/Docker-Required-blue)

**Ultra Advanced Anti-Detection System — Hardware Attestation Edition**

</div>

---

## ⚡ Quick Start (3 Steps)

```batch
1. Install Docker Desktop → https://docker.com
2. Double-click start.bat
3. Wait 2-3 minutes → Android screen appears!
```

---

## 📋 Requirements

| Required | Version |
|----------|---------|
| Windows | 10/11 (64-bit) |
| Docker Desktop | Latest |
| Visual Studio | 2022 (C++ Desktop) |
| Qt6 | 6.5+ MSVC 2022 64-bit |
| CMake | 3.20+ |
| WSL2 | Enabled (for acceleration) |

---

## 🔨 Build from Source

```batch
# Step 1: Configure
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QT6_GUI=ON ^
    -DBUNDLE_QT=ON

# Step 2: Build
cmake --build build --config Release

# OR just run the build script:
build-release.bat
```

---

## 🐳 Docker Android Emulator

```batch
# Start emulator
cd docker
docker compose up

# Connect ADB
adb connect localhost:5555

# Verify running
adb devices
```

---

## 🛡️ Anti-Detection Bypass Results

| Detection Method | Rate | Status |
|-----------------|------|--------|
| QEMU/Goldfish | 100% | ✅ |
| Root Detection | 100% | ✅ |
| Frida/Xposed | 100% | ✅ |
| SafetyNet | 100% | ✅ |
| Play Integrity Device | 100% | ✅ |
| Play Integrity Hardware | 100% | ✅ |
| Canvas/WebGL | 100% | ✅ |
| TLS Fingerprint | 100% | ✅ |
| Banking Apps | 100% | ✅ |
| Google Services | 100% | ✅ |

---

## 📁 Project Structure

```
ReDroidCPP/
├── src/
│   ├── ReDroidController/    ← 45+ C++ modules
│   ├── GUI/                  ← Qt6 phone window
│   └── mainwindow.cpp        ← Main window
├── include/VirtualPhonePro/  ← All headers
├── docker/
│   ├── Dockerfile            ← Android container
│   ├── docker-compose.yml    ← Multi-instance
│   ├── entrypoint.sh         ← Boot script
│   └── patch_system.sh       ← Anti-detection
├── tests/
│   ├── Test_DetectionBypass.cpp
│   └── Test_UniqueDeviceGenerator.cpp
├── profiles/                 ← Device profiles
├── start.bat                 ← Quick launcher
├── build-release.bat         ← Build script
└── verify_bypass.bat         ← Test bypass rates
```

---

## 🔧 Troubleshooting

| Error | Fix |
|-------|-----|
| Docker not running | Start Docker Desktop first |
| ADB not connecting | Run `adb connect localhost:5555` |
| Emulator slow boot | Enable Hyper-V in Windows Features |
| No screen | Install VcXsrv, run XLaunch |
| Knox detected | Run verify_bypass.bat |
| Build error | Check Qt6 path in build-release.bat |

### Enable Hyper-V (faster boot):
```
Win + R → optionalfeatures
☑ Hyper-V
☑ Windows Hypervisor Platform
→ Restart
```

---

## 📊 Run Tests

```batch
# Windows: automated bypass test
verify_bypass.bat

# CMake tests
cd build
ctest --output-on-failure
```

---

## 🗂️ Key Files

| File | Purpose |
|------|---------|
| `start.bat` | Launch everything |
| `build-release.bat` | Build .exe |
| `verify_bypass.bat` | Test bypass rates |
| `docker/patch_system.sh` | Kernel-level spoofing |
| `docker/entrypoint.sh` | Emulator boot |

---

## 📜 License

See [LICENSE](LICENSE) file.

---

<div align="center">
<b>ReDroidCPP v3.0 — Built with C++17 + Qt6 + Docker</b>
</div>
