/**
 * @file HardwareFingerprintSpoofer.cpp
 * @brief Hardware Fingerprint Spoofer Implementation
 */

#include "VirtualPhonePro/HardwareFingerprintSpoofer.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

HardwareFingerprintSpoofer* HardwareFingerprintSpoofer::s_instance = nullptr;

HardwareFingerprintSpoofer& HardwareFingerprintSpoofer::instance() {
    if (!s_instance) {
        s_instance = new HardwareFingerprintSpoofer();
    }
    return *s_instance;
}

HardwareFingerprintSpoofer::HardwareFingerprintSpoofer() {
    initializeHardwareProfiles();
}

HardwareFingerprintSpoofer::~HardwareFingerprintSpoofer() {
    for (auto& timers : m_simulationTimers) {
        for (auto* timer : timers) {
            timer->stop();
            delete timer;
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool HardwareFingerprintSpoofer::initialize(const QString& instanceId) {
    qDebug() << "Initializing HardwareFingerprintSpoofer for:" << instanceId;
    
    HardwareFingerprintState state;
    state.instanceId = instanceId;
    state.isInitialized = false;
    state.isActive = false;
    
    // Default to Samsung S24 Ultra profile
    state.cpu = getCPUConfigForProfile(HardwareProfile::SAMSUNG_S24_ULTRA);
    state.gpu = getGPUConfigForProfile(HardwareProfile::SAMSUNG_S24_ULTRA);
    state.battery = getBatteryConfigForProfile(HardwareProfile::SAMSUNG_S24_ULTRA);
    state.thermal = getThermalConfigForProfile(HardwareProfile::SAMSUNG_S24_ULTRA);
    
    m_states[instanceId] = state;
    
    return applyAllSpoofing(instanceId);
}

bool HardwareFingerprintSpoofer::applyProfile(const QString& instanceId, HardwareProfile profile) {
    if (!m_states.contains(instanceId)) {
        initialize(instanceId);
    }
    
    HardwareFingerprintState& state = m_states[instanceId];
    state.cpu = getCPUConfigForProfile(profile);
    state.gpu = getGPUConfigForProfile(profile);
    state.battery = getBatteryConfigForProfile(profile);
    state.thermal = getThermalConfigForProfile(profile);
    
    return applyAllSpoofing(instanceId);
}

bool HardwareFingerprintSpoofer::applyAllSpoofing(const QString& instanceId) {
    bool success = true;
    
    success &= applyCPUSpoofing(instanceId);
    success &= applyGPUSpoofing(instanceId);
    success &= applyBatterySpoofing(instanceId);
    success &= applyThermalSpoofing(instanceId);
    success &= applyMemoryStorageSpoofing(instanceId);
    success &= applyPowerSupplySpoofing(instanceId);
    success &= applySensorCalibration(instanceId);
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isInitialized = true;
        m_states[instanceId].isActive = true;
    }
    
    return success;
}

// ============================================================================
// CPU Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configureCPU(const QString& instanceId, const CPUConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].cpu = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofCpuInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const CPUConfig& cpu = m_states[instanceId].cpu;
    QString cpuInfo = generateCpuInfoContent(cpu);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create overlay directory
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware");
    
    // Write spoofed /proc/cpuinfo
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/cpuinfo << 'CPUINFO'\n%1\nCPUINFO").arg(cpuInfo));
    
    // Also try to write to actual /proc/cpuinfo if writable
    ctrl.executeShell(instanceId, QString("cat > /proc/cpuinfo << 'CPUINFO'\n%1\nCPUINFO 2>/dev/null || true").arg(cpuInfo));
    
    qDebug() << "Spoofed /proc/cpuinfo";
    return true;
}

bool HardwareFingerprintSpoofer::spoofCpuFrequency(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const CPUConfig& cpu = m_states[instanceId].cpu;
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set CPU frequencies
    ctrl.executeShell(instanceId, QString("mkdir -p /data/local/tmp/vpp_hardware/cpuinfo"));
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/cpuinfo_max_freq << 'EOF'\n%1\nEOF").arg(cpu.cpuFrequencyMax));
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/cpuinfo_min_freq << 'EOF'\n%1\nEOF").arg(cpu.cpuFrequencyMin));
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/cpuinfo_cur_freq << 'EOF'\n%1\nEOF").arg(cpu.cpuFrequencyCurrent));
    
    return true;
}

bool HardwareFingerprintSpoofer::setCpuGovernor(const QString& instanceId, const QString& governor) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set CPU governor for all cores
    for (int i = 0; i < 8; i++) {
        ctrl.executeShell(instanceId, QString("echo '%1' > /sys/devices/system/cpu/cpu%2/cpufreq/scaling_governor 2>/dev/null || true").arg(governor).arg(i));
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::applyCPUSpoofing(const QString& instanceId) {
    bool success = true;
    success &= spoofCpuInfo(instanceId);
    success &= spoofCpuFrequency(instanceId);
    success &= setCpuGovernor(instanceId, "schedutil");
    return success;
}

// ============================================================================
// GPU Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configureGPU(const QString& instanceId, const GPUConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].gpu = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofGPUInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const GPUConfig& gpu = m_states[instanceId].gpu;
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set GPU properties
    QStringList commands = {
        QString("setprop ro.hardware.gpu.vendor %1").arg(gpu.vendor),
        QString("setprop ro.hardware.gpu.renderer %1").arg(gpu.renderer),
        QString("setprop ro.hardware.gpu.version %1").arg(gpu.version),
        QString("setprop ro.opengles.version %1").arg("0x00030002"),
        QString("setprop debug.sf.backend 1"),
        
        // GPU sysfs
        "mkdir -p /data/local/tmp/vpp_hardware",
        QString("cat > /data/local/tmp/vpp_hardware/gpu_info << 'GPUINFO'\n%1\n%2\n%3\nGPUINFO").arg(gpu.vendor, gpu.renderer, gpu.version),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::spoofGraphicsInfo(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set OpenGL/Vulkan properties
    QStringList commands = {
        "setprop ro.opengles.version 196609",  // OpenGL ES 3.2
        "setprop debug.opengles.version 196609",
        "setprop ro.surface_flinger.use_glES 3",
        "setprop debug.gralloc.enabled_udp 1",
        
        // Vulkan
        "setprop ro.hardware.vulkan VK_1_1",
        "setprop ro.vulkan.api_version 1.1",
        
        // GPU memory
        "setprop ro.gpu.available_memory 8589934592",  // 8GB
        "setprop ro.gpu.max_total_memory 8589934592",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::applyGPUSpoofing(const QString& instanceId) {
    bool success = true;
    success &= spoofGPUInfo(instanceId);
    success &= spoofGraphicsInfo(instanceId);
    return success;
}

// ============================================================================
// Battery Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configureBattery(const QString& instanceId, const BatteryConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].battery = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofBatteryInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const BatteryConfig& battery = m_states[instanceId].battery;
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create battery overlay
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware/battery");
    
    // Write battery power supply info
    QString batteryContent = generateBatteryContent(battery);
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/battery/battery_info << 'BATT'\n%1\nBATT").arg(batteryContent));
    
    // Write to actual sysfs if possible
    ctrl.executeShell(instanceId, QString("cat > /sys/class/power_supply/battery/present << 'BATT'\n%1\nBATT 2>/dev/null || true").arg(battery.isPresent ? "1" : "0"));
    ctrl.executeShell(instanceId, QString("cat > /sys/class/power_supply/battery/capacity << 'BATT'\n%1\nBATT 2>/dev/null || true").arg(battery.levelPercent));
    ctrl.executeShell(instanceId, QString("cat > /sys/class/power_supply/battery/status << 'BATT'\n%1\nBATT 2>/dev/null || true").arg(battery.status));
    
    return true;
}

bool HardwareFingerprintSpoofer::setBatteryLevel(const QString& instanceId, int level) {
    if (!m_states.contains(instanceId)) return false;
    
    m_states[instanceId].battery.levelPercent = level;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("dumpsys battery set level %1").arg(level));
    
    return true;
}

bool HardwareFingerprintSpoofer::setChargingState(const QString& instanceId, bool charging, const QString& type) {
    if (!m_states.contains(instanceId)) return false;
    
    m_states[instanceId].battery.isCharging = charging;
    m_states[instanceId].battery.pluggedType = type;
    m_states[instanceId].battery.status = charging ? "Charging" : "Discharging";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (charging) {
        if (type == "AC") {
            ctrl.executeShell(instanceId, "dumpsys battery set ac 1");
        } else if (type == "USB") {
            ctrl.executeShell(instanceId, "dumpsys battery set usb 1");
        } else if (type == "Wireless") {
            ctrl.executeShell(instanceId, "dumpsys battery set wireless 1");
        }
        ctrl.executeShell(instanceId, "dumpsys battery set status charging");
    } else {
        ctrl.executeShell(instanceId, "dumpsys battery unplug");
        ctrl.executeShell(instanceId, "dumpsys battery set status discharging");
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::applyBatterySpoofing(const QString& instanceId) {
    bool success = true;
    success &= spoofBatteryInfo(instanceId);
    
    // Also use dumpsys for Android battery service
    if (m_states.contains(instanceId)) {
        const BatteryConfig& battery = m_states[instanceId].battery;
        ReDroidController& ctrl = ReDroidController::instance();
        
        ctrl.executeShell(instanceId, QString("dumpsys battery set level %1").arg(battery.levelPercent));
        ctrl.executeShell(instanceId, QString("dumpsys battery set health %1").arg(battery.health));
        ctrl.executeShell(instanceId, QString("dumpsys battery set temperature %1").arg(battery.temperature));
        ctrl.executeShell(instanceId, QString("dumpsys battery set voltage %1").arg(battery.voltage));
        ctrl.executeShell(instanceId, QString("dumpsys battery set capacity %1").arg(battery.capacityMah));
    }
    
    return success;
}

// ============================================================================
// Thermal Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configureThermal(const QString& instanceId, const ThermalConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].thermal = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofThermalZones(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const ThermalConfig& thermal = m_states[instanceId].thermal;
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create thermal overlay
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware/thermal");
    
    // Write thermal zone data
    QString thermalContent = generateThermalContent(thermal);
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/thermal_info << 'THERMAL'\n%1\nTHERMAL").arg(thermalContent));
    
    // Write to actual thermal zones
    for (auto it = thermal.thermalZones.begin(); it != thermal.thermalZones.end(); ++it) {
        QString zonePath = QString("/sys/class/thermal/%1/temp").arg(it.key());
        ctrl.executeShell(instanceId, QString("cat > %1 << 'TZ'\n%2\nTZ 2>/dev/null || true").arg(zonePath).arg(it.value() * 1000));
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::setCpuTemperature(const QString& instanceId, int tempCelsius) {
    if (!m_states.contains(instanceId)) return false;
    
    m_states[instanceId].thermal.cpuTemp = tempCelsius;
    m_states[instanceId].thermal.thermalZones["cpu-thermal"] = tempCelsius;
    
    return spoofThermalZones(instanceId);
}

bool HardwareFingerprintSpoofer::applyThermalSpoofing(const QString& instanceId) {
    return spoofThermalZones(instanceId);
}

// ============================================================================
// Memory & Storage Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configureMemory(const QString& instanceId, const MemoryConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].memory = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofMemInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const MemoryConfig& mem = m_states[instanceId].memory;
    QString memInfo = generateMemInfoContent(mem);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware");
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/meminfo << 'MEMINFO'\n%1\nMEMINFO").arg(memInfo));
    ctrl.executeShell(instanceId, QString("cat > /proc/meminfo << 'MEMINFO'\n%1\nMEMINFO 2>/dev/null || true").arg(memInfo));
    
    return true;
}

bool HardwareFingerprintSpoofer::configureStorage(const QString& instanceId, const StorageConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].storage = config;
    return true;
}

bool HardwareFingerprintSpoofer::spoofStorageInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    const StorageConfig& storage = m_states[instanceId].storage;
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware");
    
    // Write storage info
    QString storageInfo = QString(
        "Filesystem: %1\n"
        "Total: %2 bytes\n"
        "Available: %3 bytes\n"
        "Used: %4 bytes\n"
        "Path: %5\n"
    ).arg(storage.fileSystem)
     .arg(storage.totalStorageBytes)
     .arg(storage.availableStorageBytes)
     .arg(storage.usedStorageBytes)
     .arg(storage.storagePath);
    
    ctrl.executeShell(instanceId, QString("cat > /data/local/tmp/vpp_hardware/storage_info << 'STORAGE'\n%1\nSTORAGE").arg(storageInfo));
    
    return true;
}

bool HardwareFingerprintSpoofer::applyMemoryStorageSpoofing(const QString& instanceId) {
    bool success = true;
    success &= spoofMemInfo(instanceId);
    success &= spoofStorageInfo(instanceId);
    return success;
}

// ============================================================================
// Sensor Calibration
// ============================================================================

bool HardwareFingerprintSpoofer::addSensorCalibration(const QString& instanceId, const SensorCalibration& calibration) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].sensorCalibrations.append(calibration);
    return true;
}

QList<float> HardwareFingerprintSpoofer::generateRealisticSensorData(const QString& instanceId, const QString& sensorType) {
    QList<float> data;
    
    if (!m_states.contains(instanceId)) return data;
    
    // Find calibration for this sensor
    SensorCalibration calib;
    for (const auto& c : m_states[instanceId].sensorCalibrations) {
        if (c.sensorName == sensorType) {
            calib = c;
            break;
        }
    }
    
    QRandomGenerator* gen = QRandomGenerator::global();
    
    if (sensorType == "accelerometer") {
        // Real accelerometer data with noise
        float baseX = 0.0f + calib.offsetX + (gen->bounded(-100, 100) / 1000.0f);
        float baseY = 0.0f + calib.offsetY + (gen->bounded(-100, 100) / 1000.0f);
        float baseZ = 9.81f + calib.offsetZ + (gen->bounded(-100, 100) / 1000.0f);
        
        data.append(baseX * calib.scaleX);
        data.append(baseY * calib.scaleY);
        data.append(baseZ * calib.scaleZ);
    } else if (sensorType == "gyroscope") {
        // Real gyroscope data
        data.append(calib.offsetX + (gen->bounded(-50, 50) / 1000.0f));
        data.append(calib.offsetY + (gen->bounded(-50, 50) / 1000.0f));
        data.append(calib.offsetZ + (gen->bounded(-50, 50) / 1000.0f));
    } else if (sensorType == "magnetic_field") {
        // Real magnetometer data
        data.append(25.0f + calib.offsetX + (gen->bounded(-500, 500) / 100.0f));
        data.append(-10.0f + calib.offsetY + (gen->bounded(-500, 500) / 100.0f));
        data.append(45.0f + calib.offsetZ + (gen->bounded(-500, 500) / 100.0f));
    }
    
    return data;
}

bool HardwareFingerprintSpoofer::applySensorCalibration(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create calibration data for Android sensors
    for (const auto& calib : m_states[instanceId].sensorCalibrations) {
        QString sensorPath = QString("/data/local/tmp/vpp_hardware/sensor_%1.cal").arg(calib.sensorName);
        
        QString calibData = QString(
            "sensor=%1\n"
            "offset_x=%2\n"
            "offset_y=%3\n"
            "offset_z=%4\n"
            "scale_x=%5\n"
            "scale_y=%6\n"
            "scale_z=%7\n"
            "accuracy=%8\n"
            "calibration_date=%9\n"
        ).arg(calib.sensorName)
         .arg(calib.offsetX).arg(calib.offsetY).arg(calib.offsetZ)
         .arg(calib.scaleX).arg(calib.scaleY).arg(calib.scaleZ)
         .arg(calib.accuracy)
         .arg(calib.calibrationDate.toString(Qt::ISODate));
        
        ctrl.executeShell(instanceId, QString("cat > %1 << 'CALIB'\n%2\nCALIB").arg(sensorPath).arg(calibData));
    }
    
    return true;
}

// ============================================================================
// Power Supply Spoofing
// ============================================================================

bool HardwareFingerprintSpoofer::configurePowerSupply(const QString& instanceId, const PowerSupplyConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = HardwareFingerprintState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].powerSupplies.append(config);
    return true;
}

