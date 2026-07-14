#!/bin/bash
# ==============================================================================
# RedroidCPP - Device Management Script
# Manage Android emulator containers
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
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DATA_DIR="${PROJECT_ROOT}/data"
PROFILES_DIR="${PROJECT_ROOT}/profiles"
CONTAINER_NAME="redroid-emulator"

# Default values
DEVICE_NAME=""
DEVICE_MANUFACTURER="Samsung"
DEVICE_MODEL="Galaxy S24 Ultra"
DEVICE_ANDROID_VERSION="15"
DEVICE_MEMORY="4096"

# Logging
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

# Check prerequisites
check_prereqs() {
    log_info "Checking prerequisites..."
    
    # Check Docker
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed"
        exit 1
    fi
    
    # Check Docker Compose
    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        log_warn "Docker Compose is not installed"
    fi
    
    # Check KVM
    if [ ! -c /dev/kvm ]; then
        log_warn "KVM device not found (/dev/kvm)"
        log_warn "Hardware acceleration may not be available"
    else
        log_success "KVM available"
    fi
    
    log_success "Prerequisites checked"
}

# Initialize directories
init_dirs() {
    mkdir -p "$DATA_DIR"
    mkdir -p "$PROFILES_DIR"
    mkdir -p "${DATA_DIR}/profiles"
    mkdir -p "${DATA_DIR}/logs"
}

# Build Docker image
build_image() {
    log_info "Building Docker image..."
    
    cd "$PROJECT_ROOT"
    
    if command -v docker-compose &> /dev/null; then
        docker-compose build redroid
    else
        docker build -t redroid-cpp/emulator:latest .
    fi
    
    log_success "Docker image built"
}

# Generate device profile
generate_profile() {
    local manufacturer="$1"
    local model="$2"
    local android_version="$3"
    local profile_name="$4"
    
    log_info "Generating device profile..."
    log_info "  Manufacturer: $manufacturer"
    log_info "  Model: $model"
    log_info "  Android: $android_version"
    
    # Use the C++ CLI to generate profile
    if [ -f "${PROJECT_ROOT}/redroid-cli" ]; then
        "${PROJECT_ROOT}/redroid-cli" profile -m "$manufacturer" -a "$android_version" > "${PROFILES_DIR}/${profile_name}.json"
        log_success "Profile saved to ${PROFILES_DIR}/${profile_name}.json"
    else
        log_warn "CLI tool not found, creating basic profile..."
        cat > "${PROFILES_DIR}/${profile_name}.json" << EOF
{
    "manufacturer": "$manufacturer",
    "model": "$model",
    "android_version": "$android_version",
    "generated_at": "$(date -Iseconds)"
}
EOF
    fi
}

# Start emulator
start_emulator() {
    local name="${1:-$DEVICE_NAME}"
    local manufacturer="${2:-$DEVICE_MANUFACTURER}"
    local model="${3:-$DEVICE_MODEL}"
    local android_version="${4:-$DEVICE_ANDROID_VERSION}"
    
    log_info "Starting emulator..."
    log_info "  Name: ${name:-default}"
    log_info "  Device: $manufacturer $model"
    
    # Check if already running
    if docker ps | grep -q "${CONTAINER_NAME}"; then
        log_warn "Emulator is already running"
        return 0
    fi
    
    # Stop if exists but stopped
    if docker ps -a | grep -q "${CONTAINER_NAME}"; then
        log_info "Removing existing container..."
        docker rm -f "${CONTAINER_NAME}" > /dev/null 2>&1
    fi
    
    # Start with docker-compose
    cd "$PROJECT_ROOT"
    
    # Create profile if needed
    if [ -z "$name" ]; then
        name="device-$(date +%s)"
    fi
    
    DEVICE_PROFILE_ID="$name" \
    DEVICE_MANUFACTURER="$manufacturer" \
    DEVICE_MODEL="$model" \
    DEVICE_ANDROID_VERSION="$android_version" \
    docker-compose up -d redroid
    
    # Wait for startup
    log_info "Waiting for emulator to start..."
    sleep 10
    
    # Check status
    if docker ps | grep -q "${CONTAINER_NAME}"; then
        log_success "Emulator started successfully"
        echo ""
        echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
        echo "  Container: ${CONTAINER_NAME}"
        echo "  ADB Port:  15555"
        echo "  VNC Port:  5900 (optional)"
        echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
        echo ""
        echo "Connect with: adb connect localhost:15555"
    else
        log_error "Failed to start emulator"
        docker logs "${CONTAINER_NAME}" | tail -20
        exit 1
    fi
}

