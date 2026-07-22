/**
 * @file NetworkRealismEnhancer.h
 * @brief Network Realism Enhancer - Wi-Fi Calling, VoLTE, CA, Dual SIM
 * @version 3.0.0
 * 
 * Provides enhanced network realism features:
 * - Wi-Fi Calling configuration and behavior
 * - VoLTE (Voice over LTE) simulation
 * - VoWiFi (Voice over Wi-Fi) simulation
 * - Carrier Aggregation (CA) information
 * - Dual SIM Dual Standby (DSDS) simulation
 * - Network band simulation
 * - Realistic signal strength patterns
 */

#pragma once

#ifndef VIRTUALPHONEPRO_NETWORK_REALISM_ENHANCER_H
#define VIRTUALPHONEPRO_NETWORK_REALISM_ENHANCER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include <QRandomGenerator>

namespace VirtualPhonePro {

// Network Technology Types
enum class NetworkTechnology {
    GSM_2G,
    UMTS_3G,
    LTE_4G,
    LTE_ADVANCED,
    NR_5G,
    NR_ADVANCED
};

// Wi-Fi Calling Status
enum class WiFiCallingStatus {
    DISABLED,
    ENABLED,
    PREFERRED,
    ONLY_WIFI
};

// VoLTE Status
enum class VoLTEStatus {
    DISABLED,
    ENABLED,
    UNAVAILABLE,
    TEMPORARILY_UNAVAILABLE
};

// SIM State
enum class SIMState {
    ABSENT,
    READY,
    LOCKED_PIN,
    LOCKED_PUK,
    NETWORK_LOCKED,
    DISABLED
};

// Carrier Aggregation Band Combination
struct CABandCombination {
    int primaryBand;
    int secondaryBand;
    int componentCarrierCount;
    int maxSpeedMbps;
    QString combinationName;
};

// Dual SIM Configuration
struct DualSIMConfig {
    bool isDualSIMEnabled;
    bool isDualSIMActive;
    int sim1SlotActive;
    int sim2SlotActive;
    bool isSIM1Enabled;
    bool isSIM2Enabled;
    QString sim1Operator;
    QString sim2Operator;
    QString sim1OperatorName;
    QString sim2OperatorName;
    QString sim1MCCMNC;
    QString sim2MCCMNC;
    SIMState sim1State;
    SIMState sim2State;
    bool isDualStandby;
    QString standbyType; // "dsds", "dsda", "srlte"
};

// Network Band Configuration
struct NetworkBandConfig {
    QList<int> lteBands;
    QList<int> nrBands;
    int primaryLteBand;
    int primaryNrBand;
    bool isBandAggregationSupported;
    bool isCAEnabled;
};

// Signal Strength Configuration
struct SignalStrengthConfig {
    int rsrp;       // Reference Signal Received Power (-140 to -44 dBm)
    int rsrq;       // Reference Signal Received Quality (-20 to -3 dB)
    int sinr;       // Signal to Interference plus Noise Ratio (0 to 40 dB)
    int signalBars; // 0-4 bars
    int asuLevel;   // Arbitrary Strength Unit (0-97, 99 for unknown)
    QString networkType;
    NetworkTechnology technology;
};

// Wi-Fi Calling Configuration
struct WiFiCallingConfig {
    bool isSupported;
    bool isEnabled;
    WiFiCallingStatus status;
    bool isProvisioned;
    bool isEmergencyOnly;
    QString preferredCallingMode; // "wifi_preferred", "cellular_preferred", "wifi_only"
    int wifiCallQuality;
    QString wifiCallingPackage;
};

// VoLTE Configuration
struct VoLTEConfig {
    bool isSupported;
    bool isEnabled;
    VoLTEStatus status;
    bool isProvisioned;
    bool isEmergencyEnabled;
    QString voltePackage;
    QString epsFallBack;
};

// Complete Network Realism State
struct NetworkRealismState {
    QString instanceId;
    
    // Technology
    NetworkTechnology currentTechnology;
    NetworkTechnology preferredTechnology;
    
    // Dual SIM
    DualSIMConfig dualSIM;
    
    // Bands
    NetworkBandConfig bands;
    
    // Signal
    SignalStrengthConfig signal;
    
    // Wi-Fi Calling
    WiFiCallingConfig wifiCalling;
    
    // VoLTE
    VoLTEConfig volte;
    
    // Carrier Aggregation
    QList<CABandCombination> caCombinations;
    CABandCombination currentCA;
    bool isCAConnected;
    
    // Additional
    bool isRoaming;
    bool isDataRoamingEnabled;
    int networkLoadPercent;
    QString networkCountryCode;
    QString simOperatorCode;
    QString simOperatorName;
};

class NetworkRealismEnhancer {
public:
    static NetworkRealismEnhancer& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure for single SIM
     */
    bool configureSingleSIM(const QString& instanceId, const QString& operatorCode, 
                            const QString& operatorName, NetworkTechnology tech);
    
