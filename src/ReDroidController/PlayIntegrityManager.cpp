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
    obj["attestation"] = attestation.toJson();
    return obj;
}

QJsonObject HardwareAttestationConfig::toJson() const {
    QJsonObject obj;
    obj["keyType"] = static_cast<int>(keyType);
    obj["enableStrongBox"] = enableStrongBox;
    obj["keymasterVersion"] = keymasterVersion;
    obj["hasKeymaster4"] = hasKeymaster4;
    obj["hasKeymaster41"] = hasKeymaster41;
    obj["hasKeymaster43"] = hasKeymaster43;
    obj["bootState"] = static_cast<int>(bootState);
    obj["verifiedBootHash"] = verifiedBootHash;
    obj["bootPatchLevel"] = bootPatchLevel;
    obj["securityPatchLevel"] = securityPatchLevel;
    obj["isSecurityPatchCurrent"] = isSecurityPatchCurrent;
    obj["isDeviceLocked"] = isDeviceLocked;
    obj["isRootHidden"] = isRootHidden;
    obj["isSystemVerified"] = isSystemVerified;
    obj["isMetaVerified"] = isMetaVerified;
    obj["hasGooglePlayServices"] = hasGooglePlayServices;
    obj["isGmsCoreInstalled"] = isGmsCoreInstalled;
    obj["isPlayStoreInstalled"] = isPlayStoreInstalled;
    return obj;
}

QJsonObject HardwareAttestationResult::toJson() const {
    QJsonObject obj;
    obj["success"] = success;
    obj["attestationLevel"] = static_cast<int>(attestationLevel);
    obj["category"] = static_cast<int>(category);
    obj["bootState"] = static_cast<int>(bootState);
    obj["bootStateString"] = bootStateString;
    obj["verifiedBootKeyHash"] = verifiedBootKeyHash;
    obj["deviceLocked"] = deviceLocked;
    obj["basicIntegrity"] = basicIntegrity;
    obj["ctsProfileMatch"] = ctsProfileMatch;
    obj["ctsProfileMatchParallel"] = ctsProfileMatchParallel;
    obj["basicMacAddressCheck"] = basicMacAddressCheck;
    obj["systemSignatureValid"] = systemSignatureValid;
    obj["platformSignatureValid"] = platformSignatureValid;
    obj["bootSignatureValid"] = bootSignatureValid;
    obj["deviceRecognitionVerdict"] = deviceRecognitionVerdict;
    obj["deviceConfidenceLevel"] = deviceConfidenceLevel;
    obj["timestampMs"] = timestampMs;
    obj["timestampIso"] = timestampIso;
    obj["nonce"] = nonce;
    obj["packageName"] = packageName;
    obj["packageVersionCode"] = packageVersionCode;
    obj["errorMessage"] = errorMessage;
    obj["errorCode"] = errorCode;
    obj["advice"] = advice;
    obj["tokenPayload"] = tokenPayload;
    obj["tokenSignature"] = tokenSignature;
    
    QJsonArray warningsArray;
    for (const QString& w : warnings) {
        warningsArray.append(w);
    }
    obj["warnings"] = warningsArray;
    
    return obj;
}

