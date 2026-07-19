/**
 * @file DeviceBehaviorManager.cpp
 * @brief Device Behavior Manager Implementation
 */

#include "VirtualPhonePro/DeviceBehaviorManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QJsonArray>
#include <QJsonObject>

namespace VirtualPhonePro {

DeviceBehaviorManager* DeviceBehaviorManager::s_instance = nullptr;

DeviceBehaviorManager& DeviceBehaviorManager::instance() {
    if (!s_instance) {
        s_instance = new DeviceBehaviorManager();
    }
    return *s_instance;
}

DeviceBehaviorManager::DeviceBehaviorManager() {
}

// ============================================================================
// Configuration
// ============================================================================

bool DeviceBehaviorManager::configure(const QString& instanceId, const DeviceBehaviorState& config) {
    m_states[instanceId] = config;
    m_states[instanceId].instanceId = instanceId;
    
    qDebug() << "Configured device behavior for instance:" << instanceId;
    
    return applyToInstance(instanceId);
}

bool DeviceBehaviorManager::applyToInstance(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    DeviceBehaviorState& state = m_states[instanceId];
    
    QStringList commands;
    
    // Power Profile
    switch (state.currentPowerProfile) {
        case PowerProfile::HIGH_PERFORMANCE:
            commands += {
                "settings put global power_profile 0",
                "settings put global high_performance_enabled 1",
                "setprop persist.power.profile performance",
                "setprop sys.perf.profile 0",
            };
            break;
        case PowerProfile::BALANCED:
            commands += {
                "settings put global power_profile 1",
                "setprop persist.power.profile balanced",
                "setprop sys.perf.profile 1",
            };
            break;
        case PowerProfile::POWER_SAVER:
            commands += {
                "settings put global power_profile 2",
                "settings put global high_performance_enabled 0",
                "setprop persist.power.profile powersave",
                "setprop sys.perf.profile 2",
            };
            break;
        case PowerProfile::ADAPTIVE:
            commands += {
                "settings put global power_profile 3",
                "setprop persist.power.profile adaptive",
                "setprop sys.perf.profile 3",
            };
            break;
    }
    
    // Adaptive Battery
    commands += QString("settings put global adaptive_battery_enabled %1")
                    .arg(state.isAdaptiveBatteryEnabled ? "1" : "0");
    
    // Battery Saver Threshold
    commands += QString("settings put global low_power_trigger_level %1")
                    .arg(state.batterySaverThreshold);
    
    // Battery Optimization
    switch (state.globalOptimization) {
        case BatteryOptimizationLevel::UNRESTRICTED:
            commands += "settings put global battery_optimization_mode 0";
            break;
        case BatteryOptimizationLevel::OPTIMIZED:
            commands += "settings put global battery_optimization_mode 1";
            break;
        case BatteryOptimizationLevel::RESTRICTED:
            commands += "settings put global battery_optimization_mode 2";
            break;
        default:
            break;
    }
    
    // App Hibernation
    commands += QString("settings put global app_hibernation_enabled %1")
                    .arg(state.isAppHibernationEnabled ? "1" : "0");
    
    // Background Limit
    commands += QString("settings put global background_limit_enabled %1")
                    .arg(state.isBackgroundLimitEnabled ? "1" : "0");
    commands += QString("settings put global max_background_apps %1")
                    .arg(state.backgroundLimitCount);
    
    // Data Usage
    commands += QString("settings put global data_limit_enabled %1")
                    .arg(state.isDataLimitEnabled ? "1" : "0");
    commands += QString("settings put global mobile_data_limit %1")
                    .arg(state.mobileDataLimit);
    
    // Execute all commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Device behavior applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Power Profiles
// ============================================================================

bool DeviceBehaviorManager::setPowerProfile(const QString& instanceId, PowerProfile profile) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].currentPowerProfile = profile;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    switch (profile) {
        case PowerProfile::HIGH_PERFORMANCE:
            ctrl.executeShell(instanceId, "settings put global power_profile 0");
            ctrl.executeShell(instanceId, "settings put global high_performance_enabled 1");
            ctrl.executeShell(instanceId, "setprop persist.power.profile performance");
            break;
        case PowerProfile::BALANCED:
            ctrl.executeShell(instanceId, "settings put global power_profile 1");
            ctrl.executeShell(instanceId, "setprop persist.power.profile balanced");
            break;
        case PowerProfile::POWER_SAVER:
            ctrl.executeShell(instanceId, "settings put global power_profile 2");
            ctrl.executeShell(instanceId, "settings put global high_performance_enabled 0");
            ctrl.executeShell(instanceId, "setprop persist.power.profile powersave");
            break;
        case PowerProfile::ADAPTIVE:
            ctrl.executeShell(instanceId, "settings put global power_profile 3");
            ctrl.executeShell(instanceId, "setprop persist.power.profile adaptive");
            break;
    }
    
