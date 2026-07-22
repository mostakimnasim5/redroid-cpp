/**
 * @file CarrierNetworkSimulator.cpp
 * @brief Realistic Carrier & Network Simulation Implementation
 */

#include "VirtualPhonePro/CarrierNetworkSimulator.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QThread>
#include <QRandomGenerator>

namespace VirtualPhonePro {

CarrierNetworkSimulator* CarrierNetworkSimulator::s_instance = nullptr;

CarrierNetworkSimulator& CarrierNetworkSimulator::instance() {
    if (!s_instance) {
        s_instance = new CarrierNetworkSimulator();
    }
    return *s_instance;
}

CarrierNetworkSimulator::CarrierNetworkSimulator() {
    // Initialize carrier presets
    m_carrierPresets = {
        // US Carriers
        {"T-Mobile", "TMO", "310", "260", "United States", "US", NetworkType::NR},
        {"AT&T", "ATT", "310", "410", "United States", "US", NetworkType::LTE},
        {"Verizon", "VZ", "311", "480", "United States", "US", NetworkType::NR},
        {"Sprint", "SPR", "310", "120", "United States", "US", NetworkType::LTE},
        {"US Cellular", "USC", "311", "580", "United States", "US", NetworkType::LTE},
        
        // UK Carriers
        {"EE", "EE", "234", "33", "United Kingdom", "GB", NetworkType::NR},
        {"O2", "O2", "234", "10", "United Kingdom", "GB", NetworkType::LTE},
        {"Vodafone UK", "VOD", "234", "15", "United Kingdom", "GB", NetworkType::LTE},
        {"Three UK", "THREE", "234", "20", "United Kingdom", "GB", NetworkType::LTE},
        
        // Europe
        {"Deutsche Telekom", "DT", "262", "01", "Germany", "DE", NetworkType::NR},
        {"Vodafone DE", "VFD", "262", "02", "Germany", "DE", NetworkType::LTE},
        {"Orange FR", "ORA", "208", "01", "France", "FR", NetworkType::LTE},
        {"Bouygues", "BYG", "208", "20", "France", "FR", NetworkType::LTE},
        {"Vodafone ES", "VFE", "214", "01", "Spain", "ES", NetworkType::LTE},
        {"Movistar", "MOV", "214", "03", "Spain", "ES", NetworkType::LTE},
        
        // Asia
        {"Jio", "JIO", "405", "800", "India", "IN", NetworkType::LTE},
        {"Airtel", "AIR", "404", "10", "India", "IN", NetworkType::LTE},
        {"SoftBank", "SBT", "440", "20", "Japan", "JP", NetworkType::NR},
        {"NTT DOCOMO", "DCM", "440", "10", "Japan", "JP", NetworkType::NR},
        {"SK Telecom", "SKT", "450", "05", "South Korea", "KR", NetworkType::NR},
        {"KT", "KT", "450", "08", "South Korea", "KR", NetworkType::NR},
        {"China Mobile", "CMCC", "460", "00", "China", "CN", NetworkType::NR},
        {"China Unicom", "CUC", "460", "01", "China", "CN", NetworkType::LTE},
        
        // Middle East
        {"Etisalat UAE", "etisalat", "424", "02", "UAE", "AE", NetworkType::NR},
        {"du", "du", "424", "03", "UAE", "AE", NetworkType::LTE},
        {"STC", "STC", "420", "01", "Saudi Arabia", "SA", NetworkType::NR},
    };
}

// ============================================================================
// Configuration
// ============================================================================

bool CarrierNetworkSimulator::configureCarrier(const QString& instanceId, const QString& carrierName, const QString& country) {
    // Find matching carrier preset
    CarrierPreset matched;
    bool found = false;
    
    for (const auto& preset : m_carrierPresets) {
        if (preset.name.contains(carrierName, Qt::CaseInsensitive) ||
            preset.shortName.contains(carrierName, Qt::CaseInsensitive)) {
            matched = preset;
            if (country.isEmpty() || preset.iso == country) {
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        qWarning() << "Carrier not found:" << carrierName;
        // Create generic carrier
        matched.name = carrierName;
        matched.shortName = carrierName.toUpper().left(3);
        matched.mcc = "310";
        matched.mnc = "000";
        matched.iso = country.isEmpty() ? "US" : country;
        matched.defaultNetwork = NetworkType::LTE;
    }
    
    CarrierInfo info;
    info.name = matched.name;
    info.shortName = matched.shortName;
    info.mcc = matched.mcc;
    info.mnc = matched.mnc;
    info.numeric = matched.mcc + matched.mnc;
    info.iso = matched.iso;
    info.simOperator = matched.name;
    info.simCountry = matched.iso;
    info.networkOperator = matched.name;
    info.networkCountry = matched.iso;
    info.iso = matched.iso;
    info.country = matched.country;
    info.imsi = generateIMSI(matched.mcc, matched.mnc);
    info.simSerialNumber = generateICCID(matched.mcc);
    info.networkType = matched.defaultNetwork;
    info.operatorType = OperatorType::CARRIER;
    info.isRoaming = false;
    info.isMultiSim = false;
    info.simSlotCount = 1;
    info.activeSimSlot = 0;
    info.signal.level = 4;
    info.signal.dBm = -65;
    info.signal.asu = 25;
    info.signal.levelPercent = 85;
    info.isDataEnabled = true;
    info.isMobileDataEnabled = true;
    info.isDataRoamingEnabled = false;
    info.voiceMailCount = 0;
    info.isVoiceMailReady = true;
    info.networkRegisterTime = QDateTime::currentMSecsSinceEpoch();
    info.lastNetworkChangeTime = QDateTime::currentMSecsSinceEpoch();
    
    m_carrierStates[instanceId] = info;
    
    qDebug() << "Carrier configured:" << matched.name << "for instance:" << instanceId;
    return applyToInstance(instanceId);
}

bool CarrierNetworkSimulator::configureCustomCarrier(const QString& instanceId, const CarrierInfo& carrier) {
    m_carrierStates[instanceId] = carrier;
    return applyToInstance(instanceId);
}

CarrierInfo CarrierNetworkSimulator::getCarrierInfo(const QString& instanceId) const {
    if (m_carrierStates.contains(instanceId)) {
        return m_carrierStates[instanceId];
    }
    
    // Return default
    CarrierInfo defaultInfo;
    defaultInfo.name = "Android";
    defaultInfo.shortName = "AND";
    defaultInfo.mcc = "310";
    defaultInfo.mnc = "260";
    defaultInfo.numeric = "310260";
    defaultInfo.iso = "US";
    defaultInfo.country = "United States";
    defaultInfo.networkType = NetworkType::LTE;
    defaultInfo.operatorType = OperatorType::CARRIER;
    defaultInfo.isRoaming = false;
    defaultInfo.signal.level = 4;
    defaultInfo.signal.dBm = -70;
    defaultInfo.isDataEnabled = true;
    defaultInfo.isMobileDataEnabled = true;
    return defaultInfo;
}

bool CarrierNetworkSimulator::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo info = m_carrierStates[instanceId];
    
    QStringList commands = {
        // SIM operator
        QString("setprop gsm.sim.operator.numeric %1").arg(info.numeric),
        QString("setprop gsm.sim.operator.alpha %1").arg(info.simOperator),
        QString("setprop gsm.sim.operator.iso-country %1").arg(info.iso),
        
        // Network operator
        QString("setprop gsm.operator.numeric %1").arg(info.numeric),
        QString("setprop gsm.operator.alpha %1").arg(info.networkOperator),
        QString("setprop gsm.operator.iso-country %1").arg(info.iso),
        
        // SIM state
        "setprop gsm.sim.state READY",
        "setprop gsm.sim.present true",
        QString("setprop gsm.sim.num %1").arg(info.simSlotCount),
        
        // IMSI
        QString("setprop persist.radio.simmanager.ims %1").arg(info.imsi),
        QString("setprop gsm.sim.imsi %1").arg(info.imsi),
        
        // ICCID
        QString("setprop persist.radio.iccid %1").arg(info.simSerialNumber),
        QString("setprop gsm.sim.serial %1").arg(info.simSerialNumber),
        
        // Network type
        QString("setprop gsm.network.type %1").arg(networkTypeToString(info.networkType)),
        QString("setprop ro.telephony.default_network %1").arg(
            info.networkType == NetworkType::NR ? "33" :
            info.networkType == NetworkType::LTE ? "9" : "0"
        ),
        
        // Roaming
        QString("setprop persist.radio.roaming %1").arg(info.isRoaming ? "true" : "false"),
        QString("setprop gsm.operator.isroaming %1").arg(info.isRoaming ? "true" : "false"),
        
        // Signal
        QString("setprop gsm.signal.strength %1").arg(info.signal.dBm),
        QString("setprop gsm.signal.level %1").arg(info.signal.level),
        
        // Data state
        "settings put global mobile_data 1",
        "settings put global data_roaming 0",
        
        // Voicemail
        "settings put secure voicemail_number *86",
        
        // Country
        QString("setprop persist.sys.country %1").arg(info.iso.toLower()),
        QString("setprop ro.product.locale.region %1").arg(info.iso),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Also register to network
    ctrl.executeShell(instanceId, 
        QString("service call iphonesubinfo 15 i32 1 s16 %1").arg(info.imsi));
    
    qDebug() << "Carrier applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Signal & Network
// ============================================================================

bool CarrierNetworkSimulator::setSignalStrength(const QString& instanceId, const SignalStrength& signal) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.signal = signal;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("setprop gsm.signal.strength %1").arg(signal.dBm),
        QString("setprop gsm.signal.level %1").arg(signal.level),
        QString("setprop gsm.current.signal.strength %1").arg(signal.dBm),
        QString("setprop persist.radio.signal.level %1").arg(signal.level),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool CarrierNetworkSimulator::setNetworkType(const QString& instanceId, NetworkType type) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.networkType = type;
    info.lastNetworkChangeTime = QDateTime::currentMSecsSinceEpoch();
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString networkStr = networkTypeToString(type);
    
    QStringList commands = {
        QString("setprop gsm.network.type %1").arg(networkStr),
        QString("setprop persist.telephony.network.type %1").arg(networkStr),
        QString("setprop ro.telephony.suggested_network_type %1").arg(networkStr),
    };
    
    // Set network preference based on type
    QString networkPref;
    switch (type) {
        case NetworkType::NR:
        case NetworkType::NR_SA:
            networkPref = "33"; // 5G preferred
            break;
        case NetworkType::LTE_CA:
        case NetworkType::LTE:
            networkPref = "9"; // LTE
            break;
        case NetworkType::HSDPA:
        case NetworkType::UMTS:
            networkPref = "3"; // 3G
            break;
        default:
            networkPref = "0"; // GSM
            break;
    }
    
    commands.append(QString("setprop ro.telephony.default_network %1").arg(networkPref));
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool CarrierNetworkSimulator::simulateNetworkTransition(const QString& instanceId, NetworkType targetType, int durationMs) {
    // Simulate network type changes (e.g., 4G -> 5G when entering coverage)
    QList<NetworkType> transitions;
    
    if (targetType == NetworkType::NR) {
        transitions = {NetworkType::LTE, NetworkType::LTE_CA, NetworkType::NR};
    } else if (targetType == NetworkType::LTE) {
        transitions = {NetworkType::UMTS, NetworkType::LTE};
    }
    
    int stepDuration = durationMs / transitions.size();
    
    for (const auto& type : transitions) {
        setNetworkType(instanceId, type);
        QThread::msleep(stepDuration);
    }
    
    return true;
}

bool CarrierNetworkSimulator::setWifiState(const QString& instanceId, bool enabled, const QString& ssid) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands;
    
    if (enabled) {
        commands = {
            "svc wifi enable",
            QString("settings put global wifi_on 1")
        };
        
        if (!ssid.isEmpty()) {
            commands.append(QString("settings put global wifi_ssid \"%1\"").arg(ssid));
        }
    } else {
        commands = {
            "svc wifi disable",
            "settings put global wifi_on 0"
        };
    }
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool CarrierNetworkSimulator::setAirplaneMode(const QString& instanceId, bool enabled) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("settings put global airplane_mode_on %1").arg(enabled ? 1 : 0),
        QString("settings put global two_ground_on_normal 0"),
    };
    
    if (enabled) {
        commands.append("svc power stayon-true");
    }
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Broadcast airplane mode change
    ctrl.executeShell(instanceId, 
        enabled ? "am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true"
                : "am broadcast -a android.intent.action.AIRPLANE_MODE --ez state false");
    
    return true;
}

// ============================================================================
// Data Management
// ============================================================================

bool CarrierNetworkSimulator::setMobileDataEnabled(const QString& instanceId, bool enabled) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.isMobileDataEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, 
        QString("settings put global mobile_data %1").arg(enabled ? 1 : 0));
    