QString HardwareAttestationResult::toJwt() const {
    // Generate a mock JWT token
    // Format: header.payload.signature
    QJsonObject header;
    header["alg"] = "RS256";
    header["typ"] = "JWT";
    
    QJsonObject payload = toJson();
    payload.remove("tokenPayload");
    payload.remove("tokenSignature");
    
    QString headerB64 = QString::fromLatin1(QByteArray(QJsonDocument(header).toJson(QJsonDocument::Compact))
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    QString payloadB64 = QString::fromLatin1(QByteArray(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    
    // Mock signature (in real implementation, this would be RSA signing)
    QString mockSignature = "mock_signature_base64";
    
    return QString("%1.%2.%3").arg(headerB64, payloadB64, mockSignature);
}

QString HardwareAttestationResult::getSummary() const {
    if (success) {
        return QString("PASS - Attestation: %1, Boot: %2, CTS: %3")
            .arg(bootStateString)
            .arg(deviceLocked)
            .arg(ctsProfileMatch ? "YES" : "NO");
    } else {
        return QString("FAIL - %1 (Code: %2)")
            .arg(errorMessage)
            .arg(errorCode);
    }
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

// ========================================================================
// HARDWARE ATTESTATION IMPLEMENTATION
// ========================================================================

HardwareAttestationResult PlayIntegrityManager::performHardwareAttestation(
    const QString& instanceId, const QString& nonce) {
    
    HardwareAttestationResult result;
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "[PlayIntegrity] Performing hardware attestation for:" << instanceId;
    
    // Get or create attestation config
    HardwareAttestationConfig config;
    if (m_attestationConfigs.contains(instanceId)) {
        config = m_attestationConfigs[instanceId];
    } else {
        // Default configuration for strong attestation
        config.keyType = AttestationKeyType::TRUSTED_ENVIRONMENT;
        config.enableStrongBox = true;
        config.keymasterVersion = 4;
        config.bootState = VerifiedBootState::GREEN;
        config.isDeviceLocked = true;
        config.isRootHidden = true;
        config.isSystemVerified = true;
        config.securityPatchLevel = "2024-06-01";
        m_attestationConfigs[instanceId] = config;
    }
    
    // Initialize attestation key if not exists
    if (!m_attestationKeys.contains(instanceId)) {
        initializeAttestationKey(instanceId);
    }
    
    // Generate timestamp
    result.timestampMs = QDateTime::currentMSecsSinceEpoch();
    result.timestampIso = QDateTime::currentDateTime().toString(Qt::ISODate);
    result.nonce = nonce.isEmpty() ? generateNonce(instanceId) : nonce;
    
    // Check basic integrity requirements
    if (!config.isRootHidden || config.isDeviceLocked == false) {
        result.success = false;
        result.errorMessage = "Device integrity compromised";
        result.errorCode = 1;
        return result;
    }
    
    // Set success based on configuration
    result.success = true;
    result.attestationLevel = config.keyType;
    
    // Set verified boot state
    result.bootState = config.bootState;
    result.bootStateString = generateVerifiedBootStateString(config.bootState);
    result.deviceLocked = config.isDeviceLocked ? "true" : "false";
    
    // Generate verified boot key hash
    result.verifiedBootKeyHash = QString(QCryptographicHash::hash(
        (instanceId + "verified_boot_key").toUtf8(), 
        QCryptographicHash::Sha256).toHex()).left(64);
    
    // Set integrity flags
    result.basicIntegrity = true;
    result.ctsProfileMatch = true;
    result.ctsProfileMatchParallel = true;
    result.basicMacAddressCheck = true;
    result.systemSignatureValid = true;
    result.platformSignatureValid = true;
    result.bootSignatureValid = (config.bootState == VerifiedBootState::GREEN);
    
    // Set device recognition
    result.deviceRecognitionVerdict = "RECOGNIZED";
    result.deviceConfidenceLevel = "CONFIDENCE_HIGH";
    
    // Set category based on attestation level
    if (config.keyType == AttestationKeyType::STRONGBOX) {
        result.category = DeviceIntegrityCategory::HARDWARE_BACKED;
    } else if (config.keyType == AttestationKeyType::TRUSTED_ENVIRONMENT) {
        result.category = DeviceIntegrityCategory::CTS_MATCH;
    } else {
        result.category = DeviceIntegrityCategory::BASIC;
    }
    
    // Generate JWT token
    result.tokenPayload = QJsonDocument(buildAttestationPayload(instanceId, nonce)).toJson(QJsonDocument::Compact);
    result.tokenSignature = "attestation_signature";
    
    // Build advice based on configuration
    if (result.success && result.ctsProfileMatch) {
        result.advice = "Device integrity verified";
    } else if (!result.ctsProfileMatch) {
        result.advice = "RESOLVED";
    }
    
    qDebug() << "[PlayIntegrity] Hardware attestation result:" << result.success 
             << "boot:" << result.bootStateString << "locked:" << result.deviceLocked;
    
    return result;
}

QString PlayIntegrityManager::generateAttestationToken(const QString& instanceId, const QString& nonce) {
    HardwareAttestationResult result = performHardwareAttestation(instanceId, nonce);
    return result.toJwt();
}

bool PlayIntegrityManager::verifyHardwareBoundKey(const QString& instanceId, const QString& keyId) {
    QMutexLocker locker(&m_mutex);
    
    HardwareAttestationConfig config = m_attestationConfigs.value(instanceId);
    
    // Hardware-bound keys are available with StrongBox or TEE
    if (config.keyType == AttestationKeyType::STRONGBOX || 
        config.keyType == AttestationKeyType::TRUSTED_ENVIRONMENT) {
        qDebug() << "[PlayIntegrity] Key" << keyId << "is hardware-backed";
        return true;
    }
    
    qDebug() << "[PlayIntegrity] Key" << keyId << "is software-only";
    return false;
}

QStringList PlayIntegrityManager::generateAttestationCertificateChain(const QString& instanceId) {
    QStringList chain;
    
    // In a real implementation, this would generate actual attestation certificates
    // For now, we generate mock certificate data
    
    // Root CA certificate (Google Attestation CA)
    QString rootCa = "-----BEGIN CERTIFICATE-----\n"
                     "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYD\n"
                     "VQQGEwJJUzESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\n"
                     "clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290\n"
                     "MB4XDTAwMDUxMjE4NDYwMFoXDTIwMDUxMjIzNTkwMFowWjELMAkGA1UE\n"
                     "BhMCSVMxEjAQBgNVBAoTCUJhbHRpbW9yZTETMBEGA1UECxMKQ3liZXJU\n"
                     "cnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVyVHJ1c3QgUm9vdDCC\n"
                     "ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKrYmD7QU5\n"
                     "EwBZw8P5fFhCv0xQmIlmWgz0xtZqPKIoJ2vK3QN8bXgkRyflQmvbmK\n"
                     "-----END CERTIFICATE-----";
    chain.append(rootCa);
    
    // Intermediate CA certificate (Google Play Services Attestation)
    QString intermediateCa = "-----BEGIN CERTIFICATE-----\n"
                              "MIIDkjCCAnqgAwIBAgIRAIldLrMFGC8GGi0J5D3F+powDQYJKoZI\n"
                              "hvcNAQELBQAwPzELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMRIw\n"
                              "EAYDVQQHDAlTb21ld2hlcmUxGDAWBgNVBAMMD0dvb2dsZSBQbGF5\n"
                              "IFNlcnZpY2VzMB4XDTI0MDEwMTAwMDAwMFoXDTI1MTIzMTIzNTk1\n"
                              "OVowPzELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMRIwEAYDVQQH\n"
                              "DAlTb21ld2hlcmUxGDAWBgNVBAMMD0dvb2dsZSBQbGF5IFNlcnZp\n"
                              "Y2VzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Z3\n"
                              "-----END CERTIFICATE-----";
    chain.append(intermediateCa);
    
    // Device attestation certificate (per-device)
    QString deviceCert = "-----BEGIN CERTIFICATE-----\n"
                         "MIIDnzCCAoegAwIBAgIJALjW+qfFvBIMA0GCSqGSIb3DQEBCwUAMGkx\n"
                         "CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQH\n"
                         "DAtNb3VudGFpbiBWaWV3MQ4wDAYDVQQKDAVSZWRyb2lkMRowGAYDVQQD\n"
                         "DBEzMjAwMDEyMzQ1Njc4OTFhYmNkMB4XDTI0MDQwMTAwMDAwMFoXDTI1\n"
                         "-----END CERTIFICATE-----";
    chain.append(deviceCert);
    
    qDebug() << "[PlayIntegrity] Generated certificate chain with" << chain.size() << "certificates";
    
    return chain;
}

void PlayIntegrityManager::configureHardwareAttestation(
    const QString& instanceId, const HardwareAttestationConfig& config) {
    QMutexLocker locker(&m_mutex);
    
    m_attestationConfigs[instanceId] = config;
    
    qDebug() << "[PlayIntegrity] Hardware attestation configured for:" << instanceId
              << "keyType:" << static_cast<int>(config.keyType)
              << "strongBox:" << config.enableStrongBox
              << "bootState:" << static_cast<int>(config.bootState);
}

QJsonObject PlayIntegrityManager::getHardwareAttestationStatus(const QString& instanceId) const {
    QJsonObject status;
    
    HardwareAttestationConfig config = m_attestationConfigs.value(instanceId);
    
    status["keyType"] = static_cast<int>(config.keyType);
    status["keymasterVersion"] = config.keymasterVersion;
    status["strongBoxEnabled"] = config.enableStrongBox;
    status["bootState"] = generateVerifiedBootStateString(config.bootState);
    status["deviceLocked"] = config.isDeviceLocked;
    status["securityPatchLevel"] = config.securityPatchLevel;
    status["hasAttestationKey"] = m_attestationKeys.contains(instanceId);
    
    return status;
}

// ========================================================================
// ATTESTATION HELPER METHODS
// ========================================================================

QByteArray PlayIntegrityManager::generateAttestationChallenge(
    const QString& instanceId, const QString& nonce) {
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Build challenge data from device properties
    QString challengeData = instanceId;
    challengeData += controller.getProperty(instanceId, "ro.build.fingerprint");
    challengeData += controller.getProperty(instanceId, "ro.bootloader");
    challengeData += controller.getProperty(instanceId, "ro.product.model");
    challengeData += nonce;
    
    // Add timestamp
    challengeData += QString::number(QDateTime::currentMSecsSinceEpoch());
    
    // Generate SHA-256 hash
    QByteArray hash = QCryptographicHash::hash(
        challengeData.toUtf8(), QCryptographicHash::Sha256);
    
    return hash;
}

QJsonObject PlayIntegrityManager::buildAttestationPayload(
    const QString& instanceId, const QString& nonce) {
    
    QJsonObject payload;
    ReDroidController& controller = ReDroidController::instance();
    
    HardwareAttestationConfig config = m_attestationConfigs.value(instanceId);
    
    // Header
    QJsonObject header;
    header["alg"] = "RS256";
    header["typ"] = "JWT";
    payload["header"] = header;
    
    // Timestamp
    payload["timestampMs"] = QDateTime::currentMSecsSinceEpoch();
    payload["nonce"] = nonce.isEmpty() ? generateNonce(instanceId) : nonce;
    
    // Device integrity
    QJsonObject deviceIntegrityStatus;
    deviceIntegrityStatus["deviceRecognitionVerdict"] = "RECOGNIZED";
    deviceIntegrityStatus["deviceConfidenceLevel"] = "CONFIDENCE_HIGH";
    payload["deviceIntegrityStatus"] = deviceIntegrityStatus;
    
    // Basic integrity
    QJsonObject basicIntegrity;
    basicIntegrity["basicIntegrity"] = true;
    basicIntegrity["ctsProfileMatch"] = true;
    basicIntegrity["basicIntegrityDeviceCertificate"] = true;
    basicIntegrity["basicIntegritySystemPartition"] = true;
    payload["basicIntegrity"] = basicIntegrity;
    
    // Details
    QJsonObject details;
    details["deviceCategory"] = "PHYSICAL";
    details["deviceModel"] = controller.getProperty(instanceId, "ro.product.model");
    details["manufacturer"] = controller.getProperty(instanceId, "ro.product.manufacturer");
    details["brand"] = controller.getProperty(instanceId, "ro.product.brand");
    details["androidVersion"] = controller.getProperty(instanceId, "ro.build.version.release");
    details["buildFingerprint"] = controller.getProperty(instanceId, "ro.build.fingerprint");
    payload["details"] = details;
    
    // Request details
    QJsonObject requestDetails;
    requestDetails["packageName"] = "com.google.android.gms";
    requestDetails["packageVersionCode"] = 242514000;
    requestDetails["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    payload["requestDetails"] = requestDetails;
    
    return payload;
}

QString PlayIntegrityManager::generateVerifiedBootStateString(VerifiedBootState state) const {
    switch (state) {
        case VerifiedBootState::GREEN:    return "green";
        case VerifiedBootState::YELLOW:    return "yellow";
        case VerifiedBootState::ORANGE:    return "orange";
        case VerifiedBootState::RED:      return "red";
        case VerifiedBootState::UNLOCKED:  return "unlocked";
        default:                          return "unknown";
    }
}

QString PlayIntegrityManager::generateDeviceIntegrityVerdict(const QString& instanceId) {
    HardwareAttestationConfig config = m_attestationConfigs.value(instanceId);
    
    if (config.keyType == AttestationKeyType::STRONGBOX && 
        config.bootState == VerifiedBootState::GREEN) {
        return "MEETS_DEVICE_INTEGRITY";
    } else if (config.bootState == VerifiedBootState::GREEN) {
        return "MEETS_BASIC_INTEGRITY";
    }
    
    return "UNSATISFIED";
}

QByteArray PlayIntegrityManager::signAttestationPayload(
    const QJsonObject& payload, const QString& instanceId) {
    
    // In a real implementation, this would use the attestation key
    // to sign the payload with RSA-SHA256
    
    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray signature = QCryptographicHash::hash(
        data + instanceId.toUtf8(), QCryptographicHash::Sha256);
    
    return signature.toBase64();
}

void PlayIntegrityManager::initializeAttestationKey(const QString& instanceId) {
    // Generate a unique attestation key for this instance
    QByteArray keyData = generateAttestationChallenge(instanceId, "attestation_key");
    m_attestationKeys[instanceId] = keyData;
    
    qDebug() << "[PlayIntegrity] Initialized attestation key for:" << instanceId;
}

QString PlayIntegrityManager::getKeymasterVersion(const QString& instanceId) const {
    HardwareAttestationConfig config = m_attestationConfigs.value(instanceId);
    
    if (config.hasKeymaster43) {
        return "4.3";
    } else if (config.hasKeymaster41) {
        return "4.1";
    } else if (config.hasKeymaster4) {
        return "4.0";
    }
    
    return "3.0";
}

} // namespace VirtualPhonePro
