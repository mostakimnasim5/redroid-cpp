/**
 * @file SELinuxManager.cpp
 * @brief SELinux Enforcement Masking Implementation
 */

#include "VirtualPhonePro/SELinuxManager.hpp"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonArray>
#include <QJsonObject>

namespace VirtualPhonePro {

// ========================================================================
// SINGLETON
// ========================================================================

SELinuxManager* SELinuxManager::s_instance = nullptr;

SELinuxManager& SELinuxManager::instance() {
    if (!s_instance) {
        s_instance = new SELinuxManager();
    }
    return *s_instance;
}

SELinuxManager::SELinuxManager(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[SELinux] Manager initialized";
}


// ========================================================================
// JSON CONVERSION
// ========================================================================

QJsonObject MitigationSELinuxConfig::toJson() const {
    QJsonObject obj;
    obj["state"] = static_cast<int>(state);
    obj["enforcementLevel"] = static_cast<int>(enforcementLevel);
    obj["spoofSELinuxStatus"] = spoofSELinuxStatus;
    obj["hideEnforcingLabel"] = hideEnforcingLabel;
    obj["maskAuditEvents"] = maskAuditEvents;
    obj["maskEnforce"] = maskEnforce;
    obj["maskSELinuxStatus"] = maskSELinuxStatus;
    obj["hideSEAndroid"] = hideSEAndroid;
    obj["forceUntrustedAppDomain"] = forceUntrustedAppDomain;
    obj["hideMagiskDomain"] = hideMagiskDomain;
    obj["hideInitDomain"] = hideInitDomain;
    obj["spoofFileContexts"] = spoofFileContexts;
    obj["hideRootfsContext"] = hideRootfsContext;
    obj["useMLSEnforce"] = useMLSEnforce;
    obj["mlsLevel"] = mlsLevel;
    obj["policyVersion"] = policyVersion;
    obj["policyType"] = policyType;
    return obj;
}

QJsonObject SELinuxCheckResult::toJson() const {
    QJsonObject obj;
    obj["isEnforcing"] = isEnforcing;
    obj["isProperlyConfigured"] = isProperlyConfigured;
    obj["isDetectedAsEmulator"] = isDetectedAsEmulator;
    obj["selinuxStatusHidden"] = selinuxStatusHidden;
    obj["enforceStatusHidden"] = enforceStatusHidden;
    obj["seAndroidHidden"] = seAndroidHidden;
    obj["processContextValid"] = processContextValid;
    obj["fileContextsValid"] = fileContextsValid;
    obj["auditEventsMasked"] = auditEventsMasked;
    obj["getenforceResult"] = getenforceResult;
    obj["sestatusResult"] = sestatusResult;
    obj["selinuxEnabledResult"] = selinuxEnabledResult;
    obj["currentContext"] = currentContext;
    
    QJsonArray warningsArray;
    for (const QString& w : warnings) {
        warningsArray.append(w);
    }
    obj["warnings"] = warningsArray;
    
    QJsonArray issuesArray;
    for (const QString& i : detectedIssues) {
        issuesArray.append(i);
    }
    obj["detectedIssues"] = issuesArray;
    
    obj["errorMessage"] = errorMessage;
    obj["hasError"] = hasError;
    
    return obj;
}

QString SELinuxCheckResult::getSummary() const {
    if (hasError) {
        return QString("ERROR: %1").arg(errorMessage);
    }
    
    if (isDetectedAsEmulator) {
        return QString("DETECTED - %1 issues found").arg(detectedIssues.size());
    }
    
    if (isProperlyConfigured) {
        return QString("PASS - SELinux properly configured, enforcing");
    }
    
    return QString("WARN - %1 warnings").arg(warnings.size());
}

QString SEPolicRule::toString() const {
    QString rule = QString("%1 %2 %3:%3 { %4 }")
        .arg(ruleType)
        .arg(sourceType)
        .arg(targetType)
        .arg(classType)
        .arg(permissions.join(" "));
    return rule;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void SELinuxManager::configure(const QString& instanceId, const MitigationSELinuxConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId] = config;
    qDebug() << "[SELinux] Configured for:" << instanceId;
}

MitigationSELinuxConfig SELinuxManager::getConfig(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_configs.value(instanceId);
}

void SELinuxManager::resetConfig(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    MitigationSELinuxConfig defaultConfig;
    defaultConfig.state = SELinuxState::ENFORCING;
    defaultConfig.enforcementLevel = SELinuxEnforcementLevel::FULL_ENFORCING;
    defaultConfig.spoofSELinuxStatus = true;
    defaultConfig.maskAuditEvents = true;
    defaultConfig.maskEnforce = true;
    defaultConfig.hideMagiskDomain = true;
    
    m_configs[instanceId] = defaultConfig;
    m_states[instanceId] = SELinuxState::ENFORCING;
    
    qDebug() << "[SELinux] Config reset for:" << instanceId;
}

// ========================================================================
// STATE SPOOFING
// ========================================================================

bool SELinuxManager::applyEnforcementMasking(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    ReDroidController& controller = ReDroidController::instance();
    MitigationSELinuxConfig config = m_configs.value(instanceId);
    
    qDebug() << "[SELinux] Applying enforcement masking for:" << instanceId;
    
    // Set SELinux state
    QString stateStr = (config.state == SELinuxState::ENFORCING) ? "Enforcing" : "Permissive";
    
    // Apply system properties to hide SELinux status
    QStringList commands = {
        // Hide SELinux status from getenforce
        "setprop roSELinuxStatus Enforcing",
        "setprop selinux.enforce.mode enforcing",
        
        // Hide SEAndroid (common emulator detection)
        "setprop ro.seAndroid false",
        "setprop ro.seandroid false",
        
        // Set proper security context
        "setprop ro.SECURITY_DOMAIN_TYPE kernel",
        
        // Hide enforcing status
        "setprop ro.security.selinux.enforcing 1",
        
        // Set MLS level
        QString("setprop ro.mls.level %1").arg(config.mlsLevel),
        
        // Policy version
        QString("setprop ro.security.selinux.version %1").arg(config.policyVersion),
        
        // Policy type
        QString("setprop ro.security.selinux.policy_type %1").arg(config.policyType),
        
        // Hide Magisk domain if configured
        "setprop persist.security.magisk_domain none"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    // Create fake SELinux status files
    QString fakeStatus = generateFakeSELinuxFile();
    
    // Write to file for cat command responses
    controller.executeShell(instanceId, "mkdir -p /data/local/tmp/selinux");
    controller.executeShell(instanceId, 
        QString("echo '%1' > /data/local/tmp/selinux/status").arg(fakeStatus));
    
    // Ensure /sys/fs/selinux exists (hidden for container)
    controller.executeShell(instanceId, 
        "touch /sys/fs/selinux/enforce 2>/dev/null || echo 1 > /sys/fs/selinux/enforce");
    
    m_states[instanceId] = config.state;
    
    qDebug() << "[SELinux] Enforcement masking applied successfully";
    return true;
}

bool SELinuxManager::setSELinuxState(const QString& instanceId, SELinuxState state) {
    QMutexLocker locker(&m_mutex);
    
    ReDroidController& controller = ReDroidController::instance();
    
    QString stateStr = (state == SELinuxState::ENFORCING) ? "Enforcing" : "Permissive";
    
    // Apply via setenforce command
    QString cmd = QString("setenforce %1")
        .arg(state == SELinuxState::ENFORCING ? "1" : "0");
    controller.executeShell(instanceId, cmd);
    
    // Also set via property
    controller.executeShell(instanceId, 
        QString("setprop selinux.enforce.mode %1").arg(stateStr.toLower()));
    
    m_states[instanceId] = state;
    m_configs[instanceId].state = state;
    
    emit selinuxStateChanged(instanceId, state);
    
    qDebug() << "[SELinux] State set to:" << stateStr;
    return true;
}

SELinuxState SELinuxManager::getSELinuxState(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_states.value(instanceId, SELinuxState::ENFORCING);
}

QString SELinuxManager::getEnforceStatus(const QString& instanceId) const {
    MitigationSELinuxConfig config = m_configs.value(instanceId);
    
    if (config.state == SELinuxState::ENFORCING) {
        return "Enforcing";
    } else if (config.state == SELinuxState::PERMISSIVE) {
        return "Permissive";
    }
    
    return "Disabled";
}

QString SELinuxManager::getSEStatus(const QString& instanceId) const {
    MitigationSELinuxConfig config = m_configs.value(instanceId);
    
    QString status = "SELinux status:                 enabled\n";
    status += QString("Current mode:                   %1\n")
        .arg(config.state == SELinuxState::ENFORCING ? "enforcing" : "permissive");
    status += "Mode from config:              enforcing\n";
    status += QString("Policy version:                 %1\n").arg(config.policyVersion);
    status += QString("Policy type:                    %1\n").arg(config.policyType);
    status += "MLS status:                      enabled\n";
    status += QString("MLS level:                      %1\n").arg(config.mlsLevel);
    status += "Enforce mode:                    Enforcing\n";
    status += "Domain transition status:        enabled\n";
    
    return status;
}

// ========================================================================
// SEPOLICY MANAGEMENT
// ========================================================================

bool SELinuxManager::addPolicyRule(const QString& instanceId, const SEPolicRule& rule) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_policyRules.contains(instanceId)) {
        m_policyRules[instanceId] = QList<SEPolicRule>();
    }
    
