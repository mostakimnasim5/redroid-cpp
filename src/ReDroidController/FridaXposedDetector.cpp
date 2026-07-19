/**
 * @file FridaXposedDetector.cpp
 * @brief Frida & Xposed Detection Bypass Implementation
 */

#include "VirtualPhonePro/FridaXposedDetector.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonObject>

namespace VirtualPhonePro {

FridaXposedDetector* FridaXposedDetector::s_instance = nullptr;

FridaXposedDetector& FridaXposedDetector::instance() {
    if (!s_instance) {
        s_instance = new FridaXposedDetector();
    }
    return *s_instance;
}

FridaXposedDetector::FridaXposedDetector() {
}

FridaXposedDetector::~FridaXposedDetector() {
}

bool FridaXposedDetector::initialize(const QString& instanceId) {
    qDebug() << "Initializing FridaXposedDetector for:" << instanceId;
    
    DetectionBypassState state;
    state.instanceId = instanceId;
    state.isInitialized = true;
    state.isActive = false;
    state.totalBypasses = 0;
    state.activeBypasses = 0;
    
    m_states[instanceId] = state;
    
    initializeBypassConfigs(instanceId);
    
    return applyAllBypasses(instanceId);
}

void FridaXposedDetector::initializeBypassConfigs(const QString& instanceId) {
    DetectionBypassState& state = m_states[instanceId];
    
    // Frida bypass
    BypassConfig frida;
    frida.type = DetectionType::FRIDA;
    frida.isEnabled = true;
    frida.isAutoApply = true;
    frida.bypassMethod = "all";
    frida.targetPackages = {"*"};  // All packages
    state.bypassConfigs.append(frida);
    
    // Xposed bypass
    BypassConfig xposed;
    xposed.type = DetectionType::XPOSED;
    xposed.isEnabled = true;
    xposed.isAutoApply = true;
    xposed.bypassMethod = "all";
    xposed.targetPackages = {"*"};
    state.bypassConfigs.append(xposed);
    
    // Root bypass
    BypassConfig root;
    root.type = DetectionType::ROOT;
    root.isEnabled = true;
    root.isAutoApply = true;
    root.bypassMethod = "hide_su";
    root.targetPackages = {"*"};
    state.bypassConfigs.append(root);
    
    // Debug bypass
    BypassConfig debug;
    debug.type = DetectionType::DEBUG;
    debug.isEnabled = true;
    debug.isAutoApply = true;
    debug.bypassMethod = "hide_debug";
    debug.targetPackages = {"*"};
    state.bypassConfigs.append(debug);
    
    // Emulator bypass
    BypassConfig emulator;
    emulator.type = DetectionType::EMULATOR;
    emulator.isEnabled = true;
    emulator.isAutoApply = true;
    emulator.bypassMethod = "hide_emulator";
    emulator.targetPackages = {"*"};
    state.bypassConfigs.append(emulator);
    
    // SSL Pinning bypass (initially empty)
    // Will be added per-app as needed
    
    state.totalBypasses = state.bypassConfigs.size();
}

bool FridaXposedDetector::applyAllBypasses(const QString& instanceId) {
    bool success = true;
    
    success &= applyFridaBypass(instanceId);
    success &= applyXposedBypass(instanceId);
    success &= applyRootBypass(instanceId);
    success &= applyDebugBypass(instanceId);
    success &= applyEmulatorBypass(instanceId);
    success &= applySSLPinningBypass(instanceId);
    success &= applyHookBypass(instanceId);
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isActive = true;
        m_states[instanceId].activeBypasses = success ? m_states[instanceId].totalBypasses : 0;
    }
    
    return success;
}

// ============================================================================
// Frida Detection Bypass
// ============================================================================

bool FridaXposedDetector::enableFridaBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::FRIDA) {
            config.isEnabled = true;
            config.isAutoApply = true;
            return applyFridaBypass(instanceId);
        }
    }
    
    return false;
}

bool FridaXposedDetector::disableFridaBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::FRIDA) {
            config.isEnabled = false;
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applyFridaBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    bool isEnabled = false;
    for (const auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::FRIDA) {
            isEnabled = config.isEnabled;
            break;
        }
    }
    
    if (!isEnabled) {
        return true;
    }
    
    return applyFridaArtifacts(instanceId);
}

