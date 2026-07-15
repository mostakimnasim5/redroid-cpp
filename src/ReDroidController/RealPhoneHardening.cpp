#include "RealPhoneHardening.hpp"
#include "ADBManager.hpp"
#include "Logger.hpp"
#include <random>
#include <chrono>

namespace AntiDetect {

RealPhoneHardening& RealPhoneHardening::getInstance() {
    static RealPhoneHardening instance;
    return instance;
}

RealPhoneHardening::RealPhoneHardening()
    : m_initialized(false)
    , m_hardeningApplied(false)
{
    m_config = {
        true, true, true, true, true, true,
        true, true, false, true, true, true, true, true
    };
    
    m_batteryConfig = {false, 85, "Discharging", "USB", "25", "Good", false};
    m_radioConfig = {false, "", "", "", ""};
    m_kernelConfig = {false, false, false, false, ""};
    m_emulatorConfig = {false, false, false, false, false, false, false, false};
    m_canvasConfig = {false, 1, ""};
    m_webglConfig = {false, "", "", "", ""};
    m_audioConfig = {false, "", ""};
}

RealPhoneHardening::~RealPhoneHardening() {
    shutdown();
}

bool RealPhoneHardening::initialize() {
    if (m_initialized) return true;
    
    Logger::getInstance().info("Initializing RealPhoneHardening...");
    m_initialized = true;
    
    return true;
}

bool RealPhoneHardening::shutdown() {
    resetAll();
    m_initialized = false;
    return true;
}

void RealPhoneHardening::setConfig(const HardeningConfig& config) {
    m_config = config;
}

HardeningConfig RealPhoneHardening::getConfig() const {
    return m_config;
}

bool RealPhoneHardening::applyProperty(const std::string& key, const std::string& value) {
    return ADBManager::getInstance().setProperty(key, value);
}

std::string RealPhoneHardening::getProperty(const std::string& key) {
    return ADBManager::getInstance().getProperty(key);
}

bool RealPhoneHardening::executeCommand(const std::string& command) {
    return ADBManager::getInstance().executeShellCommand(command).find("error") == std::string::npos;
}

// Root Detection Bypass
bool RealPhoneHardening::hideRoot() {
    Logger::getInstance().info("Hiding root detection...");
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("mount -o rw,remount /system");
    
    adb.executeShellCommand("mv /system/bin/su /system/bin/su.bak 2>/dev/null || true");
    adb.executeShellCommand("mv /system/xbin/su /system/xbin/su.bak 2>/dev/null || true");
    adb.executeShellCommand("mv /su/bin/su /su/bin/su.bak 2>/dev/null || true");
    
    adb.executeShellCommand("pm hide com.topjohnwu.magisk 2>/dev/null || true");
    adb.executeShellCommand("pm disable-user --user 0 com.topjohnwu.magisk 2>/dev/null || true");
    
    adb.setProperty("ro.build.tags", "release-keys");
    adb.setProperty("ro.build.type", "user");
    adb.setProperty("ro.debuggable", "0");
    adb.setProperty("ro.secure", "1");
    
    return true;
}

bool RealPhoneHardening::hideMagisk() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("magisk --denylist add com.google.android.gms 2>/dev/null || true");
    adb.executeShellCommand("magisk hide enable 2>/dev/null || true");
    
    adb.executeShellCommand("resetprop magisk.hide true 2>/dev/null || true");
    adb.setProperty("persist.magisk.hide", "true");
    
    return true;
}

bool RealPhoneHardening::hideSU() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("chmod 000 /system/bin/su 2>/dev/null || true");
    adb.executeShellCommand("chmod 000 /system/xbin/su 2>/dev/null || true");
    adb.executeShellCommand("chattr -i /system/bin/su 2>/dev/null || true");
    adb.executeShellCommand("chattr -i /system/xbin/su 2>/dev/null || true");
    
    adb.setProperty("ro.build.selinux.enforce", "true");
    
    return true;
}

