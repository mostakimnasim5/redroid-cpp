/**
 * @file FindMyDeviceManager.h
 * @brief Find My Device Manager
 * @version 3.0.0
 * 
 * Provides Find My Device (FMD) simulation:
 * - Find My Device status and configuration
 * - Device location services
 * - Remote lock/wipe capability
 * - Device health indicators
 */

#pragma once

#ifndef VIRTUALPHONEPRO_FIND_MY_DEVICE_MANAGER_H
#define VIRTUALPHONEPRO_FIND_MY_DEVICE_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// Find My Device Status
enum class FindMyDeviceStatus {
    DISABLED,
    ENABLED,
    ACTIVATING,
    UNAVAILABLE
};

// Device Health Status
enum class DeviceHealthStatus {
    HEALTHY,
    WARNING,
    CRITICAL,
    UNKNOWN
};

// Last Known Location
struct DeviceLocation {
    double latitude;
    double longitude;
    double altitude;
    double accuracy;
    QString provider;      // gps, network, passive
    QDateTime timestamp;
    int batteryLevel;
    bool isCharging;
};

// Find My Device Configuration
struct FindMyDeviceConfig {
    FindMyDeviceStatus status;
    bool isDeviceSecure;
    bool isLocationEnabled;
    bool isOnline;
    bool isBatteryOptimizationExempt;
    bool isBackgroundRestrictionExempt;
    QString lastSyncTime;
    QString accountEmail;
    QString accountId;
    DeviceHealthStatus healthStatus;
    int batteryLevel;
    bool isCharging;
    QString deviceName;
    QString imei;
    QString lastKnownNetwork;
};

// Device Owner Info
struct DeviceOwnerInfo {
    QString ownerName;
    QString ownerEmail;
    QString ownerAccountType;  // google, samsung, huawei
    bool isOwnerAccountActive;
    bool isManagedDevice;
    QString managementServer;
};

class FindMyDeviceManager {
public:
    static FindMyDeviceManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure Find My Device
     */
    bool configure(const QString& instanceId, const FindMyDeviceConfig& config);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Status Management
    // =========================================================================
    
    /**
     * @brief Enable Find My Device
     */
    bool enable(const QString& instanceId);
    
    /**
     * @brief Disable Find My Device
     */
    bool disable(const QString& instanceId);
    
    /**
     * @brief Get Find My Device status
     */
    FindMyDeviceStatus getStatus(const QString& instanceId) const;
    
    /**
     * @brief Check if device is online
     */
    bool isOnline(const QString& instanceId) const;
    
    // =========================================================================
    // Device Location
    // =========================================================================
    
    /**
     * @brief Set last known location
     */
    bool setLocation(const QString& instanceId, const DeviceLocation& location);
    
    /**
     * @brief Get last known location
     */
    DeviceLocation getLocation(const QString& instanceId) const;
    
    /**
     * @brief Enable location services
     */
    bool enableLocation(const QString& instanceId);
    
    /**
     * @brief Disable location services
     */
    bool disableLocation(const QString& instanceId);
    
    // =========================================================================
    // Device Health
    // =========================================================================
    
    /**
     * @brief Set device health status
     */
    bool setDeviceHealth(const QString& instanceId, DeviceHealthStatus health);
    
    /**
     * @brief Get device health status
     */
    DeviceHealthStatus getDeviceHealth(const QString& instanceId) const;
    
    /**
     * @brief Update battery status
     */
    bool updateBatteryStatus(const QString& instanceId, int level, bool charging);
    
    // =========================================================================
    // Owner Management
    // =========================================================================
    
    /**
     * @brief Set device owner info
     */
    bool setOwnerInfo(const QString& instanceId, const DeviceOwnerInfo& owner);
    
    /**
     * @brief Get device owner info
     */
    DeviceOwnerInfo getOwnerInfo(const QString& instanceId) const;
    
    /**
     * @brief Check if device is owned
     */
    bool isDeviceOwned(const QString& instanceId) const;
    
    // =========================================================================
    // Remote Actions (Simulation)
    // =========================================================================
    
    /**
     * @brief Simulate ring device
     */
    bool simulateRing(const QString& instanceId);
    
    /**
     * @brief Simulate locate device
     */
    DeviceLocation simulateLocate(const QString& instanceId);
    
    /**
     * @brief Simulate remote lock
     */
    bool simulateRemoteLock(const QString& instanceId, const QString& password);
    
    /**
     * @brief Simulate remote wipe
     */
    bool simulateRemoteWipe(const QString& instanceId);
    
    /**
     * @brief Simulate safe mode
     */
    bool simulateSafeMode(const QString& instanceId, bool enable);
    
    // =========================================================================
    // Account Linking
    // =========================================================================
    
    /**
     * @brief Link Find My Device account
     */
    bool linkAccount(const QString& instanceId, const QString& email, const QString& accountType);
    
    /**
     * @brief Unlink Find My Device account
     */
    bool unlinkAccount(const QString& instanceId);
    
    /**
     * @brief Sync Find My Device
     */
    bool sync(const QString& instanceId);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get complete FMD state
     */
    FindMyDeviceConfig getCompleteState(const QString& instanceId) const;
    
    /**
     * @brief Get FMD info as JSON
     */
    QJsonObject getFMDInfoJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
private:
    static FindMyDeviceManager* s_instance;
    FindMyDeviceManager();
    
    // Helper methods
    QString statusToString(FindMyDeviceStatus status) const;
    QString healthToString(DeviceHealthStatus health) const;
    QString generateDeviceId(const QString& instanceId) const;
    
    QMap<QString, FindMyDeviceConfig> m_states;
    QMap<QString, DeviceOwnerInfo> m_owners;
    QMap<QString, DeviceLocation> m_locations;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_FIND_MY_DEVICE_MANAGER_H
