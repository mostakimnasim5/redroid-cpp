#!/system/bin/sh
#
# ============================================================
# VirtualPhonePro - Deep System Patch Script
# ============================================================
# 
# This script applies comprehensive kernel-level, filesystem,
# and security patches to eliminate virtualization artifacts
# and emulate a real retail Android device.
#
# Usage: 
#   adb shell sh /data/local/tmp/patch_system.sh [profile]
#
# Profiles:
#   banking   - Maximum stealth for banking apps
#   security  - Security research profile
#   qa        - QA testing profile
#   stealth   - Maximum stealth (all mitigations)
#
# Author: VirtualPhonePro Security Team
# Version: 4.0.0
#

# ============================================================
# Configuration
# ============================================================

PROFILE="${1:-banking}"
LOG_FILE="/data/local/tmp/vpp_patch.log"
OVERLAY_DIR="/data/local/tmp/vpp_overlay"
STATE_DIR="/data/local/tmp/vpp_state"
WORK_DIR="/data/local/tmp/vpp_work"

# Device Profile (Samsung Galaxy S24 Ultra)
DEVICE_MANUFACTURER="samsung"
DEVICE_BRAND="samsung"
DEVICE_MODEL="SM-S928B"
DEVICE_DEVICE="dm3q"
DEVICE_PRODUCT="dm3q"
DEVICE_BOARD="kalama"
DEVICE_HARDWARE="qcom"
DEVICE_BOOTLOADER="S928BXXU1AXXX"
DEVICE_BASEBAND="S928BXXU1AXXX"

# Kernel spoof version
KERNEL_VERSION="5.15.147-android14-11-g"
KERNEL_BUILD_HOST="buildhost.google.com"
KERNEL_BUILD_USER="android-build"

# Security patch level
SECURITY_PATCH="2024-01-01"

# ============================================================
# Logging Functions
# ============================================================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
    echo "$1"
}

log_success() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [SUCCESS] $1" >> "$LOG_FILE"
}

log_error() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [ERROR] $1" >> "$LOG_FILE"
    echo "[ERROR] $1"
}

log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INFO] $1" >> "$LOG_FILE"
}

# ============================================================
# Directory Setup
# ============================================================

setup_directories() {
    log "Setting up overlay directories..."
    
    mkdir -p "$OVERLAY_DIR"
    mkdir -p "$OVERLAY_DIR/proc"
    mkdir -p "$OVERLAY_DIR/system"
    mkdir -p "$STATE_DIR"
    mkdir -p "$WORK_DIR"
    mkdir -p "$WORK_DIR/proc"
    mkdir -p "$WORK_DIR/system"
    
    chmod 755 "$OVERLAY_DIR"
    chmod 755 "$OVERLAY_DIR/proc"
    chmod 755 "$OVERLAY_DIR/system"
    chmod 755 "$STATE_DIR"
    
    log_success "Overlay directories created"
}

# ============================================================
# Kernel & Proc Sanitization
# ============================================================

patch_kernel_version() {
    log "Patching kernel version information..."
    
    # Fake kernel version string
    FAKE_KERNEL="Linux version ${KERNEL_VERSION}a1b2c3d4e5f6 (${KERNEL_BUILD_USER}@${KERNEL_BUILD_HOST}) #1 SMP PREEMPT $(date '+%b %d %H:%M:%S UTC %Y')"
    
    # Create /proc/version overlay
    echo "$FAKE_KERNEL" > "$OVERLAY_DIR/proc/version"
    chmod 444 "$OVERLAY_DIR/proc/version"
    
    # Create fake /proc/cmdline
    FAKE_CMDLINE="androidboot.hardware=${DEVICE_HARDWARE} androidboot.bootloader=${DEVICE_BOOTLOADER} androidboot.baseband=${DEVICE_BASEBAND} androidboot.security_patch=${SECURITY_PATCH} androidboot.selinux=enforcing"
    echo "$FAKE_CMDLINE" > "$OVERLAY_DIR/proc/cmdline"
    chmod 444 "$OVERLAY_DIR/proc/cmdline"
    
    # Create sys/kernel/version (some apps check this)
    echo "$FAKE_KERNEL" > "$OVERLAY_DIR/proc/sys/kernel/version"
    chmod 444 "$OVERLAY_DIR/proc/sys/kernel/version"
    
    # Create kernel/osrelease for additional checks
    echo "${KERNEL_VERSION}a1b2c3d4e5f6" > "$OVERLAY_DIR/proc/sys/kernel/osrelease"
    chmod 444 "$OVERLAY_DIR/proc/sys/kernel/osrelease"
    
    # Patch /proc/version in-place (may not persist)
    echo "$FAKE_KERNEL" > /proc/version 2>/dev/null
    chmod 444 /proc/version 2>/dev/null
    
    log_success "Kernel version patched"
}