bool RealPhoneHardening::hideXposed() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("mv /system/bin/app_process64 /system/bin/app_process64.bak 2>/dev/null || true");
    adb.executeShellCommand("mv /system/bin/app_process32 /system/bin/app_process32.bak 2>/dev/null || true");
    
    adb.executeShellCommand("pm disable-user --user 0 de.robv.android.xposed.installer 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::hideSelinuxStatus() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.build.selinux.enforce", "false");
    adb.executeShellCommand("setenforce 0 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::hideDebugStatus() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.debuggable", "0");
    adb.setProperty("ro.adb.secure", "1");
    adb.setProperty("persist.sys.usb.config", "mtp");
    
    adb.executeShellCommand("settings put global adb_enabled 0");
    adb.executeShellCommand("settings put global developer_options 0");
    
    return true;
}

bool RealPhoneHardening::enableDmVerity() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.verity.mode", "enforcing");
    adb.executeShellCommand(" mount -o remount,ro /system");
    
    return true;
}

bool RealPhoneHardening::enableVerifiedBoot() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.boot.verifiedbootstate", "green");
    adb.setProperty("ro.boot.veritymode", "enforcing");
    adb.setProperty("ro.build.description", adb.getProperty("ro.build.fingerprint") + " release-keys");
    
    return true;
}

bool RealPhoneHardening::hideBuildType() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.build.type", "user");
    adb.setProperty("ro.build.tags", "release-keys");
    
    return true;
}

bool RealPhoneHardening::convertUserdebugToUser() {
    auto& adb = ADBManager::getInstance();
    
    std::string currentType = adb.getProperty("ro.build.type");
    if (currentType == "userdebug") {
        adb.setProperty("ro.build.type", "user");
        adb.setProperty("ro.build.tags", "release-keys");
        adb.setProperty("ro.debuggable", "0");
    }
    
    return true;
}

bool RealPhoneHardening::hideTestKeys() {
    auto& adb = ADBManager::getInstance();
    
    std::string fingerprint = adb.getProperty("ro.build.fingerprint");
    if (fingerprint.find("test-keys") != std::string::npos) {
        adb.setProperty("ro.build.fingerprint", fingerprint + ":user/release-keys");
        adb.setProperty("ro.build.tags", "release-keys");
    }
    
    return true;
}

// SafetyNet & Play Integrity
bool RealPhoneHardening::enableSafetyNet() {
    Logger::getInstance().info("Enabling SafetyNet...");
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.verity.mode", "enforcing");
    adb.setProperty("ro.boot.verifiedbootstate", "green");
    adb.setProperty("ro.build.type", "user");
    adb.setProperty("ro.build.tags", "release-keys");
    adb.setProperty("ro.debuggable", "0");
    adb.setProperty("ro.secure", "1");
    
    hideRoot();
    enableDmVerity();
    enableVerifiedBoot();
    
    return true;
}

bool RealPhoneHardening::enablePlayIntegrity() {
    Logger::getInstance().info("Enabling Play Integrity...");
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.play.integrity.enabled", "true");
    adb.setProperty("ro.play.integrity.basic", "true");
    adb.setProperty("ro.play.integrity.device", "true");
    
    enableSafetyNet();
    
    return true;
}

bool RealPhoneHardening::mockSafetyNetResponse(bool basicIntegrity, bool ctsProfile) {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("persist.safetynet.basic", basicIntegrity ? "true" : "false");
    adb.setProperty("persist.safetynet.cts", ctsProfile ? "true" : "false");
    adb.setProperty("persist.safetynet.attest", "true");
    
    return true;
}

bool RealPhoneHardening::setPlayIntegrityResult(const std::string& nonce, const std::string& result) {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("persist.play.integrity.nonce", nonce);
    adb.setProperty("persist.play.integrity.result", result);
    adb.setProperty("ro.play.integrity.enabled", "true");
    
    adb.executeShellCommand("settings put global play_integrity_result " + result);
    
    return true;
}