bool HardwareFingerprintSpoofer::spoofPowerSupplyInfo(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "mkdir -p /data/local/tmp/vpp_hardware/power_supply");
    
    for (const auto& ps : m_states[instanceId].powerSupplies) {
        QString psPath = QString("/data/local/tmp/vpp_hardware/power_supply/%1").arg(ps.name);
        
        QString psData = QString(
            "name=%1\n"
            "type=%2\n"
            "scope=%3\n"
            "status=%4\n"
            "capacity=%5\n"
            "voltage_now=%6\n"
            "current_now=%7\n"
            "online=%8\n"
            "present=%9\n"
        ).arg(ps.name).arg(ps.type).arg(ps.scope).arg(ps.status)
         .arg(ps.capacityPercent).arg(ps.voltageNow).arg(ps.currentNow)
         .arg(ps.online ? "1" : "0").arg(ps.present ? "1" : "0");
        
        ctrl.executeShell(instanceId, QString("cat > %1 << 'PSDATA'\n%2\nPSDATA").arg(psPath).arg(psData));
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::applyPowerSupplySpoofing(const QString& instanceId) {
    return spoofPowerSupplyInfo(instanceId);
}

// ============================================================================
// Real-time Updates
// ============================================================================

bool HardwareFingerprintSpoofer::startSimulation(const QString& instanceId, int updateIntervalMs) {
    if (!m_states.contains(instanceId)) return false;
    
    // Create timer for periodic updates
    QTimer* timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &HardwareFingerprintSpoofer::onSimulationTick);
    timer->start(updateIntervalMs);
    
    m_simulationTimers[instanceId].append(timer);
    
    m_states[instanceId].isActive = true;
    
    qDebug() << "Started hardware simulation for" << instanceId;
    return true;
}

