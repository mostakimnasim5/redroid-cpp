/**
 * @file AdvancedRealisticSimulation.h
 * @brief Advanced Realistic Phone Simulation - Master Header
 * @version 2.0.0
 * 
 * This header aggregates all advanced simulation modules for making
 * Android emulators appear as realistic physical devices.
 * 
 * Features include:
 * - Battery & Power Management
 * - Carrier & Network Simulation
 * - Screen State & Brightness Management
 * 
 * For authorized testing purposes only.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_ADVANCED_REALISTIC_SIMULATION_H
#define VIRTUALPHONEPRO_ADVANCED_REALISTIC_SIMULATION_H

#include "VirtualPhonePro/BatteryPowerManager.h"
#include "VirtualPhonePro/CarrierNetworkSimulator.h"
#include "VirtualPhonePro/ScreenStateManager.h"

namespace VirtualPhonePro {

/**
 * @brief Advanced Realistic Phone Simulator
 * 
 * Provides complete realistic phone simulation by coordinating
 * battery, carrier, and screen subsystems.
 */
class AdvancedRealisticSimulator {
public:
    static AdvancedRealisticSimulator& instance();
    
    /**
     * @brief Configure all systems for a device
     */
    bool configureDevice(const QString& instanceId, const QString& manufacturer, const QString& model);
    
    /**
     * @brief Apply all spoofing to instance
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Reset all systems
     */
    bool resetAll(const QString& instanceId);
    
    /**
     * @brief Get complete device state
     */
    QJsonObject getCompleteState(const QString& instanceId) const;
    
private:
    AdvancedRealisticSimulator() = default;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_ADVANCED_REALISTIC_SIMULATION_H