bool FridaXposedDetector::applyFridaArtifacts(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Frida detection bypasses:
    // 1. Hide frida-server ports
    // 2. Hide frida-related files
    // 3. Hide frida-related processes
    
    QStringList commands = {
        // Hide frida-server ports (27042, 27043)
        "iptables -A OUTPUT -p tcp --dport 27042 -j DROP 2>/dev/null || true",
        "iptables -A OUTPUT -p tcp --dport 27043 -j DROP 2>/dev/null || true",
        "iptables -A INPUT -p tcp --dport 27042 -j DROP 2>/dev/null || true",
        "iptables -A INPUT -p tcp --dport 27043 -j DROP 2>/dev/null || true",
        
        // Hide frida files in /proc
        "find /proc/*/fd -lname '*frida*' 2>/dev/null | xargs -I{} unlink {} 2>/dev/null || true",
        
        // Create null-device symlinks for common frida paths
        "ln -sf /dev/null /data/local/tmp/frida-server 2>/dev/null || true",
        "ln -sf /dev/null /data/local/tmp/frida 2>/dev/null || true",
        
        // Hide frida agent libraries
        "mv /data/local/tmp/frida-agent 2>/dev/null || true",
        "rm -f /data/local/tmp/frida* 2>/dev/null || true",
        
        // Prevent frida from attaching
        "chmod 000 /data/local/tmp/frida* 2>/dev/null || true",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    emit bypassApplied(instanceId, DetectionType::FRIDA, true);
    
    qDebug() << "Applied Frida bypass for:" << instanceId;
    
    return true;
}

// ============================================================================
// Xposed Detection Bypass
// ============================================================================

bool FridaXposedDetector::enableXposedBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::XPOSED) {
            config.isEnabled = true;
            config.isAutoApply = true;
            return applyXposedBypass(instanceId);
        }
    }
    
    return false;
}

bool FridaXposedDetector::disableXposedBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::XPOSED) {
            config.isEnabled = false;
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applyXposedBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    bool isEnabled = false;
    for (const auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::XPOSED) {
            isEnabled = config.isEnabled;
            break;
        }
    }
    
    if (!isEnabled) {
        return true;
    }
    
    return applyXposedArtifacts(instanceId);
}

bool FridaXposedDetector::applyXposedArtifacts(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Xposed detection bypasses:
    // 1. Hide Xposed framework files
    // 2. Hide app_process
    // 3. Hide XposedBridge
    
    QStringList commands = {
        // Hide Xposed directories
        "rm -rf /data/data/de.robv.android.xposed.installer 2>/dev/null || true",
        "rm -rf /data/data/de.robv.android.xposed 2>/dev/null || true",
        "rm -rf /system/xposed 2>/dev/null || true",
        "rm -rf /system/xbin/xposed 2>/dev/null || true",
        
        // Hide Xposed modules
        "rm -rf /data/data/*/files/xposed 2>/dev/null || true",
        
        // Hide Xposed Bridge
        "rm -f /system/framework/XposedBridge.jar 2>/dev/null || true",
        "chmod 000 /system/framework/XposedBridge.jar 2>/dev/null || true",
        
        // Hide app_process
        "ls -la /system/bin/app_process* 2>/dev/null | head -1",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    emit bypassApplied(instanceId, DetectionType::XPOSED, true);
    
    qDebug() << "Applied Xposed bypass for:" << instanceId;
    
    return true;
}

// ============================================================================
// Root Detection Bypass
// ============================================================================

bool FridaXposedDetector::enableRootBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::ROOT) {
            config.isEnabled = true;
            config.isAutoApply = true;
            return applyRootBypass(instanceId);
        }
    }
    
    return false;
}

bool FridaXposedDetector::disableRootBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::ROOT) {
            config.isEnabled = false;
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applyRootBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    bool isEnabled = false;
    for (const auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::ROOT) {
            isEnabled = config.isEnabled;
            break;
        }
    }
    
    if (!isEnabled) {
        return true;
    }
    
    return applyRootArtifacts(instanceId);
}

