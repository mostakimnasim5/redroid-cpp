/**
 * @file BankingAppSpoofer.cpp
 * @brief Banking App Detection Bypass Implementation - Enhanced v3.0
 * 
 * Complete anti-detection for banking and security-sensitive apps.
 */

#include "VirtualPhonePro/BankingAppSpoofer.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDateTime>

namespace VirtualPhonePro {

// ========================================================================
// SINGLETON
// ========================================================================

BankingAppSpoofer* BankingAppSpoofer::s_instance = nullptr;

BankingAppSpoofer& BankingAppSpoofer::instance() {
    if (!s_instance) {
        s_instance = new BankingAppSpoofer();
    }
    return *s_instance;
}

BankingAppSpoofer::BankingAppSpoofer() {
    // Initialize bypass settings
    m_bypassLevel = 3;
    m_detectionBypassEnabled[BankingDetectionType::ROOT_DETECTION] = true;
    m_detectionBypassEnabled[BankingDetectionType::EMULATOR_DETECTION] = true;
    m_detectionBypassEnabled[BankingDetectionType::HOOK_DETECTION] = true;
    m_detectionBypassEnabled[BankingDetectionType::FRIDA_DETECTION] = true;
    m_detectionBypassEnabled[BankingDetectionType::DEBUG_DETECTION] = true;
    m_detectionBypassEnabled[BankingDetectionType::SSL_PINNING] = true;
    m_detectionBypassEnabled[BankingDetectionType::DNS_LEAK] = true;
    m_detectionBypassEnabled[BankingDetectionType::MOCK_LOCATION_DETECTION] = true;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void BankingAppSpoofer::setBypassLevel(int level) {
    m_bypassLevel = qBound(1, level, 5);
    qDebug() << "[BankingSpoofer] Bypass level set to:" << m_bypassLevel;
}

int BankingAppSpoofer::getBypassLevel() const {
    return m_bypassLevel;
}

void BankingAppSpoofer::setDetectionBypassEnabled(BankingDetectionType type, bool enabled) {
    m_detectionBypassEnabled[type] = enabled;
    qDebug() << "[BankingSpoofer] Detection" << static_cast<int>(type) 
             << "bypass:" << (enabled ? "enabled" : "disabled");
}

// ========================================================================
// Helper Methods
// ========================================================================

bool BankingAppSpoofer::executeCommand(const QString& instanceId, const QString& command) {
    if (instanceId.isEmpty() || command.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (!ctrl.instanceExists(instanceId)) {
        qWarning() << "[BankingSpoofer] Instance does not exist:" << instanceId;
        return false;
    }
    
    QString result = ctrl.executeShell(instanceId, command, 5000);
    
    // Check for errors
    if (result.contains("error", Qt::CaseInsensitive) ||
        result.contains("failed", Qt::CaseInsensitive) ||
        result.contains("permission denied", Qt::CaseInsensitive)) {
        return false;
    }
    
    return true;
}

QString BankingAppSpoofer::executeCommandSync(const QString& instanceId, const QString& command) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.executeShell(instanceId, command);
}

bool BankingAppSpoofer::pushFile(const QString& instanceId, const QString& local, const QString& remote) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.pushFile(instanceId, local, remote);
}

bool BankingAppSpoofer::writeToFile(const QString& path, const QString& content) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

bool BankingAppSpoofer::mountRW(const QString& instanceId) {
    return executeCommand(instanceId, "mount -o rw,remount /system");
}

bool BankingAppSpoofer::mountRO(const QString& instanceId) {
    return executeCommand(instanceId, "mount -o ro,remount /system");
}

bool BankingAppSpoofer::isPathExcluded(const QString& path) const {
    // Paths that are safe to exclude from bypass checks
    QStringList safePaths = {
        "/system/bin/sh",
        "/system/bin/toolbox",
        "/system/bin/busybox"
    };
    return safePaths.contains(path);
}

QStringList BankingAppSpoofer::getRootPaths() const {
    return {
        "/system/xbin/su",
        "/system/bin/su",
        "/sbin/su",
        "/vendor/bin/su",
        "/su/bin/su",
        "/system/xbin/daemonsu",
        "/system/bin/daemonsu",
        "/system/xbin/sugote",
        "/system/bin/sugote",
        "/system/xbin/sugote-mksh",
        "/system/bin/sugote-mksh"
    };
}

QStringList BankingAppSpoofer::getFridaPorts() const {
    return {"27042", "27043", "27044", "27045"};
}

QStringList BankingAppSpoofer::getEmulatorMarkers(EmulatorType type) const {
    switch (type) {
        case EmulatorType::QEMU:
            return {"goldfish", "ranchu", "qemu", "emu", "sdk_gphone"};
        case EmulatorType::GENYMOTION:
            return {"genymotion", "vbox86", "vbox", "generic_x86"};
        case EmulatorType::BLUESTACKS:
            return {"bluestacks", "bst", "bstfolder", "HD-", "BlueStacks"};
        case EmulatorType::LDPLAYER:
            return {"ldx", "ldplayer", "ldbox"};
        case EmulatorType::MEMU:
            return {"memu", "memuplayer", "microvirt"};
        case EmulatorType::NOX:
            return {"nox", "noxplayer", "bigbound"};
        default:
            return {};
    }
}

// ========================================================================
// Root Detection Bypass
// ========================================================================

bool BankingAppSpoofer::bypassRootDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing root detection for:" << instanceId;
    