    m_policyRules[instanceId].append(rule);
    
    qDebug() << "[SELinux] Added policy rule:" << rule.toString();
    emit policyRuleAdded(instanceId, rule.toString());
    
    return true;
}

bool SELinuxManager::addPolicyRules(const QString& instanceId, const QList<SEPolicRule>& rules) {
    for (const SEPolicRule& rule : rules) {
        addPolicyRule(instanceId, rule);
    }
    return true;
}

bool SELinuxManager::loadDefaultRules(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "[SELinux] Loading default anti-detection rules";
    
    // Default rules for anti-detection
    QList<SEPolicRule> defaultRules = {
        // Allow untrusted_app to access network
        {"allow", "untrusted_app_type", "netd", "unix_stream_socket", {"connect", "getopt"}},
        {"allow", "untrusted_app_type", "property_socket", "sock_file", {"write"}},
        
        // Allow app to read system properties
        {"allow", "app_type", "system_prop", "property_service", {"read", "write"}},
        
        // Hide Magisk/SU
        {"neverallow", "untrusted_app_type", "su_exec", "file", {"execute", "execute_no_trans"}},
        {"neverallow", "untrusted_app_type", "magisk_exec", "file", {"execute", "execute_no_trans"}},
        
        // Allow file read/write
        {"allow", "app_type", "system_file", "file", {"read", "write", "open"}},
        
        // Allow network access
        {"allow", "app_type", "node", "tcp_socket", {"node_bind"}},
        {"allow", "app_type", "port", "tcp_socket", {"name_bind"}},
        
        // Hide debugfs
        {"neverallow", "app_type", "debugfs", "dir", {"search", "read"}},
        
        // Allow binder
        {"allow", "app_type", "binder", "binder", {"transfer", "call", "manage"}},
        
        // Hide kernel threads
        {"neverallow", "app_type", "kernel", "process", {"signal"}},
        
        // Allow sysfs access for device info
        {"allow", "app_type", "sysfs", "file", {"read", "open"}}
    };
    
    return addPolicyRules(instanceId, defaultRules);
}

