#!/system/bin/sh
# ==============================================================================
# init_cellular_network.sh - Cellular Network Initialization Script for ReDroid
# ==============================================================================
# This script is executed inside the ReDroid container to set up cellular
# network spoofing and make the network appear as TRANSPORT_CELLULAR
# ==============================================================================

LOGFILE="/data/local/tmp/cellular_network.log"
CONFIG_DIR="/data/local/tmp/network_config"

# Initialize logging
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOGFILE"
}

# Create directories
mkdir -p /data/local/tmp
mkdir -p /data/local/network_config

log "========================================="
log "Cellular Network Initialization Starting"
log "========================================="

# ==============================================================================
# STEP 1: Parse Configuration
# ==============================================================================
log "STEP 1: Loading configuration..."

# Load config from environment or use defaults
COUNTRY_CODE="${COUNTRY_CODE:-US}"
CARRIER_NAME="${CARRIER_NAME:-T-Mobile}"
MCC="${MCC:-310}"
MNC="${MNC:-260}"
NETWORK_TYPE="${NETWORK_TYPE:-LTE}"
PROXY_HOST="${PROXY_HOST:-}"
PROXY_PORT="${PROXY_PORT:-}"
PROXY_TYPE="${PROXY_TYPE:-socks5}"
TIMEZONE="${TIMEZONE:-America/New_York}"
GPS_LAT="${GPS_LAT:-40.7128}"
GPS_LON="${GPS_LON:--74.0060}"

# Generate cellular IP if not provided
if [ -z "$CELLULAR_IP" ]; then
    CELLULAR_IP="10.${RANDOM%256}.${RANDOM%256}.${RANDOM%256%200+50}"
fi
GATEWAY="${GATEWAY:-10.0.0.1}"

log "Country: $COUNTRY_CODE"
log "Carrier: $CARRIER_NAME"
log "MCC/MNC: $MCC$MNC"
log "Network Type: $NETWORK_TYPE"
log "Cellular IP: $CELLULAR_IP"
log "Proxy: ${PROXY_HOST:-None}"

# ==============================================================================
# STEP 2: Setup Cellular Interfaces
# ==============================================================================
log "STEP 2: Setting up cellular interfaces..."

# Rename eth0 to cellular0 (hide ethernet appearance)
ip link set eth0 down 2>/dev/null
ip link set eth0 name cellular0 2>/dev/null
ip link set cellular0 up 2>/dev/null
log "Renamed eth0 -> cellular0"

# Rename wlan0 to rmnet0 (mobile data interface naming)
ip link set wlan0 down 2>/dev/null
ip link set wlan0 name rmnet0 2>/dev/null
ip link set rmnet0 up 2>/dev/null
log "Renamed wlan0 -> rmnet0"

# Configure cellular IP address
ip addr add ${CELLULAR_IP}/24 dev rmnet0 broadcast + 2>/dev/null
ip addr add ${CELLULAR_IP}/24 dev rmnet0 2>/dev/null || true
log "Configured IP: ${CELLULAR_IP}/24 on rmnet0"

# Set up routing
ip route del default 2>/dev/null
ip route add default via ${GATEWAY} dev rmnet0 2>/dev/null || true
ip route add default via ${GATEWAY} dev rmnet0 2>/dev/null
log "Set default route via ${GATEWAY}"

# ==============================================================================
# STEP 3: Configure DNS
# ==============================================================================
log "STEP 3: Configuring DNS servers..."

# Set DNS properties
setprop net.dns1 "8.8.8.8"
setprop net.dns2 "1.1.1.1"
setprop net.dns3 "8.8.4.4"
setprop persist.net.dns1 "8.8.8.8"
setprop persist.net.dns2 "1.1.1.1"

# Mount system as writable for resolv.conf
mount -o rw,remount /system 2>/dev/null

# Update resolv.conf
cat > /system/etc/resolv.conf << 'RESOLVCONF'
nameserver 8.8.8.8
nameserver 1.1.1.1
nameserver 8.8.4.4
RESOLVCONF
chmod 644 /system/etc/resolv.conf
log "DNS configured: 8.8.8.8, 1.1.1.1"

# ==============================================================================
# STEP 4: Inject Telephony Properties
# ==============================================================================
log "STEP 4: Injecting telephony properties..."

# GSM Operator Properties
setprop gsm.sim.operator.alpha "${CARRIER_NAME}"
setprop gsm.operator.alpha "${CARRIER_NAME}"
setprop gsm.sim.operator.numeric "${MCC}${MNC}"
setprop gsm.operator.numeric "${MCC}${MNC}"

