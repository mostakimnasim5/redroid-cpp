# ReDroidCPP - Windows Build Guide

## Overview

ReDroidCPP v3.0 is a professional-grade Android emulator controller with ultra-realistic phone UI, written in C++/Qt6.

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Download: https://visualstudio.microsoft.com/downloads/
   - Install: "Desktop development with C++"

2. **Qt 6.5+** with MSVC 2022 64-bit
   - Download: https://www.qt.io/download-qt-installer
   - Components needed:
     - Qt 6.5.x
     - Qt Core, Qt Gui, Qt Widgets, Qt Network

3. **CMake 3.20+**
   - Download: https://cmake.org/download/
   - Add to PATH during installation

4. **Git for Windows** (optional)
   - Download: https://git-scm.com/download/win

## Quick Build (Recommended)

### Step 1: Run the build script

Double-click `build-release.bat` or run from command prompt:

```batch
build-release.bat
```

This script will:
- Auto-detect Visual Studio and Qt6 installations
- Build the complete project with all modules
- Create a portable release folder `ReDroidCPP_v3/`
- Copy all required DLLs and dependencies

### Step 2: Run the application

```batch
cd ReDroidCPP_v3
start.bat
```

## Manual Build

### Step 1: Open Developer Command Prompt

Open **x64 Native Tools Command Prompt for VS 2022**

### Step 2: Configure CMake

```batch
cd path\to\redroid-cpp
mkdir build
cd build

cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QT6_GUI=ON ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2022_64"
```

### Step 3: Build

```batch
cmake --build . --config Release --parallel
```

### Step 4: Run

```batch
.\bin\ReDroidCPP.exe
```

## Release Folder Structure

After running `build-release.bat`:

```
ReDroidCPP_v3/
├── ReDroidCPP.exe              # Main application
├── Qt6Core.dll                # Qt runtime
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Network.dll
├── plugins/
│   └── platforms/
│       └── qwindows.dll       # Windows platform
├── docker/
│   ├── Dockerfile
│   ├── docker-compose.yml
│   └── entrypoint.sh
├── profiles/
│   ├── samsung_s24_ultra.json
│   ├── google_pixel_8_pro.json
│   └── ...
├── start.bat                  # Launcher script
├── adb-connect.bat           # ADB helper
└── README.txt
```

## Features

### Professional UI
- Ultra-realistic phone frame with rounded corners
- Camera notch simulation
- Hardware navigation buttons
- Real-time status bar (FPS, Connection, Battery)

### Anti-Detection (40+ modules)
- SafetyNet/Play Integrity bypass
- TLS Fingerprinting (JA3/JA4)
- Hardware spoofing (CPU, GPU, RAM)
- Root/Frida/Xposed detection bypass
- Banking app optimization

### Multi-Instance Support
- Run multiple Android devices simultaneously
- Unique device identity per instance
- Isolated network per instance

## Troubleshooting

### "Qt6 not found"
```batch
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2022_64"
```

### "Visual Studio not found"
Run from **x64 Native Tools Command Prompt for VS 2022**

### Application crashes on start
```batch
set QT_PLUGIN_PATH=plugins
ReDroidCPP.exe
```

### Build errors
```batch
rmdir /s /q build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Dependencies

| Component | Version | Required |
|-----------|---------|----------|
| C++ Standard | C++17 | Yes |
| CMake | 3.20+ | Yes |
| Qt6 | 6.5+ | Yes |
| MSVC | 2022 | Yes |
| Docker | 20.10+ | Optional |

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `-DBUILD_QT6_GUI=ON` | Build Qt6 GUI | ON |
| `-DCMAKE_BUILD_TYPE=Release` | Build type | Release |
| `-DCMAKE_PREFIX_PATH` | Qt installation path | Auto |

## Support

- GitHub: https://github.com/mostakimnasim5/redroid-cpp
- Documentation: See README.md