    qDebug() << "Set power profile for" << instanceId << "to" << powerProfileToString(profile);
    
    return true;
}

PowerProfile DeviceBehaviorManager::getPowerProfile(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].currentPowerProfile;
    }
    return PowerProfile::BALANCED;
}

bool DeviceBehaviorManager::enableAdaptiveBattery(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isAdaptiveBatteryEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global adaptive_battery_enabled 1");
    ctrl.executeShell(instanceId, "settings put global adaptive_battery_management_enabled 1");
    
    return true;
}

bool DeviceBehaviorManager::disableAdaptiveBattery(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isAdaptiveBatteryEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global adaptive_battery_enabled 0");
    
    return true;
}

bool DeviceBehaviorManager::setBatterySaverThreshold(const QString& instanceId, int threshold) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].batterySaverThreshold = threshold;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put global low_power_trigger_level %1").arg(threshold));
    
    return true;
}

// ============================================================================
// App Hibernation
// ============================================================================

bool DeviceBehaviorManager::enableAppHibernation(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].isAppHibernationEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global app_hibernation_enabled 1");
    
    return true;
}

bool DeviceBehaviorManager::disableAppHibernation(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isAppHibernationEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global app_hibernation_enabled 0");
    
    return true;
}

bool DeviceBehaviorManager::hibernateApp(const QString& instanceId, const QString& packageName) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    AppHibernationConfig config;
    
    if (state.hibernationConfigs.contains(packageName)) {
        config = state.hibernationConfigs[packageName];
    } else {
        config.packageName = packageName;
        config.isHibernatable = true;
        config.isExemptFromHibernation = false;
        config.batteryThreshold = state.defaultBatteryThreshold;
        config.inactivityDays = state.defaultHibernationDays;
        config.batteryImpactMah = QRandomGenerator::global()->bounded(5, 50);
    }
    
    config.isHibernated = true;
    config.hibernatedAt = QDateTime::currentDateTime();
    
    state.hibernationConfigs[packageName] = config;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("cmd app set-hibernated %1 true").arg(packageName));
    
    qDebug() << "Hibernated app:" << packageName;
    
    return true;
}

bool DeviceBehaviorManager::unhhibernateApp(const QString& instanceId, const QString& packageName) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    
    if (state.hibernationConfigs.contains(packageName)) {
        state.hibernationConfigs[packageName].isHibernated = false;
        state.hibernationConfigs[packageName].lastUsed = QDateTime::currentDateTime();
        
        ReDroidController& ctrl = ReDroidController::instance();
        ctrl.executeShell(instanceId, QString("cmd app set-hibernated %1 false").arg(packageName));
        
        qDebug() << "Unhibernated app:" << packageName;
        
        return true;
    }
    
    return false;
}

bool DeviceBehaviorManager::configureAppHibernation(const QString& instanceId, 
                                                    const AppHibernationConfig& config) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].hibernationConfigs[config.packageName] = config;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("cmd app set-hibernate-eligible %1 %2")
                      .arg(config.packageName).arg(config.isHibernatable ? "true" : "false"));
    ctrl.executeShell(instanceId, QString("cmd app set-exemption %1 %2")
                      .arg(config.packageName).arg(config.isExemptFromHibernation ? "true" : "false"));
    
    return true;
}

AppHibernationConfig DeviceBehaviorManager::getAppHibernationConfig(const QString& instanceId, 
                                                                     const QString& packageName) const {
    AppHibernationConfig defaultConfig;
    defaultConfig.packageName = packageName;
    defaultConfig.isHibernatable = true;
    defaultConfig.isHibernated = false;
    defaultConfig.isExemptFromHibernation = false;
    defaultConfig.batteryThreshold = 20;
    defaultConfig.inactivityDays = 3;
    defaultConfig.batteryImpactMah = 10;
    
    if (m_states.contains(instanceId) && 
        m_states[instanceId].hibernationConfigs.contains(packageName)) {
        return m_states[instanceId].hibernationConfigs[packageName];
    }
    
    return defaultConfig;
}

QStringList DeviceBehaviorManager::getHibernatedApps(const QString& instanceId) const {
    QStringList apps;
    
    if (m_states.contains(instanceId)) {
        const DeviceBehaviorState& state = m_states[instanceId];
        for (auto it = state.hibernationConfigs.constBegin(); 
             it != state.hibernationConfigs.constEnd(); ++it) {
            if (it.value().isHibernated) {
                apps.append(it.key());
            }
        }
    }
    
    return apps;
}