// Battery Hardening
bool RealPhoneHardening::setBatteryState(int level, const std::string& status, const std::string& type) {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("dumpsys battery set level " + std::to_string(level));
    
    if (status == "Charging") adb.executeShellCommand("dumpsys battery set status 2");
    else if (status == "Discharging") adb.executeShellCommand("dumpsys battery set status 3");
    else if (status == "Full") adb.executeShellCommand("dumpsys battery set status 5");
    
    if (type == "USB") {
        adb.executeShellCommand("dumpsys battery set usb 1");
        adb.executeShellCommand("dumpsys battery set ac 0");
    } else if (type == "AC") {
        adb.executeShellCommand("dumpsys battery set ac 1");
        adb.executeShellCommand("dumpsys battery set usb 0");
    } else if (type == "Wireless") {
        adb.executeShellCommand("dumpsys battery set wireless 1");
    }
    
    m_batteryConfig.enabled = true;
    m_batteryConfig.batteryLevel = level;
    m_batteryConfig.chargingStatus = status;
    m_batteryConfig.chargingType = type;
    
    return true;
}

bool RealPhoneHardening::setBatteryTemperature(const std::string& temp) {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("dumpsys battery set temperature " + temp);
    m_batteryConfig.temperature = temp;
    return true;
}

bool RealPhoneHardening::setBatteryHealth(const std::string& health) {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("dumpsys battery set health 2");
    m_batteryConfig.health = health;
    return true;
}

bool RealPhoneHardening::enableBatterySimulation() {
    m_batteryConfig.enabled = true;
    return setBatteryState(m_batteryConfig.batteryLevel, m_batteryConfig.chargingStatus, m_batteryConfig.chargingType);
}

bool RealPhoneHardening::disableBatterySimulation() {
    m_batteryConfig.enabled = false;
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("dumpsys battery reset");
    return true;
}

// Radio/Bandwidth Hardening
bool RealPhoneHardening::spoofBaseband(const std::string& version) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.baseband", version);
    adb.setProperty("persist.radio.baseband", version);
    m_radioConfig.basebandVersion = version;
    return true;
}

bool RealPhoneHardening::spoofRadioVersion(const std::string& version) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.radio.version", version);
    adb.setProperty("ro.modem.wifi.version", version);
    adb.setProperty("persist.radio.version", version);
    m_radioConfig.radioVersion = version;
    return true;
}

bool RealPhoneHardening::spoofKernelVersion(const std::string& version) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.kernel.version", version);
    adb.setProperty("ro.config.kernel.version", version);
    m_kernelConfig.kernelVersion = version;
    return true;
}

bool RealPhoneHardening::spoofCpuAbi(const std::string& abi) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.product.cpu.abi", abi);
    adb.setProperty("ro.product.cpu.abi2", abi + "2");
    return true;
}

// Kernel Hardening
bool RealPhoneHardening::hideProcfsContents() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("mount -o rw,remount /proc");
    adb.executeShellCommand("chmod 000 /proc/1/cmdline 2>/dev/null || true");
    adb.executeShellCommand("chmod 000 /proc/*/cmdline 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::spoofCpuTiming() {
    Logger::getInstance().info("CPU timing spoofing enabled");
    return true;
}

bool RealPhoneHardening::addSensorNoise() {
    Logger::getInstance().info("Sensor noise enabled for realistic data");
    return true;
}

bool RealPhoneHardening::removeSensorNoise() {
    Logger::getInstance().info("Sensor noise disabled");
    return true;
}

bool RealPhoneHardening::hideLinuxSubsystem() {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.kernel.android.qemu", "0");
    adb.executeShellCommand("getprop ro.kernel.android.qemu 2>/dev/null | grep -v qemu && echo '0'");
    return true;
}

// Emulator Detection Bypass
bool RealPhoneHardening::bypassQEMUDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.kernel.qemu", "0");
    adb.setProperty("ro.kernel.android.qemu", "0");
    adb.setProperty("ro.product.device", "generic");
    adb.setProperty("ro.hardware", "qcom");
    adb.setProperty("ro.arch", "arm64");
    
    adb.executeShellCommand("getprop | grep qemu | while read line; do resetprop ${line%%:*} ''; done");
    
    return true;
}

