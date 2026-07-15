/**
 * @file FindMyDeviceManager.cpp
 * @brief Find My Device Manager Implementation
 */

#include "VirtualPhonePro/FindMyDeviceManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>

namespace VirtualPhonePro {

FindMyDeviceManager* FindMyDeviceManager::s_instance = nullptr;

FindMyDeviceManager& FindMyDeviceManager::instance() {
    if (!s_instance) {
        s_instance = new FindMyDeviceManager();
    }
    return *s_instance;
}

FindMyDeviceManager::FindMyDeviceManager() {
}

// ============================================================================
// Configuration
// ============================================================================

bool FindMyDeviceManager::configure(const QString& instanceId, const FindMyDeviceConfig& config) {
    m_states[instanceId] = config;
    
    qDebug() << "Configured Find My Device for instance:" << instanceId;
    
    return applyToInstance(instanceId);
}

bool FindMyDeviceManager::applyToInstance(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    const FindMyDeviceConfig& config = m_states[instanceId];
    
    QStringList commands;
    
    // Find My Device status
    commands += {
        QString("settings put secure find_my_device_enabled %1").arg(
            config.status == FindMyDeviceStatus::ENABLED ? "1" : "0"),
        QString("settings put secure location_mode %1").arg(
            config.isLocationEnabled ? "3" : "0"),
    };
    
    // Device secure status
    if (config.isDeviceSecure) {
        commands += "settings put secure device_pwd_state 1";
    }
    
    // Battery optimization exemption
    if (config.isBatteryOptimizationExempt) {
        commands += "cmd deviceidle whitelist +com.google.android.gms";
    }
    
    // Account info
    if (!config.accountEmail.isEmpty()) {
        commands += {
            QString("settings put secure android_id %1").arg(
                generateDeviceId(instanceId)),
        };
    }
    
    // Execute commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Find My Device configuration applied to instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Status Management
// ============================================================================

bool FindMyDeviceManager::enable(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = FindMyDeviceConfig();
        m_states[instanceId].status = FindMyDeviceStatus::ENABLED;
    }
    
    m_states[instanceId].status = FindMyDeviceStatus::ENABLED;
    m_states[instanceId].isDeviceSecure = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure find_my_device_enabled 1");
    
    qDebug() << "Find My Device enabled for instance:" << instanceId;
    
    return true;
}

bool FindMyDeviceManager::disable(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].status = FindMyDeviceStatus::DISABLED;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure find_my_device_enabled 0");
    
    qDebug() << "Find My Device disabled for instance:" << instanceId;
    
    return true;
}

FindMyDeviceStatus FindMyDeviceManager::getStatus(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].status;
    }
    return FindMyDeviceStatus::DISABLED;
}

bool FindMyDeviceManager::isOnline(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isOnline;
    }
    return false;
}

// ============================================================================
// Device Location
// ============================================================================

bool FindMyDeviceManager::setLocation(const QString& instanceId, const DeviceLocation& location) {
    m_locations[instanceId] = location;
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].isLocationEnabled = true;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put secure location_mode 3"));
    
    return true;
}

DeviceLocation FindMyDeviceManager::getLocation(const QString& instanceId) const {
    DeviceLocation defaultLocation;
    defaultLocation.latitude = 37.7749;
    defaultLocation.longitude = -122.4194;
    defaultLocation.altitude = 10.0;
    defaultLocation.accuracy = 5.0;
    defaultLocation.provider = "gps";
    defaultLocation.timestamp = QDateTime::currentDateTime();
    defaultLocation.batteryLevel = 75;
    defaultLocation.isCharging = false;
    
    if (m_locations.contains(instanceId)) {
        return m_locations[instanceId];
    }
    
    return defaultLocation;
}

bool FindMyDeviceManager::enableLocation(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isLocationEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure location_mode 3");
    ctrl.executeShell(instanceId, "settings put secure location_providers_allowed gps,network");
    
    return true;
}

bool FindMyDeviceManager::disableLocation(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isLocationEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure location_mode 0");
    ctrl.executeShell(instanceId, "settings put secure location_providers_allowed -gps,-network");
    
    return true;
}

// ============================================================================
// Device Health
// ============================================================================

bool FindMyDeviceManager::setDeviceHealth(const QString& instanceId, DeviceHealthStatus health) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].healthStatus = health;
    
    QString healthString = healthToString(health);
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop device.health %1").arg(healthString.toLower()));
    
    return true;
}

