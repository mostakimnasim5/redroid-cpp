#!/bin/sh
# ==============================================================================
# VirtualPhonePro — ReDroid Container Entrypoint
#
# CRITICAL RULE: exec /init must happen as fast as possible.
# Android's property daemon (property_service) starts INSIDE /init.
# setprop / getprop talk to that daemon via /dev/socket/property_service.
# Calling setprop or getprop BEFORE exec /init = socket does not exist
# → every call silently fails or blocks indefinitely.
#
# Pre-boot allowed:  mount checks, binder checks, logging, env var reads.
# Post-boot (setprop, getprop, ADB, stop/start service): handled by the
# C++ host application after it detects sys.boot_completed = 1 via ADB.
# ==============================================================================

set -e

log() {
    echo "[VPP] $(date '+%H:%M:%S') $*"
}

# ==============================================================================
# 1. Binder device check — required for Android IPC
# ==============================================================================
log "Checking binder..."
if [ -e /dev/binder ]; then
    log "  /dev/binder OK"
elif [ -e /dev/binderfs/binder ]; then
    log "  /dev/binderfs/binder OK (binderfs)"
else
    log "  WARNING: no binder device — WSL2 custom kernel required"
fi

# ==============================================================================
# 2. KVM check — optional, improves performance
# ==============================================================================
if [ -e /dev/kvm ]; then
    log "  /dev/kvm OK (hardware acceleration)"
else
    log "  /dev/kvm absent — swiftshader/software rendering"
fi

# ==============================================================================
# 3. Log active ReDroid env-var config
#    ReDroid reads these directly from the process environment during /init.
#    No setprop call needed or possible here.
# ==============================================================================
log "Config:"
log "  GPU mode  : ${REDROID_GPU_MODE:-swiftshader_indirect}"
log "  Mem limit : ${REDROID_MEM_SIZE:-1536M}"
log "  ADB port  : ${REDROID_ADBD_PORT:-5555}"
log "  Profile   : ${DEVICE_PROFILE_ID:-(none)}"

# ==============================================================================
# 4. Optional: load a pre-boot device-profile env script
#    The script may only export additional REDROID_* env vars — no setprop.
# ==============================================================================
PROFILE_DIR="/opt/vpp/config"
if [ -n "$DEVICE_PROFILE_ID" ] && [ -f "$PROFILE_DIR/${DEVICE_PROFILE_ID}.sh" ]; then
    log "Loading profile env: $DEVICE_PROFILE_ID"
    # shellcheck disable=SC1090
    . "$PROFILE_DIR/${DEVICE_PROFILE_ID}.sh"
fi

# ==============================================================================
# 5. Hand off to Android init — THIS is where Android boots.
#    Everything after this line never executes.
#    Post-boot config (setprop, getprop, ADB commands, adbd restart) is
#    performed by ReDroidController.cpp after sys.boot_completed = 1.
# ==============================================================================
log "Handing off to Android init (exec /init)..."
exec /init