// ============================================================================
// Battery Optimization
// ============================================================================

bool DeviceBehaviorManager::setBatteryOptimization(const QString& instanceId, 
                                                    const QString& packageName,
                                                    BatteryOptimizationLevel level) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].appBatteryOptimization[packageName] = level;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    switch (level) {
        case BatteryOptimizationLevel::UNRESTRICTED:
            ctrl.executeShell(instanceId, QString("cmd app whitelist %1").arg(packageName));
            break;
        case BatteryOptimizationLevel::RESTRICTED:
        case BatteryOptimizationLevel::UNSPECIFIED:
            ctrl.executeShell(instanceId, QString("cmd app unwhitelist %1").arg(packageName));
            break;
        default:
            break;
    }
    
    return true;
}

BatteryOptimizationLevel DeviceBehaviorManager::getBatteryOptimization(const QString& instanceId, 
                                                                         const QString& packageName) const {
    if (m_states.contains(instanceId) && 
        m_states[instanceId].appBatteryOptimization.contains(packageName)) {
        return m_states[instanceId].appBatteryOptimization[packageName];
    }
    
    return BatteryOptimizationLevel::OPTIMIZED;
}

bool DeviceBehaviorManager::requestOptimizationExemption(const QString& instanceId, 
                                                         const QString& packageName) {
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("cmd app whitelist %1").arg(packageName));
    
    if (m_states.contains(instanceId)) {
        m_states[instanceId].appBatteryOptimization[packageName] = BatteryOptimizationLevel::UNRESTRICTED;
    }
    
    return true;
}

QStringList DeviceBehaviorManager::getRestrictedApps(const QString& instanceId) const {
    QStringList apps;
    
    if (m_states.contains(instanceId)) {
        const DeviceBehaviorState& state = m_states[instanceId];
        for (auto it = state.appBatteryOptimization.constBegin(); 
             it != state.appBatteryOptimization.constEnd(); ++it) {
            if (it.value() == BatteryOptimizationLevel::RESTRICTED) {
                apps.append(it.key());
            }
        }
    }
    
    return apps;
}

// ============================================================================
// Data Usage
// ============================================================================

DataUsageRecord DeviceBehaviorManager::getDataUsage(const QString& instanceId, 
                                                    const QString& packageName) const {
    DataUsageRecord defaultRecord;
    defaultRecord.packageName = packageName;
    defaultRecord.wifiBytes = 0;
    defaultRecord.mobileBytes = 0;
    defaultRecord.totalBytes = 0;
    
    if (m_states.contains(instanceId) && 
        m_states[instanceId].dataUsageRecords.contains(packageName)) {
        return m_states[instanceId].dataUsageRecords[packageName];
    }
    
    return defaultRecord;
}

bool DeviceBehaviorManager::resetDataUsage(const QString& instanceId, const QString& packageName) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    
    if (state.dataUsageRecords.contains(packageName)) {
        DataUsageRecord& record = state.dataUsageRecords[packageName];
        record.wifiBytesLastReset = record.wifiBytes;
        record.mobileBytesLastReset = record.mobileBytes;
        record.lastReset = QDateTime::currentDateTime();
        record.wifiBytes = 0;
        record.mobileBytes = 0;
        record.totalBytes = 0;
        
        ReDroidController& ctrl = ReDroidController::instance();
        ctrl.executeShell(instanceId, QString("dumpsys data_usage --reset %1").arg(packageName));
        
        return true;
    }
    
    return false;
}

bool DeviceBehaviorManager::resetAllDataUsage(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    
    for (auto& record : state.dataUsageRecords) {
        record.wifiBytesLastReset = record.wifiBytes;
        record.mobileBytesLastReset = record.mobileBytes;
        record.lastReset = QDateTime::currentDateTime();
        record.wifiBytes = 0;
        record.mobileBytes = 0;
        record.totalBytes = 0;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "dumpsys data_usage --reset-all");
    
    return true;
}

bool DeviceBehaviorManager::setDataLimit(const QString& instanceId, qint64 limitBytes) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].mobileDataLimit = limitBytes;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put global mobile_data_limit %1").arg(limitBytes));
    
    return true;
}

bool DeviceBehaviorManager::setDataLimitEnabled(const QString& instanceId, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isDataLimitEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("settings put global data_limit_enabled %1").arg(enabled ? "1" : "0"));
    
    return true;
}

