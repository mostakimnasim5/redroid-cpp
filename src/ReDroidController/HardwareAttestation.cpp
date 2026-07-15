/**
 * @file HardwareAttestation.cpp
 * @brief Hardware Attestation & Keystore Implementation
 */

#include "VirtualPhonePro/HardwareAttestation.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>

namespace VirtualPhonePro {

HardwareAttestation* HardwareAttestation::s_instance = nullptr;

HardwareAttestation& HardwareAttestation::instance() {
    if (!s_instance) {
        s_instance = new HardwareAttestation();
    }
    return *s_instance;
}

HardwareAttestation::HardwareAttestation() {
}

// ============================================================================
// Configuration
// ============================================================================

bool HardwareAttestation::configureForDevice(const QString& manufacturer, const QString& model) {
    qDebug() << "Configuring hardware attestation for:" << manufacturer << model;
    
    HardwareSecurityState defaults = getDeviceDefaults(manufacturer, model);
    
    // Apply defaults to all new instances
    return true;
}

bool HardwareAttestation::setSecurityState(const QString& instanceId, const HardwareSecurityState& state) {
    HardwareSecurityState newState = state;
    m_securityStates[instanceId] = newState;
    
    qDebug() << "Security state set for instance:" << instanceId;
    return applyToInstance(instanceId);
}

HardwareSecurityState HardwareAttestation::getSecurityState(const QString& instanceId) const {
    if (m_securityStates.contains(instanceId)) {
        return m_securityStates[instanceId];
    }
    
    // Return default secure state
    HardwareSecurityState defaultState;
    defaultState.keymasterVersion = KeymasterVersion::KM_4_1;
    defaultState.isStrongBox = true;
    defaultState.isTEEPresent = true;
    defaultState.isSEPresent = true;
    defaultState.verifiedBootState = VerifiedBootState::VERIFIED;
    defaultState.bootloaderState = BootloaderState::LOCKED;
    defaultState.verifiedBootKey = generateVerifiedBootKey();
    defaultState.verifiedBootHash = "";
    defaultState.verifiedBootStateString = "green";
    defaultState.isHardwareAttestationSupported = true;
    defaultState.isDeviceLockEnabled = true;
    defaultState.isSecureHardwarePresent = true;
    defaultState.isEncryptionEnabled = true;
    defaultState.isEncryptionSupported = true;
    defaultState.drmLevel = DRMLevel::L1;
    defaultState.hdcpLevel = 2;
    defaultState.isHDCPCompliant = true;
    defaultState.teeVendor = "QSEE";
    defaultState.teeVersion = "4.1";
    defaultState.teePatchLevel = "2024-01";
    defaultState.socManufacturer = "Qualcomm";
    defaultState.socModel = "Snapdragon 8 Gen 3";
    defaultState.hardwareVendor = "Samsung";
    
    return defaultState;
}

bool HardwareAttestation::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    HardwareSecurityState state = getSecurityState(instanceId);
    
