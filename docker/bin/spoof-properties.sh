#!/system/bin/sh
# ==============================================================================
# VirtualPhonePro - Device Property Spoofing Script
# Run this script to spoof all device properties
# ==============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration file location
CONFIG_DIR="/opt/vpp/config"
PROPS_FILE="${CONFIG_DIR}/device.properties"
LOG_FILE="/data/logs/spoof.log"

# ==============================================================================
# Logging Functions
# ==============================================================================
log() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[${timestamp}] [${level}] ${message}" >> "$LOG_FILE"
    
    case "$level" in
        "INFO")  echo -e "${GREEN}[✓]${NC} ${message}" ;;
        "WARN")  echo -e "${YELLOW}[!]${NC} ${message}" ;;
        "ERROR") echo -e "${RED}[✗]${NC} ${message}" ;;
        *)       echo -e "${BLUE}[*]${NC} ${message}" ;;
    esac
}

# ==============================================================================
# Helper Functions
# ==============================================================================

# Set property with fallback
set_prop_safe() {
    local prop="$1"
    local value="$2"
    
    if setprop "$prop" "$value" 2>/dev/null; then
        log "INFO" "Set ${prop} = ${value}"
        return 0
    else
        log "WARN" "Failed to set ${prop} (may need root)"
        return 1
    fi
}

# Generate random hex string
generate_hex() {
    local length="${1:-16}"
    cat /dev/urandom | tr -dc 'a-f0-9' | head -c "$length"
}

# Generate random numeric string
generate_numeric() {
    local length="${1:-10}"
    cat /dev/urandom | tr -dc '0-9' | head -c "$length"
}