bool HardwareFingerprintSpoofer::stopSimulation(const QString& instanceId) {
    if (m_simulationTimers.contains(instanceId)) {
        for (auto* timer : m_simulationTimers[instanceId]) {
            timer->stop();
            delete timer;
        }
        m_simulationTimers[instanceId].clear();
    }
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isActive = false;
    }
    
    return true;
}

bool HardwareFingerprintSpoofer::updateHardwareValues(const QString& instanceId) {
    if (!m_states.contains(instanceId)) return false;
    
    // Add realistic variations to sensor data
    QRandomGenerator* gen = QRandomGenerator::global();
    
    // Slightly vary temperature
    if (m_states[instanceId].thermal.cpuTemp > 0) {
        int tempVariation = gen->bounded(-2, 3);
        m_states[instanceId].thermal.cpuTemp += tempVariation;
        m_states[instanceId].thermal.cpuTemp = qBound(30, m_states[instanceId].thermal.cpuTemp, 55);
    }
    
    // Slightly vary battery level
    if (m_states[instanceId].battery.levelPercent > 0) {
        // Natural drain simulation (very slow)
        if (gen->bounded(100) < 1) {  // ~1% chance per update
            m_states[instanceId].battery.levelPercent--;
        }
    }
    
    // Re-apply spoofing with new values
    applyThermalSpoofing(instanceId);
    applyBatterySpoofing(instanceId);
    
    emit hardwareValueUpdated(instanceId, "thermal");
    emit hardwareValueUpdated(instanceId, "battery");
    
    return true;
}