QList<SEPolicRule> SELinuxManager::getPolicyRules(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_policyRules.value(instanceId);
}

// ========================================================================
// CONTEXT MANAGEMENT
// ========================================================================

bool SELinuxManager::setProcessContext(const QString& instanceId, const QString& process,
                                       const QString& context) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_processContexts.contains(instanceId)) {
        m_processContexts[instanceId] = QMap<QString, QString>();
    }
    
    m_processContexts[instanceId][process] = context;
    
    ReDroidController& controller = ReDroidController::instance();
    controller.executeShell(instanceId, 
        QString("run-as %1 setenforce 0 2>/dev/null || true").arg(context));
    
    qDebug() << "[SELinux] Set context for" << process << "to" << context;
    return true;
}

QString SELinuxManager::getProcessContext(const QString& instanceId, const QString& process) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    
    if (m_processContexts.contains(instanceId) && 
        m_processContexts[instanceId].contains(process)) {
        return m_processContexts[instanceId][process];
    }
    
    return getDefaultContext(process);
}

bool SELinuxManager::hideProcessContexts(const QString& instanceId, const QStringList& contexts) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_hiddenContexts.contains(instanceId)) {
        m_hiddenContexts[instanceId] = QStringList();
    }
    
    m_hiddenContexts[instanceId].append(contexts);
    
    qDebug() << "[SELinux] Hidden contexts:" << contexts.join(", ");
    return true;
}

