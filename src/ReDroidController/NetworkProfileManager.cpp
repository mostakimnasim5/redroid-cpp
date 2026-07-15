
// ========================================================================
// DATA STRUCTURE IMPLEMENTATIONS
// ========================================================================

QJsonObject GeolocationData::toJson() const {
    QJsonObject obj;
    obj["status"] = status;
    obj["country"] = country;
    obj["countryCode"] = countryCode;
    obj["region"] = region;
    obj["regionName"] = regionName;
    obj["city"] = city;
    obj["zip"] = zip;
    obj["latitude"] = latitude;
    obj["longitude"] = longitude;
    obj["timezone"] = timezone;
    obj["isp"] = isp;
    obj["org"] = org;
    obj["as"] = asNumber;
    obj["query"] = query;
    obj["carrierName"] = carrierName;
    obj["carrierMCC"] = carrierMCC;
    obj["carrierMNC"] = carrierMNC;
    return obj;
}

void GeolocationData::fromJson(const QJsonObject& json) {
    status = json.value("status").toString();
    country = json.value("country").toString();
    countryCode = json.value("countryCode").toString();
    region = json.value("region").toString();
    regionName = json.value("regionName").toString();
    city = json.value("city").toString();
    zip = json.value("zip").toString();
    latitude = json.value("latitude").toDouble();
    longitude = json.value("longitude").toDouble();
    timezone = json.value("timezone").toString();
    isp = json.value("isp").toString();
    org = json.value("org").toString();
    asNumber = json.value("as").toString();
    query = json.value("query").toString();
    carrierName = json.value("carrierName").toString();
    carrierMCC = json.value("carrierMCC").toString();
    carrierMNC = json.value("carrierMNC").toString();
}

QList<CountryData> CountryData::getAllCountries() {
    return NetworkProfileManager::instance().getAvailableCountries();
}

CountryData CountryData::getCountry(const QString& code) {
    for (const CountryData& country : NetworkProfileManager::instance().getAvailableCountries()) {
        if (country.code == code) {
            return country;
        }
    }
    return CountryData();
}

QJsonObject CellularNetworkConfig::toJson() const {
    QJsonObject obj;
    obj["transportType"] = transportTypeStr;
    obj["operatorName"] = operatorName;
    obj["operatorNumeric"] = operatorNumeric;
    obj["simState"] = simState;
    obj["mobileDataEnabled"] = mobileDataEnabled;
    obj["dataRoamingEnabled"] = dataRoamingEnabled;
    obj["networkMode"] = networkMode;
    obj["preferredNetworkType"] = preferredNetworkType;
    obj["ipAddress"] = ipAddress;
    obj["subnetMask"] = subnetMask;
    obj["gateway"] = gateway;
    obj["dnsServers"] = QJsonArray::fromStringList(dnsServers);
    obj["connectionState"] = connectionState;
    obj["signalStrength"] = signalStrength;
    obj["cellId"] = cellId;
    obj["lac"] = lac;
    return obj;
}

QJsonObject TCPFingerprint::toJson() const {
    QJsonObject obj;
    obj["windowSize"] = windowSize;
    obj["mss"] = mss;
    obj["windowScaling"] = windowScaling;
    obj["sackOk"] = sackOk;
    obj["timestamp"] = timestamp;
    obj["selectiveAck"] = selectiveAck;
    obj["mode"] = mode;
    return obj;
}

QJsonObject DNSCOnfig::toJson() const {
    QJsonObject obj;
    obj["primary"] = QJsonArray::fromStringList(primary);
    obj["secondary"] = QJsonArray::fromStringList(secondary);
    obj["doh"] = QJsonArray::fromStringList(doh);
    obj["dot"] = QJsonArray::fromStringList(dot);
    return obj;
}

