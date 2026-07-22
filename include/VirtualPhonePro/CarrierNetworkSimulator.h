/**
 * @file CarrierNetworkSimulator.h
 * @brief Realistic Carrier & Network Simulation
 * @version 2.0.0
 * 
 * Provides realistic carrier information, signal strength, network type
 * transitions, and SIM card details for authentic device simulation.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_CARRIER_NETWORK_SIMULATOR_H
#define VIRTUALPHONEPRO_CARRIER_NETWORK_SIMULATOR_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// Network type
enum class NetworkType {
    GSM,        // 2G
    CDMA,       // 2G CDMA
    UMTS,       // 3G
    HSDPA,      // 3.5G
    LTE,        // 4G
    LTE_CA,     // 4G+ (Carrier Aggregation)
    NR,         // 5G
    NR_SA,      // 5G Standalone
    WIFI,
    UNKNOWN
};

// SIM operator type
enum class OperatorType {
    CARRIER,
    MVNO,
    VIRTUAL
};

// Roaming status
enum class RoamingStatus {
    HOME,
    ROAMING,
    INTERNATIONAL
};

// Signal strength
struct SignalStrength {
    int level;              // 0-4 (none, poor, fair, good, excellent)
    int dBm;                // Signal strength in dBm (-100 to -30)
    int asu;                // Arbitrary Strength Unit
    int levelPercent;       // 0-100%
    
    SignalStrength() : level(4), dBm(-70), asu(25), levelPercent(80) {}
};

// Complete carrier information
struct CarrierInfo {
    QString name;                    // Carrier display name (e.g., "T-Mobile")
    QString shortName;              // Short name (e.g., "TMO")
    QString numeric;                // Numeric MCC+MNC (e.g., "310260")
    QString mcc;                    // Mobile Country Code (3 digits)
    QString mnc;                    // Mobile Network Code (2-3 digits)
    QString iso;                    // Country ISO code (e.g., "US")
    QString country;                 // Country name
    QString simOperator;            // SIM operator name
    QString simCountry;             // SIM country
    QString networkOperator;        // Network operator name
    QString networkCountry;         // Network country
    QString simSerialNumber;        // ICCID
    QString imsi;                   // IMSI
    QString gsmBranch;              // GSM branch
    QString cdmaBranch;             // CDMA branch
    QString esn;                    // Electronic Serial Number
    QString meid;                   // Mobile Equipment Identifier
    
    OperatorType operatorType;
    bool isRoaming;
    bool isMultiSim;
    int simSlotCount;
    int activeSimSlot;
    
    SignalStrength signal;
    NetworkType networkType;
    
    // Voice mail
    int voiceMailCount;
    bool isVoiceMailReady;
    
    // Data state
    bool isDataEnabled;
    bool isMobileDataEnabled;
    bool isDataRoamingEnabled;
    
    // Timestamps
    qint64 networkRegisterTime;
    qint64 lastNetworkChangeTime;
};

// Data usage tracking
struct DataUsage {
    quint64 mobileRxBytes;
    quint64 mobileTxBytes;
    quint64 wifiRxBytes;
    quint64 wifiTxBytes;
    quint64 totalRxBytes;
    quint64 totalTxBytes;
    
    qint64 cycleStartTime;
    int cycleDays;
};

// Preset carriers
struct CarrierPreset {
    QString name;
    QString shortName;
    QString mcc;
    QString mnc;
    QString country;
    QString iso;
    NetworkType defaultNetwork;
};

class CarrierNetworkSimulator {
public:
    static CarrierNetworkSimulator& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure carrier for instance
     */
    bool configureCarrier(const QString& instanceId, const QString& carrierName, const QString& country = "US");
    
    /**
     * @brief Configure custom carrier
     */
    bool configureCustomCarrier(const QString& instanceId, const CarrierInfo& carrier);
    
    /**
     * @brief Get carrier info
     */
    CarrierInfo getCarrierInfo(const QString& instanceId) const;
    
    /**
     * @brief Apply carrier configuration to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Signal & Network
    // =========================================================================
    
    /**
     * @brief Set signal strength
     */
    bool setSignalStrength(const QString& instanceId, const SignalStrength& signal);
    
    /**
     * @brief Set network type
     */
    bool setNetworkType(const QString& instanceId, NetworkType type);
    
    /**
     * @brief Simulate network type transition
     */
    bool simulateNetworkTransition(const QString& instanceId, NetworkType targetType, int durationMs);
    
    /**
     * @brief Set WiFi state
     */
    bool setWifiState(const QString& instanceId, bool enabled, const QString& ssid = QString());
    
    /**
     * @brief Set airplane mode
     */
    bool setAirplaneMode(const QString& instanceId, bool enabled);
    
    // =========================================================================
    // Data Management
    // =========================================================================
    
    /**
     * @brief Enable/disable mobile data
     */
    bool setMobileDataEnabled(const QString& instanceId, bool enabled);
    
    /**
     * @brief Enable/disable data roaming
     */
    bool setDataRoamingEnabled(const QString& instanceId, bool enabled);
    
    /**
     * @brief Get data usage
     */
    DataUsage getDataUsage(const QString& instanceId) const;
    
    /**
     * @brief Reset data usage counters
     */
    bool resetDataUsage(const QString& instanceId);
    
    // =========================================================================
    // SIM Management
    // =========================================================================
    
    /**
     * @brief Set active SIM slot
     */
    bool setActiveSimSlot(const QString& instanceId, int slot);
    
    /**
     * @brief Set dual SIM configuration
     */
    bool configureDualSim(const QString& instanceId, const CarrierInfo& sim1, const CarrierInfo& sim2);
    
    // =========================================================================
    // Roaming
    // =========================================================================
    
    /**
     * @brief Set roaming status
     */
    bool setRoamingStatus(const QString& instanceId, RoamingStatus status);
    
    /**
     * @brief Simulate traveling (changes location and roaming)
     */
    bool simulateTravel(const QString& instanceId, const QString& country, int durationMs);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get all carrier properties for spoofing
     */
    QMap<QString, QString> getAllCarrierProperties(const QString& instanceId);
    
    /**
     * @brief Apply all carrier spoofing
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Reset carrier to default
     */
    bool resetCarrier(const QString& instanceId);
    
    /**
     * @brief Get list of available carrier presets
     */
    QList<CarrierPreset> getCarrierPresets() const;
    
    QString networkTypeToString(NetworkType type) const;
    
private:
    CarrierNetworkSimulator();
    static CarrierNetworkSimulator* s_instance;
    
    NetworkType stringToNetworkType(const QString& str) const;
    QString generateIMSI(const QString& mcc, const QString& mnc);
    QString generateICCID(const QString& mcc);
    SignalStrength dBmToSignalLevel(int dBm) const;
    
    QMap<QString, CarrierInfo> m_carrierStates;
    QMap<QString, DataUsage> m_dataUsage;
    QList<CarrierPreset> m_carrierPresets;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_CARRIER_NETWORK_SIMULATOR_H
