/**
 * @file DeviceIntegrityManager.cpp
 * @brief Device Integrity Manager Implementation
 */

#include "VirtualPhonePro/DeviceIntegrityManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>

namespace VirtualPhonePro {

DeviceIntegrityManager* DeviceIntegrityManager::s_instance = nullptr;

DeviceIntegrityManager& DeviceIntegrityManager::instance() {
    if (!s_instance) {
        s_instance = new DeviceIntegrityManager();
    }
    return *s_instance;
}

DeviceIntegrityManager::DeviceIntegrityManager() {
}

// ============================================================================
// Configuration
// ============================================================================

bool DeviceIntegrityManager::configure(const QString& instanceId, const DeviceIntegrityState& config) {
    m_states[instanceId] = config;
    m_states[instanceId].instanceId = instanceId;
    
    qDebug() << "Configured device integrity for instance:" << instanceId;
    
    return applyToInstance(instanceId);
}

bool DeviceIntegrityManager::configureForLevel(const QString& instanceId, IntegrityLevel level) {
    DeviceIntegrityState state;
    state.instanceId = instanceId;
    state.integrityLevel = level;
    
    // Configure based on integrity level
    switch (level) {
        case IntegrityLevel::BASIC:
            state.isBasicIntegrity = true;
            state.isCtsProfileMatch = true;
            state.verifiedBootState = VerifiedBootState::ORANGE;
            state.isVerifiedBootEnabled = true;
            state.isSecurityPatchCurrent = true;
            state.isSELinuxEnforcing = true;
            state.isRooted = false;
            state.isEmulatorDetected = false;
            break;
            
        case IntegrityLevel::BASIC_HARDWARE:
            state.isBasicIntegrity = true;
            state.isCtsProfileMatch = true;
            state.verifiedBootState = VerifiedBootState::YELLOW;
            state.isVerifiedBootEnabled = true;
            state.isVerifiedBootLocked = true;
            state.isSecurityPatchCurrent = true;
            state.isSELinuxEnforcing = true;
            state.isHardwareBackedKeyStore = true;
            state.isSecureHardwarePresent = true;
            state.isRooted = false;
            state.isEmulatorDetected = false;
            break;
            
        case IntegrityLevel::CERTIFIED:
            state.isBasicIntegrity = true;
            state.isCtsProfileMatch = true;
            state.verifiedBootState = VerifiedBootState::GREEN;
            state.isVerifiedBootEnabled = true;
            state.isVerifiedBootLocked = true;
            state.isSecurityPatchCurrent = true;
            state.isSELinuxEnforcing = true;
            state.isHardwareBackedKeyStore = true;
            state.isSecureHardwarePresent = true;
            state.isEncryptionEnabled = true;
            state.isFileBasedEncryptionEnabled = true;
            state.isRooted = false;
            state.isEmulatorDetected = false;
            state.isDebuggerDetected = false;
            break;
            
        case IntegrityLevel::VERIFIED_BOOT:
            state.isBasicIntegrity = true;
            state.isCtsProfileMatch = true;
            state.verifiedBootState = VerifiedBootState::GREEN;
            state.isVerifiedBootEnabled = true;
            state.isVerifiedBootLocked = true;
            state.isSecurityPatchCurrent = true;
            state.isSELinuxEnforcing = true;
            state.isHardwareBackedKeyStore = true;
            state.isSecureHardwarePresent = true;
            state.isEncryptionEnabled = true;
            state.isFileBasedEncryptionEnabled = true;
            state.isGatekeeperEnabled = true;
            state.isGatekeeperLocked = true;
            state.isRooted = false;
            state.isRootDetectionBypassed = true;
            state.isEmulatorDetected = false;
            state.isEmulatorDetectionBypassed = true;
            state.isDebuggerDetected = false;
            break;
    }
    
    state.isADBEnabled = false;
    state.lockState = DeviceLockState::LOCKED;
    state.isPasswordSet = true;
    state.isBiometricEnabled = true;
    state.isStrongBiometricEnabled = true;
    state.lastIntegrityCheck = QDateTime::currentDateTime();
    
    return configure(instanceId, state);
}

bool DeviceIntegrityManager::applyToInstance(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    const DeviceIntegrityState& state = m_states[instanceId];
    
    QStringList commands;
    
    // Verified Boot
    commands += {
        QString("setprop ro.boot.verifiedbootstate %1").arg(bootStateToString(state.verifiedBootState).toLower()),
        QString("setprop ro.boot.veritymode %1").arg(
            state.verifiedBootState == VerifiedBootState::GREEN ? "enforcing" : "disabled"),
        "setprop ro.boot.verity.enabled " + QString(state.isVerifiedBootEnabled ? "true" : "false"),
    };
    
    // Security Patch
    if (state.isSecurityPatchCurrent) {
        commands += {
            "setprop ro.build.version.security_patch " + state.securityPatchLevel,
            "setprop ro.system_ext.security_patch " + state.securityPatchLevel,
        };
    }
    
    // SELinux
    if (state.isSELinuxEnforcing) {
        commands += {
            "setprop ro.build.selinux Enforcing",
            "setenforce 1",
        };
    } else {
        commands += {
            "setprop ro.build.selinux Permissive",
            "setenforce 0",
        };
    }
    
    // Basic Integrity
    commands += {
        QString("setprop ro.build.version CTS %1").arg(state.isCtsProfileMatch ? "true" : "false"),
        "setprop ro. CTS true",
        "setprop ro.build.tags release-keys",
    };
    
    // Emulator/Root Detection
    if (!state.isEmulatorDetected) {
        commands += {
            "setprop ro.kernel.qemu 0",
            "setprop ro.hardware sensors",
        };
    }
    
    if (!state.isRooted) {
        commands += {
            "setprop ro.debuggable 0",
            "setprop ro.secure 1",
        };
    }
    
    // Debugger
    commands += QString("setprop ro.debuggable %1").arg(state.isDebuggerDetected ? "1" : "0");
    
    // ADB
    commands += QString("setprop persist.adb.enabled %1").arg(state.isADBEnabled ? "1" : "0");
    
    // Encryption
    if (state.isEncryptionEnabled) {
        commands += {
            "setprop ro.crypto.state encrypted",
            "setprop ro.crypto.type file",
        };
    }
    
    // Execute commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Device integrity configuration applied to instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Verified Boot
// ============================================================================

bool DeviceIntegrityManager::setVerifiedBootState(const QString& instanceId, VerifiedBootState state) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].verifiedBootState = state;
    m_states[instanceId].verifiedBootStateString = bootStateToString(state);
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.boot.verifiedbootstate %1").arg(bootStateToString(state).toLower()));
    
