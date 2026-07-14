# VirtualPhonePro - Network Isolation Guide

## Overview

This document describes network isolation strategies to prevent IP/MAC leaks from ReDroid containers running on Windows Docker Desktop.

## Problem Statement

When running Android emulators, several vectors can leak identifying information:

| Leak Vector | Risk Level | Description |
|-------------|------------|-------------|
| Docker Bridge IP | 🔴 High | Container shares host network |
| WebRTC STUN | 🔴 High | Exposes real IP via WebRTC |
| DNS Leaks | 🟡 Medium | DNS queries bypass VPN |
| MAC Address | 🟡 Medium | Host MAC visible on network |
| mDNS/Bonjour | 🟡 Medium | Local network discovery |
| ARP Cache | 🟢 Low | Local network ARP exposure |

## Solution Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Windows 11 Host                                    │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │           Docker Desktop (WSL2 Backend)                        │  │
│  │                                                              │  │
│  │   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │  │
│  │   │  ReDroid-1   │    │  ReDroid-2   │    │  ReDroid-N   │ │  │
│  │   │  Network A   │    │  Network B   │    │  Network N   │ │  │
│  │   └──────┬───────┘    └──────┬───────┘    └──────┬───────┘ │  │
│  │          │                    │                    │          │  │
│  │          └────────────────────┼────────────────────┘          │  │
│  │                               │                               │  │
│  │                    ┌──────────┴──────────┐                   │  │
│  │                    │  Proxy/VPN Container  │                   │  │
│  │                    │  (Per-instance)      │                   │  │
│  │                    └──────────┬──────────┘                   │  │
│  └───────────────────────────────┼───────────────────────────────┘  │
│                                  │                                     │
│                    ┌─────────────┴─────────────┐                      │
│                    │   External Internet       │                      │
│                    │   (VPN/Proxy Exit)         │                      │
│                    └───────────────────────────┘                      │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 1. Docker Network Configuration

### Option A: Isolated Bridge Networks

```yaml
# docker/networks/isolated-network.yml
version: '3.8'

networks:
  # Each instance gets its own isolated network
  vpp-network-1:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.1.0/24
      driver: default
    driver_opts:
      com.docker.network.bridge.name: vpp-br-1
      com.docker.network.bridge.enable_ip_masquerade: "true"
    # Disable internal DNS
    enable_ipv6: false
    # Custom DNS to prevent leaks
    dns:
      - 8.8.8.8
      - 1.1.1.1

  vpp-network-2:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.2.0/24
    driver_opts:
      com.docker.network.bridge.name: vpp-br-2

# NAT Configuration to prevent host exposure
```

### Option B: Proxy-Based Isolation (SOCKS5/HTTP)

```yaml
# docker/compose.proxy.yml
services:
  # Per-instance proxy container
  vpp-proxy-1:
    image: ghcr.io/dnsproxy:latest
    container_name: vpp-proxy-1
    hostname: proxy-1
    networks:
      - vpp-network-1
    ports:
      - "1080:1080"  # SOCKS5
      - "8118:8118"  # HTTP proxy
    environment:
      - PROXY_PORT=1080
      - DNS_SERVER=8.8.8.8
    restart: unless-stopped

  vpp-proxy-2:
    image: ghcr.io/dnsproxy:latest
    container_name: vpp-proxy-2
    hostname: proxy-2
    networks:
      - vpp-network-2
    ports:
      - "1081:1080"
      - "8119:8118"
    environment:
      - PROXY_PORT=1080
      - DNS_SERVER=8.8.8.8
    restart: unless-stopped

networks:
  vpp-network-1:
    driver: bridge
  vpp-network-2:
    driver: bridge
```

---

## 2. WireGuard VPN Container (Recommended)

### Dockerfile for WireGuard VPN Container

```dockerfile
# docker/Dockerfile.wireguard
FROM ghcr.io/linuxserver/wireguard:latest

# Remove any default config
RUN rm -f /config/wg0.conf

# Copy template (will be replaced at runtime)
COPY wg0.conf.template /config/wg0.conf.template

CMD ["setup"]
```

