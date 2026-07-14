/**
 * @file AppCloner.cpp
 * @brief App Cloning Implementation
 * @version 2.0.0
 * 
 * Handles app cloning and multi-account functionality.
 */

#include "VirtualPhonePro/AppCloner.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

AppCloner* AppCloner::s_instance = nullptr;

AppCloner& AppCloner::instance() {
    if (!s_instance) {
        s_instance = new AppCloner();
    }
    return *s_instance;
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
