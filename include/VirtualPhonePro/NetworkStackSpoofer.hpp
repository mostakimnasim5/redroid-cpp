#pragma once

#include <string>
#include <map>
#include <vector>

namespace AntiDetect {

struct NetworkStackConfig {
    std::string tcpCongestionControl;
    std::string tcpWindowScaling;
    std::string mtuSize;
    std::string tcpTimestamps;
    std::string tcpSACK;
    std::string ipForward;
    std::string tcpFastOpen;
    
    // TTL Settings
    int defaultTTL;
    int wifiTTL;
    int mobileTTL;
    
    // Window Size
    int tcpRmemMin;
    int tcpRmemDefault;
    int tcpRmemMax;
    int tcpWmemMin;
    int tcpWmemDefault;
    int tcpWmemMax;
};

struct NetworkSpoofResult2 {
    bool success;
    std::string message;
    std::string error;
    std::map<std::string, std::string> details;
};

class NetworkStackSpoofer {
public:
    static NetworkStackSpoofer& getInstance();
    
    NetworkStackSpoofer();
    ~NetworkStackSpoofer();
    
    bool initialize();
    bool isInitialized() const;
    void shutdown();
    
    // TCP/IP Stack Spoofing
    NetworkSpoofResult2 enableStackSpoofing();
    NetworkSpoofResult2 disableStackSpoofing();
    
    // TCP Congestion Control
    NetworkSpoofResult2 setCongestionControl(const std::string& algorithm);
    NetworkSpoofResult2 setCubicProfile();
    NetworkSpoofResult2 setBbrProfile();
    NetworkSpoofResult2 setRenoProfile();
    NetworkSpoofResult2 setWestwoodProfile();
    
    // TTL Spoofing
    NetworkSpoofResult2 spoofTTL(int ttl);
    NetworkSpoofResult2 spoofWifiTTL();
    NetworkSpoofResult2 spoofMobileTTL();
    NetworkSpoofResult2 setDeviceTTL();  // Real device TTL (64)
    
    // TCP Window Scaling
    NetworkSpoofResult2 enableWindowScaling();
    NetworkSpoofResult2 disableWindowScaling();
    NetworkSpoofResult2 setWindowScalingFactor(int factor);
    
    // TCP Options
    NetworkSpoofResult2 enableTimestamps();
    NetworkSpoofResult2 disableTimestamps();
    NetworkSpoofResult2 enableSACK();
    NetworkSpoofResult2 disableSACK();
    NetworkSpoofResult2 enableTcpFastOpen();
    NetworkSpoofResult2 disableTcpFastOpen();
    
    // MTU Spoofing
    NetworkSpoofResult2 setMTU(int mtu);
    NetworkSpoofResult2 setWifiMTU();
    NetworkSpoofResult2 setMobileMTU();
    
    // Buffer Sizes
    NetworkSpoofResult2 setReceiveBuffer(int min, int default_val, int max);
    NetworkSpoofResult2 setSendBuffer(int min, int default_val, int max);
    NetworkSpoofResult2 optimizeBuffers();
    
    // DNS Spoofing
    NetworkSpoofResult2 setCustomDNS(const std::vector<std::string>& dnsServers);
    NetworkSpoofResult2 setGoogleDNS();
    NetworkSpoofResult2 setCloudflareDNS();
    NetworkSpoofResult2 setISPDefaultDNS();
    
    // HTTP Headers Spoofing
    NetworkSpoofResult2 spoofUserAgent(const std::string& userAgent);
    NetworkSpoofResult2 spoofAcceptLanguage(const std::string& language);
    NetworkSpoofResult2 spoofAcceptEncoding(const std::string& encoding);
    NetworkSpoofResult2 spoofHTTPVersion(const std::string& version);
    NetworkSpoofResult2 setChromeUserAgent();
    NetworkSpoofResult2 setFirefoxUserAgent();
    NetworkSpoofResult2 setSafariUserAgent();
    
    // WebRTC Spoofing
    NetworkSpoofResult2 spoofWebRTCIP(const std::string& ipAddress);
    NetworkSpoofResult2 disableWebRTC();
    NetworkSpoofResult2 enableWebRTCProxyMode();
    
    // Proxy Detection Bypass
    NetworkSpoofResult2 hideProxySettings();
    NetworkSpoofResult2 bypassProxyDetection();
    
    // SSL/TLS Spoofing
    NetworkSpoofResult2 setTLSVersion(int min, int max);
    NetworkSpoofResult2 enableTLS12();
    NetworkSpoofResult2 enableTLS13();
    NetworkSpoofResult2 setCipherSuites(const std::string& ciphers);
    
    // Network Interface Spoofing
    NetworkSpoofResult2 spoofMACAddress(const std::string& mac);
    NetworkSpoofResult2 spoofInterfaceName();
    NetworkSpoofResult2 randomizeMAC();
    NetworkSpoofResult2 setSamsungMAC();
    NetworkSpoofResult2 setAppleMAC();
    
    // Mobile Network Spoofing
    NetworkSpoofResult2 spoofMobileOperator(const std::string& name);
    NetworkSpoofResult2 spoofMobileCountryCode(int mcc);
    NetworkSpoofResult2 spoofMobileNetworkCode(int mnc);
    NetworkSpoofResult2 spoofNetworkType(const std::string& type);  // 5G, 4G, 3G
    
    // Real Device Profiles
    NetworkSpoofResult2 applySamsungNetworkProfile();
    NetworkSpoofResult2 applyGoogleNetworkProfile();
    NetworkSpoofResult2 applyAppleNetworkProfile();
    
    // Validation
    NetworkSpoofResult2 validateSpoofing();
    bool isSpoofingActive() const;
    NetworkStackConfig getCurrentConfig();
    
    // Status
    std::map<std::string, std::string> getDetailedStatus();
    NetworkSpoofResult2 getStatus();

private:
    void applyAllChanges();
    void restoreOriginalSettings();
    int generateRandomMACOctet(bool multicast = false);
    
    bool m_initialized;
    bool m_spoofingActive;
    NetworkStackConfig m_currentConfig;
    NetworkStackConfig m_originalConfig;
    
    std::map<std::string, std::string> m_modifiedSettings;
    std::map<std::string, std::string> m_originalSettings;
};

}