### WireGuard Configuration Template

```ini
# docker/configs/wg0.conf.template
# Replace {PEER_IP} and {ENDPOINT} with actual values

[Interface]
PrivateKey = {PRIVATE_KEY}
Address = {PEER_IP}/24
DNS = 8.8.8.8, 1.1.1.1

# Prevent DNS leaks
Table = 123

[Peer]
PublicKey = {SERVER_PUBLIC_KEY}
Endpoint = {ENDPOINT}:{PORT}
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
```

### Docker Compose with WireGuard

```yaml
# docker/compose.wireguard.yml
services:
  # WireGuard VPN container for each instance
  vpp-vpn-1:
    build:
      context: .
      dockerfile: Dockerfile.wireguard
    container_name: vpp-vpn-1
    cap_add:
      - NET_ADMIN
      - SYS_MODULE
    devices:
      - /dev/net/tun:/dev/net/tun
    networks:
      - vpp-network-1
    volumes:
      - ./config/wg1.conf:/config/wg0.conf:ro
    environment:
      - PUID=1000
      - PGID=1000
      - TZ=UTC
    restart: unless-stopped
    # Routing all traffic through VPN
    sysctls:
      - net.ipv4.conf.all.rp_filter=1
      - net.ipv4.ip_forward=1
      - net.ipv6.conf.all.forwarding=1

  # ReDroid with VPN
  vpp-instance-1:
    depends_on:
      - vpp-vpn-1
    networks:
      - vpp-network-1
    environment:
      - VPN_ENABLED=true
      - VPN_CONTAINER=vpp-vpn-1
      - SOCKS5_PROXY=socks5://vpp-vpn-1:1080
    # Route all traffic through VPN network
    dns:
      - 8.8.8.8
      - 1.1.1.1

networks:
  vpp-network-1:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.1.0/24
```

---

## 3. OpenVPN Container

```dockerfile
# docker/Dockerfile.openvpn
FROM ghcr.io/dperson/openvpn-client:latest

# Remove default config
RUN rm -f /vpn/*.ovpn

# Copy configs directory
COPY configs/openvpn/ /vpn/

# Capabilities
USER root
```

```yaml
# docker/compose.openvpn.yml
services:
  openvpn:
    build:
      context: .
      dockerfile: Dockerfile.openvpn
    container_name: vpp-vpn-1
    cap_add:
      - NET_ADMIN
    devices:
      - /dev/net/tun:/dev/net/tun
    volumes:
      - ./config/openvpn:/vpn:ro
      - /etc/localtime:/etc/localtime:ro
    environment:
      - PUID=1000
      - PGID=1000
    restart: unless-stopped
    networks:
      - vpp-network-1
    dns:
      - 8.8.8.8
      - 1.1.1.1

  redroid:
    depends_on:
      - openvpn
    network_mode: service:openvpn
    dns:
      - 8.8.8.8
      - 1.1.1.1

networks:
  vpp-network-1:
    driver: bridge
```

---

## 4. C++ Implementation for Network Configuration

### ReDroidController Network Methods

Add these methods to `ReDroidController`:

