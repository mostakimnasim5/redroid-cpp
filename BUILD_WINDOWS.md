# ReDroidCPP - Windows Build Guide

## Overview

ReDroidCPP is a professional-grade Android emulator controller written in C++/Qt6. This guide will help you build the project on Windows.

## Prerequisites

### Required Software

1. **Visual Studio 2022** or **MSVC Build Tools**
   - Download: https://visualstudio.microsoft.com/downloads/
   - Install: "Desktop development with C++"

2. **CMake 3.20+**
   - Download: https://cmake.org/download/
   - Add to PATH during installation

3. **Qt6 SDK**
   - Download: https://www.qt.io/download-qt-installer
   - Install Qt 6.5+ with MSVC 2022 64-bit
   - Components needed:
     - Qt 6.5.x
     - Qt Core
     - Qt Gui
     - Qt Widgets
     - Qt Network

4. **Git for Windows**
   - Download: https://git-scm.com/download/win

## Build Instructions

### Method 1: Using CMake GUI

1. Open CMake GUI
2. Set source code directory to: `C:\path\to\redroid-cpp`
3. Set build directory to: `C:\path\to\redroid-cpp\build`
4. Click "Configure"
5. Select generator: "Visual Studio 17 2022" or "Visual Studio 17 2022 Win64"
6. Check option: `BUILD_QT6_GUI`
7. Click "Generate"
8. Click "Open Project"
9. Build in Visual Studio

### Method 2: Using Command Line

```batch
@echo off
REM ReDroidCPP Windows Build Script

echo ========================================
echo  ReDroidCPP - Windows Build
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake 3.20+
    pause
    exit /b 1
)

REM Check for Qt6
if not exist "C:\Qt\6.5\msvc2022_64\bin\qmake.exe" (
    echo WARNING: Qt6 not found in default location.
    echo Please install Qt6 from https://www.qt.io/download
    echo.
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring project...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DBUILD_QT6_GUI=ON ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.5\msvc2022_64"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo.
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo  Executable location: build\Release\virtualphonepro-qt.exe
echo ========================================
pause
```

### Method 3: Using Qt Creator

1. Open Qt Creator
2. File → Open File or Project
3. Select `CMakeLists.txt`
4. Configure kit: Desktop Qt 6.5.x MSVC2022 64bit
5. Run → Build All
6. Run → Execute

## Project Structure

```
redroid-cpp/
├── CMakeLists.txt              # Main build configuration
├── src/
│   ├── main.cpp                # CLI entry point
│   ├── mainwindow.cpp/h        # Qt6 main window
│   ├── qtmain.cpp              # Qt6 Windows entry
│   ├── AutoStartDialog.cpp/h   # Docker auto-start dialog
│   └── ReDroidController/       # Core controller library
│       ├── ReDroidController.cpp
│       ├── MultiInstanceManager.cpp
│       ├── SafetyNetSpoofer.cpp
│       ├── BankingAppSpoofer.cpp
│       ├── DeepDeviceSpoofer.cpp
│       ├── GoogleFacebookSpoofer.cpp
│       ├── UniqueDeviceGenerator.cpp
│       ├── AdvancedAntiDetection.cpp
│       ├── TLSFingerprint.cpp
│       ├── HyperRealisticTouchEmulator.cpp
│       ├── EnhancedDeviceProfile.cpp
│       └── DeviceProfile.cpp
├── include/VirtualPhonePro/     # Public headers
├── docker/                     # Docker configuration
└── profiles/                   # Device profiles
```

## Features

### Core Features
- **Device Profile Generation**: Samsung, Google Pixel, Xiaomi, Huawei, OnePlus, etc.
- **Multi-Instance Management**: Run multiple Android instances simultaneously
- **SafetyNet Bypass**: Hardware attestation spoofing
- **Banking App Support**: Root/Hook/Emulator detection bypass

### Anti-Detection Features
- **Behavioral Analysis Prevention**: Human typing patterns, touch simulation
- **TLS Fingerprinting**: JA3/JA4 SSL fingerprint spoofing
- **Hardware Emulation**: CPU, GPU, Battery, Thermal simulation
- **OEM Deep Spoofing**: Samsung Knox, Huawei HMS, Xiaomi MIUI

### Windows-Specific
- **Native Qt6 GUI**: Modern Windows application
- **Windows Defender Compatible**: No special permissions required
- **x64 Native**: Optimized for modern Windows PCs

## Usage

### Running the Application

1. Launch `virtualphonepro-qt.exe`
2. Click "New Instance" to create an Android device
3. Select device profile (Samsung S24 Ultra recommended)
4. Click "Create"
5. Connect via VNC or ADB

### CLI Usage

```bash
# Create instance
redroid-cli create --manufacturer samsung --model "SM-S928B"

# List instances
redroid-cli list

# Apply spoofing
redroid-cli spoof --instance 0 --profile banking

# Start instance
redroid-cli start --instance 0

# Stop instance
redroid-cli stop --instance 0
```

## Troubleshooting

### CMake not found
```
set PATH=C:\Program Files\CMake\bin;%PATH%
```

### Qt6 not found
```
set Qt6_DIR=C:\Qt\6.5\msvc2022_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=C:\Qt\6.5\msvc2022_64
```

### Build errors
```
# Clean build directory
rmdir /s /q build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

## Dependencies

| Component | Version | Required |
|-----------|---------|----------|
| C++ Standard | C++17 | Yes |
| CMake | 3.20+ | Yes |
| Qt6 | 6.5+ | Yes (for GUI) |
| MSVC | 2022 | Yes (Windows) |
| OpenSSL | 1.1+ | Optional |

## License

This project is for authorized testing purposes only.

## Support

- GitHub Issues: https://github.com/mostakimnasim5/redroid-cpp/issues
- Documentation: See README.md
