#!/bin/bash
# ==============================================================================
# ReDroidCPP - Pure ReDroid Container Entrypoint
# 
# Lightweight entrypoint for ReDroid (no QEMU/AVD dependencies)
# Boot time target: <3 seconds
# RAM usage: ~1GB-1.5GB per instance
# ==============================================================================

set -e

# Colors
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration (can be overridden via environment)
: "${REDROID_MEM_SIZE:=1536M}"       # Default: 1.5GB
: "${REDROID_GPU_MODE:=swiftshader}" # Default: software rendering
: "${REDROID_VGPU:=auto}"            # Default: auto GPU mode
: "${REDROID_RAM_SIZE:=1536}"       # Default: 1536MB for container memory
: "${ADB_PORT:=5555}"

log() {
    echo -e "${GREEN}[ReDroidCPP]${NC} $(date '+%H:%M:%S') - $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $(date '+%H:%M:%S') - $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%H:%M:%S') - $1"
}

# ==============================================================================
# CHECK PREREQUISITES
# ==============================================================================
check_prerequisites() {
    log "Checking prerequisites..."
    
    # Check KVM access
    if [ -c /dev/kvm ]; then
        log "KVM: /dev/kvm available ✓"
    else
        warn "KVM: /dev/kvm not available - using software rendering"
    fi
    
    # Check binder devices
    if [ -e /dev/binder ]; then
        log "Binder: /dev/binder available ✓"
    else
        warn "Binder: /dev/binder not available"
    fi
    
    # Check other required devices
    for device in /dev/binderfs/binder /dev/vndbinder /dev/hwbinder; do
        if [ -c "$device" ] || [ -e "$device" ]; then
            log "$(basename $device): available ✓"
        fi
    done
}

# ==============================================================================
# APPLY DEVICE SPOOFING (from mounted configs)
# ==============================================================================
apply_spoofing() {
    log "Applying device spoofing profiles..."
    
    # Mounted from docker/configs or C++ application
    CONFIG_DIR="/opt/redroid/configs"
    
    if [ -d "$CONFIG_DIR" ]; then
        # Apply build.prop spoofing
        if [ -f "$CONFIG_DIR/build.prop" ]; then
            log "Applying build.prop spoofing..."
            # In ReDroid, we apply via setprop commands
            grep "^ro\." "$CONFIG_DIR/build.prop" 2>/dev/null | while read line; do
                key=$(echo "$line" | cut -d'=' -f1)
                value=$(echo "$line" | cut -d'=' -f2-)
                setprop "$key" "$value" 2>/dev/null || true
            done
            log "build.prop applied ✓"
        fi
        
        # Apply spoof properties script
        if [ -f "$CONFIG_DIR/spoof-properties.sh" ]; then
            log "Running spoof-properties.sh..."
            chmod +x "$CONFIG_DIR/spoof-properties.sh"
            "$CONFIG_DIR/spoof-properties.sh" 2>/dev/null || true
        fi
        
        # Apply identity script
        if [ -f "$CONFIG_DIR/apply-identity.sh" ]; then
            log "Applying device identity..."
            chmod +x "$CONFIG_DIR/apply-identity.sh"
            "$CONFIG_DIR/apply-identity.sh" 2>/dev/null || true
        fi
    else
        log "No custom configs mounted, using defaults"
    fi
}

# ==============================================================================
# CONFIGURE NETWORK
# ==============================================================================
configure_network() {
    log "Configuring network..."
    
    # Set hostname
    hostname redroid-emulator
    echo "redroid-emulator" > /etc/hostname 2>/dev/null || true
    
    # Enable ADB over TCP
    setprop service.adb.tcp.port "${ADB_PORT}" 2>/dev/null || true
    
    # Configure network properties
    setprop net.hostname "redroid-emulator" 2>/dev/null || true
    
    log "Network configured ✓"
}

# ==============================================================================
# WAIT FOR BOOT
# ==============================================================================
wait_for_boot() {
    log "Waiting for Android boot completion..."
    
    local timeout=120
    local count=0
    
    while [ $count -lt $timeout ]; do
        if getprop sys.boot_completed 2>/dev/null | grep -q "1"; then
            log "Android boot completed! ✓"
            return 0
        fi
        # Progress indicator every 10 seconds
        if [ $((count % 10)) -eq 0 ] && [ $count -gt 0 ]; then
            echo -n " [$count/$timeout]"
        fi
        echo -n "."
        sleep 1
        count=$((count + 1))
    done
    
    echo ""
    warn "Boot timeout reached, continuing anyway..."
    return 1
}

# ==============================================================================
# START ADB SERVER
# ==============================================================================
start_adb() {
    log "Starting ADB server on port ${ADB_PORT}..."
    
    # Restart adbd with TCP support
    stop adbd 2>/dev/null || true
    setprop service.adb.tcp.port "${ADB_PORT}" 2>/dev/null || true
    start adbd 2>/dev/null || true
    
    log "ADB server started ✓"
}

# ==============================================================================
# PRINT STATUS
# ==============================================================================
print_status() {
    echo ""
    echo "=========================================="
    echo -e "${CYAN}  ReDroidCPP - Android Container Ready${NC}"
    echo "=========================================="
    echo ""
    echo "  Configuration:"
    echo "    Memory:      ${REDROID_MEM_SIZE}"
    echo "    GPU Mode:    ${REDROID_GPU_MODE}"
    echo "    ADB Port:    ${ADB_PORT}"
    echo ""
    echo "  Device Info:"
    echo "    Manufacturer: $(getprop ro.product.manufacturer 2>/dev/null || echo 'Unknown')"
    echo "    Model:        $(getprop ro.product.model 2>/dev/null || echo 'Unknown')"
    echo "    Android:      $(getprop ro.build.version.release 2>/dev/null || echo 'Unknown')"
    echo "    SDK:          $(getprop ro.build.version.sdk 2>/dev/null || echo 'Unknown')"
    echo ""
    echo "  Connection:"
    echo "    Command:     adb connect localhost:${ADB_PORT}"
    echo ""
    echo "=========================================="
    echo ""
}

# ==============================================================================
# CLEANUP HANDLER
# ==============================================================================
cleanup() {
    log "Shutting down ReDroid..."
    stop surfaceflinger 2>/dev/null || true
    stop boot-animation 2>/dev/null || true
}

trap cleanup EXIT

# ==============================================================================
# MAIN
# ==============================================================================
main() {
    echo ""
    echo "=========================================="
    echo -e "${CYAN}  ReDroidCPP v3.0 - Pure ReDroid Container${NC}"
    echo "=========================================="
    echo ""
    log "Starting pure ReDroid container..."
    log "Target: <3s boot, ~1GB-1.5GB RAM"
    echo ""
    
    # Run startup sequence
    check_prerequisites
    configure_network
    apply_spoofing
    
    # Wait for boot (ReDroid boots very fast!)
    wait_for_boot
    
    # Start ADB
    start_adb
    
    # Print status
    print_status
    
    log "Container running. Press Ctrl+C to stop."
    echo ""
    
    # Keep container alive - ReDroid handles its own process
    # Use exec to replace shell with the main container process
    if command -v toybox &>/dev/null; then
        exec toybox cat /dev/null 2>/dev/null || true
    fi
    
    # Alternative: wait on init
    while true; do
        sleep 60 & 
        wait $!
    done
}

main "$@"