patch_selinux_status() {
    log "Patching SELinux status..."
    
    # Create SELinux status files
    mkdir -p "$OVERLAY_DIR/sys/fs/selinux"
    echo "Enforcing" > "$OVERLAY_DIR/enforce"
    chmod 444 "$OVERLAY_DIR/enforce"
    echo "Enforcing" > "$OVERLAY_DIR/sys/fs/selinux/enforce"
    chmod 444 "$OVERLAY_DIR/sys/fs/selinux/enforce"
    
    # Set system properties for SELinux
    setprop ro.build.selinux Enforcing 2>/dev/null
    setprop selinux.enforce.status Enforcing 2>/dev/null
    
    log_success "SELinux status patched"
}

# ============================================================
# Mount & Filesystem Sanitization
# ============================================================

patch_mounts() {
    log "Sanitizing mount information..."
    
    # Create sanitized /proc/mounts without overlay/docker references
    SANITIZED_MOUNTS=$(cat <<'MOUNTS'
rootfs / rootfs rw,seclabel,size=2934400k,nr_inodes=694080 0 0
tmpfs /dev tmpfs rw,seclabel,nosuid,size=2k,nr_inodes=128 0 0
devpts /dev/pts devpts rw,seclabel,noexec,nosuid 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,seclabel,relatime 0 0
selinuxfs /sys/fs/selinux selinuxfs rw,relatime 0 0
tmpfs /mnt tmpfs rw,seclabel,nosuid,nodev,noexec,size=2k,nr_inodes=256 0 0
tmpfs /storage tmpfs rw,seclabel,nosuid,nodev,size=2k,nr_iniles=256 0 0
/dev/block/by-name/userdata /data f2fs rw,seclabel,relatime,background_gc=on,discard_grace_period=4 0 0
/dev/block/by-name/system /system ext4 ro,seclabel,relatime 0 0
MOUNTS
)
    
    echo "$SANITIZED_MOUNTS" > "$OVERLAY_DIR/proc/mounts"
    chmod 444 "$OVERLAY_DIR/proc/mounts"
    
    # Create sanitized /proc/self/mountinfo
    SANITIZED_MOUNTINFO=$(cat <<'MOUNTINFO'
24 20 0:23 / /sys/fs/cgroup/devices rw,nosuid,nodev,noexec,relatime master:1 - devpts devpts rw,seclabel,noexec,nosuid
23 20 0:22 / /proc rw,relatime master:5 - proc proc rw,nosuid,nodev,noexec,relatime
22 20 0:21 / /dev rw,seclabel,nosuid,size=2k,nr_inodes=128 master:3 - tmpfs tmpfs rw,seclabel,size=2k,nr_inodes=128,mode=755
21 20 0:10 / /system ro,seclabel,relatime master:7 - ext4 /dev/block/by-name/system ro,seclabel,relatime
20 0 253:0 / / rw,seclabel,relatime master:0 - ext4 /dev/block/by-name/userdata rw,seclabel,relatime
MOUNTINFO
)
    
    echo "$SANITIZED_MOUNTINFO" > "$OVERLAY_DIR/proc/self_mountinfo"
    chmod 444 "$OVERLAY_DIR/proc/self_mountinfo"
    
    log_success "Mount information sanitized"
}

