#include "VirtualPhonePro/HardwareFingerprintSpoofer.h"
#include "ADBManager.hpp"
#include "Logger.hpp"
#include <sstream>
#include <iomanip>
#include <random>

namespace VirtualPhonePro {

HardwareFingerprintSpoofer& HardwareFingerprintSpoofer::getInstance() {
    static HardwareFingerprintSpoofer instance;
    return instance;
}

HardwareFingerprintSpoofer::HardwareFingerprintSpoofer()
    : m_initialized(false)
    , m_spoofingActive(false)
{
}

HardwareFingerprintSpoofer::~HardwareFingerprintSpoofer() {
    shutdown();
}

bool HardwareFingerprintSpoofer::initialize() {
    Logger::getInstance().info("Initializing Hardware Fingerprint Spoofer...");
    
    auto& adb = ADBManager::getInstance();
    if (!adb.isConnected()) {
        Logger::getInstance().warning("ADB not connected - hardware spoofing limited");
    }
    
    // Store original values
    if (adb.isConnected()) {
        m_originalSpoof.cpuModel = adb.getProperty("ro.product.cpu.abi");
        m_originalSpoof.deviceModel = adb.getProperty("ro.product.model");
        m_originalSpoof.deviceManufacturer = adb.getProperty("ro.product.manufacturer");
        m_originalSpoof.deviceBrand = adb.getProperty("ro.product.brand");
        m_originalSpoof.deviceHardware = adb.getProperty("ro.hardware");
        m_originalSpoof.bootloaderVersion = adb.getProperty("ro.bootloader");
        m_originalSpoof.kernelVersion = adb.getProperty("ro.kernel.version");
    }
    
    m_initialized = true;
    Logger::getInstance().info("Hardware Fingerprint Spoofer initialized");
    return true;
}

bool HardwareFingerprintSpoofer::isInitialized() const {
    return m_initialized;
}

void HardwareFingerprintSpoofer::shutdown() {
    if (m_initialized) {
        if (m_spoofingActive) {
            restoreOriginalValues();
        }
        m_initialized = false;
        Logger::getInstance().info("Hardware Fingerprint Spoofer shutdown complete");
    }
}

SpoofResult HardwareFingerprintSpoofer::enableAllSpoofing() {
    SpoofResult result = {false, "", "", {}};
    
    if (!m_initialized) {
        result.error = "Not initialized";
        return result;
    }
    
    setExynos2100Profile();
    setMaliG78Profile();
    setSamsungGalaxyS21Profile();
    setRealHardwareDMI();
    
    m_spoofingActive = true;
    result.success = true;
    result.message = "All hardware fingerprint spoofing enabled";
    
    Logger::getInstance().info(result.message);
    return result;
}

SpoofResult HardwareFingerprintSpoofer::disableAllSpoofing() {
    SpoofResult result = {false, "", "", {}};
    
    restoreOriginalValues();
    m_spoofingActive = false;
    
    result.success = true;
    result.message = "All hardware fingerprint spoofing disabled";
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofCPUInfo(const std::string& cpuModel, int cores, int threads) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Set CPU model
    adb.setProperty("ro.product.cpu.model", cpuModel);
    adb.setProperty("dalvik.vm.dex2oat-Xms", "64m");
    adb.setProperty("dalvik.vm.dex2oat-Xmx", "512m");
    
    // Set cores
    adb.setProperty("ro.product.cpu.cores", std::to_string(cores));
    adb.setProperty("sys.cpu.nums", std::to_string(cores));
    
    // Set threads
    adb.setProperty("ro.product.cpu.threads", std::to_string(threads));
    
    m_currentSpoof.cpuModel = cpuModel;
    m_currentSpoof.cpuCores = cores;
    m_currentSpoof.cpuThreads = threads;
    
    m_spoofedProperties["ro.product.cpu.model"] = cpuModel;
    m_spoofedProperties["ro.product.cpu.cores"] = std::to_string(cores);
    
    result.success = true;
    result.message = "CPU info spoofed: " + cpuModel + " (" + std::to_string(cores) + " cores)";
    result.details["cpu_model"] = cpuModel;
    result.details["cores"] = std::to_string(cores);
    result.details["threads"] = std::to_string(threads);
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setExynos2100Profile() {
    return spoofCPUInfo("Exynos 2100", 8, 8);
}

SpoofResult HardwareFingerprintSpoofer::setSnapdragon888Profile() {
    return spoofCPUInfo("Snapdragon 888", 8, 8);
}

SpoofResult HardwareFingerprintSpoofer::setSnapdragon8Gen1Profile() {
    return spoofCPUInfo("Snapdragon 8 Gen 1", 8, 8);
}

SpoofResult HardwareFingerprintSpoofer::setDimensity9000Profile() {
    return spoofCPUInfo("Dimensity 9000", 8, 8);
}

SpoofResult HardwareFingerprintSpoofer::spoofGPUInfo(const std::string& gpuModel) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.hardware.gpu", gpuModel);
    adb.setProperty("debug.hwui.render", gpuModel);
    adb.executeShellCommand("setprop debug.hwui.use_gpu_rasterizer true");
    adb.setProperty("debug.gralloc.gpu", gpuModel);
    
    m_currentSpoof.gpuRenderer = gpuModel;
    m_spoofedProperties["ro.hardware.gpu"] = gpuModel;
    
    result.success = true;
    result.message = "GPU info spoofed: " + gpuModel;
    result.details["gpu"] = gpuModel;
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setMaliG78Profile() {
    return spoofGPUInfo("Mali-G78");
}

SpoofResult HardwareFingerprintSpoofer::setAdreno660Profile() {
    return spoofGPUInfo("Adreno 660");
}

SpoofResult HardwareFingerprintSpoofer::setAdreno730Profile() {
    return spoofGPUInfo("Adreno 730");
}

SpoofResult HardwareFingerprintSpoofer::setMaliG710Profile() {
    return spoofGPUInfo("Mali-G710");
}

SpoofResult HardwareFingerprintSpoofer::spoofDeviceInfo(const std::string& manufacturer,
                                                       const std::string& model,
                                                       const std::string& brand) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.setProperty("ro.product.manufacturer", manufacturer);
    adb.setProperty("ro.product.model", model);
    adb.setProperty("ro.product.brand", brand);
    adb.setProperty("ro.product.name", model);
    adb.setProperty("ro.product.device", model);
    
    m_currentSpoof.deviceManufacturer = manufacturer;
    m_currentSpoof.deviceModel = model;
    m_currentSpoof.deviceBrand = brand;
    
    m_spoofedProperties["ro.product.manufacturer"] = manufacturer;
    m_spoofedProperties["ro.product.model"] = model;
    m_spoofedProperties["ro.product.brand"] = brand;
    
    result.success = true;
    result.message = "Device info spoofed: " + manufacturer + " " + model;
    result.details["manufacturer"] = manufacturer;
    result.details["model"] = model;
    result.details["brand"] = brand;
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setSamsungGalaxyS21Profile() {
    auto result = spoofDeviceInfo("samsung", "SM-G998B", "samsung");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "exynos2100");
        adb.setProperty("ro.board.platform", "exynos2100");
        adb.setProperty("ro.arch", "arm64");
        adb.setProperty("ro.build.fingerprint", 
            "samsung/o1sxx/o1s:13/SP1A.210812.016/G998BXXU5EWH5:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "exynos2100";
        m_spoofedProperties["ro.board.platform"] = "exynos2100";
        
        result.message += " [Samsung Galaxy S21 Ultra Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setSamsungGalaxyS22Profile() {
    auto result = spoofDeviceInfo("samsung", "SM-S908B", "samsung");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "exynos2200");
        adb.setProperty("ro.board.platform", "exynos2200");
        adb.setProperty("ro.build.fingerprint",
            "samsung/o1sxx/o1s:13/SP1A.210812.016/S908BXXU2BWK3:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "exynos2200";
        result.message += " [Samsung Galaxy S22 Ultra Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setGooglePixel6Profile() {
    auto result = spoofDeviceInfo("Google", "Pixel 6", "google");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "oriole");
        adb.setProperty("ro.board.platform", "oriole");
        adb.setProperty("ro.build.fingerprint",
            "google/oriole/oriole:13/TP1A.220624.014/9477233:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "oriole";
        result.message += " [Google Pixel 6 Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setGooglePixel7Profile() {
    auto result = spoofDeviceInfo("Google", "Pixel 7", "google");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "panther");
        adb.setProperty("ro.board.platform", "panther");
        adb.setProperty("ro.build.fingerprint",
            "google/panther/panther:13/TP1A.220624.014/9477233:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "panther";
        result.message += " [Google Pixel 7 Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setXiaomi12Profile() {
    auto result = spoofDeviceInfo("Xiaomi", "2201123G", "Xiaomi");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "thyme");
        adb.setProperty("ro.board.platform", "thyme");
        adb.setProperty("ro.build.fingerprint",
            "Xiaomi/thyme/thyme:13/V14.0.3.0.TLCEUXM:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "thyme";
        result.message += " [Xiaomi 12 Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setOnePlus10Profile() {
    auto result = spoofDeviceInfo("OnePlus", "LE2125", "OnePlus");
    if (result.success) {
        auto& adb = ADBManager::getInstance();
        adb.setProperty("ro.hardware", "lahaina");
        adb.setProperty("ro.board.platform", "lahaina");
        adb.setProperty("ro.build.fingerprint",
            "OnePlus/LE2125/OPR4:13/SP1A.210812.016/2204152345:user/release-keys");
        
        m_spoofedProperties["ro.hardware"] = "lahaina";
        result.message += " [OnePlus 10 Pro Profile Applied]";
    }
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofBootloaderVersion(const std::string& version) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.bootloader", version);
    adb.setProperty("ro.boot.bootloader", version);
    
    m_currentSpoof.bootloaderVersion = version;
    m_spoofedProperties["ro.bootloader"] = version;
    
    result.success = true;
    result.message = "Bootloader version spoofed: " + version;
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofRadioVersion(const std::string& version) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.build.version.radio", version);
    adb.setProperty("gsm.version.radio-initial", version);
    
    m_currentSpoof.radioVersion = version;
    m_spoofedProperties["ro.build.version.radio"] = version;
    
    result.success = true;
    result.message = "Radio version spoofed: " + version;
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofKernelVersion(const std::string& version) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.kernel.version", version);
    adb.setProperty("ro.build.version.kernel", version);
    
    m_currentSpoof.kernelVersion = version;
    m_spoofedProperties["ro.kernel.version"] = version;
    
    result.success = true;
    result.message = "Kernel version spoofed: " + version;
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofDMIInfo(const std::string& vendor,
                                                     const std::string& product,
                                                     const std::string& boardVendor,
                                                     const std::string& boardProduct) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // DMI info for emulator detection bypass
    adb.executeShellCommand("setprop sys.dmi.system.vendor " + vendor + " 2>/dev/null || true");
    adb.executeShellCommand("setprop sys.dmi.system.product " + product + " 2>/dev/null || true");
    adb.executeShellCommand("setprop sys.dmi.board.vendor " + boardVendor + " 2>/dev/null || true");
    adb.executeShellCommand("setprop sys.dmi.board.product " + boardProduct + " 2>/dev/null || true");
    
    m_currentSpoof.dmiSystemVendor = vendor;
    m_currentSpoof.dmiSystemProduct = product;
    m_currentSpoof.dmiBoardVendor = boardVendor;
    m_currentSpoof.dmiBoardProduct = boardProduct;
    
    result.success = true;
    result.message = "DMI info spoofed for real hardware appearance";
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::setRealHardwareDMI() {
    return spoofDMIInfo("SAMSUNG", "SM-G998B", "SAMSUNG", "o1s");
}

SpoofResult HardwareFingerprintSpoofer::spoofBatteryInfo(int level, const std::string& status,
                                                         const std::string& health) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("dumpsys battery set level " + std::to_string(level));
    adb.executeShellCommand(std::string("dumpsys battery set status ") + 
        (status == "charging" ? "2" : status == "discharging" ? "3" : "1"));
    adb.executeShellCommand(std::string("dumpsys battery set health ") + 
        (health == "good" ? "2" : health == "overheat" ? "4" : "1"));
    
    result.success = true;
    result.message = "Battery info spoofed: " + std::to_string(level) + "% - " + health;
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofHardwareFeatures() {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Hide virtualization features
    adb.executeShellCommand("resetprop ro.hardware.virtual 0 2>/dev/null || true");
    adb.executeShellCommand("resetprop ro.kernel.virtual false 2>/dev/null || true");
    
    // Set real hardware features
    adb.setProperty("ro.hwui.disable_vulkan", "false");
    adb.setProperty("ro.hwui.texture_cache_size", "72");
    adb.setProperty("ro.hwui.layer_cache_size", "48");
    
    result.success = true;
    result.message = "Hardware features configured for real device";
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofSupportedABIs(const std::vector<std::string>& abis) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    std::string abiList;
    for (size_t i = 0; i < abis.size(); ++i) {
        if (i > 0) abiList += ",";
        abiList += abis[i];
    }
    
    adb.setProperty("ro.product.cpu.abilist", abiList);
    adb.setProperty("ro.product.cpu.abilist32", "armeabi-v7a,armeabi");
    adb.setProperty("ro.product.cpu.abilist64", "arm64-v8a");
    adb.setProperty("ro.product.cpu.abi", abis.empty() ? "arm64-v8a" : abis[0]);
    
    result.success = true;
    result.message = "Supported ABIs spoofed: " + abiList;
    result.details["abis"] = abiList;
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofBiometricInfo() {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("settings put secure biometric_enrolled 1");
    adb.executeShellCommand("settings put secure fingerprint_enrolled 1");
    
    result.success = true;
    result.message = "Biometric enrollment spoofed";
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::hideBiometricEnrollment() {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Hide biometric enrollment
    adb.executeShellCommand("settings put secure biometric_enrolled 0");
    adb.executeShellCommand("settings put secure fingerprint_enrolled 0");
    
    result.success = true;
    result.message = "Biometric enrollment hidden";
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::spoofBuildFingerprint(const std::string& fingerprint) {
    SpoofResult result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    adb.setProperty("ro.build.fingerprint", fingerprint);
    adb.setProperty("ro.vendor.build.fingerprint", fingerprint);
    adb.setProperty("ro.odm.build.fingerprint", fingerprint);
    adb.setProperty("ro.product.build.fingerprint", fingerprint);
    
    m_spoofedProperties["ro.build.fingerprint"] = fingerprint;
    
    result.success = true;
    result.message = "Build fingerprint spoofed";
    result.details["fingerprint"] = fingerprint;
    
    return result;
}

SpoofResult HardwareFingerprintSpoofer::generateSamsungFingerprint() {
    std::stringstream ss;
    ss << "samsung/o1sxx/o1s:13/SP1A.210812.016/G998BXXU5EWH5:user/release-keys";
    return spoofBuildFingerprint(ss.str());
}

SpoofResult HardwareFingerprintSpoofer::generateGoogleFingerprint() {
    std::stringstream ss;
    ss << "google/panther/panther:13/TP1A.220624.014/9477233:user/release-keys";
    return spoofBuildFingerprint(ss.str());
}

SpoofResult HardwareFingerprintSpoofer::generateXiaomiFingerprint() {
    std::stringstream ss;
    ss << "Xiaomi/thyme/thyme:13/V14.0.3.0.TLCEUXM:user/release-keys";
    return spoofBuildFingerprint(ss.str());
}

SpoofResult HardwareFingerprintSpoofer::validateSpoofing() {
    SpoofResult result = {false, "", "", {}};
    
    // Check if spoofing is active
    result.success = m_spoofingActive;
    result.message = m_spoofingActive ? 
        "Hardware spoofing is active" : 
        "Hardware spoofing is not active";
    
    // Count spoofed properties
    result.details["spoofed_properties_count"] = std::to_string(m_spoofedProperties.size());
    
    return result;
}

bool HardwareFingerprintSpoofer::isSpoofingActive() const {
    return m_spoofingActive;
}

HardwareFingerprint HardwareFingerprintSpoofer::getCurrentSpoofedFingerprint() {
    return m_currentSpoof;
}

std::map<std::string, std::string> HardwareFingerprintSpoofer::getDetailedStatus() {
    std::map<std::string, std::string> status;
    
    status["initialized"] = m_initialized ? "true" : "false";
    status["spoofing_active"] = m_spoofingActive ? "true" : "false";
    status["spoofed_properties"] = std::to_string(m_spoofedProperties.size());
    
    status["cpu_model"] = m_currentSpoof.cpuModel;
    status["cpu_cores"] = std::to_string(m_currentSpoof.cpuCores);
    status["gpu"] = m_currentSpoof.gpuRenderer;
    status["device_manufacturer"] = m_currentSpoof.deviceManufacturer;
    status["device_model"] = m_currentSpoof.deviceModel;
    status["device_brand"] = m_currentSpoof.deviceBrand;
    
    return status;
}

SpoofResult HardwareFingerprintSpoofer::getStatus() {
    SpoofResult result = {false, "", "", {}};
    
    std::stringstream ss;
    ss << "Hardware Fingerprint Spoofer Status:\n";
    ss << "  Initialized: " << (m_initialized ? "Yes" : "No") << "\n";
    ss << "  Spoofing Active: " << (m_spoofingActive ? "Yes" : "No") << "\n";
    ss << "  Spoofed Properties: " << m_spoofedProperties.size() << "\n";
    ss << "  CPU: " << m_currentSpoof.cpuModel << " (" << m_currentSpoof.cpuCores << " cores)\n";
    ss << "  GPU: " << m_currentSpoof.gpuRenderer << "\n";
    ss << "  Device: " << m_currentSpoof.deviceManufacturer << " " << m_currentSpoof.deviceModel;
    
    result.success = true;
    result.message = ss.str();
    
    return result;
}

void HardwareFingerprintSpoofer::applyCPUChanges(const HardwareFingerprint& fp) {
    auto& adb = ADBManager::getInstance();
    
    // Apply CPU model
    if (!fp.cpuModel.empty()) {
        adb.setProperty("ro.product.cpu.model", fp.cpuModel);
        adb.setProperty("dalvik.vm.dex2oat-Xms", "64m");
        adb.setProperty("dalvik.vm.dex2oat-Xmx", "512m");
    }
    
    // Apply core count
    if (fp.cpuCores > 0) {
        adb.setProperty("ro.product.cpu.cores", std::to_string(fp.cpuCores));
        adb.setProperty("sys.cpu.nums", std::to_string(fp.cpuCores));
        adb.executeShellCommand("echo " + std::to_string(fp.cpuCores) + " > /sys/devices/system/cpu/online 2>/dev/null || true");
    }
    
    // Apply thread count
    if (fp.cpuThreads > 0) {
        adb.setProperty("ro.product.cpu.threads", std::to_string(fp.cpuThreads));
    }
    
    // Apply CPU architecture
    if (!fp.cpuArchitecture.empty()) {
        adb.setProperty("ro.product.cpu.abi", fp.cpuArchitecture);
        adb.setProperty("ro.product.cpu.abi2", "");
    }
    
    // Apply CPU frequency
    if (fp.cpuFrequencyMax > 0) {
        adb.executeShellCommand("echo " + std::to_string(fp.cpuFrequencyMax) + " > /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq 2>/dev/null || true");
    }
    if (fp.cpuFrequencyMin > 0) {
        adb.executeShellCommand("echo " + std::to_string(fp.cpuFrequencyMin) + " > /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq 2>/dev/null || true");
    }
    
    // Apply /proc/cpuinfo spoofing
    QString cpuInfoPath = "/proc/cpuinfo";
    QString cpuInfoContent = generateCpuInfoContentFromFingerprint(fp);
    
    // Create spoofed cpuinfo
    adb.executeShellCommand("mount -o rw,remount /system 2>/dev/null || true");
    adb.executeShellCommand("echo '" + cpuInfoContent + "' > " + cpuInfoPath.toStdString() + " 2>/dev/null || true");
    
    qDebug() << "[HardwareSpoofer] Applied CPU changes:" << QString::fromStdString(fp.cpuModel).trimmed();
}

void HardwareFingerprintSpoofer::applyGPUChanges(const HardwareFingerprint& fp) {
    auto& adb = ADBManager::getInstance();
    
    // Apply GPU model
    if (!fp.gpuRenderer.empty()) {
        adb.setProperty("ro.hardware.gpu", fp.gpuRenderer);
        adb.setProperty("debug.hwui.render", fp.gpuRenderer);
        adb.executeShellCommand("setprop debug.hwui.use_gpu_rasterizer true");
        adb.setProperty("debug.gralloc.gpu", fp.gpuRenderer);
        adb.setProperty("ro.opengles.version", "196610"); // OpenGL ES 3.2
        
        // GPU frequency
        if (fp.gpuMaxFreq > 0) {
            adb.executeShellCommand("echo " + std::to_string(fp.gpuMaxFreq) + " > /sys/class/kgsl/kgsl-3d0/max_gpuclk 2>/dev/null || true");
        }
        
        // GPU core count
        if (fp.gpuCoreCount > 0) {
            adb.executeShellCommand("echo " + std::to_string(fp.gpuCoreCount) + " > /sys/class/kgsl/kgsl-3d0/gpu_model 2>/dev/null || true");
        }
    }
    
    // Apply OpenGL vendor and renderer
    if (!fp.gpuVendor.empty()) {
        adb.setProperty("ro.hardware.vulkan", fp.gpuVendor);
    }
    
    // Spoof /sys/class/misc/gpu information
    QString gpuInfo = QString::fromStdString(fp.gpuRenderer);
    adb.executeShellCommand("echo '" + gpuInfo + "' > /sys/class/misc/gpu/model 2>/dev/null || true");
    adb.executeShellCommand("chmod 444 /sys/class/misc/gpu/model 2>/dev/null || true");
    
    qDebug() << "[HardwareSpoofer] Applied GPU changes:" << gpuInfo;
}

void HardwareFingerprintSpoofer::applyDeviceChanges(const HardwareFingerprint& fp) {
    auto& adb = ADBManager::getInstance();
    
    // Apply manufacturer
    if (!fp.deviceManufacturer.empty()) {
        adb.setProperty("ro.product.manufacturer", fp.deviceManufacturer);
        adb.setProperty("ro.product.vendor.manufacturer", fp.deviceManufacturer);
    }
    
    // Apply model
    if (!fp.deviceModel.empty()) {
        adb.setProperty("ro.product.model", fp.deviceModel);
        adb.setProperty("ro.product.name", fp.deviceModel);
        adb.setProperty("ro.product.device", fp.deviceModel);
        adb.setProperty("ro.product.brand.model", fp.deviceModel);
    }
    
    // Apply brand
    if (!fp.deviceBrand.empty()) {
        adb.setProperty("ro.product.brand", fp.deviceBrand);
        adb.setProperty("ro.product.vendor.brand", fp.deviceBrand);
    }
    
    // Apply hardware
    if (!fp.deviceHardware.empty()) {
        adb.setProperty("ro.hardware", fp.deviceHardware);
        adb.setProperty("ro.arch", fp.deviceHardware);
        adb.setProperty("ro.board.platform", fp.deviceHardware);
    }
    
    // Apply bootloader
    if (!fp.bootloaderVersion.empty()) {
        adb.setProperty("ro.bootloader", fp.bootloaderVersion);
        adb.setProperty("ro.bootmode", "normal");
    }
    
    // Apply build fingerprint
    if (!fp.buildFingerprint.empty()) {
        QStringList fingerprints = {
            "ro.build.fingerprint",
            "ro.vendor.build.fingerprint",
            "ro.odm.build.fingerprint",
            "ro.product.build.fingerprint",
            "ro.system.build.fingerprint"
        };
        for (const QString& fp_prop : fingerprints) {
            adb.setProperty(fp_prop.toStdString(), fp.buildFingerprint);
        }
    }
    
    // Apply /system/build.prop modifications
    adb.executeShellCommand("mount -o rw,remount /system 2>/dev/null || true");
    adb.executeShellCommand("getprop ro.product.manufacturer > /system/build.prop 2>/dev/null || true");
    
    qDebug() << "[HardwareSpoofer] Applied device changes for:" 
             << QString::fromStdString(fp.deviceManufacturer).trimmed() << QString::fromStdString(fp.deviceModel).trimmed();
}

void HardwareFingerprintSpoofer::applyDMIChanges(const HardwareFingerprint& fp) {
    auto& adb = ADBManager::getInstance();
    
    // Apply DMI/SMBIOS information for BIOS detection bypass
    QStringList dmiPaths = {
        "/sys/class/dmi/id/board_name",
        "/sys/class/dmi/id/bios_vendor",
        "/sys/class/dmi/id/sys_vendor",
        "/sys/class/dmi/id/product_name",
        "/sys/class/dmi/id/product_version"
    };
    
    QString boardVendor = QString::fromStdString(fp.boardVendor.empty() ? fp.deviceManufacturer : fp.boardVendor);
    QString boardName = QString::fromStdString(fp.boardName.empty() ? fp.deviceModel : fp.boardName);
    QString sysVendor = QString::fromStdString(fp.sysVendor.empty() ? fp.deviceManufacturer : fp.sysVendor);
    
    // Apply board information
    adb.executeShellCommand("echo '" + boardVendor + "' > /sys/class/dmi/id/board_vendor 2>/dev/null || true");
    adb.executeShellCommand("echo '" + boardName + "' > /sys/class/dmi/id/board_name 2>/dev/null || true");
    adb.executeShellCommand("echo '" + sysVendor + "' > /sys/class/dmi/id/sys_vendor 2>/dev/null || true");
    adb.executeShellCommand("echo '" + boardName + "' > /sys/class/dmi/id/product_name 2>/dev/null || true");
    adb.executeShellCommand("echo '1.0' > /sys/class/dmi/id/product_version 2>/dev/null || true");
    
    // Apply BIOS information
    adb.executeShellCommand("echo 'American Megatrends' > /sys/class/dmi/id/bios_vendor 2>/dev/null || true");
    adb.executeShellCommand("echo '" + boardVendor + "' > /sys/class/dmi/id/boardVendor 2>/dev/null || true");
    
    // Make DMI files read-only to prevent modification
    for (const QString& path : dmiPaths) {
        adb.executeShellCommand("chmod 444 '" + path + "' 2>/dev/null || true");
    }
    
    // Apply kernel command line for virtualization hiding
    QString kernelCmdline = "androidboot.hardware=" + QString::fromStdString(fp.deviceHardware) + 
                           " androidboot.bootdevice=bootdevice androidboot.slot_suffix=_a";
    adb.executeShellCommand("echo '" + kernelCmdline + "' > /proc/cmdline 2>/dev/null || true");
    
    qDebug() << "[HardwareSpoofer] Applied DMI changes:" << boardName;
}

void HardwareFingerprintSpoofer::restoreOriginalValues() {
    auto& adb = ADBManager::getInstance();
    
    // Restore all spoofed properties to their original values
    QStringList propertiesToRestore = {
        "ro.product.cpu.model", "ro.product.cpu.cores", "ro.product.cpu.threads",
        "ro.hardware.gpu", "debug.hwui.render", "debug.gralloc.gpu",
        "ro.product.manufacturer", "ro.product.model", "ro.product.brand",
        "ro.product.name", "ro.product.device", "ro.hardware",
        "ro.board.platform", "ro.bootloader", "ro.bootmode",
        "ro.build.fingerprint", "ro.vendor.build.fingerprint",
        "ro.odm.build.fingerprint", "ro.product.build.fingerprint",
        "ro.debuggable", "ro.secure"
    };
    
    for (const QString& prop : propertiesToRestore) {
        adb.executeShellCommand("resetprop " + prop + " 2>/dev/null || true");
    }
    
    // Clear spoofed properties tracking
    m_spoofedProperties.clear();
    m_spoofingActive = false;
    
    // Restore DMI information to defaults
    QStringList dmiPaths = {
        "/sys/class/dmi/id/board_name",
        "/sys/class/dmi/id/bios_vendor",
        "/sys/class/dmi/id/sys_vendor",
        "/sys/class/dmi/id/product_name",
        "/sys/class/dmi/id/product_version"
    };
    
    for (const QString& path : dmiPaths) {
        adb.executeShellCommand("chmod 644 '" + path + "' 2>/dev/null || true");
    }
    
    // Restore /proc/cpuinfo
    adb.executeShellCommand("mount -o ro,remount /system 2>/dev/null || true");
    
    qDebug() << "[HardwareSpoofer] Original values restored";
}

std::string HardwareFingerprintSpoofer::generateRandomHex(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < length; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string HardwareFingerprintSpoofer::generateBuildFingerprint(const std::string& brand,
                                                               const std::string& device,
                                                               const std::string& model) {
    std::stringstream ss;
    ss << brand << "/" << device << "/" << model << ":13/SP1A.210812.016/" << generateRandomHex(16) << ":user/release-keys";
    return ss.str();
}

QString HardwareFingerprintSpoofer::generateCpuInfoContentFromFingerprint(const HardwareFingerprint& fp) {
    QStringList lines;
    
    // Processor info
    lines << "Processor       : " + QString::fromStdString(fp.cpuModel);
    lines << "processor      : 0";
    lines << "BogoMIPS       : 384.00";
    lines << "Features       : fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid";
    
    // CPU implementer
    if (!fp.cpuImplementer.empty()) {
        lines << "CPU implementer : 0x" + QString::fromStdString(fp.cpuImplementer);
    } else {
        lines << "CPU implementer : 0x41"; // ARM
    }
    
    // CPU architecture
    lines << "CPU architecture: 8";
    
    // CPU variant
    if (!fp.cpuVariant.empty()) {
        lines << "CPU variant     : 0x" + QString::fromStdString(fp.cpuVariant);
    } else {
        lines << "CPU variant     : 0x2";
    }
    
    // CPU part
    if (!fp.cpuPart.empty()) {
        lines << "CPU part        : 0x" + QString::fromStdString(fp.cpuPart);
    } else {
        lines << "CPU part        : 0xd05"; // Cortex-A76
    }
    
    // CPU revision
    if (!fp.cpuRevision.empty()) {
        lines << "CPU revision    : " + QString::fromStdString(fp.cpuRevision);
    } else {
        lines << "CPU revision    : 1";
    }
    
    // Hardware
    if (!fp.deviceHardware.empty()) {
        lines << "Hardware        : " + QString::fromStdString(fp.deviceHardware);
    } else {
        lines << "Hardware        : Qualcomm";
    }
    
    // Revision
    lines << "Revision        : 0000";
    
    // Serial
    lines << "Serial          : " + QString::fromStdString(fp.serialNumber);
    
    return lines.join("\n");
}

}
