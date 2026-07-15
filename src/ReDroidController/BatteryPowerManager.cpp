/**
 * @file BatteryPowerManager.cpp
 * @brief Realistic Battery & Power Management Implementation
 */

#include "VirtualPhonePro/BatteryPowerManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>

namespace VirtualPhonePro {

BatteryPowerManager* BatteryPowerManager::s_instance = nullptr;

BatteryPowerManager& BatteryPowerManager::instance() {
    if (!s_instance) {
        s_instance = new BatteryPowerManager();
    }
    return *s_instance;
}

BatteryPowerManager::BatteryPowerManager() {
    // Initialize with default Android profile
    m_currentProfile = getDefaultProfile("generic");
}

// ============================================================================
// Configuration
// ============================================================================

bool BatteryPowerManager::configureForDevice(const QString& manufacturer, const QString& model) {
    qDebug() << "Configuring battery for:" << manufacturer << model;
    
    m_currentProfile = getDefaultProfile(manufacturer);
    return true;
}

PowerProfile BatteryPowerManager::getPowerProfile() const {
    return m_currentProfile;
}

void BatteryPowerManager::setPowerProfile(const PowerProfile& profile) {
    m_currentProfile = profile;
    qDebug() << "Power profile set:" << profile.name;
}

// ============================================================================
// Battery State Management
// ============================================================================

bool BatteryPowerManager::setBatteryState(const QString& instanceId, const BatteryState& state) {
    BatteryState newState = state;
    newState.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_batteryStates[instanceId] = newState;
    
    qDebug() << "Battery state set for" << instanceId 
             << "- Level:" << state.level << "%"
             << "Temp:" << (state.temperature / 100.0) << "°C";
    
    return applyToInstance(instanceId);
}

BatteryState BatteryPowerManager::getBatteryState(const QString& instanceId) const {
    if (m_batteryStates.contains(instanceId)) {
        return m_batteryStates[instanceId];
    }
    
    // Return default state
    BatteryState defaultState;
    defaultState.level = 80;
    defaultState.temperature = 250; // 25°C
    defaultState.voltage = 4200;
    defaultState.currentNow = -500;
    defaultState.isCharging = false;
    defaultState.isFull = false;
    defaultState.isOnline = true;
    defaultState.health = BatteryHealth::GOOD;
    defaultState.technology = BatteryTechnology::Li-ion;
    defaultState.capacityMah = 5000;
    defaultState.plugState = BatteryPlugState::UNPLUGGED;
    defaultState.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return defaultState;
}

bool BatteryPowerManager::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (!m_batteryStates.contains(instanceId)) {
        qWarning() << "No battery state for instance:" << instanceId;
        return false;
    }
    
    BatteryState state = m_batteryStates[instanceId];
    
    // Build property commands
    QStringList commands = {
        // Battery status
        QString("dumpsys battery set level %1").arg(state.level),
        QString("dumpsys battery set temperature %1").arg(state.temperature),
        QString("dumpsys battery set voltage %1").arg(state.voltage),
        QString("dumpsys battery set usb online %1").arg(state.isOnline ? 1 : 0),
        
        // Charging status
        QString("dumpsys battery set status %1").arg(state.isCharging ? 2 : 3),
        QString("dumpsys battery set plugged %1").arg(static_cast<int>(state.plugState)),
        
        // Health
        QString("dumpsys battery set health %1").arg(static_cast<int>(state.health)),
        
        // Technology
        QString("dumpsys battery set technology %1").arg(
            state.technology == BatteryTechnology::Li-ion ? "Li-ion" :
            state.technology == BatteryTechnology::Li-poly ? "Li-poly" : "Li-ion"
        ),
        
        // AC powered status
        QString("dumpsys battery set ac %1").arg(
            state.plugState == BatteryPlugState::AC ? 1 : 0
        ),
        
        // USB powered status
        QString("dumpsys battery set usb %1").arg(
            state.plugState == BatteryPlugState::USB ? 1 : 0
        ),
        
        // Wireless powered status
        QString("dumpsys battery set wireless %1").arg(
            state.plugState == BatteryPlugState::WIRELESS ? 1 : 0
        ),
    };
    