# Calculate Luhn check digit for IMEI
calculate_luhn() {
    local base="$1"
    local sum=0
    local alt=1
    
    for (( i=${#base}-1; i>=0; i-- )); do
        local n=${base:$i:1}
        if [ $alt -eq 1 ]; then
            n=$((n * 2))
            [ $n -gt 9 ] && n=$((n - 9))
        fi
        sum=$((sum + n))
        alt=$((1 - alt))
    done
    
    echo $((10 - (sum % 10) % 10))
}

# Generate valid IMEI
generate_imei() {
    local tac="$1"
    local serial=$(generate_numeric 6)
    local base="${tac}${serial}"
    local check=$(calculate_luhn "$base")
    echo "${base}${check}"
}

# ==============================================================================
# Device Identity Spoofing
# ==============================================================================
spoof_device_identity() {
    log "INFO" "=== Spoofing Device Identity ==="
    
    # Load from environment or generate
    local brand="${VPP_DEVICE_BRAND:-samsung}"
    local manufacturer="${VPP_DEVICE_MANUFACTURER:-Samsung}"
    local model="${VPP_DEVICE_MODEL:-SM-S928B}"
    local device="${VPP_DEVICE:-dm3q}"
    local product="${VPP_PRODUCT:-dm3q}"
    
    # Product properties
    set_prop_safe "ro.product.brand" "$brand"
    set_prop_safe "ro.product.manufacturer" "$manufacturer"
    set_prop_safe "ro.product.model" "$model"
    set_prop_safe "ro.product.device" "$device"
    set_prop_safe "ro.product.name" "$product"
    
    # Legacy product properties
    set_prop_safe "ro.product.board" "$device"
    set_prop_safe "ro.product.cpu.abi" "arm64-v8a"
    set_prop_safe "ro.product.cpu.abi2" ""
    
    log "INFO" "Device identity spoofed: ${manufacturer} ${model}"
}

# ==============================================================================
# Build Properties Spoofing
# ==============================================================================
spoof_build_properties() {
    log "INFO" "=== Spoofing Build Properties ==="
    
    local android_version="${VPP_ANDROID_VERSION:-14}"
    local build_id="${VPP_BUILD_ID:-UP1A.231005.007}"
    local build_type="${VPP_BUILD_TYPE:-userdebug}"
    local security_patch="${VPP_SECURITY_PATCH:-2024-01-01}"
    local bootloader="${VPP_BOOTLOADER:-S928BXXU1AXXX}"
    
    # Build properties
    set_prop_safe "ro.build.id" "$build_id"
    set_prop_safe "ro.build.type" "$build_type"
    set_prop_safe "ro.build.version.release" "$android_version"
    set_prop_safe "ro.build.version.sdk" "34"
    set_prop_safe "ro.build.version.security_patch" "$security_patch"
    set_prop_safe "ro.bootloader" "$bootloader"
    
    # Build fingerprint
    local fingerprint_prefix="${VPP_FINGERPRINT_PREFIX:-samsung/dm3q/dm3q}"
    local fingerprint="${fingerprint_prefix}:${android_version}/${build_id}/$(generate_hex 8):${build_type}/release-keys"
    
    set_prop_safe "ro.build.fingerprint" "$fingerprint"
    set_prop_safe "ro.product.build.fingerprint" "$fingerprint"
    set_prop_safe "ro.build.product" "$device"
    
    log "INFO" "Build properties spoofed"
}

# ==============================================================================
# IMEI & Serial Spoofing
# ==============================================================================
spoof_identity() {
    log "INFO" "=== Spoofing IMEI and Serial ==="
    
    # IMEI
    local imei="${VPP_IMEI:-}"
    if [ -z "$imei" ]; then
        # Generate valid IMEI
        local tac="35875109"  # Samsung TAC
        imei=$(generate_imei "$tac")
    fi
    set_prop_safe "ro.gsm.device.imei" "$imei"
    set_prop_safe "persist.radio.imei" "$imei"
    set_prop_safe "ril.imei" "$imei"
    
    # IMEI2 for dual SIM
    local imei2="${VPP_IMEI2:-}"
    if [ -z "$imei2" ]; then
        local tac2="35875108"
        imei2=$(generate_imei "$tac2")
    fi
    set_prop_safe "ro.gsm.device.imei2" "$imei2"
    
    # Serial Number
    local serial="${VPP_SERIAL:-}"
    if [ -z "$serial" ]; then
        serial="R5CW$(generate_hex 8)"
    fi
    set_prop_safe "ro.serialno" "$serial"
    set_prop_safe "ro.boot.serialno" "$serial"
    
    # Android ID
    local android_id="${VPP_ANDROID_ID:-}"
    if [ -z "$android_id" ]; then
        android_id=$(generate_hex 16)
    fi
    set_prop_safe "ro.android_id" "$android_id"
    settings put secure android_id "$android_id" 2>/dev/null || true
    
    # GSF ID
    local gsf_id="${VPP_GSF_ID:-}"
    if [ -z "$gsf_id" ]; then
        gsf_id=$(generate_numeric 10)
    fi
    set_prop_safe "ro.com.google.gms.gsf" "$gsf_id"
    set_prop_safe "persist.gservices.gsfid" "$gsf_id"
    
    log "INFO" "IMEI spoofed: ${imei}"
    log "INFO" "Serial spoofed: ${serial}"
}

# ==============================================================================
# MAC Address Spoofing
# ==============================================================================
spoof_network_identity() {
    log "INFO" "=== Spoofing Network Identity ==="
    
    # WiFi MAC
    local wifi_mac="${VPP_WIFI_MAC:-}"
    if [ -z "$wifi_mac" ]; then
        wifi_mac="8C:71:F8:$(generate_hex 2):$(generate_hex 2):$(generate_hex 2)"
    fi
    set_prop_safe "wifi.interface" "wlan0"
    set_prop_safe "wifi_mac" "$wifi_mac"
    set_prop_safe "ro.mac_wifi" "$wifi_mac"
    
    # Bluetooth MAC
    local bt_mac="${VPP_BLUETOOTH_MAC:-}"
    if [ -z "$bt_mac" ]; then
        bt_mac="8C:71:F8:$(generate_hex 2):$(generate_hex 2):$(generate_hex 2)"
    fi
    set_prop_safe "ro.mac_bt" "$bt_mac"
    set_prop_safe "bluetooth_mac" "$bt_mac"
    
    # Ethernet MAC
    local eth_mac="00:1A:11:$(generate_hex 2):$(generate_hex 2):$(generate_hex 2)"
    set_prop_safe "ro.mac_eth" "$eth_mac"
    set_prop_safe "net.eth0.mac" "$eth_mac"
    
    # Hostname
    local hostname="${VPP_HOSTNAME:-android}"
    set_prop_safe "net.hostname" "$hostname"
    
    log "INFO" "WiFi MAC: ${wifi_mac}"
    log "INFO" "Bluetooth MAC: ${bt_mac}"
}

# ==============================================================================
# SIM Configuration Spoofing
# ==============================================================================
spoof_sim() {
    log "INFO" "=== Spoofing SIM Configuration ==="
    
    local iccid="${VPP_ICCID:-}"
    if [ -z "$iccid" ]; then
        iccid="89610$(generate_numeric 15)"
    fi
    set_prop_safe "ro.sim Ic cid" "$iccid"
    settings put secure sim_iccid "$iccid" 2>/dev/null || true
    
    local imsi="${VPP_IMSI:-}"
    if [ -z "$imsi" ]; then
        local mcc="${VPP_MCC:-310}"
        local mnc="${VPP_MNC:-260}"
        imsi="${mcc}${mnc}$(generate_numeric 9)"
    fi
    set_prop_safe "ro.gsm.sim.imsi" "$imsi"
    set_prop_safe "ro.simoperator.numeric" "${VPP_MCC}${VPP_MNC}"
    set_prop_safe "ro.simoperator" "${VPP_CARRIER}"
    
    # Carrier info
    set_prop_safe "gsm.sim.operator.numeric" "${VPP_MCC}${VPP_MNC}"
    set_prop_safe "gsm.sim.operator.alpha" "${VPP_CARRIER}"
    set_prop_safe "gsm.operator.iso-country" "US"
    set_prop_safe "gsm.network.type" "LTE"
    
    log "INFO" "SIM IMSI spoofed: ${imsi}"
}

# ==============================================================================
# Verified Boot State
# ==============================================================================
spoof_verified_boot() {
    log "INFO" "=== Spoofing Verified Boot State ==="
    
    # Green = unlocked/verified
    set_prop_safe "ro.boot.verifiedbootstate" "green"
    set_prop_safe "ro.boot.flash.locked" "1"
    set_prop_safe "ro.secure_state" "unlocked"
    
    # VBMeta digest (fake)
    local vbmeta_digest="sha256:$(generate_hex 64)"
    set_prop_safe "ro.boot.vbmeta.digest" "$vbmeta_digest"
    set_prop_safe "ro.vbmeta.digest" "$vbmeta_digest"
    
    # Keymaster
    set_prop_safe "ro.keymaster.version" "4.1"
    set_prop_safe "ro.hardware.keystore" "strongbox"
    
    # Verified boot key hash (fake but valid format)
    local vbkey_hash=$(generate_hex 32)
    set_prop_safe "ro.verifiedbootkey.hash" "$vbkey_hash"
    
    log "INFO" "Verified boot state: green (spoofed)"
}

# ==============================================================================
# Hardware Properties
# ==============================================================================
spoof_hardware() {
    log "INFO" "=== Spoofing Hardware Properties ==="
    
    # CPU/Hardware
    set_prop_safe "ro.hardware" "qcom"
    set_prop_safe "ro.board.platform" "taro"
    set_prop_safe "ro.arch" "arm64"
    
    # GPU
    set_prop_safe "ro.hardware.gralloc" "default"
    set_prop_safe "ro.opengles.version" "196610"  # OpenGL ES 3.2
    
    # SELinux
    set_prop_safe "ro.build.selinux" "1"
    setprop ctl.restart adbd 2>/dev/null || true
    
    log "INFO" "Hardware properties spoofed"
}

# ==============================================================================
# GPS Spoofing
# ==============================================================================
spoof_gps() {
    log "INFO" "=== Spoofing GPS Location ==="
    
    local lat="${VPP_GPS_LAT:-37.7749}"
    local lon="${VPP_GPS_LON:--122.4194}"
    
    # Set GPS location via settings
    settings put secure location_providers_allowed "gps,network" 2>/dev/null || true
    
    # Fake GPS location (requires location spoofing app)
    log "INFO" "GPS location set: ${lat}, ${lon}"
    log "WARN" "For actual GPS spoofing, install a location spoofing app"
}

# ==============================================================================
# Google Services Spoofing
# ==============================================================================
spoof_google_services() {
    log "INFO" "=== Spoofing Google Services ==="
    
    set_prop_safe "ro.com.google.gms.gsf" "${VPP_GSF_ID:-1234567890}"
    set_prop_safe "ro.google.gms.version" "230604034"
    set_prop_safe "ro.setupwizard.mode" "OPTIONAL"
    set_prop_safe "ro.setupwizard.enable_bypass" "true"
    
    # SafetyNet/Play Integrity
    set_prop_safe "ro.verity.mode" "enforcing"
    set_prop_safe "ro.ignore_atrace" "false"
    
    log "INFO" "Google services properties set"
}

# ==============================================================================
# Main Spoofing Function
# ==============================================================================
main() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║       VirtualPhonePro - Device Property Spoofer v1.0          ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log "INFO" "Starting device property spoofing..."
    
    # Run all spoofing functions
    spoof_device_identity
    spoof_build_properties
    spoof_identity
    spoof_network_identity
    spoof_sim
    spoof_verified_boot
    spoof_hardware
    spoof_gps
    spoof_google_services
    
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║            Property Spoofing Completed Successfully!          ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log "INFO" "=== All properties spoofed successfully ==="
}

# Run main function
main "$@"
