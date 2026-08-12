#!/bin/sh
# ==============================================================================
# ReDroidCPP - ReDroid Container Entrypoint
#
# Rule: exec /init MUST happen as fast as possible.
# Android cannot boot until exec /init is called.
# DO NOT wait for boot here — boot hasn't started yet!
#
# Post-boot configuration (setprop, ADB commands) is handled
# by the C++ application after sys.boot_completed is detected.
# ==============================================================================

set -e

log() {
    echo "[ReDroidCPP] $(date '+%H:%M:%S') $1"
}

# ==============================================================================
# 1. Quick device checks (non-blocking, no waits)
# ==============================================================================
log "Checking binder devices..."
if [ -e /dev/binder ]; then
    log "  /dev/binder OK"
else
    log "  WARNING: /dev/binder not found - WSL2 custom kernel required"
fi

# ==============================================================================
# 2. Apply environment-based configuration
# If REDROID_MEM_SIZE or other env vars are set, ReDroid reads them directly
# ==============================================================================
log "Config: GPU=${REDROID_GPU_MODE:-swiftshader} MEM=${REDROID_MEM_SIZE:-1536M}"

# ==============================================================================
# 3. Hand off to ReDroid Android init — THIS STARTS ANDROID
#
# exec /init replaces this shell process with Android's init system.
# Everything after this line never runs.
# Post-boot tasks must be done via ADB from the host/C++ app.
# ==============================================================================
log "Starting Android (exec /init)..."
exec /init