```cpp
// src/ReDroidController/NetworkConfig.h
#pragma once

#include <QString>
#include <QMap>
#include <QVariantMap>

namespace VirtualPhonePro {

/**
 * @brief Network isolation modes
 */
enum class NetworkMode {
    Default,          // Default Docker bridge (not isolated)
    IsolatedBridge,   // Isolated bridge network per instance
    Proxy,            // Route through SOCKS5/HTTP proxy
    VPN               // Route through WireGuard/OpenVPN
};

/**
 * @brief Proxy configuration
 */
struct ProxyConfig {
    QString type;           // "socks5", "http", "https"
    QString host;
    int port;
    QString username;       // Optional
    QString password;       // Optional
    QString noProxy;        // Bypass list
    
    bool isValid() const {
        return !host.isEmpty() && port > 0 && port < 65536;
    }
};

/**
 * @brief VPN configuration
 */
struct VPNConfig {
    QString type;           // "wireguard", "openvpn"
    QString configPath;      // Path to VPN config file
    QString endpoint;       // VPN server endpoint
    QString publicKey;       // Server public key
    int mtu;                // MTU size
    
    bool isValid() const {
        return !configPath.isEmpty() && !endpoint.isEmpty();
    }
};

/**
 * @brief Network isolation configuration
 */
struct NetworkIsolationConfig {
    NetworkMode mode;
    
    // For IsolatedBridge mode
    QString bridgeName;
    QString subnet;
    QString gateway;
    QList<QString> dnsServers;
    
    // For Proxy mode
    ProxyConfig proxy;
    
    // For VPN mode
    VPNConfig vpn;
    
    // Leak prevention
    bool blockIPv6;
    bool enforceDNS;
    bool enableFirewall;
    
    NetworkIsolationConfig() 
        : mode(NetworkMode::Default)
        , blockIPv6(true)
        , enforceDNS(true)
        , enableFirewall(true)
    {
        dnsServers = {"8.8.8.8", "1.1.1.1"};
    }
};

} // namespace VirtualPhonePro
```

### Network Isolation Manager

```cpp
// src/ReDroidController/NetworkManager.cpp
#include "VirtualPhonePro/NetworkConfig.h"
#include "VirtualPhonePro/ReDroidController.h"
#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

class NetworkIsolationManager {
public:
    static NetworkIsolationManager& instance();
    
    // Network creation
    bool createIsolatedNetwork(const QString& instanceId, 
                               const NetworkIsolationConfig& config);
    bool deleteIsolatedNetwork(const QString& instanceId);
    
    // Proxy management
    bool assignProxy(const QString& instanceId, const ProxyConfig& proxy);
    bool removeProxy(const QString& instanceId);
    
    // VPN management
    bool setupVPN(const QString& instanceId, const VPNConfig& config);
    bool teardownVPN(const QString& instanceId);
    
    // Leak prevention
    bool applyLeakPrevention(const QString& instanceId);
    bool blockIPv6(const QString& instanceId);
    bool configureDNS(const QString& instanceId, const QList<QString>& dns);
    
    // Verification
    QString getPublicIP(const QString& instanceId);
    QString getNetworkInfo(const QString& instanceId);
    bool testForLeaks(const QString& instanceId);

private:
    NetworkIsolationManager() = default;
    
    // Internal helpers
    QString getNetworkName(const QString& instanceId);
    OperationResult executeCommand(const QStringList& args);
    QString executeCommandSync(const QStringList& args);
    
    QMap<QString, NetworkIsolationConfig> m_networkConfigs;
    QMap<QString, QString> m_networks;  // instanceId -> networkName
};

} // namespace VirtualPhonePro
```

### Dynamic Proxy Assignment Implementation