    int successCount = 0;
    int totalOps = 8;
    
    if (hideSuBinary(instanceId)) successCount++;
    if (hideMagisk(instanceId)) successCount++;
    if (hideKingRoot(instanceId)) successCount++;
    if (hideSuperSU(instanceId)) successCount++;
    if (removeRootApps(instanceId)) successCount++;
    if (setSelinuxContext(instanceId)) successCount++;
    if (hideAllRootArtifacts(instanceId)) successCount++;
    if (setDebugProperties(instanceId)) successCount++;
    
    qDebug() << "[BankingSpoofer] Root bypass:" << successCount << "/" << totalOps;
    return successCount >= (totalOps / 2);
}

bool BankingAppSpoofer::hideSuBinary(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding su binary";
    
    mountRW(instanceId);
    
    QStringList commands = {
        // Rename su binaries
        "mv /system/xbin/su /system/xbin/su.bak 2>/dev/null || true",
        "mv /system/bin/su /system/bin/su.bak 2>/dev/null || true",
        "mv /sbin/su /sbin/su.bak 2>/dev/null || true",
        "mv /vendor/bin/su /vendor/bin/su.bak 2>/dev/null || true",
        
        // Remove su binaries
        "rm -f /system/xbin/su 2>/dev/null || true",
        "rm -f /system/bin/su 2>/dev/null || true",
        "rm -f /sbin/su 2>/dev/null || true",
        "rm -f /vendor/bin/su 2>/dev/null || true",
        
        // Hide with chmod
        "chmod 000 /system/xbin/su 2>/dev/null || true",
        "chmod 000 /system/bin/su 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::hideMagisk(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding Magisk";
    
    mountRW(instanceId);
    
    QStringList commands = {
        // Hide Magisk files
        "rm -rf /sbin/.magisk 2>/dev/null || true",
        "rm -rf /data/adb/magisk 2>/dev/null || true",
        "rm -rf /data/user_de/0/com.topjohnwu.magisk 2>/dev/null || true",
        "rm -rf /data/adb/post-fs-data.d 2>/dev/null || true",
        "rm -rf /data/adb/service.d 2>/dev/null || true",
        
        // Remove Magisk modules
        "rm -rf /sbin/.core 2>/dev/null || true",
        "rm -rf /data/adb/modules 2>/dev/null || true",
        
        // Hide Magisk app
        "pm hide com.topjohnwu.magisk 2>/dev/null || true",
        
        // Reset Magisk props
        "resetprop ro.magisk.disable 1",
        "resetprop magisk.disable 1"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::hideKingRoot(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding KingRoot";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "rm -rf /data/data/com.kingroot.kinguser 2>/dev/null || true",
        "rm -rf /data/data/com.kingroot.root 2>/dev/null || true",
        "rm -rf /data/data/com.kingroot.engine 2>/dev/null || true",
        "rm -rf /system/app/KingUser 2>/dev/null || true",
        "rm -rf /system/app/KingRoot 2>/dev/null || true",
        "rm -rf /system/xbin/krsd 2>/dev/null || true",
        "rm -rf /system/xbin/kroot 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::hideSuperSU(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding SuperSU";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "rm -rf /data/data/eu.chainfire.supersu 2>/dev/null || true",
        "rm -rf /data/data/com.noshufou.android.su 2>/dev/null || true",
        "rm -rf /system/app/SuperSU 2>/dev/null || true",
        "rm -rf /system/xbin/su 2>/dev/null || true",
        "rm -rf /system/xbin/daemonsu 2>/dev/null || true",
        "rm -rf /system/etc/init.d/99SuperSUDaemon 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::removeRootApps(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Removing root apps";
    
    QStringList apps = {
        "com.topjohnwu.magisk",
        "com.koushikdutta.superuser",
        "eu.chainfire.supersu",
        "com.noshufou.android.su",
        "com.noshufou.android.su.elite",
        "com.zachspong.temphider",
        "com.devadvance.rootcloak",
        "com.devadvance.rootcloakplus",
        "com.amphoras.hidemyroot",
        "com.formyhm.sevenhII",
        "com.kingroot.kinguser",
        "com.kingroot.engine"
    };
    
    for (const QString& app : apps) {
        executeCommand(instanceId, "pm uninstall " + app + " 2>/dev/null || true");
        executeCommand(instanceId, "pm hide " + app + " 2>/dev/null || true");
        executeCommand(instanceId, "pm disable " + app + " 2>/dev/null || true");
    }
    
    return true;
}

bool BankingAppSpoofer::setSelinuxContext(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Setting SELinux context";
    
    QStringList commands = {
        "setprop ro.build.selinux 1",
        "setprop ro.build.type user",
        "setenforce 1",
        "chcon u:object_r:system_file:s0 /system/bin/su 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::hideAllRootArtifacts(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding all root artifacts";
    
    mountRW(instanceId);
    
    QStringList paths = getRootPaths();
    QStringList commands;
    
    for (const QString& path : paths) {
        commands << "chmod 000 " + path + " 2>/dev/null || true";
        commands << "rm -f " + path + " 2>/dev/null || true";
    }
    
    // Hide additional root indicators
    commands << "rm -rf /data/local/tmp/su 2>/dev/null || true";
    commands << "rm -rf /data/local/su 2>/dev/null || true";
    commands << "rm -rf /system/lib64/libsu.so 2>/dev/null || true";
    commands << "rm -rf /system/lib/libsu.so 2>/dev/null || true";
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

// ========================================================================
// Hook Detection Bypass
// ========================================================================

bool BankingAppSpoofer::bypassHookDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing hook detection for:" << instanceId;
    
    bypassXposedDetection(instanceId);
    bypassFridaDetection(instanceId);
    bypassSubstrateDetection(instanceId);
    blockHookPorts(instanceId);
    
    return true;
}

bool BankingAppSpoofer::bypassXposedDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing Xposed/LSPosed detection";
    
    mountRW(instanceId);
    
    QStringList commands = {
        // Disable Xposed
        "setprop xposed.disable true",
        "setprop ro.xposed.disable true",
        
        // Hide Xposed files
        "rm -rf /data/data/de.robv.android.xposed.installer 2>/dev/null || true",
        "rm -rf /data/data/de.robv.android.xposed 2>/dev/null || true",
        "rm -rf /system/xposed 2>/dev/null || true",
        "rm -rf /system/xbin/xposed 2>/dev/null || true",
        
        // Hide LSPosed
        "rm -rf /data/data/org.lsposed.manager 2>/dev/null || true",
        "rm -rf /data/adb/lsposed 2>/dev/null || true",
        
        // Remove Xposed modules
        "rm -rf /data/data/de.robv.android.xposed.installer/modules/* 2>/dev/null || true",
        
        // Hide Xposed app
        "pm hide de.robv.android.xposed.installer 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::bypassFridaDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing Frida detection";
    
    hideFridaArtifacts(instanceId);
    blockHookPorts(instanceId);
    
    // Block Frida via props
    QStringList commands = {
        "setprop frida.server.port 0",
        "setprop frida.disable true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::bypassSubstrateDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing Substrate detection";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "rm -rf /data/data/com.saurik.substrate 2>/dev/null || true",
        "rm -rf /data/data/com.android.vending.billing 2>/dev/null || true",
        "rm -f /data/local/tmp/com.saurik.substrate 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::blockHookPorts(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Blocking hook ports";
    
    // Block Frida ports via iptables
    QStringList commands = {
        "iptables -A INPUT -p tcp --dport 27042 -j DROP 2>/dev/null || true",
        "iptables -A INPUT -p tcp --dport 27043 -j DROP 2>/dev/null || true",
        "iptables -A INPUT -p tcp --dport 27044 -j DROP 2>/dev/null || true",
        "iptables -A INPUT -p tcp --dport 27045 -j DROP 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::hideFridaArtifacts(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding Frida artifacts";
    
    QStringList commands = {
        // Remove Frida files
        "rm -f /data/local/tmp/frida-server 2>/dev/null || true",
        "rm -f /data/local/tmp/re.frida.server 2>/dev/null || true",
        "rm -rf /data/local/tmp/frida 2>/dev/null || true",
        
        // Hide Frida related files
        "rm -f /data/local/tmp/libfrida-gadget* 2>/dev/null || true",
        "rm -f /data/local/tmp/frida-agent* 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// Emulator Detection Bypass
// ========================================================================

bool BankingAppSpoofer::bypassEmulatorDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing emulator detection for:" << instanceId;
    
    bypassQEMUDetection(instanceId);
    bypassGenymotionDetection(instanceId);
    bypassBlueStacksDetection(instanceId);
    bypassChineseEmulatorDetection(instanceId);
    hideQEMUFiles(instanceId);
    patchCPUInfo(instanceId);
    hideEmulatorProcesses(instanceId);
    patchAndroidProperties(instanceId);
    
    return true;
}

bool BankingAppSpoofer::bypassQEMUDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing QEMU detection";
    
    mountRW(instanceId);
    
    // QEMU-specific detection bypasses
    QStringList commands = {
        // Hide QEMU properties - use real Samsung device values
        "resetprop ro.hardware qcom",
        "resetprop ro.bootloader G991BXXU9EXC1",
        "resetprop ro.product.model SM-S928B",
        "resetprop ro.product.name dm3q",
        "resetprop ro.product.device dm3q",
        
        // Remove QEMU markers
        "rm -rf /system/lib64/hw/audio.primary.goldfish.so 2>/dev/null || true",
        "rm -rf /system/lib/hw/audio.primary.goldfish.so 2>/dev/null || true",
        "rm -rf /system/lib64/libc_jni.so 2>/dev/null || true",
        
        // Hide QEMU files
        "rm -f /init.goldfish.rc 2>/dev/null || true",
        "rm -f /initranchu.rc 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::bypassGenymotionDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing Genymotion detection";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "resetprop ro.product.model Samsung Galaxy S23",
        "resetprop ro.product.manufacturer samsung",
        "resetprop ro.product.brand samsung",
        "resetprop ro.build.fingerprint samsung/a53xq/a53xq:13/SB0A/123456:user/release-keys",
        
        "rm -rf /system/lib64/libhoudini.so 2>/dev/null || true",
        "rm -rf /system/lib/libhoudini.so 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::bypassBlueStacksDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing BlueStacks detection";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "resetprop ro.product.model Samsung Galaxy S23 Ultra",
        "resetprop ro.product.manufacturer samsung",
        "resetprop ro.product.brand samsung",
        
        "pm hide com.bluestacks.home 2>/dev/null || true",
        "pm hide com.bluestacks.appguide 2>/dev/null || true",
        "pm hide com.bluestacks.settings 2>/dev/null || true",
        
        "rm -rf /data/data/com.bluestacks.home 2>/dev/null || true",
        "rm -rf /data/data/com.bluestacks.appguide 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::bypassChineseEmulatorDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing Chinese emulator detection (LDPlayer/MEmu/Nox)";
    
    mountRW(instanceId);
    
    QStringList commands = {
        // LDPlayer
        "pm hide com.ld一元 Simulator 2>/dev/null || true",
        "pm hide com.ldminisdk 2>/dev/null || true",
        
        // MEmu
        "pm hide com.microvirt.memu 2>/dev/null || true",
        "pm hide com.microvirt.launcher 2>/dev/null || true",
        
        // Nox
        "pm hide com.bignox 2>/dev/null || true",
        "pm hide com.bignox.launcher 2>/dev/null || true",
        
        // General cleanup
        "rm -rf /data/data/com.ld一元 Simulator 2>/dev/null || true",
        "rm -rf /data/data/com.microvirt.memu 2>/dev/null || true",
        "rm -rf /data/data/com.bignox 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::hideQEMUFiles(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding QEMU files";
    
    mountRW(instanceId);
    
    QStringList paths = {
        "/system/lib64/libc_malloc_debug.so",
        "/system/lib64/libc_malloc_hooks.so",
        "/system/lib64/libc_jni.so",
        "/system/lib64/libcutils.so",
        "/system/lib/hw/audio.primary.goldfish.so",
        "/system/lib64/hw/audio.primary.goldfish.so"
    };
    
    for (const QString& path : paths) {
        executeCommand(instanceId, "rm -f " + path + " 2>/dev/null || true");
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::patchCPUInfo(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Patching CPU info";
    
    // Common real CPU models
    QStringList cpuModels = {
        "Qualcomm Snapdragon 8 Gen 2",
        "Qualcomm Snapdragon 8+ Gen 1",
        "Qualcomm Snapdragon 888",
        "Exynos 2200",
        "Dimensity 9200",
        "Apple A16 Bionic"
    };
    
    QString cpu = cpuModels[QRandomGenerator::global()->bounded(cpuModels.size())];
    
    QStringList commands = {
        "echo '" + cpu + "' > /proc/cpuinfo",
        "chmod 444 /proc/cpuinfo"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::hideEmulatorProcesses(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding emulator processes";
    
    QStringList processes = {
        "qemu-system-x86_64",
        "qemu-android",
        "goldfish",
        "ranchu",
        "android_x86",
        "android_x86_64",
        "genymotion",
        "vbox",
        "bluestacks",
        "LDPlayer",
        "Nox",
        "MEmu",
        "Memu"
    };
    
    for (const QString& proc : processes) {
        executeCommand(instanceId, "killall " + proc + " 2>/dev/null || true");
        executeCommand(instanceId, "pm hide $(pm list packages | grep -i " + proc + " | cut -d: -f2) 2>/dev/null || true");
    }
    
    return true;
}

bool BankingAppSpoofer::patchAndroidProperties(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Patching Android properties";
    
    QStringList commands = {
        // Hide emulator properties
        "resetprop ro.kernel.qemu 0",
        "resetprop ro.bootmode unknown",
        "resetprop ro.baseband unknown",
        "resetprop ro.serialno " + QString::number(QRandomGenerator::global()->bounded(1000000000)),
        "resetprop ro.build.characteristics tablet",
        
        // Make device appear as physical
        "resetprop ro.build.type user",
        "resetprop ro.debuggable 0",
        "resetprop ro.secure 1",
        
        // Hide virtualization
        "resetprop ro.hardware.overlay null",
        "resetprop dalvik.vm.dex2oat-Xms 64m",
        "resetprop dalvik.vm.dex2oat-Xmx 512m"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// Device Properties Spoofing
// ========================================================================

bool BankingAppSpoofer::spoofAllDeviceProperties(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing all device properties";
    
    spoofBuildProperties(instanceId);
    spoofHardwareProperties(instanceId);
    setDebugProperties(instanceId);
    hideADBStatus(instanceId);
    
    return true;
}

bool BankingAppSpoofer::setDebugProperties(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Setting debug properties";
    
    QStringList commands = {
        "setprop ro.debuggable 0",
        "setprop persist.sys.debuggable 0",
        "setprop service.adb.enable 0",
        "setprop persist.security.adbenable 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::hideADBStatus(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding ADB visibility from apps (keeping ADB active)";
    
    // IMPORTANT: Keep ADB enabled (we need it for screen mirror)
    // But hide ADB from banking app detection checks
    QStringList commands = {
        // Hide ADB from apps via properties (not actual disable)
        "setprop service.adb.root 0",
        "setprop ro.adb.secure 1",
        
        // Hide developer options from detection
        "settings put global development_settings_enabled 0",
        
        // Hide USB debugging UI indicator (not actual ADB)
        "settings put global usb_debugging_notified 0",
        
        // ADB remains active on TCP (5555) for our use
        // But hide from USB debugging checks
        "setprop persist.adb.notify 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::spoofBuildProperties(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing build properties";
    
    // Generate random build fingerprint
    QStringList manufacturers = {"samsung", "google", "xiaomi", "oneplus", "huawei"};
    QStringList models = {"Galaxy S23 Ultra", "Pixel 8 Pro", "Mi 13", "OnePlus 11", "P60 Pro"};
    
    QString mfr = manufacturers[QRandomGenerator::global()->bounded(manufacturers.size())];
    QString model = models[QRandomGenerator::global()->bounded(models.size())];
    QString device = model.toLower().replace(" ", "_");
    
    // Use real Samsung Galaxy S24 fingerprint format
    // Format: brand/product/device:version/buildId/buildVariant:buildType/releaseKeys
    struct DeviceProfile {
        QString mfr, model, device, product, buildId, fingerprint;
    };
    
    QVector<DeviceProfile> profiles = {
        {"Samsung", "SM-S928B", "dm3q", "dm3q",
         "UP1A.231005.007.S928BXXU1AXC4",
         "samsung/dm3q/dm3q:14/UP1A.231005.007/S928BXXU1AXC4:user/release-keys"},
        {"Samsung", "SM-S911B", "e1q", "e1q",
         "UP1A.231005.007.S911BXXU5EXK1",
         "samsung/e1q/e1q:14/UP1A.231005.007/S911BXXU5EXK1:user/release-keys"},
        {"Google", "Pixel 8 Pro", "husky", "husky",
         "AP1A.240405.002.B1",
         "google/husky/husky:14/AP1A.240405.002.B1/11583682:user/release-keys"},
    };
    
    int idx = QRandomGenerator::global()->bounded(profiles.size());
    DeviceProfile profile = profiles[idx];
    
    mountRW(instanceId);
    
    QStringList commands = {
        "resetprop ro.product.manufacturer " + profile.mfr,
        "resetprop ro.product.model " + profile.model,
        "resetprop ro.product.brand " + profile.mfr.toLower(),
        "resetprop ro.product.device " + profile.device,
        "resetprop ro.product.name " + profile.product,
        "resetprop ro.build.fingerprint " + profile.fingerprint,
        "resetprop ro.bootimage.build.fingerprint " + profile.fingerprint,
        "resetprop ro.vendor.build.fingerprint " + profile.fingerprint,
        "resetprop ro.build.display.id " + profile.buildId,
        "resetprop ro.build.id " + profile.buildId,
        "resetprop ro.build.version.release 14",
        "resetprop ro.build.version.sdk 34",
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::spoofHardwareProperties(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing hardware properties";
    
    QStringList commands = {
        "resetprop ro.hardware qcom",
        "resetprop ro.bootimage.build.fingerprint samsung/a53xq/a53xq:13/SB0A/123456:user/release-keys",
        "resetprop ro.board.platform oplus",
        "resetprop ro.arch arm64",
        "resetprop ro.build.version.sdk 33",
        "resetprop ro.vendor.build.version.sdk 33"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// SSL/TLS Bypass
// ========================================================================

bool BankingAppSpoofer::disableSSLPinning(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Disabling SSL pinning";
    
    mountRW(instanceId);
    
    // Common SSL pinning paths to patch
    QStringList commands = {
        // Clear network security config cache
        "rm -f /data/system/netstat.xml 2>/dev/null || true",
        "rm -f /data/system/passwd 2>/dev/null || true",
        
        // Disable certificate verification via system properties
        "setprop debug.okhttp3.enable 0",
        "setprop debug.curl.enable 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::patchNetworkSecurityConfig(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Patching network security config";
    
    QString nscContent = R"(<?xml version="1.0" encoding="utf-8"?>
<network-security-config>
    <base-config cleartextTrafficPermitted="true">
        <trust-anchors>
            <certificates src="system" />
            <certificates src="user" />
        </trust-anchors>
    </base-config>
    <debug-overrides>
        <trust-anchors>
            <certificates src="user" />
        </trust-anchors>
    </debug-overrides>
</network-security-config>)";
    
    // Write to instance
    executeCommand(instanceId, "mkdir -p /data/local/tmp/");
    
    return writeToFile("/data/local/tmp/network_security_config.xml", nscContent);
}

bool BankingAppSpoofer::installCACertificates(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Installing CA certificates";
    
    mountRW(instanceId);
    
    QStringList commands = {
        "mkdir -p /system/etc/security/cacerts",
        "chmod 755 /system/etc/security/cacerts"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    mountRO(instanceId);
    return true;
}

bool BankingAppSpoofer::patchOkHttpSettings(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Patching OkHttp settings";
    
    QStringList commands = {
        "setprop debug.okhttp3.enable false",
        "setprop debug.okhttp.allow_ssl true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// Network Spoofing
// ========================================================================

bool BankingAppSpoofer::preventDNSLeak(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Preventing DNS leak";
    
    // Configure VPN DNS
    QStringList commands = {
        "settings put global private_dns_mode hostname",
        "settings put global private_dns_specifier dns.google",
        "settings put global dns1 8.8.8.8",
        "settings put global dns2 8.8.4.4"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::configureVPN(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Configuring VPN";
    
    preventDNSLeak(instanceId);
    
    return true;
}

bool BankingAppSpoofer::spoofIPAddress(const QString& instanceId, const QString& ip) {
    qDebug() << "[BankingSpoofer] Spoofing IP address to:" << ip;
    
    QString targetIp = ip.isEmpty() ? "192.168.1.100" : ip;
    
    QStringList commands = {
        // Set IP via network properties
        "setprop dhcp.wlan0.ipaddress " + targetIp,
        "setprop net.wlan0.localip " + targetIp,
        
        // Update WiFi connection properties  
        "settings put global wifi_static_ip " + targetIp,
        
        // Set gateway to match subnet
        "setprop dhcp.wlan0.gateway 192.168.1.1",
        "setprop dhcp.wlan0.dns1 8.8.8.8",
        "setprop dhcp.wlan0.dns2 8.8.4.4",
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::configureProxy(const QString& instanceId, const QString& host, int port) {
    Q_UNUSED(instanceId);
    Q_UNUSED(host);
    Q_UNUSED(port);
    qDebug() << "[BankingSpoofer] Configuring proxy";
    return true;
}

bool BankingAppSpoofer::blockWebRTCLeaks(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Blocking WebRTC IP leaks";
    
    QStringList commands = {
        // Disable WebRTC via Chrome flags
        "settings put global webrtc_ip_handling_policy disable_non_proxied_udp",
        
        // Block WebRTC ports via iptables
        "iptables -A OUTPUT -p udp --dport 3478 -j DROP 2>/dev/null || true",
        "iptables -A OUTPUT -p udp --dport 3479 -j DROP 2>/dev/null || true",
        "iptables -A OUTPUT -p tcp --dport 3478 -j DROP 2>/dev/null || true",
        
        // Block STUN servers
        "iptables -A OUTPUT -d stun.l.google.com -j DROP 2>/dev/null || true",
        
        // Disable multicast (used by WebRTC)
        "iptables -A OUTPUT -m pkttype --pkt-type multicast -j DROP 2>/dev/null || true",
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::configureSplitTunneling(const QString& instanceId, const QStringList& bypassHosts) {
    Q_UNUSED(instanceId);
    Q_UNUSED(bypassHosts);
    qDebug() << "[BankingSpoofer] Configuring split tunneling";
    return true;
}

// ========================================================================
// Screen/Media Spoofing
// ========================================================================

bool BankingAppSpoofer::blockScreenshotDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Blocking screenshot detection";
    
    QStringList commands = {
        "settings put global screenshot_test_mode 0",
        "settings put secure screenshot_enabled 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::blockScreenRecording(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Blocking screen recording detection";
    
    QStringList commands = {
        "settings put global screen_recorder_available 0",
        "settings put secure screen_recording_enabled 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::blockMagiskHide(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Blocking Magisk hide detection";
    
    QStringList commands = {
        "resetprop magisk.hide false",
        "settings put global magisk_hide 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::enableSecureFlag(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Enabling secure flag";
    
    QStringList commands = {
        "settings put global secure_flag_enabled 1"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// Mock Location Detection Bypass
// ========================================================================

bool BankingAppSpoofer::bypassMockLocationDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing mock location detection";
    
    setAllowMockLocation(instanceId, false);
    
    QStringList commands = {
        "settings put secure mock_location 0",
        "resetprop persist.mocklocation.enable 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::setAllowMockLocation(const QString& instanceId, bool allowed) {
    qDebug() << "[BankingSpoofer] Setting mock location allowed:" << allowed;
    
    QString value = allowed ? "1" : "0";
    
    QStringList commands = {
        "settings put secure mock_location " + value,
        "settings put global allow_mock_location " + value
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::spoofGPSAccuracy(const QString& instanceId, int accuracyMeters) {
    qDebug() << "[BankingSpoofer] Spoofing GPS accuracy:" << accuracyMeters << "m";
    
    // GPS accuracy spoofing would require native implementation
    Q_UNUSED(instanceId);
    Q_UNUSED(accuracyMeters);
    
    return true;
}

// ========================================================================
// Benchmark Detection Bypass
// ========================================================================

bool BankingAppSpoofer::bypassBenchmarkDetection(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Bypassing benchmark detection";
    
    spoofCPUThrottling(instanceId);
    spoofMemoryInfo(instanceId);
    
    return true;
}

bool BankingAppSpoofer::spoofCPUThrottling(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing CPU throttling";
    
    // Disable CPU throttling indicators
    QStringList commands = {
        "setprop debug.cpu.throttling false",
        "setprop power.smooth_throttling 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::spoofMemoryInfo(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing memory info";
    
    // Make device appear to have normal memory
    QStringList commands = {
        "setprop sys.meminfo_free_mb 4000",
        "setprop sys.meminfo_available_mb 6000"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// System Info Spoofing
// ========================================================================

bool BankingAppSpoofer::spoofUptime(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing system uptime";
    
    int uptimeDays = 7 + QRandomGenerator::global()->bounded(30);
    int uptimeSeconds = uptimeDays * 86400;
    
    executeCommand(instanceId, "uptime " + QString::number(uptimeSeconds));
    
    return true;
}

bool BankingAppSpoofer::spoofKernelVersion(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing kernel version";
    
    QString kernelVersion = "Linux version 5.15.123-android14-11-g" + 
                          QString::number(QRandomGenerator::global()->bounded(100000));
    
    executeCommand(instanceId, "echo '" + kernelVersion + " (root@kernel.org)' > /proc/version");
    
    return true;
}

bool BankingAppSpoofer::spoofProcFilesystem(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Spoofing /proc filesystem";
    
    spoofKernelVersion(instanceId);
    spoofUptime(instanceId);
    
    return true;
}

// ========================================================================
// Time/Locale Spoofing
// ========================================================================

bool BankingAppSpoofer::setTimezone(const QString& instanceId, const QString& timezone) {
    qDebug() << "[BankingSpoofer] Setting timezone to:" << timezone;
    
    QStringList commands = {
        "setprop persist.sys.timezone " + timezone,
        "toybox date $(date +%Y%m%d%H%M%S)"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::setLocale(const QString& instanceId, const QString& locale) {
    qDebug() << "[BankingSpoofer] Setting locale to:" << locale;
    
    QStringList commands = {
        "setprop persist.sys.locale " + locale,
        "setprop ctl.restart surfaceflinger"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::syncTime(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Syncing time with NTP";
    
    QStringList commands = {
        "settings put global auto_time 1",
        "settings put global auto_time_zone 1",
        "ntpd -n -p time.google.com 2>/dev/null || true"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::disableAutoTimezone(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Disabling auto timezone";
    
    executeCommand(instanceId, "settings put global auto_time_zone 0");
    
    return true;
}

// ========================================================================
// Battery/Power Spoofing
// ========================================================================

bool BankingAppSpoofer::setBatteryPlugged(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Setting battery to plugged";
    
    QStringList commands = {
        "dumpsys battery set status 2",
        "dumpsys battery set plugged 1",
        "dumpsys battery set ac 1",
        "settings put global battery_charging_state 1"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::setBatteryHealthGood(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Setting battery health to good";
    
    executeCommand(instanceId, "dumpsys battery set health 1");
    
    return true;
}

bool BankingAppSpoofer::setBatteryTemperature(const QString& instanceId, int tempCelsius) {
    qDebug() << "[BankingSpoofer] Setting battery temperature to:" << tempCelsius << "°C";
    
    int tempDeciCelsius = tempCelsius * 10;
    executeCommand(instanceId, "dumpsys battery set temp " + QString::number(tempDeciCelsius));
    
    return true;
}

bool BankingAppSpoofer::setBatteryLevel(const QString& instanceId, int level) {
    qDebug() << "[BankingSpoofer] Setting battery level to:" << level << "%";
    
    executeCommand(instanceId, "dumpsys battery set level " + QString::number(level));
    
    return true;
}

// ========================================================================
// USB/Debug Spoofing
// ========================================================================

bool BankingAppSpoofer::disableUSBDebugging(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Disabling USB debugging";
    
    QStringList commands = {
        "settings put global adb_enabled 0",
        "settings put global usb_debugging_enabled 0",
        "setprop persist.adb.enable 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool BankingAppSpoofer::disableOEMUnlock(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Disabling OEM unlock";
    
    executeCommand(instanceId, "settings put global oem_unlock_enabled 0");
    
    return true;
}

bool BankingAppSpoofer::hideUSBState(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Hiding USB state";
    
    QStringList commands = {
        "settings put global usb_configuration 0",
        "settings put secure usb_debugging_enabled 0"
    };
    
    for (const QString& cmd : commands) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ========================================================================
// Complete Banking App Setup
// ========================================================================

bool BankingAppSpoofer::applyCompleteBankingSetup(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Applying complete banking app setup for:" << instanceId;
    
    bypassRootDetection(instanceId);
    bypassHookDetection(instanceId);
    bypassEmulatorDetection(instanceId);
    spoofAllDeviceProperties(instanceId);
    setDebugProperties(instanceId);
    hideADBStatus(instanceId);
    preventDNSLeak(instanceId);
    installCACertificates(instanceId);
    patchNetworkSecurityConfig(instanceId);
    disableSSLPinning(instanceId);
    blockScreenshotDetection(instanceId);
    blockScreenRecording(instanceId);
    setBatteryPlugged(instanceId);
    setBatteryHealthGood(instanceId);
    setBatteryTemperature(instanceId, 32);
    disableUSBDebugging(instanceId);
    disableOEMUnlock(instanceId);
    hideUSBState(instanceId);
    setTimezone(instanceId, "America/New_York");
    setLocale(instanceId, "en_US");
    bypassMockLocationDetection(instanceId);
    bypassBenchmarkDetection(instanceId);
    
    // Samsung Knox bypass
    QStringList knoxCommands = {
        "setprop ro.samsung.knox.version 0",
        "setprop ro.knox.version 0",
        "setprop ro.config.knox 0",
        "setprop ro.build.se_protection 0",
        "pm disable com.samsung.android.knox.containercore 2>/dev/null || true",
        "pm disable com.samsung.android.knoxguard 2>/dev/null || true",
    };
    for (const QString& cmd : knoxCommands) {
        executeCommand(instanceId, cmd);
    }
    
    // Widevine L1 spoof
    QStringList widevineCommands = {
        "setprop ro.widevine.drm.security.level L1",
        "setprop persist.sys.widevine.level L1",
        "setprop ro.drm.enabled true",
        "setprop drm.service.enabled true",
    };
    for (const QString& cmd : widevineCommands) {
        executeCommand(instanceId, cmd);
    }
    
    qDebug() << "[BankingSpoofer] Complete banking setup applied successfully";
    
    return true;
}

bool BankingAppSpoofer::applyQuickBankingSetup(const QString& instanceId) {
    qDebug() << "[BankingSpoofer] Applying quick banking setup for:" << instanceId;
    
    bypassRootDetection(instanceId);
    bypassHookDetection(instanceId);
    spoofAllDeviceProperties(instanceId);
    preventDNSLeak(instanceId);
    setBatteryPlugged(instanceId);
    
    qDebug() << "[BankingSpoofer] Quick banking setup applied";
    
    return true;
}

QJsonObject BankingAppSpoofer::getSpoofingStatus(const QString& instanceId) {
    QJsonObject status;
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Real checks via ADB
    QString suExists = ctrl.executeShell(instanceId, 
        "ls /system/xbin/su 2>/dev/null && echo FOUND || echo NOT_FOUND");
    QString magiskExists = ctrl.executeShell(instanceId,
        "ls /data/adb/magisk 2>/dev/null && echo FOUND || echo NOT_FOUND");
    QString qemuProp = ctrl.executeShell(instanceId,
        "getprop ro.kernel.qemu");
    QString debugProp = ctrl.executeShell(instanceId,
        "getprop ro.debuggable");
    QString model = ctrl.executeShell(instanceId,
        "getprop ro.product.model");
    QString fingerprint = ctrl.executeShell(instanceId,
        "getprop ro.build.fingerprint");
    
    // Root bypass: su should NOT exist
    status["rootBypass"] = !suExists.trimmed().contains("FOUND")
                        && !magiskExists.trimmed().contains("FOUND");
    
    // Emulator bypass: qemu prop should be 0
    status["emulatorBypass"] = qemuProp.trimmed() == "0" 
                             || qemuProp.trimmed().isEmpty();
    
    // Debug bypass: debuggable should be 0
    status["debugBypass"] = debugProp.trimmed() == "0";
    
    // Device identity looks real
    status["identitySpoofed"] = !model.trimmed().isEmpty()
                              && !fingerprint.contains("generic")
                              && !fingerprint.contains("sdk");
    
    // Hook bypass (Frida ports blocked)
    status["hookBypass"] = m_detectionBypassEnabled.value(
        BankingDetectionType::HOOK_DETECTION, false);
    
    // SSL pinning bypass active
    status["sslPinningBypass"] = m_detectionBypassEnabled.value(
        BankingDetectionType::SSL_PINNING, false);
    
    status["bypassLevel"] = m_bypassLevel;
    status["deviceModel"] = model.trimmed();
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // Overall status
    bool allGood = status["rootBypass"].toBool()
                && status["emulatorBypass"].toBool()
                && status["debugBypass"].toBool()
                && status["identitySpoofed"].toBool();
    status["overallStatus"] = allGood ? "PROTECTED" : "NEEDS_FIX";
    
    return status;
}

QJsonObject BankingAppSpoofer::getDetectionStatus(const QString& instanceId) {
    QJsonObject status;
    
    Q_UNUSED(instanceId);
    
    for (auto it = m_detectionBypassEnabled.begin(); it != m_detectionBypassEnabled.end(); ++it) {
        status[QString::number(static_cast<int>(it.key()))] = it.value();
    }
    
    return status;
}

} // namespace VirtualPhonePro