    QStringList commands = {
        // Keymaster version
        QString("setprop ro.keymaster.version %1").arg(
            static_cast<int>(state.keymasterVersion)),
        QString("setprop ro.hardware.keystore %1").arg(
            state.isStrongBox ? "strongbox" : "tee"),
        
        // StrongBox
        QString("setprop ro.hardware.strongbox_keystore %1").arg(
            state.isStrongBox ? "true" : "false"),
        QString("setprop ro.security.strongbox %1").arg(
            state.isStrongBox ? "true" : "false"),
        
        // Verified Boot
        QString("setprop ro.boot.verifiedbootstate %1").arg(
            verifiedBootStateToString(state.verifiedBootState)),
        QString("setprop ro.boot.veritymode enforcing").arg(),
        QString("setprop ro.verifiedbootstate %1").arg(
            verifiedBootStateToString(state.verifiedBootState)),
        
        // Bootloader
        QString("setprop ro.boot.flash.locked %1").arg(
            state.bootloaderState == BootloaderState::LOCKED ? "1" : "0"),
        QString("setprop ro.bootloader.locked %1").arg(
            state.bootloaderState == BootloaderState::LOCKED ? "true" : "false"),
        
        // Device integrity
        "setprop ro.secure 1",
        "setprop ro.adb.secure 1",
        "setprop ro.build.selinux Enforcing",
        
        // Hardware attestation
        QString("setprop ro.hardware.attestation %1").arg(
            state.isHardwareAttestationSupported ? "true" : "false"),
        QString("setprop ro.hardware.device_lock %1").arg(
            state.isDeviceLockEnabled ? "true" : "false"),
        
        // Secure hardware
        QString("setprop ro.hardware.secure_element %1").arg(
            state.isSEPresent ? "true" : "false"),
        QString("setprop ro.hardware.tee %1").arg(
            state.isTEEPresent ? "true" : "false"),
        
        // TEE Info
        QString("setprop ro.hardware.tee.vendor %1").arg(state.teeVendor),
        QString("setprop ro.hardware.tee.version %1").arg(state.teeVersion),
        QString("setprop ro.hardware.tee.patch %1").arg(state.teePatchLevel),
        
        // Encryption
        "setprop ro.crypto.state encrypted",
        "setprop ro.crypto.triple_des_evt_log 1",
        
        // DRM
        QString("setprop ro.drm.level %1").arg(
            drmLevelToString(state.drmLevel)),
        QString("setprop ro.widevine.level %1").arg(
            state.drmLevel == DRMLevel::L1 ? "L1" : "L3"),
        QString("persist.drm.cdmi %1").arg(
            state.drmLevel == DRMLevel::L1 ? "L1" : "L3"),
        
        // HDCP
        QString("persist.drm.hdcp.level %1").arg(state.hdcpLevel),
        "setprop drm.hdcp.enable true",
        
        // SoC/Chipset
        QString("setprop ro.soc.manufacturer %1").arg(state.socManufacturer),
        QString("setprop ro.soc.model %1").arg(state.socModel),
        QString("setprop ro.hardware.soc %1").arg(state.socModel),
        
        // Additional security props
        "setprop ro.kernel.qemu 0",
        "setprop ro.boot.qemu false",
        "setprop sys.boot_completed 1",
        "setprop ro.build.tags release-keys",
        "setprop ro.build.type user",
        
        // Verified boot hash
        QString("setprop ro.boot.verity.hash %1").arg(state.verifiedBootHash),
        QString("setprop ro.verity.mode enforcing").arg(),
        
        //dm-verity
        "setprop ro.config.dmverity true",
        "setprop ro.config.dmverity_enforcing true",
        
        // Keymaster props for banking apps
        "setprop ro.crypto.keymaster.options_supported true",
        "setprop ro.crypto.boot_verification_key ro.verity.key",
    };
    
    // Execute all commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Set keymaster HIDL service
    if (state.isStrongBox) {
        ctrl.executeShell(instanceId, 
            "setprop ro.hardware.keystore impl strongbox");
    } else {
        ctrl.executeShell(instanceId, 
            "setprop ro.hardware.keystore impl tee");
    }
    
    qDebug() << "Hardware attestation applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Keymaster Operations
// ============================================================================

bool HardwareAttestation::enableStrongBox(const QString& instanceId) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.isStrongBox = true;
    state.keymasterVersion = KeymasterVersion::KM_STRONGBOX;
    state.verifiedBootState = VerifiedBootState::VERIFIED;
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::enableTEEKeymaster(const QString& instanceId) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.isStrongBox = false;
    state.isTEEPresent = true;
    state.keymasterVersion = KeymasterVersion::KM_4_1;
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::setKeymasterVersion(const QString& instanceId, KeymasterVersion version) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.keymasterVersion = version;
    
    if (version == KeymasterVersion::KM_STRONGBOX) {
        state.isStrongBox = true;
    }
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::generateAttestationKey(const QString& instanceId, AttestationKeyType type) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Generate attestation key using keymaster
    QString keyType;
    switch (type) {
        case AttestationKeyType::ATTESTATION:
            keyType = "attestation";
            break;
        case AttestationKeyType::RSA_2048:
            keyType = "rsa:2048";
            break;
        case AttestationKeyType::RSA_3072:
            keyType = "rsa:3072";
            break;
        case AttestationKeyType::EC_P256:
            keyType = "ec:prime256v1";
            break;
        case AttestationKeyType::EC_P384:
            keyType = "ec:secp384r1";
            break;
    }
    
