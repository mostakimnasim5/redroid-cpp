#!/bin/bash
# ==============================================================================
# RedroidCPP - Docker Entrypoint Script
# Initialize and start the Android emulator
# ==============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
ANDROID_HOME="/system"
REDROID_DATA="/data"
PROFILE_DIR="/opt/profiles"
ADB_PORT="${ADB_PORT:-15555}"

# Print banner
print_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║                                                                  ║"
    echo "║   ██████╗ ███████╗███╗   ██╗███████╗████████╗███████╗██████╗ ███╗   ║"
    echo "║   ██╔══██╗██╔════╝████╗  ██║██╔════╝╚══██╔══╝██╔════╝██╔══██╗████╗  ║"
    echo "║   ██████╔╝█████╗  ██╔██╗ ██║███████╗   ██║   █████╗  ██████╔╝██╔████╔██║"
    echo "║   ██╔═══╝ ██╔══╝  ██║╚██╗██║╚════██║   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║"
    echo "║   ██║     ███████╗██║ ╚████║███████║   ██║   ███████╗██║  ██║██║ ╚═╝ ║"
    echo "║   ╚═╝     ╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝ ║"
    echo "║                                                                  ║"
    echo "║         Docker-based Android Emulator v3.0.0                     ║"
    echo "║         Device Profile Integration Ready                         ║"
    echo "║                                                                  ║"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $(date '+%H:%M:%S') - $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $(date '+%H:%M:%S') - $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%H:%M:%S') - $1"
}

# Check for required devices
check_devices() {
    log_info "Checking required devices..."
    
    local required_devices=(
        "/dev/kvm"
        "/dev/binderfs/binder"
        "/dev/binder"
    )
    
    local missing=0
    for device in "${required_devices[@]}"; do
        if [ -c "$device" ] || [ -d "$device" ]; then
            echo "  ✓ $device"
        else
            echo "  ✗ $device (missing)"
            missing=1
        fi
    done
    
    if [ $missing -eq 1 ]; then
        log_warn "Some devices are missing. Emulator may not work properly."
    fi
}

# Setup device profile
setup_device() {
    log_info "Setting up device profile..."
    
    # Check for profile file
    if [ -n "$DEVICE_PROFILE_ID" ] && [ -f "$PROFILE_DIR/${DEVICE_PROFILE_ID}.sh" ]; then
        log_info "Loading profile: $DEVICE_PROFILE_ID"
        source "$PROFILE_DIR/${DEVICE_PROFILE_ID}.sh"
    else
        log_info "Generating new device profile..."
        
        # Check if init script exists
        if [ -f /opt/redroid-init.sh ]; then
            chmod +x /opt/redroid-init.sh
            /opt/redroid-init.sh setup
        else
            log_warn "Init script not found, skipping device setup"
        fi
    fi
}

# Configure network
setup_network() {
    log_info "Configuring network..."
    
    # Set hostname
    hostname android-emulator
    echo "android-emulator" > /etc/hostname
    
    # Configure ADB
    setprop service.adb.tcp.port 5555 2>/dev/null || true
}

# Configure GPU
setup_gpu() {
    log_info "Configuring GPU (mode: ${REDROID_GPU_MODE:-auto})..."
    
    # GPU mode can be: host, swiftshader, software, auto
    if [ -n "$REDROID_GPU_MODE" ]; then
        setprop ro.hardware.gralloc "$REDROID_GPU_MODE" 2>/dev/null || true
    fi
}

# Start Android services
start_android() {
    log_info "Starting Android system..."
    
    # Wait for boot
    local boot_timeout=120
    local boot_count=0
    
    echo -n "Waiting for Android to boot"
    while [ $boot_count -lt $boot_timeout ]; do
        if getprop sys.boot_completed 2>/dev/null | grep -q "1"; then
            echo ""
            log_info "Android boot completed!"
            return 0
        fi
        echo -n "."
        sleep 1
        boot_count=$((boot_count + 1))
    done
    
    echo ""
    log_warn "Boot timeout reached, continuing anyway..."
    return 1
}

# Run ADB server
start_adb() {
    log_info "Starting ADB server on port $ADB_PORT..."
    
    # Enable ADB over network
    setprop service.adb.tcp.port 5555 2>/dev/null || true
    
    # Stop existing adbd if running
    stop adbd 2>/dev/null || true
    
    # Start adbd
    start adbd 2>/dev/null || true
    
    log_info "ADB server started"
}

# Start VNC server (if installed)
start_vnc() {
    if command -v x11vnc &> /dev/null; then
        log_info "Starting VNC server..."
        x11vnc -display :0 -forever -shared -rfbport 5900 &
    else
        log_warn "VNC server not installed"
    fi
}

# Print status
print_status() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "                    EMULATOR STATUS"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    echo -e "${YELLOW}[ DEVICE INFO ]${NC}"
    echo "  Manufacturer: $(getprop ro.product.manufacturer 2>/dev/null || echo 'Unknown')"
    echo "  Model:        $(getprop ro.product.model 2>/dev/null || echo 'Unknown')"
    echo "  Android:      $(getprop ro.build.version.release 2>/dev/null || echo 'Unknown')"
    echo "  SDK:          $(getprop ro.build.version.sdk 2>/dev/null || echo 'Unknown')"
    echo ""
    
    echo -e "${YELLOW}[ IDENTITY ]${NC}"
    echo "  Serial:       $(getprop ro.serialno 2>/dev/null || echo 'Not set')"
    echo "  IMEI:         $(getprop ro.gsm.imei 2>/dev/null || echo 'Not set')"
    echo "  Android ID:   $(getprop ro.android_id 2>/dev/null || echo 'Not set')"
    echo ""
    
    echo -e "${YELLOW}[ NETWORK ]${NC}"
    echo "  ADB Port:     $ADB_PORT"
    echo "  MAC (WiFi):   $(getprop wifi_mac 2>/dev/null || echo 'Not set')"
    echo ""
    
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
}

# Cleanup on exit
cleanup() {
    log_info "Shutting down emulator..."
    stop adbd 2>/dev/null || true
}

trap cleanup EXIT

# Main
main() {
    print_banner
    
    log_info "RedroidCPP Emulator starting..."
    log_info "Data directory: $REDROID_DATA"
    log_info "Profile directory: $PROFILE_DIR"
    
    # Check devices
    check_devices
    
    # Setup
    setup_device
    setup_network
    setup_gpu
    
    # Start Android
    start_android
    
    # Start services
    start_adb
    
    # Print status
    print_status
    
    # Handle commands
    case "${1:-shell}" in
        start)
            log_info "Emulator running. Press Ctrl+C to stop."
            while true; do
                sleep 60
            done
            ;;
        shell)
            log_info "Starting shell..."
            exec /system/bin/sh
            ;;
        adb)
            exec adb "$@"
            ;;
        *)
            exec "$@"
            ;;
    esac
}

main "$@"