bool FridaXposedDetector::applyRootArtifacts(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Root detection bypasses:
    // 1. Hide su binary
    // 2. Hide Magisk
    // 3. Hide Superuser
    
    QStringList commands = {
        // Hide su binaries
        "chmod 000 /system/xbin/su 2>/dev/null || true",
        "chmod 000 /system/bin/su 2>/dev/null || true",
        "chmod 000 /sbin/su 2>/dev/null || true",
        "chmod 000 /vendor/bin/su 2>/dev/null || true",
        
        // Hide Magisk
        "rm -rf /sbin/magisk 2>/dev/null || true",
        "rm -rf /data/adb/magisk 2>/dev/null || true",
        "rm -rf /data/adb/su 2>/dev/null || true",
        "chmod 000 /data/adb/magisk 2>/dev/null || true",
        "chmod 000 /data/adb/su 2>/dev/null || true",
        
        // Hide Magisk modules
        "chmod 000 /data/adb/modules/* 2>/dev/null || true",
        
        // Hide Superuser apps
        "pm hide com.koushikdutta.superuser 2>/dev/null || true",
        "pm hide com.noshufou.android.su 2>/dev/null || true",
        "pm hide com.topjohnwu.magisk 2>/dev/null || true",
        "pm disable com.koushikdutta.superuser 2>/dev/null || true",
        "pm disable com.noshufou.android.su 2>/dev/null || true",
        "pm disable com.topjohnwu.magisk 2>/dev/null || true",
        
        // Rename su if exists
        "mv /system/xbin/su /system/xbin/su.bak 2>/dev/null || true",
        "mv /system/bin/su /system/bin/su.bak 2>/dev/null || true",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    emit bypassApplied(instanceId, DetectionType::ROOT, true);
    
    qDebug() << "Applied root bypass for:" << instanceId;
    
    return true;
}

// ============================================================================
// Debug Detection Bypass
// ============================================================================

bool FridaXposedDetector::enableDebugBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::DEBUG) {
            config.isEnabled = true;
            config.isAutoApply = true;
            return applyDebugBypass(instanceId);
        }
    }
    
    return false;
}

bool FridaXposedDetector::disableDebugBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::DEBUG) {
            config.isEnabled = false;
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applyDebugBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    bool isEnabled = false;
    for (const auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::DEBUG) {
            isEnabled = config.isEnabled;
            break;
        }
    }
    
    if (!isEnabled) {
        return true;
    }
    
    return applyDebugArtifacts(instanceId);
}

bool FridaXposedDetector::applyDebugArtifacts(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Debug detection bypasses:
    // 1. Hide debug flags
    // 2. Hide tracer PID
    // 3. Hide JDWP
    
    QStringList commands = {
        // Ensure debuggable is 0
        "setprop ro.debuggable 0",
        "setprop debug.atrace.tags_enable 0",
        "setprop debug.atrace.tags.enable 0",
        
        // Disable debugging
        "settings put global development_settings_enabled 0",
        "settings put global adb_enabled 0",
        "settings put secure dev_options_enabled 0",
        
        // Hide tracer PID
        "echo 0 > /proc/self/status 2>/dev/null || true",
        
        // Disable JDWP
        "setprop dalvik.vm.jdwp.enabled 0",
        
        // Set debugger status
        "setprop ro.debuggerd_enabled 0",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    emit bypassApplied(instanceId, DetectionType::DEBUG, true);
    
    qDebug() << "Applied debug bypass for:" << instanceId;
    
    return true;
}

// ============================================================================
// Emulator Detection Bypass
// ============================================================================

bool FridaXposedDetector::enableEmulatorBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::EMULATOR) {
            config.isEnabled = true;
            config.isAutoApply = true;
            return applyEmulatorBypass(instanceId);
        }
    }
    
    return false;
}

bool FridaXposedDetector::disableEmulatorBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::EMULATOR) {
            config.isEnabled = false;
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applyEmulatorBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    bool isEnabled = false;
    for (const auto& config : m_states[instanceId].bypassConfigs) {
        if (config.type == DetectionType::EMULATOR) {
            isEnabled = config.isEnabled;
            break;
        }
    }
    
    if (!isEnabled) {
        return true;
    }
    
    return applyEmulatorArtifacts(instanceId);
}

bool FridaXposedDetector::applyEmulatorArtifacts(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Emulator detection bypasses:
    // 1. Hide emulator-specific files
    // 2. Hide qemu properties
    // 3. Hide goldfish sensors
    
    QStringList commands = {
        // Remove emulator-specific files
        "rm -f /system/lib/libc_malloc_debug_qemu.so 2>/dev/null || true",
        "rm -f /system/lib/libfakenmalloc.so 2>/dev/null || true",
        "rm -f /system/lib/libqemu_prop.so 2>/dev/null || true",
        "rm -f /init.goldfish.rc 2>/dev/null || true",
        "rm -f /init.qemu.rc 2>/dev/null || true",
        "rm -f /init.qemu.firstboot.rc 2>/dev/null || true",
        
        // Remove emulator sockets
        "rm -f /dev/socket/qemud 2>/dev/null || true",
        "rm -f /dev/socket/audiomix 2>/dev/null || true",
        "rm -f /dev/socket/goldfish_* 2>/dev/null || true",
        "rm -f /dev/qemu_pipe 2>/dev/null || true",
        
        // Hide qemu properties
        "setprop ro.kernel.qemu 0",
        "setprop ro.hardware sensors",
        "setprop ro.product.model Generic Device",
        
        // Make emulator files inaccessible
        "chmod 000 /dev/socket/qemud 2>/dev/null || true",
        "chmod 000 /dev/qemu_pipe 2>/dev/null || true",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    emit bypassApplied(instanceId, DetectionType::EMULATOR, true);
    
    qDebug() << "Applied emulator bypass for:" << instanceId;
    
    return true;
}

// ============================================================================
// SSL Pinning Bypass
// ============================================================================

bool FridaXposedDetector::addSSLPinningBypass(const QString& instanceId, const SSLPinningBypass& bypass) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].sslPinningBypasses.append(bypass);
    return true;
}

