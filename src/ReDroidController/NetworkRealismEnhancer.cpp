/**
 * @file NetworkRealismEnhancer.cpp
 * @brief Network Realism Enhancer Implementation
 */

#include "VirtualPhonePro/NetworkRealismEnhancer.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QJsonObject>

namespace VirtualPhonePro {

NetworkRealismEnhancer* NetworkRealismEnhancer::s_instance = nullptr;

NetworkRealismEnhancer& NetworkRealismEnhancer::instance() {
    if (!s_instance) {
        s_instance = new NetworkRealismEnhancer();
    }
    return *s_instance;
}

NetworkRealismEnhancer::NetworkRealismEnhancer() {
    initializeCACombinations();
}

// ============================================================================
// Configuration
// ============================================================================

bool NetworkRealismEnhancer::configureSingleSIM(const QString& instanceId, const QString& operatorCode,
                                                 const QString& operatorName, NetworkTechnology tech) {
    NetworkRealismState& state = m_states[instanceId];
    state.instanceId = instanceId;
    
    // Single SIM configuration
    state.dualSIM.isDualSIMEnabled = false;
    state.dualSIM.isDualSIMActive = false;
    state.dualSIM.isSIM1Enabled = true;
    state.dualSIM.isSIM2Enabled = false;
    state.dualSIM.sim1Operator = operatorCode;
    state.dualSIM.sim1OperatorName = operatorName;
    state.dualSIM.sim1MCCMNC = operatorCode;
    state.dualSIM.sim1State = SIMState::READY;
    
    state.currentTechnology = tech;
    state.preferredTechnology = tech;
    state.simOperatorCode = operatorCode;
    state.simOperatorName = operatorName;
    
    qDebug() << "Configured single SIM for instance:" << instanceId
             << "- Operator:" << operatorName
             << "- Technology:" << technologyToString(tech);
    
    return applyToInstance(instanceId);
}

bool NetworkRealismEnhancer::configureDualSIM(const QString& instanceId, const DualSIMConfig& config) {
    NetworkRealismState& state = m_states[instanceId];
    state.instanceId = instanceId;
    state.dualSIM = config;
    
    // If both SIMs are active, it's DSDS
    if (config.isSIM1Enabled && config.isSIM2Enabled) {
        state.dualSIM.isDualStandby = true;
        state.dualSIM.standbyType = "dsds";
    }
    
    qDebug() << "Configured dual SIM for instance:" << instanceId
             << "- SIM1:" << config.sim1OperatorName
             << "- SIM2:" << config.sim2OperatorName
             << "- Standby:" << config.standbyType;
    
    return applyToInstance(instanceId);
}

bool NetworkRealismEnhancer::configureNetworkBands(const QString& instanceId, const NetworkBandConfig& bands) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].bands = bands;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Build LTE bands string
    QString lteBandsStr;
    for (int i = 0; i < bands.lteBands.size(); ++i) {
        lteBandsStr += QString::number(bands.lteBands[i]);
        if (i < bands.lteBands.size() - 1) lteBandsStr += ",";
    }
    
    // Build NR bands string
    QString nrBandsStr;
    for (int i = 0; i < bands.nrBands.size(); ++i) {
        nrBandsStr += QString::number(bands.nrBands[i]);
        if (i < bands.nrBands.size() - 1) nrBandsStr += ",";
    }
    
    ctrl.executeShell(instanceId, QString("setprop persist.radio.lte.band %1").arg(lteBandsStr));
    ctrl.executeShell(instanceId, QString("setprop persist.radio.nr.band %1").arg(nrBandsStr));
    
    return true;
}