bool SELinuxManager::applyFileContextSpoofing(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    qDebug() << "[SELinux] Applying file context spoofing";
    
    // Create fake file_contexts file
    QString fileContexts = R"(
/system(/.*)?                      u:object_r:system_file:s0
/system/bin/.*                     u:object_r:system_exec:s0
/system/lib/.*                     u:object_r:system_library_file:s0
/data(/.*)?                        u:object_r:system_data_file:s0
/data/local/tmp(/.*)?              u:object_r:rootfs:s0
/vendor(/.*)?                     u:object_r:vendor_file:s0
/proc(/.*)?                        u:object_r:proc:s0
/sys(/.*)?                        u:object_r:sysfs:s0
)";
    
    controller.executeShell(instanceId, 
        "echo '" + fileContexts + "' > /data/local/tmp/selinux/file_contexts");
    
    return true;
}

// ========================================================================
// AUDIT EVENT MASKING
// ========================================================================

bool SELinuxManager::enableAuditMasking(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Add common detection patterns to ignore
    QStringList patterns = {
        "avc: denied",
        "SELinux: asking",
        "device is emulated",
        "tracing_mark_write",
        "qemu_pipe_open"
    };
    
    addAuditIgnorePatterns(instanceId, patterns);
    
    // Configure auditd to ignore these
    controller.executeShell(instanceId, 
        "setprop persist.selinux.audit.ignore true");
    controller.executeShell(instanceId, 
        "setprop persist.selinux.audit.deny true");
    
    m_configs[instanceId].maskAuditEvents = true;
    
    qDebug() << "[SELinux] Audit masking enabled";
    return true;
}

bool SELinuxManager::disableAuditMasking(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    m_configs[instanceId].maskAuditEvents = false;
    
    qDebug() << "[SELinux] Audit masking disabled";
    return true;
}

bool SELinuxManager::addAuditIgnorePatterns(const QString& instanceId, const QStringList& patterns) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_auditIgnorePatterns.contains(instanceId)) {
        m_auditIgnorePatterns[instanceId] = QStringList();
    }
    
    m_auditIgnorePatterns[instanceId].append(patterns);
    
    emit auditEventMasked(instanceId, patterns.join(", "));
    
    return true;
}

// ========================================================================
// VERIFICATION
// ========================================================================

SELinuxCheckResult SELinuxManager::verifyConfiguration(const QString& instanceId) {
    SELinuxCheckResult result;
    
    qDebug() << "[SELinux] Verifying configuration for:" << instanceId;
    
    ReDroidController& controller = ReDroidController::instance();
    MitigationSELinuxConfig config = getConfig(instanceId);
    
    // Check current state
    result.isEnforcing = (getSELinuxState(instanceId) == SELinuxState::ENFORCING);
    result.getenforceResult = getEnforceStatus(instanceId);
    result.sestatusResult = getSEStatus(instanceId);
    
    // Check for emulator detection issues
    QStringList detectionChecks = {
        "getprop ro.kernel.qemu",
        "getprop ro.product.device",
        "getprop ro.HWDETA",
        "cat /sys/class/power_supply/battery/status"
    };
    
    for (const QString& check : detectionChecks) {
        QString output = controller.executeShell(instanceId, check);
        
        if (output.contains("1") || output.contains("emulator") || 
            output.contains("goldfish")) {
            result.detectedIssues.append(QString("Potential detection: %1").arg(check));
        }
    }
    
    // Check SELinux properties
    QString selinuxStatus = controller.getProperty(instanceId, "ro.SELinuxStatus");
    if (selinuxStatus.isEmpty() || selinuxStatus != "Enforcing") {
        result.selinuxStatusHidden = false;
        result.warnings.append("SELinux status not properly masked");
    } else {
        result.selinuxStatusHidden = true;
    }
    
    // Check enforcing status
    QString enforceStatus = controller.getProperty(instanceId, "selinux.enforce.mode");
    if (enforceStatus == "enforcing") {
        result.enforceStatusHidden = true;
    } else {
        result.enforceStatusHidden = false;
        result.warnings.append("Enforce status not masked");
    }
    
    // Check SEAndroid
    QString seAndroid = controller.getProperty(instanceId, "ro.seAndroid");
    if (seAndroid.isEmpty() || seAndroid == "false" || seAndroid == "0") {
        result.seAndroidHidden = true;
    } else {
        result.seAndroidHidden = false;
        result.detectedIssues.append("SEAndroid detected");
    }
    
    // Overall assessment
    result.isProperlyConfigured = result.warnings.isEmpty();
    result.isDetectedAsEmulator = !result.detectedIssues.isEmpty();
    
    if (result.isDetectedAsEmulator) {
        result.warnings.append("Device may be detected as emulator!");
    }
    
    return result;
}

