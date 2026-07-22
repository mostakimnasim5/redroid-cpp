/**
 * @file BatteryPowerManager.h
 * @brief Realistic Battery & Power Management Simulation
 * @version 2.0.0
 * 
 * Provides realistic battery states, temperature, health, and charging patterns
 * to make emulated devices appear as real physical devices.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_BATTERY_POWER_MANAGER_H
#define VIRTUALPHONEPRO_BATTERY_POWER_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// Battery health status
enum class BatteryHealth {
    GOOD,
    EXCELLENT,
    OVERHEAT,
    DEAD,
    OVER_VOLTAGE,
    UNSPEC_FAILURE,
    COLD
};

// Battery plug state
enum class BatteryPlugState {
    UNPLUGGED,
    AC,
    USB,
    WIRELESS
};

// Battery technology
enum class BatteryTechnology {
    LiIon,
    LiPoly,
    NiMH,
    NiCd,
    LiFePO4
};

// Realistic battery data
struct BatteryState {
    // Basic properties
    int level;                      // 0-100 percentage
    int temperature;                // Temperature in centi-celsius (e.g., 350 = 35.0°C)
    int voltage;                    // Voltage in millivolts
    int currentNow;                 // Current in mA (negative = discharging)
    
    // Status
    bool isCharging;
    bool isFull;
    bool isOnline;
    
    // Health & Technology
    BatteryHealth health;
    BatteryTechnology technology;
    
    // Capacity
    int capacityMah;                // Design capacity in mAh
    int currentCapacityMah;         // Current capacity
    int levelCounter;               // Battery cycle count estimate
    
    // Plug state
    BatteryPlugState plugState;
    
    // Energy calculation
    int energyCounter;              // Energy consumed in mWh
    int chargeCounter;             // Charge cycles
    
    // Timestamp
    qint64 timestamp;
};

// Power profiles for different usage scenarios
struct PowerProfile {
    QString name;
    
    // Screen power consumption (mW)
    int screenPowerBright;
    int screenPowerMedium;
    int screenPowerDim;
    
    // CPU power consumption (mW)
    int cpuPowerIdle;
    int cpuPowerActive;
    int cpuPowerHot;
    
    // Network power (mW)
    int wifiPowerActive;
    int cellularPowerActive;
    
    // Battery capacity (mAh)
    int batteryCapacity;
    
    // Charging speed (mA)
    int chargingSpeedNormal;
    int chargingSpeedFast;
    int chargingSpeedWireless;
};

class BatteryPowerManager {
public:
    static BatteryPowerManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set battery profile for device model
     */
    bool configureForDevice(const QString& manufacturer, const QString& model);
    
    /**
     * @brief Get current power profile
     */
    PowerProfile getPowerProfile() const;
    
    /**
     * @brief Set custom power profile
     */
    void setPowerProfile(const PowerProfile& profile);
    
    // =========================================================================
    // Battery State Management
    // =========================================================================
    
    /**
     * @brief Set initial battery state
     */
    bool setBatteryState(const QString& instanceId, const BatteryState& state);
    
    /**
     * @brief Get current battery state
     */
    BatteryState getBatteryState(const QString& instanceId) const;
    
    /**
     * @brief Apply battery state to instance via ADB
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Charging Simulation
    // =========================================================================
    
    /**
     * @brief Start charging
     */
    bool startCharging(const QString& instanceId, BatteryPlugState plugType = BatteryPlugState::AC);
    
    /**
     * @brief Stop charging (unplug)
     */
    bool stopCharging(const QString& instanceId);
    
    /**
     * @brief Fast charge mode
     */
    bool enableFastCharging(const QString& instanceId);
    
    /**
     * @brief Wireless charging
     */
    bool enableWirelessCharging(const QString& instanceId);
    
    /**
     * @brief Simulate battery drain over time
     */
    bool simulateDrain(const QString& instanceId, int drainPercent, int durationMs);
    
    // =========================================================================
    // Temperature Simulation
    // =========================================================================
    
    /**
     * @brief Set battery temperature
     */
    bool setTemperature(const QString& instanceId, int temperatureCelsius);
    
    /**
     * @brief Simulate temperature changes
     */
    bool simulateTemperatureChange(const QString& instanceId, int targetTemp, int durationMs);
    
    /**
     * @brief Normal operating temperature range
     */
    bool setNormalTemperature(const QString& instanceId);
    
    /**
     * @brief Simulate overheating
     */
    bool simulateOverheating(const QString& instanceId, int durationMs);
    
    // =========================================================================
    // Battery Health
    // =========================================================================
    
    /**
     * @brief Set battery health status
     */
    bool setBatteryHealth(const QString& instanceId, BatteryHealth health);
    
    /**
     * @brief Generate realistic health based on device age
     */
    bool setBatteryHealthByAge(const QString& instanceId, int daysOld);
    
    // =========================================================================
    // Utility Methods
    // =========================================================================
    
    /**
     * @brief Get all battery properties for device info spoofing
     */
    QMap<QString, QString> getAllBatteryProperties(const QString& instanceId);
    
    /**
     * @brief Apply all battery spoofing at once
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Reset battery to default state
     */
    bool resetBattery(const QString& instanceId);
    
private:
    static BatteryPowerManager* s_instance;
    BatteryPowerManager();
    
    PowerProfile getDefaultProfile(const QString& manufacturer);
    int calculateTemperature(int baseTemp, bool isCharging, bool isHeavyUse);
    BatteryHealth calculateHealthFromAge(int daysOld);
    
    QMap<QString, BatteryState> m_batteryStates;
    QMap<QString, PowerProfile> m_powerProfiles;
    PowerProfile m_currentProfile;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_BATTERY_POWER_MANAGER_H