bool NetworkRealismEnhancer::applyToInstance(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    NetworkRealismState& state = m_states[instanceId];
    
    QStringList commands;
    
    // Network technology
    commands += {
        QString("setprop ro.telephony.default.network %1").arg(
            state.currentTechnology == NetworkTechnology::NR_5G ? "11" :
            state.currentTechnology == NetworkTechnology::LTE_ADVANCED ? "9" :
            state.currentTechnology == NetworkTechnology::LTE_4G ? "10" :
            state.currentTechnology == NetworkTechnology::UMTS_3G ? "3" : "2"),
    };
    
    // Dual SIM
    if (state.dualSIM.isDualSIMEnabled) {
        commands += {
            "setprop persist.radio.multisim.config dsds",
            "setprop ro.telephony.default.dsda false",
            QString("setprop gsm.sim.operator.num %1").arg(state.dualSIM.sim1Operator),
            QString("setprop gsm.sim.operator.alpha %1").arg(state.dualSIM.sim1OperatorName),
            QString("setprop gsm.sim.operator.iso-country %1").arg(state.networkCountryCode),
        };
    } else {
        commands += {
            "setprop persist.radio.multisim.config ssss",
            "setprop gsm.sim.operator.num " + state.simOperatorCode,
            "setprop gsm.sim.operator.alpha " + state.simOperatorName,
        };
    }
    
    // Wi-Fi Calling
    commands += {
        QString("setprop persist.wfc.enable %1").arg(state.wifiCalling.isEnabled ? "true" : "false"),
        QString("setprop ro.config.wifi_callingsvc %1").arg(state.wifiCalling.isSupported ? "true" : "false"),
        QString("setprop ro.wfc.enable %1").arg(state.wifiCalling.isEnabled ? "true" : "false"),
    };
    
    // VoLTE
    commands += {
        QString("setprop persist.volte.enable %1").arg(state.volte.isEnabled ? "true" : "false"),
        QString("ro.config.hw_volte_activated %1").arg(state.volte.isEnabled ? "true" : "false"),
        "setprop persist.dbg.volte_avail_ovr true",
        "setprop persist.dbg.wfc_avail_ovr true",
    };
    
    // Carrier Aggregation
    if (state.bands.isCAEnabled) {
        commands += {
            "setprop persist.radio.enable_ca true",
            "setprop ro.lte.ca.enabled true",
        };
    }
    
    // Roaming
    commands += {
        QString("setprop persist.roaming %1").arg(state.isRoaming ? "true" : "false"),
        QString("setprop persist.data_roaming %1").arg(state.isDataRoamingEnabled ? "1" : "0"),
    };
    
    // Network country
    commands += {
        "setprop persist.sys.timezone " + getenv("TZ") ? getenv("TZ") : "UTC",
        "setprop gsm.network.countrycode " + state.networkCountryCode,
    };
    
    // Execute all commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Network realism configuration applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Wi-Fi Calling
// ============================================================================

bool NetworkRealismEnhancer::enableWiFiCalling(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.wifiCalling.isEnabled = true;
    state.wifiCalling.status = WiFiCallingStatus::ENABLED;
    state.wifiCalling.isSupported = true;
    state.wifiCalling.isProvisioned = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.wfc.enable true");
    ctrl.executeShell(instanceId, "settings put global wifi_call_on 1");
    ctrl.executeShell(instanceId, "settings put global wfc_ims_mode 1");
    
    qDebug() << "Wi-Fi Calling enabled for instance:" << instanceId;
    return true;
}

bool NetworkRealismEnhancer::disableWiFiCalling(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.wifiCalling.isEnabled = false;
    state.wifiCalling.status = WiFiCallingStatus::DISABLED;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.wfc.enable false");
    ctrl.executeShell(instanceId, "settings put global wifi_call_on 0");
    
    qDebug() << "Wi-Fi Calling disabled for instance:" << instanceId;
    return true;
}

bool NetworkRealismEnhancer::setWiFiCallingMode(const QString& instanceId, const QString& mode) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].wifiCalling.preferredCallingMode = mode;
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (mode == "wifi_preferred") {
        ctrl.executeShell(instanceId, "settings put global wfc_ims_mode 1");
    } else if (mode == "cellular_preferred") {
        ctrl.executeShell(instanceId, "settings put global wfc_ims_mode 0");
    } else if (mode == "wifi_only") {
        ctrl.executeShell(instanceId, "settings put global wfc_ims_mode 2");
    }
    
    return true;
}

WiFiCallingStatus NetworkRealismEnhancer::getWiFiCallingStatus(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].wifiCalling.status;
    }
    return WiFiCallingStatus::DISABLED;
}

// ============================================================================
// VoLTE
// ============================================================================

bool NetworkRealismEnhancer::enableVoLTE(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.volte.isEnabled = true;
    state.volte.status = VoLTEStatus::ENABLED;
    state.volte.isSupported = true;
    state.volte.isProvisioned = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.volte.enable true");
    ctrl.executeShell(instanceId, "settings put global volteo_toggle 1");
    ctrl.executeShell(instanceId, "setprop dbg.volte_avail_ovr true");
    
    qDebug() << "VoLTE enabled for instance:" << instanceId;
    return true;
}

