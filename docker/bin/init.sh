#!/bin/bash
# ==============================================================================
# VirtualPhonePro - Device Provisioning Script
# Apply device profile properties to Android container
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
CONFIG_DIR="/opt/vpp/config"
SCRIPTS_DIR="/opt/vpp/scripts"
DATA_DIR="/data"
PROFILE_ID=""
LOG_FILE="/data/logs/init.log"

# Device defaults (can be overridden by environment variables)
MANUFACTURER="${VPP_DEVICE_MANUFACTURER:-Samsung}"
MODEL="${VPP_DEVICE_MODEL:-Galaxy S24 Ultra}"
BRAND="${VPP_DEVICE_BRAND:-samsung}"
DEVICE="${VPP_DEVICE:-dm3q}"
ANDROID_VERSION="${VPP_ANDROID_VERSION:-14}"

# Logging
log_info() {
    echo -e "${GREEN}[INFO]${NC} $(date '+%H:%M:%S') - $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INFO] $1" >> "$LOG_FILE"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $(date '+%H:%M:%S') - $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [WARN] $1" >> "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%H:%M:%S') - $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [ERROR] $1" >> "$LOG_FILE"
}

log_success() {
    echo -e "${CYAN}[✓]${NC} $(date '+%H:%M:%S') - $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [OK] $1" >> "$LOG_FILE"
}

# Check if running as root
check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        log_error "This script must be run as root"
        exit 1
    fi
}

# Check if properties can be set
check_properties() {
    if [ ! -d /system ]; then
        log_warn "Android system not mounted at /system"
        return 1
    fi
    return 0
}