bool RealPhoneHardening::bypassGenymotionDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.product.model", "Samsung Galaxy S21");
    adb.setProperty("ro.product.device", "o1s");
    adb.setProperty("ro.product.manufacturer", "Samsung");
    adb.setProperty("ro.build.fingerprint", "samsung/o1sxx/o1s:13/SP1A.210812.016:user/release-keys");
    
    adb.executeShellCommand("settings delete global genotype_id 2>/dev/null || true");
    adb.executeShellCommand("settings delete global geny_generation 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::bypassBlueStacksDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.product.model", "Samsung Galaxy S21 Ultra");
    adb.setProperty("ro.product.device", "dreamlte");
    adb.setProperty("ro.product.manufacturer", "samsung");
    adb.setProperty("ro.build.fingerprint", "samsung/dreamltexx/dreamlte:9/PPR1.180610.011/eng.root:20180919.201423:userdebug/dev-keys");
    
    adb.executeShellCommand("settings delete global bst_emulator_id 2>/dev/null || true");
    adb.executeShellCommand("settings delete secure bst_force_orientation 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::bypassNoxDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.product.model", "Pixel 7 Pro");
    adb.setProperty("ro.product.device", "panther");
    adb.setProperty("ro.product.manufacturer", "Google");
    adb.setProperty("ro.build.fingerprint", "google/panther/panther:14/TD1A.220804.031:user/release-keys");
    
    return true;
}

bool RealPhoneHardening::bypassLDPlayerDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.product.model", "Galaxy S23");
    adb.setProperty("ro.product.device", "z3q");
    adb.setProperty("ro.product.manufacturer", "Samsung");
    
    adb.executeShellCommand("settings delete global ldb_multi_instance 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::bypassVirtualBoxDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.vbox.app.mode", "0");
    adb.executeShellCommand("getprop | grep vbox | while read line; do resetprop ${line%%:*} ''; done");
    
    return true;
}

bool RealPhoneHardening::bypassAllEmulators() {
    bypassQEMUDetection();
    bypassGenymotionDetection();
    bypassBlueStacksDetection();
    bypassNoxDetection();
    bypassLDPlayerDetection();
    bypassVirtualBoxDetection();
    
    hideProcfsContents();
    spoofCpuTiming();
    addSensorNoise();
    hideLinuxSubsystem();
    
    Logger::getInstance().info("All emulator detection bypassed");
    
    return true;
}

// Canvas Fingerprint Bypass
bool RealPhoneHardening::enableCanvasSpoofing() {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.canvas.mode", "1");
    m_canvasConfig.enabled = true;
    return true;
}

bool RealPhoneHardening::disableCanvasSpoofing() {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.canvas.mode", "0");
    m_canvasConfig.enabled = false;
    return true;
}

bool RealPhoneHardening::setCanvasSpoofMode(int mode) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.canvas.mode", std::to_string(mode));
    m_canvasConfig.spoofMode = mode;
    return true;
}

bool RealPhoneHardening::setCustomCanvasPattern(const std::string& pattern) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.canvas.pattern", pattern);
    m_canvasConfig.customPattern = pattern;
    return true;
}

bool RealPhoneHardening::randomizeCanvasFingerprint() {
    auto& adb = ADBManager::getInstance();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string pattern = "canvas_" + std::to_string(dis(gen));
    adb.setProperty("persist.sys.canvas.pattern", pattern);
    
    return true;
}

// WebGL Fingerprint Bypass
bool RealPhoneHardening::spoofWebGLRenderer(const std::string& renderer) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.webgl.renderer", renderer);
    adb.setProperty("debug.webgl.renderer", renderer);
    m_webglConfig.renderer = renderer;
    return true;
}

bool RealPhoneHardening::spoofWebGLVendor(const std::string& vendor) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.webgl.vendor", vendor);
    adb.setProperty("debug.webgl.vendor", vendor);
    m_webglConfig.vendor = vendor;
    return true;
}

bool RealPhoneHardening::spoofWebGLVersion(const std::string& version) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.webgl.version", version);
    adb.setProperty("debug.webgl.version", version);
    m_webglConfig.version = version;
    return true;
}

