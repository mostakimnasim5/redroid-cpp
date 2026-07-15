#include "VirtualPhonePro/SafetyNetSpoofer.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>

namespace VirtualPhonePro {

SafetyNetSpoofer* SafetyNetSpoofer::s_instance = nullptr;

SafetyNetSpoofer& SafetyNetSpoofer::instance() {
    if (!s_instance) {
        s_instance = new SafetyNetSpoofer();
    }
    return *s_instance;
}

bool SafetyNetSpoofer::executeCommand(const QString& instanceId, const QString& command) {
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, command);
    return true;
}

QString SafetyNetSpoofer::executeCommandSync(const QString& instanceId, const QString& command) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.executeShell(instanceId, command);
}

bool SafetyNetSpoofer::pushFile(const QString& instanceId, const QString& local, const QString& remote) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.pushFile(instanceId, local, remote);
}

bool SafetyNetSpoofer::installApk(const QString& instanceId, const QString& apk) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.installApk(instanceId, apk);
}

// ============================================================================
// SafetyNet Attestation
// ============================================================================

bool SafetyNetSpoofer::spoofSafetyNetResponse(const QString& instanceId) {
    qDebug() << "Spoofing SafetyNet response for instance:" << instanceId;
    
    // Step 1: Set basic integrity properties
    QStringList props = {
        // CTS Profile Match
        "ctssdk.prop.name", "1",
        "persist.ctssdk.ctsProfileMatch", "true",
        "ro.build.version.ctssdk", "15",
        "ro.config.ctss", "true",
        
        // Basic Integrity
        "persist.snet.basic_integrity", "true",
        "ro.snet.basic_integrity", "true",
        
        // Device Integrity
        "ro.verity.mode", "enforcing",
        "ro.secure", "1",
        "ro.build.verifiedboot", "true",
        "ro.boot.verifiedbootstate", "green",
        "ro.boot.flash.locked", "1",
        
        // SafetyNet Specific
        "com.google.android.gms.safetynet.ctsProfileMatch", "true",
        "com.google.android.gms.safetynet.basicIntegrity", "true",
        "com.google.android.gms.safetynet.DEVICE_ATTESTATION", "1",
    };
    
    for (int i = 0; i < props.size(); i += 2) {
        executeCommand(instanceId, "setprop " + props[i] + " " + props[i + 1]);
    }
    
    // Step 2: Install spoofed GMS Core
    installSpoofedGmsCore(instanceId);
    
    // Step 3: Set debug flags to hide emulator detection
    setDebugFlags(instanceId);
    
    // Step 4: Clear root-related flags
    clearRootFlags(instanceId);
    
    // Step 5: Set secure flags
    setSecureFlags(instanceId);
    
    qDebug() << "SafetyNet response spoofed successfully";
    return true;
}

bool SafetyNetSpoofer::spoofPlayIntegrity(const QString& instanceId) {
    qDebug() << "Spoofing Play Integrity for instance:" << instanceId;
    
    // Play Integrity API flags
    QStringList props = {
        // Device Integrity
        "play Integrity.attest.key.version", "1",
        "play Integrity.enabled", "true",
        "play Integrity.basicIntegrity", "true",
        "play Integrity.deviceIntegrity", "true",
        "play Integrity.strongIntegrity", "true",
        "play Integrity.meetsDeviceIntegrity", "true",
        "play Integrity.meetsStrongIntegrity", "true",
        
        // Google Play Services
        "com.google.android.gms.play Integrity.enabled", "true",
        "com.google.android.gms.play Integrity.v2", "true",
        
        // Device Recognition
        "ro.product.first_api_level", "34",
        "ro.build.version.sdk", "34",
        "ro.build.version.all_codenames", "REL",
        
        // System Properties
        "sys.boot_completed", "1",
        "sys.settings_secure_version", "1",
    };
    
    for (int i = 0; i < props.size(); i += 2) {
        executeCommand(instanceId, "setprop " + props[i] + " " + props[i + 1]);
    }
    
    // Set Play Integrity response generation
    executeCommand(instanceId, "setprop debug.play Integrity.force_pass true");
    executeCommand(instanceId, "setprop debug.play Integrity.basic_integrity true");
    executeCommand(instanceId, "setprop debug.play Integrity.device_integrity true");
    
    qDebug() << "Play Integrity spoofed successfully";
    return true;
}

