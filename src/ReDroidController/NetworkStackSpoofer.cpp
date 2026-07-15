#include "NetworkStackSpoofer.hpp"
#include "ADBManager.hpp"
#include "Logger.hpp"
#include <sstream>
#include <random>

namespace AntiDetect {

NetworkStackSpoofer& NetworkStackSpoofer::getInstance() {
    static NetworkStackSpoofer instance;
    return instance;
}

NetworkStackSpoofer::NetworkStackSpoofer()
    : m_initialized(false)
    , m_spoofingActive(false)
{
    // Default network configuration for real Android device
    m_currentConfig = {
        "cubic",      // tcpCongestionControl
        "1",          // tcpWindowScaling
        "1500",       // mtuSize
        "1",          // tcpTimestamps
        "1",          // tcpSACK
        "0",          // ipForward
        "3",          // tcpFastOpen
        64,           // defaultTTL - real device default
        65,           // wifiTTL
        65,           // mobileTTL
        4096,         // tcpRmemMin
        87380,        // tcpRmemDefault
        6291456,      // tcpRmemMax
        4096,         // tcpWmemMin
        16384,        // tcpWmemDefault
        4194304       // tcpWmemMax
    };
}

NetworkStackSpoofer::~NetworkStackSpoofer() {
    shutdown();
}

bool NetworkStackSpoofer::initialize() {
    Logger::getInstance().info("Initializing Network Stack Spoofer...");
    
    auto& adb = ADBManager::getInstance();
    if (!adb.isConnected()) {
        Logger::getInstance().warning("ADB not connected - network spoofing limited");
    }
    
    m_initialized = true;
    Logger::getInstance().info("Network Stack Spoofer initialized");
    return true;
}

bool NetworkStackSpoofer::isInitialized() const {
    return m_initialized;
}

void NetworkStackSpoofer::shutdown() {
    if (m_initialized) {
        if (m_spoofingActive) {
            restoreOriginalSettings();
        }
        m_initialized = false;
        Logger::getInstance().info("Network Stack Spoofer shutdown complete");
    }
}

NetworkSpoofResult2 NetworkStackSpoofer::enableStackSpoofing() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    if (!m_initialized) {
        result.error = "Not initialized";
        return result;
    }
    
    setDeviceTTL();
    enableTimestamps();
    enableWindowScaling();
    applySamsungNetworkProfile();
    
    m_spoofingActive = true;
    result.success = true;
    result.message = "Network stack spoofing enabled";
    
    Logger::getInstance().info(result.message);
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::disableStackSpoofing() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    restoreOriginalSettings();
    m_spoofingActive = false;
    
    result.success = true;
    result.message = "Network stack spoofing disabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setCongestionControl(const std::string& algorithm) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Set TCP congestion control algorithm
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_congestion_control=" + algorithm);
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_available_congestion_control=" + algorithm + " reno cubic");
    
    m_currentConfig.tcpCongestionControl = algorithm;
    m_modifiedSettings["tcp_congestion_control"] = algorithm;
    
    result.success = true;
    result.message = "TCP congestion control set to: " + algorithm;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setCubicProfile() {
    return setCongestionControl("cubic");
}

NetworkSpoofResult2 NetworkStackSpoofer::setBbrProfile() {
    return setCongestionControl("bbr");
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofTTL(int ttl) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Set default TTL for all interfaces
    adb.executeShellCommand("iptables -t mangle -A POSTROUTING -j TTL --ttl-set " + std::to_string(ttl));
    adb.executeShellCommand("iptables -t mangle -A PREROUTING -j TTL --ttl-set " + std::to_string(ttl));
    
    m_currentConfig.defaultTTL = ttl;
    m_modifiedSettings["default_ttl"] = std::to_string(ttl);
    
    result.success = true;
    result.message = "TTL spoofed to: " + std::to_string(ttl);
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofWifiTTL() {
    return spoofTTL(65);  // Real device WiFi TTL
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofMobileTTL() {
    return spoofTTL(65);  // Real device mobile TTL
}

NetworkSpoofResult2 NetworkStackSpoofer::setDeviceTTL() {
    return spoofTTL(64);  // Real Linux device default TTL
}

NetworkSpoofResult2 NetworkStackSpoofer::enableWindowScaling() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_window_scaling=1");
    
    m_currentConfig.tcpWindowScaling = "1";
    m_modifiedSettings["tcp_window_scaling"] = "1";
    
    result.success = true;
    result.message = "TCP window scaling enabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::disableWindowScaling() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_window_scaling=0");
    
    m_currentConfig.tcpWindowScaling = "0";
    m_modifiedSettings["tcp_window_scaling"] = "0";
    
    result.success = true;
    result.message = "TCP window scaling disabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::enableTimestamps() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_timestamps=1");
    
