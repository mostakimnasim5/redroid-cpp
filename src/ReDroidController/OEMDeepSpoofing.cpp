/**
 * @file OEMDeepSpoofing.cpp
 * @brief OEM-Specific Deep Spoofing Implementation
 */

#include "VirtualPhonePro/OEMDeepSpoofing.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>

namespace VirtualPhonePro {

OEMDeepSpoofing* OEMDeepSpoofing::s_instance = nullptr;

OEMDeepSpoofing& OEMDeepSpoofing::instance() {
    if (!s_instance) {
        s_instance = new OEMDeepSpoofing();
    }
    return *s_instance;
}

OEMDeepSpoofing::OEMDeepSpoofing() {
}

// ============================================================================
// Configuration
// ============================================================================

bool OEMDeepSpoofing::configureForOEM(const QString& instanceId, OEMType type) {
    OEMState state = getDefaultsForType(type);
    state.type = type;
    m_oemStates[instanceId] = state;
    
    qDebug() << "Configured OEM for instance:" << instanceId 
             << "- Type:" << oemTypeToString(type);
    
    return applyToInstance(instanceId);
}

bool OEMDeepSpoofing::configureSamsung(const QString& instanceId) {
    return configureForOEM(instanceId, OEMType::SAMSUNG);
}

bool OEMDeepSpoofing::configureGoogle(const QString& instanceId) {
    return configureForOEM(instanceId, OEMType::GOOGLE);
}

bool OEMDeepSpoofing::configureXiaomi(const QString& instanceId) {
    return configureForOEM(instanceId, OEMType::XIAOMI);
}

bool OEMDeepSpoofing::configureHuawei(const QString& instanceId) {
    return configureForOEM(instanceId, OEMType::HUAWEI);
}

bool OEMDeepSpoofing::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    OEMState& state = m_oemStates[instanceId];
    
    QStringList commands;
    
    // Samsung Knox
    if (state.type == OEMType::SAMSUNG || state.samsung.isKnoxEnabled) {
        commands
        << QString("setprop ro.warranty_bit %1").arg(
                state.samsung.isSecurityPolicyEnforced ? "0" : "0")
        << QString("setprop ro.build.selinux %1").arg(
                state.samsung.isSecurityPolicyEnforced ? "Enforcing" : "Enforcing")
        << "setprop ro.com.google.clientidbase.vs android-samsung"
        << "setprop ro.product.first_api_level 34"
        << "setprop ro.build.version.oneui 6.0"
        << "setprop ro.device.knox " + QString(state.samsung.isKnoxEnabled ? "1" : "0")
        << "setprop ro.spay.enabled " + QString(state.samsung.isSamsungPaySupported ? "true" : "false");
    }
    
    // Huawei HMS
    if (state.type == OEMType::HUAWEI || state.huawei.isHMSSupported) {
        commands
        << "setprop ro.huawei.build.display.id " + state.huawei.hmsVersion
        << "setprop ro.build.huawei.version " + state.huawei.hmsVersion
        << "setprop ro.config.hw_voice_assistant com.huawei.vassistant"
        << "setprop ro.compact.system.font true";
    }
    
    // Xiaomi MIUI
    if (state.type == OEMType::XIAOMI || state.xiaomi.isMIUISupported) {
        commands
        << "setprop ro.miui.ui.version.code " + state.xiaomi.miuiBuildVersion
        << "setprop ro.miui.build.version.incremental " + state.xiaomi.miuiBuildVersion
        << "setprop ro.miui.build.version.sdk 34"
        << "setprop ro.com.google.clientidbase.ms android-xiaomi";
    }
    
    // Google Services (always for most OEMs)
    if (state.gms.isGMSInstalled) {
        commands
        << "setprop ro.gms.client.channel 3"
        << "setprop ro.com.google.clientidbase.am android-google"
        << "setprop ro.com.google.clientidbase.gmm android-google"
        << "setprop ro.com.google.clientidbase.yt android-google"
        << "setprop ro.google.devicecert.path /data/gservices/google-certified";
    }
    
    // Universal OEM properties
    commands
        << QString("setprop ro.product.manufacturer.origin %1").arg(state.oemName)
        << "setprop persist.radio.country " + state.gms.deviceCountryCode;
    
