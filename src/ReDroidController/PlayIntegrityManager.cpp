/**
 * @file PlayIntegrityManager.cpp
 * @brief Play Integrity & SafetyNet Handler Implementation
 */

#include "VirtualPhonePro/PlayIntegrityManager.hpp"
#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/UniqueDeviceGenerator.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QProcess>

namespace VirtualPhonePro {

// ========================================================================
// SINGLETON
// ========================================================================

PlayIntegrityManager* PlayIntegrityManager::s_instance = nullptr;

PlayIntegrityManager& PlayIntegrityManager::instance() {
    if (!s_instance) {
        s_instance = new PlayIntegrityManager();
    }
    return *s_instance;
}

PlayIntegrityManager::PlayIntegrityManager(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[PlayIntegrity] Manager initialized";
}

PlayIntegrityManager::~PlayIntegrityManager() {
}

// ========================================================================
// JSON CONVERSION
// ========================================================================

QJsonObject IntegrityConfig::toJson() const {
    QJsonObject obj;
    obj["targetVerdict"] = static_cast<int>(targetVerdict);
    obj["requireHardwareAttestation"] = requireHardwareAttestation;
    obj["isDeviceRooted"] = isDeviceRooted;
    obj["isSystemRooted"] = isSystemRooted;
    obj["isDebuggable"] = isDebuggable;
    obj["isDebuggableByADB"] = isDebuggableByADB;
    obj["hasUnknownSources"] = hasUnknownSources;
    obj["isEmulator"] = isEmulator;
    obj["isHookDetected"] = isHookDetected;
    obj["isFridaDetected"] = isFridaDetected;
    obj["isXposedDetected"] = isXposedDetected;
    obj["isKVMEnabled"] = isKVMEnabled;
    obj["hasHardwareVirtualization"] = hasHardwareVirtualization;
    obj["hasVirGLGPU"] = hasVirGLGPU;
    obj["usesVirtIO"] = usesVirtIO;
    obj["bootloaderLockState"] = bootloaderLockState;
    obj["verifiedBootState"] = verifiedBootState;
    obj["isVerifiedBootEnabled"] = isVerifiedBootEnabled;
    obj["isBetaAutoUpdate"] = isBetaAutoUpdate;
    obj["isSysIntegrityCheckEnabled"] = isSysIntegrityCheckEnabled;
    obj["isSecurityPatchCurrent"] = isSecurityPatchCurrent;
    obj["securityPatchLevel"] = securityPatchLevel;
    obj["isGMSCertified"] = isGMSCertified;
    obj["isPlayServicesValid"] = isPlayServicesValid;
    return obj;
}

QJsonObject IntegrityCheckResult::toJson() const {
    QJsonObject obj;
    obj["success"] = success;
    obj["verdict"] = static_cast<int>(verdict);
    obj["isDeviceIntegrityPass"] = isDeviceIntegrityPass;
    obj["isBasicIntegrityPass"] = isBasicIntegrityPass;
    obj["isStrongIntegrityPass"] = isStrongIntegrityPass;
    obj["isCtsProfileMatch"] = isCtsProfileMatch;
    obj["isCtsProfileMatchParallel"] = isCtsProfileMatchParallel;
    obj["isBasicMacAddressCheck"] = isBasicMacAddressCheck;
    obj["isRuntimeBit"] = isRuntimeBit;
    obj["deviceCategory"] = deviceCategory;
    obj["manufacturer"] = manufacturer;
    obj["model"] = model;
    obj["brand"] = brand;
    obj["androidVersion"] = androidVersion;
    obj["buildFingerprint"] = buildFingerprint;
    obj["advice"] = advice;
    obj["evaluationType"] = evaluationType;
    obj["timestamp"] = timestamp;
    obj["errorMessage"] = errorMessage;
    obj["errorCode"] = errorCode;
    return obj;
}

QString IntegrityCheckResult::getSummary() const {
    if (success) {
        return QString("PASS - Device: %1 %2, Verdict: %3")
            .arg(manufacturer, model)
            .arg(static_cast<int>(verdict));
    } else {
        return QString("FAIL - %1 (Code: %2)")
            .arg(errorMessage)
            .arg(errorCode);
    }
}

QJsonObject SafetyNetResponse::toJson() const {
    QJsonObject obj;
    obj["isValid"] = isValid;
    obj["basicIntegrity"] = basicIntegrity;
    obj["ctsProfileMatch"] = ctsProfileMatch;
    obj["bootloader"] = bootloader;
    obj["carrier"] = carrier;
    obj["device"] = device;
    obj["fingerprint"] = fingerprint;
    obj["hardware"] = hardware;
    obj["manufacturer"] = manufacturer;
    obj["model"] = model;
    obj["product"] = product;
    obj["osVersion"] = osVersion;
    obj["securityPatch"] = securityPatch;
    obj["measurement"] = measurement;
    obj["nonce"] = nonce;
    obj["timestampMs"] = timestampMs;
    obj["errorType"] = errorType;
    obj["errorCode"] = errorCode;
    return obj;
}

QJsonObject PlayIntegrityResponse::toJson() const {
    QJsonObject obj;
    obj["isValid"] = isValid;
    obj["tokenPayload"] = tokenPayload;
    obj["deviceIntegrityVerdict"] = deviceIntegrityVerdict;
    obj["deviceRecognitionVerdict"] = deviceRecognitionVerdict;
    obj["accountDetails"] = accountDetails;
    obj["nonce"] = nonce;
    obj["timestampMs"] = timestampMs;
    obj["errorMessage"] = errorMessage;
    obj["errorCode"] = errorCode;
    return obj;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void PlayIntegrityManager::setConfig(const QString& instanceId, const IntegrityConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId] = config;
    qDebug() << "[PlayIntegrity] Config set for:" << instanceId;
}

IntegrityConfig PlayIntegrityManager::getConfig(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_configs.value(instanceId);
}

void PlayIntegrityManager::resetConfig(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    IntegrityConfig defaultConfig;
    defaultConfig.targetVerdict = IntegrityVerdict::PLAY_INTEGRITY_DEVICE;
    defaultConfig.isKVMEnabled = true;
    defaultConfig.hasHardwareVirtualization = true;
    defaultConfig.verifiedBootState = "green";
    defaultConfig.bootloaderLockState = "locked";
    defaultConfig.isGMSCertified = true;
    
    m_configs[instanceId] = defaultConfig;
    qDebug() << "[PlayIntegrity] Config reset for:" << instanceId;
}

// ========================================================================
// VIRTUALIZATION SETUP
// ========================================================================

bool PlayIntegrityManager::isKVMAvailable() const {
    QFile kvmCheck("/dev/kvm");
    return kvmCheck.exists();
}

void PlayIntegrityManager::configureKVM(const QString& instanceId, bool enableKVM,
                                       const QString& cpuModel, bool gpuPassthrough) {
    QMutexLocker locker(&m_mutex);
    
    IntegrityConfig& config = m_configs[instanceId];
    config.isKVMEnabled = enableKVM;
    config.hasHardwareVirtualization = enableKVM;
    config.hasVirGLGPU = gpuPassthrough;
    
    m_kvmStatus[instanceId] = enableKVM;
    
    qDebug() << "[PlayIntegrity] KVM configured for" << instanceId 
             << "enabled:" << enableKVM << "cpu:" << cpuModel;
    
    emit kvmStatusChanged(instanceId, enableKVM);
}

QJsonObject PlayIntegrityManager::getVirtualizationStatus(const QString& instanceId) const {
    QJsonObject status;
    
    status["kvmAvailable"] = isKVMAvailable();
    status["kvmEnabled"] = m_kvmStatus.value(instanceId, false);
    
    IntegrityConfig config = getConfig(instanceId);
    status["hardwareVirtualization"] = config.hasHardwareVirtualization;
    status["virGLGPU"] = config.hasVirGLGPU;
    status["virtIO"] = config.usesVirtIO;
    status["cpuModel"] = "cortex-a76";
    status["gpuType"] = config.hasVirGLGPU ? "virgl" : "swiftshader";
    
    return status;
}

// ========================================================================
// INTEGRITY CHECKS
// ========================================================================

IntegrityCheckResult PlayIntegrityManager::performIntegrityCheck(const QString& instanceId) {
    IntegrityCheckResult result;
    result.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    IntegrityConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    qDebug() << "[PlayIntegrity] Performing integrity check for:" << instanceId;
    
    // Check 1: Basic Integrity
    result.isBasicIntegrityPass = !config.isDebuggable && 
                                  !config.hasUnknownSources &&
                                  !config.isHookDetected;
    
    // Check 2: Device Integrity
    result.isDeviceIntegrityPass = result.isBasicIntegrityPass &&
                                   !config.isDeviceRooted &&
                                   !config.isEmulator &&
                                   config.isKVMEnabled;
    
    // Check 3: Strong Integrity (if hardware attestation available)
    result.isStrongIntegrityPass = result.isDeviceIntegrityPass &&
                                   config.hasHardwareVirtualization;
    
    // Check 4: CTS Profile Match
    result.isCtsProfileMatch = result.isBasicIntegrityPass &&
                               !config.isDebuggableByADB &&
                               config.isVerifiedBootEnabled;
    
    result.isCtsProfileMatchParallel = result.isCtsProfileMatch;
    result.isBasicMacAddressCheck = true;
    result.isRuntimeBit = false;
    
    // Generate verdicts
    if (config.isKVMEnabled && config.hasHardwareVirtualization) {
        result.verdict = IntegrityVerdict::PLAY_INTEGRITY_DEVICE;
        result.success = true;
    } else if (result.isBasicIntegrityPass) {
        result.verdict = IntegrityVerdict::PLAY_INTEGRITY_BASIC;
        result.success = true;
    } else {
        result.verdict = IntegrityVerdict::PLAY_INTEGRITY_NONE;
        result.success = false;
        result.errorMessage = "Integrity checks failed";
        result.errorCode = 1;
    }
    
    // Device info
    result.deviceCategory = config.isKVMEnabled ? "PHYSICAL" : "VIRTUAL";
    result.manufacturer = controller.getProperty(instanceId, "ro.product.manufacturer");
    result.model = controller.getProperty(instanceId, "ro.product.model");
    result.brand = controller.getProperty(instanceId, "ro.product.brand");
    result.androidVersion = controller.getProperty(instanceId, "ro.build.version.release");
    result.buildFingerprint = controller.getProperty(instanceId, "ro.build.fingerprint");
    
    result.advice = result.success ? "MEETS_DEVICE_INTEGRITY" : "UNSATISFIED";
    result.evaluationType = result.success ? "INTEGRITY" : "NONE";
    
    qDebug() << "[PlayIntegrity] Check result:" << result.getSummary();
    
    emit integrityCheckCompleted(instanceId, result);
    return result;
}

IntegrityVerdict PlayIntegrityManager::checkBasicIntegrity(const QString& instanceId) {
    IntegrityConfig config = getConfig(instanceId);
    
    if (config.isDebuggable || config.hasUnknownSources || config.isHookDetected) {
        return IntegrityVerdict::PLAY_INTEGRITY_NONE;
    }
    
    return IntegrityVerdict::PLAY_INTEGRITY_BASIC;
}

IntegrityVerdict PlayIntegrityManager::checkDeviceIntegrity(const QString& instanceId) {
    IntegrityConfig config = getConfig(instanceId);
    
    if (!config.isKVMEnabled) {
        return IntegrityVerdict::PLAY_INTEGRITY_UNSATISFIED;
    }
    
    if (config.isDeviceRooted || config.isEmulator) {
        return IntegrityVerdict::PLAY_INTEGRITY_BASIC;
    }
    
    return IntegrityVerdict::PLAY_INTEGRITY_DEVICE;
}

bool PlayIntegrityManager::checkGMSCertification(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    return m_gmsCertified.value(instanceId, true);
}

// ========================================================================
// GMS CERTIFICATION
// ========================================================================

void PlayIntegrityManager::configureGMSCertification(const QString& instanceId, bool certified) {
    QMutexLocker locker(&m_mutex);
    m_gmsCertified[instanceId] = certified;
    
    IntegrityConfig& config = m_configs[instanceId];
    config.isGMSCertified = certified;
    config.isPlayServicesValid = certified;
    
    qDebug() << "[PlayIntegrity] GMS certification set for" << instanceId << certified;
    
    emit gmsCertificationChanged(instanceId, certified);
}

bool PlayIntegrityManager::isGMSCertified(const QString& instanceId) const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_gmsCertified.value(instanceId, true);
}