void HardwareFingerprintSpoofer::onSimulationTick() {
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;
    
    QString instanceId;
    for (auto it = m_simulationTimers.begin(); it != m_simulationTimers.end(); ++it) {
        if (it.value().contains(timer)) {
            instanceId = it.key();
            break;
        }
    }
    
    if (!instanceId.isEmpty()) {
        updateHardwareValues(instanceId);
    }
}

// ============================================================================
// Utility
// ============================================================================

HardwareFingerprintState HardwareFingerprintSpoofer::getHardwareState(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    HardwareFingerprintState defaultState;
    defaultState.instanceId = instanceId;
    return defaultState;
}

QJsonObject HardwareFingerprintSpoofer::getHardwareStateJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (!m_states.contains(instanceId)) {
        json["error"] = "Instance not found";
        return json;
    }
    
    const HardwareFingerprintState& state = m_states[instanceId];
    
    // CPU
    QJsonObject cpu;
    cpu["processor"] = state.cpu.processorName;
    cpu["cores"] = state.cpu.coreCount;
    cpu["threads"] = state.cpu.threadCount;
    cpu["architecture"] = state.cpu.architecture;
    json["cpu"] = cpu;
    
    // GPU
    QJsonObject gpu;
    gpu["vendor"] = state.gpu.vendor;
    gpu["renderer"] = state.gpu.renderer;
    gpu["version"] = state.gpu.version;
    json["gpu"] = gpu;
    
    // Battery
    QJsonObject battery;
    battery["level"] = state.battery.levelPercent;
    battery["health"] = state.battery.health;
    battery["temperature"] = state.battery.temperature;
    battery["status"] = state.battery.status;
    json["battery"] = battery;
    
    // Thermal
    QJsonObject thermal;
    thermal["cpuTemp"] = state.thermal.cpuTemp;
    thermal["isThrottling"] = state.thermal.isThrottling;
    json["thermal"] = thermal;
    
    // Memory
    QJsonObject memory;
    memory["totalKb"] = state.memory.totalMemoryKb;
    memory["availableKb"] = state.memory.availableMemoryKb;
    json["memory"] = memory;
    
    json["isInitialized"] = state.isInitialized;
    json["isActive"] = state.isActive;
    
    return json;
}