qint64 DeviceBehaviorManager::getTotalDataUsage(const QString& instanceId) const {
    qint64 total = 0;
    
    if (m_states.contains(instanceId)) {
        const DeviceBehaviorState& state = m_states[instanceId];
        for (const auto& record : state.dataUsageRecords) {
            total += record.totalBytes;
        }
    }
    
    return total;
}

bool DeviceBehaviorManager::simulateDataUsage(const QString& instanceId, const QString& packageName,
                                              qint64 wifiBytes, qint64 mobileBytes) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    DataUsageRecord& record = state.dataUsageRecords[packageName];
    
    record.packageName = packageName;
    record.wifiBytes += wifiBytes;
    record.mobileBytes += mobileBytes;
    record.totalBytes = record.wifiBytes + record.mobileBytes;
    
    return true;
}

// ============================================================================
// App Standby
// ============================================================================

bool DeviceBehaviorManager::setAppStandbyBucket(const QString& instanceId, 
                                                const QString& packageName,
                                                AppStandbyBucket bucket) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].appStandbyBuckets[packageName] = bucket;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("cmd appops set %1 RUN_IN_BACKGROUND %2")
                      .arg(packageName)
                      .arg(standbyBucketToString(bucket).toLower()));
    
    return true;
}

AppStandbyBucket DeviceBehaviorManager::getAppStandbyBucket(const QString& instanceId, 
                                                            const QString& packageName) const {
    if (m_states.contains(instanceId) && 
        m_states[instanceId].appStandbyBuckets.contains(packageName)) {
        return m_states[instanceId].appStandbyBuckets[packageName];
    }
    
    return AppStandbyBucket::WORKING_SET;
}

bool DeviceBehaviorManager::enableBackgroundLimit(const QString& instanceId, int maxBackgroundApps) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = DeviceBehaviorState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    DeviceBehaviorState& state = m_states[instanceId];
    state.isBackgroundLimitEnabled = true;
    state.backgroundLimitCount = maxBackgroundApps;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global background_limit_enabled 1");
    ctrl.executeShell(instanceId, QString("settings put global max_background_apps %1").arg(maxBackgroundApps));
    
    return true;
}

bool DeviceBehaviorManager::disableBackgroundLimit(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isBackgroundLimitEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global background_limit_enabled 0");
    
    return true;
}

// ============================================================================
// Battery Stats
// ============================================================================

BatteryStats DeviceBehaviorManager::getBatteryStats(const QString& instanceId) const {
    BatteryStats defaultStats;
    defaultStats.batteryLevel = 75;
    defaultStats.batteryHealthPercent = 95;
    defaultStats.batteryHealth = "Good";
    defaultStats.batteryStatus = "Discharging";
    defaultStats.batteryPluggedType = "Not Charging";
    defaultStats.batteryTemperature = 320;
    defaultStats.batteryVoltage = 4200;
    defaultStats.batteryCycleCount = 150;
    defaultStats.batteryCapacityMah = 5000;
    defaultStats.batteryCurrentNow = -500;
    defaultStats.batteryCurrentAvg = -200;
    
    if (m_states.contains(instanceId)) {
        const DeviceBehaviorState& state = m_states[instanceId];
        defaultStats.batteryHealthPercent = state.batteryHealthPercent;
    }
    
    return defaultStats;
}

bool DeviceBehaviorManager::simulateBatteryDrain(const QString& instanceId, int percent) {
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("dumpsys battery set level %1").arg(percent));
    ctrl.executeShell(instanceId, "dumpsys battery set status discharging");
    ctrl.executeShell(instanceId, "dumpsys battery unplug");
    
    return true;
}

bool DeviceBehaviorManager::simulateCharging(const QString& instanceId, int targetPercent) {
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "dumpsys battery set ac 1");
    ctrl.executeShell(instanceId, "dumpsys battery set status charging");
    ctrl.executeShell(instanceId, QString("dumpsys battery set level %1").arg(targetPercent));
    
    return true;
}

bool DeviceBehaviorManager::setBatteryTemperature(const QString& instanceId, int temperatureCelsius) {
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("dumpsys battery set temperature %1").arg(temperatureCelsius * 10));
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

DeviceBehaviorState DeviceBehaviorManager::getBehaviorState(const QString& instanceId) const {
    DeviceBehaviorState defaultState;
    defaultState.instanceId = instanceId;
    defaultState.currentPowerProfile = PowerProfile::BALANCED;
    defaultState.globalOptimization = BatteryOptimizationLevel::OPTIMIZED;
    defaultState.isAppHibernationEnabled = false;
    defaultState.isAdaptiveBatteryEnabled = true;
    defaultState.batterySaverThreshold = 20;
    defaultState.batteryHealthPercent = 95;
    defaultState.mobileDataLimit = -1;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    return defaultState;
}

