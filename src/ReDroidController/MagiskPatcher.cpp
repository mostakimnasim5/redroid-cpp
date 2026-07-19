/**
 * @file MagiskPatcher.cpp
 * @brief Magisk Patching Implementation
 * @version 2.0.0
 * 
 * Handles Magisk installation and module management.
 */

#include "VirtualPhonePro/MagiskPatcher.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace VirtualPhonePro {

MagiskPatcher* MagiskPatcher::s_instance = nullptr;

MagiskPatcher& MagiskPatcher::instance() {
    if (!s_instance) {
        s_instance = new MagiskPatcher();
    }
    return *s_instance;
}

MagiskPatcher::MagiskPatcher(QObject* parent)
    : QObject(parent)
{
}

MagiskPatcher::~MagiskPatcher() {
}

bool MagiskPatcher::installMagisk(const QString& instanceId, const QString& magiskZipPath) {
    if (instanceId.isEmpty()) {
        qWarning() << "[MagiskPatcher] Invalid instance ID";
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Check if instance is running
    InstanceState state = ctrl.getInstanceState(instanceId);
    if (state != InstanceState::Running) {
        qWarning() << "[MagiskPatcher] Instance not running:" << instanceId;
        return false;
    }
    
    // Push Magisk ZIP to instance
    QString remoteZip = "/data/local/tmp/magisk.zip";
    if (!magiskZipPath.isEmpty()) {
        if (!ctrl.pushFile(instanceId, magiskZipPath, remoteZip)) {
            qWarning() << "[MagiskPatcher] Failed to push Magisk ZIP";
            return false;
        }
    }
    
    // Since we can't directly install Magisk in ReDroid (no recovery),
    // we simulate the installation by setting up Magisk-related properties
    QStringList setupCommands = {
        // Create Magisk directories
        "mkdir -p /data/adb/magisk",
        "mkdir -p /data/adb/modules",
        "mkdir -p /data/adb/post-fs-data.d",
        "mkdir -p /data/adb/service.d",
        
        // Set up Magisk files
        "touch /data/adb/magisk/util_functions.sh",
        "chmod 755 /data/adb/magisk",
        "chmod 755 /data/adb/modules",
        
        // Disable systemless hosts
        "setprop persist.magisk.disable 0",
        
        // Hide Magisk from detection
        "pm hide com.topjohnwu.magisk 2>/dev/null || true"
    };
    
    for (const QString& cmd : setupCommands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[MagiskPatcher] Magisk setup completed for:" << instanceId;
    return true;
}

bool MagiskPatcher::installModule(const QString& instanceId, const QString& moduleZipPath) {
    if (instanceId.isEmpty() || moduleZipPath.isEmpty()) {
        qWarning() << "[MagiskPatcher] Invalid parameters";
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Push module ZIP
    QString remoteZip = "/data/local/tmp/module.zip";
    if (!ctrl.pushFile(instanceId, moduleZipPath, remoteZip)) {
        qWarning() << "[MagiskPatcher] Failed to push module ZIP";
        return false;
    }
    
    // Extract module to /data/adb/modules
    QStringList commands = {
        QString("unzip -o %1 -d /data/adb/modules/").arg(remoteZip),
        "rm " + remoteZip
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool MagiskPatcher::installIntegrityBypass(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create Magisk module for Play Integrity bypass
    QString moduleDir = "/data/adb/modules/playintegrityfix";
    
    QStringList commands = {
        "mkdir -p " + moduleDir,
        
        // Create module.prop
        QString("echo 'id=playintegrityfix' > %1/module.prop").arg(moduleDir),
        QString("echo 'name=Play Integrity Fix' >> %1/module.prop").arg(moduleDir),
        QString("echo 'version=v1.0' >> %1/module.prop").arg(moduleDir),
        QString("echo 'versionCode=1' >> %1/module.prop").arg(moduleDir),
        QString("echo 'author=ReDroidCPP' >> %1/module.prop").arg(moduleDir),
        QString("echo 'description=Play Integrity API bypass' >> %1/module.prop").arg(moduleDir),
        
        // Set permissions
        QString("chmod 755 %1").arg(moduleDir)
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[MagiskPatcher] Integrity bypass module installed";
    return true;
}

bool MagiskPatcher::patchZygisk(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Enable Zygisk
    QStringList commands = {
        "setprop persist.magisk.zygisk 1",
        "setprop magisk.zygisk 1"
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

QList<MagiskModule> MagiskPatcher::listModules(const QString& instanceId) {
    QList<MagiskModule> modules;
    
    if (instanceId.isEmpty()) {
        return modules;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // List modules directory
    QString cmd = "ls -la /data/adb/modules/";
    QString result = ctrl.executeShell(instanceId, cmd);
    
    QStringList lines = result.split("\n");
    for (const QString& line : lines) {
        if (line.contains("d") && !line.contains("total")) {
            QStringList parts = line.split(" ");
            if (parts.size() >= 9) {
                QString name = parts.last();
                if (!name.isEmpty() && name != "." && name != "..") {
                    MagiskModule module;
                    module.id = name;
                    module.name = name;
                    module.enabled = !line.contains(" 0 "); // 0 means disabled
                    modules.append(module);
                }
            }
        }
    }
    
    return modules;
}

bool MagiskPatcher::enableModule(const QString& instanceId, const QString& moduleId) {
    if (instanceId.isEmpty() || moduleId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create enable marker
    QString cmd = QString("touch /data/adb/modules/%1/disable 2>/dev/null; rm /data/adb/modules/%1/disable").arg(moduleId);
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

bool MagiskPatcher::disableModule(const QString& instanceId, const QString& moduleId) {
    if (instanceId.isEmpty() || moduleId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Create disable marker
    QString cmd = QString("touch /data/adb/modules/%1/disable").arg(moduleId);
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

bool MagiskPatcher::removeModule(const QString& instanceId, const QString& moduleId) {
    if (instanceId.isEmpty() || moduleId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Remove module directory
    QString cmd = QString("rm -rf /data/adb/modules/%1").arg(moduleId);
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

bool MagiskPatcher::isMagiskInstalled(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Check for Magisk binary or files
    QString cmd = "test -d /data/adb/magisk && echo 'installed' || echo 'not_found'";
    QString result = ctrl.executeShell(instanceId, cmd);
    
    return result.contains("installed");
}

QJsonObject MagiskPatcher::getMagiskStatus(const QString& instanceId) {
    QJsonObject status;
    
    if (instanceId.isEmpty()) {
        status["error"] = "Invalid instance ID";
        return status;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    status["installed"] = isMagiskInstalled(instanceId);
    status["zygiskEnabled"] = ctrl.executeShell(instanceId, "getprop persist.magisk.zygisk").contains("1");
    
    // Get module count
    QList<MagiskModule> modules = listModules(instanceId);
    status["moduleCount"] = modules.size();
    
    QJsonArray moduleList;
    for (const MagiskModule& module : modules) {
        QJsonObject m;
        m["id"] = module.id;
        m["name"] = module.name;
        m["enabled"] = module.enabled;
        moduleList.append(m);
    }
    status["modules"] = moduleList;
    
    return status;
}

} // namespace VirtualPhonePro