bool PlayIntegrityManager::applyPlayServicesValidation(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    // Apply GMS properties
    QStringList commands = {
        // GMS Certification
        "setprop ro.com.google.gmsgame 1",
        "setprop ro.com.google.clientidbase.gms 1",
        "setprop ro.setupwizard.mode=OPTIONAL",
        
        // Play Services validation
        "setprop ro.com.google.gms com.google.android.gms",
        "setprop gms.play.games.C2D_BACKUP_ID 3794883744023732994",
        
        // Hide root
        "setprop ro.build.selinux.enforce 0",
        
        // Hide unknown sources
        "settings put global install_non_market_apps 0",
        
        // Disable debug
        "setprop ro.debuggable 0",
        
        // Verified boot
        "setprop ro.boot.verifiedbootstate green",
        "setprop ro.verity.mode enforcing"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[PlayIntegrity] Play Services validation applied";
    return true;
}

// ========================================================================
// DEVICE PROPERTY SPOOFING
// ========================================================================

bool PlayIntegrityManager::applyIntegrityProperties(const QString& instanceId) {
    IntegrityConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands;
    
    // Bootloader lock state
    commands.append(QString("setprop ro.boot.flash.locked %1").arg(
        config.bootloaderLockState == "locked" ? "1" : "0"));
    commands.append(QString("setprop ro.bootloader.locked %1").arg(
        config.bootloaderLockState == "locked" ? "true" : "false"));
    
    // Verified boot state
    commands.append(QString("setprop ro.boot.verifiedbootstate %1").arg(config.verifiedBootState));
    commands.append(QString("setprop ro.verity.mode %1").arg(
        config.verifiedBootState == "green" ? "enforcing" : "disabled"));
    commands.append(QString("setprop ro.verifiedbootstate %1").arg(config.verifiedBootState));
    
    // Device integrity
    commands.append("setprop ro.device.flags 0");
    commands.append("setprop ro.setupwizard.mode=OPTIONAL");
    
    // Hide debug flags
    if (!config.isDebuggable) {
        commands.append("setprop ro.debuggable 0");
        commands.append("setprop persist.sys.debug.atrace 0");
    }
    
    // Security patch
    commands.append(QString("setprop ro.build.version.security_patch %1").arg(config.securityPatchLevel));
    commands.append(QString("setprop ro.build.version.all_codenames %1").arg(config.androidVersion()));
    
    // Hide root
    if (!config.isDeviceRooted) {
        commands.append("setprop ro.build.selinux.enforce 0");
    }
    
    // System integrity
    if (config.isSysIntegrityCheckEnabled) {
        commands.append("setprop ro.config.system_integrity.enabled true");
    }
    
    // Apply all commands
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[PlayIntegrity] Integrity properties applied";
    return true;
}

void PlayIntegrityManager::setBootloaderLockState(const QString& instanceId, const QString& state) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId].bootloaderLockState = state;
}

