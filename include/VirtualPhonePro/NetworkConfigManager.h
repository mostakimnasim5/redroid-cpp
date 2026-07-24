#pragma once
#ifndef VIRTUALPHONEPRO_NETWORK_CONFIG_MANAGER_H
#define VIRTUALPHONEPRO_NETWORK_CONFIG_MANAGER_H
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QStringList>
#include "VirtualPhonePro/NetworkConfig.h"

namespace VirtualPhonePro {

/**
 * @brief Network isolation modes
 */
enum class NetworkMode {
    Default,           // Default Docker bridge (not isolated)
    IsolatedBridge,    // Isolated bridge network per instance
    Proxy,             // Route through SOCKS5/HTTP proxy
    VPN                // Route through WireGuard/OpenVPN
};

/**
 * @brief Proxy configuration for network isolation
 */
struct ProxyConfig {
    QString type;           // "socks5", "http", "https"
    QString host;           // Proxy server hostname/IP
    int port;               // Proxy port (default: 1080)
    QString username;       // Optional authentication
    QString password;       // Optional authentication
    QString noProxy;        // Bypass list (comma-separated)
    
    ProxyConfig()
        : type("socks5")
        , port(1080)
    {}
    
    bool isValid() const {
        return !host.isEmpty() && port > 0 && port < 65536;
    }
    
    QString toUrl() const {
        QString auth = username.isEmpty() ? "" : 
            QString("%1:%2@").arg(username).arg(password);
        return QString("%1://%2%3:%4")
            .arg(type)
            .arg(auth)
            .arg(host)
            .arg(port);
    }
};

/**
 * @brief VPN configuration for network isolation
 */
struct VPNConfig {
    QString type;            // "wireguard", "openvpn"
    QString configPath;     // Path to VPN config file
    QString endpoint;       // VPN server endpoint (host:port)
    QString publicKey;      // Server public key
    QString privateKey;     // Client private key
    QString address;        // Client VPN address (e.g., 10.0.0.2/24)
    int mtu;                // MTU size (default: 1420)
    QList<QString> dns;     // DNS servers
    QList<QString> allowedIPs;  // Routes to include
    
    VPNConfig()
        : type("wireguard")
        , mtu(1420)
    {
        dns = {"8.8.8.8", "1.1.1.1"};
        allowedIPs = {"0.0.0.0/0", "::/0"};
    }
    
    bool isValid() const {
        return !configPath.isEmpty() && !endpoint.isEmpty();
    }
};

/**
 * @brief Network isolation configuration
 */
struct NetworkIsolationConfig {
    // Isolation mode
    NetworkMode mode;
    
    // For IsolatedBridge mode
    QString networkName;
    QString bridgeName;
    QString subnet;
    QString gateway;
    QList<QString> dnsServers;
    
    // For Proxy mode
    ProxyConfig proxy;
    
    // For VPN mode
    VPNConfig vpn;
    
    // Leak prevention settings
    bool blockIPv6;
    bool enforceDNS;
    bool enableFirewall;
    bool spoofMAC;
    bool preventWebRTC;
    
    NetworkIsolationConfig()
        : mode(NetworkMode::IsolatedBridge)
        , blockIPv6(true)
        , enforceDNS(true)
        , enableFirewall(true)
        , spoofMAC(true)
        , preventWebRTC(true)
    {
        dnsServers = {"8.8.8.8", "1.1.1.1"};
    }
    
    bool isValid() const {
        if (mode == NetworkMode::Proxy && !proxy.isValid()) {
            return false;
        }
        if (mode == NetworkMode::VPN && !vpn.isValid()) {
            return false;
        }
        return true;
    }
};

/**
 * @brief Network statistics for an instance
 */
struct NetworkStats {
    quint64 bytesReceived;
    quint64 bytesSent;
    quint64 packetsReceived;
    quint64 packetsSent;
    QString ipAddress;
    QString macAddress;
    QString networkName;
    qint64 timestamp;
};


/**
 * @brief NetworkConfig - Network configuration manager singleton
 */
class NetworkConfigManager {
public:
    static NetworkConfigManager& instance();
    explicit NetworkConfigManager(QObject* parent = nullptr);
    ~NetworkConfigManager();

    bool configureProxy(const QString& instanceId, const ProxyConfig& proxy);
    bool removeProxy(const QString& instanceId);
    bool spoofMacAddress(const QString& instanceId, const QString& interface,
                         const QString& mac);
    QString getMacAddress(const QString& instanceId, const QString& interface);
    bool setupVPN(const QString& instanceId, const VPNConfig& vpn);
    bool disconnectVPN(const QString& instanceId);
    bool blockIPv6(const QString& instanceId);
    bool enableIPv6(const QString& instanceId);
    bool setDNSServers(const QString& instanceId, const QStringList& servers);
    QStringList getDNSServers(const QString& instanceId);
    bool testNetworkLeak(const QString& instanceId);
    QJsonObject getNetworkInfo(const QString& instanceId);

private:
    static NetworkConfigManager* s_instance;
    QMap<QString, ProxyConfig> m_proxyConfigs;
    QMap<QString, VPNConfig> m_vpnConfigs;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_NETWORK_CONFIG_MANAGER_H
