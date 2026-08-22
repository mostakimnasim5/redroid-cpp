/**
 * @file AppCloner.cpp
 * @brief App Cloning Implementation
 * @version 2.0.0
 * 
 * Handles app cloning and multi-account functionality.
 */

#include "VirtualPhonePro/AppCloner.hpp"
#include "VirtualPhonePro/UniqueDeviceGenerator.hpp"
#include "VirtualPhonePro/DeviceProfile.hpp"
#include "VirtualPhonePro/ReDroidController.hpp"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

AppCloner& AppCloner::instance() {
    static AppCloner s_instance;
    return s_instance;
}

AppCloner::AppCloner(QObject* parent)
    : QObject(parent)
{
}

AppCloner::~AppCloner() {
}

bool AppCloner::cloneApp(const QString& instanceId, const QString& sourcePackage,
                         const QString& targetPackage, const QString& targetName) {
    if (instanceId.isEmpty() || sourcePackage.isEmpty()) {
        qWarning() << "[AppCloner] Invalid parameters";
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Check if instance is running
    if (!ctrl.instanceExists(instanceId)) {
        qWarning() << "[AppCloner] Instance not found:" << instanceId;
        return false;
    }
    
    // Get source APK path
    QStringList cmd = {"pm", "path", sourcePackage};
    QString result = ctrl.executeShell(instanceId, cmd.join(" "));
    
    if (result.isEmpty() || result.contains("package not found")) {
        qWarning() << "[AppCloner] Source package not found:" << sourcePackage;
        return false;
    }
    
    // Extract APK path from result
    QString apkPath;
    QStringList lines = result.split("\n");
    for (const QString& line : lines) {
        if (line.startsWith("package:")) {
            apkPath = line.mid(8).trimmed();
            break;
        }
    }
    
    if (apkPath.isEmpty()) {
        qWarning() << "[AppCloner] Could not extract APK path";
        return false;
    }
    
    // Pull APK
    QString localPath = QDir::temp().filePath("clone_source.apk");
    if (!ctrl.pullFile(instanceId, apkPath, localPath)) {
        qWarning() << "[AppCloner] Failed to pull APK";
        return false;
    }
    
    // Install with new package name
    bool success = installAsPackage(instanceId, localPath, targetPackage, targetName);
    
    // Cleanup
    QFile::remove(localPath);
    
    return success;
}

bool AppCloner::installAsPackage(const QString& instanceId, const QString& apkPath,
                                  const QString& packageName, const QString& appName) {
    if (instanceId.isEmpty() || apkPath.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Push APK to instance
    QString remotePath = "/data/local/tmp/cloned_app.apk";
    if (!ctrl.pushFile(instanceId, apkPath, remotePath)) {
        qWarning() << "[AppCloner] Failed to push APK";
        return false;
    }
    
    // Install with pm command using -p flag
    QString installCmd = QString("pm install -p %1 %2").arg(packageName).arg(remotePath);
    QString result = ctrl.executeShell(instanceId, installCmd);
    
    // Cleanup
    ctrl.executeShell(instanceId, "rm " + remotePath);
    
    return result.contains("Success");
}

bool AppCloner::createWorkProfile(const QString& instanceId, const QString& profileName) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create work profile using pm command
    QString cmd = QString("pm create-user --profileOf 0 --managed %1").arg(profileName);
    QString result = ctrl.executeShell(instanceId, cmd);
    
    return result.contains("Success") || result.contains("created");
}

bool AppCloner::clearAppData(const QString& instanceId, const QString& packageName) {
    if (instanceId.isEmpty() || packageName.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = QString("pm clear %1").arg(packageName);
    QString result = ctrl.executeShell(instanceId, cmd);
    
    return result.contains("Success");
}

bool AppCloner::uninstallApp(const QString& instanceId, const QString& packageName) {
    if (instanceId.isEmpty() || packageName.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = QString("pm uninstall %1").arg(packageName);
    QString result = ctrl.executeShell(instanceId, cmd);
    
    return result.contains("Success");
}

QStringList AppCloner::listInstalledApps(const QString& instanceId) {
    QStringList apps;
    
    if (instanceId.isEmpty()) {
        return apps;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = "pm list packages";
    QString result = ctrl.executeShell(instanceId, cmd);
    
    // Parse output
    QStringList lines = result.split("\n");
    for (const QString& line : lines) {
        if (line.startsWith("package:")) {
            apps.append(line.mid(8).trimmed());
        }
    }
    
    return apps;
}

QMap<QString, QString> AppCloner::listClonedApps(const QString& instanceId) {
    QMap<QString, QString> clones;
    
    if (instanceId.isEmpty()) {
        return clones;
    }
    
    // Get list of packages and check for cloned ones
    QStringList packages = listInstalledApps(instanceId);
    
    for (const QString& pkg : packages) {
        if (pkg.contains(".clone") || pkg.contains("_clone") || pkg.contains("second")) {
            clones[pkg] = pkg; // Map target package to itself
        }
    }
    
    return clones;
}

QJsonObject AppCloner::getAppInfo(const QString& instanceId, const QString& packageName) {
    QJsonObject info;
    
    if (instanceId.isEmpty() || packageName.isEmpty()) {
        return info;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = QString("dumpsys package %1").arg(packageName);
    QString result = ctrl.executeShell(instanceId, cmd);
    
    if (result.isEmpty()) {
        return info;
    }
    
    info["packageName"] = packageName;
    info["installed"] = !result.contains("PackageNotFoundException");
    
    // Parse version
    if (result.contains("versionName=")) {
        int idx = result.indexOf("versionName=");
        QString version = result.mid(idx + 12).split("\n").first();
        info["versionName"] = version.trimmed();
    }
    
    return info;
}

} // namespace VirtualPhonePro

bool AppCloner::executeCommand(const QString& instanceId, const QString& command) {
    ReDroidController& ctrl = ReDroidController::instance();
    return !ctrl.executeShell(instanceId, command).isNull();
}

QString AppCloner::executeCommandSync(const QString& instanceId, const QString& command) {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.executeShell(instanceId, command);
}

QStringList AppCloner::getInstalledApps(const QString& instanceId) {
    return listInstalledApps(instanceId);
}

bool AppCloner::installToWorkProfile(const QString& instanceId,
                                     const QString& packageName,
                                     const QString& profileName) {
    if (instanceId.isEmpty() || packageName.isEmpty() || profileName.isEmpty()) {
        return false;
    }
    // Install the already-installed package into the work profile user.
    QString cmd = QString("pm install-existing --user %1 %2").arg(profileName, packageName);
    QString result = executeCommandSync(instanceId, cmd);
    return result.contains("Success");
}

QString AppCloner::cloneInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();

    InstanceInfo src = ctrl.getInstanceInfo(instanceId);
    if (src.instanceId.isEmpty()) {
        qWarning() << "AppCloner::cloneInstance: source instance not found:" << instanceId;
        return {};
    }

    QString profilesDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
    QDir().mkpath(profilesDir);

    DeviceProfile profile = DeviceProfile::load(profilesDir + "/" + instanceId + ".json");

    // Regenerate every identity-bearing field so the clone presents as a
    // completely distinct physical device.
    QString mfr = profile.manufacturer;
    profile.id            = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.identity.imei         = gen.generateUniqueIMEI(mfr);
    profile.identity.imei2        = gen.generateUniqueIMEI(mfr);
    profile.identity.serialNumber = gen.generateUniqueSerial(mfr);
    profile.identity.androidId    = gen.generateUniqueAndroidId();
    profile.identity.gsfId        = gen.generateUniqueGSFId();
    profile.identity.advertisingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.sim.iccid             = gen.generateUniqueICCID();
    profile.mac.wifiMac           = gen.generateUniqueMAC();
    profile.mac.bluetoothMac      = gen.generateUniqueBluetoothMAC();
    profile.network.wifiMac       = profile.mac.wifiMac;
    profile.network.bluetoothMac  = profile.mac.bluetoothMac;

    QString newId = gen.generateInstanceId();
    if (!profile.save(profilesDir + "/" + newId + ".json")) {
        qWarning() << "AppCloner::cloneInstance: failed to save cloned profile";
        return {};
    }

    if (!ctrl.startInstance(newId, profile)) {
        qWarning() << "AppCloner::cloneInstance: failed to start cloned instance";
        return {};
    }

    return newId;
}