# Stop emulator
stop_emulator() {
    log_info "Stopping emulator..."
    
    docker stop "${CONTAINER_NAME}" 2>/dev/null || true
    docker rm "${CONTAINER_NAME}" 2>/dev/null || true
    
    log_success "Emulator stopped"
}

# Restart emulator
restart_emulator() {
    log_info "Restarting emulator..."
    stop_emulator
    sleep 2
    start_emulator "$@"
}

# Show emulator status
status_emulator() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
    echo "         EMULATOR STATUS"
    echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
    echo ""
    
    if docker ps | grep -q "${CONTAINER_NAME}"; then
        echo -e "${GREEN}Status:${NC} Running"
        
        # Get container info
        local ip=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "${CONTAINER_NAME}" 2>/dev/null || echo "N/A")
        local image=$(docker inspect -f '{{.Config.Image}}' "${CONTAINER_NAME}" 2>/dev/null || echo "N/A")
        local started=$(docker inspect -f '{{.State.StartedAt}}' "${CONTAINER_NAME}" 2>/dev/null || echo "N/A")
        
        echo -e "${YELLOW}Container:${NC} ${CONTAINER_NAME}"
        echo -e "${YELLOW}Image:${NC} ${image}"
        echo -e "${YELLOW}IP:${NC} ${ip}"
        echo -e "${YELLOW}Started:${NC} ${started}"
        echo ""
        
        # Try to get device info
        echo -e "${YELLOW}Device Info (via ADB):${NC}"
        adb connect localhost:15555 2>/dev/null || true
        adb -s localhost:15555 shell getprop ro.product.manufacturer 2>/dev/null | head -1 | xargs -I{} echo "  Manufacturer: {}"
        adb -s localhost:15555 shell getprop ro.product.model 2>/dev/null | head -1 | xargs -I{} echo "  Model: {}"
        adb -s localhost:15555 shell getprop ro.build.version.release 2>/dev/null | head -1 | xargs -I{} echo "  Android: {}"
        
    else
        echo -e "${RED}Status:${NC} Stopped"
    fi
    
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
}

# List profiles
list_profiles() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
    echo "         AVAILABLE PROFILES"
    echo -e "${CYAN}═══════════════════════════════════════════════${NC}"
    echo ""
    
    if [ -d "$PROFILES_DIR" ]; then
        local count=$(find "$PROFILES_DIR" -name "*.json" -o -name "*.sh" 2>/dev/null | wc -l)
        if [ "$count" -gt 0 ]; then
            find "$PROFILES_DIR" -name "*.json" -o -name "*.sh" 2>/dev/null | while read -r profile; do
                local name=$(basename "$profile")
                local size=$(stat -f%z "$profile" 2>/dev/null || stat -c%s "$profile" 2>/dev/null || echo "0")
                echo "  📱 $name ($size bytes)"
            done
        else
            echo "  No profiles found"
        fi
    else
        echo "  Profiles directory not created"
    fi
    
    echo ""
}

# Provision device
provision_device() {
    local profile="$1"
    
    if [ -z "$profile" ]; then
        log_error "Profile name required"
        exit 1
    fi
    
    log_info "Provisioning device with profile: $profile"
    
    # Copy profile to container
    docker cp "${PROFILES_DIR}/${profile}.sh" "${CONTAINER_NAME}:/opt/profile.sh" 2>/dev/null || {
        log_error "Profile not found: ${PROFILES_DIR}/${profile}.sh"
        exit 1
    }
    
    # Run provisioning script
    docker exec "${CONTAINER_NAME}" /opt/redroid-init.sh load "$profile"
    
    log_success "Device provisioned"
}