DeviceHealthStatus FindMyDeviceManager::getDeviceHealth(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].healthStatus;
    }
    return DeviceHealthStatus::UNKNOWN;
}

bool FindMyDeviceManager::updateBatteryStatus(const QString& instanceId, int level, bool charging) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].batteryLevel = level;
    m_states[instanceId].isCharging = charging;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("dumpsys battery set level %1").arg(level));
    ctrl.executeShell(instanceId, charging ? "dumpsys battery set ac 1" : "dumpsys battery unplug");
    
    return true;
}

// ============================================================================
// Owner Management
// ============================================================================

bool FindMyDeviceManager::setOwnerInfo(const QString& instanceId, const DeviceOwnerInfo& owner) {
    m_owners[instanceId] = owner;
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].accountEmail = owner.ownerEmail;
        m_states[instanceId].accountId = owner.ownerAccountType;
        m_states[instanceId].isDeviceSecure = owner.isOwnerAccountActive;
    }
    
    qDebug() << "Set owner info for instance:" << instanceId << "-" << owner.ownerEmail;
    
    return true;
}

DeviceOwnerInfo FindMyDeviceManager::getOwnerInfo(const QString& instanceId) const {
    DeviceOwnerInfo defaultOwner;
    defaultOwner.isOwnerAccountActive = false;
    defaultOwner.isManagedDevice = false;
    
    if (m_owners.contains(instanceId)) {
        return m_owners[instanceId];
    }
    
    return defaultOwner;
}

bool FindMyDeviceManager::isDeviceOwned(const QString& instanceId) const {
    if (m_owners.contains(instanceId)) {
        return m_owners[instanceId].isOwnerAccountActive;
    }
    return false;
}

// ============================================================================
// Remote Actions (Simulation)
// ============================================================================

bool FindMyDeviceManager::simulateRing(const QString& instanceId) {
    qDebug() << "Simulating ring for instance:" << instanceId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "media volume play ringtone");
    ctrl.executeShell(instanceId, "setprop persist.sys.ring.enabled true");
    
    return true;
}

DeviceLocation FindMyDeviceManager::simulateLocate(const QString& instanceId) {
    qDebug() << "Simulating locate for instance:" << instanceId;
    
    DeviceLocation location = getLocation(instanceId);
    location.timestamp = QDateTime::currentDateTime();
    location.provider = "fused";
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "dumpsys location");
    
    return location;
}

bool FindMyDeviceManager::simulateRemoteLock(const QString& instanceId, const QString& password) {
    qDebug() << "Simulating remote lock for instance:" << instanceId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "locksmith reset_password " + password);
    ctrl.executeShell(instanceId, "locksmith lock");
    ctrl.executeShell(instanceId, "input keyevent 26"); // Power button
    
    return true;
}

bool FindMyDeviceManager::simulateRemoteWipe(const QString& instanceId) {
    qDebug() << "Simulating remote wipe for instance:" << instanceId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "wipe all");
    ctrl.executeShell(instanceId, "reboot -w");
    
    return true;
}

bool FindMyDeviceManager::simulateSafeMode(const QString& instanceId, bool enable) {
    qDebug() << "Simulating safe mode" << (enable ? "enable" : "disable") 
             << "for instance:" << instanceId;
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (enable) {
        ctrl.executeShell(instanceId, "setprop persist.sys.safe_mode true");
        ctrl.executeShell(instanceId, "reboot safe");
    } else {
        ctrl.executeShell(instanceId, "setprop persist.sys.safe_mode false");
        ctrl.executeShell(instanceId, "reboot");
    }
    
    return true;
}

// ============================================================================
// Account Linking
// ============================================================================

bool FindMyDeviceManager::linkAccount(const QString& instanceId, const QString& email, 
                                      const QString& accountType) {
    DeviceOwnerInfo owner;
    owner.ownerEmail = email;
    owner.ownerAccountType = accountType;
    owner.isOwnerAccountActive = true;
    owner.isManagedDevice = false;
    
    setOwnerInfo(instanceId, owner);
    
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = FindMyDeviceConfig();
    }
    
    m_states[instanceId].accountEmail = email;
    m_states[instanceId].accountId = accountType;
    m_states[instanceId].status = FindMyDeviceStatus::ENABLED;
    m_states[instanceId].isOnline = true;
    m_states[instanceId].lastSyncTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "am start -a android.settings.ADD_ACCOUNT_SETTINGS");
    
    qDebug() << "Linked FMD account" << email << "for instance:" << instanceId;
    
    return true;
}