    // Execute all commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "OEM configuration applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Samsung Knox
// ============================================================================

bool OEMDeepSpoofing::enableKnox(const QString& instanceId) {
    OEMState& state = m_oemStates[instanceId];
    
    state.samsung.isKnoxEnabled = true;
    state.samsung.isKnoxSupported = true;
    state.samsung.isKnoxActive = true;
    state.samsung.knoxVersion = 3;
    state.samsung.isSecurityPolicyEnforced = true;
    state.samsung.isSamsungPaySupported = true;
    state.samsung.isSamsungPassSupported = true;
    state.samsung.knoxLicenseStatus = "ACTIVE";
    
    // Generate Knox ID
    QRandomGenerator* gen = QRandomGenerator::global();
    QString knoxId;
    for (int i = 0; i < 16; i++) {
        knoxId += QString::number(gen->bounded(10));
    }
    state.samsung.knoxId = knoxId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        "setprop ro.boot.warranty_bit 0",
        "setprop ro.warranty_bit 0",
        "setprop ro.build.selinux Enforcing",
        "setprop ro.device.knox 1",
        "setprop ro.device.knox.version 3",
        "setprop ro.device.owner 0",
        "setprop ro.knox.version 3",
        "setprop ro.knox.privacy_policy 1",
        "setprop ro.samsung.knox.edm true",
        "setprop ro.spkp.tegra.kl true",
        "setprop ro.security.vkp true",
        "setprop ro.se.rim.kes true",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool OEMDeepSpoofing::disableKnox(const QString& instanceId) {
    OEMState& state = m_oemStates[instanceId];
    state.samsung.isKnoxEnabled = false;
    state.samsung.isKnoxActive = false;
    
    return applyToInstance(instanceId);
}

bool OEMDeepSpoofing::setKnoxContainer(const QString& instanceId, const QString& containerId) {
    OEMState& state = m_oemStates[instanceId];
    state.samsung.knoxContainerId = containerId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId,
        "setprop ro.knox.container.id " + containerId);
    
    return true;
}

// ============================================================================
// Huawei HMS
// ============================================================================

bool OEMDeepSpoofing::enableHMS(const QString& instanceId) {
    OEMState& state = m_oemStates[instanceId];
    
    state.huawei.isHMSSupported = true;
    state.huawei.isHMSCoreInstalled = true;
    state.huawei.isAppGalleryAvailable = true;
    state.huawei.isHMSEnabled = true;
    state.huawei.hmsVersion = "6.12.0.300";
    state.huawei.isSafetyDetAvailable = true;
    state.huawei.isIntegrityAvailable = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        "setprop ro.huawei.hw_fastboot true",
        "setprop ro.build.version.emui 13",
        "setprop ro.build.version.hwaivi 13.0",
        "setprop ro.huawei.market.version 13.0.0.300",
        "setprop ro.huawei.hw_experience.version 13.0",
        "setprop ro.com.huawei.android.launcherstring com.huawei.android.launcher",
        "setprop ro.config.hw_voice_assistant com.huawei.vassistant",
        "setprop ro.huawei.appgallery.version 13.4.0",
        "setprop ro.huawei.hwid.version 6.12.0",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool OEMDeepSpoofing::setHMSIntegrity(const QString& instanceId, bool valid) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (valid) {
        ctrl.executeShell(instanceId, "setprop ro.huawei.hw_integrity true");
        ctrl.executeShell(instanceId, "setprop ro.huawei.devicecert true");
        ctrl.executeShell(instanceId, "setprop ro.huawei.safety_det true");
    } else {
        ctrl.executeShell(instanceId, "setprop ro.huawei.hw_integrity false");
    }
    
    return true;
}

// ============================================================================
// Xiaomi MIUI
// ============================================================================

bool OEMDeepSpoofing::enableMIUI(const QString& instanceId) {
    OEMState& state = m_oemStates[instanceId];
    
    state.xiaomi.isMIUISupported = true;
    state.xiaomi.isMIUIEnhanced = true;
    state.xiaomi.miuiVersion = "V14.0";
    state.xiaomi.miuiBuildVersion = "14.0.24";
    state.xiaomi.isMiPaySupported = true;
    state.xiaomi.isGameTurboEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        "setprop ro.miui.ui.version.name V14",
        "setprop ro.miui.ui.version.code 14",
        "setprop ro.miui.build.version.incremental 14.0.24.0",
        "setprop ro.miui.build.version.sdk 34",
        "setprop ro.miui.build.version.alter.release V14.0",
        "setprop ro.miui.customMIUIVersion true",
        "setprop ro.miui.internal.storage.optimized true",
        "setprop ro.mipay true",
        "setprop ro.gameturbo.enabled true",
        "setprop ro.game.auto.performance true",
        "setprop ro.ayawservice.enabled true",
        "setprop ro.com.google.clientidbase.ms android-xiaomi",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool OEMDeepSpoofing::enableGameTurbo(const QString& instanceId, bool enabled) {
    OEMState& state = m_oemStates[instanceId];
    state.xiaomi.isGameTurboEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (enabled) {
        ctrl.executeShell(instanceId, "setprop ro.gameturbo.enabled true");
        ctrl.executeShell(instanceId, "setprop ro.game.auto.performance true");
        ctrl.executeShell(instanceId, "setprop ro.game.mode.enabled true");
    } else {
        ctrl.executeShell(instanceId, "setprop ro.gameturbo.enabled false");
    }
    
    return true;
}

// ============================================================================
// Google Services
// ============================================================================

bool OEMDeepSpoofing::enableFullGMS(const QString& instanceId) {
    OEMState& state = m_oemStates[instanceId];
    
    state.gms.isGMSInstalled = true;
    state.gms.isPlayStoreInstalled = true;
    state.gms.isGooglePlayServicesInstalled = true;
    state.gms.isSafetyNetAvailable = true;
    state.gms.isPlayIntegrityAvailable = true;
    state.gms.isDeviceCertificationInstalled = true;
    state.gms.isGoogleBackupSupported = true;
    state.gms.gmsVersion = "23.06.034";
    state.gms.playServicesVersion = "23.22.18";
    state.gms.playStoreVersion = "35.6.18";
    state.gms.playServicesVersionCode = "235612234";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        // GMS
        "setprop ro.com.google.gmsversion 23.06.034",
        "setprop ro.gms.client.channel 3",
        "setprop ro.google.gmsversion pkg 23.06.034",
        
        // Play Services
        "setprop ro.com.google.clientidbase.gms android-google",
        "setprop ro.com.google.clientidbase.am android-google",
        "setprop ro.com.google.clientidbase.gmm android-google",
        "setprop ro.com.google.clientidbase.yt android-google",
        "setprop ro.com.google.clientidbase.vs android-google",
        
        // SafetyNet
        "setprop ro.gms.safetynet.enabled true",
        "setprop ro.gms.play_integrity.enabled true",
        
        // Device Certification
        "setprop ro.com.google.devicecert true",
        "setprop ro.google.devicecert.path /data/gservices/google-certified",
        "setprop ro.google.gms.db DC=AFgBITEAAAA2XbsxZ0l3pVCw==",
        
        // Google Play Store
        "setprop ro.appsflyer.preinstall.path /system/etc/gms-precious",
        "setprop ro.setupwizard.mode OPTIONAL",
        "setprop ro.com.google.android.carrier false",
        
        // Backup
        "setprop ro.google.backupapi.enabled true",
        "setprop ro.backup.service.enabled true",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool OEMDeepSpoofing::setDeviceCertification(const QString& instanceId, bool certified) {
    OEMState& state = m_oemStates[instanceId];
    state.gms.isDeviceCertificationInstalled = certified;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (certified) {
        ctrl.executeShell(instanceId, "setprop ro.com.google.devicecert true");
        ctrl.executeShell(instanceId, "setprop ro.google.devicecert.status OK");
    } else {
        ctrl.executeShell(instanceId, "setprop ro.com.google.devicecert false");
    }
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

OEMState OEMDeepSpoofing::getOEMState(const QString& instanceId) const {
    if (m_oemStates.contains(instanceId)) {
        return m_oemStates[instanceId];
    }
    
    return getDefaultsForType(OEMType::GENERIC);
}

QMap<QString, QString> OEMDeepSpoofing::getAllOEMProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    OEMState state = getOEMState(instanceId);
    
    // Samsung
    props["samsung.knox.enabled"] = state.samsung.isKnoxEnabled ? "true" : "false";
    props["samsung.knox.active"] = state.samsung.isKnoxActive ? "true" : "false";
    props["samsung.knox.version"] = QString::number(state.samsung.knoxVersion);
    props["samsung.pay.supported"] = state.samsung.isSamsungPaySupported ? "true" : "false";
    
    // Huawei
    props["huawei.hms.supported"] = state.huawei.isHMSSupported ? "true" : "false";
    props["huawei.hms.installed"] = state.huawei.isHMSCoreInstalled ? "true" : "false";
    props["huawei.hms.version"] = state.huawei.hmsVersion;
    
    // Xiaomi
    props["xiaomi.miui.enabled"] = state.xiaomi.isMIUISupported ? "true" : "false";
    props["xiaomi.miui.version"] = state.xiaomi.miuiVersion;
    props["xiaomi.mipay.supported"] = state.xiaomi.isMiPaySupported ? "true" : "false";
    props["xiaomi.gameturbo.enabled"] = state.xiaomi.isGameTurboEnabled ? "true" : "false";
    
    // Google
    props["google.gms.installed"] = state.gms.isGMSInstalled ? "true" : "false";
    props["google.playstore.installed"] = state.gms.isPlayStoreInstalled ? "true" : "false";
    props["google.gms.version"] = state.gms.gmsVersion;
    props["google.playstore.version"] = state.gms.playStoreVersion;
    props["google.devicecert"] = state.gms.isDeviceCertificationInstalled ? "true" : "false";
    
    return props;
}

bool OEMDeepSpoofing::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

bool OEMDeepSpoofing::resetOEM(const QString& instanceId) {
    OEMState defaultState = getDefaultsForType(OEMType::SAMSUNG);
    m_oemStates[instanceId] = defaultState;
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

OEMState OEMDeepSpoofing::getDefaultsForType(OEMType type) const {
    OEMState state;
    state.type = type;
    state.allOEMFeaturesEnabled = true;
    
    switch (type) {
        case OEMType::SAMSUNG:
            state.oemId = "samsung";
            state.oemName = "samsung";
            state.oemBrand = "samsung";
            state.oemModel = "SM-S928B";
            
            state.samsung.isKnoxEnabled = true;
            state.samsung.isKnoxSupported = true;
            state.samsung.isKnoxActive = true;
            state.samsung.knoxVersion = 3;
            state.samsung.isSecurityPolicyEnforced = true;
            state.samsung.isSamsungPaySupported = true;
            state.samsung.isSamsungPassSupported = true;
            state.samsung.knoxLicenseStatus = "ACTIVE";
            
            state.gms.isGMSInstalled = true;
            state.gms.gmsVersion = "23.06.034";
            state.gms.carrierId = "samsung";
            break;
            
        case OEMType::GOOGLE:
            state.oemId = "google";
            state.oemName = "google";
            state.oemBrand = "google";
            state.oemModel = "Pixel 8 Pro";
            
            state.gms.isGMSInstalled = true;
            state.gms.isPlayStoreInstalled = true;
            state.gms.isGooglePlayServicesInstalled = true;
            state.gms.gmsVersion = "23.06.034";
            state.gms.carrierId = "google";
            break;
            
        case OEMType::XIAOMI:
            state.oemId = "xiaomi";
            state.oemName = "xiaomi";
            state.oemBrand = "xiaomi";
            state.oemModel = "Mi 14";
            
            state.xiaomi.isMIUISupported = true;
            state.xiaomi.isMIUIEnhanced = true;
            state.xiaomi.miuiVersion = "V14.0";
            state.xiaomi.miuiBuildVersion = "14.0.24";
            state.xiaomi.isMiPaySupported = true;
            state.xiaomi.isGameTurboEnabled = true;
            
            state.gms.isGMSInstalled = true;
            state.gms.gmsVersion = "23.06.034";
            state.gms.carrierId = "xiaomi";
            break;
            
        case OEMType::HUAWEI:
            state.oemId = "huawei";
            state.oemName = "huawei";
            state.oemBrand = "huawei";
            state.oemModel = "P60 Pro";
            
            state.huawei.isHMSSupported = true;
            state.huawei.isHMSCoreInstalled = true;
            state.huawei.isAppGalleryAvailable = true;
            state.huawei.isHMSEnabled = true;
            state.huawei.hmsVersion = "6.12.0.300";
            state.huawei.isSafetyDetAvailable = true;
            state.huawei.isIntegrityAvailable = true;
            break;
            
        default:
            state.oemId = "generic";
            state.oemName = "generic";
            state.oemBrand = "generic";
            state.oemModel = "Android Device";
            
            state.gms.isGMSInstalled = true;
            state.gms.gmsVersion = "23.06.034";
            state.gms.carrierId = "default";
            break;
    }
    
    return state;
}

QString OEMDeepSpoofing::oemTypeToString(OEMType type) const {
    switch (type) {
        case OEMType::SAMSUNG: return "Samsung";
        case OEMType::GOOGLE: return "Google";
        case OEMType::XIAOMI: return "Xiaomi";
        case OEMType::HUAWEI: return "Huawei";
        case OEMType::OPPO: return "OPPO";
        case OEMType::VIVO: return "Vivo";
        case OEMType::ONEPLUS: return "OnePlus";
        case OEMType::REALME: return "Realme";
        default: return "Generic";
    }
}

} // namespace VirtualPhonePro