bool NetworkRealismEnhancer::disableVoLTE(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.volte.isEnabled = false;
    state.volte.status = VoLTEStatus::DISABLED;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.volte.enable false");
    ctrl.executeShell(instanceId, "settings put global volteo_toggle 0");
    
    qDebug() << "VoLTE disabled for instance:" << instanceId;
    return true;
}

VoLTEStatus NetworkRealismEnhancer::getVoLTEStatus(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].volte.status;
    }
    return VoLTEStatus::DISABLED;
}

// ============================================================================
// Dual SIM
// ============================================================================

DualSIMConfig NetworkRealismEnhancer::getDualSIMConfig(const QString& instanceId) const {
    DualSIMConfig defaultConfig;
    defaultConfig.isDualSIMEnabled = false;
    defaultConfig.isDualSIMActive = false;
    defaultConfig.isSIM1Enabled = true;
    defaultConfig.isSIM2Enabled = false;
    defaultConfig.isDualStandby = false;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].dualSIM;
    }
    
    return defaultConfig;
}

bool NetworkRealismEnhancer::setSIMEnabled(const QString& instanceId, int simSlot, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    
    if (simSlot == 1) {
        state.dualSIM.isSIM1Enabled = enabled;
        state.dualSIM.sim1State = enabled ? SIMState::READY : SIMState::DISABLED;
    } else if (simSlot == 2) {
        state.dualSIM.isSIM2Enabled = enabled;
        state.dualSIM.sim2State = enabled ? SIMState::READY : SIMState::DISABLED;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop cind.slot%1.enabled %2").arg(simSlot).arg(enabled ? "true" : "false"));
    
    return true;
}

bool NetworkRealismEnhancer::switchActiveSIM(const QString& instanceId, int simSlot) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.dualSIM.sim1SlotActive = (simSlot == 1) ? 1 : 0;
    state.dualSIM.sim2SlotActive = (simSlot == 2) ? 1 : 0;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop gsm.active_slot %1").arg(simSlot));
    
    return true;
}

bool NetworkRealismEnhancer::simulateSIMChange(const QString& instanceId, int simSlot, 
                                               const QString& operatorCode, const QString& operatorName) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    
    if (simSlot == 1) {
        state.dualSIM.sim1Operator = operatorCode;
        state.dualSIM.sim1OperatorName = operatorName;
        state.dualSIM.sim1MCCMNC = operatorCode;
    } else if (simSlot == 2) {
        state.dualSIM.sim2Operator = operatorCode;
        state.dualSIM.sim2OperatorName = operatorName;
        state.dualSIM.sim2MCCMNC = operatorCode;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop gsm.sim%1.operator.num %2").arg(simSlot).arg(operatorCode));
    ctrl.executeShell(instanceId, QString("setprop gsm.sim%1.operator.alpha %2").arg(simSlot).arg(operatorName));
    
    qDebug() << "Simulated SIM change - Slot" << simSlot << "to" << operatorName;
    return true;
}

// ============================================================================
// Carrier Aggregation
// ============================================================================

bool NetworkRealismEnhancer::enableCA(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].bands.isCAEnabled = true;
    m_states[instanceId].isCAConnected = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.radio.enable_ca true");
    ctrl.executeShell(instanceId, "setprop ro.lte.ca.enabled true");
    
    return true;
}

bool NetworkRealismEnhancer::disableCA(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].bands.isCAEnabled = false;
    m_states[instanceId].isCAConnected = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "setprop persist.radio.enable_ca false");
    ctrl.executeShell(instanceId, "setprop ro.lte.ca.enabled false");
    
    return true;
}

bool NetworkRealismEnhancer::setCABandCombination(const QString& instanceId, const CABandCombination& ca) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].currentCA = ca;
    m_states[instanceId].isCAConnected = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop persist.radio.ca_bands %1+%2")
                      .arg(ca.primaryBand).arg(ca.secondaryBand));
    
    qDebug() << "Set CA band combination:" << ca.combinationName 
             << "-" << ca.primaryBand << "+" << ca.secondaryBand
             << "@" << ca.maxSpeedMbps << "Mbps";
    
    return true;
}

bool NetworkRealismEnhancer::isCAConnected(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isCAConnected;
    }
    return false;
}

QList<CABandCombination> NetworkRealismEnhancer::getAvailableCACombinations(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].caCombinations;
    }
    return m_caCombinations;
}

// ============================================================================
// Signal Strength
// ============================================================================