QJsonObject NetworkProfile::toJson() const {
    QJsonObject obj;
    obj["profileId"] = profileId;
    obj["instanceId"] = instanceId;
    obj["mode"] = static_cast<int>(mode);
    
    // Proxy config
    obj["proxyHost"] = proxyHost;
    obj["proxyPort"] = proxyPort;
    obj["proxyUsername"] = proxyUsername;
    obj["proxyPassword"] = proxyPassword;
    obj["proxyType"] = proxyType;
    
    // Country
    obj["countryCode"] = countryCode;
    obj["countryName"] = countryName;
    
    // Geolocation
    obj["geolocation"] = geolocation.toJson();
    
    // Cellular
    obj["cellular"] = cellular.toJson();
    
    // TCP/DNS
    obj["tcpFingerprint"] = tcpFingerprint.toJson();
    obj["dnsConfig"] = dnsConfig.toJson();
    
    // Telephony properties
    QJsonObject telephonyObj;
    for (auto it = telephonyProperties.constBegin(); it != telephonyProperties.constEnd(); ++it) {
        telephonyObj[it.key()] = it.value();
    }
    obj["telephonyProperties"] = telephonyObj;
    
    // Settings
    obj["blockIPv6"] = blockIPv6;
    obj["enforceDNS"] = enforceDNS;
    obj["spoofWebRTC"] = spoofWebRTC;
    obj["enableFirewall"] = enableFirewall;
    
    // Status
    obj["status"] = static_cast<int>(status);
    obj["lastError"] = lastError;
    obj["createdAt"] = createdAt;
    obj["appliedAt"] = appliedAt;
    
    return obj;
}