bool HardwareFingerprintSpoofer::reset(const QString& instanceId) {
    stopSimulation(instanceId);
    m_states.remove(instanceId);
    return true;
}

// ============================================================================
// Private Helpers
// ============================================================================

void HardwareFingerprintSpoofer::initializeHardwareProfiles() {
    // Pre-configured hardware profiles are loaded via getCPUConfigForProfile, etc.
}

CPUConfig HardwareFingerprintSpoofer::getCPUConfigForProfile(HardwareProfile profile) {
    CPUConfig config;
    
    switch (profile) {
        case HardwareProfile::SAMSUNG_S24_ULTRA:
            config.processorName = "ARMv8 Processor";
            config.architecture = "aarch64";
            config.coreCount = 8;
            config.threadCount = 8;
            config.cpuImplementer = "0x51";
            config.cpuVariant = "0x1";
            config.cpuPart = "0xc00";
            config.cpuRevision = "1";
            config.hardware = "qcom";
            config.cpuFrequencyMin = 300000000;
            config.cpuFrequencyMax = 3300000000;
            config.cpuFrequencyCurrent = 1800000000;
            config.bogoMips = "486.40";
            config.features = "fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid";
            break;
            
        case HardwareProfile::GOOGLE_PIXEL_8_PRO:
            config.processorName = "ARMv8 Processor";
            config.architecture = "aarch64";
            config.coreCount = 8;
            config.threadCount = 8;
            config.cpuImplementer = "0x48";
            config.cpuVariant = "0x1";
            config.cpuPart = "0x411";
            config.cpuRevision = "13";
            config.hardware = "gs101";
            config.cpuFrequencyMin = 300000000;
            config.cpuFrequencyMax = 3000000000;
            config.cpuFrequencyCurrent = 1700000000;
            config.bogoMips = "480.00";
            config.features = "fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp";
            break;
            
        case HardwareProfile::XIAOMI_14_PRO:
            config.processorName = "ARMv8 Processor";
            config.architecture = "aarch64";
            config.coreCount = 8;
            config.threadCount = 8;
            config.cpuImplementer = "0x51";
            config.cpuVariant = "0x2";
            config.cpuPart = "0xc00";
            config.cpuRevision = "1";
            config.hardware = "qcom";
            config.cpuFrequencyMin = 300000000;
            config.cpuFrequencyMax = 3200000000;
            config.cpuFrequencyCurrent = 1900000000;
            config.bogoMips = "490.00";
            config.features = "fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid";
            break;
            
        default:
            config.processorName = "ARMv8 Processor";
            config.architecture = "aarch64";
            config.coreCount = 8;
            config.threadCount = 8;
            config.cpuFrequencyMin = 300000000;
            config.cpuFrequencyMax = 3000000000;
            config.cpuFrequencyCurrent = 1800000000;
            config.bogoMips = "480.00";
            break;
    }
    
    return config;
}