QJsonObject DeviceBehaviorManager::getBehaviorInfoJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_states.contains(instanceId)) {
        const DeviceBehaviorState& state = m_states[instanceId];
        
        json["powerProfile"] = powerProfileToString(state.currentPowerProfile);
        json["adaptiveBattery"] = state.isAdaptiveBatteryEnabled;
        json["batterySaverThreshold"] = state.batterySaverThreshold;
        json["appHibernation"] = state.isAppHibernationEnabled;
        json["dataLimitEnabled"] = state.isDataLimitEnabled;
        json["batteryHealth"] = state.batteryHealthPercent;
        
        // Data usage
        QJsonObject dataUsage;
        dataUsage["mobileLimit"] = state.mobileDataLimit;
        dataUsage["totalUsed"] = getTotalDataUsage(instanceId);
        json["dataUsage"] = dataUsage;
        
        // Hibernated apps
        json["hibernatedApps"] = QJsonArray::fromStringList(getHibernatedApps(instanceId));
        
        // Restricted apps
        json["restrictedApps"] = QJsonArray::fromStringList(getRestrictedApps(instanceId));
    }
    
    return json;
}

bool DeviceBehaviorManager::reset(const QString& instanceId) {
    if (m_states.contains(instanceId)) {
        m_states.remove(instanceId);
        return true;
    }
    return false;
}

bool DeviceBehaviorManager::applyAllBehaviors(const QString& instanceId) {
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

QString DeviceBehaviorManager::powerProfileToString(PowerProfile profile) const {
    switch (profile) {
        case PowerProfile::HIGH_PERFORMANCE: return "High Performance";
        case PowerProfile::BALANCED: return "Balanced";
        case PowerProfile::POWER_SAVER: return "Power Saver";
        case PowerProfile::ADAPTIVE: return "Adaptive";
        default: return "Unknown";
    }
}

QString DeviceBehaviorManager::optimizationLevelToString(BatteryOptimizationLevel level) const {
    switch (level) {
        case BatteryOptimizationLevel::UNRESTRICTED: return "Unrestricted";
        case BatteryOptimizationLevel::OPTIMIZED: return "Optimized";
        case BatteryOptimizationLevel::RESTRICTED: return "Restricted";
        case BatteryOptimizationLevel::UNSPECIFIED: return "Unspecified";
        default: return "Unknown";
    }
}

QString DeviceBehaviorManager::standbyBucketToString(AppStandbyBucket bucket) const {
    switch (bucket) {
        case AppStandbyBucket::ACTIVE: return "ACTIVE";
        case AppStandbyBucket::WORKING_SET: return "WORKING_SET";
        case AppStandbyBucket::FREQUENT: return "FREQUENT";
        case AppStandbyBucket::RARE: return "RARE";
        case AppStandbyBucket::RESTRICTED: return "RESTRICTED";
        case AppStandbyBucket::NEVER: return "NEVER";
        default: return "UNKNOWN";
    }
}

BatteryOptimizationLevel DeviceBehaviorManager::stringToOptimizationLevel(const QString& level) const {
    QString lower = level.toLower();
    if (lower == "unrestricted") return BatteryOptimizationLevel::UNRESTRICTED;
    if (lower == "restricted") return BatteryOptimizationLevel::RESTRICTED;
    if (lower == "unspecified") return BatteryOptimizationLevel::UNSPECIFIED;
    return BatteryOptimizationLevel::OPTIMIZED;
}

AppStandbyBucket DeviceBehaviorManager::stringToStandbyBucket(const QString& bucket) const {
    QString lower = bucket.toLower();
    if (lower == "active") return AppStandbyBucket::ACTIVE;
    if (lower == "frequent") return AppStandbyBucket::FREQUENT;
    if (lower == "rare") return AppStandbyBucket::RARE;
    if (lower == "restricted") return AppStandbyBucket::RESTRICTED;
    if (lower == "never") return AppStandbyBucket::NEVER;
    return AppStandbyBucket::WORKING_SET;
}

qint64 DeviceBehaviorManager::estimateBatterySavings(const QStringList& hibernatedApps) const {
    qint64 totalSavings = 0;
    
    if (m_states.contains("")) {
        const DeviceBehaviorState& state = m_states[""];
        for (const QString& app : hibernatedApps) {
            if (state.hibernationConfigs.contains(app)) {
                totalSavings += state.hibernationConfigs[app].batteryImpactMah;
            } else {
                totalSavings += 10; // Default estimate
            }
        }
    }
    
    return totalSavings;
}

} // namespace VirtualPhonePro