bool NetworkProfile::fromJson(const QJsonObject& json) {
    profileId = json.value("profileId").toString();
    instanceId = json.value("instanceId").toString();
    mode = static_cast<NetworkConfigMode>(json.value("mode").toInt());
    
    // Proxy config
    proxyHost = json.value("proxyHost").toString();
    proxyPort = json.value("proxyPort").toInt();
    proxyUsername = json.value("proxyUsername").toString();
    proxyPassword = json.value("proxyPassword").toString();
    proxyType = json.value("proxyType").toString();
    
    // Country
    countryCode = json.value("countryCode").toString();
    countryName = json.value("countryName").toString();
    
    // Geolocation
    geolocation.fromJson(json.value("geolocation").toObject());
    
    // Cellular
    QJsonObject cellularJson = json.value("cellular").toObject();
    cellular.operatorName = cellularJson.value("operatorName").toString();
    cellular.operatorNumeric = cellularJson.value("operatorNumeric").toString();
    cellular.simState = cellularJson.value("simState").toInt();
    cellular.mobileDataEnabled = cellularJson.value("mobileDataEnabled").toString();
    cellular.dataRoamingEnabled = cellularJson.value("dataRoamingEnabled").toString();
    cellular.networkMode = cellularJson.value("networkMode").toString();
    cellular.preferredNetworkType = cellularJson.value("preferredNetworkType").toString();
    cellular.ipAddress = cellularJson.value("ipAddress").toString();
    cellular.subnetMask = cellularJson.value("subnetMask").toString();
    cellular.gateway = cellularJson.value("gateway").toString();
    cellular.dnsServers = cellularJson.value("dnsServers").toVariant().toStringList();
    cellular.connectionState = cellularJson.value("connectionState").toString();
    cellular.signalStrength = cellularJson.value("signalStrength").toInt();
    cellular.cellId = cellularJson.value("cellId").toInt();
    cellular.lac = cellularJson.value("lac").toInt();
    
    // TCP/DNS
    QJsonObject tcpJson = json.value("tcpFingerprint").toObject();
    tcpFingerprint.windowSize = tcpJson.value("windowSize").toUInt();
    tcpFingerprint.mss = tcpJson.value("mss").toUInt();
    tcpFingerprint.windowScaling = tcpJson.value("windowScaling").toBool();
    tcpFingerprint.sackOk = tcpJson.value("sackOk").toUInt();
    tcpFingerprint.timestamp = tcpJson.value("timestamp").toUInt();
    tcpFingerprint.selectiveAck = tcpJson.value("selectiveAck").toUInt();
    tcpFingerprint.mode = tcpJson.value("mode").toString();
    
    QJsonObject dnsJson = json.value("dnsConfig").toObject();
    dnsConfig.primary = dnsJson.value("primary").toVariant().toStringList();
    dnsConfig.secondary = dnsJson.value("secondary").toVariant().toStringList();
    dnsConfig.doh = dnsJson.value("doh").toVariant().toStringList();
    dnsConfig.dot = dnsJson.value("dot").toVariant().toStringList();
    
    // Telephony properties
    QJsonObject telephonyObj = json.value("telephonyProperties").toObject();
    for (auto it = telephonyObj.constBegin(); it != telephonyObj.constEnd(); ++it) {

// ========================================================================
// CELLULAR NETWORK SPOOFING
// ========================================================================

CellularNetworkConfig NetworkProfileManager::generateCellularConfig(const NetworkProfile& profile) {
    CellularNetworkConfig config;
    
    CountryData country = CountryData::getCountry(profile.countryCode);
    QString carrier = country.carriers.isEmpty() ? "Carrier" : 
                      country.carriers[QRandomGenerator::global()->bounded(country.carriers.size())];
    
    config.operatorName = carrier;
    
    QString mcc = generateMCCFromCountry(profile.countryCode);
    QString mnc = generateMNCForCarrier(carrier, mcc);
    config.operatorNumeric = mcc + mnc;
    
    config.simState = 5;
    
    int totalWeight = 0;
    for (auto it = country.networkTypeWeights.constBegin(); 
         it != country.networkTypeWeights.constEnd(); ++it) {
        totalWeight += it.value();
    }
    
    int randomValue = QRandomGenerator::global()->bounded(totalWeight);
    int cumulativeWeight = 0;
    
    for (auto it = country.networkTypeWeights.constBegin(); 
         it != country.networkTypeWeights.constEnd(); ++it) {
        cumulativeWeight += it.value();
        if (randomValue < cumulativeWeight) {
            config.transportType = it.key();
            break;
        }
    }
    
    switch (config.transportType) {
        case CellularTransportType::LTE:
            config.transportTypeStr = "LTE";
            config.networkMode = "LTE";
            config.preferredNetworkType = "9";
            break;
        case CellularTransportType::5G_NSA:
        case CellularTransportType::5G_SA:
            config.transportTypeStr = "5G";
            config.networkMode = "5G";
            config.preferredNetworkType = "20";
            break;
        case CellularTransportType::WCDMA:
            config.transportTypeStr = "HSDPA";
            config.networkMode = "WCDMA";
            config.preferredNetworkType = "3";
            break;
        case CellularTransportType::GSM:
            config.transportTypeStr = "EDGE";
            config.networkMode = "GSM";
            config.preferredNetworkType = "1";
            break;
    }
    
    config.ipAddress = QString("10.%1.%2.%3")
        .arg(QRandomGenerator::global()->bounded(1, 255))
        .arg(QRandomGenerator::global()->bounded(1, 255))
        .arg(QRandomGenerator::global()->bounded(2, 254));
    config.subnetMask = "255.255.255.0";
    config.gateway = QString("10.%1.%2.1")
        .arg(config.ipAddress.split(".")[1])
        .arg(config.ipAddress.split(".")[2]);
    
    config.dnsServers = country.dnsServers;
    
    config.connectionState = "connected";
    config.signalStrength = QRandomGenerator::global()->bounded(-85, -55);
    config.cellId = QRandomGenerator::global()->bounded(1, 65535);
    config.lac = QRandomGenerator::global()->bounded(1, 65535);
    
    config.mobileDataEnabled = "true";
    config.dataRoamingEnabled = "false";
    
    return config;
}

QMap<QString, QString> NetworkProfileManager::generateTelephonyProperties(const NetworkProfile& profile) {
    QMap<QString, QString> props;
    CellularNetworkConfig cellular = profile.cellular;
    
    props["gsm.sim.operator.alpha"] = cellular.operatorName;
    props["gsm.operator.alpha"] = cellular.operatorName;
    props["gsm.sim.operator.numeric"] = cellular.operatorNumeric;
    props["gsm.operator.numeric"] = cellular.operatorNumeric;
    props["gsm.sim.state"] = QString::number(cellular.simState);
    props["ril.sim.state"] = "READY";
    props["gsm.sim.present"] = "true";
    props["ro.setupwizard.mode"] = "OPTIONAL";
    props["telephony.lteOnCdmaDevice"] = "0";
    props["telephony.lteOnGsmDevice"] = "1";
    props["persist.radio.network.mode"] = cellular.preferredNetworkType;
    props["ro.telephony.default_network"] = cellular.preferredNetworkType;
    props["persist.data.roaming"] = cellular.dataRoamingEnabled;
    props["ro.com.google.clientidbase"] = "android-google";
    props["persist.radio.mobile.data"] = cellular.mobileDataEnabled;
    props["net.change"] = "net.dns1";
    props["gsm.cell.id"] = QString::number(cellular.cellId);
    props["gsm.cell.location"] = QString::number(cellular.lac);
    props["gsm.signal.strength"] = QString::number(cellular.signalStrength);
    props["ro.product.locale.region"] = profile.countryCode;
    props["persist.sys.country"] = profile.countryCode;
    
    if (!profile.geolocation.timezone.isEmpty()) {
        props["persist.sys.timezone"] = profile.geolocation.timezone;
    }
    
    props["mock.location.enabled"] = "false";
    props["persist.sys.gps.lat"] = QString::number(profile.geolocation.latitude, 'f', 8);
    props["persist.sys.gps.lon"] = QString::number(profile.geolocation.longitude, 'f', 8);
    
    return props;
}

QStringList NetworkProfileManager::generateTransportHookCommands(const QString& instanceId,
                                                                  const NetworkProfile& profile) {
    QStringList commands;
    
    commands.append("# Disable ethernet interface appearance");
    commands.append("ip link set eth0 down 2>/dev/null || true");
    commands.append("ip link set eth0 name cellular0 2>/dev/null || true");
    commands.append("ip link set cellular0 up 2>/dev/null || true");
    
    commands.append("# Setup cellular interface");
    commands.append("ip link set wlan0 down 2>/dev/null || true");
    commands.append("ip link set wlan0 name rmnet0 2>/dev/null || true");
    commands.append("ip link set rmnet0 up 2>/dev/null || true");
    
    commands.append("# Configure IP");
    commands.append("ip addr add " + profile.cellular.ipAddress + "/24 dev rmnet0 2>/dev/null || true");
    
    commands.append("# Set routing for cellular");
    commands.append("ip route del default 2>/dev/null || true");
    commands.append("ip route add default via " + profile.cellular.gateway + " dev rmnet0 2>/dev/null || true");
    
    return commands;
}

// ========================================================================
// NETWORK SPOOFING
// ========================================================================

TCPFingerprint NetworkProfileManager::generateTCPFingerprint(const QString& countryCode) {
    return TCPFingerprint::getForCountry(countryCode);
}

TCPFingerprint TCPFingerprint::getForCountry(const QString& countryCode) {
    TCPFingerprint fp;
    
    if (countryCode == "US" || countryCode == "CA") {
        fp.windowSize = 65535;
        fp.mss = 1460;
        fp.windowScaling = true;
        fp.sackOk = 1;
        fp.timestamp = 1;
        fp.selectiveAck = 1;
        fp.mode = "normal";
    } else if (countryCode == "GB" || countryCode == "DE" || countryCode == "FR") {
        fp.windowSize = 29200;
        fp.mss = 1460;
        fp.windowScaling = true;
        fp.sackOk = 1;
        fp.timestamp = 1;
        fp.selectiveAck = 1;
        fp.mode = "normal";
    } else if (countryCode == "JP" || countryCode == "KR" || countryCode == "SG") {
        fp.windowSize = 29200;
        fp.mss = 1448;
        fp.windowScaling = true;
        fp.sackOk = 1;
        fp.timestamp = 1;
        fp.selectiveAck = 1;
        fp.mode = "conservative";
    } else if (countryCode == "CN") {
        fp.windowSize = 65535;
        fp.mss = 1440;
        fp.windowScaling = false;
        fp.sackOk = 0;
        fp.timestamp = 0;
        fp.selectiveAck = 0;
        fp.mode = "conservative";
    } else {
        fp = getDefault();
    }
    
    return fp;
}

TCPFingerprint TCPFingerprint::getDefault() {
    TCPFingerprint fp;
    fp.windowSize = 29200;
    fp.mss = 1460;
    fp.windowScaling = true;
    fp.sackOk = 1;
    fp.timestamp = 1;
    fp.selectiveAck = 1;
    fp.mode = "normal";
    return fp;
}

DNSCOnfig NetworkProfileManager::generateDNSConfig(const QString& countryCode) {
    return DNSCOnfig::getForCountry(countryCode);
}

DNSCOnfig DNSCOnfig::getForCountry(const QString& countryCode) {
    Q_UNUSED(countryCode)
    DNSCOnfig dns;
    dns.primary = {"8.8.8.8", "1.1.1.1"};
    dns.secondary = {"8.8.4.4", "1.0.0.1"};
    dns.doh = {
        "https://dns.google/dns-query",
        "https://cloudflare-dns.com/dns-query",
        "https://dns.quad9.net/dns-query"
    };
    dns.dot = {
        "dns.google",
        "cloudflare-dns.com",
        "dns.quad9.net"
    };
    return dns;
}

QStringList NetworkProfileManager::generateWebRTCSetupCommands(const QString& instanceId,
                                                               const QString& localIP) {
    QStringList commands;
    
    commands.append("# WebRTC IP Spoofing Setup");
    commands.append("setprop net.rWbcmLe.localip " + localIP);
    commands.append("setprop net.rWbcmLe.enable 0");
    commands.append("iptables -A OUTPUT -p udp --dport 19302 -j DROP 2>/dev/null || true");
    commands.append("iptables -A OUTPUT -p udp --dport 3478 -j DROP 2>/dev/null || true");
    commands.append("setprop persist.rWbcmLe.interface rmnet0");
    
    return commands;
}

CellularNetworkConfig NetworkProfileManager::generateCellularConfig(const NetworkProfile& profile) {
    CellularNetworkConfig config;
    CountryData country = CountryData::getCountry(profile.countryCode);
    QString carrier = country.carriers.isEmpty() ? "Carrier" : country.carriers[QRandomGenerator::global()->bounded(country.carriers.size())];
    config.operatorName = carrier;
    QString mcc = generateMCCFromCountry(profile.countryCode);
    QString mnc = generateMNCForCarrier(carrier, mcc);
    config.operatorNumeric = mcc + mnc;
    config.simState = 5;
    int totalWeight = 0;
    for (auto it = country.networkTypeWeights.constBegin(); it != country.networkTypeWeights.constEnd(); ++it) totalWeight += it.value();
    int randomValue = QRandomGenerator::global()->bounded(totalWeight);
    int cumulativeWeight = 0;
    for (auto it = country.networkTypeWeights.constBegin(); it != country.networkTypeWeights.constEnd(); ++it) {
        cumulativeWeight += it.value();
        if (randomValue < cumulativeWeight) { config.transportType = it.key(); break; }
    }
    switch (config.transportType) {
        case CellularTransportType::LTE: config.transportTypeStr = "LTE"; config.networkMode = "LTE"; config.preferredNetworkType = "9"; break;
        case CellularTransportType::5G_NSA: case CellularTransportType::5G_SA: config.transportTypeStr = "5G"; config.networkMode = "5G"; config.preferredNetworkType = "20"; break;
        case CellularTransportType::WCDMA: config.transportTypeStr = "HSDPA"; config.networkMode = "WCDMA"; config.preferredNetworkType = "3"; break;
        case CellularTransportType::GSM: config.transportTypeStr = "EDGE"; config.networkMode = "GSM"; config.preferredNetworkType = "1"; break;
    }
    config.ipAddress = QString("10.%1.%2.%3").arg(QRandomGenerator::global()->bounded(1, 255)).arg(QRandomGenerator::global()->bounded(1, 255)).arg(QRandomGenerator::global()->bounded(2, 254));
    config.subnetMask = "255.255.255.0";
    config.gateway = QString("10.%1.%2.1").arg(config.ipAddress.split(".")[1]).arg(config.ipAddress.split(".")[2]);
    config.dnsServers = country.dnsServers;
    config.connectionState = "connected";
    config.signalStrength = QRandomGenerator::global()->bounded(-85, -55);
    config.cellId = QRandomGenerator::global()->bounded(1, 65535);
    config.lac = QRandomGenerator::global()->bounded(1, 65535);
    config.mobileDataEnabled = "true";
    config.dataRoamingEnabled = "false";
    return config;
}

QMap<QString, QString> NetworkProfileManager::generateTelephonyProperties(const NetworkProfile& profile) {
    QMap<QString, QString> props;
    CellularNetworkConfig cellular = profile.cellular;
    props["gsm.sim.operator.alpha"] = cellular.operatorName;
    props["gsm.operator.alpha"] = cellular.operatorName;
    props["gsm.sim.operator.numeric"] = cellular.operatorNumeric;
    props["gsm.operator.numeric"] = cellular.operatorNumeric;
    props["gsm.sim.state"] = QString::number(cellular.simState);
    props["ril.sim.state"] = "READY";
    props["gsm.sim.present"] = "true";
    props["ro.setupwizard.mode"] = "OPTIONAL";
    props["telephony.lteOnCdmaDevice"] = "0";
    props["telephony.lteOnGsmDevice"] = "1";
    props["persist.radio.network.mode"] = cellular.preferredNetworkType;
    props["ro.telephony.default_network"] = cellular.preferredNetworkType;
    props["persist.data.roaming"] = cellular.dataRoamingEnabled;
    props["ro.com.google.clientidbase"] = "android-google";
    props["persist.radio.mobile.data"] = cellular.mobileDataEnabled;
    props["net.change"] = "net.dns1";
    props["gsm.cell.id"] = QString::number(cellular.cellId);
    props["gsm.cell.location"] = QString::number(cellular.lac);
    props["gsm.signal.strength"] = QString::number(cellular.signalStrength);
    props["ro.product.locale.region"] = profile.countryCode;
    props["persist.sys.country"] = profile.countryCode;
    if (!profile.geolocation.timezone.isEmpty()) props["persist.sys.timezone"] = profile.geolocation.timezone;
    props["mock.location.enabled"] = "false";
    props["persist.sys.gps.lat"] = QString::number(profile.geolocation.latitude, 'f', 8);
    props["persist.sys.gps.lon"] = QString::number(profile.geolocation.longitude, 'f', 8);
    return props;
}

QStringList NetworkProfileManager::generateTransportHookCommands(const QString& instanceId, const NetworkProfile& profile) {
    QStringList commands;
    commands.append("# Disable ethernet interface");
    commands.append("ip link set eth0 down 2>/dev/null || true");
    commands.append("ip link set eth0 name cellular0 2>/dev/null || true");
    commands.append("ip link set cellular0 up 2>/dev/null || true");
    commands.append("# Setup cellular interface");
    commands.append("ip link set wlan0 down 2>/dev/null || true");
    commands.append("ip link set wlan0 name rmnet0 2>/dev/null || true");
    commands.append("ip link set rmnet0 up 2>/dev/null || true");
    commands.append("ip addr add " + profile.cellular.ipAddress + "/24 dev rmnet0 2>/dev/null || true");
    commands.append("ip route del default 2>/dev/null || true");
    commands.append("ip route add default via " + profile.cellular.gateway + " dev rmnet0 2>/dev/null || true");
    return commands;
}

TCPFingerprint NetworkProfileManager::generateTCPFingerprint(const QString& countryCode) { return TCPFingerprint::getForCountry(countryCode); }

TCPFingerprint TCPFingerprint::getForCountry(const QString& countryCode) {
    TCPFingerprint fp;
    if (countryCode == "US" || countryCode == "CA") { fp.windowSize = 65535; fp.mss = 1460; fp.windowScaling = true; fp.sackOk = 1; fp.timestamp = 1; fp.selectiveAck = 1; fp.mode = "normal"; }
    else if (countryCode == "GB" || countryCode == "DE" || countryCode == "FR") { fp.windowSize = 29200; fp.mss = 1460; fp.windowScaling = true; fp.sackOk = 1; fp.timestamp = 1; fp.selectiveAck = 1; fp.mode = "normal"; }
    else if (countryCode == "JP" || countryCode == "KR" || countryCode == "SG") { fp.windowSize = 29200; fp.mss = 1448; fp.windowScaling = true; fp.sackOk = 1; fp.timestamp = 1; fp.selectiveAck = 1; fp.mode = "conservative"; }
    else if (countryCode == "CN") { fp.windowSize = 65535; fp.mss = 1440; fp.windowScaling = false; fp.sackOk = 0; fp.timestamp = 0; fp.selectiveAck = 0; fp.mode = "conservative"; }
    else { fp = getDefault(); }
    return fp;
}

TCPFingerprint TCPFingerprint::getDefault() { TCPFingerprint fp; fp.windowSize = 29200; fp.mss = 1460; fp.windowScaling = true; fp.sackOk = 1; fp.timestamp = 1; fp.selectiveAck = 1; fp.mode = "normal"; return fp; }

DNSCOnfig NetworkProfileManager::generateDNSConfig(const QString& countryCode) { return DNSCOnfig::getForCountry(countryCode); }

DNSCOnfig DNSCOnfig::getForCountry(const QString& countryCode) {
    Q_UNUSED(countryCode)
    DNSCOnfig dns;
    dns.primary = {"8.8.8.8", "1.1.1.1"};
    dns.secondary = {"8.8.4.4", "1.0.0.1"};
    dns.doh = {"https://dns.google/dns-query", "https://cloudflare-dns.com/dns-query", "https://dns.quad9.net/dns-query"};
    dns.dot = {"dns.google", "cloudflare-dns.com", "dns.quad9.net"};
    return dns;
}

QStringList NetworkProfileManager::generateWebRTCSetupCommands(const QString& instanceId, const QString& localIP) {
    QStringList commands;
    commands.append("setprop net.rWbcmLe.localip " + localIP);
    commands.append("setprop net.rWbcmLe.enable 0");
    commands.append("iptables -A OUTPUT -p udp --dport 19302 -j DROP 2>/dev/null || true");
    commands.append("iptables -A OUTPUT -p udp --dport 3478 -j DROP 2>/dev/null || true");
    commands.append("setprop persist.rWbcmLe.interface rmnet0");
    return commands;
}

// ========================================================================
// DATA STRUCTURE IMPLEMENTATIONS
// ========================================================================

QJsonObject GeolocationData::toJson() const {
    QJsonObject obj;
    obj["status"] = status; obj["country"] = country; obj["countryCode"] = countryCode;
    obj["region"] = region; obj["regionName"] = regionName; obj["city"] = city;
    obj["zip"] = zip; obj["latitude"] = latitude; obj["longitude"] = longitude;
    obj["timezone"] = timezone; obj["isp"] = isp; obj["org"] = org;
    obj["as"] = asNumber; obj["query"] = query; obj["carrierName"] = carrierName;
    obj["carrierMCC"] = carrierMCC; obj["carrierMNC"] = carrierMNC;
    return obj;
}

void GeolocationData::fromJson(const QJsonObject& json) {
    status = json.value("status").toString(); country = json.value("country").toString();
    countryCode = json.value("countryCode").toString(); region = json.value("region").toString();
    regionName = json.value("regionName").toString(); city = json.value("city").toString();
    zip = json.value("zip").toString(); latitude = json.value("latitude").toDouble();
    longitude = json.value("longitude").toDouble(); timezone = json.value("timezone").toString();
    isp = json.value("isp").toString(); org = json.value("org").toString();
    asNumber = json.value("as").toString(); query = json.value("query").toString();
    carrierName = json.value("carrierName").toString();
    carrierMCC = json.value("carrierMCC").toString();
    carrierMNC = json.value("carrierMNC").toString();
}

QList<CountryData> CountryData::getAllCountries() { return NetworkProfileManager::instance().getAvailableCountries(); }

CountryData CountryData::getCountry(const QString& code) {
    for (const CountryData& country : NetworkProfileManager::instance().getAvailableCountries()) {
        if (country.code == code) return country;
    }
    return CountryData();
}

QJsonObject CellularNetworkConfig::toJson() const {
    QJsonObject obj;
    obj["transportType"] = transportTypeStr; obj["operatorName"] = operatorName;
    obj["operatorNumeric"] = operatorNumeric; obj["simState"] = simState;
    obj["mobileDataEnabled"] = mobileDataEnabled; obj["dataRoamingEnabled"] = dataRoamingEnabled;
    obj["networkMode"] = networkMode; obj["preferredNetworkType"] = preferredNetworkType;
    obj["ipAddress"] = ipAddress; obj["subnetMask"] = subnetMask;
    obj["gateway"] = gateway; obj["dnsServers"] = QJsonArray::fromStringList(dnsServers);
    obj["connectionState"] = connectionState; obj["signalStrength"] = signalStrength;
    obj["cellId"] = cellId; obj["lac"] = lac;
    return obj;
}

QJsonObject TCPFingerprint::toJson() const {
    QJsonObject obj;
    obj["windowSize"] = windowSize; obj["mss"] = mss;
    obj["windowScaling"] = windowScaling; obj["sackOk"] = sackOk;
    obj["timestamp"] = timestamp; obj["selectiveAck"] = selectiveAck;
    obj["mode"] = mode;
    return obj;
}

QJsonObject DNSCOnfig::toJson() const {
    QJsonObject obj;
    obj["primary"] = QJsonArray::fromStringList(primary);
    obj["secondary"] = QJsonArray::fromStringList(secondary);
    obj["doh"] = QJsonArray::fromStringList(doh);
    obj["dot"] = QJsonArray::fromStringList(dot);
    return obj;
}

QJsonObject NetworkProfile::toJson() const {
    QJsonObject obj;
    obj["profileId"] = profileId; obj["instanceId"] = instanceId;
    obj["mode"] = static_cast<int>(mode);
    obj["proxyHost"] = proxyHost; obj["proxyPort"] = proxyPort;
    obj["proxyUsername"] = proxyUsername; obj["proxyPassword"] = proxyPassword;
    obj["proxyType"] = proxyType;
    obj["countryCode"] = countryCode; obj["countryName"] = countryName;
    obj["geolocation"] = geolocation.toJson();
    obj["cellular"] = cellular.toJson();
    obj["tcpFingerprint"] = tcpFingerprint.toJson();
    obj["dnsConfig"] = dnsConfig.toJson();
    QJsonObject telephonyObj;
    for (auto it = telephonyProperties.constBegin(); it != telephonyProperties.constEnd(); ++it) telephonyObj[it.key()] = it.value();
    obj["telephonyProperties"] = telephonyObj;
    obj["blockIPv6"] = blockIPv6; obj["enforceDNS"] = enforceDNS;
    obj["spoofWebRTC"] = spoofWebRTC; obj["enableFirewall"] = enableFirewall;
    obj["status"] = static_cast<int>(status); obj["lastError"] = lastError;
    obj["createdAt"] = createdAt; obj["appliedAt"] = appliedAt;
    return obj;
}

bool NetworkProfile::fromJson(const QJsonObject& json) {
    profileId = json.value("profileId").toString(); instanceId = json.value("instanceId").toString();
    mode = static_cast<NetworkConfigMode>(json.value("mode").toInt());
    proxyHost = json.value("proxyHost").toString(); proxyPort = json.value("proxyPort").toInt();
    proxyUsername = json.value("proxyUsername").toString(); proxyPassword = json.value("proxyPassword").toString();
    proxyType = json.value("proxyType").toString();
    countryCode = json.value("countryCode").toString(); countryName = json.value("countryName").toString();
    geolocation.fromJson(json.value("geolocation").toObject());
    QJsonObject cellularJson = json.value("cellular").toObject();
    cellular.operatorName = cellularJson.value("operatorName").toString();
    cellular.operatorNumeric = cellularJson.value("operatorNumeric").toString();
    cellular.simState = cellularJson.value("simState").toInt();
    cellular.mobileDataEnabled = cellularJson.value("mobileDataEnabled").toString();
    cellular.dataRoamingEnabled = cellularJson.value("dataRoamingEnabled").toString();
    cellular.networkMode = cellularJson.value("networkMode").toString();
    cellular.preferredNetworkType = cellularJson.value("preferredNetworkType").toString();
    cellular.ipAddress = cellularJson.value("ipAddress").toString();
    cellular.subnetMask = cellularJson.value("subnetMask").toString();
    cellular.gateway = cellularJson.value("gateway").toString();
    cellular.dnsServers = cellularJson.value("dnsServers").toVariant().toStringList();
    cellular.connectionState = cellularJson.value("connectionState").toString();
    cellular.signalStrength = cellularJson.value("signalStrength").toInt();
    cellular.cellId = cellularJson.value("cellId").toInt();
    cellular.lac = cellularJson.value("lac").toInt();
    QJsonObject tcpJson = json.value("tcpFingerprint").toObject();
    tcpFingerprint.windowSize = tcpJson.value("windowSize").toUInt();
    tcpFingerprint.mss = tcpJson.value("mss").toUInt();
    tcpFingerprint.windowScaling = tcpJson.value("windowScaling").toBool();
    tcpFingerprint.sackOk = tcpJson.value("sackOk").toUInt();
    tcpFingerprint.timestamp = tcpJson.value("timestamp").toUInt();
    tcpFingerprint.selectiveAck = tcpJson.value("selectiveAck").toUInt();
    tcpFingerprint.mode = tcpJson.value("mode").toString();
    QJsonObject dnsJson = json.value("dnsConfig").toObject();
    dnsConfig.primary = dnsJson.value("primary").toVariant().toStringList();
    dnsConfig.secondary = dnsJson.value("secondary").toVariant().toStringList();
    dnsConfig.doh = dnsJson.value("doh").toVariant().toStringList();
    dnsConfig.dot = dnsJson.value("dot").toVariant().toStringList();
    QJsonObject telephonyObj = json.value("telephonyProperties").toObject();
    for (auto it = telephonyObj.constBegin(); it != telephonyObj.constEnd(); ++it) telephonyProperties[it.key()] = it.value().toString();
    blockIPv6 = json.value("blockIPv6").toBool(); enforceDNS = json.value("enforceDNS").toBool();
    spoofWebRTC = json.value("spoofWebRTC").toBool(); enableFirewall = json.value("enableFirewall").toBool();
    status = static_cast<NetworkProfileStatus>(json.value("status").toInt());
    lastError = json.value("lastError").toString();
    createdAt = json.value("createdAt").toLongLong(); appliedAt = json.value("appliedAt").toLongLong();
    return true;
}

bool NetworkProfile::save(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

NetworkProfile NetworkProfile::load(const QString& filePath) {
    NetworkProfile profile;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return profile;
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull() && doc.isObject()) profile.fromJson(doc.object());
    return profile;
}

} // namespace VirtualPhonePro