bool SafetyNetSpoofer::spoofBasicIntegrity(const QString& instanceId, bool isSecure) {
    QString value = isSecure ? "true" : "false";
    
    QStringList props = {
        "ro.snet.basic_integrity", value,
        "persist.snet.basic_integrity", value,
        "com.google.android.gms.safetynet.basicIntegrity", value,
    };
    
    for (const QString& prop : props) {
        QString cmd = "setprop " + prop.split(" ").first() + " " + value;
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool SafetyNetSpoofer::spoofDeviceIntegrity(const QString& instanceId) {
    QStringList props = {
        "ro.boot.verifiedbootstate", "green",
        "ro.boot.flash.locked", "1",
        "ro.secure_state", "unlocked",
        "ro.verity.mode", "enforcing",
        "ro.secure", "1",
    };
    
    for (const QString& prop : props) {
        executeCommand(instanceId, "setprop " + prop);
    }
    
    // Set device locked state
    executeCommand(instanceId, "locksettings set-global-adept true");
    
    return true;
}

bool SafetyNetSpoofer::spoofGooglePlayServices(const QString& instanceId) {
    qDebug() << "Spoofing Google Play Services for instance:" << instanceId;
    
    // GMS Core flags
    QStringList cmds = {
        // Enable Play Services
        "settings put global play_services_enabled 1",
        "settings put global games_services_enabled 1",
        "settings put global app_operations_enabled 1",
        
        // Play Protect
        "settings put secure play_protect_enabled 1",
        "settings put secure play_protect_last_scan_time 0",
        
        // Device Certification
        "settings put global device_provisioned 1",
        "settings put secure user_setup_complete 1",
        "settings put secure provisioning_manual_agent 0",
        
        // Backup
        "settings put global backup_enabled 1",
        "settings put global backup_enabled2 1",
    };
    
    for (const QString& cmd : cmds) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

// ============================================================================
// Integrity Check Configuration
// ============================================================================

bool SafetyNetSpoofer::spoofCtsProfileMatch(const QString& instanceId, bool shouldMatch) {
    QString value = shouldMatch ? "true" : "false";
    
    executeCommand(instanceId, "setprop persist.ctssdk.ctsProfileMatch " + value);
    executeCommand(instanceId, "setprop ro.build.version.ctssdk 15");
    executeCommand(instanceId, "setprop ro.config.ctss true");
    executeCommand(instanceId, "setprop ctssdk.prop.name 1");
    
    return true;
}

bool SafetyNetSpoofer::spoofBasicIntegrityCheck(const QString& instanceId, bool isSecure) {
    QString value = isSecure ? "true" : "false";
    
    executeCommand(instanceId, "setprop ro.snet.basic_integrity " + value);
    executeCommand(instanceId, "setprop persist.snet.basic_integrity " + value);
    executeCommand(instanceId, "setprop debug.safetynet.override_basic_integrity " + value);
    
    return true;
}

bool SafetyNetSpoofer::spoofStrongIntegrity(const QString& instanceId, bool isSecure) {
    QString value = isSecure ? "true" : "false";
    
    executeCommand(instanceId, "setprop ro.snet.strong_integrity " + value);
    executeCommand(instanceId, "setprop persist.snet.strong_integrity " + value);
    executeCommand(instanceId, "setprop debug.safetynet.override_strong_integrity " + value);
    
    return true;
}

bool SafetyNetSpoofer::spoofMeetsDeviceIntegrity(const QString& instanceId, bool meets) {
    QString value = meets ? "MEETS_DEVICE_INTEGRITY" : "MEETS_DEVICE_INTEGRITY";
    
    executeCommand(instanceId, "setprop debug.play Integrity.meetsDeviceIntegrity " + value);
    executeCommand(instanceId, "setprop persist.play Integrity.meetsDeviceIntegrity " + value);
    
    return true;
}

bool SafetyNetSpoofer::spoofMeetsStrongIntegrity(const QString& instanceId, bool meets) {
    QString value = meets ? "MEETS_STRONG_INTEGRITY" : "MEETS_STRONG_INTEGRITY";
    
    executeCommand(instanceId, "setprop debug.play Integrity.meetsStrongIntegrity " + value);
    executeCommand(instanceId, "setprop persist.play Integrity.meetsStrongIntegrity " + value);
    
    return true;
}

// ============================================================================
// Verification Helpers
// ============================================================================

bool SafetyNetSpoofer::verifySafetyNet(const QString& instanceId) {
    QString result = executeCommandSync(instanceId, 
        "dumpsys package com.google.android.gms | grep -i safetynet");
    
    return result.contains("ctsProfileMatch=true") && 
           result.contains("basicIntegrity=true");
}

bool SafetyNetSpoofer::verifyPlayIntegrity(const QString& instanceId) {
    QString result = executeCommandSync(instanceId,
        "getprop | grep -i play Integrity");
    
    return result.contains("basicIntegrity=true") &&
           result.contains("deviceIntegrity=true");
}

QJsonObject SafetyNetSpoofer::getIntegrityStatus(const QString& instanceId) {
    QJsonObject status;
    
    // SafetyNet status
    status["safetyNet"] = QJsonObject{
        {"ctsProfileMatch", executeCommandSync(instanceId, "getprop persist.ctssdk.ctsProfileMatch").trimmed()},
        {"basicIntegrity", executeCommandSync(instanceId, "getprop ro.snet.basic_integrity").trimmed()},
        {"deviceIntegrity", executeCommandSync(instanceId, "getprop ro.boot.verifiedbootstate").trimmed()},
    };
    
    // Play Integrity status
    status["playIntegrity"] = QJsonObject{
        {"basicIntegrity", executeCommandSync(instanceId, "getprop debug.play Integrity.basic_integrity").trimmed()},
        {"deviceIntegrity", executeCommandSync(instanceId, "getprop debug.play Integrity.device_integrity").trimmed()},
        {"strongIntegrity", executeCommandSync(instanceId, "getprop debug.play Integrity.strong_integrity").trimmed()},
    };
    
    // Boot state
    status["bootState"] = QJsonObject{
        {"verifiedBootState", executeCommandSync(instanceId, "getprop ro.boot.verifiedbootstate").trimmed()},
        {"flashLocked", executeCommandSync(instanceId, "getprop ro.boot.flash.locked").trimmed()},
        {"secureState", executeCommandSync(instanceId, "getprop ro.secure_state").trimmed()},
    };
    
    return status;
}

// ============================================================================
// Certificate & Signature Spoofing
// ============================================================================

bool SafetyNetSpoofer::installSpoofedGmsCore(const QString& instanceId) {
    qDebug() << "Installing spoofed GMS Core for instance:" << instanceId;
    
    // Set GMS Core spoofing flags
    QStringList cmds = {
        // GMS Core spoofing
        "settings put global gms_core_version 230604034",
        "settings put global play_services_version 230604034",
        
        // SafetyNet Provider
        "settings put secure safetynet.enabled 1",
        
        // attestation
        "setprop ro.attestation.enabled true",
        "setprop ro.config.safetynet.enabled true",
        
        // Device info matching real device
        "setprop ro.product.model \"SM-S928B\"",
        "setprop ro.product.brand \"samsung\"",
        "setprop ro.product.manufacturer \"samsung\"",
    };
    
    for (const QString& cmd : cmds) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool SafetyNetSpoofer::patchSafetyNetAttestation(const QString& instanceId) {
    qDebug() << "Patching SafetyNet attestation for instance:" << instanceId;
    
    // Create fake attestation response hook
    QString script = R"(
        #!/system/bin/sh
        # SafetyNet Hook
        if [ "$1" = "attest" ]; then
            echo '{"ctsProfileMatch":true,"basicIntegrity":true,"evaluationType":"BASIC"}'
            exit 0
        fi
    )";
    
    // Push script to instance
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QFile file(tempPath + "/snet_hook.sh");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(script.toUtf8());
        file.close();
    }
    
    pushFile(instanceId, file.fileName(), "/system/bin/snet_hook.sh");
    executeCommand(instanceId, "chmod 755 /system/bin/snet_hook.sh");
    
    return true;
}

bool SafetyNetSpoofer::setPlayProtectionRating(const QString& instanceId, const QString& rating) {
    executeCommand(instanceId, 
        "settings put secure play_protect_certification_level " + rating);
    return true;
}

// ============================================================================
// Internal Helpers
// ============================================================================

bool SafetyNetSpoofer::setDebugFlags(const QString& instanceId) {
    QStringList cmds = {
        // Hide emulator indicators
        "setprop ro.kernel.qemu 0",
        "setprop ro.boot.qemu false",
        "setprop sys.boot_completed 1",
        "persist.sys.boot_completed 1",
        
        // Hide debugging
        "setprop ro.debuggable 0",
        "setprop persist.sys.debuggable 0",
        
        // Hide test keys
        "setprop ro.build.tags release-keys",
        "setprop ro.build.type user",
        
        // Hide ADB
        "setprop persist.adb.notify 0",
        "setprop service.adb.enable 1",
    };
    
    for (const QString& cmd : cmds) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool SafetyNetSpoofer::clearRootFlags(const QString& instanceId) {
    QStringList cmds = {
        // Clear root detection
        "setprop ro.build.selinux 1",
        "setprop ro.secure 1",
        "setprop ro.adb.secure 1",
        
        // Remove su binaries if present
        "rm -f /system/xbin/su",
        "rm -f /system/bin/su",
        "rm -f /sbin/su",
        
        // Disable root access
        "setprop persist.sys.root_access 0",
    };
    
    for (const QString& cmd : cmds) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

bool SafetyNetSpoofer::setSecureFlags(const QString& instanceId) {
    QStringList cmds = {
        // Set verified boot
        "setprop ro.boot.verifiedbootstate green",
        "setprop ro.boot.flash.locked 1",
        
        // Set keymaster
        "setprop ro.keymaster.version 4",
        "setprop ro.hardware.keystore strongbox",
        
        // Set StrongBox if available
        "setprop ro.hardware.strongbox_keystore true",
        
        // Set hardware attestation
        "setprop ro.hardware.attestation true",
        
        // SELinux enforcing
        "setprop ro.build.selinux Enforcing",
    };
    
    for (const QString& cmd : cmds) {
        executeCommand(instanceId, cmd);
    }
    
    return true;
}

QByteArray SafetyNetSpoofer::generateSafetyNetJws(const QString& instanceId) {
    // Generate a fake but valid-looking SafetyNet JWS response
    QJsonObject header = {
        {"alg", "RS256"},
        {"typ", "JWT"}
    };
    
    QJsonObject payload = {
        {"nonce", QString(QCryptographicHash::hash(
            instanceId.toUtf8(), QCryptographicHash::Sha256).toBase64())},
        {"timestampMs", QDateTime::currentMSecsSinceEpoch()},
        {"apkPackageName", "com.google.android.gms"},
        {"ctsProfileMatch", true},
        {"basicIntegrity", true},
        {"evaluationType", "BASIC"},
    };
    
    // Add device info
    QJsonObject deviceInfo = {
        {"manufacturer", "Samsung"},
        {"model", "SM-S928B"},
        {"brand", "samsung"},
        {"device", "dm3q"},
        {"product", "dm3q"},
        {"bootloader", "S928BXXU1AXXX"},
        {"buildFingerprint", "samsung/dm3q/dm3q:14/UP1A.231005.007/20240115.200015:user/release-keys"},
    };
    payload["deviceInfo"] = deviceInfo;
    
    QJsonDocument headerDoc(header), payloadDoc(payload);
    
    QString headerB64 = headerDoc.toJson(QJsonDocument::Compact).toBase64();
    QString payloadB64 = payloadDoc.toJson(QJsonDocument::Compact).toBase64();
    
    // Fake signature
    QString fakeSig = QString(QCryptographicHash::hash(
        (headerB64 + "." + payloadB64).toUtf8(),
        QCryptographicHash::Sha256).toBase64());
    
    return (headerB64 + "." + payloadB64 + "." + fakeSig).toUtf8();
}

QJsonObject SafetyNetSpoofer::generatePlayIntegrityResponse(const QString& instanceId) {
    QJsonObject response = {
        {"tokenPayloadExternal", QJsonObject{
            {"requestDetails", QJsonObject{
                {"requestPackageName", "com.google.android.gms"},
                {"nonce", QString(QCryptographicHash::hash(
                    instanceId.toUtf8(), QCryptographicHash::Sha256).toBase64())},
                {"timestampMs", QDateTime::currentMSecsSinceEpoch()},
            }},
            {"deviceIntegrityVerdict", QJsonObject{
                {"deviceRecognitionVerdict", QJsonArray{"MEETS_DEVICE_INTEGRITY"}},
            }},
            {"appIntegrityVerdict", QJsonObject{
                {"appRecognitionVerdict", "PASS"},
                {"packageName", "com.google.android.gms"},
                {"signatureHashes", QJsonArray{"abc123..."}},
            }},
            {"accountDetails", QJsonObject{
                {"appLicensingVerdict", "LICENSED"},
            }},
        }},
    };
    
    return response;
}

} // namespace VirtualPhonePro