    // Generate key via keytool or keystore
    QString cmd = QString("keymaster_generate_key --key-size 2048 --algorithm RSA");
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

// ============================================================================
// Verified Boot
// ============================================================================

bool HardwareAttestation::setVerifiedBootState(const QString& instanceId, VerifiedBootState state) {
    HardwareSecurityState& securityState = m_securityStates[instanceId];
    securityState.verifiedBootState = state;
    securityState.verifiedBootStateString = verifiedBootStateToString(state);
    securityState.bootVerifiedTime = QDateTime::currentMSecsSinceEpoch();
    
    return setSecurityState(instanceId, securityState);
}

bool HardwareAttestation::setBootloaderState(const QString& instanceId, BootloaderState state) {
    HardwareSecurityState& securityState = m_securityStates[instanceId];
    securityState.bootloaderState = state;
    
    // Update verified boot state based on bootloader
    if (state == BootloaderState::UNLOCKED) {
        securityState.verifiedBootState = VerifiedBootState::UNLOCKED;
        securityState.verifiedBootStateString = "orange";
    }
    
    return setSecurityState(instanceId, securityState);
}

bool HardwareAttestation::lockBootloader(const QString& instanceId) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.bootloaderState = BootloaderState::LOCKED;
    state.verifiedBootState = VerifiedBootState::VERIFIED;
    state.verifiedBootStateString = "green";
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::unlockBootloader(const QString& instanceId) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.bootloaderState = BootloaderState::UNLOCKED;
    state.verifiedBootState = VerifiedBootState::UNLOCKED;
    state.verifiedBootStateString = "orange";
    
    return setSecurityState(instanceId, state);
}

// ============================================================================
// DRM & Widevine
// ============================================================================

bool HardwareAttestation::setWidevineLevel(const QString& instanceId, DRMLevel level) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.drmLevel = level;
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::setHDCPLevel(const QString& instanceId, int level) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.hdcpLevel = level;
    state.isHDCPCompliant = (level >= 1);
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::installWidevineKeybox(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set Widevine keybox property
    QString keybox = generateWidevineKeybox();
    ctrl.executeShell(instanceId, "setprop ro.widevine.keybox '" + keybox + "'");
    
    // Set Widevine properties
    QStringList cmds = {
        "setprop ro.drm.enabled true",
        "setprop drm.service.enabled true",
        "setprop ro.hardware.drm widevine",
        "setprop ro.com.google.widevine.license.level L1",
        "setprop persist.sys.widevine.level L1",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

// ============================================================================
// TEE Configuration
// ============================================================================

bool HardwareAttestation::configureTEE(const QString& instanceId, const QString& vendor, const QString& version) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.teeVendor = vendor;
    state.teeVersion = version;
    state.isTEEPresent = true;
    
    return setSecurityState(instanceId, state);
}

bool HardwareAttestation::setSecureElementPresent(const QString& instanceId, bool present) {
    HardwareSecurityState state = getSecurityState(instanceId);
    state.isSEPresent = present;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, 
        QString("setprop ro.hardware.secure_element %1").arg(present ? "true" : "false"));
    
    return setSecurityState(instanceId, state);
}

// ============================================================================
// Utility
// ============================================================================

QMap<QString, QString> HardwareAttestation::getAllSecurityProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    HardwareSecurityState state = getSecurityState(instanceId);
    
    // Keymaster
    props["keymaster.version"] = QString::number(static_cast<int>(state.keymasterVersion));
    props["keymaster.strongbox"] = state.isStrongBox ? "true" : "false";
    props["keymaster.tee"] = state.isTEEPresent ? "true" : "false";
    
    // Verified Boot
    props["verifiedboot.state"] = verifiedBootStateToString(state.verifiedBootState);
    props["bootloader.locked"] = state.bootloaderState == BootloaderState::LOCKED ? "true" : "false";
    props["verifiedboot.hash"] = state.verifiedBootHash;
    
    // Security
    props["hardware.attestation"] = state.isHardwareAttestationSupported ? "true" : "false";
    props["hardware.secure_element"] = state.isSEPresent ? "true" : "false";
    props["hardware.tee"] = state.isTEEPresent ? "true" : "false";
    
    // TEE
    props["tee.vendor"] = state.teeVendor;
    props["tee.version"] = state.teeVersion;
    props["tee.patch_level"] = state.teePatchLevel;
    
    // DRM
    props["widevine.level"] = state.drmLevel == DRMLevel::L1 ? "L1" : "L3";
    props["drm.level"] = drmLevelToString(state.drmLevel);
    props["hdcp.level"] = QString::number(state.hdcpLevel);
    
    // Encryption
    props["encryption.enabled"] = state.isEncryptionEnabled ? "true" : "false";
    props["crypto.state"] = "encrypted";
    
    // SoC
    props["soc.manufacturer"] = state.socManufacturer;
    props["soc.model"] = state.socModel;
    
    // Device lock
    props["device.lock"] = state.isDeviceLockEnabled ? "true" : "false";
    
    return props;
}