    return true;
}

bool CarrierNetworkSimulator::setDataRoamingEnabled(const QString& instanceId, bool enabled) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.isDataRoamingEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId,
        QString("settings put global data_roaming %1").arg(enabled ? 1 : 0));
    
    return true;
}

DataUsage CarrierNetworkSimulator::getDataUsage(const QString& instanceId) const {
    if (m_dataUsage.contains(instanceId)) {
        return m_dataUsage[instanceId];
    }
    
    DataUsage usage;
    return usage;
}

bool CarrierNetworkSimulator::resetDataUsage(const QString& instanceId) {
    DataUsage usage;
    usage.cycleStartTime = QDateTime::currentMSecsSinceEpoch();
    usage.cycleDays = 30;
    m_dataUsage[instanceId] = usage;
    return true;
}

// ============================================================================
// SIM Management
// ============================================================================

bool CarrierNetworkSimulator::setActiveSimSlot(const QString& instanceId, int slot) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.activeSimSlot = slot;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId,
        QString("setprop persist.radio.multisim.config %1").arg(
            info.isMultiSim ? "dsds" : "ss"
        ));
    
    return true;
}

bool CarrierNetworkSimulator::configureDualSim(const QString& instanceId, const CarrierInfo& sim1, const CarrierInfo& sim2) {
    CarrierInfo info = sim1;
    info.isMultiSim = true;
    info.simSlotCount = 2;
    
    m_carrierStates[instanceId] = info;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        "setprop persist.radio.multisim.config dsds",
        QString("setprop gsm.sim.operator.alpha.1 %1").arg(sim1.name),
        QString("setprop gsm.sim.operator.alpha.2 %1").arg(sim2.name),
        QString("setprop gsm.sim.operator.numeric.1 %1").arg(sim1.numeric),
        QString("setprop gsm.sim.operator.numeric.2 %1").arg(sim2.numeric),
        "settings put global multi_sim_voice_call 1",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

// ============================================================================
// Roaming
// ============================================================================

bool CarrierNetworkSimulator::setRoamingStatus(const QString& instanceId, RoamingStatus status) {
    if (!m_carrierStates.contains(instanceId)) {
        return false;
    }
    
    CarrierInfo& info = m_carrierStates[instanceId];
    info.isRoaming = (status != RoamingStatus::HOME);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString roamingStr = (status == RoamingStatus::HOME) ? "false" : "true";
    
    ctrl.executeShell(instanceId,
        QString("setprop gsm.operator.isroaming %1").arg(roamingStr));
    ctrl.executeShell(instanceId,
        QString("setprop persist.radio.roaming %1").arg(roamingStr));
    
    return true;
}

bool CarrierNetworkSimulator::simulateTravel(const QString& instanceId, const QString& country, int durationMs) {
    // Find a carrier from the destination country
    CarrierPreset targetCarrier;
    bool found = false;
    
    for (const auto& preset : m_carrierPresets) {
        if (preset.iso == country) {
            targetCarrier = preset;
            found = true;
            break;
        }
    }
    
    if (found) {
        // Change to local carrier
        configureCarrier(instanceId, targetCarrier.name, country);
        
        // Enable roaming
        setRoamingStatus(instanceId, RoamingStatus::ROAMING);
        setDataRoamingEnabled(instanceId, true);
    }
    
    // Simulate duration
    QThread::msleep(durationMs);
    
    // Return to home carrier (optional)
    return true;
}

// ============================================================================
// Utility
// ============================================================================

QMap<QString, QString> CarrierNetworkSimulator::getAllCarrierProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    CarrierInfo info = getCarrierInfo(instanceId);
    
    props["telephony.sms.prompt_telephone_number"] = "false";
    props["telephony.sms.default_subscription"] = "0";
    props["telephony.lteOnCdmaDevice"] = "0";
    props["telephony.lteOnGsmDevice"] = "1";
    props["telephony.num.physicals"] = QString::number(info.simSlotCount);
    
    props["gsm.sim.operator.numeric"] = info.numeric;
    props["gsm.sim.operator.alpha"] = info.name;
    props["gsm.sim.operator.iso-country"] = info.iso;
    props["gsm.sim.imsi"] = info.imsi;
    props["gsm.sim.serial"] = info.simSerialNumber;
    props["gsm.sim.state"] = "READY";
    
    props["gsm.operator.numeric"] = info.numeric;
    props["gsm.operator.alpha"] = info.name;
    props["gsm.operator.iso-country"] = info.iso;
    props["gsm.operator.isroaming"] = info.isRoaming ? "true" : "false";
    props["gsm.network.type"] = networkTypeToString(info.networkType);
    
    props["gsm.signal.strength"] = QString::number(info.signal.dBm);
    props["gsm.signal.level"] = QString::number(info.signal.level);
    
    props["ro.product.locale.region"] = info.iso;
    props["persist.sys.country"] = info.iso.toLower();
    
    return props;
}

