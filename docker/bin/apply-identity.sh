#!/system/bin/sh
# ==============================================================================
# VirtualPhonePro - Apply Device Identity Script
# Run after boot to apply all spoofed identities
# ==============================================================================

set -e

LOG_FILE="/data/logs/identity.log"
CONFIG_FILE="/opt/vpp/config/device.properties"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# ==============================================================================
# Load Configuration
# ==============================================================================
load_config() {
    if [ -f "$CONFIG_FILE" ]; then
        log "Loading configuration from $CONFIG_FILE"
        . "$CONFIG_FILE"
    else
        log "Config file not found, using environment variables"
    fi
}

# ==============================================================================
# Apply Device Identity
# ==============================================================================
apply_product_info() {
    log "Applying product info..."
    
    setprop ro.product.brand "${RO_PRODUCT_BRAND:-samsung}"
    setprop ro.product.manufacturer "${RO_PRODUCT_MANUFACTURER:-Samsung Electronics}"
    setprop ro.product.model "${RO_PRODUCT_MODEL:-SM-S928B}"
    setprop ro.product.device "${RO_PRODUCT_DEVICE:-dm3q}"
    setprop ro.product.name "${RO_PRODUCT_NAME:-dm3q}"
    
    # Copy to system properties
    setprop ro.product.brand.system "${RO_PRODUCT_BRAND:-samsung}"
    setprop ro.product.manufacturer.system "${RO_PRODUCT_MANUFACTURER:-Samsung Electronics}"
    setprop ro.product.model.system "${RO_PRODUCT_MODEL:-SM-S928B}"
    setprop ro.product.device.system "${RO_PRODUCT_DEVICE:-dm3q}"
}

# ==============================================================================
# Apply Build Info
# ==============================================================================
apply_build_info() {
    log "Applying build info..."
    
    local android_ver="${RO_BUILD_VERSION_RELEASE:-14}"
    local build_id="${RO_BUILD_ID:-UP1A.231005.007}"
    local build_fingerprint="${RO_BUILD_FINGERPRINT}"
    
    if [ -z "$build_fingerprint" ]; then
        build_fingerprint="samsung/dm3q/dm3q:${android_ver}/${build_id}/$(date +%Y%m%d):userdebug/release-keys"
    fi
    
    setprop ro.build.fingerprint "$build_fingerprint"
    setprop ro.product.build.fingerprint "$build_fingerprint"
    setprop ro.build.id "$build_id"
    setprop ro.build.type "${RO_BUILD_TYPE:-userdebug}"
    setprop ro.build.version.release "$android_ver"
    setprop ro.build.version.sdk "${RO_BUILD_VERSION_SDK:-34}"
    setprop ro.build.version.security_patch "${RO_BUILD_VERSION_SECURITY_PATCH:-2024-01-01}"
    setprop ro.bootloader "${RO_BOOTLOADER:-S928BXXU1AXXX}"
}

# ==============================================================================
# Apply Device Identity (IMEI, Serial, etc.)
# ==============================================================================
apply_device_ids() {
    log "Applying device IDs..."
    
    # IMEI
    if [ -n "${RO_GSM_DEVICE_IMEI}" ]; then
        setprop ro.gsm.device.imei "${RO_GSM_DEVICE_IMEI}"
        setprop persist.radio.imei "${RO_GSM_DEVICE_IMEI}"
        setprop gsm.imei "${RO_GSM_DEVICE_IMEI}"
    fi
    
    # IMEI2
    if [ -n "${RO_GSM_DEVICE_IMEI2}" ]; then
        setprop ro.gsm.device.imei2 "${RO_GSM_DEVICE_IMEI2}"
    fi
    
    # Serial Number
    if [ -n "${RO_SERIALNO}" ]; then
        setprop ro.serialno "${RO_SERIALNO}"
        setprop ro.boot.serialno "${RO_SERIALNO}"
    fi
    
    # Android ID
    if [ -n "${RO_ANDROID_ID}" ]; then
        setprop ro.android_id "${RO_ANDROID_ID}"
        settings put secure android_id "${RO_ANDROID_ID}" 2>/dev/null || true
    fi
    
    # GSF ID
    if [ -n "${RO_GSF_ID}" ]; then
        setprop ro.com.google.gms.gsf "${RO_GSF_ID}"
        setprop persist.gservices.gsfid "${RO_GSF_ID}"
    fi
    
    # Advertising ID
    if [ -n "${RO_ADVERTISING_ID}" ]; then
        setprop ro.google.advertising_id "${RO_ADVERTISING_ID}"
        settings put secure limit_ad_tracking "0" 2>/dev/null || true
    fi
}

