# Windows Native Display Setup Guide

## Overview

This guide explains how to run the Android emulator in a **native desktop window** on Windows using X11 forwarding with VcXsrv or GWSL.

---

## Prerequisites

- Windows 10/11 (64-bit)
- Docker Engine inside WSL2 — click the "⬇ Install" button in the app once
  (provisions WSL2 + custom binder kernel + docker-ce automatically;
  Docker Desktop is NOT required)
- Git Bash or PowerShell

---

## Step 1: Install VcXsrv (Windows X Server)

### Option A: VcXsrv (Recommended)

1. Download VcXsrv from SourceForge:
   ```
   https://sourceforge.net/projects/vcxsrv/
   ```

2. Run the installer and follow the default installation steps

### Option B: GWSL (Windows Subsystem for Linux)

1. Install GWSL from Microsoft Store:
   ```
   https://apps.microsoft.com/store/detail/gwsl/9NL6KD1H6V3T
   ```

---

## Step 2: Configure VcXsrv (XLaunch)

1. **Open XLaunch** from the Start Menu

2. **Select Display Settings**:
   - Choose **"Multiple windows"**
   - Set **Display number**: `0`
   - Click **Next**

3. **Select Client Mode**:
   - Choose **"Start no client"**
   - Click **Next**

4. **Extra Settings**:
   - ✅ **Disable access control** ← IMPORTANT!
   - ✅ **Native opengl** (optional)
   - ✅ **Clipboard integration** (optional)
   - Click **Next**

5. **Save Configuration** (optional):
   - Click **Save configuration** to save settings
   - Save as `redroid.xlaunch` for easy access

6. **Finish**:
   - Click **Finish** to start VcXsrv
   - You should see the VcXsrv icon in the system tray

---

## Step 3: Docker Engine (in-WSL, no Docker Desktop)

Docker runs inside the `redroid-engine` WSL2 distro that the app's
"⬇ Install" button provisions. All `docker` commands route through it:

```powershell
# The app does this automatically for every docker call:
wsl -d redroid-engine -- docker info

# To run the manual docker/compose commands below from Windows, either
# prefix them with `wsl -d redroid-engine -- ` or open a shell in the distro:
wsl -d redroid-engine
```

**Host gateway** (required for X11 forwarding) is already configured in
Docker Compose:
```yaml
extra_hosts:
  - "host.docker.internal:host-gateway"
```

---

## Step 4: Build and Run the Container

### Using Docker Compose

```bash
# Navigate to project directory
cd redroid-cpp

# Build the Docker image
docker build -f docker/Dockerfile -t redroid-emulator .

# Run with docker-compose
docker compose -f docker/docker-compose.yml up -d

# View logs
docker compose -f docker/docker-compose.yml logs -f
```

### Using Docker Run (Alternative)

```bash
docker run -it \
  --privileged \
  -e DISPLAY=host.docker.internal:0 \
  --add-host=host.docker.internal:host-gateway \
  redroid-emulator
```

---

## Step 5: Connect to the Emulator

### Via ADB

```bash
# Connect to the emulator
adb connect localhost:15555

# Verify connection
adb devices

# List available devices
adb -s localhost:15555 shell getprop ro.build.display.id
```

### Via VNC

```bash
# Connect using a VNC viewer (e.g., RealVNC, TightVNC)
# Host: localhost
# Port: 5900
```

---

## Troubleshooting

### Issue: X11 Display Not Working

**Symptoms**: Emulator doesn't display in a window

**Solutions**:

1. **Check if VcXsrv is running**:
   - Look for VcXsrv icon in system tray
   - If not running, start XLaunch again

2. **Verify firewall settings**:
   ```powershell
   # Allow VcXsrv through Windows Firewall
   netsh advfirewall firewall add rule name="VcXsrv" dir=in action=allow program="C:\Program Files\VcXsrv\vcxsrv.exe"
   ```

3. **Check DISPLAY variable**:
   ```bash
   docker exec redroid-emulator echo $DISPLAY
   # Should output: host.docker.internal:0
   ```

### Issue: "Cannot open display"

**Solutions**:

1. Ensure VcXsrv is running with **"Disable access control"** checked
2. Restart the in-WSL Docker Engine: `wsl -d redroid-engine -- sudo systemctl restart docker`
3. Check if Windows Defender is blocking connections

### Issue: Slow Display Performance

**Solutions**:

1. Use GPU acceleration:
   ```bash
   docker run --privileged -e REDROID_GPU=swiftshader_indirect ...
   ```

2. Reduce emulator resolution:
   ```bash
   docker run --privileged -e EMULATOR_FLAGS="-skin 720x1280" ...
   ```

---

## Quick Start Script

Create a file `start-emulator.bat`:

```batch
@echo off
echo Starting VcXsrv...
start "" "C:\Program Files\VcXsrv\vcbxsrv.exe" -multiwindow -ac

echo Waiting 3 seconds...
timeout /t 3

echo Building Docker image...
docker build -f docker/Dockerfile -t redroid-emulator .

echo Starting emulator container...
docker compose -f docker/docker-compose.yml up -d

echo.
echo Emulator is starting...
echo.
echo Connect with: adb connect localhost:15555
echo Or open VNC viewer: localhost:5900
pause
```

---

## Security Notes

> ⚠️ **"Disable access control"** allows any X11 client to connect. Only use this on a trusted network or localhost.

For production environments, consider:
- Using SSH X11 forwarding
- Setting up xhost restrictions
- Using GWSL which has built-in security

---

## Additional Resources

| Resource | Link |
|----------|------|
| VcXsrv Download | https://sourceforge.net/projects/vcxsrv/ |
| GWSL | https://apps.microsoft.com/store/detail/gwsl/9NL6KD1H6V3T |
| In-WSL Docker Engine (docker-ce) | https://docs.docker.com/engine/install/ |
| Binder kernel fork | https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling |

---

**Version:** 1.0.0  
**Last Updated:** 2026-07-14
