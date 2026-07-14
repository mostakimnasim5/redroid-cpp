# RedroidCPP Docker Guide

Docker-based Android Emulator with Device Profile Integration

## 🚀 Quick Start

### Prerequisites

- Docker Engine 20.10+
- Docker Compose v2.0+ (or `docker compose`)
- Linux kernel with KVM support
- 4GB+ RAM available

### Build and Run

```bash
# Build the emulator image
./scripts/manage.sh build

# Start emulator with default settings (Samsung Galaxy S24 Ultra)
./scripts/manage.sh start

# Or use docker-compose directly
docker compose up -d
```

### Connect via ADB

```bash
./scripts/manage.sh connect
# or
adb connect localhost:15555
```

---

## 📋 Management Commands

### Start Emulator

```bash
# Default Samsung device
./scripts/manage.sh start

# Custom device
./scripts/manage.sh start -m Samsung -d "Galaxy S24 Ultra" -a 15

# Google Pixel
./scripts/manage.sh start -m Google -d "Pixel 8 Pro" -a 15

# Named device
./scripts/manage.sh start -n my-device -m OnePlus -d "OnePlus 11" -a 14
```

### Stop Emulator

```bash
./scripts/manage.sh stop
```

### Check Status

```bash
./scripts/manage.sh status
```

### View Logs

```bash
./scripts/manage.sh logs 100
```

---

## 🔧 Device Profiles

### List Available Profiles

```bash
./scripts/manage.sh profiles
```

### Apply a Profile

```bash
./scripts/manage.sh provision my-profile
```

### Generate Profile with CLI

```bash
# Generate from CLI (if built)
./redroid-cli profile -m Samsung

# Export to file
./redroid-cli profile -m Xiaomi > profiles/xiaomi-14.json
```

---

## 🌐 Network Configuration

### Port Mapping

| Service | Port | Description |
|---------|------|-------------|
| ADB | 15555 | Android Debug Bridge |
| VNC | 5900 | VNC (optional) |
| noVNC | 6080 | Web-based VNC |

### GPU Modes

Set via `REDROID_GPU_MODE` environment variable:

- `host` - Use host GPU (best performance, Linux only)
- `swiftshader` - Software rendering
- `auto` - Automatic selection

---

## 📁 Directory Structure

```
redroid-cpp/
├── docker/
│   ├── init.sh           # Device provisioning script
│   └── entrypoint.sh     # Container startup script
├── scripts/
│   └── manage.sh         # Device management CLI
├── profiles/             # Device profile storage
├── data/                 # Persistent data (created at runtime)
├── Dockerfile
└── docker-compose.yml
```

---

## 🔐 Device Identity Generation

The emulator automatically generates:

- **IMEI** - Luhn-validated 15-digit number
- **Android ID** - 16-character hex string
- **Serial Number** - Manufacturer-specific format
- **GSF ID** - 10-digit Google Services Framework ID
- **AAID** - Google Advertising ID (UUID)
- **MAC Addresses** - WiFi, Bluetooth, Ethernet

---

## 🛠️ Development

### Build with CMake

```bash
mkdir build && cd build
cmake ..
make
```

### Build Docker Image Manually

```bash
docker build -t redroid-cpp/emulator:latest .
```

### Run Container Interactively

```bash
docker run -it --rm \
    --privileged \
    --device /dev/kvm:/dev/kvm \
    -e DEVICE_MANUFACTURER=Samsung \
    redroid-cpp/emulator:latest shell
```

---

## ❓ Troubleshooting

### KVM Not Available

```bash
# Check if KVM is available
ls -la /dev/kvm

# Enable KVM in BIOS or use nested virtualization
modprobe kvm-intel  # or kvm-amd
```

### ADB Connection Failed

```bash
# Restart ADB server
adb kill-server
adb start-server
adb connect localhost:15555
```

### Container Not Starting

```bash
# Check logs
docker logs redroid-emulator

# Check container status
docker ps -a | grep redroid
```

---

## 📜 License

Apache License 2.0

---

**Version 3.0.0** - Docker Integration
