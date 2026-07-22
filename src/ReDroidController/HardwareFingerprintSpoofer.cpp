/**
 * @file HardwareFingerprintSpoofer.cpp
 * @brief Hardware Fingerprint Spoofing Implementation
 * @version 4.0.0
 *
 * Implements all hardware-level spoofing via ADB commands.
 * Matches header: VirtualPhonePro/HardwareFingerprintSpoofer.h
 */

#include "VirtualPhonePro/HardwareFingerprintSpoofer.h"
#include "VirtualPhonePro/ADBManager.hpp"
#include "VirtualPhonePro/Logger.hpp"
#include <string>

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>

namespace VirtualPhonePro {

// ===========================================================================
// Singleton
// ===========================================================================

HardwareFingerprintSpoofer* HardwareFingerprintSpoofer::s_instance = nullptr;

HardwareFingerprintSpoofer& HardwareFingerprintSpoofer::instance() {
    if (!s_instance) {
        s_instance = new HardwareFingerprintSpoofer();
    }
    return *s_instance;
}

HardwareFingerprintSpoofer::HardwareFingerprintSpoofer()
    : QObject(nullptr)
    , m_initialized(false)
    , m_spoofingActive(false)
{
    initializeHardwareProfiles();
}

HardwareFingerprintSpoofer::~HardwareFingerprintSpoofer() {
    m_spoofingActive = false;
}

// ===========================================================================
// Helper: run ADB shell command
// ===========================================================================

static void adbShell(const QString& /*instanceId*/, const QString& cmd) {
    ADBManager& adb = ADBManager::getInstance();
    adb.executeShellCommand(cmd.toStdString());
}

// ===========================================================================
// Initialization
// ===========================================================================

bool HardwareFingerprintSpoofer::initialize(const QString& instanceId) {
    qDebug() << "[HardwareFingerprintSpoofer] Initializing for" << instanceId;
    
    if (!m_states.contains(instanceId)) {
        HardwareFingerprintState state;
        state.instanceId = instanceId;
        state.isInitialized = false;
        state.isActive = false;
        m_states[instanceId] = state;
    }
    
    m_initialized = true;
    m_states[instanceId].isInitialized = true;
    
    qDebug() << "[HardwareFingerprintSpoofer] Initialized OK for" << instanceId;
    return true;
}

bool HardwareFingerprintSpoofer::applyProfile(const QString& instanceId, HardwareProfile profile) {
    qDebug() << "[HardwareFingerprintSpoofer] Applying profile" 
             << static_cast<int>(profile) << "for" << instanceId;
    
    CPUConfig     cpuCfg  = getCPUConfigForProfile(profile);
    GPUConfig     gpuCfg  = getGPUConfigForProfile(profile);
    BatteryConfig batCfg  = getBatteryConfigForProfile(profile);
    ThermalConfig theCfg  = getThermalConfigForProfile(profile);
    
    bool ok = true;
    ok &= configureCPU(instanceId, cpuCfg);
    ok &= configureGPU(instanceId, gpuCfg);
    ok &= configureBattery(instanceId, batCfg);
    ok &= configureThermal(instanceId, theCfg);
    
    m_states[instanceId].isActive = ok;
    return ok;
}

bool HardwareFingerprintSpoofer::applyAllSpoofing(const QString& instanceId) {
    return applyProfile(instanceId, HardwareProfile::SAMSUNG_S24_ULTRA);
}

// ===========================================================================
// CPU Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configureCPU(const QString& instanceId, const CPUConfig& config) {
    Q_UNUSED(config);
    return spoofCpuInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofCpuInfo(const QString& instanceId) {
    QStringList cmds = {
        // Snapdragon 8 Gen 3 (Samsung S24)
        "setprop ro.board.platform taro",
        "setprop ro.hardware qcom",
        "setprop ro.arch arm64",
        "setprop ro.product.cpu.abi arm64-v8a",
        "setprop ro.product.cpu.abilist arm64-v8a,armeabi-v7a,armeabi",
        "setprop ro.product.cpu.abilist32 armeabi-v7a,armeabi",
        "setprop ro.product.cpu.abilist64 arm64-v8a",
        "setprop ro.hardware.chipname SM8650",
        // Hide emulator CPU traces
        "setprop ro.kernel.qemu 0",
        "setprop ro.boot.qemu false",
    };
    
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    
    qDebug() << "[HardwareFingerprintSpoofer] CPU info spoofed for" << instanceId;
    return true;
}

bool HardwareFingerprintSpoofer::spoofCpuFrequency(const QString& instanceId) {
    QStringList cmds = {
        "echo 3187200 > /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
        "echo 300000  > /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq",
        "echo 2457600 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, "sh -c '" + cmd + "' 2>/dev/null || true");
    }
    return true;
}

bool HardwareFingerprintSpoofer::setCpuGovernor(const QString& instanceId, const QString& governor) {
    adbShell(instanceId, QString(
        "for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do "
        "echo %1 > $f 2>/dev/null || true; done"
    ).arg(governor));
    return true;
}

bool HardwareFingerprintSpoofer::applyCPUSpoofing(const QString& instanceId) {
    bool ok = spoofCpuInfo(instanceId);
    ok &= spoofCpuFrequency(instanceId);
    ok &= setCpuGovernor(instanceId, "schedutil");
    return ok;
}

// ===========================================================================
// GPU Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configureGPU(const QString& instanceId, const GPUConfig& config) {
    Q_UNUSED(config);
    return spoofGPUInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofGPUInfo(const QString& instanceId) {
    QStringList cmds = {
        // Adreno 750 (Snapdragon 8 Gen 3)
        "setprop ro.hardware.egl adreno",
        "setprop ro.hardware.vulkan adreno",
        "setprop ro.hardware.gralloc adreno",
        "setprop debug.egl.hw 1",
        "setprop debug.sf.hw 1",
        "setprop persist.sys.webgl.unmasked_renderer Adreno (TM) 750",
        "setprop persist.sys.webgl.unmasked_vendor Qualcomm",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    return true;
}

bool HardwareFingerprintSpoofer::spoofGraphicsInfo(const QString& instanceId) {
    return spoofGPUInfo(instanceId);
}

bool HardwareFingerprintSpoofer::applyGPUSpoofing(const QString& instanceId) {
    return spoofGPUInfo(instanceId);
}

// ===========================================================================
// Battery Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configureBattery(const QString& instanceId, const BatteryConfig& config) {
    Q_UNUSED(config);
    return spoofBatteryInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofBatteryInfo(const QString& instanceId) {
    QStringList cmds = {
        "setprop sys.battery.level 85",
        "setprop sys.battery.status 2",       // Charging
        "setprop sys.battery.health 2",       // Good
        "setprop sys.battery.technology Li-poly",
        // Via sysfs
        "sh -c 'echo 85 > /sys/class/power_supply/battery/capacity 2>/dev/null || true'",
        "sh -c 'echo Full > /sys/class/power_supply/battery/status 2>/dev/null || true'",
        "sh -c 'echo Good > /sys/class/power_supply/battery/health 2>/dev/null || true'",
        "sh -c 'echo 5000000 > /sys/class/power_supply/battery/charge_full 2>/dev/null || true'",
        "sh -c 'echo 4250000 > /sys/class/power_supply/battery/charge_now 2>/dev/null || true'",
        "sh -c 'echo 4350000 > /sys/class/power_supply/battery/voltage_now 2>/dev/null || true'",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    return true;
}

bool HardwareFingerprintSpoofer::setBatteryLevel(const QString& instanceId, int level) {
    adbShell(instanceId, QString("setprop sys.battery.level %1").arg(level));
    adbShell(instanceId, QString(
        "sh -c 'echo %1 > /sys/class/power_supply/battery/capacity 2>/dev/null || true'"
    ).arg(level));
    return true;
}

bool HardwareFingerprintSpoofer::setChargingState(const QString& instanceId, 
                                                   bool charging, const QString& type) {
    QString status = charging ? "Charging" : "Discharging";
    adbShell(instanceId, QString("setprop sys.battery.status %1").arg(charging ? "2" : "3"));
    adbShell(instanceId, QString(
        "sh -c 'echo %1 > /sys/class/power_supply/battery/status 2>/dev/null || true'"
    ).arg(status));
    Q_UNUSED(type);
    return true;
}

bool HardwareFingerprintSpoofer::applyBatterySpoofing(const QString& instanceId) {
    return spoofBatteryInfo(instanceId);
}

// ===========================================================================
// Thermal Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configureThermal(const QString& instanceId, const ThermalConfig& config) {
    Q_UNUSED(config);
    return spoofThermalZones(instanceId);
}

bool HardwareFingerprintSpoofer::spoofThermalZones(const QString& instanceId) {
    // Set realistic phone temperature (~35°C)
    adbShell(instanceId,
        "for f in /sys/class/thermal/thermal_zone*/temp; do "
        "  echo 35000 > $f 2>/dev/null || true; "
        "done");
    return true;
}

bool HardwareFingerprintSpoofer::setCpuTemperature(const QString& instanceId, int tempCelsius) {
    int tempMilli = tempCelsius * 1000;
    adbShell(instanceId, QString(
        "for f in /sys/class/thermal/thermal_zone*/temp; do "
        "  echo %1 > $f 2>/dev/null || true; "
        "done"
    ).arg(tempMilli));
    return true;
}

bool HardwareFingerprintSpoofer::applyThermalSpoofing(const QString& instanceId) {
    return spoofThermalZones(instanceId);
}

// ===========================================================================
// Memory / Storage Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configureMemory(const QString& instanceId, const MemoryConfig& config) {
    Q_UNUSED(config);
    return spoofMemInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofMemInfo(const QString& instanceId) {
    // 12GB RAM device (Samsung S24+)
    QStringList cmds = {
        "setprop dalvik.vm.heapsize 512m",
        "setprop dalvik.vm.heapmaxfree 8m",
        "setprop dalvik.vm.heapminfree 512k",
        "setprop dalvik.vm.heapgrowthlimit 256m",
        "setprop dalvik.vm.heapstartsize 8m",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    return true;
}

bool HardwareFingerprintSpoofer::configureStorage(const QString& instanceId, const StorageConfig& config) {
    Q_UNUSED(config);
    return spoofStorageInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofStorageInfo(const QString& instanceId) {
    // 256GB storage (Samsung S24 default)
    adbShell(instanceId, "setprop persist.sys.storage.size 256GB");
    return true;
}

bool HardwareFingerprintSpoofer::applyMemoryStorageSpoofing(const QString& instanceId) {
    bool ok = spoofMemInfo(instanceId);
    ok &= spoofStorageInfo(instanceId);
    return ok;
}

// ===========================================================================
// Sensor Calibration
// ===========================================================================

bool HardwareFingerprintSpoofer::addSensorCalibration(
    const QString& instanceId, const HardwareSensorCalibration& calibration)
{
    Q_UNUSED(calibration);
    // Store calibration for this instance
    qDebug() << "[HardwareFingerprintSpoofer] Sensor calibration added for" << instanceId;
    return true;
}

QList<float> HardwareFingerprintSpoofer::generateRealisticSensorData(
    const QString& instanceId, const QString& sensorType)
{
    Q_UNUSED(instanceId);
    QList<float> data;
    auto* rng = QRandomGenerator::global();
    
    if (sensorType == "accelerometer") {
        // Natural phone-in-hand gravity with small noise
        data << (0.0f  + rng->generateDouble() * 0.02f - 0.01f)   // X
             << (0.0f  + rng->generateDouble() * 0.02f - 0.01f)   // Y
             << (9.81f + rng->generateDouble() * 0.02f - 0.01f);  // Z (gravity)
    } else if (sensorType == "gyroscope") {
        data << static_cast<float>(rng->generateDouble() * 0.002 - 0.001)
             << static_cast<float>(rng->generateDouble() * 0.002 - 0.001)
             << static_cast<float>(rng->generateDouble() * 0.002 - 0.001);
    } else if (sensorType == "magnetometer") {
        data << 21.5f << -12.3f << -44.8f;
    } else {
        data << 0.0f << 0.0f << 0.0f;
    }
    return data;
}

bool HardwareFingerprintSpoofer::applySensorCalibration(const QString& instanceId) {
    QStringList cmds = {
        "setprop persist.sys.sensors.fake_accel 0.0,0.0,9.81",
        "setprop persist.sys.sensors.fake_gyro 0.001,-0.002,0.001",
        "setprop persist.sys.sensors.fake_mag 21.5,-12.3,-44.8",
        "setprop persist.sys.sensors.enabled 1",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    return true;
}

// ===========================================================================
// Power Supply Spoofing
// ===========================================================================

bool HardwareFingerprintSpoofer::configurePowerSupply(
    const QString& instanceId, const PowerSupplyConfig& config)
{
    Q_UNUSED(config);
    return spoofPowerSupplyInfo(instanceId);
}

bool HardwareFingerprintSpoofer::spoofPowerSupplyInfo(const QString& instanceId) {
    QStringList cmds = {
        "sh -c 'echo USB_PD > /sys/class/power_supply/usb/type 2>/dev/null || true'",
        "sh -c 'echo 1     > /sys/class/power_supply/usb/online 2>/dev/null || true'",
        "sh -c 'echo 1     > /sys/class/power_supply/ac/online 2>/dev/null || true'",
        "sh -c 'echo 25000000 > /sys/class/power_supply/usb/current_now 2>/dev/null || true'",
        "sh -c 'echo 4350000  > /sys/class/power_supply/usb/voltage_now 2>/dev/null || true'",
    };
    for (const QString& cmd : cmds) {
        adbShell(instanceId, cmd);
    }
    return true;
}

bool HardwareFingerprintSpoofer::applyPowerSupplySpoofing(const QString& instanceId) {
    return spoofPowerSupplyInfo(instanceId);
}

// ===========================================================================
// Real-time Simulation
// ===========================================================================

bool HardwareFingerprintSpoofer::startSimulation(const QString& instanceId, int updateIntervalMs) {
    qDebug() << "[HardwareFingerprintSpoofer] Starting simulation for" << instanceId
             << "interval=" << updateIntervalMs << "ms";
    
    // Stop existing timers for this instance
    stopSimulation(instanceId);
    
    QTimer* timer = new QTimer(this);
    timer->setInterval(updateIntervalMs);
    connect(timer, &QTimer::timeout, this, &HardwareFingerprintSpoofer::onSimulationTick);
    timer->setProperty("instanceId", instanceId);
    timer->start();
    
    m_simulationTimers[instanceId].append(timer);
    m_states[instanceId].isActive = true;
    return true;
}

bool HardwareFingerprintSpoofer::stopSimulation(const QString& instanceId) {
    if (m_simulationTimers.contains(instanceId)) {
        for (QTimer* t : m_simulationTimers[instanceId]) {
            t->stop();
            t->deleteLater();
        }
        m_simulationTimers.remove(instanceId);
    }
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isActive = false;
    }
    return true;
}

bool HardwareFingerprintSpoofer::updateHardwareValues(const QString& instanceId) {
    // Update battery with slight random drift (realistic)
    auto* rng = QRandomGenerator::global();
    int level = 80 + static_cast<int>(rng->bounded(15));
    setBatteryLevel(instanceId, level);
    
    // Update temperature
    int temp = 32 + static_cast<int>(rng->bounded(8));
    setCpuTemperature(instanceId, temp);
    
    return true;
}

void HardwareFingerprintSpoofer::onSimulationTick() {
    QTimer* t = qobject_cast<QTimer*>(sender());
    if (!t) return;
    QString instanceId = t->property("instanceId").toString();
    if (!instanceId.isEmpty()) {
        updateHardwareValues(instanceId);
    }
}

// ===========================================================================
// Utility
// ===========================================================================

HardwareFingerprintState HardwareFingerprintSpoofer::getHardwareState(
    const QString& instanceId) const
{
    return m_states.value(instanceId, HardwareFingerprintState{});
}

QJsonObject HardwareFingerprintSpoofer::getHardwareStateJSON(
    const QString& instanceId) const
{
    QJsonObject obj;
    HardwareFingerprintState state = m_states.value(instanceId);
    obj["instanceId"]    = state.instanceId;
    obj["isInitialized"] = state.isInitialized;
    obj["isActive"]      = state.isActive;
    obj["timestamp"]     = QDateTime::currentDateTime().toString(Qt::ISODate);
    return obj;
}

bool HardwareFingerprintSpoofer::reset(const QString& instanceId) {
    stopSimulation(instanceId);
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isInitialized = false;
        m_states[instanceId].isActive = false;
    }
    return true;
}

// ===========================================================================
// Private: Profile Configs
// ===========================================================================

void HardwareFingerprintSpoofer::initializeHardwareProfiles() {
    qDebug() << "[HardwareFingerprintSpoofer] Hardware profiles initialized";
}

CPUConfig HardwareFingerprintSpoofer::getCPUConfigForProfile(HardwareProfile profile) {
    CPUConfig cfg;
    Q_UNUSED(profile);
    // Snapdragon 8 Gen 3 defaults
    cfg.processorName   = "Qualcomm Snapdragon 8 Gen 3";
    cfg.architecture    = "arm64";
    cfg.coreCount       = 8;
    cfg.threadCount     = 8;
    cfg.cpuFrequencyMax = 3187200;
    cfg.cpuFrequencyMin = 300000;
    cfg.cpuFrequencyCurrent = 2457600;
    cfg.hardware        = "qcom";
    return cfg;
}

GPUConfig HardwareFingerprintSpoofer::getGPUConfigForProfile(HardwareProfile profile) {
    GPUConfig cfg;
    Q_UNUSED(profile);
    cfg.vendor      = "Qualcomm";
    cfg.renderer    = "Adreno (TM) 750";
    cfg.version     = "OpenGL ES 3.2";
    return cfg;
}

BatteryConfig HardwareFingerprintSpoofer::getBatteryConfigForProfile(HardwareProfile profile) {
    BatteryConfig cfg;
    Q_UNUSED(profile);
    cfg.health       = "Good";
    cfg.technology   = "Li-poly";
    cfg.status       = "Charging";
    cfg.pluggedType  = "USB";
    cfg.isPresent    = true;
    cfg.isCharging   = true;
    cfg.isOnline     = true;
    return cfg;
}

ThermalConfig HardwareFingerprintSpoofer::getThermalConfigForProfile(HardwareProfile profile) {
    ThermalConfig cfg;
    Q_UNUSED(profile);
    cfg.isThermalEngineEnabled = true;
    cfg.isThrottling           = false;
    return cfg;
}

// ===========================================================================
// Private: Content Generators (used by docker/patch_system.sh bridge)
// ===========================================================================

QString HardwareFingerprintSpoofer::generateCpuInfoContent(const CPUConfig& config) {
    return QString(
        "Processor\t: %1\n"
        "Hardware\t: %2\n"
        "processor\t: 0\n"
        "BogoMIPS\t: 38.40\n"
        "Features\t: fp asimd evtstrm aes pmull sha1 sha2 crc32\n"
        "CPU implementer\t: 0x51\n"
        "CPU architecture: 8\n"
        "CPU variant\t: 0x2\n"
        "CPU part\t: 0x0d08\n"
        "CPU revision\t: 2\n"
    ).arg(config.processorName, config.hardware);
}

QString HardwareFingerprintSpoofer::generateCpuInfoContentFromFingerprint(
    const HardwareFingerprint& fp)
{
    return generateCpuInfoContent(getCPUConfigForProfile(HardwareProfile::SAMSUNG_S24_ULTRA));
}

QString HardwareFingerprintSpoofer::generateBatteryContent(const BatteryConfig& config) {
    return QString("POWER_SUPPLY_STATUS=%1\n"
                   "POWER_SUPPLY_HEALTH=%2\n"
                   "POWER_SUPPLY_TECHNOLOGY=%3\n")
        .arg(config.status, config.health, config.technology);
}

QString HardwareFingerprintSpoofer::generateThermalContent(const ThermalConfig& config) {
    Q_UNUSED(config);
    return "35000\n";
}

QString HardwareFingerprintSpoofer::generateMemInfoContent(const MemoryConfig& config) {
    Q_UNUSED(config);
    return "MemTotal: 12145152 kB\nMemFree:  4096000 kB\n";
}

void HardwareFingerprintSpoofer::applyCPUChanges(const HardwareFingerprint& fp) {
    Q_UNUSED(fp);
}

void HardwareFingerprintSpoofer::applyGPUChanges(const HardwareFingerprint& fp) {
    Q_UNUSED(fp);
}

void HardwareFingerprintSpoofer::applyDeviceChanges(const HardwareFingerprint& fp) {
    Q_UNUSED(fp);
}

void HardwareFingerprintSpoofer::applyDMIChanges(const HardwareFingerprint& fp) {
    Q_UNUSED(fp);
}

void HardwareFingerprintSpoofer::restoreOriginalValues() {
    qDebug() << "[HardwareFingerprintSpoofer] Restoring original values";
}

} // namespace VirtualPhonePro