bool HardwareAttestation::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

QJsonObject HardwareAttestation::verifyAttestation(const QString& instanceId) {
    QJsonObject result;
    HardwareSecurityState state = getSecurityState(instanceId);
    
    // Check all attestation requirements
    result["verified_boot_state"] = verifiedBootStateToString(state.verifiedBootState);
    result["bootloader_locked"] = state.bootloaderState == BootloaderState::LOCKED;
    result["keymaster_version"] = static_cast<int>(state.keymasterVersion);
    result["strongbox_enabled"] = state.isStrongBox;
    result["tee_present"] = state.isTEEPresent;
    result["secure_element_present"] = state.isSEPresent;
    result["widevine_level"] = state.drmLevel == DRMLevel::L1 ? "L1" : "L3";
    result["hdcp_compliant"] = state.isHDCPCompliant;
    result["encryption_enabled"] = state.isEncryptionEnabled;
    
    // Overall pass/fail
    bool pass = true;
    pass &= (state.verifiedBootState == VerifiedBootState::VERIFIED);
    pass &= (state.bootloaderState == BootloaderState::LOCKED);
    pass &= (state.keymasterVersion >= KeymasterVersion::KM_4_0);
    pass &= (state.drmLevel == DRMLevel::L1);
    
    result["overall_pass"] = pass;
    result["attestation_timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}

bool HardwareAttestation::resetSecurity(const QString& instanceId) {
    HardwareSecurityState defaultState;
    defaultState.keymasterVersion = KeymasterVersion::KM_STRONGBOX;
    defaultState.isStrongBox = true;
    defaultState.isTEEPresent = true;
    defaultState.isSEPresent = true;
    defaultState.verifiedBootState = VerifiedBootState::VERIFIED;
    defaultState.bootloaderState = BootloaderState::LOCKED;
    defaultState.verifiedBootKey = generateVerifiedBootKey();
    defaultState.verifiedBootStateString = "green";
    defaultState.isHardwareAttestationSupported = true;
    defaultState.isDeviceLockEnabled = true;
    defaultState.isSecureHardwarePresent = true;
    defaultState.isEncryptionEnabled = true;
    defaultState.isEncryptionSupported = true;
    defaultState.drmLevel = DRMLevel::L1;
    defaultState.hdcpLevel = 2;
    defaultState.isHDCPCompliant = true;
    defaultState.teeVendor = "QSEE";
    defaultState.teeVersion = "4.1";
    defaultState.teePatchLevel = "2024-01";
    defaultState.socManufacturer = "Qualcomm";
    defaultState.socModel = "Snapdragon 8 Gen 3";
    defaultState.hardwareVendor = "Samsung";
    
    m_securityStates[instanceId] = defaultState;
    
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

QString HardwareAttestation::keymasterVersionToString(KeymasterVersion version) const {
    switch (version) {
        case KeymasterVersion::KM_1_0: return "1.0";
        case KeymasterVersion::KM_2_0: return "2.0";
        case KeymasterVersion::KM_3_0: return "3.0";
        case KeymasterVersion::KM_4_0: return "4.0";
        case KeymasterVersion::KM_4_1: return "4.1";
        case KeymasterVersion::KM_STRONGBOX: return "strongbox";
        default: return "4.1";
    }
}

QString HardwareAttestation::verifiedBootStateToString(VerifiedBootState state) const {
    switch (state) {
        case VerifiedBootState::VERIFIED: return "green";
        case VerifiedBootState::SELF_SIGNED: return "yellow";
        case VerifiedBootState::UNVERIFIED: return "orange";
        case VerifiedBootState::FAILED: return "red";
        case VerifiedBootState::UNLOCKED: return "orange";
        default: return "green";
    }
}

QString HardwareAttestation::drmLevelToString(DRMLevel level) const {
    switch (level) {
        case DRMLevel::L1: return "L1";
        case DRMLevel::L2: return "L2";
        case DRMLevel::L3: return "L3";
        default: return "L1";
    }
}

QString HardwareAttestation::generateVerifiedBootHash(const QString& manufacturer, const QString& model) {
    QString data = manufacturer + model + QString::number(QDateTime::currentMSecsSinceEpoch());
    QByteArray hash = QCryptographicHash::hash(
        data.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex();
}

QString HardwareAttestation::generateVerifiedBootKey() {
    // Generate a random 256-bit key for verified boot
    QRandomGenerator* gen = QRandomGenerator::global();
    QByteArray key;
    for (int i = 0; i < 32; i++) {
        key.append(static_cast<char>(gen->bounded(256)));
    }
    return key.toHex();
}

QString HardwareAttestation::generateWidevineKeybox() {
    // Generate a fake Widevine keybox (in real scenario this would be a proper keybox)
    QRandomGenerator* gen = QRandomGenerator::global();
    QByteArray keybox;
    keybox.append("widevine"); // Magic
    keybox.append(static_cast<char>(0x01)); // Version
    for (int i = 0; i < 52; i++) { // Fill rest with random
        keybox.append(static_cast<char>(gen->bounded(256)));
    }
    return keybox.toBase64();
}

HardwareSecurityState HardwareAttestation::getDeviceDefaults(const QString& manufacturer, const QString& model) {
    HardwareSecurityState state;
    
    if (manufacturer.contains("samsung", Qt::CaseInsensitive)) {
        state.keymasterVersion = KeymasterVersion::KM_STRONGBOX;
        state.isStrongBox = true;
        state.teeVendor = "QSEE";
        state.teeVersion = "4.1";
        state.socManufacturer = "Samsung";
        state.socModel = "Exynos 2400";
        state.hardwareVendor = "Samsung";
        state.drmLevel = DRMLevel::L1;
        state.hdcpLevel = 2;
    } else if (manufacturer.contains("google", Qt::CaseInsensitive) ||
               manufacturer.contains("pixel", Qt::CaseInsensitive)) {
        state.keymasterVersion = KeymasterVersion::KM_STRONGBOX;
        state.isStrongBox = true;
        state.teeVendor = "QSEE";
        state.teeVersion = "4.0";
        state.socManufacturer = "Google";
        state.socModel = "Tensor G3";
        state.hardwareVendor = "Google";
        state.drmLevel = DRMLevel::L1;
        state.hdcpLevel = 2;
    } else if (manufacturer.contains("xiaomi", Qt::CaseInsensitive)) {
        state.keymasterVersion = KeymasterVersion::KM_4_1;
        state.isStrongBox = false;
        state.teeVendor = "QSEE";
        state.teeVersion = "4.1";
        state.socManufacturer = "Qualcomm";
        state.socModel = "Snapdragon 8 Gen 3";
        state.hardwareVendor = "Xiaomi";
        state.drmLevel = DRMLevel::L1;
        state.hdcpLevel = 2;
    } else {
        // Default - secure configuration
        state.keymasterVersion = KeymasterVersion::KM_4_0;
        state.isStrongBox = true;
        state.teeVendor = "QSEE";
        state.teeVersion = "4.0";
        state.socManufacturer = "Qualcomm";
        state.socModel = "Snapdragon 8 Gen 2";
        state.hardwareVendor = manufacturer;
        state.drmLevel = DRMLevel::L1;
        state.hdcpLevel = 2;
    }
    
    state.isTEEPresent = true;
    state.isSEPresent = true;
    state.verifiedBootState = VerifiedBootState::VERIFIED;
    state.bootloaderState = BootloaderState::LOCKED;
    state.verifiedBootKey = generateVerifiedBootKey();
    state.verifiedBootStateString = "green";
    state.isHardwareAttestationSupported = true;
    state.isDeviceLockEnabled = true;
    state.isSecureHardwarePresent = true;
    state.isEncryptionEnabled = true;
    state.isEncryptionSupported = true;
    state.isHDCPCompliant = true;
    state.teePatchLevel = "2024-01";
    
    return state;
}

} // namespace VirtualPhonePro