GPUConfig HardwareFingerprintSpoofer::getGPUConfigForProfile(HardwareProfile profile) {
    GPUConfig config;
    
    switch (profile) {
        case HardwareProfile::SAMSUNG_S24_ULTRA:
            config.vendor = "Qualcomm";
            config.renderer = "Adreno (TM) 750";
            config.version = "3.2.v@09372773";
            config.driverVersion = "v0332.75";
            config.coreCount = 768;
            config.maxClock = 900;
            config.minClock = 265;
            config.architecture = "A7xx";
            config.openGLVersion = "3.2";
            config.vulkanVersion = "1.1";
            config.openCLVersion = "2.0";
            break;
            
        case HardwareProfile::GOOGLE_PIXEL_8_PRO:
            config.vendor = "ARM";
            config.renderer = "Mali-G710 v11";
            config.version = "1.2";
            config.driverVersion = "v11r0";
            config.coreCount = 20;
            config.maxClock = 850;
            config.minClock = 200;
            config.architecture = "Valhall";
            config.openGLVersion = "3.2";
            config.vulkanVersion = "1.1";
            config.openCLVersion = "2.0";
            break;
            
        case HardwareProfile::XIAOMI_14_PRO:
            config.vendor = "Qualcomm";
            config.renderer = "Adreno (TM) 750";
            config.version = "3.2.v@09372773";
            config.driverVersion = "v0332.75";
            config.coreCount = 768;
            config.maxClock = 900;
            config.minClock = 265;
            config.architecture = "A7xx";
            config.openGLVersion = "3.2";
            config.vulkanVersion = "1.1";
            config.openCLVersion = "2.0";
            break;
            
        default:
            config.vendor = "Qualcomm";
            config.renderer = "Adreno (TM) 740";
            config.version = "3.2.v@09372773";
            config.coreCount = 512;
            config.maxClock = 800;
            break;
    }
    
    return config;
}

