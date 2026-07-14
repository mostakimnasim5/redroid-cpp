# VirtualPhonePro - Systems Architecture Blueprint
## Commercial-Grade Anti-Detect Android Emulator Manager

**Version:** 1.0.0  
**Date:** 2024  
**Target Platform:** Windows 11 x64  
**Technology Stack:** C++17, Qt6, CMake, ReDroid (Android 14), Docker Desktop (WSL2)

---

## 1. OVERVIEW ANALYSIS

### 1.1 Project Objective

VirtualPhonePro is a Windows-native desktop application that orchestrates multiple ReDroid Android emulator instances for:
- 🏦 **Banking App Testing** - Authorized security testing
- 🔒 **Security Research** - Penetration testing, vulnerability assessment
- 🛡️ **Anti-Detection Verification** - Testing app detection mechanisms

### 1.2 Architecture Summary

```
┌─────────────────────────────────────────────────────────────────────┐
│                        WINDOWS 11 HOST                               │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │          VirtualPhonePro.exe (Qt6 C++ Application)          │    │
│  │                                                              │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │    │
│  │  │  Qt GUI   │  │  Docker  │  │   ADB    │  │ Profile  │   │    │
│  │  │  Layer    │  │  Engine  │  │  Bridge  │  │ Manager  │   │    │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │    │
│  │       │             │             │             │           │    │
│  │  ┌────┴─────────────┴─────────────┴─────────────┴────┐     │    │
│  │  │              Core Engine (C++)                     │     │    │
│  │  │   • Instance Manager  • Device Spoofer  • IPC     │     │    │
│  │  └──────────────────────┬────────────────────────────┘     │    │
│  └─────────────────────────┼──────────────────────────────────┘    │
│                            │                                           │
│  ┌─────────────────────────┴──────────────────────────────────┐     │
│  │              Communication Layer                              │     │
│  │  • Named Pipes: \\.\pipe\docker_engine                      │     │
│  │  • TCP: localhost:2375 (Docker REST API)                   │     │
│  │  • TCP: localhost:5555+ (ADB per instance)                  │     │
│  └─────────────────────────┬──────────────────────────────────┘     │
└────────────────────────────┼───────────────────────────────────────────┘
                             │
┌────────────────────────────┼───────────────────────────────────────────┐
│                     DOCKER DESKTOP (WSL2)                             │
│  ┌─────────────────────────────────────────────────────────────┐     │
│  │                    Docker Engine                             │     │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │     │
│  │  │  ReDroid-1  │ │  ReDroid-2  │ │  ReDroid-N  │           │     │
│  │  │  :5555      │ │  :5557      │ │  :5555+2N   │           │     │
│  │  │  (Android)  │ │  (Android)  │ │  (Android)  │           │     │
│  │  │  IMEI:XXX   │ │  IMEI:YYY   │ │  IMEI:ZZZ   │           │     │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │     │
│  └─────────────────────────────────────────────────────────────┘     │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │               WSL2 Linux Kernel                              │    │
│  │  • binder.ko  • ashmem.ko  • ion.ko                        │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.3 Technical Roadblocks & Solutions

| Roadblock | Risk Level | Solution |
|-----------|------------|----------|
| **WSL2 GPU Passthrough** | 🔴 High | Use `virglrenderer` for software GPU with host OpenGL |
| **binder/ashmem modules** | 🔴 High | Custom WSL2 kernel with Android modules or bind-mount |
| **Named Pipe Permissions** | 🟡 Medium | Run Docker Desktop with administrator privileges |
| **Port Conflicts** | 🟡 Medium | Dynamic port allocation with port reservation system |
| **Resource Isolation** | 🟡 Medium | Docker cgroups + network namespace isolation |
| **ADB Authentication** | 🟢 Low | Pre-authorize ADB connections with keys |
| **Memory Overcommit** | 🟡 Medium | Monitor via cgroup memory limits (512MB-768MB/instance) |
| **WSL2 Startup Latency** | 🟢 Low | Keep WSL2 warm, use persistent connections |

---

## 2. SYSTEM COMMUNICATION SETUP

### 2.1 Docker Engine Communication

#### Option A: Named Pipes (Windows)
```cpp
// Windows Named Pipe for Docker Engine
// Path: \\.\pipe\docker_engine