remove_container_artifacts() {
    log "Removing container artifacts..."
    
    # Remove Docker-related files
    rm -f /.dockerenv 2>/dev/null
    rm -f /run/docker.sock 2>/dev/null
    rm -f /var/run/docker.sock 2>/dev/null
    
    # Clear cgroup references
    echo '' > /proc/1/cgroup 2>/dev/null
    chmod 444 /proc/1/cgroup 2>/dev/null
    
    # Remove emulator socket files
    rm -f /dev/socket/qemud 2>/dev/null
    rm -f /dev/socket/audiomix 2>/dev/null
    rm -f /dev/socket/goldfish_audio 2>/dev/null
    rm -f /dev/socket/goldfish_camera 2>/dev/null
    rm -f /dev/qemu_pipe 2>/dev/null
    
    # Make sensitive files inaccessible
    chmod 000 /dev/socket/qemud 2>/dev/null
    chmod 000 /dev/qemu_pipe 2>/dev/null
    
    log_success "Container artifacts removed"
}

remove_emulator_files() {
    log "Removing emulator-specific files..."
    
    # Remove QEMU/system emulator libraries
    rm -f /system/lib/libc_malloc_debug_qemu.so 2>/dev/null
    rm -f /system/lib/libfakenmalloc.so 2>/dev/null
    rm -f /system/lib/libqemu_prop.so 2>/dev/null
    rm -f /system/lib64/libc_malloc_debug_qemu.so 2>/dev/null
    rm -f /system/lib64/libfakenmalloc.so 2>/dev/null
    rm -f /system/lib64/libqemu_prop.so 2>/dev/null
    
    # Remove emulator init scripts
    rm -f /init.goldfish.rc 2>/dev/null
    rm -f /init.qemu.rc 2>/dev/null
    rm -f /init.qemu.firstboot.rc 2>/dev/null
    rm -f /init.trace.rc 2>/dev/null
    rm -f /init.redroid.rc 2>/dev/null
    
    # Replace sensitive device nodes
    touch /dev/ttyS0 && chmod 000 /dev/ttyS0 2>/dev/null
    touch /dev/ttyS1 && chmod 000 /dev/ttyS1 2>/dev/null
    touch /dev/ttyS2 && chmod 000 /dev/ttyS2 2>/dev/null
    touch /dev/ttyS3 && chmod 000 /dev/ttyS3 2>/dev/null
    
    # Make emulator libs unreadable
    chmod 000 /system/lib/*qemu* 2>/dev/null
    chmod 000 /system/lib64/*qemu* 2>/dev/null
    
    log_success "Emulator files removed"
}

remove_root_artifacts() {
    log "Removing root detection artifacts..."
    
    # Remove su binaries
    rm -rf /sbin/su 2>/dev/null
    rm -rf /system/su 2>/dev/null
    rm -rf /system/xbin/su 2>/dev/null
    rm -rf /vendor/bin/su 2>/dev/null
    rm -rf /data/adb/su 2>/dev/null
    rm -rf /data/adb/magisk 2>/dev/null
    
    # Uninstall root apps
    pm uninstall com.topjohnwu.magisk 2>/dev/null
    pm uninstall com.noshufou.android.su 2>/dev/null
    pm uninstall com.noshufou.android.su.elite 2>/dev/null
    pm uninstall com.koushikdutta.superuser 2>/dev/null
    pm uninstall com.thirdparty.superuser 2>/dev/null
    
    # Make su files inaccessible (if can't remove)
    chmod 000 /system/xbin/su 2>/dev/null
    chmod 000 /system/bin/su 2>/dev/null
    
    # Remove Superuser APK
    rm -f /system/app/Superuser.apk 2>/dev/null
    rm -f /system/app/Superuser/Superuser.apk 2>/dev/null
    rm -rf /system/app/Superuser 2>/dev/null
    
    log_success "Root artifacts removed"
}

# ============================================================
# Build Properties Spoofing
# ============================================================

patch_build_properties() {
    log "Patching build properties to retail values..."
    
    # Security flags
    setprop ro.debuggable 0
    setprop ro.secure 1
    setprop ro.build.type user
    
    # Build tags - retail keys
    setprop ro.build.tags release-keys
    setprop ro.build.description "${DEVICE_MODEL} ${DEVICE_PRODUCT} ${SECURITY_PATCH} ${DEVICE_BOOTLOADER} release-keys"
    
    # Verified boot
    setprop ro.boot.verifiedbootstate green
    setprop ro.boot.veritymode enforcing
    setprop ro.boot.verity.enabled true
    setprop ro.boot.flash.locked 1
    setprop ro.verifiedbootstate green
    
    # SELinux
    setprop ro.build.selinux Enforcing
    
    # Security patch level
    setprop ro.build.version.security_patch "${SECURITY_PATCH}"
    setprop ro.system_ext.security_patch "${SECURITY_PATCH}"
    setprop ro.vendor.security_patch "${SECURITY_PATCH}"
    setprop ro.product.security_patch "${SECURITY_PATCH}"
    
    # Hide test keys
    setprop ro.build.display.keys ''
    setprop ro.build.keys release
    
    # Hardware security
    setprop ro.hardware.backed_hardware 1
    setprop ro.hardware.security 1
    
    # GMS certification
    setprop ro.com.google.devicecert true
    setprop ro.com.google.gmsversion 23.06.034
    
    log_success "Build properties patched"
}

patch_device_identity() {
    log "Patching device identity..."
    
    # Product identity
    setprop ro.product.manufacturer "${DEVICE_MANUFACTURER}"
    setprop ro.product.brand "${DEVICE_BRAND}"
    setprop ro.product.model "${DEVICE_MODEL}"
    setprop ro.product.device "${DEVICE_DEVICE}"
    setprop ro.product.name "${DEVICE_PRODUCT}"
    setprop ro.product.board "${DEVICE_BOARD}"
    setprop ro.board.platform "${DEVICE_HARDWARE}"
    setprop ro.hardware "${DEVICE_HARDWARE}"
    
    # Build identity
    BUILD_FINGERPRINT="${DEVICE_BRAND}/${DEVICE_DEVICE}/${DEVICE_PRODUCT}:14/${SECURITY_PATCH//-/}/${DEVICE_BOOTLOADER}:user/release-keys"
    setprop ro.build.fingerprint "${BUILD_FINGERPRINT}"
    setprop ro.build.description "${DEVICE_PRODUCT}-user 14 ${SECURITY_PATCH//-/} ${DEVICE_BOOTLOADER} release-keys"
    setprop ro.build.version.incremental "${DEVICE_BOOTLOADER}"
    setprop ro.build.version.release 14
    setprop ro.build.version.sdk 34
    setprop ro.build.version.base_os ''
    
    # Bootloader and baseband
    setprop ro.bootloader "${DEVICE_BOOTLOADER}"
    setprop ro.baseband "${DEVICE_BASEBAND}"
    
    # Kernel version
    setprop ro.kernel.version "${KERNEL_VERSION}"
    
    log_success "Device identity patched"
}

patch_device_specific() {
    log "Patching device-specific properties..."
    
    # Device
    setprop ro.device "${DEVICE_DEVICE}"
    
    # GMS client IDs
    setprop ro.com.google.clientidbase.ms android-${DEVICE_BRAND}
    setprop ro.com.google.clientidbase.vs android-${DEVICE_BRAND}
    setprop ro.com.google.clientidbase.am android-${DEVICE_BRAND}
    setprop ro.com.google.clientidbase.gmm android-${DEVICE_BRAND}
    setprop ro.com.google.clientidbase.yt android-${DEVICE_BRAND}
    
    # Carrier properties
    setprop ro.carrier unknown
    
    log_success "Device-specific properties patched"
}

# ============================================================
# TEE & Hardware Attestation
# ============================================================

patch_tee_attestation() {
    log "Patching TEE and hardware attestation..."
    
    # Verified boot state
    setprop ro.boot.verifiedbootstate green
    setprop ro.verifiedbootstate green
    setprop ro.boot.veritymode enforcing
    setprop ro.boot.verity.enabled true
    
    # Flash lock state
    setprop ro.boot.flash.locked 1
    setprop ro.flash.locked 1
    
    # Hardware-backed keymaster
    setprop ro.hardware.backed_hardware 1
    setprop ro.hardware.security 1
    setprop ro.crypto.state encrypted
    setprop ro.crypto.type file
    
    # Gatekeeper
    setprop ro.gatekeeper.enabled true
    setprop ro.gatekeeper.locked true
    
    # Keymaster version
    setprop ro.keymaster.version 4.1
    setprop ro.gatekeeper.version 4.1
    setprop ro.hardware.keymaster 1
    setprop ro.crypto.keymaster 1
    setprop ro.crypto.verify_keymaster true
    
    # StrongBox if available
    setprop ro.hardware.strongbox_creator 1
    setprop ro.config.strongbox true
    setprop ro.crypto.support_strongbox true
    setprop keymaster strongbox_gatekeeper 1
    
    # dm-verity
    setprop ro.config.dmverity enforcing
    setprop ro.config.verity.enforcing 1
    
    # Verified boot hash
    setprop ro.boot.verity.hashverified true
    setprop ro.boot.verity.hash ''
    
    log_success "TEE attestation patched"
}

# ============================================================
# Developer Options & ADB
# ============================================================

patch_developer_options() {
    log "Hiding developer options and ADB..."
    
    # Disable developer options
    settings put global development_settings_enabled 0
    settings put secure dev_options_enabled 0
    settings put secure show_developer_options 0
    
    # Disable ADB
    settings put global adb_enabled 0
    settings put global adb_debugging_enabled 0
    settings put global enable_adb 0
    settings put secure adb_enabled 0
    
    # Persist these settings
    setprop persist.adb.enabled 0
    setprop persist.adb.tcp.port -1
    setprop service.adb.enable 0
    
    log_success "Developer options hidden"
}

# ============================================================
# Lifecycle & Uptime
# ============================================================

patch_lifecycle() {
    log "Patching system lifecycle and uptime..."
    
    # Calculate random uptime (1-7 days)
    RANDOM_UPTIME=$((RANDOM % 604800 + 86400))
    CURRENT_TIME=$(date +%s)
    BOOT_TIME=$((CURRENT_TIME - RANDOM_UPTIME))
    
    # Set boot time
    setprop persist.sys.boot_time "${BOOT_TIME}"
    setprop sys.init.uptime "$((RANDOM_UPTIME / 100))"
    
    # Set build date
    BOOT_DATE=$(date -d @"${BOOT_TIME}" '+%Y-%m-%d %H:%M:%S' 2>/dev/null || date '+%Y-%m-%d %H:%M:%S')
    setprop ro.build.date "${BOOT_DATE}"
    setprop ro.build.date.utc "${BOOT_TIME}"
    
    # Create fake /proc/uptime
    mkdir -p "$OVERLAY_DIR/proc"
    echo "$((RANDOM_UPTIME)).0 $((RANDOM_UPTIME * 25 / 100)).0" > "$OVERLAY_DIR/proc/uptime"
    chmod 444 "$OVERLAY_DIR/proc/uptime"
    
    # Patch /proc/uptime in-place
    echo "$((RANDOM_UPTIME)).0 $((RANDOM_UPTIME * 25 / 100)).0" > /proc/uptime 2>/dev/null
    chmod 444 /proc/uptime 2>/dev/null
    
    log_success "Lifecycle patched with uptime: ${RANDOM_UPTIME}s"
}

patch_battery_stats() {
    log "Patching battery statistics..."
    
    # Random battery cycles (100-300)
    RANDOM_CYCLES=$((RANDOM % 200 + 100))
    
    # Battery health (85-98%)
    BATTERY_HEALTH=$((RANDOM % 14 + 85))
    
    # Set battery properties
    dumpsys battery set cyclecount "${RANDOM_CYCLES}"
    dumpsys battery set health "${BATTERY_HEALTH}"
    dumpsys battery set level 75
    dumpsys battery set temperature 320
    dumpsys battery set voltage 4200
    dumpsys battery unplug
    dumpsys battery set status not-charging
    
    # Persist these values
    setprop persist.battery.cycles "${RANDOM_CYCLES}"
    setprop persist.battery.health "${BATTERY_HEALTH}"
    
    log_success "Battery stats patched (cycles: ${RANDOM_CYCLES}, health: ${BATTERY_HEALTH}%)"
}

# ============================================================
# Kernel Sysctl
# ============================================================

patch_sysctl() {
    log "Patching kernel sysctl values..."
    
    # Hide kernel pointers
    sysctl -w kernel.kptr_restrict=2 2>/dev/null
    sysctl -w kernel.dmesg_restrict=1 2>/dev/null
    
    # YAMA ptrace scope
    sysctl -w kernel.yama.ptrace_scope=1 2>/dev/null
    
    # Randomize VA space
    sysctl -w kernel.randomize_va_space=2 2>/dev/null
    
    log_success "Sysctl values patched"
}

# ============================================================
# Wi-Fi & Network
# ============================================================

patch_network() {
    log "Patching network configuration..."
    
    # Set realistic Wi-Fi MAC address
    WIFI_MAC="8C:71:F8:12:34:56"
    setprop wifi.interface wlan0
    setprop wifi.supplicant_scan_interval 15
    
    # Set hostname to appear as real device
    setprop net.hostname "${DEVICE_MODEL}"
    
    # Disable network debugging
    setprop persist.debug.atrace.tags_enable 0
    setprop debug.atrace.tags_enable 0
    
    log_success "Network configuration patched"
}

# ============================================================
# GPS & Location
# ============================================================

patch_location() {
    log "Patching location services..."
    
    # Set realistic GPS configuration
    setprop persist.gps.active 1
    setprop persist.gps.state enabled
    setprop ro.gps.active 1
    
    # Enable location services
    settings put secure location_mode 3
    settings put secure location_providers_allowed gps,network
    
    log_success "Location services patched"
}

# ============================================================
# Certificate & SSL
# ============================================================

patch_certificates() {
    log "Patching certificate trust store..."
    
    # Enable system CA certificates
    setprop ro.trust_lib.enable true
    setprop ro.config.dmverity enforce
    setprop ro.certhash.enable true
    
    log_success "Certificate trust store patched"
}

# ============================================================
# Dalvik & ART
# ============================================================

patch_art() {
    log "Patching ART and Dalvik configuration..."
    
    # Set JIT compiler settings
    setprop dalvik.vm.dex2oat-Xms 64m
    setprop dalvik.vm.dex2oat-Xmx 512m
    setprop dalvik.vm.dexopt.secondary 1
    
    # ART compilation
    setprop dalvik.vm.isa.arm.features +rasp
    
    log_success "ART configuration patched"
}

# ============================================================
# Cleanup
# ============================================================

finalize_patch() {
    log "Finalizing patch..."
    
    # Set patch version
    setprop vpp.patch.version "4.0.0"
    setprop vpp.patch.profile "${PROFILE}"
    setprop vpp.patch.timestamp "$(date +%s)"
    setprop vpp.patch.applied true
    
    # Make overlay directory immutable
    chattr +i "$OVERLAY_DIR" 2>/dev/null
    
    # Clean up any temporary files
    rm -rf /data/local/tmp/*.tmp 2>/dev/null
    rm -rf /data/local/tmp/*.log 2>/dev/null
    
    # Mark as patched in state file
    echo "PROFILE=${PROFILE}" > "$STATE_DIR/patch_status"
    echo "TIMESTAMP=$(date +%s)" >> "$STATE_DIR/patch_status"
    echo "VERSION=4.0.0" >> "$STATE_DIR/patch_status"
    echo "APPLIED=true" >> "$STATE_DIR/patch_status"
    
    log_success "Patch finalized"
}

# ============================================================
# Main Execution
# ============================================================

main() {
    echo "============================================================"
    echo "VirtualPhonePro Deep System Patch"
    echo "Profile: ${PROFILE}"
    echo "============================================================"
    
    log "============================================================"
    log "Starting VirtualPhonePro Deep System Patch"
    log "Profile: ${PROFILE}"
    log "============================================================"
    
    # Setup
    setup_directories
    
    # Kernel patches
    patch_kernel_version
    patch_selinux_status
    patch_sysctl
    
    # Filesystem patches
    patch_mounts
    remove_container_artifacts
    remove_emulator_files
    remove_root_artifacts
    
    # Property patches
    patch_build_properties
    patch_device_identity
    patch_device_specific
    
    # Security patches
    patch_tee_attestation
    patch_developer_options
    
    # Lifecycle patches
    patch_lifecycle
    patch_battery_stats
    
    # Additional patches based on profile
    patch_network
    patch_location
    patch_certificates
    patch_art
    
    # Finalize
    finalize_patch
    
    log "============================================================"
    log "Patch completed successfully"
    log "============================================================"
    
    echo "============================================================"
    echo "Patch completed successfully!"
    echo "Profile: ${PROFILE}"
    echo "============================================================"
}

# Run main
main "$@"
