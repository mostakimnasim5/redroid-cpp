# RedroidCPP - Professional Android Emulator Manager v3.0.0

<div align="center">

![Version](https://img.shields.io/badge/Version-3.0.0-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-green)
![License](https://img.shields.io/badge/License-Apache--2.0-orange)

**Professional-grade C++ application for managing virtual Android devices with complete device profile generation.**

*Built for Banking App Testing, Security Testing, and Anti-Detection Verification*

</div>

---

## 🎯 Complete Device Profile Features

### 📱 Device IDs
| Property | Description | Format |
|----------|-------------|--------|
| IMEI | International Mobile Equipment Identity | 15 digits (Luhn validated) |
| IMEI2 | Secondary IMEI (Dual SIM) | 15 digits |
| Serial | Device Serial Number | Manufacturer-specific |
| Android ID | Android System Identifier | 16 hex chars |
| GSF ID | Google Services Framework ID | 10 digits |
| AAID | Google Advertising ID | UUID format |

### 🔗 MAC Addresses
| Property | Description |
|---------|-------------|
| WiFi MAC | WiFi interface MAC address |
| Bluetooth MAC | Bluetooth interface MAC address |
| Ethernet MAC | Ethernet interface MAC address |

### 📋 SIM Configuration
| Property | Description |
|---------|-------------|
| ICCID | Integrated Circuit Card Identifier (20 digits) |
| IMSI | International Mobile Subscriber Identity (15 digits) |
| Carrier | Network operator name |
| MCC/MNC | Mobile Country/Network Codes |

### ⚙️ Hardware
| Component | Properties |
|----------|-----------|
| **CPU** | Architecture, cores, frequency, BogoMIPS |
| **GPU** | Renderer, vendor, OpenGL ES, Vulkan |
| **Memory** | Total RAM, heap sizes |
| **Battery** | Capacity, temperature, voltage |

### 🔨 Build Information
| Property | Description |
|---------|-------------|
| Fingerprint | Full Android build fingerprint |
| Bootloader | Bootloader version |
| Build ID | Build identifier (UP1A.xxxxxxx) |
| Security Patch | Security patch level date |

### 🔒 Verified Boot
| Property | Description |
|---------|-------------|
| State | Boot state (green/yellow/orange/red) |
| Locked | Device lock status |
| VBMeta Digest | Verified boot metadata digest |

### 🌐 Network Configuration
| Property | Description |
|---------|-------------|
| Hostname | Device hostname |
| IP Address | DHCP-assigned IP |
| DNS | Primary and secondary DNS |
| TCP Buffers | Network buffer configurations |

### 📍 GPS/Location
| Property | Description |
|---------|-------------|
| Latitude | GPS latitude coordinate |
| Longitude | GPS longitude coordinate |
| Altitude | Altitude in meters |
| Accuracy | Location accuracy in meters |
| Constellations | GPS, GLONASS, BeiDou, Galileo, QZSS |

### 📊 Sensors
| Sensor | Properties |
|--------|-----------|
| Accelerometer | Name, vendor, range, resolution |
| Gyroscope | Name, vendor, range, resolution |
| Magnetic Field | Name, vendor, range, resolution |
| Barometer | Name, vendor, pressure range |
| Light | Name, vendor, lux range |
| Proximity | Name, vendor, distance range |

### 🛡️ Security
| Property | Description |
|---------|-------------|
| SELinux | SELinux status (Enforcing) |
| Keymaster | Keymaster version |
| Strongbox | Strongbox availability |
| KNOX | Samsung KNOX ID and version |
| Hardware Attestation | Attestation status |

---

## 📦 Supported Manufacturers

| Manufacturer | Models |
|-------------|--------|
| Samsung | Galaxy S24/S23/Z Fold/A-series |
| Google | Pixel 8/7/6/Fold |
| Xiaomi | Xiaomi 14/13, Redmi, POCO |
| OnePlus | OnePlus 12/11/10T |
| OPPO | Find X7/X6, Reno |
| Vivo | X100/X90, V30 |
| Huawei | Mate 60, P60, Mate X5 |
| Motorola | Edge 50/40/30 |
| Sony | Xperia 1/5/10 |
| ASUS | ROG Phone, Zenfone |
| Realme | GT 5 Pro, C67 |
| Nokia | G42, X30 |

---

## 🚀 Installation

```bash
# Clone the repository
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp

# Compile
g++ -std=c++17 -O2 -Wall -I include -o redroid-cli src/main.cpp src/Core/DeviceProfile.cpp

# Run
./redroid-cli help
```

---

## 📖 Usage

```bash
# Create device
./redroid-cli create -m Samsung -a 14

# Create Google device
./redroid-cli create -m Google

# List devices
./redroid-cli list

# Show device info
./redroid-cli info <device-id>

# Show all properties
./redroid-cli profile -m Xiaomi

# Validate IMEI
./redroid-cli validate 358751090123456

# System status
./redroid-cli status

# Supported manufacturers
./redroid-cli manufacturers
```

---

## 📁 Project Structure

```
redroid-cpp/
├── src/
│   ├── main.cpp                 # CLI application entry point
│   └── Core/
│       └── DeviceProfile.cpp     # Device profile implementation
├── include/
│   ├── Core/
│   │   ├── DeviceProfile.h       # Complete device profile header
│   │   └── DeviceManager.h       # Device manager
│   └── Data/
│       └── TACDatabase.h         # TAC database for IMEI
├── docker/
│   ├── Dockerfile                # Multi-stage Docker build
│   ├── docker-compose.yml        # Container orchestration
│   └── bin/
│       ├── entrypoint.sh         # Container startup script
│       └── init.sh               # Device provisioning script
├── tools/
│   └── manage.sh                 # Device management CLI
├── profiles/                     # Device profile storage
├── data/                         # Runtime data
├── CMakeLists.txt
├── LICENSE
├── README.md
└── CONTRIBUTING.md
```

---

## ⚖️ Disclaimer

> **This software is for authorized testing purposes only.**
> - Banking App Testing
> - Security Research
> - Anti-Detection Verification
> - QA Testing

---

## 📄 License

Licensed under the Apache License 2.0.

---

**Version 3.0.0** - Complete Device Profile Implementation