    m_currentConfig.tcpTimestamps = "1";
    m_modifiedSettings["tcp_timestamps"] = "1";
    
    result.success = true;
    result.message = "TCP timestamps enabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::disableTimestamps() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_timestamps=0");
    
    m_currentConfig.tcpTimestamps = "0";
    m_modifiedSettings["tcp_timestamps"] = "0";
    
    result.success = true;
    result.message = "TCP timestamps disabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::enableSACK() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_sack=1");
    
    m_currentConfig.tcpSACK = "1";
    m_modifiedSettings["tcp_sack"] = "1";
    
    result.success = true;
    result.message = "TCP SACK enabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::disableSACK() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("sysctl -w net.ipv4.tcp_sack=0");
    
    m_currentConfig.tcpSACK = "0";
    m_modifiedSettings["tcp_sack"] = "0";
    
    result.success = true;
    result.message = "TCP SACK disabled";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setMTU(int mtu) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Set MTU for common interfaces
    adb.executeShellCommand("ip link set wlan0 mtu " + std::to_string(mtu));
    adb.executeShellCommand("ip link set eth0 mtu " + std::to_string(mtu));
    
    m_currentConfig.mtuSize = std::to_string(mtu);
    m_modifiedSettings["mtu"] = std::to_string(mtu);
    
    result.success = true;
    result.message = "MTU set to: " + std::to_string(mtu);
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setWifiMTU() {
    return setMTU(1500);  // Standard WiFi MTU
}

NetworkSpoofResult2 NetworkStackSpoofer::setMobileMTU() {
    return setMTU(1500);  // Standard mobile MTU
}

NetworkSpoofResult2 NetworkStackSpoofer::setCustomDNS(const std::vector<std::string>& dnsServers) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    std::string dnsStr;
    for (size_t i = 0; i < dnsServers.size(); ++i) {
        if (i > 0) dnsStr += " ";
        dnsStr += dnsServers[i];
    }
    
    adb.executeShellCommand("settings put global dns1 " + (dnsServers.size() > 0 ? dnsServers[0] : "8.8.8.8"));
    adb.executeShellCommand("settings put global dns2 " + (dnsServers.size() > 1 ? dnsServers[1] : "8.8.4.4"));
    
    result.success = true;
    result.message = "Custom DNS set: " + dnsStr;
    result.details["dns"] = dnsStr;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setGoogleDNS() {
    return setCustomDNS({"8.8.8.8", "8.8.4.4"});
}