bool NetworkRealismEnhancer::setSignalStrength(const QString& instanceId, int rsrp, int rsrq) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    state.signal.rsrp = rsrp;
    state.signal.rsrq = rsrq;
    state.signal.sinr = qBound(0, (rsrq + 20) * 2, 40);
    state.signal.asuLevel = qBound(0, (rsrp + 140) * 97 / 97, 97);
    state.signal.signalBars = calculateSignalBars(rsrp);
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop gsm.signal strength %1").arg(state.signal.asuLevel));
    ctrl.executeShell(instanceId, QString("setprop persist.radio.rssi.rsrp %1").arg(rsrp));
    ctrl.executeShell(instanceId, QString("setprop persist.radio.rssi.rsrq %1").arg(rsrq));
    
    return true;
}

bool NetworkRealismEnhancer::simulateSignalFluctuation(const QString& instanceId, int range) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    int currentRSRP = state.signal.rsrp;
    int fluctuation = QRandomGenerator::global()->bounded(-range, range + 1);
    int newRSRP = qBound(-120, currentRSRP + fluctuation, -60);
    
    return setSignalStrength(instanceId, newRSRP, state.signal.rsrq);
}

SignalStrengthConfig NetworkRealismEnhancer::getSignalStrength(const QString& instanceId) const {
    SignalStrengthConfig defaultSignal;
    defaultSignal.rsrp = -85;
    defaultSignal.rsrq = -10;
    defaultSignal.sinr = 20;
    defaultSignal.signalBars = 4;
    defaultSignal.asuLevel = 55;
    defaultSignal.networkType = "LTE";
    defaultSignal.technology = NetworkTechnology::LTE_4G;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].signal;
    }
    
    return defaultSignal;
}

// ============================================================================
// Network Technology
// ============================================================================

bool NetworkRealismEnhancer::setNetworkTechnology(const QString& instanceId, NetworkTechnology tech) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].currentTechnology = tech;
    m_states[instanceId].signal.technology = tech;
    m_states[instanceId].signal.networkType = technologyToString(tech);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString networkType = (tech == NetworkTechnology::NR_5G || tech == NetworkTechnology::NR_ADVANCED) ? "11" :
                          (tech == NetworkTechnology::LTE_ADVANCED || tech == NetworkTechnology::LTE_4G) ? "10" :
                          (tech == NetworkTechnology::UMTS_3G) ? "3" : "2";
    
    ctrl.executeShell(instanceId, QString("setprop ro.telephony.default.network %1").arg(networkType));
    
    qDebug() << "Set network technology for" << instanceId << "to" << technologyToString(tech);
    
    return true;
}

NetworkTechnology NetworkRealismEnhancer::getNetworkTechnology(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].currentTechnology;
    }
    return NetworkTechnology::LTE_4G;
}

bool NetworkRealismEnhancer::simulateNetworkHandover(const QString& instanceId, NetworkTechnology targetTech) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    NetworkRealismState& state = m_states[instanceId];
    
    // Simulate gradual handover
    qDebug() << "Simulating network handover from" << technologyToString(state.currentTechnology)
             << "to" << technologyToString(targetTech);
    
    state.currentTechnology = targetTech;
    state.signal.technology = targetTech;
    state.signal.networkType = technologyToString(targetTech);
    
    return applyToInstance(instanceId);
}

// ============================================================================
// Roaming
// ============================================================================

bool NetworkRealismEnhancer::enableDataRoaming(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isDataRoamingEnabled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global data_roaming 1");
    ctrl.executeShell(instanceId, "settings put global data_roaming_expand 1");
    
    return true;
}

bool NetworkRealismEnhancer::disableDataRoaming(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isDataRoamingEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put global data_roaming 0");
    
    return true;
}

bool NetworkRealismEnhancer::setRoaming(const QString& instanceId, bool isRoaming) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].isRoaming = isRoaming;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop persist.roaming %1").arg(isRoaming ? "true" : "false"));
    
    return true;
}

bool NetworkRealismEnhancer::isRoaming(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId].isRoaming;
    }
    return false;
}

// ============================================================================
// Utility
// ============================================================================

NetworkRealismState NetworkRealismEnhancer::getNetworkState(const QString& instanceId) const {
    NetworkRealismState defaultState;
    defaultState.instanceId = instanceId;
    defaultState.currentTechnology = NetworkTechnology::LTE_4G;
    
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    return defaultState;
}

