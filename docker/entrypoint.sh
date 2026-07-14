#!/bin/bash
# ==============================================================================
# ReDroidCPP - Android Emulator Entrypoint Script
# ==============================================================================

set -e

echo "[INFO] Starting Android Emulator..."

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
# Step 2: Start ADB server
# ==============================================================================
echo "[INFO] Starting ADB server..."
adb start-server

# ==============================================================================
# Step 3: Start emulator with flags
# ==============================================================================
echo "[INFO] Launching Android Emulator..."
emulator -avd AndroidEmulator \
    -no-audio \
    -no-boot-anim \
    -no-snapshot \
    -wipe-data \
    $ACCEL_FLAG &

EMULATOR_PID=$!

# ==============================================================================
# Step 4: Wait for full boot
# ==============================================================================
echo "[INFO] Waiting for device to boot..."

# Wait for device
adb wait-for-device

# Wait for boot completion
BOOT_COMPLETED=""
while [ "$BOOT_COMPLETED" != "1" ]; do
    BOOT_COMPLETED=$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')
    echo "[INFO] Boot status: $BOOT_COMPLETED"
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
echo ""

# ==============================================================================
# Step 6: Keep container alive
# ==============================================================================
echo "[INFO] Container running. Press Ctrl+C to stop."

# Wait for emulator process
wait $EMULATOR_PID