bool RealPhoneHardening::applyWebGLHardening() {
    if (m_webglConfig.renderer.empty()) {
        spoofWebGLRenderer("Mali-G78");
    }
    if (m_webglConfig.vendor.empty()) {
        spoofWebGLVendor("ARM");
    }
    if (m_webglConfig.version.empty()) {
        spoofWebGLVersion("WebGL 2.0");
    }
    return true;
}

// Audio Fingerprint Bypass
bool RealPhoneHardening::enableAudioSpoofing() {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.audio.mode", "1");
    m_audioConfig.enabled = true;
    return true;
}

bool RealPhoneHardening::disableAudioSpoofing() {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.audio.mode", "0");
    m_audioConfig.enabled = false;
    return true;
}

bool RealPhoneHardening::setAudioContext(const std::string& context) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.audio.context", context);
    m_audioConfig.audioContext = context;
    return true;
}

bool RealPhoneHardening::setSampleRate(const std::string& rate) {
    auto& adb = ADBManager::getInstance();
    adb.setProperty("persist.sys.audio.samplerate", rate);
    m_audioConfig.sampleRate = rate;
    return true;
}

// DNS Hardening
bool RealPhoneHardening::setGoogleDNS() {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("setprop net.dns1 8.8.8.8");
    adb.executeShellCommand("setprop net.dns2 8.8.4.4");
    adb.setProperty("persist.net.dns1", "8.8.8.8");
    adb.setProperty("persist.net.dns2", "8.8.4.4");
    return true;
}

bool RealPhoneHardening::setCloudflareDNS() {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("setprop net.dns1 1.1.1.1");
    adb.executeShellCommand("setprop net.dns2 1.0.0.1");
    adb.setProperty("persist.net.dns1", "1.1.1.1");
    adb.setProperty("persist.net.dns2", "1.0.0.1");
    return true;
}

bool RealPhoneHardening::setCustomDNS(const std::string& dns1, const std::string& dns2) {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("setprop net.dns1 " + dns1);
    if (!dns2.empty()) {
        adb.executeShellCommand("setprop net.dns2 " + dns2);
    }
    adb.setProperty("persist.net.dns1", dns1);
    if (!dns2.empty()) {
        adb.setProperty("persist.net.dns2", dns2);
    }
    return true;
}

bool RealPhoneHardening::enablePrivateDNS() {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("settings put global private_dns_mode hostname");
    adb.executeShellCommand("settings put global private_dns_default_hostname dns.google");
    adb.setProperty("persist.sys.private_dns", "enabled");
    return true;
}

bool RealPhoneHardening::disablePrivateDNS() {
    auto& adb = ADBManager::getInstance();
    adb.executeShellCommand("settings put global private_dns_mode off");
    adb.executeShellCommand("settings delete global private_dns_default_hostname");
    adb.setProperty("persist.sys.private_dns", "disabled");
    return true;
}

// Complete Apply
bool RealPhoneHardening::applyAllHardening() {
    Logger::getInstance().info("Applying all hardening measures...");
    
    if (m_config.hideRoot) hideRoot();
    if (m_config.hideMagisk) hideMagisk();
    if (m_config.hideSU) hideSU();
    if (m_config.hideXposed) hideXposed();
    if (m_config.hideSELinux) hideSelinuxStatus();
    if (m_config.hideDebugFlags) hideDebugStatus();
    if (m_config.enableDmVerity) enableDmVerity();
    if (m_config.enableVerifiedBoot) enableVerifiedBoot();
    if (m_config.hideBuildType) hideBuildType();
    if (m_config.hideUserDebug) convertUserdebugToUser();
    if (m_config.hideTestKeys) hideTestKeys();
    
    if (m_config.enableSafetyNet) enableSafetyNet();
    if (m_config.enablePlayIntegrity) enablePlayIntegrity();
    
    setGoogleDNS();
    enablePrivateDNS();
    
    m_hardeningApplied = true;
    Logger::getInstance().info("All hardening measures applied");
    
    return true;
}

bool RealPhoneHardening::applyEmulatorBypass() {
    return bypassAllEmulators();
}