    return true;
}

VerifiedBootState DeviceIntegrityManager::getVerifiedBootState(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].verifiedBootState;
    }
    return VerifiedBootState::UNKNOWN;
}

bool DeviceIntegrityManager::enableVerifiedBoot(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isVerifiedBootEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.boot.verity.enabled true");
    
    return true;
}

bool DeviceIntegrityManager::disableVerifiedBoot(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isVerifiedBootEnabled = false;
    m_states[instanceId].verifiedBootState = VerifiedBootState::ORANGE;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.boot.verity.enabled false");
    
    return true;
}

// ============================================================================
// Basic Integrity
// ============================================================================

bool DeviceIntegrityManager::setBasicIntegrity(const QString& instanceId, bool integrity) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isBasicIntegrity = integrity;
    m_states[instanceId].isBasicIntegrityPassed = integrity;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.build.version.CTS %1").arg(integrity ? "true" : "false"));
    
    return true;
}

bool DeviceIntegrityManager::setCtsProfileMatch(const QString& instanceId, bool match) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isCtsProfileMatch = match;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro. CTS %1").arg(match ? "true" : "false"));
    
    return true;
}

IntegrityCheckResult DeviceIntegrityManager::runIntegrityCheck(const QString& instanceId, const QString& checkName) {
    IntegrityCheckResult result;
    result.checkName = checkName;
    result.checkTime = QDateTime::currentDateTime();
    result.passed = false;
    
    if (!m_states.contains(instanceId)) {
        result.errorMessage = "Instance not configured";
        result.errorCode = "NOT_CONFIGURED";
        return result;
    }
    
    const DeviceIntegrityState& state = m_states[instanceId];
    
    // Run specific checks
    if (checkName == "basic_integrity") {
        result.passed = state.isBasicIntegrity;
        result.errorCode = result.passed ? "NONE" : "BASIC_INTEGRITY_FAIL";
    } else if (checkName == "cts_profile_match") {
        result.passed = state.isCtsProfileMatch;
        result.errorCode = result.passed ? "NONE" : "CTS_MISMATCH";
    } else if (checkName == "verified_boot") {
        result.passed = state.verifiedBootState == VerifiedBootState::GREEN;
        result.errorCode = result.passed ? "NONE" : "VERIFIED_BOOT_FAIL";
    } else if (checkName == "selinux") {
        result.passed = state.isSELinuxEnforcing;
        result.errorCode = result.passed ? "NONE" : "SELINUX_PERMISSIVE";
    } else if (checkName == "system_integrity") {
        result.passed = !state.isDebuggerDetected && !state.isRooted && !state.isEmulatorDetected;
        result.errorCode = result.passed ? "NONE" : "SYSTEM_TAMPERED";
    } else if (checkName == "hardware_attestation") {
        result.passed = state.isSecureHardwarePresent;
        result.errorCode = result.passed ? "NONE" : "NO_HARDWARE_ATTESTATION";
    } else if (checkName == "encryption") {
        result.passed = state.isEncryptionEnabled;
        result.errorCode = result.passed ? "NONE" : "ENCRYPTION_DISABLED";
    } else if (checkName == "gatekeeper") {
        result.passed = state.isGatekeeperEnabled && state.isGatekeeperLocked;
        result.errorCode = result.passed ? "NONE" : "GATEKEEPER_DISABLED";
    } else {
        result.passed = true;
        result.errorCode = "NONE";
    }
    
    if (!result.passed) {
        result.errorMessage = "Integrity check failed: " + checkName;
        m_states[instanceId].totalChecksFailed++;
    } else {
        m_states[instanceId].totalChecksPassed++;
    }
    
    m_checkResults[instanceId].append(result);
    
    return result;
}