class DockerNamedPipe {
private:
    HANDLE hPipe;
    const std::string pipeName = "\\\\.\\pipe\\docker_engine";
    
public:
    bool connect() {
        hPipe = CreateFileA(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        return hPipe != INVALID_HANDLE_VALUE;
    }
    
    std::string sendCommand(const std::string& json) {
        // Send JSON request to Docker Engine API
        // Receive JSON response
    }
};
```

#### Option B: TCP Localhost (Docker REST API)
```cpp
// Enable Docker REST API on localhost:2375
// Configure in Docker Desktop -> Settings -> General

class DockerTCPClient {
private:
    std::string baseUrl = "http://localhost:2375";
    HINTERNET hInternet;
    
public:
    bool connect() {
        hInternet = WinHttpOpen(
            L"VirtualPhonePro/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            NULL, NULL, 0
        );
        return hInternet != NULL;
    }
    
    std::string listContainers() {
        return httpGet(baseUrl + "/containers/json");
    }
    
    bool createContainer(const nlohmann::json& config) {
        return httpPost(baseUrl + "/containers/create", config.dump());
    }
};
```

#### Recommended: Docker SDK (libdocker)
```cpp
// Use official Docker SDK for C++
#include <docker/docker.hpp>

class DockerEngine {
private:
    docker::DockerClient client;
    
public:
    std::vector<ContainerInfo> listInstances() {
        return client.listContainers();
    }
    
    std::string createInstance(const InstanceConfig& config) {
        return client.createContainer(config.toJson());
    }
    
    bool startInstance(const std::string& id) {
        return client.startContainer(id);
    }
};
```

### 2.2 ADB Communication

```cpp
// ADB.exe location: Windows SDK platform-tools
// Per-instance ADB connection

class ADBBridge {
private:
    std::string adbPath;
    std::string instanceIP;
    int adbPort;  // Base: 5555, Instance 1: 5555, Instance 2: 5557, etc.
    
public:
    bool connect(const std::string& instanceIP, int port) {
        // Execute: adb.exe connect <ip>:<port>
        std::string cmd = fmt::format("{} connect {}:{}", 
            adbPath, instanceIP, port);
        return execSync(cmd) == 0;
    }
    
    bool executeShell(const std::string& instanceID, 
                      const std::string& command,
                      std::string& output) {
        // Execute: adb.exe -s <instanceID> shell <command>
        std::string cmd = fmt::format("{} -s {} shell \"{}\"",
            adbPath, instanceID, command);
        return execSyncCapture(cmd, output) == 0;
    }
    
    bool setProp(const std::string& instanceID,
                 const std::string& prop, 
                 const std::string& value) {
        return executeShell(instanceID, 
            fmt::format("setprop {} {}", prop, value));
    }
    