bool CarrierNetworkSimulator::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

bool CarrierNetworkSimulator::resetCarrier(const QString& instanceId) {
    return configureCarrier(instanceId, "T-Mobile", "US");
}

QList<CarrierPreset> CarrierNetworkSimulator::getCarrierPresets() const {
    return m_carrierPresets;
}

// ============================================================================
// Private Helpers
// ============================================================================

QString CarrierNetworkSimulator::networkTypeToString(NetworkType type) const {
    switch (type) {
        case NetworkType::GSM: return "GSM";
        case NetworkType::CDMA: return "CDMA";
        case NetworkType::UMTS: return "UMTS";
        case NetworkType::HSDPA: return "HSDPA";
        case NetworkType::LTE: return "LTE";
        case NetworkType::LTE_CA: return "LTE_CA";
        case NetworkType::NR: return "NR";
        case NetworkType::NR_SA: return "NR_SA";
        case NetworkType::WIFI: return "WIFI";
        default: return "UNKNOWN";
    }
}

NetworkType CarrierNetworkSimulator::stringToNetworkType(const QString& str) const {
    if (str == "LTE") return NetworkType::LTE;
    if (str == "LTE_CA") return NetworkType::LTE_CA;
    if (str == "NR") return NetworkType::NR;
    if (str == "UMTS") return NetworkType::UMTS;
    if (str == "HSDPA") return NetworkType::HSDPA;
    if (str == "GSM") return NetworkType::GSM;
    if (str == "WIFI") return NetworkType::WIFI;
    return NetworkType::LTE;
}

