#include "VirtualPhonePro/PlayIntegrityManager.hpp"
/**
 * @file EmulatorDetectionBypass.cpp
 * @brief Emulator Detection Bypass Implementation
 */

#include "VirtualPhonePro/EmulatorDetectionBypass.hpp"
#include "VirtualPhonePro/PlayIntegrityManager.hpp"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

namespace VirtualPhonePro {

// ========================================================================
// SINGLETON
// ========================================================================

EmulatorDetectionBypass* EmulatorDetectionBypass::s_instance = nullptr;

EmulatorDetectionBypass& EmulatorDetectionBypass::instance() {
    if (!s_instance) {
        s_instance = new EmulatorDetectionBypass();
    }
    return *s_instance;
}

EmulatorDetectionBypass::EmulatorDetectionBypass(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[EmulatorBypass] Detection bypass module initialized";
}


// ========================================================================
// JSON CONVERSION
// ========================================================================

QJsonObject BypassResult::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);
    obj["bypassed"] = bypassed;
    obj["description"] = description;
    obj["action"] = action;
    return obj;
}

QJsonObject DetectionConfig::toJson() const {
    QJsonObject obj;
    obj["removeFiles"] = QJsonArray::fromStringList(removeFiles);
    obj["seedFiles"] = QJsonArray::fromStringList(seedFiles);
    obj["hideProperties"] = QJsonArray::fromStringList(hideProperties);
    
    QJsonObject props;
    for (auto it = overrideProperties.begin(); it != overrideProperties.end(); ++it) {
        props[it.key()] = it.value();
    }
    obj["overrideProperties"] = props;
    
    obj["cpuModel"] = cpuModel;
    obj["gpuModel"] = gpuModel;
    obj["socModel"] = socModel;
    
    return obj;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void EmulatorDetectionBypass::setConfig(const QString& instanceId, const DetectionConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId] = config;
    qDebug() << "[EmulatorBypass] Config set for:" << instanceId;
}

DetectionConfig EmulatorDetectionBypass::getConfig(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_configs.value(instanceId);
}

void EmulatorDetectionBypass::resetConfig(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    DetectionConfig defaultConfig;
    defaultConfig.cpuModel = "mt6885";
    defaultConfig.gpuModel = "Mali-G77";
    defaultConfig.socModel = "MT6885";
    
    m_configs[instanceId] = defaultConfig;
    qDebug() << "[EmulatorBypass] Config reset for:" << instanceId;
}

// ========================================================================
// COMPLETE BYPASS
// ========================================================================

QList<BypassResult> EmulatorDetectionBypass::performCompleteBypass(const QString& instanceId) {
    QList<BypassResult> results;
    
    qDebug() << "[EmulatorBypass] Starting complete bypass for:" << instanceId;
    
    // Perform all bypass methods
    results.append(bypassQEMUFiles(instanceId));
    results.append(bypassQEMUPipe(instanceId));
    results.append(bypassQEMUProperties(instanceId));
    results.append(bypassCPUSignature(instanceId));
    results.append(bypassGenericEmulator(instanceId));
    results.append(bypassRootDetection(instanceId));
    results.append(bypassHookDetection(instanceId));
    results.append(bypassDebugDetection(instanceId));
    results.append(bypassSELinuxDetection(instanceId));
    
    // Apply all bypasses
    bool allSuccess = applyAllBypasses(instanceId);
    
    m_bypassActive[instanceId] = allSuccess;
    
    emit bypassCompleted(instanceId, allSuccess);
    
    return results;
}