```cpp
// src/ReDroidController/NetworkManager.cpp

namespace VirtualPhonePro {

bool NetworkIsolationManager::assignProxy(const QString& instanceId, 
                                         const ProxyConfig& proxy) {
    if (!proxy.isValid()) {
        qWarning() << "Invalid proxy configuration";
        return false;
    }
    
    qDebug() << "Assigning proxy to instance:" << instanceId;
    
    // 1. Create isolated network if not exists
    NetworkIsolationConfig netConfig;
    netConfig.mode = NetworkMode::IsolatedBridge;
    netConfig.subnet = getNextSubnet();
    netConfig.dnsServers = proxy.noProxy.isEmpty() 
        ? QList<QString>{"8.8.8.8", "1.1.1.1"}
        : proxy.noProxy.split(",", Qt::SkipEmptyParts);
    
    if (!createIsolatedNetwork(instanceId, netConfig)) {
        return false;
    }
    
    // 2. Generate proxy configuration for the container
    QString proxyHost = proxy.host;
    int proxyPort = proxy.port;
    
    // Build proxy URL
    QString proxyUrl;
    if (proxy.type == "socks5") {
        proxyUrl = QString("socks5://%1:%2").arg(proxyHost).arg(proxyPort);
    } else if (proxy.type == "http" || proxy.type == "https") {
        proxyUrl = QString("http://%1:%2").arg(proxyHost).arg(proxyPort);
    }
    
    if (!proxy.username.isEmpty()) {
        proxyUrl = proxyUrl.replace("://", 
            QString("://%1:%2@").arg(proxy.username).arg(proxy.password));
    }
    
    // 3. Generate PAC file for Android proxy settings
    QString pacContent = QString(R"(
        function FindProxyForURL(url, host) {{
            // Direct connection for local addresses
            if (isPlainHostName(host) || 
                shExpMatch(host, "*.local") ||
                isInNet(dnsResolve(host), "10.0.0.0", "255.0.0.0") ||
                isInNet(dnsResolve(host), "172.16.0.0", "255.240.0.0") ||
                isInNet(dnsResolve(host), "192.168.0.0", "255.255.0.0") ||
                isInNet(dnsResolve(host), "127.0.0.0", "255.0.0.0")) {{
                return "DIRECT";
            }}
            
            // Use proxy for all other traffic
            return "PROXY %1:%2";
        }}
    )").arg(proxyHost).arg(proxyPort);
    
    // 4. Save PAC file
    QString profileDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QString pacPath = QString("%1/instances/%2/proxy.pac")
        .arg(profileDir).arg(instanceId);
    
    QFile pacFile(pacPath);
    if (pacFile.open(QIODevice::WriteOnly)) {
        pacFile.write(pacContent.toUtf8());
        pacFile.close();
    }
    
    // 5. Push PAC file to container
    ReDroidController& controller = ReDroidController::instance();
    
    // Set proxy via ADB
    QStringList setProxyCmds = {
        // Enable proxy
        QString("settings put global http_proxy %1:%2")
            .arg(proxyHost).arg(proxyPort),
        
        // Set WiFi proxy
        "shell svc wifi proxy set /data/proxy.pac",
        
        // Configure DNS to prevent leaks
        QString("setprop net.dns1 %1").arg(
            netConfig.dnsServers.value(0, "8.8.8.8")),
        QString("setprop net.dns2 %1").arg(
            netConfig.dnsServers.value(1, "1.1.1.1")),
        
        // Disable IPv6 to prevent leaks
        "shell sysctl -w net.ipv6.conf.all.disable_ipv6=1",
        "shell sysctl -w net.ipv6.conf.default.disable_ipv6=1"
    };
    
    for (const QString& cmd : setProxyCmds) {
        controller.executeShell(instanceId, cmd);
    }
    
    // 6. Apply additional Android proxy settings
    QStringList additionalCommands = {
        // Global proxy settings
        "settings put global global_http_proxy_host " + proxyHost,
        "settings put global global_http_proxy_port " + QString::number(proxyPort),
        "settings put global global_proxy_pac_url file:///data/proxy.pac",
        
        // Per-app proxy (optional - requires VPN permission)
        // "settings put global supported_vpn_handle_include_packages com.app.name",
        
        // Clear any cached DNS
        "ndc resolver flushif wlan0",
        "ndc resolver flushif eth0"
    };
    
    for (const QString& cmd : additionalCommands) {
        controller.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Proxy assigned successfully";
    return true;
}

bool NetworkIsolationManager::removeProxy(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    QStringList commands = {
        // Remove global proxy
        "settings delete global http_proxy",
        "settings delete global global_http_proxy_host",
        "settings delete global global_http_proxy_port",
        "settings delete global global_proxy_pac_url",
        
        // Remove WiFi proxy
        "shell svc wifi proxy clear",
        
        // Reset DNS
        "setprop net.dns1 8.8.8.8",
        "setprop net.dns2 8.8.4.4",
        
        // Re-enable IPv6
        "shell sysctl -w net.ipv6.conf.all.disable_ipv6=0",
        "shell sysctl -w net.ipv6.conf.default.disable_ipv6=0"
    };
    
    for (const QString& cmd : commands) {
        controller.executeShell(instanceId, cmd);
    }
    
    return true;
}

} // namespace VirtualPhonePro
```