    bool pushFile(const std::string& instanceID,
                  const std::string& localPath,
                  const std::string& remotePath) {
        std::string cmd = fmt::format("{} -s {} push \"{}\" \"{}\"",
            adbPath, instanceID, localPath, remotePath);
        return execSync(cmd) == 0;
    }
};
```

### 2.3 Instance-Specific Communication Matrix

```
┌────────────┬──────────┬──────────┬─────────────────────────┐
│ Instance # │ ADB Port │ VNC Port │ Internal IP (Docker)   │
├────────────┼──────────┼──────────┼─────────────────────────┤
│ 1          │ 5555     │ 5900     │ 172.17.0.2             │
│ 2          │ 5557     │ 5902     │ 172.17.0.3             │
│ 3          │ 5559     │ 5904     │ 172.17.0.4             │
│ N          │ 5555+2N  │ 5900+2N  │ 172.17.0.(N+1)         │
└────────────┴──────────┴──────────┴─────────────────────────┘

Note: Even ports for ADB, odd ports for VNC
```

---

## 3. WSL2 ENVIRONMENT CONFIGURATION

### 3.1 Prerequisites

```powershell
# 1. Enable required Windows features
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart

# 2. Install WSL2
wsl --install

# 3. Set WSL2 as default
wsl --set-default-version 2

# 4. Install Ubuntu (or your preferred distro)
wsl --install -d Ubuntu-22.04
```

### 3.2 Custom WSL2 Kernel with Android Modules

#### Option A: Use Pre-built Android Kernel
```bash
# Clone WSL2 kernel sources with Android patches
git clone --depth 1 https://github.com/microsoft/WSL2-Linux-Kernel.git
cd WSL2-Linux-Kernel

# Apply Android binder/ashmem patches
git apply patches/android-binder-*.patch

# Build kernel
make KCONFIG_CONFIG=Microsoft/config-wsl KERNEL_VERSION=5.15.y
cp vmlinux /mnt/c/Users/$USER/wsl2-android-kernel
```

#### Option B: Use WSLg with system modules
```bash
# In WSL2 Ubuntu
sudo apt update
sudo apt install -y linux-modules-extra-$(uname -r)

# Load Android modules manually
sudo modprobe binder
sudo modprobe ashmem_linux
sudo modprobe ion
```

### 3.3 Configure WSL2 .wslconfig

```ini
# C:\Users\<username>\.wslconfig

[wsl2]
# Memory allocation
memory=8GB
processors=4

# Swap file
swap=4GB

# WSL2 kernel path
kernel=C:\\Users\\<username>\\wsl2-android-kernel

# Network driver settings
vmIdleTimeout=60000

# Enable localhost forwarding
localhostForwarding=true

# Enable DNS tunneling
dnsTunneling=true

# GPU settings (if using virglrenderer)
gpuSupport=false  # Use CPU rendering for stability
```

### 3.4 WSL2 Docker Configuration

```bash
# In WSL2 Ubuntu
# Install Docker Engine
curl -fsSL https://get.docker.com | sh

# Add current user to docker group
sudo usermod -aG docker $USER

# Configure Docker daemon for ReDroid
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json <<EOF
{
    "hosts": ["unix:///var/run/docker.sock", "tcp://0.0.0.0:2375"],
    "storage-driver": "overlay2",
    "default-ulimits": {
        "nofile": {
            "Name": "nofile",
            "Hard": 65536,
            "Soft": 65536
        }
    }
}
EOF

# Enable ReDroid kernel modules on boot
sudo tee /etc/modules-load.d/android.conf <<EOF
binder
ashmem_linux
ion
EOF

# Restart Docker
sudo systemctl restart docker
```

### 3.5 Verify WSL2 Android Support

```bash
# Check kernel modules
lsmod | grep -E "binder|ashmem|ion"

# Check device nodes
ls -la /dev/binder /dev/ashmem /dev/ion

# Test Docker with ReDroid
docker run -it --rm \
    --privileged \
    --device /dev/binder:/dev/binder \
    ghcr.io/redroid/redroid:14.0.0_google_64only \
    /bin/bash
```

---

## 4. PROJECT FOLDER STRUCTURE

### 4.1 Root Directory Layout

```
VirtualPhonePro/
│
├── src/                              # C++ source files
│   ├── main.cpp                      # Application entry point
│   ├── CMakeLists.txt                 # Main CMake config
│   │
│   ├── core/                         # Core engine
│   │   ├── CMakeLists.txt
│   │   ├── InstanceManager.cpp       # Instance lifecycle
│   │   ├── InstanceManager.h
│   │   ├── DeviceSpoofer.cpp         # Device identity spoofing
│   │   ├── DeviceSpoofer.h
│   │   ├── ProfileEngine.cpp         # Profile generation/loading
│   │   ├── ProfileEngine.h
│   │   ├── NetworkManager.cpp        # Network isolation
│   │   └── NetworkManager.h
│   │
│   ├── docker/                       # Docker integration
│   │   ├── CMakeLists.txt
│   │   ├── DockerEngine.cpp          # Docker API client
│   │   ├── DockerEngine.h
│   │   ├── ContainerConfig.cpp       # Container configuration
│   │   └── ContainerConfig.h
│   │
│   ├── adb/                          # ADB bridge
│   │   ├── CMakeLists.txt
│   │   ├── ADBBridge.cpp             # ADB communication
│   │   ├── ADBBridge.h
│   │   ├── ADBCommand.cpp            # Command execution
│   │   └── ADBCommand.h
│   │
│   ├── ui/                           # Qt6 GUI layer
│   │   ├── CMakeLists.txt
│   │   ├── MainWindow.cpp            # Main window
│   │   ├── MainWindow.h
│   │   ├── MainWindow.ui             # Qt Designer file
│   │   ├── InstanceListWidget.cpp    # Instance list
│   │   ├── InstanceListWidget.h
│   │   ├── ProfileEditorWidget.cpp   # Profile editor
│   │   ├── ProfileEditorWidget.h
│   │   ├── DeviceInfoWidget.cpp      # Device info display
│   │   └── DeviceInfoWidget.h
│   │
│   ├── utils/                        # Utilities
│   │   ├── CMakeLists.txt
│   │   ├── Logger.cpp                # Logging system
│   │   ├── Logger.h
│   │   ├── ConfigManager.cpp          # Configuration
│   │   ├── ConfigManager.h
│   │   ├── ProcessRunner.cpp          # Process management
│   │   └── ProcessRunner.h
│   │
│   └── include/                      # Public headers
│       ├── VirtualPhonePro.h         # Version info
│       ├── Common.h                  # Common types
│       └── Export.h                  # DLL export macros
│
├── include/                          # Third-party headers
│   ├── Qt6/                          # Qt6 framework
│   ├── nlohmann/                     # JSON library
│   └── fmt/                          # Formatting library
│
├── resources/                        # Application resources
│   ├── icons/                        # Application icons
│   ├── profiles/                     # Built-in profiles
│   │   ├── samsung_galaxy_s24.json
│   │   ├── google_pixel_8.json
│   │   └── ...
│   ├── scripts/                      # Helper scripts
│   │   └── inject_device_props.sh
│   └── styles/                       # UI stylesheets
│       └── dark.qss
│
├── docker/                           # Docker files
│   ├── Dockerfile.redroid            # ReDroid image customization
│   ├── docker-compose.yml            # Instance orchestration
│   ├── configs/
│   │   └── redroid-entrypoint.sh     # Container startup
│   └── templates/
│       └── instance-template.json    # Instance config template
│
├── tools/                            # External tools
│   ├── adb/                          # Android Debug Bridge
│   │   ├── adb.exe                   # Windows ADB binary
│   │   ├── AdbWinApi.dll
│   │   ├── AdbWinUsbApi.dll
│   │   └── NOTICE.txt
│   └── scrcpy/                       # Screen mirroring (optional)
│       └── scrcpy.exe
│
├── build/                            # Build output (gitignore)
│
├── tests/                            # Unit tests
│   ├── core/
│   ├── docker/
│   └── adb/
│
├── docs/                             # Documentation
│   ├── ARCHITECTURE.md               # This file
│   ├── API.md                        # API documentation
│   └── DEVELOPMENT.md                 # Dev guide
│
├── CMakeLists.txt                    # Root CMake (first file!)
├── CMakePresets.json                 # CMake presets
├── conanfile.py                      # Conan dependencies
├── vcpkg.json                        # vcpkg manifest (optional)
├── .gitignore
├── LICENSE
└── README.md
```

### 4.2 Key Directory Purposes

| Directory | Purpose | Contains |
|-----------|---------|----------|
| `src/core/` | Instance lifecycle management | InstanceManager, DeviceSpoofer |
| `src/docker/` | Docker API communication | DockerEngine, ContainerConfig |
| `src/adb/` | ADB protocol implementation | ADBBridge, ADBCommand |
| `src/ui/` | Qt6 GUI components | MainWindow, Widgets |
| `src/utils/` | Shared utilities | Logger, Config |
| `resources/profiles/` | Device profiles | JSON profiles |
| `docker/` | ReDroid Docker setup | Dockerfile, scripts |
| `tools/adb/` | ADB binaries | adb.exe, DLLs |

---

## 5. WINDOWS COMPILER STRATEGY

### 5.1 Build System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     BUILD TOOLS                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Option A: MSVC (Visual Studio 2022)                       │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  • Best Windows integration                         │   │
│  │  • Native debugger                                  │   │
│  │  • Pre-built Qt6 binaries                           │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
│  Option B: MinGW-w64 + CMake                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  • GCC-based                                        │   │
│  │  • Portable builds                                  │   │
│  │  • More compilation options                         │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
│  RECOMMENDED: MSVC + vcpkg                                 │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 MSVC + vcpkg Setup

#### Step 1: Install Visual Studio 2022
```
Download: https://visualstudio.microsoft.com/downloads/
Components:
☑ Desktop development with C++
☑ Windows 11 SDK
☑ C++ CMake tools for Windows
```

#### Step 2: Install vcpkg
```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

#### Step 3: Install Dependencies
```powershell
# Qt6 and required libraries
.\vcpkg install qt6 qt6-tools qt6serialbus nlohmann-json fmt
```

### 5.3 Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(VirtualPhonePro VERSION 1.0.0 LANGUAGES CXX)

# ═══════════════════════════════════════════════════════════════
# Standards & Flags
# ═══════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_UNICODE)
    add_definitions(-DUNICODE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    # Character set
    set(CHARACTER_SET "Unicode" CACHE STRING "")
endif()

# ═══════════════════════════════════════════════════════════════
# Output Configuration
# ═══════════════════════════════════════════════════════════════
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ═══════════════════════════════════════════════════════════════
# Subdirectories
# ═══════════════════════════════════════════════════════════════
add_subdirectory(src/core)
add_subdirectory(src/docker)
add_subdirectory(src/adb)
add_subdirectory(src/ui)
add_subdirectory(src/utils)

# ═══════════════════════════════════════════════════════════════
# Executable
# ═══════════════════════════════════════════════════════════════
add_executable(${PROJECT_NAME} 
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Core
    DockerEngine
    ADBBridge
    UI
    Utils
)

# ═══════════════════════════════════════════════════════════════
# Install Rules
# ═══════════════════════════════════════════════════════════════
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY tools/
    DESTINATION tools
)

install(DIRECTORY resources/
    DESTINATION resources
)
```

### 5.4 Subdirectory CMakeLists.txt Example (src/core)

```cmake
add_library(Core STATIC
    InstanceManager.cpp
    DeviceSpoofer.cpp
    ProfileEngine.cpp
    NetworkManager.cpp
)

target_include_directories(Core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(Core PUBLIC
    Qt6::Core
    Qt6::Network
    Utils
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# Windows DLL support
if(WIN32)
    target_compile_definitions(Core PRIVATE
        CORE_EXPORTS
    )
endif()
```

### 5.5 CMake Presets (CMakePresets.json)

```json
{
    "version": 6,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 25,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "msvc-debug",
            "displayName": "MSVC Debug",
            "description": "Visual Studio 2022 Debug",
            "generator": "Visual Studio 17 2022",
            "binaryDir": "${sourceDir}/build/msvc-debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_INSTALL_PREFIX": "${sourceDir}/install/msvc-debug"
            }
        },
        {
            "name": "msvc-release",
            "displayName": "MSVC Release",
            "description": "Visual Studio 2022 Release",
            "generator": "Visual Studio 17 2022",
            "binaryDir": "${sourceDir}/build/msvc-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_INSTALL_PREFIX": "${sourceDir}/install/msvc-release"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "debug",
            "configurePreset": "msvc-debug"
        },
        {
            "name": "release",
            "configurePreset": "msvc-release"
        }
    ]
}
```

### 5.6 Build Commands

```powershell
# Configure
cmake --preset=msvc-release

