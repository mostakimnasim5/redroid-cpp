#pragma once

#ifndef VIRTUALPHONEPRO_MAGISK_PATCHER_H
#define VIRTUALPHONEPRO_MAGISK_PATCHER_H

#include <QString>
#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <QMap>
#include <QJsonObject>

namespace VirtualPhonePro {

/**
 * @brief Magisk module configuration
 */
struct MagiskModule {
    QString id;
    QString name;
    QString version;
    QString description;
    QString author;
    QString downloadUrl;
    bool enabled;
};

/**
 * @brief RomPatcher - Custom ROM and Magisk module integration
 * 
 * Provides Magisk/Zygisk module installation and management
 * for enhanced system modifications.
 */
class MagiskPatcher {
public:
    static MagiskPatcher& instance();
    
    // =========================================================================
    // Root & Magisk Installation
    // =========================================================================
    
    /**
     * @brief Check if device is rooted
     */
    bool isRooted(const QString& instanceId);
    
    /**
     * @brief Enable root access
     */
    bool enableRoot(const QString& instanceId);
    
    /**
     * @brief Install Magisk
     */
    bool installMagisk(const QString& instanceId, const QString& magiskZip);
    
    /**
     * @brief Check Magisk version
     */
    QString getMagiskVersion(const QString& instanceId);
    
    // =========================================================================
    // Module Management
    // =========================================================================
    
    /**
     * @brief Install Magisk module
     */
    bool installModule(const QString& instanceId, const QString& moduleZip);
    
    /**
     * @brief Remove Magisk module
     */
    bool removeModule(const QString& instanceId, const QString& moduleId);
    
    /**
     * @brief Enable module
     */
    bool enableModule(const QString& instanceId, const QString& moduleId);
    
    /**
     * @brief Disable module
     */
    bool disableModule(const QString& instanceId, const QString& moduleId);
    
    /**
     * @brief List installed modules
     */
    QList<MagiskModule> listModules(const QString& instanceId);
    
    // =========================================================================
    // System Patching
    // =========================================================================
    
    /**
     * @brief Patch system properties
     */
    bool patchSystemProperties(const QString& instanceId,
                              const QMap<QString, QString>& props);
    
    /**
     * @brief Install custom hosts file
     */
    bool installHosts(const QString& instanceId, const QString& hostsPath);
    
    /**
     * @brief Install CA certificates
     */
    bool installCACert(const QString& instanceId, const QString& certPath);
    
    /**
     * @brief Patch SELinux policy
     */
    bool patchSELinux(const QString& instanceId);
    
    // =========================================================================
    // SafetyNet/Play Integrity
    // =========================================================================
    
    /**
     * @brief Install SafetyNet/Play Integrity bypass modules
     */
    bool installIntegrityBypass(const QString& instanceId);
    
    /**
     * @brief Patch Zygisk configuration
     */
    bool patchZygisk(const QString& instanceId);
    
    // Additional methods used in implementation
    bool isMagiskInstalled(const QString& instanceId);
    QJsonObject getMagiskStatus(const QString& instanceId);

private:
    MagiskPatcher();
    ~MagiskPatcher();
    static MagiskPatcher* s_instance;
    
    bool executeCommand(const QString& instanceId, const QString& command);
    QString executeCommandSync(const QString& instanceId, const QString& command);
    bool pushFile(const QString& instanceId, const QString& local, const QString& remote);
    bool installZip(const QString& instanceId, const QString& zipPath);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_MAGISK_PATCHER_H