BatteryConfig HardwareFingerprintSpoofer::getBatteryConfigForProfile(HardwareProfile profile) {
    BatteryConfig config;
    
    config.levelPercent = 75;
    config.healthPercent = 95;
    config.health = "good";
    config.technology = "Li-ion";
    config.status = "Discharging";
    config.pluggedType = "Not Charging";
    config.cycleCount = 150;
    config.temperature = 320;
    config.voltage = 4200;
    config.isPresent = true;
    config.isCharging = false;
    config.isOnline = false;
    
    switch (profile) {
        case HardwareProfile::SAMSUNG_S24_ULTRA:
            config.capacityMah = 5000;
            config.currentChargeMah = 3750;
            config.modelName = "EB-BS928ABY";
            config.manufacturer = "Samsung SDI";
            break;
            
        case HardwareProfile::GOOGLE_PIXEL_8_PRO:
            config.capacityMah = 5000;
            config.currentChargeMah = 3750;
            config.modelName = "G0233";
            config.manufacturer = "ATL";
            break;
            
        case HardwareProfile::XIAOMI_14_PRO:
            config.capacityMah = 4880;
            config.currentChargeMah = 3660;
            config.modelName = "BM50";
            config.manufacturer = "Sunwoda";
            break;
            
        default:
            config.capacityMah = 5000;
            config.currentChargeMah = 3750;
            config.modelName = "Generic";
            config.manufacturer = "Generic";
            break;
    }
    
    return config;
}

ThermalConfig HardwareFingerprintSpoofer::getThermalConfigForProfile(HardwareProfile profile) {
    ThermalConfig config;
    
    config.isThermalEngineEnabled = true;
    config.cpuTemp = 35;
    config.batteryTemp = 32;
    config.skinTemp = 30;
    config.isThrottling = false;
    
    config.thermalZones["cpu-thermal"] = 35;
    config.thermalZones["battery-thermal"] = 32;
    config.thermalZones["skin-thermal"] = 30;
    config.thermalZones["xo-thermal"] = 28;
    
    config.thermalTypes["cpu-thermal"] = "cpu";
    config.thermalTypes["battery-thermal"] = "battery";
    config.thermalTypes["skin-thermal"] = "shell";
    config.thermalTypes["xo-thermal"] = "wifi";
    
    config.cpuThrottlingTemps["cpu-thermal"] = 45;
    config.cpuShutdownTemps["cpu-thermal"] = 60;
    
    return config;
}