void PlayIntegrityManager::setVerifiedBootState(const QString& instanceId, const QString& state) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId].verifiedBootState = state;
}

void PlayIntegrityManager::configureSystemIntegrity(const QString& instanceId, bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId].isSysIntegrityCheckEnabled = enabled;
}

void PlayIntegrityManager::setSecurityPatchLevel(const QString& instanceId, const QString& patchLevel) {
    QMutexLocker locker(&m_mutex);
    m_configs[instanceId].securityPatchLevel = patchLevel;
}

// ========================================================================
// EMULATOR DETECTION BYPASS
// ========================================================================

bool PlayIntegrityManager::bypassEmulatorDetection(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    qDebug() << "[PlayIntegrity] Bypassing emulator detection for:" << instanceId;
    
    // Remove emulator-specific properties
    QStringList removeCommands = {
        // Hide QEMU detection
        "settings delete global/qemu_adb_enabled",
        "resetprop -v ro.kernel.qemu 0",
        "resetprop -v ro.boot.qemu 0",
        "resetprop -vro.bootloader.flash.locked 1",
        
        // Hide Goldfish (Android emulator)
        "resetprop -v ro.hardware goldfish",
        "resetprop -v ro.hardware.audio.primary goldfish",
        "resetprop -v ro.opengles.version 131072",
        
        // Remove emulator files
        "rm -f /system/lib/libc_qemud.so",
        "rm -f /system/lib/libcutils.so",
        "rm -f /system/bin/qemud",
        "rm -f /dev/qemu_pipe",
        "rm -f /dev/socket/qemud",
        
        // Hide emulator detection files
        "rm -f /system/xbin/su.bak",
        "rm -f /system/app/Superuser.apk"
    };
    
    // Replace with device properties
    QStringList replaceCommands = {
        // Hide QEMU properties
        "resetprop ro.kernel.qemu 0",
        "resetprop ro.boot.qemu 0",
        
        // Set real hardware
        "resetprop ro.hardware mt6885",
        "resetprop ro.product.board mtk6885",
        "resetprop ro.mediatek.platform MT6885",
        
        // Hide emulator files by creating placeholders
        "touch /dev/qemu_pipe",
        "chmod 000 /dev/qemu_pipe",
        
        // Remove detection paths
        "rm -rf /data/local/tmp",
        "mkdir /data/local/tmp",
        "chmod 000 /data/local/tmp"
    };
    
    for (const QString& cmd : removeCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    for (const QString& cmd : replaceCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool PlayIntegrityManager::configureHardwareVirtualization(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    IntegrityConfig& config = m_configs[instanceId];
    
    // Enable hardware virtualization
    config.isKVMEnabled = true;
    config.hasHardwareVirtualization = true;
    config.isEmulator = false; // ReDroid with KVM is NOT emulator
    
    m_kvmStatus[instanceId] = true;
    
    qDebug() << "[PlayIntegrity] Hardware virtualization configured for:" << instanceId;
    
    emit kvmStatusChanged(instanceId, true);
    return true;
}

bool PlayIntegrityManager::removeEmulatorArtifacts(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList removeCommands = {
        // Remove QEMU/Goldfish artifacts
        "rm -f /system/lib/libc_qemud.so",
        "rm -f /system/lib64/libc_qemud.so",
        "rm -f /system/lib/egl/libEGL_qemu.so",
        "rm -f /system/lib64/egl/libEGL_qemu.so",
        "rm -f /system/lib/egl/libGLESv1_CM_qemu.so",
        "rm -f /system/lib64/egl/libGLESv1_CM_qemu.so",
        "rm -f /system/lib/egl/libGLESv2_qemu.so",
        "rm -f /system/lib64/egl/libGLESv2_qemu.so",
        
        // Remove emulator detection tools
        "rm -f /system/xbin/property-test",
        "rm -f /system/xbin/crash_report",
        
        // Remove goldfish-specific stuff
        "rm -f /system/bin/qemu-props",
        "rm -f /system/bin/qemu-service",
        
        // Clean up
        "rm -f /data/*.log",
        "rm -f /data/local/*.log"
    };
    
    for (const QString& cmd : removeCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[PlayIntegrity] Emulator artifacts removed";
    return true;
}

bool PlayIntegrityManager::hideVirtualizationArtifacts(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    // Hide container/virtualization detection
    QStringList hideCommands = {
        // Hide container
        "resetprop ro.build.type user",
        
        // Hide KVM
        "resetprop ro.hardware.virtual false",
        "resetprop ro.hardware.kvm 0",
        
        // Hide VirtIO
        "resetprop ro.virtio.enabled false",
        
        // Set real device type
        "resetprop ro.product.device mt6885",
        "resetprop ro.product.model.sm-s928b",
        "resetprop ro.product.manufacturer samsung",
        
        // Hide systemd container
        "resetprop init.svc.console started",
        "resetprop init.svc.debuggerd running"
    };
    
    for (const QString& cmd : hideCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[PlayIntegrity] Virtualization artifacts hidden";
    return true;
}

// ========================================================================
// SAFETYNET & PLAY INTEGRITY
// ========================================================================

SafetyNetResponse PlayIntegrityManager::verifySafetyNet(const QString& instanceId, const QString& attestationResponse) {
    SafetyNetResponse response;
    
    IntegrityConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    qDebug() << "[PlayIntegrity] Verifying SafetyNet attestation";
    
    // If we have KVM and device is properly configured
    if (config.isKVMEnabled && !config.isDeviceRooted && !config.isDebuggable) {
        response.isValid = true;
        response.basicIntegrity = true;
        response.ctsProfileMatch = true;
        
        // Get device properties
        response.bootloader = controller.getProperty(instanceId, "ro.bootloader");
        response.manufacturer = controller.getProperty(instanceId, "ro.product.manufacturer");
        response.model = controller.getProperty(instanceId, "ro.product.model");
        response.device = controller.getProperty(instanceId, "ro.product.device");
        response.fingerprint = controller.getProperty(instanceId, "ro.build.fingerprint");
        response.hardware = controller.getProperty(instanceId, "ro.hardware");
        response.carrier = "Samsung";
        response.osVersion = controller.getProperty(instanceId, "ro.build.version.release");
        response.securityPatch = config.securityPatchLevel;
        
        // Generate measurement (SHA256 of device properties)
        QString measurementInput = response.fingerprint + response.bootloader;
        response.measurement = QString(QCryptographicHash::hash(
            measurementInput.toUtf8(), QCryptographicHash::Sha256).toHex());
        
        response.timestampMs = QDateTime::currentMSecsSinceEpoch();
        response.nonce = generateNonce(instanceId);
    } else {
        response.isValid = false;
        response.basicIntegrity = false;
        response.ctsProfileMatch = false;
        response.errorType = "NOT_SUPPORTED";
        response.errorCode = -1;
    }
    
    return response;
}

PlayIntegrityResponse PlayIntegrityManager::verifyPlayIntegrity(const QString& instanceId, const QString& token) {
    PlayIntegrityResponse response;
    
    IntegrityConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    qDebug() << "[PlayIntegrity] Verifying Play Integrity token";
    
    // Check if device meets requirements
    IntegrityVerdict verdict = checkDeviceIntegrity(instanceId);
    
    response.isValid = (verdict == IntegrityVerdict::PLAY_INTEGRITY_DEVICE ||
                       verdict == IntegrityVerdict::PLAY_INTEGRITY_BASIC);
    
    response.timestampMs = QDateTime::currentMSecsSinceEpoch();
    response.nonce = generateNonce(instanceId);
    
    // Generate device integrity verdict
    if (config.isKVMEnabled && config.hasHardwareVirtualization && !config.isDeviceRooted) {
        response.deviceIntegrityVerdict = "MEETS_DEVICE_INTEGRITY";
    } else if (!config.isDeviceRooted && !config.isDebuggable) {
        response.deviceIntegrityVerdict = "MEETS_BASIC_INTEGRITY";
    } else {
        response.deviceIntegrityVerdict = "UNSATISFIED";
        response.isValid = false;
    }
    
    response.deviceRecognitionVerdict = generateDeviceRecognitionVerdict(instanceId);
    
    return response;
}

// ========================================================================
// HELPER METHODS
// ========================================================================

QString PlayIntegrityManager::generateNonce(const QString& instanceId) {
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString random = QString::number(QRandomGenerator::global()->generate());
    QString combined = instanceId + timestamp + random;
    
    return QString(QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QJsonObject PlayIntegrityManager::buildIntegrityPayload(const QString& instanceId) {
    QJsonObject payload;
    IntegrityConfig config = getConfig(instanceId);
    ReDroidController& controller = ReDroidController::instance();
    
    payload["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    payload["nonce"] = generateNonce(instanceId);
    payload["deviceCategory"] = generateDeviceCategory(instanceId);
    
    QJsonObject device;
    device["manufacturer"] = controller.getProperty(instanceId, "ro.product.manufacturer");
    device["model"] = controller.getProperty(instanceId, "ro.product.model");
    device["brand"] = controller.getProperty(instanceId, "ro.product.brand");
    device["fingerprint"] = controller.getProperty(instanceId, "ro.build.fingerprint");
    device["bootloader"] = controller.getProperty(instanceId, "ro.bootloader");
    
    payload["device"] = device;
    
    QJsonObject integrity;
    integrity["basicIntegrity"] = !config.isDebuggable && !config.hasUnknownSources;
    integrity["deviceIntegrity"] = config.isKVMEnabled && !config.isDeviceRooted;
    integrity["strongIntegrity"] = config.hasHardwareVirtualization;
    integrity["ctsProfileMatch"] = !config.isDebuggableByADB;
    
    payload["integrity"] = integrity;
    
    return payload;
}

QString PlayIntegrityManager::generateDeviceRecognitionVerdict(const QString& instanceId) {
    IntegrityConfig config = getConfig(instanceId);
    
    if (config.isKVMEnabled && config.hasHardwareVirtualization) {
        return "RECOGNIZED";
    } else if (!config.isEmulator) {
        return "UNRECOGNIZED";
    }
    
    return "UNAVAILABLE";
}

QString PlayIntegrityManager::generateDeviceCategory(const QString& instanceId) {
    IntegrityConfig config = getConfig(instanceId);
    
    if (config.isKVMEnabled) {
        return "PHYSICAL"; // KVM makes it appear as physical
    }
    
    return "VIRTUAL";
}

// ========================================================================
// TESTING & VERIFICATION
// ========================================================================

QJsonObject PlayIntegrityManager::runIntegrityTest(const QString& instanceId) {
    QJsonObject testResult;
    
    qDebug() << "[PlayIntegrity] Running integrity test for:" << instanceId;
    
    // Run all checks
    IntegrityCheckResult result = performIntegrityCheck(instanceId);
    testResult["integrityCheck"] = result.toJson();
    
    // Check virtualization
    testResult["virtualizationStatus"] = getVirtualizationStatus(instanceId);
    
    // Check GMS
    testResult["isGMSCertified"] = isGMSCertified(instanceId);
    
    // Check KVM
    testResult["kvmAvailable"] = isKVMAvailable();
    testResult["kvmEnabled"] = m_kvmStatus.value(instanceId, false);
    
    // Overall result
    testResult["overallPass"] = result.success && 
                                m_kvmStatus.value(instanceId, false) &&
                                isGMSCertified(instanceId);
    
    return testResult;
}

QString PlayIntegrityManager::generateMockAttestation(const QString& instanceId, bool shouldPass) {
    IntegrityCheckResult result = performIntegrityCheck(instanceId);
    
    QJsonObject mockResponse;
    mockResponse["success"] = shouldPass;
    mockResponse["nonce"] = generateNonce(instanceId);
    mockResponse["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QJsonObject evaluation;
    evaluation["basicIntegrity"] = shouldPass;
    evaluation["ctsProfileMatch"] = shouldPass;
    evaluation["basicIntegrityDeviceCertificate"] = shouldPass;
    evaluation["basicIntegritySystemPartition"] = shouldPass;
    
    mockResponse["evaluation"] = evaluation;
    
    return QString(QJsonDocument(mockResponse).toJson(QJsonDocument::Compact));
}

bool PlayIntegrityManager::verifyMockResponse(const QString& response) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject obj = doc.object();
    return obj["success"].toBool(false);
}

} // namespace VirtualPhonePro
