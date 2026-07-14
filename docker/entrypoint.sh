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
# Step 1b: Verify AVD exists
# ==============================================================================
echo "[INFO] Checking AVD..."
ls -la ${ANDROID_AVD_HOME}/ 2>/dev/null || echo "[WARN] AVD directory not found"

if [ -d "${ANDROID_AVD_HOME}/AndroidEmulator.avd" ]; then
    echo "[INFO] AVD found at: ${ANDROID_AVD_HOME}/AndroidEmulator.avd"
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
    -avd-home ${ANDROID_AVD_HOME} \
    -sysdir ${ANDROID_SDK_ROOT}/system-images \
    -system ${ANDROID_SDK_ROOT}/system-images/android-34/google_apis/x86_64/system.img \
    -no-audio \
    -no-boot-anim \
    -no-snapshot \
    -wipe-data \
    -no-window \
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
BOOT_COMPLETED=""
BOOT_WAIT_COUNT=0
while [ "$BOOT_COMPLETED" != "1" ]; do
    BOOT_COMPLETED=$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')
    BOOT_WAIT_COUNT=$((BOOT_WAIT_COUNT + 1))
    echo "[INFO] Boot status: $BOOT_COMPLETED (wait count: $BOOT_WAIT_COUNT)"
    
    if [ $BOOT_WAIT_COUNT -gt 180 ]; then
        echo "[ERROR] Boot timeout! Emulator may have crashed."
        exit 1
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