    // Also set system properties for app compatibility
    QStringList propCommands = {
        // Battery properties
        QString("setprop sys.battery.level %1").arg(state.level),
        QString("setprop sys.battery.temp %1").arg(state.temperature),
        QString("setprop sys.battery.voltage %1").arg(state.voltage),
        QString("setprop sys.battery.charge_counter %1").arg(state.currentCapacityMah),
        QString("setprop sys.battery.current_now %1").arg(state.currentNow),
        QString("setprop sys.battery.health %1").arg(static_cast<int>(state.health)),
        
        // Power state
        QString("setprop sys.power管理水平 %1").arg(state.isCharging ? "charging" : "discharging"),
        QString("setprop persist.sys.battery.capacity %1").arg(state.level),
        
        // Charging
        QString("setprop persist.sys.battery.charging %1").arg(state.isCharging ? "1" : "0"),
    };
    
    // Execute all commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    for (const QString& cmd : propCommands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Battery state applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Charging Simulation
// ============================================================================

bool BatteryPowerManager::startCharging(const QString& instanceId, BatteryPlugState plugType) {
    BatteryState state = getBatteryState(instanceId);
    state.isCharging = true;
    state.isOnline = true;
    state.plugState = plugType;
    
    // Set charging current based on plug type
    switch (plugType) {
        case BatteryPlugState::AC:
            state.currentNow = -m_currentProfile.chargingSpeedNormal;
            break;
        case BatteryPlugState::USB:
            state.currentNow = -500; // USB typically 500mA
            break;
        case BatteryPlugState::WIRELESS:
            state.currentNow = -1000;
            break;
        default:
            state.currentNow = -1000;
            break;
    }
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::stopCharging(const QString& instanceId) {
    BatteryState state = getBatteryState(instanceId);
    state.isCharging = false;
    state.plugState = BatteryPlugState::UNPLUGGED;
    state.currentNow = -500; // Typical discharge rate
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::enableFastCharging(const QString& instanceId) {
    BatteryState state = getBatteryState(instanceId);
    state.isCharging = true;
    state.plugState = BatteryPlugState::AC;
    state.currentNow = -m_currentProfile.chargingSpeedFast;
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::enableWirelessCharging(const QString& instanceId) {
    BatteryState state = getBatteryState(instanceId);
    state.isCharging = true;
    state.plugState = BatteryPlugState::WIRELESS;
    state.currentNow = -m_currentProfile.chargingSpeedWireless;
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::simulateDrain(const QString& instanceId, int drainPercent, int durationMs) {
    BatteryState state = getBatteryState(instanceId);
    int newLevel = qMax(0, state.level - drainPercent);
    
    // Calculate current based on drain rate
    int estimatedCurrent = (drainPercent * state.capacityMah * 1000) / (durationMs / 1000);
    state.currentNow = estimatedCurrent;
    
    state.level = newLevel;
    
    return setBatteryState(instanceId, state);
}

// ============================================================================
// Temperature Simulation
// ============================================================================

bool BatteryPowerManager::setTemperature(const QString& instanceId, int temperatureCelsius) {
    BatteryState state = getBatteryState(instanceId);
    state.temperature = temperatureCelsius * 100; // Convert to centi-celsius
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::simulateTemperatureChange(const QString& instanceId, int targetTemp, int durationMs) {
    // Calculate step size
    BatteryState current = getBatteryState(instanceId);
    int currentTempC = current.temperature / 100;
    int steps = qMax(1, durationMs / 1000);
    int tempStep = (targetTemp - currentTempC) / steps;
    
    for (int i = 0; i < steps; i++) {
        currentTempC += tempStep;
        setTemperature(instanceId, currentTempC);
        
        // Small delay between updates
        QThread::msleep(1000);
    }
    
    // Ensure final temperature is exact
    return setTemperature(instanceId, targetTemp);
}

bool BatteryPowerManager::setNormalTemperature(const QString& instanceId) {
    // Normal range: 20-35°C
    int normalTemp = QRandomGenerator::global()->bounded(200, 350);
    return setTemperature(instanceId, normalTemp);
}

bool BatteryPowerManager::simulateOverheating(const QString& instanceId, int durationMs) {
    // Set high temperature
    setTemperature(instanceId, 45);
    
    // Keep overheating for duration
    QThread::msleep(durationMs);
    
    // Cool down gradually
    return setNormalTemperature(instanceId);
}

// ============================================================================
// Battery Health
// ============================================================================

bool BatteryPowerManager::setBatteryHealth(const QString& instanceId, BatteryHealth health) {
    BatteryState state = getBatteryState(instanceId);
    state.health = health;
    
    return setBatteryState(instanceId, state);
}

bool BatteryPowerManager::setBatteryHealthByAge(const QString& instanceId, int daysOld) {
    BatteryState state = getBatteryState(instanceId);
    state.health = calculateHealthFromAge(daysOld);
    
    // Adjust capacity based on health
    if (state.health == BatteryHealth::GOOD) {
        state.currentCapacityMah = state.capacityMah;
    } else if (state.health == BatteryHealth::EXCELLENT) {
        state.currentCapacityMah = state.capacityMah;
    } else {
        // Reduce capacity for degraded batteries
        int degradation = qMin(daysOld / 100, 30); // Max 30% degradation
        state.currentCapacityMah = state.capacityMah * (100 - degradation) / 100;
    }
    
    return setBatteryState(instanceId, state);
}

// ============================================================================
// Utility Methods
// ============================================================================

QMap<QString, QString> BatteryPowerManager::getAllBatteryProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    BatteryState state = getBatteryState(instanceId);
    
    // Battery service properties
    props["battery.level"] = QString::number(state.level);
    props["battery.level.scale"] = "100";
    props["battery.voltage"] = QString::number(state.voltage);
    props["battery.temperature"] = QString::number(state.temperature);
    props["battery.technology"] = state.technology == BatteryTechnology::Li-ion ? "Li-ion" : "Li-poly";
    
    // Status
    props["battery.status"] = state.isCharging ? "charging" : "discharging";
    props["battery.health"] = state.health == BatteryHealth::GOOD ? "good" : "excellent";
    props["battery.plugged"] = QString::number(static_cast<int>(state.plugState));
    
    // Power supply properties
    props["power.battery.level"] = QString::number(state.level);
    props["power.battery.status"] = state.isCharging ? "charging" : "not-charging";
    props["power.battery.health"] = "good";
    props["power.battery.present"] = "true";
    props["power.battery.capacity"] = QString::number(state.level);
    props["power.battery.voltage_now"] = QString::number(state.voltage * 1000);
    props["power.battery.current_now"] = QString::number(state.currentNow);
    props["power.battery.charge_counter"] = QString::number(state.currentCapacityMah);
    props["power.battery.energy_counter"] = QString::number(state.energyCounter);
    props["power.battery.temp"] = QString::number(state.temperature / 100.0);
    
    // AC/USB/Wireless
    props["power.ac.online"] = state.plugState == BatteryPlugState::AC ? "1" : "0";
    props["power.usb.online"] = state.plugState == BatteryPlugState::USB ? "1" : "0";
    props["power.wireless.online"] = state.plugState == BatteryPlugState::WIRELESS ? "1" : "0";
    
    return props;
}

bool BatteryPowerManager::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

bool BatteryPowerManager::resetBattery(const QString& instanceId) {
    BatteryState defaultState;
    defaultState.level = 80;
    defaultState.temperature = 250;
    defaultState.voltage = 4200;
    defaultState.currentNow = -500;
    defaultState.isCharging = false;
    defaultState.isFull = false;
    defaultState.isOnline = true;
    defaultState.health = BatteryHealth::GOOD;
    defaultState.technology = BatteryTechnology::Li-ion;
    defaultState.capacityMah = 5000;
    defaultState.currentCapacityMah = 5000;
    defaultState.plugState = BatteryPlugState::UNPLUGGED;
    
    return setBatteryState(instanceId, defaultState);
}

// ============================================================================
// Private Helpers
// ============================================================================

PowerProfile BatteryPowerManager::getDefaultProfile(const QString& manufacturer) {
    PowerProfile profile;
    profile.name = manufacturer;
    
    if (manufacturer.contains("samsung", Qt::CaseInsensitive)) {
        // Samsung typically has 5000mAh batteries with 45W charging
        profile.batteryCapacity = 5000;
        profile.chargingSpeedNormal = 2100;
        profile.chargingSpeedFast = 4500;
        profile.chargingSpeedWireless = 1500;
        profile.screenPowerBright = 1200;
        profile.screenPowerMedium = 800;
        profile.screenPowerDim = 400;
        profile.cpuPowerIdle = 50;
        profile.cpuPowerActive = 3000;
        profile.cpuPowerHot = 5000;
        profile.wifiPowerActive = 300;
        profile.cellularPowerActive = 500;
    } else if (manufacturer.contains("google", Qt::CaseInsensitive) || 
               manufacturer.contains("pixel", Qt::CaseInsensitive)) {
        // Pixel devices typically have 5000mAh with 30W charging
        profile.batteryCapacity = 5000;
        profile.chargingSpeedNormal = 1800;
        profile.chargingSpeedFast = 3000;
        profile.chargingSpeedWireless = 1200;
        profile.screenPowerBright = 1000;
        profile.screenPowerMedium = 700;
        profile.screenPowerDim = 350;
        profile.cpuPowerIdle = 40;
        profile.cpuPowerActive = 2800;
        profile.cpuPowerHot = 4500;
        profile.wifiPowerActive = 250;
        profile.cellularPowerActive = 450;
    } else if (manufacturer.contains("xiaomi", Qt::CaseInsensitive) || 
               manufacturer.contains("redmi", Qt::CaseInsensitive)) {
        // Xiaomi with 120W fast charging
        profile.batteryCapacity = 5000;
        profile.chargingSpeedNormal = 2000;
        profile.chargingSpeedFast = 12000;
        profile.chargingSpeedWireless = 5000;
        profile.screenPowerBright = 1100;
        profile.screenPowerMedium = 750;
        profile.screenPowerDim = 380;
        profile.cpuPowerIdle = 45;
        profile.cpuPowerActive = 2900;
        profile.cpuPowerHot = 4800;
        profile.wifiPowerActive = 280;
        profile.cellularPowerActive = 480;
    } else {
        // Generic Android device
        profile.batteryCapacity = 4500;
        profile.chargingSpeedNormal = 1500;
        profile.chargingSpeedFast = 3000;
        profile.chargingSpeedWireless = 1000;
        profile.screenPowerBright = 1000;
        profile.screenPowerMedium = 700;
        profile.screenPowerDim = 350;
        profile.cpuPowerIdle = 50;
        profile.cpuPowerActive = 2500;
        profile.cpuPowerHot = 4000;
        profile.wifiPowerActive = 300;
        profile.cellularPowerActive = 400;
    }
    
    return profile;
}

int BatteryPowerManager::calculateTemperature(int baseTemp, bool isCharging, bool isHeavyUse) {
    int temp = baseTemp;
    
    if (isCharging) {
        temp += QRandomGenerator::global()->bounded(50, 150); // 5-15°C increase
    }
    
    if (isHeavyUse) {
        temp += QRandomGenerator::global()->bounded(30, 80); // 3-8°C increase
    }
    
    // Add slight randomness
    temp += QRandomGenerator::global()->bounded(-20, 20);
    
    return qBound(200, temp, 500); // Keep between 20-50°C
}

BatteryHealth BatteryPowerManager::calculateHealthFromAge(int daysOld) {
    if (daysOld < 90) {
        return BatteryHealth::EXCELLENT;
    } else if (daysOld < 365) {
        return BatteryHealth::GOOD;
    } else if (daysOld < 730) {
        return BatteryHealth::GOOD;
    } else {
        // Older devices might show slight degradation
        int rand = QRandomGenerator::global()->bounded(100);
        if (rand < 70) {
            return BatteryHealth::GOOD;
        } else {
            return BatteryHealth::COLD; // Could represent slight capacity loss
        }
    }
}

} // namespace VirtualPhonePro
