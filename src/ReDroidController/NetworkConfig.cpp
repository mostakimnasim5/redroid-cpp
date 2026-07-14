/**
 * @file NetworkConfig.cpp
 * @brief Network Configuration Implementation
 * @version 2.0.0
 * 
 * Handles network isolation and proxy configuration.
 */

#include "VirtualPhonePro/NetworkConfig.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

NetworkConfig* NetworkConfig::s_instance = nullptr;

NetworkConfig& NetworkConfig::instance() {
    if (!s_instance) {
        s_instance = new NetworkConfig();
    }
    return *s_instance;
}

NetworkConfig::NetworkConfig(QObject* parent)
    : QObject(parent)
{
}

NetworkConfig::~NetworkConfig() {
}

bool NetworkConfig::configureProxy(const QString& instanceId, const ProxyConfig& proxy) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set HTTP proxy
    if (!proxy.httpProxy.isEmpty()) {
        QString cmd = QString("settings put global http_proxy %1:%2")
                          .arg(proxy.httpProxy)
                          .arg(proxy.httpPort);
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Set HTTPS proxy
    if (!proxy.httpsProxy.isEmpty()) {
        QString cmd = QString("settings put global https_proxy %1:%2")
                          .arg(proxy.httpsProxy)
                          .arg(proxy.httpsPort);
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Set proxy exclusions
    if (!proxy.exclusionList.isEmpty()) {
        QString exclusions = proxy.exclusionList.join(",");
        QString cmd = QString("settings put global global_http_proxy_exclusion_list %1")
                          .arg(exclusions);
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "[NetworkConfig] Proxy configured for:" << instanceId;
    return true;
}

bool NetworkConfig::removeProxy(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        "settings put global http_proxy :0",
        "settings put global https_proxy :0",
        "settings put global global_http_proxy_exclusion_list \"\""
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool NetworkConfig::spoofMacAddress(const QString& instanceId, const QString& interface, 
                                    const QString& macAddress) {
    if (instanceId.isEmpty() || macAddress.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Bring interface down
    QString cmdDown = QString("ip link set %1 down").arg(interface);
    ctrl.executeShell(instanceId, cmdDown);
    
    // Set MAC address
    QString cmdMac = QString("ip link set %1 address %2").arg(interface).arg(macAddress);
    ctrl.executeShell(instanceId, cmdMac);
    
    // Bring interface up
    QString cmdUp = QString("ip link set %1 up").arg(interface);
    ctrl.executeShell(instanceId, cmdUp);
    
    qDebug() << "[NetworkConfig] MAC spoofed:" << macAddress << "on" << interface;
    return true;
}

QString NetworkConfig::getMacAddress(const QString& instanceId, const QString& interface) {
    if (instanceId.isEmpty()) {
        return QString();
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = QString("ip link show %1 | grep ether").arg(interface);
    QString result = ctrl.executeShell(instanceId, cmd);
    
    // Parse MAC address
    QStringList parts = result.split(" ");
    for (const QString& part : parts) {
        if (part.contains(":") && part.length() == 17) {
            return part;
        }
    }
    
    return QString();
}

bool NetworkConfig::setupVPN(const QString& instanceId, const VPNConfig& vpn) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Install VPN profile
    if (!vpn.configPath.isEmpty()) {
        QString remotePath = "/data/local/tmp/vpn.conf";
        ctrl.pushFile(instanceId, vpn.configPath, remotePath);
    }
    
    // Configure VPN settings
    QStringList commands = {
        // Set VPN DNS
        QString("settings put global vpn_dns %1").arg(vpn.dnsServers.join(",")),
        
        // Enable VPN
        "svc vpn enable"
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool NetworkConfig::disconnectVPN(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString cmd = "svc vpn disable";
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

bool NetworkConfig::blockIPv6(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Disable IPv6
    QStringList commands = {
        "sysctl -w net.ipv6.conf.all.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.wlan0.disable_ipv6=1"
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool NetworkConfig::enableIPv6(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Enable IPv6
    QStringList commands = {
        "sysctl -w net.ipv6.conf.all.disable_ipv6=0",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=0"
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool NetworkConfig::setDNSServers(const QString& instanceId, const QStringList& servers) {
    if (instanceId.isEmpty() || servers.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set net.dns1 and net.dns2
    QStringList commands;
    for (int i = 0; i < qMin(servers.size(), 4); i++) {
        commands.append(QString("setprop net.dns%1 %2").arg(i + 1).arg(servers[i]));
    }
    
    // Also set in resolv.conf style
    QString dnsContent = servers.join("\n");
    QString cmd = QString("echo '%1' > /system/etc/resolv.conf").arg(dnsContent);
    commands.append(cmd);
    
    for (const QString& c : commands) {
        ctrl.executeShell(instanceId, c);
    }
    
    return true;
}

QStringList NetworkConfig::getDNSServers(const QString& instanceId) {
    QStringList servers;
    
    if (instanceId.isEmpty()) {
        return servers;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Read DNS from getprop
    for (int i = 1; i <= 4; i++) {
        QString cmd = QString("getprop net.dns%1").arg(i);
        QString result = ctrl.executeShell(instanceId, cmd).trimmed();
        if (!result.isEmpty() && !servers.contains(result)) {
            servers.append(result);
        }
    }
    
    return servers;
}

bool NetworkConfig::testNetworkLeak(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Test for DNS leaks
    QString cmd = "getprop net.dns1";
    QString dns1 = ctrl.executeShell(instanceId, cmd).trimmed();
    
    // Test for IPv6 leaks
    cmd = "ip -6 route";
    QString ipv6Routes = ctrl.executeShell(instanceId, cmd);
    
    bool hasIpv6 = !ipv6Routes.isEmpty() && !ipv6Routes.contains("default via");
    bool hasDNS = !dns1.isEmpty();
    
    // Return true if no leaks detected
    return !hasIpv6 && !hasDNS;
}

QJsonObject NetworkConfig::getNetworkInfo(const QString& instanceId) {
    QJsonObject info;
    
    if (instanceId.isEmpty()) {
        return info;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Get IP addresses
    QString cmd = "ip addr show wlan0";
    QString ipResult = ctrl.executeShell(instanceId, cmd);
    
    // Extract IPv4
    QRegExp ipv4Regex("inet (\\d+\\.\\d+\\.\\d+\\.\\d+)");
    if (ipv4Regex.indexIn(ipResult) != -1) {
        info["ipv4"] = ipv4Regex.cap(1);
    }
    
    // Extract IPv6
    QRegExp ipv6Regex("inet6 ([a-f0-9:]+)");
    if (ipv6Regex.indexIn(ipResult) != -1) {
        info["ipv6"] = ipv6Regex.cap(1);
    }
    
    // Get MAC address
    info["mac"] = getMacAddress(instanceId, "wlan0");
    
    // Get DNS servers
    info["dns"] = QJsonArray::fromStringList(getDNSServers(instanceId));
    
    // Get gateway
    QString gatewayCmd = "ip route show default";
    QString gatewayResult = ctrl.executeShell(instanceId, gatewayCmd);
    QRegExp gatewayRegex("default via (\\d+\\.\\d+\\.\\d+\\.\\d+)");
    if (gatewayRegex.indexIn(gatewayResult) != -1) {
        info["gateway"] = gatewayRegex.cap(1);
    }
    
    return info;
}

} // namespace VirtualPhonePro