# ==============================================================================
# Apply Network Identity
# ==============================================================================
apply_network_ids() {
    log "Applying network identities..."
    
    # WiFi MAC
    if [ -n "${NET_WIFI_MAC}" ]; then
        setprop wifi_mac "${NET_WIFI_MAC}"
        setprop ro.mac_wifi "${NET_WIFI_MAC}"
    fi
    
    # Bluetooth MAC
    if [ -n "${NET_BLUETOOTH_MAC}" ]; then
        setprop ro.mac_bt "${NET_BLUETOOTH_MAC}"
        setprop bluetooth_mac "${NET_BLUETOOTH_MAC}"
    fi
    
    # Hostname
    if [ -n "${NET_HOSTNAME}" ]; then
        setprop net.hostname "${NET_HOSTNAME}"
    fi
}

# ==============================================================================
# Apply SIM Info
# ==============================================================================
apply_sim_info() {
    log "Applying SIM info..."
    
    if [ -n "${RO_SIM_ICCID}" ]; then
        setprop ro.sim.iccid "${RO_SIM_ICCID}"
        settings put secure sim_iccid "${RO_SIM_ICCID}" 2>/dev/null || true
    fi
    
    if [ -n "${RO_GSM_SIM_IMSI}" ]; then
        setprop ro.gsm.sim.imsi "${RO_GSM_SIM_IMSI}"
    fi
    
    if [ -n "${RO_SIM_OPERATOR_NUMERIC}" ]; then
        setprop ro.simoperator.numeric "${RO_SIM_OPERATOR_NUMERIC}"
        setprop gsm.sim.operator.numeric "${RO_SIM_OPERATOR_NUMERIC}"
    fi
    
    if [ -n "${RO_SIM_OPERATOR}" ]; then
        setprop gsm.sim.operator.alpha "${RO_SIM_OPERATOR}"
        setprop ro.simoperator "${RO_SIM_OPERATOR}"
    fi
}

# ==============================================================================
# Apply Verified Boot State
# ==============================================================================
apply_verified_boot() {
    log "Applying verified boot state..."
    
    setprop ro.boot.verifiedbootstate "green"
    setprop ro.boot.flash.locked "1"
    setprop ro.secure_state "unlocked"
    setprop ro.verity.mode "enforcing"
    
    # Generate fake but valid-looking VBMeta digest
    local vbmeta_digest="sha256:$(cat /dev/urandom | tr -dc 'a-f0-9' | head -c 64)"
    setprop ro.boot.vbmeta.digest "$vbmeta_digest"
    setprop ro.vbmeta.digest "$vbmeta_digest"
}

# ==============================================================================
# Apply Hardware Info
# ==============================================================================
apply_hardware_info() {
    log "Applying hardware info..."
    
    setprop ro.hardware "qcom"
    setprop ro.board.platform "taro"
    setprop ro.arch "arm64"
    setprop ro.product.cpu.abi "arm64-v8a"
    setprop ro.product.cpu.abi2 ""
}

# ==============================================================================
# Main
# ==============================================================================
main() {
    log "=========================================="
    log "VirtualPhonePro - Apply Identity Script"
    log "=========================================="
    
    load_config
    apply_product_info
    apply_build_info
    apply_device_ids
    apply_network_ids
    apply_sim_info
    apply_verified_boot
    apply_hardware_info
    
    log "=========================================="
    log "Identity application completed!"
    log "=========================================="
}

main "$@"