# SIM State - Set to READY (5)
setprop gsm.sim.state "5"
setprop ril.sim.state "READY"
setprop gsm.sim.present "true"

# Network Selection
setprop ro.setupwizard.mode "OPTIONAL"
setprop telephony.lteOnCdmaDevice "0"
setprop telephony.lteOnGsmDevice "1"

# Preferred Network Type
case "${NETWORK_TYPE}" in
    "5G")
        setprop persist.radio.network.mode "20"
        setprop ro.telephony.default_network "20"
        setprop gsm.network.type "5G"
        ;;
    "LTE")
        setprop persist.radio.network.mode "9"
        setprop ro.telephony.default_network "9"
        setprop gsm.network.type "LTE"
        ;;
    "WCDMA"|"HSDPA")
        setprop persist.radio.network.mode "3"
        setprop ro.telephony.default_network "3"
        setprop gsm.network.type "HSDPA"
        ;;
    "EDGE"|"GSM")
        setprop persist.radio.network.mode "1"
        setprop ro.telephony.default_network "1"
        setprop gsm.network.type "EDGE"
        ;;
esac

# Data Roaming Settings
setprop persist.data.roaming "false"
setprop ro.com.google.clientidbase "android-google"
setprop persist.radio.mobile.data "true"

# Cell Information
CELL_ID=$((RANDOM % 65535))
LAC=$((RANDOM % 65535))
setprop gsm.cell.id "${CELL_ID}"
setprop gsm.cell.location "${LAC}"
setprop gsm.signal.strength "$((RANDOM % 30 - 85))"

# Country Settings
setprop ro.product.locale.region "${COUNTRY_CODE}"
setprop persist.sys.country "${COUNTRY_CODE}"
setprop ro.cust.telephony "${COUNTRY_CODE}"

log "Telephony properties injected successfully"

# ==============================================================================
# STEP 5: Configure Timezone
# ==============================================================================
log "STEP 5: Configuring timezone..."

setprop persist.sys.timezone "${TIMEZONE}"
setprop persist.sys.timezone_manuel "true"

# Update timezone configuration
mkdir -p /data/property
echo "${TIMEZONE}" > /data/property/persist.sys.timezone

log "Timezone set to: ${TIMEZONE}"

# ==============================================================================
# STEP 6: Configure GPS/Location Spoofing
# ==============================================================================
log "STEP 6: Configuring GPS spoofing..."

setprop mock.location.enabled "false"
setprop persist.sys.gps.lat "${GPS_LAT}"
setprop persist.sys.gps.lon "${GPS_LON}"
setprop persist.sys.gps.alt "10.0"
setprop persist.sys.gps.acc "5.0"

log "GPS coordinates set: ${GPS_LAT}, ${GPS_LON}"

# ==============================================================================
# STEP 7: Block IPv6
# ==============================================================================
log "STEP 7: Blocking IPv6..."

sysctl -w net.ipv6.conf.all.disable_ipv6=1
sysctl -w net.ipv6.conf.default.disable_ipv6=1
sysctl -w net.ipv6.conf.lo.disable_ipv6=1
sysctl -w net.ipv6.conf.rmnet0.disable_ipv6=1
sysctl -w net.ipv6.conf.cellular0.disable_ipv6=1

# Block IPv6 in iptables
iptables -A OUTPUT -p ipv6 -j DROP 2>/dev/null || true
ip6tables -A OUTPUT -p ipv6 -j DROP 2>/dev/null || true

log "IPv6 blocked successfully"

# ==============================================================================
# STEP 8: Configure Proxy Routing (if proxy is specified)
# ==============================================================================
if [ -n "${PROXY_HOST}" ] && [ -n "${PROXY_PORT}" ]; then
    log "STEP 8: Configuring proxy routing..."
    
    # Create redsocks configuration
    cat > /data/local/tmp/redsocks.conf << 'REDSOCKSCONFIG'
base {
    log_debug = on;
    log_info = on;
    log = "file:/data/local/tmp/redsocks.log";
    daemon = on;
    redirector = iptables;
}

