# VirtualPhonePro - Advanced Features Documentation

## Overview

This document describes the advanced features implemented in VirtualPhonePro for enhanced emulator management and anti-detection capabilities.

---

## 1. SafetyNet/Play Integrity Spoofing

**Header:** `include/VirtualPhonePro/SafetyNetSpoofer.h`  
**Implementation:** `src/ReDroidController/SafetyNetSpoofer.cpp`

### Features

| Method | Description |
|--------|-------------|
| `spoofSafetyNetResponse()` | Complete SafetyNet attestation response spoofing |
| `spoofPlayIntegrity()` | Play Integrity API response spoofing |
| `spoofDeviceIntegrity()` | Device integrity flags spoofing |
| `spoofCtsProfileMatch()` | CTS profile match configuration |
| `installSpoofedGmsCore()` | Spoofed Google Mobile Services installation |

### Usage

```cpp
// Initialize SafetyNet spoofer
SafetyNetSpoofer& sniffer = SafetyNetSpoofer::instance();

// Spoof SafetyNet for banking apps
sniffer.spoofSafetyNetResponse("instance-1");

// Verify spoofing
QJsonObject status = sniffer.getIntegrityStatus("instance-1");
```

### Properties Set

- `ro.boot.verifiedbootstate=green`
- `ro.boot.flash.locked=1`
- `ctsProfileMatch=true`
- `basicIntegrity=true`
- `com.google.android.gms.safetynet.*`

---

## 2. Multi-Instance Management

**Header:** `include/VirtualPhonePro/MultiInstanceManager.h`  
**Implementation:** `src/ReDroidController/MultiInstanceManager.cpp`

### Features

| Feature | Description |
|---------|-------------|
| `deployBatch()` | Deploy multiple instances simultaneously |
| `startBatch()` | Start multiple instances |
| `stopBatch()` | Stop multiple instances |
| `createGroup()` | Create instance groups |
| `getResourceSummary()` | System resource monitoring |

### Usage

```cpp
MultiInstanceManager& manager = MultiInstanceManager::instance();

// Configure batch deployment
InstanceDeployConfig config;
config.count = 10;
config.maxConcurrent = 5;
config.delayBetween = 2000;
config.baseProfile = DeviceProfile::createSamsungS24Ultra();
config.assignUniqueIdentity = true;

// Deploy batch
BatchStatus status = manager.deployBatch(config);

// Create group
manager.createGroup("test-team", {"instance-001", "instance-002", "instance-003"});
manager.startGroup("test-team");
```

---

## 4. App Cloning & Multi-Account

**Header:** `include/VirtualPhonePro/AppCloner.h`

### Features

| Method | Description |
|--------|-------------|
| `cloneApp()` | Clone app with new package name |
| `installAsPackage()` | Install APK as specific package |
| `createWorkProfile()` | Create Android work profile |
| `clearAppData()` | Reset app data |

### Usage

```cpp
AppCloner& cloner = AppCloner::instance();

// Clone WhatsApp for second account
cloner.cloneApp("instance-1", 
               "com.whatsapp",
               "com.whatsapp.clone1",
               "WhatsApp Clone");

// List cloned apps
QMap<QString, QString> clones = cloner.listClonedApps("instance-1");
```

---

## 5. Automated Testing Framework

**Header:** `include/VirtualPhonePro/TestingFramework.h`

### Test Actions

| Action | Parameters |
|--------|------------|
| `tap` | x, y coordinates |
| `swipe` | x1, y1, x2, y2, duration |
| `input` | text string |
| `launch` | package name |
| `press` | key code |
| `assert` | condition, expected |

### Usage

```cpp
TestingFramework& testing = TestingFramework::instance();

// Load test suite
TestSuite suite = testing.loadSuite(":/tests/login-test.json");

// Execute on instance
TestReport report = testing.executeSuite("instance-1", suite);

// Results
qDebug() << "Passed:" << report.passed;
qDebug() << "Failed:" << report.failed;
```

### Test Suite JSON Format

```json
{
    "id": "login-test",
    "name": "Banking Login Test",
    "testCases": [
        {
            "id": "tc-001",
            "action": "launch",
            "params": {"package": "com.bank.app"},
            "timeout": 5000
        },
        {
            "id": "tc-002",
            "action": "tap",
            "params": {"x": 540, "y": 1200},
            "timeout": 2000
        }
    ]
}
```