QList<IntegrityCheckResult> DeviceIntegrityManager::getAllCheckResults(const QString& instanceId) {
    if (m_checkResults.contains(instanceId)) {
        return m_checkResults[instanceId];
    }
    return QList<IntegrityCheckResult>();
}

// ============================================================================
// Security Patch
// ============================================================================

bool DeviceIntegrityManager::setSecurityPatchLevel(const QString& instanceId, const QString& patchLevel) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].securityPatchLevel = patchLevel;
    m_states[instanceId].isSecurityPatchCurrent = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.build.version.security_patch %1").arg(patchLevel));
    
    return true;
}

QString DeviceIntegrityManager::getSecurityPatchLevel(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].securityPatchLevel;
    }
    return "2024-01-01";
}

bool DeviceIntegrityManager::isSecurityPatchCurrent(const QString& instanceId) {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isSecurityPatchCurrent;
    }
    return false;
}

// ============================================================================
// System Integrity
// ============================================================================

bool DeviceIntegrityManager::enableSystemIntegrity(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isSystemIntegrityEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.system_integrity.enabled true");
    
    return true;
}

bool DeviceIntegrityManager::disableSystemIntegrity(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isSystemIntegrityEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.system_integrity.enabled false");
    
    return true;
}

bool DeviceIntegrityManager::setDebuggerDetected(const QString& instanceId, bool detected) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isDebuggerDetected = detected;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.debuggable %1").arg(detected ? "1" : "0"));
    
    return true;
}

bool DeviceIntegrityManager::setADBEnabled(const QString& instanceId, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isADBEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put global adb_enabled %1").arg(enabled ? "1" : "0"));
    ctrl.executeShell(instanceId, QString("setprop persist.adb.enabled %1").arg(enabled ? "1" : "0"));
    
    return true;
}

bool DeviceIntegrityManager::setRootStatus(const QString& instanceId, bool isRooted, bool bypassed) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isRooted = isRooted;
    m_states[instanceId].isRootDetectionBypassed = bypassed;
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (!isRooted || bypassed) {
        ctrl.executeShell(instanceId, "setprop ro.debuggable 0");
        ctrl.executeShell(instanceId, "setprop ro.secure 1");
    }
    
    return true;
}

bool DeviceIntegrityManager::setEmulatorDetection(const QString& instanceId, bool detected, bool bypassed) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isEmulatorDetected = detected;
    m_states[instanceId].isEmulatorDetectionBypassed = bypassed;
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (!detected || bypassed) {
        ctrl.executeShell(instanceId, "setprop ro.kernel.qemu 0");
        ctrl.executeShell(instanceId, "setprop ro.hardware sensors");
        ctrl.executeShell(instanceId, "setprop ro.product.model Generic Device");
    }
    
    return true;
}

// ============================================================================
// SELinux
// ============================================================================

bool DeviceIntegrityManager::enableSELinuxEnforcing(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isSELinuxEnforcing = true;
    m_states[instanceId].isSELinuxPermissive = false;
    m_states[instanceId].selinuxStatus = "Enforcing";
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.build.selinux Enforcing");
    ctrl.executeShell(instanceId, "setenforce 1");
    
    return true;
}