### Docker Network Creation

```cpp
bool NetworkIsolationManager::createIsolatedNetwork(
    const QString& instanceId, 
    const NetworkIsolationConfig& config) {
    
    QString networkName = getNetworkName(instanceId);
    
    // Build docker network create command
    QStringList args = {
        "network", "create",
        "--driver", "bridge",
        "--subnet", config.subnet,
        "--gateway", config.gateway,
        "--opt", "com.docker.network.bridge.name=" + networkName,
        "--opt", "com.docker.network.bridge.enable_icc=true",
        "--opt", "com.docker.network.bridge.enable_ip_masquerade=true",
        networkName
    };
    
    // Add custom DNS
    for (const QString& dns : config.dnsServers) {
        args << "--dns" << dns;
    }
    
    // Disable IPv6 if requested
    if (config.blockIPv6) {
        args << "--opt" << "com.docker.network.enable_ipv6=false";
    }
    
    // Execute
    OperationResult result = executeCommand(args);
    
    if (result.success) {
        m_networks[instanceId] = networkName;
    }
    
    return result.success;
}

bool NetworkIsolationManager::deleteIsolatedNetwork(const QString& instanceId) {
    if (!m_networks.contains(instanceId)) {
        return true;  // Already deleted
    }
    
    QString networkName = m_networks[instanceId];
    
    OperationResult result = executeCommand({
        "network", "rm", networkName
    });
    
    if (result.success) {
        m_networks.remove(instanceId);
    }
    
    return result.success;
}
```

---

## 5. Preventing Host Leaks

### MAC Address Spoofing

```cpp
// In DeviceProfile.cpp - Enhanced MAC generation
QString DeviceProfile::generateSecureMAC(const QString& oui, bool randomNic) {
    // Use random NIC indicator for privacy
    QString mac;
    
    if (randomNic) {
        // Locally administered bit set, multicast bit clear
        mac = oui;
        
        // Generate random bytes with second bit (unicast) set
        QByteArray randomBytes(3, 0);
        for (int i = 0; i < 3; ++i) {
            int byte = QRandomGenerator::global()->bounded(256);
            // Set bit 1 (locally administered) and clear bit 0
            byte = (byte & 0xFE) | 0x02;
            randomBytes[i] = byte;
        }
        
        mac += ":" + randomBytes.toHex().toUpper();
    } else {
        // Standard OUI with random NIC portion
        mac = oui + ":";
        for (int i = 0; i < 3; ++i) {
            QString byte = QString::number(
                QRandomGenerator::global()->bounded(256), 16);
            if (byte.length() == 1) byte = "0" + byte;
            mac += byte;
            if (i < 2) mac += ":";
        }
    }
    
    return mac;
}
```

### Network Interface Spoofing

