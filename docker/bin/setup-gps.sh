#!/system/bin/sh
# ==============================================================================
# VirtualPhonePro - GPS Setup Script
# Configure GPS location settings
# ==============================================================================

set -e

LOG_FILE="/data/logs/gps.log"
CONFIG_DIR="/opt/vpp/config"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# ==============================================================================
# Enable Location Services
# ==============================================================================
enable_location() {
    log "Enabling location services..."
    
    # Enable location
    settings put secure location_mode 3 2>/dev/null || true
    settings put secure location_providers_allowed "gps,network" 2>/dev/null || true
    
    # GPS enabled
    setprop gps.enabled "true"
    setprop location.mode "high_accuracy"
    
    log "Location services enabled"
}

# ==============================================================================
# Set GPS Location
# ==============================================================================
set_gps_location() {
    log "Setting GPS location..."
    
    local latitude="${VPP_GPS_LAT:-37.7749}"
    local longitude="${VPP_GPS_LON:--122.4194}"
    local altitude="${VPP_GPS_ALT:-10.0}"
    local accuracy="${VPP_GPS_ACCURACY:-5.0}"
    local speed="${VPP_GPS_SPEED:-0.0}"
    local bearing="${VPP_GPS_BEARING:-0.0}"
    local time="${VPP_GPS_TIME:-$(date +%s)}"
    
    # Format: latitude,longitude,altitude,accuracy,speed,bearing,time,provider
    local location="${latitude},${longitude},${altitude},${accuracy},${speed},${bearing},${time},gps"
    
    # Set location via setprop (for mock location apps)
    setprop persist.mock_location "1"
    setprop ctl.mock_location "1"
    
    # Store in our custom location service
    echo "$location" > /data/device/location.txt
    
    log "GPS Location set: ${latitude}, ${longitude}"
    log "Altitude: ${altitude}m, Accuracy: ${accuracy}m"
}

# ==============================================================================
# Configure GPS Settings
# ==============================================================================
configure_gps() {
    log "Configuring GPS settings..."
    
    # GPS NMEA output settings
    setprop persist.gps.nmea_interval "1000"
    setprop persist.gps.use_location_provider "true"
    
    # AGPS settings
    setprop persist.gps.agps_mode "SUPL"
    setprop persist.gps.agps_server "1"  # Google's AGPS server
    
    # Enable various satellite systems
    setprop persist.gps.glonass "true"
    setprop persist.gps.beidou "true"
    setprop persist.gps.galileo "true"
    setprop persist.gps.qzss "true"
    
    log "GPS settings configured"
}

# ==============================================================================
# Setup Mock Location App (requires location spoofing app)
# ==============================================================================
setup_mock_location() {
    log "Setting up mock location mode..."
    
    # Enable mock location for development
    setprop persist.mock_location "1"
    
    # Allow mock locations
    settings put secure allow_mock_location "1" 2>/dev/null || true
    
    # Note: Actual GPS spoofing requires an app like "Fake GPS Location"
    # or Magisk module "MagiskHide Props Config"
    log "Mock location enabled"
    log "NOTE: Install a GPS spoofing app for actual location change"
}

# ==============================================================================
# Set Timezone Based on Location
# ==============================================================================
set_timezone() {
    log "Setting timezone based on location..."
    
    local tz="${VPP_TIMEZONE:-America/Los_Angeles}"
    
    # Set timezone
    setprop persist.sys.timezone "$tz"
    
    log "Timezone set: $tz"
}

# ==============================================================================
# Common Locations Database
# ==============================================================================
get_location_by_city() {
    local city="$1"
    
    case "$city" in
        "new_york"|"nyc")
            echo "40.7128,-74.0060,10.0"
            ;;
        "los_angeles"|"la")
            echo "34.0522,-118.2437,10.0"
            ;;
        "london")
            echo "51.5074,-0.1278,10.0"
            ;;
        "tokyo")
            echo "35.6762,139.6503,10.0"
            ;;
        "dubai")
            echo "25.2048,55.2708,10.0"
            ;;
        "singapore")
            echo "1.3521,103.8198,10.0"
            ;;
        "dhaka")
            echo "23.8103,90.4125,10.0"
            ;;
        *)
            echo ""  # Unknown city
            ;;
    esac
}

# ==============================================================================
# Main
# ==============================================================================
main() {
    log "=========================================="
    log "VirtualPhonePro - GPS Setup"
    log "=========================================="
    
    enable_location
    set_gps_location
    configure_gps
    setup_mock_location
    set_timezone
    
    log "=========================================="
    log "GPS setup completed!"
    log "=========================================="
}

main "$@"