---

## 6. Sensor Simulation

**Header:** `include/VirtualPhonePro/SensorSimulator.h`

### Sensor Types

| Sensor | Methods |
|--------|---------|
| **GPS** | `setGPSLocation()`, `startGPSRoute()`, `startGPSCircle()` |
| **Accelerometer** | `setAccelerometer()`, `startShake()`, `startTilt()` |
| **Gyroscope** | `setGyroscope()`, `startRotation()` |
| **Magnetic Field** | `setMagneticField()`, `startCompass()` |

### Usage

```cpp
SensorSimulator& sensors = SensorSimulator::instance();

// Set GPS location (San Francisco)
sensors.setGPSLocation("instance-1", 37.7749, -122.4194, 10.0, 5.0);

// Simulate walking route
QList<GPSLocation> route = {
    {37.7749, -122.4194},
    {37.7751, -122.4190},
    {37.7755, -122.4185}
};
sensors.startGPSRoute("instance-1", route, 3000);

// Shake detection simulation
sensors.startShake("instance-1", 2.0f);

// Stop all sensors
sensors.stopAllSensors("instance-1");
```

---

## 8. REST API Server

**Header:** `include/VirtualPhonePro/APIServer.h`

### Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/instances` | List all instances |
| POST | `/api/v1/instances` | Create instance |
| GET | `/api/v1/instances/:id` | Get instance details |
| POST | `/api/v1/instances/:id/start` | Start instance |
| POST | `/api/v1/instances/:id/stop` | Stop instance |
| DELETE | `/api/v1/instances/:id` | Delete instance |
| POST | `/api/v1/batch/start` | Batch start |
| POST | `/api/v1/batch/stop` | Batch stop |

### Usage

```cpp
APIServer& api = APIServer::instance();
api.setApiKey("your-secret-key");
api.start(8080);

// Example curl:
// curl -H "X-API-Key: your-secret-key" \
//      http://localhost:8080/api/v1/instances
```

---

## 9. Webhook Integration

**Header:** `include/VirtualPhonePro/WebhookManager.h`

### Events

| Event | Trigger |
|-------|---------|
| `InstanceStarted` | Instance starts |
| `InstanceStopped` | Instance stops |
| `InstanceError` | Error occurs |
| `MemoryWarning` | Low memory |
| `SafetyNetCheck` | Integrity check |

### Usage

```cpp
WebhookManager& webhooks = WebhookManager::instance();

// Configure webhook
WebhookConfig webhook;
webhook.id = "slack-notify";
webhook.url = "https://hooks.slack.com/...";
webhook.events = {"InstanceStarted", "InstanceStopped"};
webhook.retryCount = 3;

webhooks.addWebhook(webhook);

// Event triggers automatically on instance changes
```

---

## 10. Magisk & Custom ROM Patching

**Header:** `include/VirtualPhonePro/MagiskPatcher.h`

### Features

| Feature | Description |
|---------|-------------|
| `installMagisk()` | Install Magisk manager |
| `installModule()` | Install Magisk modules |
| `installIntegrityBypass()` | Install SafetyNet bypass |
| `patchZygisk()` | Configure Zygisk |

### Usage

```cpp
MagiskPatcher& patcher = MagiskPatcher::instance();

// Install Magisk
patcher.installMagisk("instance-1", "/path/to/magisk.zip");

// Install Play Integrity bypass module
patcher.installIntegrityBypass("instance-1");

// List modules
QList<MagiskModule> modules = patcher.listModules("instance-1");
```

---

## Feature Matrix

| Feature | Banking Apps | Security Testing | Multi-Account |
|---------|--------------|------------------|---------------|
| SafetyNet Spoofing | ✅ | ✅ | - |
| Multi-Instance | ✅ | ✅ | ✅ |
| App Cloning | ✅ | ✅ | ✅ |
| Testing Framework | ✅ | ✅ | ✅ |
| Sensor Simulation | ✅ | ✅ | - |
| REST API | ✅ | ✅ | ✅ |
| Webhooks | ✅ | ✅ | ✅ |
| Magisk Patching | ✅ | ✅ | - |

---

## Security Notice

⚠️ **These features are for authorized testing purposes only.**  
Do not use for unauthorized access to applications or services.

---

*VirtualPhonePro Advanced Features*
