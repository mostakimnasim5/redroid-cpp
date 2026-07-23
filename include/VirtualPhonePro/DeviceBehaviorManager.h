/**
 * @file DeviceBehaviorManager.h
 * @brief Device Behavior Manager - Power Profiles, App Hibernation, Battery Optimization
 * @version 3.0.0
 * 
 * Provides realistic device behavior simulation:
 * - Power profiles (High Performance, Balanced, Battery Saver)
 * - App Hibernation behavior
 * - Battery Optimization settings
 * - Data Usage tracking and limits
 * - Adaptive Battery management
 * - Background process throttling
 */

#pragma once

#ifndef VIRTUALPHONEPRO_DEVICE_BEHAVIOR_MANAGER_H
#define VIRTUALPHONEPRO_DEVICE_BEHAVIOR_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include <QList>

namespace VirtualPhonePro {

// Power Profile Types
enum class PowerProfile {
    HIGH_PERFORMANCE,
    BALANCED,
    POWER_SAVER,
    ADAPTIVE
};

// App Hibernation State
struct AppHibernationConfig {
    QString packageName;
    bool isHibernatable;
    bool isHibernated;
    bool isExemptFromHibernation;
    int batteryThreshold;       // Battery % threshold for hibernation
    int inactivityDays;         // Days of inactivity before hibernation
    qint64 batteryImpactMah;    // Estimated battery savings per hour (mAh)
    QDateTime lastUsed;
    QDateTime hibernatedAt;
};

// Battery Optimization Level
enum class BatteryOptimizationLevel {
    UNRESTRICTED,       // No restrictions
    OPTIMIZED,          // Balanced (default)
    RESTRICTED,         // Some restrictions
    UNSPECIFIED         // Not set by user
};

// Data Usage Record
struct DataUsageRecord {
    QString packageName;
    qint64 wifiBytes;
    qint64 mobileBytes;
    qint64 totalBytes;
    qint64 wifiBytesLastReset;
    qint64 mobileBytesLastReset;
    QDateTime lastReset;
    QDateTime startDate;
};

// App Standby Bucket
enum class AppStandbyBucket {
    ACTIVE,
    WORKING_SET,
    FREQUENT,
    RARE,
    RESTRICTED,
    NEVER
};

// Complete Device Behavior State
struct DeviceBehaviorState {
    QString instanceId;
    
    // Power Profile
    PowerProfile currentPowerProfile;
    bool isAdaptiveBatteryEnabled;
    bool isPowerReserveEnabled;
    int batterySaverThreshold;     // Auto-enable at X%
    
    // Battery Optimization
    QMap<QString, BatteryOptimizationLevel> appBatteryOptimization;
    BatteryOptimizationLevel globalOptimization;
    
    // App Hibernation
    QMap<QString, AppHibernationConfig> hibernationConfigs;
    bool isAppHibernationEnabled;
    int defaultHibernationDays;
    int defaultBatteryThreshold;
    
    // Data Usage
    QMap<QString, DataUsageRecord> dataUsageRecords;
    qint64 mobileDataLimit;         // -1 for unlimited
    qint64 totalDataLimit;
    bool isDataLimitEnabled;
    QDateTime currentCycleStart;
    int dataCycleDays;
    
    // Background Process
    QMap<QString, AppStandbyBucket> appStandbyBuckets;
    bool isBackgroundLimitEnabled;
    int backgroundLimitCount;
    
    // Adaptive Features
    bool isSmartStorageEnabled;
    bool isAdaptiveConnectivityEnabled;
    int batteryHealthPercent;       // Battery health (0-100%)
    int estimatedCapacityMah;       // Design capacity (mAh)
};

// Battery Statistics
struct BatteryStats {
    int batteryLevel;
    int batteryHealthPercent;
    QString batteryHealth;
    QString batteryStatus;         // Charging, Discharging, Full, Not Charging
    QString batteryPluggedType;     // AC, USB, Wireless
    int batteryTemperature;         // in Celsius * 10 (e.g., 320 = 32.0°C)
    int batteryVoltage;            // mV
    qint64 batteryUptime;
    qint64 batteryScreenOnTime;
    int batteryCycleCount;
    int batteryCapacityMah;
    int batteryCurrentNow;         // mA
    int batteryCurrentAvg;         // mA
};

class DeviceBehaviorManager {
public:
    static DeviceBehaviorManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure device behavior
     */
    bool configure(const QString& instanceId, const DeviceBehaviorState& config);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Power Profiles
    // =========================================================================
    
    /**
     * @brief Set power profile
     */
    bool setPowerProfile(const QString& instanceId, PowerProfile profile);
    
    /**
     * @brief Get current power profile
     */
    PowerProfile getPowerProfile(const QString& instanceId) const;
    
    /**
     * @brief Enable adaptive battery
     */
    bool enableAdaptiveBattery(const QString& instanceId);
    
    /**
     * @brief Disable adaptive battery
     */
    bool disableAdaptiveBattery(const QString& instanceId);
    