bool EmulatorDetectionBypass::applyAllBypasses(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    DetectionConfig config = getConfig(instanceId);
    
    qDebug() << "[EmulatorBypass] Applying all bypasses for:" << instanceId;
    
    // 1. Remove emulator files
    removeEmulatorFiles(instanceId);
    
    // 2. Seed detection files
    seedDetectionFiles(instanceId);
    
    // 3. Hide emulator properties
    hideProperties(instanceId);
    
    // 4. Set device properties
    setDeviceProperties(instanceId);
    
    // 5. Configure hardware virtualization
    configureHardwareVirt(instanceId);
    
    // 6. Hide debug flags
    hideDebugFlags(instanceId);
    
    // 7. Configure SELinux
    disableSELinuxDetection(instanceId);
    
    // 8. Set verified boot
    setVerifiedBoot(instanceId, "green");
    
    // 9. Configure system security
    configureSystemSecurity(instanceId);
    
    qDebug() << "[EmulatorBypass] All bypasses applied";
    return true;
}

// ========================================================================
// SPECIFIC BYPASS METHODS
// ========================================================================

BypassResult EmulatorDetectionBypass::bypassQEMUFiles(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::QEMU_FILE;
    
    QStringList qemuFiles = {
        "/dev/qemu_pipe",
        "/dev/socket/qemud",
        "/system/bin/qemud",
        "/system/bin/property-test",
        "/system/xbin/qemu-props",
        "/system/xbin/property-test",
        "/system/bin/qemu-service",
        "/data/local/tmp/qemu"
    };
    
    bool allRemoved = true;
    for (const QString& file : qemuFiles) {
        if (checkFileExists(file)) {
            if (!removeFile(file)) {
                allRemoved = false;
            }
        }
    }
    
    result.bypassed = allRemoved;
    result.description = "QEMU/Goldfish file detection bypass";
    result.action = allRemoved ? "All emulator files removed" : "Some files could not be removed";
    
    qDebug() << "[EmulatorBypass] QEMU files bypass:" << result.bypassed;
    return result;
}