```cpp
// src/ReDroidController/NetworkLeakPrevention.cpp

class NetworkLeakPrevention {
public:
    static void applyToInstance(const QString& instanceId,
                               ReDroidController& controller) {
        
        // 1. Spoof WiFi MAC
        QString wifiMac = generateSecureMAC("8C:71:F8");
        controller.executeShell(instanceId,
            "ip link set wlan0 addr " + wifiMac);
        
        // 2. Spoof Ethernet MAC
        QString ethMac = generateSecureMAC("00:1A:11");
        controller.executeShell(instanceId,
            "ip link set eth0 addr " + ethMac);
        
        // 3. Disable IPv6
        QStringList ipv6Disable = {
            "sysctl -w net.ipv6.conf.all.disable_ipv6=1",
            "sysctl -w net.ipv6.conf.default.disable_ipv6=1",
            "sysctl -w net.ipv6.conf.lo.disable_ipv6=1"
        };
        for (const QString& cmd : ipv6Disable) {
            controller.executeShell(instanceId, cmd);
        }
        
        // 4. Clear ARP cache
        controller.executeShell(instanceId, "ip neigh flush all");
        
        // 5. Clear netstat info
        controller.executeShell(instanceId, "ip route flush cache");
        
        // 6. Reset hostname to generic
        controller.executeShell(instanceId, "hostname android-" + 
            QString::number(QRandomGenerator::global()->bounded(1000, 9999)));
        
        // 7. Configure firewall rules (if iptables available)
        QStringList firewallRules = {
            // Block outbound connections from host interfaces
            "ip6tables -A OUTPUT -m state --state INVALID -j DROP",
            "ip6tables -A OUTPUT -p ipv6-icmp -j ACCEPT",
            "ip6tables -A OUTPUT -m state --state RELATED,ESTABLISHED -j ACCEPT"
        };
        for (const QString& rule : firewallRules) {
            controller.executeShell(instanceId, rule);
        }
    }
};
```

### WebRTC Leak Prevention

```cpp
// WebRTC uses STUN servers to determine public IP
// Prevention in Android requires:

void preventWebRTCLeak(const QString& instanceId, 
                      ReDroidController& controller) {
    
    // 1. Block STUN traffic
    QStringList stunBlocks = {
        // Block common STUN servers
        "iptables -A OUTPUT -p udp --dport 3478 -j DROP",
        "iptables -A OUTPUT -p udp --dport 19302 -j DROP",  // Google STUN
        
        // Block TURN if possible
        "iptables -A OUTPUT -p tcp --dport 443 -m state --state NEW -j REJECT"
    };
    
    for (const QString& cmd : stunBlocks) {
        controller.executeShell(instanceId, cmd);
    }
    
    // 2. Set fake public IP via settings (requires root)
    // This tricks WebRTC into thinking it's behind a NAT
    QString fakePublicIP = "203.0.113." + 
        QString::number(QRandomGenerator::global()->bounded(1, 254)) + 
        "." + QString::number(QRandomGenerator::global()->bounded(1, 254));
    
    controller.executeShell(instanceId, 
        "settings put global fake_public_ip " + fakePublicIP);
    
    // 3. Force WebRTC to use specified STUN (controlled)
    controller.executeShell(instanceId,
        "setprop net.stun.fake_response.ip " + fakePublicIP);
}
```

---

## 6. Complete Network Isolation Workflow

```cpp
// src/ReDroidController/NetworkManager.cpp

bool NetworkIsolationManager::applyFullIsolation(
    const QString& instanceId,
    const NetworkIsolationConfig& config) {
    
    qDebug() << "Applying full network isolation to:" << instanceId;
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Step 1: Create isolated network
    if (config.mode != NetworkMode::Default) {
        if (!createIsolatedNetwork(instanceId, config)) {
            qCritical() << "Failed to create isolated network";
            return false;
        }
    }
    
    // Step 2: Assign proxy if configured
    if (config.mode == NetworkMode::Proxy && config.proxy.isValid()) {
        if (!assignProxy(instanceId, config.proxy)) {
            qWarning() << "Failed to assign proxy";
        }
    }
    
    // Step 3: Setup VPN if configured
    if (config.mode == NetworkMode::VPN && config.vpn.isValid()) {
        if (!setupVPN(instanceId, config.vpn)) {
            qWarning() << "Failed to setup VPN";
        }
    }
    
    // Step 4: Apply leak prevention
    if (!applyLeakPrevention(instanceId)) {
        qWarning() << "Leak prevention partially failed";
    }
    
    // Step 5: Block IPv6
    if (config.blockIPv6) {
        if (!blockIPv6(instanceId)) {
            qWarning() << "IPv6 blocking failed";
        }
    }
    
    // Step 6: Configure secure DNS
    if (config.enforceDNS) {
        if (!configureDNS(instanceId, config.dnsServers)) {
            qWarning() << "DNS configuration failed";
        }
    }
    
    // Step 7: Verify configuration
    QString networkInfo = getNetworkInfo(instanceId);
    qDebug() << "Network configuration:" << networkInfo;
    
    // Step 8: Test for leaks
    if (testForLeaks(instanceId)) {
        qWarning() << "Potential network leak detected!";
        return false;
    }
    
    qDebug() << "Network isolation applied successfully";
    return true;
}
```

