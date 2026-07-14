# VirtualPhonePro - Phase 4: ADB Bridge

## Overview

Phase 4 implements the Android Debug Bridge (ADB) integration layer for controlling ReDroid instances.

## ADB Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    VirtualPhonePro.exe                          │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                     ADB Bridge Layer                      │ │
│  │                                                           │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │ │
│  │  │ ADB Connect │  │ ADB Shell   │  │ ADB Push/Pull│        │ │
│  │  │ Manager     │  │ Executor    │  │ File Transfer│        │ │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘        │ │
│  │         │                │                │               │ │
│  │  ┌──────┴────────────────┴────────────────┴──────┐        │ │
│  │  │              ADB Protocol Handler              │        │ │
│  │  └───────────────────────┬───────────────────────┘        │ │
│  └───────────────────────────┼────────────────────────────────┘ │
│                              │                                  │
│  ┌───────────────────────────┼────────────────────────────────┐ │
│  │                           │                                │ │
│  │  ┌────────────────────────┴───────────────────────┐      │ │
│  │  │              adb.exe (Windows)                  │      │ │
│  │  │  C:\Users\...\platform-tools\adb.exe           │      │ │
│  │  └────────────────────────┬───────────────────────┘      │ │
│  └───────────────────────────┼────────────────────────────────┘ │
└───────────────────────────────┼────────────────────────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
  ┌──────────┐            ┌──────────┐            ┌──────────┐
  │ ReDroid  │            │ ReDroid  │            │ ReDroid  │
  │ :5555    │            │ :5557    │            │ :5559    │
  └──────────┘            └──────────┘            └──────────┘
```

## Components

### 4.1 ADBBridge

```cpp
// src/adb/ADBBridge.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <future>

namespace VirtualPhonePro {

class ADBBridge {
public:
    static ADBBridge& getInstance();
    
    // Connection Management
    bool connect(const std::string& instanceId, 
                 const std::string& address,
                 int port);
    bool disconnect(const std::string& instanceId);
    bool isConnected(const std::string& instanceId);
    
    // Device List
    struct DeviceInfo {
        std::string serial;
        std::string product;
        std::string model;
        std::string device;
        std::string state;  // "device", "offline", "unauthorized"
    };
    
    std::vector<DeviceInfo> listDevices();
    std::optional<DeviceInfo> getDeviceInfo(const std::string& instanceId);
    
    // Shell Commands
    bool executeShell(const std::string& instanceId,
                      const std::string& command);
    
    std::string executeShellSync(const std::string& instanceId,
                                  const std::string& command,
                                  int timeoutMs = 30000);
    
    std::future<std::string> executeShellAsync(
        const std::string& instanceId,
        const std::string& command);
    
    // Property Management
    bool setProp(const std::string& instanceId,
                 const std::string& property,
                 const std::string& value);
    
    std::optional<std::string> getProp(const std::string& instanceId,
                                        const std::string& property);
    
    std::map<std::string, std::string> getAllProps(const std::string& instanceId);
    
    // File Operations
    bool pushFile(const std::string& instanceId,
                  const std::string& localPath,
                  const std::string& remotePath);
    
    bool pullFile(const std::string& instanceId,
                  const std::string& remotePath,
                  const std::string& localPath);
    
    bool installAPK(const std::string& instanceId,
                   const std::string& apkPath,
                   std::function<void(float)> progress = nullptr);
    
    // Screen Operations
    bool screenshot(const std::string& instanceId,
                    const std::string& outputPath);
    
    std::vector<uint8_t> screenshotRaw(const std::string& instanceId);
    
    // Input Events
    bool tap(const std::string& instanceId, int x, int y);
    bool swipe(const std::string& instanceId, int x1, int y1, int x2, int y2, int durationMs);
    bool inputText(const std::string& instanceId, const std::string& text);
    bool pressKey(const std::string& instanceId, int keyCode);
    
    // Root & SELinux
    bool enableRoot(const std::string& instanceId);
    bool disableRoot(const std::string& instanceId);
    bool setSELinuxMode(const std::string& instanceId, const std::string& mode);  // "enforcing" or "permissive"
    
    // Backup & Restore
    bool backup(const std::string& instanceId,
                const std::string& outputPath,
                const std::string& packages = "");
    
    bool restore(const std::string& instanceId,
                 const std::string& backupPath);
    
    // Reboot
    bool reboot(const std::string& instanceId,
                const std::string& mode = "");  // "", "recovery", "bootloader"
    
private:
    ADBBridge();
    
    std::string executeCommand(const std::string& cmd,
                               int timeoutMs = 30000,
                               bool captureOutput = true);
    
    std::string executeCommandForDevice(const std::string& instanceId,
                                         const std::string& args,
                                         int timeoutMs = 30000);
    
    std::string m_adbPath;
    std::map<std::string, std::string> m_connections;  // instanceId -> serial
};
```

### 4.2 ADB Commands Reference

```cpp
// Common ADB commands used by the bridge

namespace ADBCommands {
    // Connection
    inline std::string connect(const std::string& ip, int port) {
        return fmt::format("connect {}:{}", ip, port);
    }
    
    inline std::string disconnect(const std::string& ip, int port) {
        return fmt::format("disconnect {}:{}", ip, port);
    }
    
    inline std::string devices() {
        return "devices -l";
    }
    