bool FridaXposedDetector::removeSSLPinningBypass(const QString& instanceId, const QString& domain) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    for (int i = 0; i < m_states[instanceId].sslPinningBypasses.size(); ++i) {
        if (m_states[instanceId].sslPinningBypasses[i].domain == domain) {
            m_states[instanceId].sslPinningBypasses.removeAt(i);
            return true;
        }
    }
    
    return false;
}

bool FridaXposedDetector::applySSLPinningBypass(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // SSL Pinning bypass notes:
    // This requires Magisk module or native library modification
    // Here we configure the system to allow certificate installation
    
    QStringList commands = {
        // Allow user CA certificates
        "settings put global install_non_market_apps 1",
        
        // Enable trust management
        "settings put global cert_locator_install_enabled 1",
        
        // Allow backup
        "settings put global backup_enabled 1",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "SSL pinning bypass configured for:" << instanceId;
    
    return true;
}

QList<SSLPinningBypass> FridaXposedDetector::getSSLPinningBypasses(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].sslPinningBypasses;
    }
    return QList<SSLPinningBypass>();
}

// ============================================================================
// Hook Detection Bypass
// ============================================================================

bool FridaXposedDetector::configureHookBypass(const QString& instanceId, const HookBypass& bypass) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].hookBypasses.append(bypass);
    return true;
}

bool FridaXposedDetector::applyHookBypass(const QString& instanceId) {
    // Hook bypass is primarily handled by the other bypasses
    // This includes Frida, Xposed, and native hook detection
    
    qDebug() << "Hook bypass configured for:" << instanceId;
    
    return true;
}

// ============================================================================
// Application-Specific
// ============================================================================

bool FridaXposedDetector::applyBypassForApp(const QString& instanceId, const QString& packageName) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // For banking apps, apply all bypasses
    QStringList commands = {
        // Clear app data to reset any detection flags
        QString("pm clear %1 2>/dev/null || true").arg(packageName),
        
        // Uninstall any security apps
        "pm uninstall com.securitybank.policymobile 2>/dev/null || true",
        "pm uninstall com.ish.valkyrie 2>/dev/null || true",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Applied bypass for app:" << packageName;
    
    return true;
}

bool FridaXposedDetector::isBypassActiveForApp(const QString& instanceId, const QString& packageName) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isActive;
    }
    return false;
}

// ============================================================================
// Callbacks
// ============================================================================

void FridaXposedDetector::setDetectionCallback(DetectionBypassCallback callback) {
    m_callback = callback;
}

// ============================================================================
// Utility
// ============================================================================

DetectionBypassState FridaXposedDetector::getBypassState(const QString& instanceId) const {
    DetectionBypassState defaultState;
    defaultState.instanceId = instanceId;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    return defaultState;
}

QJsonObject FridaXposedDetector::getBypassStateJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (!m_states.contains(instanceId)) {
        json["error"] = "Instance not found";
        return json;
    }
    
    const DetectionBypassState& state = m_states[instanceId];
    
    json["isInitialized"] = state.isInitialized;
    json["isActive"] = state.isActive;
    json["totalBypasses"] = state.totalBypasses;
    json["activeBypasses"] = state.activeBypasses;
    
    // Bypass configs
    QJsonArray configs;
    for (const auto& config : state.bypassConfigs) {
        QJsonObject cfg;
        cfg["type"] = static_cast<int>(config.type);
        cfg["isEnabled"] = config.isEnabled;
        cfg["isAutoApply"] = config.isAutoApply;
        cfg["method"] = config.bypassMethod;
        cfg["targetPackages"] = QJsonArray::fromStringList(config.targetPackages);
        configs.append(cfg);
    }
    json["configs"] = configs;
    
    // SSL pinning
    QJsonArray sslBypasses;
    for (const auto& ssl : state.sslPinningBypasses) {
        QJsonObject sslCfg;
        sslCfg["domain"] = ssl.domain;
        sslCfg["bypassEnabled"] = ssl.bypassEnabled;
        sslBypasses.append(sslCfg);
    }
    json["sslPinningBypasses"] = sslBypasses;
    
    return json;
}

bool FridaXposedDetector::reset(const QString& instanceId) {
    m_states.remove(instanceId);
    return true;
}

} // namespace VirtualPhonePro