QStringList SELinuxManager::generateADBCommands(const QString& instanceId) {
    QStringList commands;
    MitigationSELinuxConfig config = getConfig(instanceId);
    
    // SELinux state commands
    if (config.state == SELinuxState::ENFORCING) {
        commands.append("setenforce 1");
    } else {
        commands.append("setenforce 0");
    }
    
    // Property spoofing
    commands.append("setprop ro.SELinuxStatus Enforcing");
    commands.append("setprop selinux.enforce.mode enforcing");
    commands.append("setprop ro.seAndroid false");
    commands.append("setprop ro.security.selinux.enforcing 1");
    commands.append(QString("setprop ro.mls.level %1").arg(config.mlsLevel));
    commands.append(QString("setprop ro.security.selinux.version %1").arg(config.policyVersion));
    commands.append(QString("setprop ro.security.selinux.policy_type %1").arg(config.policyType));
    
    // Audit masking
    if (config.maskAuditEvents) {
        commands.append("setprop persist.selinux.audit.ignore true");
    }
    
    // Hide Magisk
    if (config.hideMagiskDomain) {
        commands.append("setprop persist.security.magisk_domain none");
    }
    
    return commands;
}

// ========================================================================
// HELPER METHODS
// ========================================================================

QString SELinuxManager::getDefaultContext(const QString& process) const {
    // Return appropriate SELinux context based on process name
    if (process.contains("untrusted_app", Qt::CaseInsensitive)) {
        return "u:r:untrusted_app_type:s0";
    } else if (process.contains("platform_app", Qt::CaseInsensitive)) {
        return "u:r:platform_app_type:s0";
    } else if (process.contains("system_app", Qt::CaseInsensitive)) {
        return "u:r:system_app_type:s0";
    } else if (process.contains("su", Qt::CaseInsensitive)) {
        return "u:r:su_exec:s0";
    } else if (process.contains("magisk", Qt::CaseInsensitive)) {
        return "u:r:magisk_exec:s0";
    }
    
    return "u:r:app_type:s0";
}

bool SELinuxManager::validateContext(const QString& context) const {
    // Basic validation: context should start with u:r:
    return context.startsWith("u:r:") && context.contains("_type:");
}

QString SELinuxManager::generateFakeSELinuxFile() const {
    return R"(SELinux: Enabled
Enforcing Mode: enforcing
Current Mode: enforcing
Policy Version: 31.5
Policy Type: mls
MLS: Enabled
)"
    "Enforce Mode: Enforcing\n";
}

QStringList SELinuxManager::getCriticalPropertiesToMask(const QString& instanceId) const {
    QStringList props = {
        "ro.SELinuxStatus",
        "ro.seAndroid",
        "ro.seandroid",
        "selinux.enforce.mode",
        "ro.security.selinux.enforcing",
        "persist.selinux.audit.ignore",
        "persist.security.magisk_domain"
    };
    return props;
}

} // namespace VirtualPhonePro