    // Shell
    inline std::string shell(const std::string& instanceId, 
                             const std::string& command) {
        return fmt::format("-s {} shell \"{}\"", instanceId, command);
    }
    
    // Properties
    inline std::string getProp(const std::string& instanceId,
                               const std::string& prop) {
        return fmt::format("-s {} shell getprop {}", instanceId, prop);
    }
    
    inline std::string setProp(const std::string& instanceId,
                               const std::string& prop,
                               const std::string& value) {
        return fmt::format("-s {} shell setprop {} \"{}\"", instanceId, prop, value);
    }
    
    // Files
    inline std::string push(const std::string& instanceId,
                           const std::string& local,
                           const std::string& remote) {
        return fmt::format("-s {} push \"{}\" \"{}\"", instanceId, local, remote);
    }
    
    inline std::string pull(const std::string& instanceId,
                           const std::string& remote,
                           const std::string& local) {
        return fmt::format("-s {} pull \"{}\" \"{}\"", instanceId, remote, local);
    }
    
    // Input
    inline std::string tap(const std::string& instanceId, int x, int y) {
        return fmt::format("-s {} shell input tap {} {}", instanceId, x, y);
    }
    
    inline std::string swipe(const std::string& instanceId,
                            int x1, int y1, int x2, int y2, int duration) {
        return fmt::format("-s {} shell input swipe {} {} {} {} {}",
                          instanceId, x1, y1, x2, y2, duration);
    }
    
    inline std::string inputText(const std::string& instanceId,
                                const std::string& text) {
        return fmt::format("-s {} shell input text \"{}\"", instanceId, text);
    }
    
    inline std::string keyEvent(const std::string& instanceId, int keyCode) {
        return fmt::format("-s {} shell input keyevent {}", instanceId, keyCode);
    }
    
    // System
    inline std::string root(const std::string& instanceId) {
        return fmt::format("-s {} root", instanceId);
    }
    
    inline std::string remount(const std::string& instanceId) {
        return fmt::format("-s {} remount", instanceId);
    }
    
    inline std::string reboot(const std::string& instanceId,
                             const std::string& mode = "") {
        if (mode.empty()) {
            return fmt::format("-s {} reboot", instanceId);
        }
        return fmt::format("-s {} reboot {}", instanceId, mode);
    }
}
```

### 4.3 Key Android Properties

```cpp
// Critical properties for device spoofing

namespace AndroidProps {
    // Device Identity
    const char* RO_PRODUCT_BRAND = "ro.product.brand";           // samsung
    const char* RO_PRODUCT_MANUFACTURER = "ro.product.manufacturer"; // Samsung Electronics
    const char* RO_PRODUCT_MODEL = "ro.product.model";           // SM-S928B
    const char* RO_PRODUCT_DEVICE = "ro.product.device";        // dm3q
    const char* RO_PRODUCT_NAME = "ro.product.name";            // dm3q
    
    // Build
    const char* RO_BUILD_FINGERPRINT = "ro.build.fingerprint";
    const char* RO_BOOTLOADER = "ro.bootloader";
    const char* RO_BUILD_ID = "ro.build.id";
    const char* RO_BUILD_VERSION_SECURITY_PATCH = "ro.build.version.security_patch";
    
    // Identity
    const char* RO_GSM_DEVICE_IMEI = "ro.gsm.device.imei";
    const char* RO_GSM_SIM_IMSI = "ro.gsm.sim.imsi";
    const char* RO_SERIALNO = "ro.serialno";
    
    // Network
    const char* NET_HOSTNAME = "net.hostname";
    const char* NET.eth0_MAC = "net.eth0.mac";
    const char* NET_WIFI_MAC = "net.wifi.mac";
    
    // Hardware
    const char* RO_HARDWARE = "ro.hardware";
    const char* RO_BOARD = "ro.board.platform";
    const char* RO_ARCH = "ro.product.cpu.abi";
    
    // Google
    const char* RO_COM_GOOGLE_GMS = "ro.com.google.gms";
    const char* PERSIST_GSF_ID = "persist.gservices.gsfid";
}

// Key codes for input events
namespace KeyCodes {
    constexpr int KEYCODE_HOME = 3;
    constexpr int KEYCODE_BACK = 4;
    constexpr int KEYCODE_MENU = 82;
    constexpr int KEYCODE_POWER = 26;
    constexpr int KEYCODE_VOLUME_UP = 24;
    constexpr int KEYCODE_VOLUME_DOWN = 25;
    constexpr int KEYCODE_ENTER = 66;
    constexpr int KEYCODE_DELETE = 67;
}
```

## Implementation Tasks

### Task 4.1: ADB Connection Manager
- [ ] Implement connection pooling
- [ ] Implement auto-reconnect
- [ ] Implement connection health check
- [ ] Handle multiple concurrent connections

### Task 4.2: Shell Executor
- [ ] Implement synchronous execution
- [ ] Implement asynchronous execution
- [ ] Implement timeout handling
- [ ] Implement output capture

### Task 4.3: Property Manager
- [ ] Implement property getter/setter
- [ ] Implement bulk property operations
- [ ] Implement property verification

### Task 4.4: File Transfer
- [ ] Implement push/pull operations
- [ ] Implement progress callbacks
- [ ] Implement APK installation

---

## Next Phase

[Phase 5: Qt6 GUI](./PHASE5_QT6_GUI.md)

---

*VirtualPhonePro - Phase 4*