    /**
     * @brief Configure for dual SIM
     */
    bool configureDualSIM(const QString& instanceId, const DualSIMConfig& config);
    
    /**
     * @brief Configure network bands
     */
    bool configureNetworkBands(const QString& instanceId, const NetworkBandConfig& bands);
    
    /**
     * @brief Apply configuration to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Wi-Fi Calling
    // =========================================================================
    
    /**
     * @brief Enable Wi-Fi Calling
     */
    bool enableWiFiCalling(const QString& instanceId);
    
    /**
     * @brief Disable Wi-Fi Calling
     */
    bool disableWiFiCalling(const QString& instanceId);
    
    /**
     * @brief Set Wi-Fi Calling mode
     */
    bool setWiFiCallingMode(const QString& instanceId, const QString& mode);
    
    /**
     * @brief Get Wi-Fi Calling status
     */
    WiFiCallingStatus getWiFiCallingStatus(const QString& instanceId) const;
    
    // =========================================================================
    // VoLTE
    // =========================================================================
    
    /**
     * @brief Enable VoLTE
     */
    bool enableVoLTE(const QString& instanceId);
    
    /**
     * @brief Disable VoLTE
     */
    bool disableVoLTE(const QString& instanceId);
    
    /**
     * @brief Get VoLTE status
     */
    VoLTEStatus getVoLTEStatus(const QString& instanceId) const;
    
    // =========================================================================
    // Dual SIM
    // =========================================================================
    
    /**
     * @brief Get dual SIM configuration
     */
    DualSIMConfig getDualSIMConfig(const QString& instanceId) const;
    
    /**
     * @brief Set SIM enabled state
     */
    bool setSIMEnabled(const QString& instanceId, int simSlot, bool enabled);
    
    /**
     * @brief Switch active SIM
     */
    bool switchActiveSIM(const QString& instanceId, int simSlot);
    
    /**
     * @brief Simulate SIM change
     */
    bool simulateSIMChange(const QString& instanceId, int simSlot, const QString& operatorCode,
                          const QString& operatorName);
    
    // =========================================================================
    // Carrier Aggregation
    // =========================================================================
    
    /**
     * @brief Enable Carrier Aggregation
     */
    bool enableCA(const QString& instanceId);
    
    /**
     * @brief Disable Carrier Aggregation
     */
    bool disableCA(const QString& instanceId);
    
    /**
     * @brief Set CA band combination
     */
    bool setCABandCombination(const QString& instanceId, const CABandCombination& ca);
    
    /**
     * @brief Get current CA status
     */
    bool isCAConnected(const QString& instanceId) const;
    
    /**
     * @brief Get available CA combinations
     */
    QList<CABandCombination> getAvailableCACombinations(const QString& instanceId) const;
    
    // =========================================================================
    // Signal Strength
    // =========================================================================
    
    /**
     * @brief Set signal strength
     */
    bool setSignalStrength(const QString& instanceId, int rsrp, int rsrq);
    
    /**
     * @brief Simulate signal fluctuation
     */
    bool simulateSignalFluctuation(const QString& instanceId, int range);
    
    /**
     * @brief Get current signal strength
     */
    SignalStrengthConfig getSignalStrength(const QString& instanceId) const;
    
    // =========================================================================
    // Network Technology
    // =========================================================================
    
    /**
     * @brief Set network technology (2G/3G/4G/5G)
     */
    bool setNetworkTechnology(const QString& instanceId, NetworkTechnology tech);
    
    /**
     * @brief Get current network technology
     */
    NetworkTechnology getNetworkTechnology(const QString& instanceId) const;
    
    /**
     * @brief Simulate network handover
     */
    bool simulateNetworkHandover(const QString& instanceId, NetworkTechnology targetTech);
    
    // =========================================================================
    // Roaming
    // =========================================================================
    
    /**
     * @brief Enable data roaming
     */
    bool enableDataRoaming(const QString& instanceId);
    
    /**
     * @brief Disable data roaming
     */
    bool disableDataRoaming(const QString& instanceId);
    
    /**
     * @brief Set roaming status
     */
    bool setRoaming(const QString& instanceId, bool isRoaming);
    
    /**
     * @brief Check if roaming
     */
    bool isRoaming(const QString& instanceId) const;
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get complete network state
     */
    NetworkRealismState getNetworkState(const QString& instanceId) const;
    
    /**
     * @brief Generate realistic network info
     */
    QJsonObject getNetworkInfoJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
    /**
     * @brief Apply all realism features
     */
    bool applyAllRealism(const QString& instanceId);
    
private:
    NetworkRealismEnhancer();
    static NetworkRealismEnhancer* s_instance;
    
    // Helper methods
    void initializeCACombinations();
    int calculateSignalBars(int rsrp) const;
    QString technologyToString(NetworkTechnology tech) const;
    NetworkTechnology stringToTechnology(const QString& tech) const;
    
    QMap<QString, NetworkRealismState> m_states;
    QList<CABandCombination> m_caCombinations;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_NETWORK_REALISM_ENHANCER_H