# Generate device identity
generate_identity() {
    log_info "Generating device identity..."
    
    # Generate IMEI using Luhn algorithm
    TAC="35875109"  # Samsung TAC
    RANDOM_PART=$(head /dev/urandom | tr -dc '0-9' | head -c 6)
    IMEI="${TAC}${RANDOM_PART}"
    
    # Calculate Luhn check digit
    calculate_luhn() {
        local num=$1
        local sum=0
        local alt=1
        for (( i=${#num}-1; i>=0; i-- )); do
            local n=${num:$i:1}
            if [ $alt -eq 1 ]; then
                n=$((n * 2))
                [ $n -gt 9 ] && n=$((n - 9))
            fi
            sum=$((sum + n))
            alt=$((1 - alt))
        done
        echo $((10 - (sum % 10) % 10))
    }
    
    CHECK_DIGIT=$(calculate_luhn "${IMEI}")
    IMEI="${IMEI}${CHECK_DIGIT}"
    
    # Generate Android ID (16 hex chars)
    ANDROID_ID=$(head /dev/urandom | tr -dc 'a-f0-9' | head -c 16)
    
    # Generate Serial Number
    SERIAL="R5CW${RANDOM:0:8}${PRODUCT:0:3}"
    SERIAL=$(echo "$SERIAL" | tr '[:lower:]' '[:upper:]')
    
    # Generate GSF ID (10 digits)
    GSF_ID=$(head /dev/urandom | tr -dc '0-9' | head -c 10)
    
    # Generate AAID (UUID)
    AAID=$(cat /proc/sys/kernel/random/uuid | tr -d '-')
    
    log_info "Generated Identity:"
    log_info "  IMEI: $IMEI"
    log_info "  Android ID: $ANDROID_ID"
    log_info "  Serial: $SERIAL"
    log_info "  GSF ID: $GSF_ID"
    log_info "  AAID: $AAID"
    
    echo "IMEI=$IMEI"
    echo "ANDROID_ID=$ANDROID_ID"
    echo "SERIAL=$SERIAL"
    echo "GSF_ID=$GSF_ID"
    echo "AAID=$AAID"
}

# Generate MAC addresses
generate_mac() {
    log_info "Generating MAC addresses..."
    
    # WiFi MAC with Samsung OUI
    WIFI_MAC="8C:71:F8:$(head /dev/urandom | tr -dc '0-9A-F' | head -c 6 | sed 's/../&:/g')"
    
    # Bluetooth MAC
    BT_MAC="94:EB:2C:$(head /dev/urandom | tr -dc '0-9A-F' | head -c 6 | sed 's/../&:/g')"
    
    # Ethernet MAC
    ETH_MAC="00:1A:11:$(head /dev/urandom | tr -dc '0-9A-F' | head -c 6 | sed 's/../&:/g')"
    
    log_info "  WiFi MAC: $WIFI_MAC"
    log_info "  Bluetooth MAC: $BT_MAC"
    log_info "  Ethernet MAC: $ETH_MAC"
    
    echo "WIFI_MAC=$WIFI_MAC"
    echo "BT_MAC=$BT_MAC"
    echo "ETH_MAC=$ETH_MAC"
}

# Generate SIM configuration
generate_sim() {
    log_info "Generating SIM configuration..."
    
    # ICCID (20 digits)
    ICCID="89610$(head /dev/urandom | tr -dc '0-9' | head -c 15)"
    
    # IMSI (15 digits)
    IMSI="310260$(head /dev/urandom | tr -dc '0-9' | head -c 9)"
    
    CARRIER="T-Mobile"
    COUNTRY="US"
    MCC="310"
    MNC="260"
    
    log_info "  Carrier: $CARRIER"
    log_info "  ICCID: $ICCID"
    log_info "  IMSI: $IMSI"
    log_info "  MCC/MNC: $MCC/$MNC"
    
    echo "ICCID=$ICCID"
    echo "IMSI=$IMSI"
    echo "CARRIER=$CARRIER"
    echo "COUNTRY=$COUNTRY"
    echo "MCC=$MCC"
    echo "MNC=$MNC"
}

# Set Android system properties
set_properties() {
    local profile_file="$1"
    
    if [ -f "$profile_file" ]; then
        log_info "Loading profile: $profile_file"
        source "$profile_file"
    fi
    
    log_info "Setting Android system properties..."
    
    # Device Identity
    setprop ro.gsm.deviceIMEI "$IMEI" 2>/dev/null || true
    setprop ro.gsm.imei "$IMEI" 2>/dev/null || true
    setprop ro.ril.gps.concurrentGPS "true" 2>/dev/null || true
    
    # Serial Number
    setprop ro.serialno "$SERIAL" 2>/dev/null || true
    
    # Android ID
    setprop ro.setupwizard.mode "OPTIONAL" 2>/dev/null || true
    settings put secure android_id "$ANDROID_ID" 2>/dev/null || true
    
    # GSF ID
    setprop ro.com.google.gms.gsf $GSF_ID 2>/dev/null || true
    
    # Advertising ID
    setprop persist.gservices.enable_ad_id "true" 2>/dev/null || true
    
    # MAC Addresses
    setprop wifi.interface "wlan0" 2>/dev/null || true
    setprop eth0_mac "$ETH_MAC" 2>/dev/null || true
    setprop wifi_mac "$WIFI_MAC" 2>/dev/null || true
    setprop bluetooth_mac "$BT_MAC" 2>/dev/null || true
    
    # SIM
    setprop persist.radio.multisim.config "dsds" 2>/dev/null || true
    setprop gsm.sim.operator.numeric "$MCC$MNC" 2>/dev/null || true
    setprop gsm.sim.operator.alpha "$CARRIER" 2>/dev/null || true
    setprop gsm.operator.iso-country "$COUNTRY" 2>/dev/null || true
    
    log_info "System properties set successfully"
}

# Generate and apply complete profile
setup_device() {
    log_info "Setting up device profile..."
    log_info "Manufacturer: $MANUFACTURER"
    log_info "Model: $MODEL"
    log_info "Android Version: $ANDROID_VERSION"
    
    # Generate all identities
    generate_identity
    generate_mac
    generate_sim
    
    # Save to profile file
    PROFILE_FILE="$DATA_DIR/current_profile.sh"
    mkdir -p "$DATA_DIR"
    
    cat > "$PROFILE_FILE" << EOF
# Device Profile - Generated $(date)
MANUFACTURER=$MANUFACTURER
MODEL=$MODEL
ANDROID_VERSION=$ANDROID_VERSION
IMEI=$IMEI
ANDROID_ID=$ANDROID_ID
SERIAL=$SERIAL
GSF_ID=$GSF_ID
AAID=$AAID
WIFI_MAC=$WIFI_MAC
BT_MAC=$BT_MAC
ETH_MAC=$ETH_MAC
ICCID=$ICCID
IMSI=$IMSI
CARRIER=$CARRIER
COUNTRY=$COUNTRY
MCC=$MCC
MNC=$MNC
EOF
    
    chmod 600 "$PROFILE_FILE"
    log_info "Profile saved to $PROFILE_FILE"
    
    # Apply properties
    set_properties "$PROFILE_FILE"
    
    # Create device info JSON
    cat > "$DATA_DIR/device_info.json" << EOF
{
    "profile_id": "$(cat /proc/sys/kernel/random/uuid)",
    "manufacturer": "$MANUFACTURER",
    "model": "$MODEL",
    "android_version": "$ANDROID_VERSION",
    "imei": "$IMEI",
    "android_id": "$ANDROID_ID",
    "serial": "$SERIAL",
    "gsf_id": "$GSF_ID",
    "advertising_id": "$AAID",
    "wifi_mac": "$WIFI_MAC",
    "bluetooth_mac": "$BT_MAC",
    "ethernet_mac": "$ETH_MAC",
    "carrier": {
        "name": "$CARRIER",
        "country": "$COUNTRY",
        "mcc": "$MCC",
        "mnc": "$MNC"
    },
    "created_at": "$(date -Iseconds)"
}
EOF
    
    log_info "Device setup complete!"
}

# Load existing profile
load_profile() {
    local profile_id="$1"
    local profile_file="$DEVICE_PROFILE_DIR/${profile_id}.sh"
    
    if [ -f "$profile_file" ]; then
        log_info "Loading profile: $profile_id"
        set_properties "$profile_file"
        return 0
    else
        log_error "Profile not found: $profile_id"
        return 1
    fi
}

# Apply custom fingerprint
set_fingerprint() {
    local manufacturer="$1"
    local model="$2"
    local brand="$3"
    local sdk_version="$4"
    
    local fingerprint="${manufacturer}/${model}/${model}:${sdk_version}/$(date +%Y%m%d)/eng.root.$(date +%Y%m%d%H%M):userdebug/release-keys"
    
    log_info "Setting fingerprint: $fingerprint"
    setprop ro.build.fingerprint "$fingerprint" 2>/dev/null || true
    setprop ro.product.build.fingerprint "$fingerprint" 2>/dev/null || true
}

# Main entry point
main() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║       VirtualPhonePro - Device Provisioning Script v1.0         ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log_info "=========================================="
    log_info "  VirtualPhonePro Device Setup"
    log_info "  Version: 1.0.0"
    log_info "=========================================="
    
    # Create log directory
    mkdir -p /data/logs
    
    case "${1:-setup}" in
        setup)
            setup_device
            ;;
        load)
            if [ -z "$2" ]; then
                log_error "Profile ID required"
                exit 1
            fi
            load_profile "$2"
            ;;
        identity)
            generate_identity
            ;;
        mac)
            generate_mac
            ;;
        sim)
            generate_sim
            ;;
        fingerprint)
            set_fingerprint "${2:-Samsung}" "${3:-Galaxy S24 Ultra}" "${4:-Samsung}" "${5:-15}"
            ;;
        status)
            echo "=== Device Identity ==="
            echo "IMEI: $(getprop ro.gsm.imei 2>/dev/null || echo 'Not set')"
            echo "Serial: $(getprop ro.serialno 2>/dev/null || echo 'Not set')"
            echo "Android ID: $(settings get secure android_id 2>/dev/null || echo 'Not set')"
            ;;
        spoof)
            # Run the comprehensive spoofing script
            if [ -f "${SCRIPTS_DIR}/spoof-properties.sh" ]; then
                log_info "Running comprehensive property spoofing..."
                "${SCRIPTS_DIR}/spoof-properties.sh"
            else
                log_warn "Spoof script not found"
            fi
            ;;
        all)
            # Run all setup scripts
            log_info "Running full device configuration..."
            setup_device
            if [ -f "${SCRIPTS_DIR}/spoof-properties.sh" ]; then
                "${SCRIPTS_DIR}/spoof-properties.sh"
            fi
            if [ -f "${SCRIPTS_DIR}/setup-network.sh" ]; then
                "${SCRIPTS_DIR}/setup-network.sh"
            fi
            if [ -f "${SCRIPTS_DIR}/setup-gps.sh" ]; then
                "${SCRIPTS_DIR}/setup-gps.sh"
            fi
            log_success "Full configuration completed!"
            ;;
        *)
            echo "Usage: $0 {setup|load|identity|mac|sim|fingerprint|spoof|all|status}"
            echo ""
            echo "Commands:"
            echo "  setup              - Generate and apply complete device profile"
            echo "  load <profile_id>  - Load existing profile from profiles directory"
            echo "  identity           - Generate only device identity"
            echo "  mac                - Generate only MAC addresses"
            echo "  sim                - Generate only SIM configuration"
            echo "  fingerprint        - Set custom fingerprint"
            echo "  spoof              - Run comprehensive property spoofing"
            echo "  all                - Run full device configuration"
            echo "  status             - Show current device identity"
            exit 1
            ;;
    esac
}

# Run
main "$@"