QString HardwareFingerprintSpoofer::generateCpuInfoContent(const CPUConfig& config) {
    QString content;
    
    for (int i = 0; i < config.coreCount; i++) {
        content += QString(
            "Processor\t: %1\n"
            "BogoMIPS\t: %2\n"
            "Features\t: %3\n"
            "CPU architecture\t: %4\n"
            "CPU implementer\t: %5\n"
            "CPU variant\t: %6\n"
            "CPU part\t: %7\n"
            "CPU revision\t: %8\n"
            "Hardware\t: %9\n"
        ).arg(config.processorName)
         .arg(config.bogoMips)
         .arg(config.features)
         .arg(config.architecture)
         .arg(config.cpuImplementer)
         .arg(config.cpuVariant)
         .arg(config.cpuPart)
         .arg(config.cpuRevision)
         .arg(config.hardware);
        
        if (i < config.coreCount - 1) {
            content += "\n";
        }
    }
    
    return content;
}

QString HardwareFingerprintSpoofer::generateBatteryContent(const BatteryConfig& config) {
    return QString(
        "Technology: %1\n"
        "Voltage: %2 uV\n"
        "Temperature: %3 cK\n"
        "Status: %4\n"
        "Capacity: %5 %%level\n"
        "Health: %6\n"
        "Present: %7\n"
    ).arg(config.technology)
     .arg(config.voltage)
     .arg(config.temperature)
     .arg(config.status)
     .arg(config.levelPercent)
     .arg(config.health)
     .arg(config.isPresent ? "1" : "0");
}

QString HardwareFingerprintSpoofer::generateThermalContent(const ThermalConfig& config) {
    QString content;
    
    for (auto it = config.thermalZones.begin(); it != config.thermalZones.end(); ++it) {
        content += QString("%1: %2 degC\n").arg(it.key()).arg(it.value());
    }
    
    return content;
}

QString HardwareFingerprintSpoofer::generateMemInfoContent(const MemoryConfig& config) {
    return QString(
        "MemTotal:        %1 kB\n"
        "MemFree:         %2 kB\n"
        "MemAvailable:    %3 kB\n"
        "Buffers:         10000 kB\n"
        "Cached:          %4 kB\n"
        "SwapCached:      0 kB\n"
        "Active:          50000 kB\n"
        "Inactive:        40000 kB\n"
        "Active(anon):    20000 kB\n"
        "Inactive(anon):  10000 kB\n"
        "Active(file):    30000 kB\n"
        "Inactive(file):  30000 kB\n"
        "Unevictable:     0 kB\n"
        "Mlocked:         0 kB\n"
        "LowFree:         %2 kB\n"
        "LowTotal:        %1 kB\n"
        "HighFree:        0 kB\n"
        "HighTotal:       0 kB\n"
        "SwapTotal:       %5 kB\n"
        "SwapFree:        %6 kB\n"
        "Dirty:            0 kB\n"
        "Writeback:        0 kB\n"
        "AnonPages:       18000 kB\n"
        "Mapped:          10000 kB\n"
        "Shmem:           1000 kB\n"
        "KReclaimable:    5000 kB\n"
        "KernelStack:     2000 kB\n"
        "PageTables:      1000 kB\n"
        "NFS_Unstable:    0 kB\n"
        "Bounce:           0 kB\n"
        "WritebackTmp:     0 kB\n"
        "CommitLimit:     %1 kB\n"
        "Committed_AS:    %3 kB\n"
        "VmallocTotal:   2621440 kB\n"
        "VmallocUsed:      5000 kB\n"
        "VmallocChunk:    2000 kB\n"
    ).arg(config.totalMemoryKb)
     .arg(config.totalMemoryKb - config.availableMemoryKb)
     .arg(config.availableMemoryKb)
     .arg((config.totalMemoryKb - config.availableMemoryKb) / 2)
     .arg(config.totalSwapKb)
     .arg(config.freeSwapKb);
}

} // namespace VirtualPhonePro