bool FindMyDeviceManager::unlinkAccount(const QString& instanceId) {
    m_owners.remove(instanceId);
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].accountEmail = "";
        m_states[instanceId].accountId = "";
        m_states[instanceId].status = FindMyDeviceStatus::DISABLED;
    }
    
    qDebug() << "Unlinked FMD account for instance:" << instanceId;
    
    return true;
}

bool FindMyDeviceManager::sync(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isOnline = true;
    m_states[instanceId].lastSyncTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "dumpsys android.gms.gms_core_ready");
    
    qDebug() << "Synced FMD for instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

FindMyDeviceConfig FindMyDeviceManager::getCompleteState(const QString& instanceId) const {
    FindMyDeviceConfig defaultConfig;
    defaultConfig.status = FindMyDeviceStatus::DISABLED;
    defaultConfig.isDeviceSecure = false;
    defaultConfig.isLocationEnabled = false;
    defaultConfig.isOnline = false;
    defaultConfig.healthStatus = DeviceHealthStatus::UNKNOWN;
    defaultConfig.batteryLevel = 75;
    defaultConfig.isCharging = false;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    return defaultConfig;
}

QJsonObject FindMyDeviceManager::getFMDInfoJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_states.contains(instanceId)) {
        const FindMyDeviceConfig& config = m_states[instanceId];
        
        json["status"] = statusToString(config.status);
        json["isOnline"] = config.isOnline;
        json["isDeviceSecure"] = config.isDeviceSecure;
        json["isLocationEnabled"] = config.isLocationEnabled;
        json["healthStatus"] = healthToString(config.healthStatus);
        json["batteryLevel"] = config.batteryLevel;
        json["isCharging"] = config.isCharging;
        json["accountEmail"] = config.accountEmail;
        json["lastSync"] = config.lastSyncTime;
        json["deviceName"] = config.deviceName;
        
        // Location
        if (m_locations.contains(instanceId)) {
            const DeviceLocation& loc = m_locations[instanceId];
            QJsonObject location;
            location["latitude"] = loc.latitude;
            location["longitude"] = loc.longitude;
            location["accuracy"] = loc.accuracy;
            location["provider"] = loc.provider;
            location["timestamp"] = loc.timestamp.toString(Qt::ISODate);
            json["location"] = location;
        }
        
        // Owner
        if (m_owners.contains(instanceId)) {
            const DeviceOwnerInfo& owner = m_owners[instanceId];
            QJsonObject ownerInfo;
            ownerInfo["email"] = owner.ownerEmail;
            ownerInfo["type"] = owner.ownerAccountType;
            ownerInfo["isActive"] = owner.isOwnerAccountActive;
            ownerInfo["isManaged"] = owner.isManagedDevice;
            json["owner"] = ownerInfo;
        }
    }
    
    return json;
}

bool FindMyDeviceManager::reset(const QString& instanceId) {
    m_states.remove(instanceId);
    m_owners.remove(instanceId);
    m_locations.remove(instanceId);
    
    return true;
}

// ============================================================================
// Private Helpers
// ============================================================================

QString FindMyDeviceManager::statusToString(FindMyDeviceStatus status) const {
    switch (status) {
        case FindMyDeviceStatus::ENABLED: return "Enabled";
        case FindMyDeviceStatus::DISABLED: return "Disabled";
        case FindMyDeviceStatus::ACTIVATING: return "Activating";
        case FindMyDeviceStatus::UNAVAILABLE: return "Unavailable";
        default: return "Unknown";
    }
}

QString FindMyDeviceManager::healthToString(DeviceHealthStatus health) const {
    switch (health) {
        case DeviceHealthStatus::HEALTHY: return "Healthy";
        case DeviceHealthStatus::WARNING: return "Warning";
        case DeviceHealthStatus::CRITICAL: return "Critical";
        case DeviceHealthStatus::UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

QString FindMyDeviceManager::generateDeviceId(const QString& instanceId) const {
    QByteArray hash = QCryptographicHash::hash(
        (instanceId + "findmydevice").toUtf8(), 
        QCryptographicHash::Md5
    );
    return hash.toHex().left(16);
}

} // namespace VirtualPhonePro