QJsonObject NetworkRealismEnhancer::getNetworkInfoJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_states.contains(instanceId)) {
        const NetworkRealismState& state = m_states[instanceId];
        
        json["technology"] = technologyToString(state.currentTechnology);
        json["isRoaming"] = state.isRoaming;
        json["isDataRoamingEnabled"] = state.isDataRoamingEnabled;
        
        // Signal
        QJsonObject signal;
        signal["rsrp"] = state.signal.rsrp;
        signal["rsrq"] = state.signal.rsrq;
        signal["sinr"] = state.signal.sinr;
        signal["bars"] = state.signal.signalBars;
        signal["asu"] = state.signal.asuLevel;
        json["signal"] = signal;
        
        // Wi-Fi Calling
        QJsonObject wfc;
        wfc["enabled"] = state.wifiCalling.isEnabled;
        wfc["supported"] = state.wifiCalling.isSupported;
        wfc["mode"] = state.wifiCalling.preferredCallingMode;
        json["wifiCalling"] = wfc;
        
        // VoLTE
        QJsonObject volte;
        volte["enabled"] = state.volte.isEnabled;
        volte["supported"] = state.volte.isSupported;
        json["volte"] = volte;
        
        // Dual SIM
        QJsonObject dualSim;
        dualSim["enabled"] = state.dualSIM.isDualSIMEnabled;
        dualSim["sim1"] = state.dualSIM.sim1OperatorName;
        dualSim["sim2"] = state.dualSIM.sim2OperatorName;
        json["dualSim"] = dualSim;
        
        // Carrier Aggregation
        QJsonObject ca;
        ca["connected"] = state.isCAConnected;
        ca["primaryBand"] = state.currentCA.primaryBand;
        ca["secondaryBand"] = state.currentCA.secondaryBand;
        ca["maxSpeed"] = state.currentCA.maxSpeedMbps;
        json["carrierAggregation"] = ca;
    }
    
    return json;
}

bool NetworkRealismEnhancer::reset(const QString& instanceId) {
    if (m_states.contains(instanceId)) {
        m_states.remove(instanceId);
        return true;
    }
    return false;
}

bool NetworkRealismEnhancer::applyAllRealism(const QString& instanceId) {
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

void NetworkRealismEnhancer::initializeCACombinations() {
    // Common LTE-A CA combinations
    m_caCombinations = {
        {2, 12, 2, 150, "B2+B12"},
        {2, 66, 2, 300, "B2+B66"},
        {4, 13, 2, 150, "B4+B13"},
        {4, 66, 2, 400, "B4+B66"},
        {5, 66, 2, 150, "B5+B66"},
        {12, 30, 2, 150, "B12+B30"},
        {2, 5, 2, 150, "B2+B5"},
        {2, 4, 2, 300, "B2+B4"},
        {4, 12, 2, 150, "B4+B12"},
        {25, 26, 2, 200, "B25+B26"},
        {41, 42, 2, 300, "B41+B42"},
        {66, 71, 2, 400, "B66+B71"},
        
        // 5G NR combinations
        {71, 2, 2, 500, "n71+n2"},
        {41, 78, 2, 1000, "n41+n78"},
        {78, 257, 2, 2000, "n78+n257"},
    };
}

int NetworkRealismEnhancer::calculateSignalBars(int rsrp) const {
    if (rsrp >= -85) return 4;
    if (rsrp >= -95) return 3;
    if (rsrp >= -105) return 2;
    if (rsrp >= -115) return 1;
    return 0;
}

QString NetworkRealismEnhancer::technologyToString(NetworkTechnology tech) const {
    switch (tech) {
        case NetworkTechnology::GSM_2G: return "2G";
        case NetworkTechnology::UMTS_3G: return "3G";
        case NetworkTechnology::LTE_4G: return "LTE";
        case NetworkTechnology::LTE_ADVANCED: return "LTE-A";
        case NetworkTechnology::NR_5G: return "5G";
        case NetworkTechnology::NR_ADVANCED: return "5G-SA";
        default: return "UNKNOWN";
    }
}

NetworkTechnology NetworkRealismEnhancer::stringToTechnology(const QString& tech) const {
    QString lower = tech.toLower();
    if (lower == "2g" || lower == "gsm") return NetworkTechnology::GSM_2G;
    if (lower == "3g" || lower == "umts") return NetworkTechnology::UMTS_3G;
    if (lower == "lte-a" || lower == "lteadvanced") return NetworkTechnology::LTE_ADVANCED;
    if (lower == "5g" || lower == "nr") return NetworkTechnology::NR_5G;
    if (lower == "5g-sa" || lower == "nr-advanced") return NetworkTechnology::NR_ADVANCED;
    return NetworkTechnology::LTE_4G;
}

} // namespace VirtualPhonePro