    /**
     * @brief Set battery saver threshold
     */
    bool setBatterySaverThreshold(const QString& instanceId, int threshold);
    
    // =========================================================================
    // App Hibernation
    // =========================================================================
    
    /**
     * @brief Enable app hibernation
     */
    bool enableAppHibernation(const QString& instanceId);
    
    /**
     * @brief Disable app hibernation
     */
    bool disableAppHibernation(const QString& instanceId);
    
    /**
     * @brief Hiberate an app
     */
    bool hibernateApp(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Unhiberate an app
     */
    bool unhhibernateApp(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Configure app hibernation settings
     */
    bool configureAppHibernation(const QString& instanceId, const AppHibernationConfig& config);
    
    /**
     * @brief Get hibernation config
     */
    AppHibernationConfig getAppHibernationConfig(const QString& instanceId, const QString& packageName) const;
    
    /**
     * @brief Get all hibernated apps
     */
    QStringList getHibernatedApps(const QString& instanceId) const;
    
    // =========================================================================
    // Battery Optimization
    // =========================================================================
    
    /**
     * @brief Set battery optimization for app
     */
    bool setBatteryOptimization(const QString& instanceId, const QString& packageName, 
                                BatteryOptimizationLevel level);
    
    /**
     * @brief Get battery optimization for app
     */
    BatteryOptimizationLevel getBatteryOptimization(const QString& instanceId, 
                                                    const QString& packageName) const;
    
    /**
     * @brief Request battery optimization exemption
     */
    bool requestOptimizationExemption(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Get all restricted apps
     */
    QStringList getRestrictedApps(const QString& instanceId) const;
    
    // =========================================================================
    // Data Usage
    // =========================================================================
    
    /**
     * @brief Get data usage for app
     */
    DataUsageRecord getDataUsage(const QString& instanceId, const QString& packageName) const;
    
    /**
     * @brief Reset data usage for app
     */
    bool resetDataUsage(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Reset all data usage
     */
    bool resetAllDataUsage(const QString& instanceId);
    
    /**
     * @brief Set data limit
     */
    bool setDataLimit(const QString& instanceId, qint64 limitBytes);
    
    /**
     * @brief Enable/disable data limit
     */
    bool setDataLimitEnabled(const QString& instanceId, bool enabled);
    
    /**
     * @brief Get total data usage
     */
    qint64 getTotalDataUsage(const QString& instanceId) const;
    
    /**
     * @brief Simulate data usage
     */
    bool simulateDataUsage(const QString& instanceId, const QString& packageName, 
                          qint64 wifiBytes, qint64 mobileBytes);
    
    // =========================================================================
    // App Standby
    // =========================================================================
    
    /**
     * @brief Set app standby bucket
     */
    bool setAppStandbyBucket(const QString& instanceId, const QString& packageName, 
                             AppStandbyBucket bucket);
    
    /**
     * @brief Get app standby bucket
     */
    AppStandbyBucket getAppStandbyBucket(const QString& instanceId, 
                                         const QString& packageName) const;
    
    /**
     * @brief Enable background limit
     */
    bool enableBackgroundLimit(const QString& instanceId, int maxBackgroundApps);
    
    /**
     * @brief Disable background limit
     */
    bool disableBackgroundLimit(const QString& instanceId);
    
    // =========================================================================
    // Battery Stats
    // =========================================================================
    
    /**
     * @brief Get battery statistics
     */
    BatteryStats getBatteryStats(const QString& instanceId) const;
    
    /**
     * @brief Simulate battery drain
     */
    bool simulateBatteryDrain(const QString& instanceId, int percent);
    
    /**
     * @brief Simulate charging
     */
    bool simulateCharging(const QString& instanceId, int targetPercent);
    
    /**
     * @brief Set battery temperature
     */
    bool setBatteryTemperature(const QString& instanceId, int temperatureCelsius);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get complete behavior state
     */
    DeviceBehaviorState getBehaviorState(const QString& instanceId) const;
    
    /**
     * @brief Get behavior info as JSON
     */
    QJsonObject getBehaviorInfoJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
    /**
     * @brief Apply all behaviors
     */
    bool applyAllBehaviors(const QString& instanceId);
    
private:
    DeviceBehaviorManager();
    static DeviceBehaviorManager* s_instance;
    
    // Helper methods
    QString powerProfileToString(PowerProfile profile) const;
    QString optimizationLevelToString(BatteryOptimizationLevel level) const;
    QString standbyBucketToString(AppStandbyBucket bucket) const;
    BatteryOptimizationLevel stringToOptimizationLevel(const QString& level) const;
    AppStandbyBucket stringToStandbyBucket(const QString& bucket) const;
    qint64 estimateBatterySavings(const QStringList& hibernatedApps) const;
    
    QMap<QString, DeviceBehaviorState> m_states;

};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_DEVICE_BEHAVIOR_MANAGER_H