NetworkSpoofResult2 NetworkStackSpoofer::setCloudflareDNS() {
    return setCustomDNS({"1.1.1.1", "1.0.0.1"});
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofUserAgent(const std::string& userAgent) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Store user agent for apps to use
    adb.executeShellCommand("settings put global user_agent " + userAgent);
    
    m_modifiedSettings["user_agent"] = userAgent;
    
    result.success = true;
    result.message = "User agent spoofed";
    result.details["user_agent"] = userAgent;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::setChromeUserAgent() {
    return spoofUserAgent(
        "Mozilla/5.0 (Linux; Android 13; SM-G998B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
    );
}

NetworkSpoofResult2 NetworkStackSpoofer::setFirefoxUserAgent() {
    return spoofUserAgent(
        "Mozilla/5.0 (Android 13; Mobile; rv:121.0) Gecko/121.0 Firefox/121.0"
    );
}

NetworkSpoofResult2 NetworkStackSpoofer::setSafariUserAgent() {
    return spoofUserAgent(
        "Mozilla/5.0 (iPhone; CPU iPhone OS 17_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1"
    );
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofMACAddress(const std::string& mac) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Validate MAC format
    if (mac.length() != 17) {
        result.error = "Invalid MAC address format";
        return result;
    }
    
    // Spoof WiFi MAC
    adb.executeShellCommand("ip link set wlan0 addr " + mac);
    adb.executeShellCommand("svc wifi disable && svc wifi enable");
    
    m_modifiedSettings["wifi_mac"] = mac;
    
    result.success = true;
    result.message = "MAC address spoofed: " + mac;
    result.details["mac"] = mac;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::randomizeMAC() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    // Samsung OUI (first 3 octets)
    ss << std::setw(2) << 48 << ":";
    ss << std::setw(2) << 74 << ":";
    ss << std::setw(2) << 50 << ":";
    
    // Random last 3 octets
    for (int i = 0; i < 3; ++i) {
        ss << std::setw(2) << dis(gen);
        if (i < 2) ss << ":";
    }
    
    return spoofMACAddress(ss.str());
}

NetworkSpoofResult2 NetworkStackSpoofer::setSamsungMAC() {
    return spoofMACAddress("48:74:40:XX:XX:XX");
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofMobileOperator(const std::string& name) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("settings put global operator_name " + name);
    adb.executeShellCommand("settings put secure carrier_names " + name);
    
    result.success = true;
    result.message = "Mobile operator spoofed: " + name;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofMobileCountryCode(int mcc) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("settings put global mcc " + std::to_string(mcc));
    
    result.success = true;
    result.message = "Mobile Country Code set: " + std::to_string(mcc);
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofMobileNetworkCode(int mnc) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("settings put global mnc " + std::to_string(mnc));
    
    result.success = true;
    result.message = "Mobile Network Code set: " + std::to_string(mnc);
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::spoofNetworkType(const std::string& type) {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    auto& adb = ADBManager::getInstance();
    
    // Network type spoofing
    if (type == "5G") {
        adb.executeShellCommand("settings put global preferred_network_mode 20");
    } else if (type == "4G" || type == "LTE") {
        adb.executeShellCommand("settings put global preferred_network_mode 9");
    } else if (type == "3G") {
        adb.executeShellCommand("settings put global preferred_network_mode 3");
    } else if (type == "2G") {
        adb.executeShellCommand("settings put global preferred_network_mode 1");
    }
    
    result.success = true;
    result.message = "Network type spoofed: " + type;
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::applySamsungNetworkProfile() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    setDeviceTTL();
    setCubicProfile();
    enableTimestamps();
    enableWindowScaling();
    enableSACK();
    setMTU(1500);
    setGoogleDNS();
    setSamsungMAC();
    
    result.success = true;
    result.message = "Samsung network profile applied - optimized for real device appearance";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::applyGoogleNetworkProfile() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    setDeviceTTL();
    setCubicProfile();
    enableTimestamps();
    enableWindowScaling();
    setMTU(1500);
    setGoogleDNS();
    
    result.success = true;
    result.message = "Google network profile applied";
    
    return result;
}

NetworkSpoofResult2 NetworkStackSpoofer::validateSpoofing() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    result.success = m_spoofingActive;
    result.message = m_spoofingActive ? 
        "Network spoofing is active" : 
        "Network spoofing is not active";
    
    result.details["spoofed_settings"] = std::to_string(m_modifiedSettings.size());
    
    return result;
}

bool NetworkStackSpoofer::isSpoofingActive() const {
    return m_spoofingActive;
}

NetworkStackConfig NetworkStackSpoofer::getCurrentConfig() {
    return m_currentConfig;
}

std::map<std::string, std::string> NetworkStackSpoofer::getDetailedStatus() {
    std::map<std::string, std::string> status;
    
    status["initialized"] = m_initialized ? "true" : "false";
    status["spoofing_active"] = m_spoofingActive ? "true" : "false";
    
    status["tcp_congestion"] = m_currentConfig.tcpCongestionControl;
    status["default_ttl"] = std::to_string(m_currentConfig.defaultTTL);
    status["window_scaling"] = m_currentConfig.tcpWindowScaling;
    status["timestamps"] = m_currentConfig.tcpTimestamps;
    status["sack"] = m_currentConfig.tcpSACK;
    status["mtu"] = m_currentConfig.mtuSize;
    
    return status;
}

NetworkSpoofResult2 NetworkStackSpoofer::getStatus() {
    NetworkSpoofResult2 result = {false, "", "", {}};
    
    std::stringstream ss;
    ss << "Network Stack Spoofer Status:\n";
    ss << "  Active: " << (m_spoofingActive ? "Yes" : "No") << "\n";
    ss << "  TTL: " << m_currentConfig.defaultTTL << "\n";
    ss << "  TCP Congestion: " << m_currentConfig.tcpCongestionControl << "\n";
    ss << "  Window Scaling: " << m_currentConfig.tcpWindowScaling << "\n";
    ss << "  Timestamps: " << m_currentConfig.tcpTimestamps << "\n";
    ss << "  SACK: " << m_currentConfig.tcpSACK << "\n";
    ss << "  MTU: " << m_currentConfig.mtuSize;
    
    result.success = true;
    result.message = ss.str();
    
    return result;
}

void NetworkStackSpoofer::applyAllChanges() {
    // Apply all network stack changes
}

void NetworkStackSpoofer::restoreOriginalSettings() {
    auto& adb = ADBManager::getInstance();
    
    // Restore iptables TTL rules
    adb.executeShellCommand("iptables -t mangle -F");
    
    m_modifiedSettings.clear();
    m_spoofingActive = false;
}

int NetworkStackSpoofer::generateRandomMACOctet(bool multicast) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    int octet = dis(gen);
    
    if (multicast) {
        octet |= 0x01;
    } else {
        octet &= 0xFE;
    }
    
    return octet;
}

}