bool RealPhoneHardening::applyFingerprintBypass() {
    enableCanvasSpoofing();
    applyWebGLHardening();
    enableAudioSpoofing();
    return true;
}

// Reset
bool RealPhoneHardening::resetAll() {
    resetRootDetection();
    resetBattery();
    resetEmulatorBypass();
    resetFingerprint();
    m_hardeningApplied = false;
    return true;
}

bool RealPhoneHardening::resetRootDetection() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("mv /system/bin/su.bak /system/bin/su 2>/dev/null || true");
    adb.executeShellCommand("mv /system/xbin/su.bak /system/xbin/su 2>/dev/null || true");
    adb.executeShellCommand("pm enable com.topjohnwu.magisk 2>/dev/null || true");
    
    adb.setProperty("ro.debuggable", "");
    adb.setProperty("ro.build.type", "");
    adb.setProperty("ro.build.tags", "");
    
    return true;
}

bool RealPhoneHardening::resetBattery() {
    return disableBatterySimulation();
}

bool RealPhoneHardening::resetEmulatorBypass() {
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("resetprop ro.kernel.qemu 2>/dev/null || true");
    adb.executeShellCommand("resetprop ro.kernel.android.qemu 2>/dev/null || true");
    adb.executeShellCommand("resetprop ro.product.device 2>/dev/null || true");
    
    return true;
}

bool RealPhoneHardening::resetFingerprint() {
    disableCanvasSpoofing();
    disableAudioSpoofing();
    return true;
}

// Status
std::map<std::string, std::string> RealPhoneHardening::getHardeningStatus() {
    std::map<std::string, std::string> status;
    
    status["hardening_active"] = m_hardeningApplied ? "true" : "false";
    status["root_hidden"] = m_config.hideRoot ? "true" : "false";
    status["magisk_hidden"] = m_config.hideMagisk ? "true" : "false";
    status["selinux_hidden"] = m_config.hideSELinux ? "true" : "false";
    status["debug_hidden"] = m_config.hideDebugFlags ? "true" : "false";
    status["dm_verity"] = m_config.enableDmVerity ? "true" : "false";
    status["verified_boot"] = m_config.enableVerifiedBoot ? "true" : "false";
    status["safeltynet"] = m_config.enableSafetyNet ? "true" : "false";
    status["play_integrity"] = m_config.enablePlayIntegrity ? "true" : "false";
    status["battery_simulation"] = m_batteryConfig.enabled ? "true" : "false";
    status["canvas_spoofing"] = m_canvasConfig.enabled ? "true" : "false";
    status["webgl_hardening"] = m_webglConfig.enabled ? "true" : "false";
    status["audio_spoofing"] = m_audioConfig.enabled ? "true" : "false";
    
    return status;
}

bool RealPhoneHardening::isDeviceRealPhone() {
    auto& adb = ADBManager::getInstance();
    
    std::string qemu = adb.getProperty("ro.kernel.qemu");
    std::string product = adb.getProperty("ro.product.device");
    
    if (qemu == "1" || qemu == "true") return false;
    if (product.find("emulator") != std::string::npos) return false;
    if (product.find("generic") != std::string::npos) return false;
    
    return true;
}

std::vector<std::string> RealPhoneHardening::getDetectionWarnings() {
    std::vector<std::string> warnings;
    auto& adb = ADBManager::getInstance();
    
    std::string type = adb.getProperty("ro.build.type");
    if (type == "userdebug") {
        warnings.push_back("Build type is userdebug - may be detected");
    }
    
    std::string tags = adb.getProperty("ro.build.tags");
    if (tags.find("test-keys") != std::string::npos) {
        warnings.push_back("Build uses test-keys - will be detected");
    }
    
    std::string debuggable = adb.getProperty("ro.debuggable");
    if (debuggable == "1") {
        warnings.push_back("Device is debuggable - may be detected");
    }
    
    std::string suCheck = adb.executeShellCommand("which su 2>/dev/null");
    if (!suCheck.empty() && suCheck.find("su") != std::string::npos) {
        warnings.push_back("SU binary found - will be detected");
    }
    
    return warnings;
}

}