QString CarrierNetworkSimulator::generateIMSI(const QString& mcc, const QString& mnc) {
    QString imsi = mcc + mnc;
    
    // Generate remaining 6 digits
    QRandomGenerator* gen = QRandomGenerator::global();
    for (int i = 0; i < 6; i++) {
        imsi += QString::number(gen->bounded(10));
    }
    
    return imsi;
}

QString CarrierNetworkSimulator::generateICCID(const QString& mcc) {
    QString iccid = "89"; // ICCID prefix
    
    // Country code
    iccid += mcc;
    
    // Generate remaining digits
    QRandomGenerator* gen = QRandomGenerator::global();
    for (int i = 0; i < 17; i++) {
        iccid += QString::number(gen->bounded(10));
    }
    
    return iccid;
}

SignalStrength CarrierNetworkSimulator::dBmToSignalLevel(int dBm) const {
    SignalStrength signal;
    signal.dBm = dBm;
    
    if (dBm >= -50) {
        signal.level = 4;
        signal.levelPercent = 100;
        signal.asu = 97;
    } else if (dBm >= -60) {
        signal.level = 4;
        signal.levelPercent = 85;
        signal.asu = 75;
    } else if (dBm >= -70) {
        signal.level = 3;
        signal.levelPercent = 60;
        signal.asu = 55;
    } else if (dBm >= -80) {
        signal.level = 2;
        signal.levelPercent = 40;
        signal.asu = 35;
    } else if (dBm >= -90) {
        signal.level = 1;
        signal.levelPercent = 20;
        signal.asu = 15;
    } else {
        signal.level = 0;
        signal.levelPercent = 0;
        signal.asu = 0;
    }
    
    return signal;
}

} // namespace VirtualPhonePro