redsocks {
    local_ip = 127.0.0.1;
    local_port = 8123;
REDSOCKSCONFIG

    # Add proxy details
    echo "    ip = ${PROXY_HOST};" >> /data/local/tmp/redsocks.conf
    echo "    port = ${PROXY_PORT};" >> /data/local/tmp/redsocks.conf
    
    if [ -n "${PROXY_USER}" ] && [ -n "${PROXY_PASS}" ]; then
        echo "    login = \"${PROXY_USER}\";" >> /data/local/tmp/redsocks.conf
        echo "    password = \"${PROXY_PASS}\";" >> /data/local/tmp/redsocks.conf
    fi
    
    if [ "${PROXY_TYPE}" = "socks5" ]; then
        echo "    type = socks5;" >> /data/local/tmp/redsocks.conf
    else
        echo "    type = http-connect;" >> /data/local/tmp/redsocks.conf
    fi
    
    echo "}" >> /data/local/tmp/redsocks.conf
    echo "" >> /data/local/tmp/redsocks.conf
    
    # DNS tunnel configuration
    cat >> /data/local/tmp/redsocks.conf << 'DNSTUNNEL'
dnstc {
    local_ip = 127.0.0.1;
    local_port = 5300;
    tunnel_ip = 8.8.8.8;
    tunnel_port = 53;
    is_udp = on;
    timeout = 30;
}
DNSTUNNEL

    log "RedSocks configuration created"

    # Set up iptables for proxy
    iptables -t nat -N REDSOCKS 2>/dev/null || true
    iptables -t nat -F REDSOCKS
    
    # Bypass local networks
    iptables -t nat -A REDSOCKS -d 0.0.0.0/8 -j RETURN
    iptables -t nat -A REDSOCKS -d 10.0.0.0/8 -j RETURN
    iptables -t nat -A REDSOCKS -d 127.0.0.0/8 -j RETURN
    iptables -t nat -A REDSOCKS -d 172.16.0.0/12 -j RETURN
    iptables -t nat -A REDSOCKS -d 192.168.0.0/16 -j RETURN
    
    # Redirect TCP through redsocks
    iptables -t nat -A REDSOCKS -p tcp -j REDIRECT --to-ports 8123
    
    # Apply to OUTPUT chain
    iptables -t nat -A OUTPUT -p tcp -j REDSOCKS 2>/dev/null || true
    
    log "Proxy routing configured"
fi

# ==============================================================================
# STEP 9: DNS Leak Prevention
# ==============================================================================
log "STEP 9: Configuring DNS leak prevention..."

# Block STUN (used for WebRTC IP detection)
iptables -A OUTPUT -p udp --dport 19302 -j DROP 2>/dev/null || true
iptables -A OUTPUT -p udp --dport 3478 -j DROP 2>/dev/null || true

# Additional leak prevention
iptables -A OUTPUT -p udp --dport 53 -d 192.168.0.0/16 -j ACCEPT 2>/dev/null || true
iptables -A OUTPUT -p udp --dport 53 -d 10.0.0.0/8 -j ACCEPT 2>/dev/null || true
iptables -A OUTPUT -p udp --dport 53 -j DROP 2>/dev/null || true

log "DNS leak prevention configured"

# ==============================================================================
# STEP 10: WebRTC Protection
# ==============================================================================
log "STEP 10: Configuring WebRTC protection..."

setprop net.rWbcmLe.localip "${CELLULAR_IP}"
setprop net.rWbcmLe.enable "0"
setprop persist.rWbcmLe.interface "rmnet0"

log "WebRTC protection enabled"

# ==============================================================================
# STEP 11: Configure Network Transport Type
# ==============================================================================
log "STEP 11: Configuring network transport..."

# Make ConnectivityManager report TRANSPORT_CELLULAR
setprop sys.connectivity_service "telephony"
setprop ro.telephony.sip_can_make_voice_call "true"

# Override network type detection
setprop persist.radio.force_gsm_mode "false"

# Mobile data enabled
svc data enable 2>/dev/null || true

log "Network transport configured as TRANSPORT_CELLULAR"

# ==============================================================================
# STEP 12: Cleanup and Finalization
# ==============================================================================
log "STEP 12: Final cleanup..."

# Clear logcat buffer to hide initialization
logcat -c 2>/dev/null

# Set boot completed
setprop sys.boot_completed "1"
setprop ctl.restart adbd

# Remove temporary files
rm -f /data/local/tmp/setup_*.sh 2>/dev/null

log "========================================="
log "Cellular Network Initialization Complete"
log "========================================="
log "Carrier: ${CARRIER_NAME}"
log "Network: ${NETWORK_TYPE}"
log "IP: ${CELLULAR_IP}"
log "MCC/MNC: ${MCC}${MNC}"
log "Timezone: ${TIMEZONE}"
log "========================================="

# Final status
exit 0
