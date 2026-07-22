#pragma once

#ifndef VIRTUALPHONEPRO_APP_CLONER_H
#define VIRTUALPHONEPRO_APP_CLONER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>

namespace VirtualPhonePro {

/**
 * @brief AppCloner - Clone apps with different data
 * 
 * Enables running multiple instances of the same app with different accounts
 * by manipulating Android's package manager and data directories.
 */
class AppCloner {
public:
    static AppCloner& instance();
    
    // =========================================================================
    // App Management
    // =========================================================================
    
    /**
     * @brief Get list of installed apps
     * @param instanceId Target instance
     * @return List of package names
     */
    QStringList getInstalledApps(const QString& instanceId);
    
    /**
     * @brief Clone an app to new package name
     * @param instanceId Target instance
     * @param originalPackage Original package name
     * @param clonePackage New package name for clone
     * @param cloneLabel Display name for clone
     * @return true if successful
     */
    bool cloneApp(const QString& instanceId, 
                  const QString& originalPackage,
                  const QString& clonePackage,
                  const QString& cloneLabel);
    
    /**
     * @brief Install app to specific package name
     * @param instanceId Target instance
     * @param apkPath Path to APK file
     * @param packageName Package name to assign
     * @return true if successful
     */
    bool installAsPackage(const QString& instanceId,
                         const QString& apkPath,
                         const QString& packageName);
    
    /**
     * @brief Clear app data for fresh start
     * @param instanceId Target instance
     * @param packageName Package name
     * @return true if successful
     */
    bool clearAppData(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Get app info
     * @param instanceId Target instance
     * @param packageName Package name
     * @return App info JSON
     */
    QJsonObject getAppInfo(const QString& instanceId, const QString& packageName);
    
    // =========================================================================
    // Multi-Account Support
    // =========================================================================
    
    /**
     * @brief Create Android work profile for multi-account
     * @param instanceId Target instance
     * @param profileName Profile name
     * @return true if successful
     */
    bool createWorkProfile(const QString& instanceId, const QString& profileName);
    
    /**
     * @brief Install app to work profile
     * @param instanceId Target instance
     * @param packageName Package name
     * @param profileName Work profile name
     * @return true if successful
     */
    bool installToWorkProfile(const QString& instanceId,
                             const QString& packageName,
                             const QString& profileName);
    
    /**
     * @brief List cloned app packages
     * @param instanceId Target instance
     * @return Map of original -> clone packages
     */
    QMap<QString, QString> listClonedApps(const QString& instanceId);
    
private:
    static AppCloner* s_instance;
    AppCloner() = default;
    
    bool executeCommand(const QString& instanceId, const QString& command);
    QString executeCommandSync(const QString& instanceId, const QString& command);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_APP_CLONER_H
