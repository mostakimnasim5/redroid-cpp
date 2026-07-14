#!/system/bin/sh
# ==============================================================================
# VirtualPhonePro - Network Setup Script
# Configure network interfaces and DNS
# ==============================================================================

set -e

LOG_FILE="/data/logs/network.log"
CONFIG_DIR="/opt/vpp/config"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# ==============================================================================
# Generate Random MAC Address
# ==============================================================================
generate_mac() {
    local oui="${1:-8C:71:F8}"  # Samsung OUI default
    local mac="${oui}:$(cat /dev/urandom | tr -dc 'A-F0-9' | head -c 2):$(cat /dev/urandom | tr -dc 'A-F0-9' | head -c 2):$(cat /dev/urandom | tr -dc 'A-F0-9' | head -c 2)"
    echo "$mac" | tr '[:lower:]' '[:upper:]'
}

# ==============================================================================
# Setup WiFi Interface
# ==============================================================================
setup_wifi() {
    log "Setting up WiFi interface..."
    
    local wifi_mac="${VPP_WIFI_MAC:-$(generate_mac)}"
    
    # Bring up wlan0 interface
    ip link set wlan0 up 2>/dev/null || true
    
    # Set MAC address
    ip link set wlan0 address "$wifi_mac" 2>/dev/null || true
    
    # Configure DHCP
    dhcpcd wlan0 2>/dev/null || netcfg wlan0 dhcp 2>/dev/null || true
    
    log "WiFi MAC: $wifi_mac"
}

# ==============================================================================
# Setup Ethernet Interface
# ==============================================================================
setup_ethernet() {
    log "Setting up Ethernet interface..."
    
    local eth_mac="${VPP_ETHERNET_MAC:-$(generate_mac 00:1A:11)}"
    
    # Bring up eth0
    ip link set eth0 up 2>/dev/null || true
    
    # Set MAC address
    ip link set eth0 address "$eth_mac" 2>/dev/null || true
    
    # Configure DHCP
    dhcpcd eth0 2>/dev/null || netcfg eth0 dhcp 2>/dev/null || true
    
    log "Ethernet MAC: $eth_mac"
}

# ==============================================================================
# Setup DNS
# ==============================================================================
setup_dns() {
    log "Setting up DNS..."
    
    local dns1="${VPP_DNS1:-8.8.8.8}"
    local dns2="${VPP_DNS2:-8.8.4.4}"
    
    # Set DNS via setprop
    setprop net.dns1 "$dns1"
    setprop net.dns2 "$dns2"
    setprop persist.net.dns1 "$dns1"
    setprop persist.net.dns2 "$dns2"
    
    # Configure netd
    setprop net.change "net.dns1" 2>/dev/null || true
    
    log "DNS configured: $dns1, $dns2"
}

# ==============================================================================
# Setup Hostname
# ==============================================================================
setup_hostname() {
    log "Setting up hostname..."
    
    local hostname="${VPP_HOSTNAME:-android-$(cat /dev/urandom | tr -dc 'a-z0-9' | head -c 6)}"
    
    # Set system hostname
    hostname "$hostname"
    setprop net.hostname "$hostname"
    
    # Add to hosts file
    echo "127.0.0.1 localhost $hostname" > /system/etc/hosts 2>/dev/null || true
    
    log "Hostname: $hostname"
}

# ==============================================================================
# Setup Proxy
# ==============================================================================
setup_proxy() {
    log "Setting up proxy..."
    
    # HTTP Proxy
    if [ -n "${HTTP_PROXY}" ]; then
        setprop net.http.proxy "$HTTP_PROXY"
        settings put global http_proxy "$HTTP_PROXY" 2>/dev/null || true
    fi
    
    # HTTPS Proxy
    if [ -n "${HTTPS_PROXY}" ]; then
        setprop net.https.proxy "$HTTPS_PROXY"
    fi
    
    # No proxy
    if [ -n "${NO_PROXY}" ]; then
        setprop net.no_proxy "$NO_PROXY"
    fi
}

# ==============================================================================
# Setup VPN (optional)
# ==============================================================================
setup_vpn() {
    log "Checking VPN configuration..."
    
    # VPN can be configured via VPN app inside Android
    # This is a placeholder for future implementation
    :
}

# ==============================================================================
# Network Speed Optimization
# ==============================================================================
optimize_network() {
    log "Optimizing network performance..."
    
    # TCP buffer sizes for better performance
    setprop net.tcp.buffersize.wifi "524287,1048575,524287,524287,1048575,8388608"
    setprop net.tcp.buffersize.default "524287,1048575,524287,524287,1048575,8388608"
    setprop net.tcp.buffersize.lte "524287,1048575,524287,524287,1048575,8388608"
    
    # TCP congestion type
    setprop net.tcp.congestion "cubic"
    
    log "Network optimization complete"
}

# ==============================================================================
# Main
# ==============================================================================
main() {
    log "=========================================="
    log "VirtualPhonePro - Network Setup"
    log "=========================================="
    
    setup_hostname
    setup_wifi
    setup_ethernet
    setup_dns
    setup_proxy
    optimize_network
    
    log "=========================================="
    log "Network setup completed!"
    log "=========================================="
}

main "$@"