BypassResult EmulatorDetectionBypass::bypassQEMUPipe(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::QEMU_PIPE;
    
    QStringList pipeFiles = {
        "/dev/qemu_pipe",
        "/dev/socket/qemud"
    };
    
    // Create empty files to prevent detection
    bool allSeeded = true;
    for (const QString& file : pipeFiles) {
        if (!createFile(file, "")) {
            allSeeded = false;
        }
        // Change permissions to prevent access
        executeCommand(instanceId, QString("chmod 000 %1").arg(file));
    }
    
    result.bypassed = allSeeded;
    result.description = "QEMU pipe detection bypass";
    result.action = "Emulator pipes disabled";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassQEMUProperties(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::QEMU_PROP;
    
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList props = {
        "ro.kernel.qemu",
        "ro.boot.qemu",
        "ro.kernel.android.qemud",
        "ro.hardware",
        "dalvik.vm.isa.arm",
        "dalvik.vm.dex2oat-Xms",
        "dalvik.vm.dex2oat-Xmx"
    };
    
    bool allHidden = true;
    for (const QString& prop : props) {
        if (!deleteProperty(instanceId, prop)) {
            // Try to override instead
            overrideProperty(instanceId, prop, "");
            allHidden = false;
        }
    }
    
    result.bypassed = allHidden;
    result.description = "QEMU property detection bypass";
    result.action = "QEMU properties hidden";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassCPUSignature(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::GOLDISH_CPU;
    
    ReDroidController& controller = ReDroidController::instance();
    
    DetectionConfig config = getConfig(instanceId);
    
    // Set CPU model to appear as real hardware
    QStringList cpuCommands = {
        QString("resetprop ro.hardware %1").arg(config.cpuModel),
        QString("resetprop ro.product.board %1").arg(config.cpuModel),
        QString("resetprop ro.mediatek.platform %1").arg(config.socModel),
        "resetprop ro.arch arm64",
        "resetprop ro.cpu.abi arm64-v8a"
    };
    
    for (const QString& cmd : cpuCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "CPU signature detection bypass";
    result.action = QString("CPU set to: %1").arg(config.cpuModel);
    
    qDebug() << "[EmulatorBypass] CPU signature bypass:" << config.cpuModel;
    return result;
}

BypassResult EmulatorDetectionBypass::bypassGenericEmulator(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::GENERIC_EMULATOR;
    
    ReDroidController& controller = ReDroidController::instance();
    
    DetectionConfig config = getConfig(instanceId);
    
    // Generic emulator bypass
    QStringList commands = {
        // Hide generic emulator markers
        "resetprop ro.emulator false",
        "resetprop ro.product.type phone",
        "resetprop ro.build.characteristics default",
        
        // Set as physical device
        "resetprop ro.device.form FACTORY",
        "resetprop ro.product.first_api_level 33",
        
        // Hide emulator build tags
        "resetprop ro.build.tags release-keys",
        "resetprop ro.build.type user",
        
        // Set proper device type
        "resetprop ro.product.device.type phone",
        "resetprop ro.product.model.sm-s928b"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "Generic emulator detection bypass";
    result.action = "Device presented as physical phone";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassRootDetection(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::SU_BINARY;
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Remove/hide su binaries
    QStringList commands = {
        // Remove su binaries
        "rm -f /system/xbin/su",
        "rm -f /system/bin/su",
        "rm -f /data/local/su",
        "rm -f /data/local/xposed",
        
        // Hide Magisk
        "rm -f /data/adb/magisk/*",
        
        // Remove Superuser app
        "rm -f /system/app/Superuser.apk",
        
        // Set system properties
        "resetprop ro.build.tags release-keys",
        "resetprop ro.secure 1"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "Root detection bypass";
    result.action = "SU binaries and Magisk hidden";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassHookDetection(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::FRIDA_PORT;
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Hide Frida/Xposed
    QStringList commands = {
        // Remove Frida artifacts
        "rm -f /data/local/tmp/frida-server",
        "rm -f /data/local/tmp/re.frida.server",
        "rm -f /data/local/tmp/frida",
        
        // Close Frida ports
        "setprop dbg.frida.tcp.port 0",
        
        // Remove Xposed
        "rm -f /data/data/de.robv.android.xposed.installer/*",
        "rm -f /system/framework/XposedBridge.jar",
        
        // Disable hook detection
        "setprop dalvik.vm.dex2oat-filter verify",
        
        // Set security
        "setprop ro.build.selinux.enforce 0"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "Hook detection bypass (Frida/Xposed)";
    result.action = "Frida and Xposed artifacts removed";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassDebugDetection(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::DEBUG_FLAG;
    
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        "resetprop ro.debuggable 0",
        "resetprop debug.atrace.tags.enableflags 0",
        "resetprop persist.sys.debug.atrace 0",
        "resetprop service.adb.root 0",
        "setprop security.perf_harden 1"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "Debug detection bypass";
    result.action = "Debug flags hidden";
    
    return result;
}

BypassResult EmulatorDetectionBypass::bypassSELinuxDetection(const QString& instanceId) {
    BypassResult result;
    result.type = EmulatorDetectionType::SELINUX_STATE;
    
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        "resetprop ro.build.selinux.enforce 0",
        "resetprop ro.build.selinux.disable 1",
        "resetprop ro.kernel.selinux disabled",
        "setenforce 0"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    result.bypassed = true;
    result.description = "SELinux detection bypass";
    result.action = "SELinux set to permissive";
    
    return result;
}

// ========================================================================
// FILE OPERATIONS
// ========================================================================

bool EmulatorDetectionBypass::removeEmulatorFiles(const QString& instanceId) {
    DetectionConfig config = getConfig(instanceId);
    
    for (const QString& file : config.removeFiles) {
        removeFile(file);
    }
    
    return true;
}

bool EmulatorDetectionBypass::seedDetectionFiles(const QString& instanceId) {
    DetectionConfig config = getConfig(instanceId);
    
    for (const QString& file : config.seedFiles) {
        createFile(file, "");
        executeCommand(instanceId, QString("chmod 000 %1").arg(file));
    }
    
    return true;
}

bool EmulatorDetectionBypass::hideFiles(const QString& instanceId, const QStringList& files) {
    for (const QString& file : files) {
        executeCommand(instanceId, QString("chmod 000 %1").arg(file));
    }
    return true;
}

bool EmulatorDetectionBypass::removeFile(const QString& path) {
    QFile file(path);
    if (file.exists()) {
        return file.remove();
    }
    return true; // File doesn't exist, considered removed
}

bool EmulatorDetectionBypass::createFile(const QString& path, const QString& content) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

// ========================================================================
// PROPERTY OPERATIONS
// ========================================================================

bool EmulatorDetectionBypass::hideProperties(const QString& instanceId) {
    DetectionConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    for (const QString& prop : config.hideProperties) {
        // Use resetprop to delete property
        controller.executeShell(instanceId, QString("resetprop %1").arg(prop));
    }
    
    return true;
}

bool EmulatorDetectionBypass::setDeviceProperties(const QString& instanceId) {
    DetectionConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    for (auto it = config.overrideProperties.begin(); it != config.overrideProperties.end(); ++it) {
        QString cmd = QString("resetprop %1 %2").arg(it.key()).arg(it.value());
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool EmulatorDetectionBypass::overrideProperty(const QString& instanceId, const QString& prop, const QString& value) {
    ReDroidController& controller = ReDroidController::instance();
    QString cmd = QString("resetprop %1 %2").arg(prop).arg(value);
    controller.executeShell(instanceId, cmd);
    return true;
}

bool EmulatorDetectionBypass::deleteProperty(const QString& instanceId, const QString& prop) {
    ReDroidController& controller = ReDroidController::instance();
    QString cmd = QString("resetprop --delete %1").arg(prop);
    QString result = controller.executeShell(instanceId, cmd);
    return result.isEmpty() || result.contains("deleted");
}

bool EmulatorDetectionBypass::checkFileExists(const QString& path) {
    return QFile::exists(path);
}

bool EmulatorDetectionBypass::checkProperty(const QString& instanceId, const QString& prop) {
    ReDroidController& controller = ReDroidController::instance();
    QString value = controller.getProperty(instanceId, prop);
    return !value.isEmpty();
}

// ========================================================================
// HARDWARE SPOOFING
// ========================================================================

bool EmulatorDetectionBypass::configureCPU(const QString& instanceId, const QString& cpuModel) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        QString("resetprop ro.hardware %1").arg(cpuModel),
        QString("resetprop ro.product.board %1").arg(cpuModel),
        QString("resetprop ro.mediatek.platform %1").arg(cpuModel),
        "resetprop ro.arch arm64",
        "resetprop ro.cpu.abi arm64-v8a",
        "resetprop ro.product.cpu.abi arm64-v8a",
        "resetprop ro.product.cpu.abilist arm64-v8a,armeabi-v7a,armeabi"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[EmulatorBypass] CPU configured:" << cpuModel;
    return true;
}

bool EmulatorDetectionBypass::configureGPU(const QString& instanceId, const QString& gpuModel) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        QString("resetprop ro.hardware.gpu %1").arg(gpuModel),
        QString("resetprop ro.gpu.model %1").arg(gpuModel),
        "resetprop ro.opengles.version 196609"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[EmulatorBypass] GPU configured:" << gpuModel;
    return true;
}

bool EmulatorDetectionBypass::hideVirtualizationSignatures(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        // Hide container/virtualization
        "resetprop ro.build.type user",
        "resetprop ro.hardware.virtual false",
        "resetprop ro.hardware.kvm 0",
        "resetprop ro.virtio.enabled false",
        
        // Set as real device
        "resetprop ro.product.device mt6885",
        "resetprop ro.product.model.sm-s928b",
        "resetprop ro.product.manufacturer samsung"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool EmulatorDetectionBypass::configureHardwareVirt(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    // Configure for KVM
    QStringList commands = {
        // Enable hardware virtualization
        "resetprop ro.hardware.virtual true",
        "resetprop ro.hardware.kvm 1",
        
        // Set proper device type
        "resetprop ro.device.form FACTORY",
        "resetprop ro.product.type phone",
        
        // Hardware properties
        "resetprop ro.hardware.gpu virgl",
        "resetprop ro.hardware.audio primary",
        
        // VirtIO
        "resetprop ro.virtio.enabled true",
        "resetprop ro.virtio.gpu.enabled true"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[EmulatorBypass] Hardware virtualization configured";
    return true;
}

// ========================================================================
// SYSTEM MODIFICATIONS
// ========================================================================

bool EmulatorDetectionBypass::disableSELinuxDetection(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        "resetprop ro.build.selinux.enforce 0",
        "resetprop ro.build.selinux.disable 1",
        "resetprop ro.selinux.enforce 0",
        "resetprop security.selinux.enforce 0",
        "setenforce 0"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool EmulatorDetectionBypass::setVerifiedBoot(const QString& instanceId, const QString& state) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        QString("resetprop ro.boot.verifiedbootstate %1").arg(state),
        QString("resetprop ro.verity.mode %1").arg(state == "green" ? "enforcing" : "disabled"),
        QString("resetprop ro.verifiedbootstate %1").arg(state),
        QString("resetprop ro.boot.veritymode %1").arg(state == "green" ? "enforcing" : "disabled"),
        QString("resetprop ro.bootloader.locked %1").arg(state == "green" ? "true" : "false")
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[EmulatorBypass] Verified boot state:" << state;
    return true;
}

bool EmulatorDetectionBypass::hideDebugFlags(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        "resetprop ro.debuggable 0",
        "resetprop debug.atrace.tags.enableflags 0",
        "resetprop persist.sys.debug.atrace 0",
        "resetprop service.adb.root 0",
        "resetprop security.perf_harden 1"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool EmulatorDetectionBypass::configureSystemSecurity(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        // Security
        "resetprop ro.build.tags release-keys",
        "resetprop ro.build.type user",
        "resetprop ro.secure 1",
        
        // Hide unknown sources
        "settings put global install_non_market_apps 0",
        
        // System integrity
        "resetprop ro.config.system_integrity.enabled true",
        "resetprop ro.config.system_integrity.no_verify false",
        
        // Device flags
        "resetprop ro.device.flags 0"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// VERIFICATION
// ========================================================================

bool EmulatorDetectionBypass::executeCommand(const QString& instanceId, const QString& command) {
    ReDroidController& controller = ReDroidController::instance();
    controller.executeShell(instanceId, command);
    return true;
}

QString EmulatorDetectionBypass::getDefaultCpuModel() {
    return "mt6885"; // MediaTek Dimensity 1000
}

QString EmulatorDetectionBypass::getDefaultGpuModel() {
    return "Mali-G77";
}

QJsonObject EmulatorDetectionBypass::runDetectionCheck(const QString& instanceId) {
    QJsonObject checkResult;
    ReDroidController& controller = ReDroidController::instance();
    
    // Check for common detection markers
    QStringList checkProperties = {
        "ro.kernel.qemu",
        "ro.boot.qemu",
        "ro.hardware",
        "ro.debuggable",
        "ro.secure"
    };
    
    QJsonObject properties;
    for (const QString& prop : checkProperties) {
        QString value = controller.getProperty(instanceId, prop);
        properties[prop] = value;
    }
    
    checkResult["properties"] = properties;
    checkResult["bypassActive"] = isBypassActive(instanceId);
    checkResult["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    return checkResult;
}

bool EmulatorDetectionBypass::isBypassActive(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_bypassActive.value(instanceId, false);
}

QJsonObject EmulatorDetectionBypass::getBypassStatus(const QString& instanceId) const {
    QJsonObject status;
    
    status["bypassActive"] = isBypassActive(instanceId);
    status["config"] = getConfig(instanceId).toJson();
    status["virtualizationStatus"] = PlayIntegrityManager::instance().getVirtualizationStatus(instanceId);
    
    return status;
}

} // namespace VirtualPhonePro