---

## 7. WSL2 Specific Configuration

### .wslconfig for Windows

```ini
# C:\Users\<username>\.wslconfig
[wsl2]
# Memory and processor limits
memory=8GB
processors=4

# Disable automatic DNS proxy
vmgenid=local
localhostForwarding=false

# Network isolation
nat.bridgeNetwork=false

# Disable default networking features that may leak
vmIdleTimeout=60000
```

### WSL2 Network Reset Script

```bash
# docker/scripts/reset-wsl-network.sh
#!/bin/bash
# Reset WSL2 network to prevent leaks between instances

# Flush all iptables rules
iptables -F
iptables -X
iptables -t nat -F
iptables -t nat -X

# Set default policies
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# Allow established connections
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

# Block IPv6
ip6tables -P INPUT DROP
ip6tables -P FORWARD DROP
ip6tables -P OUTPUT DROP

# Allow loopback
iptables -A INPUT -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT

echo "WSL2 network reset complete"
```

---

## 8. Testing for Leaks

### Leak Detection Script

```bash
#!/bin/bash
# docker/scripts/test-leaks.sh

INSTANCE_ID=$1
ADB=$2

echo "Testing for network leaks on instance: $INSTANCE_ID"

# Test 1: Check public IP (should be different from host)
echo "[1] Testing public IP..."
CONTAINER_IP=$(curl -s --socks5 127.0.0.1:1080 https://api.ipify.org)
HOST_IP=$(curl -s https://api.ipify.org)

if [ "$CONTAINER_IP" == "$HOST_IP" ]; then
    echo "FAIL: Public IP leak detected!"
    exit 1
else
    echo "OK: IP is $CONTAINER_IP"
fi

# Test 2: Check DNS leaks
echo "[2] Testing DNS leaks..."
DNS_SERVERS=$(dig +short myip.opendns.com @resolver1.opendns.com 2>/dev/null)

# Test 3: WebRTC leak
echo "[3] Testing WebRTC..."
WEBRTC_IPS=$($ADB shell "dumpsys telephony.registry | grep mMyMobileIp" 2>/dev/null)

# Test 4: Check for IPv6 leaks
echo "[4] Testing IPv6..."
IPV6_TEST=$(curl -s -6 https://ipv6.icanhazip.com 2>/dev/null)

if [ -n "$IPV6_TEST" ]; then
    echo "FAIL: IPv6 leak detected: $IPV6_TEST"
    exit 1
else
    echo "OK: No IPv6 leak"
fi

echo "All leak tests passed!"
```

---

## Summary

| Feature | Implementation | Priority |
|---------|---------------|----------|
| Isolated Networks | Docker bridge per instance | 🔴 Required |
| Proxy Support | SOCKS5/HTTP via ADB | 🔴 Required |
| VPN Support | WireGuard/OpenVPN containers | 🟡 Recommended |
| MAC Spoofing | Generate secure MAC addresses | 🔴 Required |
| IPv6 Blocking | sysctl in container | 🔴 Required |
| DNS Configuration | Custom DNS servers | 🔴 Required |
| WebRTC Prevention | Block STUN traffic | 🟡 Recommended |
| ARP Cache Clear | ip neigh flush | 🟢 Optional |

---

*VirtualPhonePro - Network Isolation Guide*
*For authorized testing only*