# Shell access
shell_access() {
    log_info "Opening shell in container..."
    docker exec -it "${CONTAINER_NAME}" /system/bin/sh
}

# Show logs
show_logs() {
    local lines="${1:-50}"
    log_info "Showing last $lines lines of logs..."
    docker logs --tail "$lines" "${CONTAINER_NAME}"
}

# ADB connect
adb_connect() {
    log_info "Connecting to emulator via ADB..."
    adb connect localhost:15555
    
    if adb devices | grep -q "localhost:15555"; then
        log_success "Connected to emulator"
    else
        log_error "Failed to connect"
    fi
}

# Print help
print_help() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║          RedroidCPP - Device Management Tool v3.0.0           ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Usage: ./manage.sh <command> [options]"
    echo ""
    echo -e "${YELLOW}Commands:${NC}"
    echo "  start [options]        Start emulator"
    echo "  stop                   Stop emulator"
    echo "  restart [options]     Restart emulator"
    echo "  status                 Show emulator status"
    echo "  logs [lines]           Show container logs"
    echo "  shell                  Shell access to container"
    echo "  profiles               List available profiles"
    echo "  provision <profile>    Apply device profile"
    echo "  connect                Connect via ADB"
    echo "  build                  Build Docker image"
    echo "  help                   Show this help"
    echo ""
    echo -e "${YELLOW}Start Options:${NC}"
    echo "  -n, --name <name>       Device name"
    echo "  -m, --manufacturer <m> Manufacturer"
    echo "  -d, --model <model>     Model name"
    echo "  -a, --android <ver>    Android version"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo "  ./manage.sh start -m Samsung -d 'Galaxy S24 Ultra' -a 15"
    echo "  ./manage.sh start -n my-device -m Google -d 'Pixel 8'"
    echo "  ./manage.sh status"
    echo "  ./manage.sh provision samsung-galaxy-s24"
    echo ""
}

# Parse arguments
parse_args() {
    case "${1:-help}" in
        start)
            shift
            # Parse start options
            while [[ $# -gt 0 ]]; do
                case $1 in
                    -n|--name)
                        DEVICE_NAME="$2"
                        shift 2
                        ;;
                    -m|--manufacturer)
                        DEVICE_MANUFACTURER="$2"
                        shift 2
                        ;;
                    -d|--model)
                        DEVICE_MODEL="$2"
                        shift 2
                        ;;
                    -a|--android)
                        DEVICE_ANDROID_VERSION="$2"
                        shift 2
                        ;;
                    *)
                        shift
                        ;;
                esac
            done
            start_emulator "$DEVICE_NAME" "$DEVICE_MANUFACTURER" "$DEVICE_MODEL" "$DEVICE_ANDROID_VERSION"
            ;;
        stop)
            stop_emulator
            ;;
        restart)
            shift
            while [[ $# -gt 0 ]]; do
                case $1 in
                    -n|--name)
                        DEVICE_NAME="$2"
                        shift 2
                        ;;
                    -m|--manufacturer)
                        DEVICE_MANUFACTURER="$2"
                        shift 2
                        ;;
                    -d|--model)
                        DEVICE_MODEL="$2"
                        shift 2
                        ;;
                    -a|--android)
                        DEVICE_ANDROID_VERSION="$2"
                        shift 2
                        ;;
                    *)
                        shift
                        ;;
                esac
            done
            restart_emulator "$DEVICE_NAME" "$DEVICE_MANUFACTURER" "$DEVICE_MODEL" "$DEVICE_ANDROID_VERSION"
            ;;
        status)
            status_emulator
            ;;
        logs)
            show_logs "${2:-50}"
            ;;
        shell)
            shell_access
            ;;
        profiles)
            list_profiles
            ;;
        provision)
            provision_device "$2"
            ;;
        connect)
            adb_connect
            ;;
        build)
            build_image
            ;;
        help|--help|-h)
            print_help
            ;;
        *)
            log_error "Unknown command: $1"
            print_help
            exit 1
            ;;
    esac
}

# Main
main() {
    check_prereqs
    init_dirs
    parse_args "$@"
}

main "$@"
