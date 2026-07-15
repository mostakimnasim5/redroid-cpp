/**
 * @file AdvancedRealisticSimulation.cpp
 * @brief Advanced Realistic Phone Simulator Implementation
 */

#include "VirtualPhonePro/AdvancedRealisticSimulation.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

AdvancedRealisticSimulator* AdvancedRealisticSimulator::s_instance = nullptr;

AdvancedRealisticSimulator& AdvancedRealisticSimulator::instance() {
    if (!s_instance) {
        s_instance = new AdvancedRealisticSimulator();
    }
    return *s_instance;
}

bool AdvancedRealisticSimulator::configureDevice(const QString& instanceId, 
                                                 const QString& manufacturer, 
                                                 const QString& model) {
    qDebug() << "Configuring advanced simulation for:" << manufacturer << model;
    
    // Configure battery
    BatteryPowerManager::instance().configureForDevice(manufacturer, model);
    BatteryPowerManager::instance().resetBattery(instanceId);
    
    // Configure carrier
    CarrierNetworkSimulator::instance().configureCarrier(instanceId, "T-Mobile", "US");
    
    // Configure screen
    ScreenStateManager::instance().configureForDevice(manufacturer, model);
    ScreenStateManager::instance().resetScreen(instanceId);
    
    return applyAllSpoofing(instanceId);
}

bool AdvancedRealisticSimulator::applyAllSpoofing(const QString& instanceId) {
    bool success = true;
    
    success &= BatteryPowerManager::instance().applyAllSpoofing(instanceId);
    success &= CarrierNetworkSimulator::instance().applyAllSpoofing(instanceId);
    success &= ScreenStateManager::instance().applyAllSpoofing(instanceId);
    
    qDebug() << "Applied all spoofing to instance:" << instanceId 
             << "- Success:" << success;
    
    return success;
}

bool AdvancedRealisticSimulator::resetAll(const QString& instanceId) {
    BatteryPowerManager::instance().resetBattery(instanceId);
    CarrierNetworkSimulator::instance().resetCarrier(instanceId);
    ScreenStateManager::instance().resetScreen(instanceId);
    
    return applyAllSpoofing(instanceId);
}

QJsonObject AdvancedRealisticSimulator::getCompleteState(const QString& instanceId) const {
    QJsonObject state;
    
    // Battery state
    BatteryState battery = BatteryPowerManager::instance().getBatteryState(instanceId);
    state["battery_level"] = battery.level;
    state["battery_temperature"] = battery.temperature / 100.0;
    state["battery_health"] = static_cast<int>(battery.health);
    state["is_charging"] = battery.isCharging;
    
    // Carrier state
    CarrierInfo carrier = CarrierNetworkSimulator::instance().getCarrierInfo(instanceId);
    state["carrier_name"] = carrier.name;
    state["network_type"] = CarrierNetworkSimulator::instance()
        .networkTypeToString(carrier.networkType);
    state["signal_level"] = carrier.signal.level;
    state["is_roaming"] = carrier.isRoaming;
    
    // Screen state
    ScreenInfo screen = ScreenStateManager::instance().getScreenInfo(instanceId);
    state["screen_on"] = screen.isOn;
    state["brightness"] = screen.brightness;
    state["refresh_rate"] = screen.refreshRate;
    state["is_auto_brightness"] = screen.isAutoBrightness;
    
    return state;
}

} // namespace VirtualPhonePro
