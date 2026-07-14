#!/bin/bash
# ==============================================================================
# ReDroidCPP - Android Emulator Entrypoint Script
# ==============================================================================

set -e

echo "[INFO] =========================================="
echo "[INFO] ReDroidCPP Android Emulator Starting..."
echo "[INFO] =========================================="

# ==============================================================================
# Debug: Print environment
# ==============================================================================
echo "[DEBUG] DISPLAY=$DISPLAY"
echo "[DEBUG] ANDROID_HOME=$ANDROID_HOME"
echo "[DEBUG] ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
echo "[DEBUG] ANDROID_AVD_HOME=$ANDROID_AVD_HOME"
echo "[DEBUG] JAVA_HOME=$JAVA_HOME"

# ==============================================================================
# Step 1: Check /dev/kvm and set acceleration flag
# ==============================================================================
if [ -e /dev/kvm ]; then
    echo "[INFO] /dev/kvm found - using hardware acceleration"
    ACCEL_FLAG="-accel kvm"
else
    echo "[WARN] /dev/kvm not found - using software rendering"
    ACCEL_FLAG="-accel off -gpu swiftshader_indirect"
fi

# ==============================================================================
# Step 1b: Verify AVD exists and clean locks
# ==============================================================================
echo "[INFO] Checking AVD..."
ls -la ${ANDROID_AVD_HOME}/ 2>/dev/null || echo "[WARN] AVD directory not found"

if [ -d "${ANDROID_AVD_HOME}/AndroidEmulator.avd" ]; then
    echo "[INFO] AVD found at: ${ANDROID_AVD_HOME}/AndroidEmulator.avd"
    # Clean up any stale lock files
    rm -f ${ANDROID_AVD_HOME}/AndroidEmulator.avd/*.lock 2>/dev/null
    rm -f ${ANDROID_AVD_HOME}/AndroidEmulator.avd/hardware-qemu.ini.lock 2>/dev/null
    rm -rf ${ANDROID_AVD_HOME}/running/* 2>/dev/null
else
    echo "[ERROR] AVD not found! Check ANDROID_AVD_HOME path."
    exit 1
fi

# ==============================================================================
# Step 2: Start ADB server
# ==============================================================================
echo "[INFO] Starting ADB server..."
adb kill-server || true
adb start-server

# ==============================================================================
# Step 3: Start emulator with flags
# ==============================================================================
echo "[INFO] Launching Android Emulator with flags..."
echo "[DEBUG] ACCEL_FLAG=$ACCEL_FLAG"

emulator -avd AndroidEmulator \
    -no-audio \
    -no-boot-anim \
    -no-snapshot \
    -no-window \
    -no-snapstorage \
    -partition-size 2048 \
    -read-only \
    $ACCEL_FLAG &

EMULATOR_PID=$!
echo "[INFO] Emulator started with PID: $EMULATOR_PID"

# ==============================================================================
# Step 4: Wait for full boot
# ==============================================================================
echo "[INFO] Waiting for device to boot..."

# Wait for device
echo "[INFO] Waiting for device..."
adb wait-for-device

# Wait for boot completion
# Note: Without KVM, boot can take 5-10 minutes
BOOT_COMPLETED=""
BOOT_WAIT_COUNT=0
MAX_BOOT_WAIT=600  # 20 minutes max wait time

while [ "$BOOT_COMPLETED" != "1" ]; do
    BOOT_COMPLETED=$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')
    BOOT_WAIT_COUNT=$((BOOT_WAIT_COUNT + 1))
    
    # Show progress every 10 iterations
    if [ $((BOOT_WAIT_COUNT % 10)) -eq 0 ]; then
        echo "[INFO] Boot in progress... (wait count: $BOOT_WAIT_COUNT)"
    fi
    
    if [ $BOOT_WAIT_COUNT -gt $MAX_BOOT_WAIT ]; then
        echo "[WARN] Boot timeout exceeded, but emulator may still be starting..."
        echo "[INFO] Continuing anyway - emulator is running in background"
        break
    fi
    
    sleep 2
done

# ==============================================================================
# Step 5: Print ready message
# ==============================================================================
echo ""
echo "=========================================="
echo "  ✅ Android Emulator is READY"
echo "=========================================="
echo ""
echo "  ADB Port: 5554/5555"
echo "  Connect:  adb connect localhost:5555"
echo "  Device:   adb devices"
echo ""

# ==============================================================================
# Step 6: Keep container alive
# ==============================================================================
echo "[INFO] Container running. Press Ctrl+C to stop."
echo "[INFO] To enter container: docker exec -it android-emulator bash"

# Trap to handle SIGTERM
trap 'echo "[INFO] Received SIGTERM, stopping..."; kill $EMULATOR_PID 2>/dev/null; exit 0' SIGTERM SIGINT

# Wait for emulator process
wait $EMULATOR_PID