bool DeviceIntegrityManager::setSELinuxPermissive(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isSELinuxPermissive = true;
    m_states[instanceId].isSELinuxEnforcing = false;
    m_states[instanceId].selinuxStatus = "Permissive";
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.build.selinux Permissive");
    ctrl.executeShell(instanceId, "setenforce 0");
    
    return true;
}

QString DeviceIntegrityManager::getSELinuxStatus(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].selinuxStatus;
    }
    return "Unknown";
}

// ============================================================================
// Gatekeeper
// ============================================================================

bool DeviceIntegrityManager::enableGatekeeper(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isGatekeeperEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.gatekeeper.enabled true");
    
    return true;
}

bool DeviceIntegrityManager::lockGatekeeper(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isGatekeeperLocked = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.gatekeeper.locked true");
    
    return true;
}

bool DeviceIntegrityManager::setGatekeeperTimeout(const QString& instanceId, int seconds) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].gatekeeperTimeoutSeconds = seconds;
    m_states[instanceId].isGatekeeperTimeoutValid = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.gatekeeper.timeout %1").arg(seconds));
    
    return true;
}

// ============================================================================
// Device Lock
// ============================================================================

bool DeviceIntegrityManager::setDeviceLockState(const QString& instanceId, DeviceLockState state) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].lockState = state;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop ro.lockstate %1").arg(lockStateToString(state).toLower()));
    
    return true;
}

DeviceLockState DeviceIntegrityManager::getDeviceLockState(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].lockState;
    }
    return DeviceLockState::UNLOCKED;
}

bool DeviceIntegrityManager::enableBiometric(const QString& instanceId, bool strong) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isBiometricEnabled = true;
    m_states[instanceId].isStrongBiometricEnabled = strong;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure biometric_enabled 1");
    if (strong) {
        ctrl.executeShell(instanceId, "settings put secure strong_biometric_enabled 1");
    }
    
    return true;
}

bool DeviceIntegrityManager::disableBiometric(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isBiometricEnabled = false;
    m_states[instanceId].isStrongBiometricEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure biometric_enabled 0");
    ctrl.executeShell(instanceId, "settings put secure strong_biometric_enabled 0");
    
    return true;
}

bool DeviceIntegrityManager::setPassword(const QString& instanceId, const QString& password) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isPasswordSet = !password.isEmpty();
    m_states[instanceId].lockState = DeviceLockState::LOCKED_CREDENTIAL;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "locksmith reset_password " + password);
    
    return true;
}

// ============================================================================
// Encryption
// ============================================================================

bool DeviceIntegrityManager::enableFileBasedEncryption(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isFileBasedEncryptionEnabled = true;
    m_states[instanceId].isEncryptionEnabled = true;
    m_states[instanceId].isEncryptionSupported = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.crypto.state encrypted");
    ctrl.executeShell(instanceId, "setprop ro.crypto.type file");
    
    return true;
}

bool DeviceIntegrityManager::isEncryptionEnabled(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isEncryptionEnabled;
    }
    return false;
}

bool DeviceIntegrityManager::enableFullDiskEncryption(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isEncryptionEnabled = true;
    m_states[instanceId].isEncryptionSupported = true;
    m_states[instanceId].isFileBasedEncryptionEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop ro.crypto.state encrypted");
    ctrl.executeShell(instanceId, "setprop ro.crypto.type block");
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

DeviceIntegrityState DeviceIntegrityManager::getIntegrityState(const QString& instanceId) const {
    DeviceIntegrityState defaultState;
    defaultState.instanceId = instanceId;
    defaultState.integrityLevel = IntegrityLevel::BASIC;
    defaultState.verifiedBootState = VerifiedBootState::UNKNOWN;
    defaultState.isSELinuxEnforcing = true;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    return defaultState;
}

QJsonObject DeviceIntegrityManager::getIntegrityInfoJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_states.contains(instanceId)) {
        const DeviceIntegrityState& state = m_states[instanceId];
        
        json["integrityLevel"] = integrityLevelToString(state.integrityLevel);
        json["verifiedBootState"] = bootStateToString(state.verifiedBootState);
        json["isBasicIntegrity"] = state.isBasicIntegrity;
        json["isCtsProfileMatch"] = state.isCtsProfileMatch;
        json["isSecurityPatchCurrent"] = state.isSecurityPatchCurrent;
        json["securityPatchLevel"] = state.securityPatchLevel;
        json["selinuxStatus"] = state.selinuxStatus;
        json["isSELinuxEnforcing"] = state.isSELinuxEnforcing;
        json["isRooted"] = state.isRooted;
        json["isRootBypassed"] = state.isRootDetectionBypassed;
        json["isEmulatorDetected"] = state.isEmulatorDetected;
        json["isEmulatorBypassed"] = state.isEmulatorDetectionBypassed;
        json["isDebuggerDetected"] = state.isDebuggerDetected;
        json["isADBEnabled"] = state.isADBEnabled;
        json["isEncryptionEnabled"] = state.isEncryptionEnabled;
        json["isFileBasedEncryption"] = state.isFileBasedEncryptionEnabled;
        json["lockState"] = lockStateToString(state.lockState);
        json["isBiometricEnabled"] = state.isBiometricEnabled;
        json["isHardwareBackedKeyStore"] = state.isHardwareBackedKeyStore;
        json["isSecureHardwarePresent"] = state.isSecureHardwarePresent;
        json["passesIntegrity"] = passesIntegrity(instanceId);
    }
    
    return json;
}