# Build
cmake --build --preset=release --config Release

# Install
cmake --install build/msvc-release --config Release

# Run
.\build\msvc-release\bin\VirtualPhonePro.exe
```

### 5.7 Dependencies Summary

| Library | Version | Purpose | Source |
|---------|---------|---------|--------|
| **Qt6** | 6.6+ | GUI Framework | vcpkg/MSVC |
| **nlohmann/json** | 3.11+ | JSON parsing | vcpkg |
| **fmt** | 10+ | String formatting | vcpkg |
| **libcurl** | 8+ | HTTP client | vcpkg |
| **spdlog** | 1.12+ | Logging | vcpkg |

---

## 6. NEXT STEPS (Multi-Part Project)

This architecture document covers **Phase 1: Foundation**.

### Project Phases:

| Phase | Title | Deliverables |
|-------|-------|--------------|
| **1** | Architecture & Setup | This document, CMake skeleton |
| **2** | Core Engine | InstanceManager, DeviceSpoofer |
| **3** | Docker Integration | DockerEngine, Container orchestration |
| **4** | ADB Bridge | ADBBridge, Device control |
| **5** | Qt6 GUI | MainWindow, Widgets |
| **6** | Device Profiles | ProfileEngine, Profile DB |
| **7** | Network Isolation | NetworkManager, MAC spoofing |
| **8** | SafetyNet Spoofing | Play Integrity bypass |
| **9** | Sensor Spoofing | GPS, Accelerometer, etc. |
| **10** | Testing & Polish | Integration tests, UI polish |

---

## 7. QUICK REFERENCE COMMANDS

### Development Environment Setup
```powershell
# 1. Clone project
git clone https://github.com/your-repo/VirtualPhonePro.git
cd VirtualPhonePro

# 2. Install dependencies
vcpkg install qt6 qt6-tools nlohmann-json fmt libcurl spdlog

# 3. Configure
cmake --preset=msvc-release

# 4. Build
cmake --build --preset=release

# 5. Run
.\build\msvc-release\bin\VirtualPhonePro.exe
```

### Docker Instance Commands
```bash
# Start single ReDroid instance
docker run -d --rm \
    --name redroid-1 \
    --privileged \
    -p 5555:5555 \
    -p 5900:5900 \
    -e DEVICE_MANUFACTURER=Samsung \
    ghcr.io/redroid/redroid:14.0.0_google_64only

# Connect via ADB
adb connect localhost:5555

# List devices
adb devices -l
```

---

**Document Status:** ✅ Architecture Blueprint Complete  
**Next Phase:** Phase 2 - Core Engine Implementation

---

*VirtualPhonePro - Professional Anti-Detect Android Emulator*  
*For Authorized Testing Only*