bool DeviceIntegrityManager::passesIntegrity(const QString& instanceId) const {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    const DeviceIntegrityState& state = m_states[instanceId];
    
    return state.isBasicIntegrity &&
           state.isCtsProfileMatch &&
           state.verifiedBootState == VerifiedBootState::GREEN &&
           state.isSecurityPatchCurrent &&
           state.isSELinuxEnforcing &&
           !state.isDebuggerDetected &&
           !state.isEmulatorDetected;
}

bool DeviceIntegrityManager::reset(const QString& instanceId) {
    m_states.remove(instanceId);
    m_checkResults.remove(instanceId);
    return true;
}

bool DeviceIntegrityManager::applyAllIntegrity(const QString& instanceId) {
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

QString DeviceIntegrityManager::integrityLevelToString(IntegrityLevel level) const {
    switch (level) {
        case IntegrityLevel::BASIC: return "Basic";
        case IntegrityLevel::BASIC_HARDWARE: return "Basic Hardware";
        case IntegrityLevel::CERTIFIED: return "Certified";
        case IntegrityLevel::VERIFIED_BOOT: return "Verified Boot";
        default: return "Unknown";
    }
}

QString DeviceIntegrityManager::bootStateToString(VerifiedBootState state) const {
    switch (state) {
        case VerifiedBootState::GREEN: return "green";
        case VerifiedBootState::YELLOW: return "yellow";
        case VerifiedBootState::ORANGE: return "orange";
        case VerifiedBootState::RED: return "red";
        case VerifiedBootState::UNKNOWN: return "unknown";
        default: return "unknown";
    }
}

QString DeviceIntegrityManager::lockStateToString(DeviceLockState state) const {
    switch (state) {
        case DeviceLockState::UNLOCKED: return "unlocked";
        case DeviceLockState::LOCKED: return "locked";
        case DeviceLockState::LOCKED_CREDENTIAL: return "locked_credential";
        case DeviceLockState::LOCKED_BIOMETRIC: return "locked_biometric";
        case DeviceLockState::PASSWORD_ONLY: return "password_only";
        default: return "unknown";
    }
}

IntegrityLevel DeviceIntegrityManager::stringToIntegrityLevel(const QString& level) const {
    QString lower = level.toLower();
    if (lower == "basic") return IntegrityLevel::BASIC;
    if (lower == "basic_hardware") return IntegrityLevel::BASIC_HARDWARE;
    if (lower == "certified") return IntegrityLevel::CERTIFIED;
    if (lower == "verified_boot") return IntegrityLevel::VERIFIED_BOOT;
    return IntegrityLevel::BASIC;
}

bool DeviceIntegrityManager::evaluateIntegrityChecks(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    DeviceIntegrityState& state = m_states[instanceId];
    state.totalChecksPassed = 0;
    state.totalChecksFailed = 0;
    
    // Run all checks
    QList<IntegrityCheckResult> results;
    results.append(runIntegrityCheck(instanceId, "basic_integrity"));
    results.append(runIntegrityCheck(instanceId, "cts_profile_match"));
    results.append(runIntegrityCheck(instanceId, "verified_boot"));
    results.append(runIntegrityCheck(instanceId, "selinux"));
    results.append(runIntegrityCheck(instanceId, "system_integrity"));
    results.append(runIntegrityCheck(instanceId, "hardware_attestation"));
    results.append(runIntegrityCheck(instanceId, "encryption"));
    results.append(runIntegrityCheck(instanceId, "gatekeeper"));
    
    m_checkResults[instanceId] = results;
    
    return true;
}

} // namespace VirtualPhonePro
