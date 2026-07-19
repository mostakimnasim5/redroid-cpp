#include "VirtualPhonePro/DeviceProfile.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QUuid>
#include <QJsonObject>

namespace VirtualPhonePro {

// ============================================================================
// DeviceIdentity Implementation
// ============================================================================

QVariantMap DeviceIdentity::toVariantMap() const {
    QVariantMap map;
    map["imei"] = imei;
    map["imei2"] = imei2;
    map["serialNumber"] = serialNumber;
    map["androidId"] = androidId;
    map["gsfId"] = gsfId;
    map["advertisingId"] = advertisingId;
    return map;
}

void DeviceIdentity::fromVariantMap(const QVariantMap& map) {
    imei = map.value("imei").toString();
    imei2 = map.value("imei2").toString();
    serialNumber = map.value("serialNumber").toString();
    androidId = map.value("androidId").toString();
    gsfId = map.value("gsfId").toString();
    advertisingId = map.value("advertisingId").toString();
}

// ============================================================================
// MACAddresses Implementation
// ============================================================================

QVariantMap MACAddresses::toVariantMap() const {
    QVariantMap map;
    map["wifiMac"] = wifiMac;
    map["bluetoothMac"] = bluetoothMac;
    map["ethernetMac"] = ethernetMac;
    return map;
}

void MACAddresses::fromVariantMap(const QVariantMap& map) {
    wifiMac = map.value("wifiMac").toString();
    bluetoothMac = map.value("bluetoothMac").toString();
    ethernetMac = map.value("ethernetMac").toString();
}

// ============================================================================
// BuildInfo Implementation
// ============================================================================

QVariantMap BuildInfo::toVariantMap() const {
    QVariantMap map;
    map["brand"] = brand;
    map["manufacturer"] = manufacturer;
    map["model"] = model;
    map["device"] = device;
    map["product"] = product;
    map["board"] = board;
    map["hardware"] = hardware;
    map["fingerprint"] = fingerprint;
    map["bootloader"] = bootloader;
    map["buildId"] = buildId;
    map["buildType"] = buildType;
    map["securityPatch"] = securityPatch;
    map["androidVersion"] = androidVersion;
    map["sdkVersion"] = sdkVersion;
    return map;
}

void BuildInfo::fromVariantMap(const QVariantMap& map) {
    brand = map.value("brand").toString();
    manufacturer = map.value("manufacturer").toString();
    model = map.value("model").toString();
    device = map.value("device").toString();
    product = map.value("product").toString();
    board = map.value("board").toString();
    hardware = map.value("hardware").toString();
    fingerprint = map.value("fingerprint").toString();
    bootloader = map.value("bootloader").toString();
    buildId = map.value("buildId").toString();
    buildType = map.value("buildType").toString();
    securityPatch = map.value("securityPatch").toString();
    androidVersion = map.value("androidVersion", 14).toInt();
    sdkVersion = map.value("sdkVersion", 34).toInt();
}

// ============================================================================
// NetworkConfig Implementation
// ============================================================================

QVariantMap NetworkConfig::toVariantMap() const {
    QVariantMap map;
    map["hostname"] = hostname;
    map["wifiMac"] = wifiMac;
    map["bluetoothMac"] = bluetoothMac;
    map["ethernetMac"] = ethernetMac;
    map["ipAddress"] = ipAddress;
    map["dns1"] = dns1;
    map["dns2"] = dns2;
    return map;
}

void NetworkConfig::fromVariantMap(const QVariantMap& map) {
    hostname = map.value("hostname").toString();
    wifiMac = map.value("wifiMac").toString();
    bluetoothMac = map.value("bluetoothMac").toString();
    ethernetMac = map.value("ethernetMac").toString();
    ipAddress = map.value("ipAddress").toString();
    dns1 = map.value("dns1").toString();
    dns2 = map.value("dns2").toString();
}

// ============================================================================
// SIMConfig Implementation
// ============================================================================

QVariantMap SIMConfig::toVariantMap() const {
    QVariantMap map;
    map["iccid"] = iccid;
    map["imsi"] = imsi;
    map["carrier"] = carrier;
    map["country"] = country;
    map["mcc"] = mcc;
    map["mnc"] = mnc;
    return map;
}

void SIMConfig::fromVariantMap(const QVariantMap& map) {
    iccid = map.value("iccid").toString();
    imsi = map.value("imsi").toString();
    carrier = map.value("carrier").toString();
    country = map.value("country").toString();
    mcc = map.value("mcc").toString();
    mnc = map.value("mnc").toString();
}

// ============================================================================
// GPSConfig Implementation
// ============================================================================

QVariantMap GPSConfig::toVariantMap() const {
    QVariantMap map;
    map["latitude"] = latitude;
    map["longitude"] = longitude;
    map["altitude"] = altitude;
    map["accuracy"] = accuracy;
    map["speed"] = speed;
    map["bearing"] = bearing;
    map["provider"] = provider;
    return map;
}

void GPSConfig::fromVariantMap(const QVariantMap& map) {
    latitude = map.value("latitude", 37.7749).toDouble();
    longitude = map.value("longitude", -122.4194).toDouble();
    altitude = map.value("altitude", 10.0).toDouble();
    accuracy = map.value("accuracy", 5.0).toFloat();
    speed = map.value("speed", 0.0).toFloat();
    bearing = map.value("bearing", 0.0).toFloat();
    provider = map.value("provider", "gps").toString();
}

// ============================================================================
// SensorCalibration Implementation
// ============================================================================

QVariantMap SensorCalibration::toVariantMap() const {
    QVariantMap map;
    map["name"] = name;
    map["vendor"] = vendor;
    map["version"] = version;
    map["resolution"] = resolution;
    map["maxRange"] = maxRange;
    return map;
}

void SensorCalibration::fromVariantMap(const QVariantMap& map) {
    name = map.value("name").toString();
    vendor = map.value("vendor").toString();
    version = map.value("version", 1).toInt();
    resolution = map.value("resolution", 0.001f).toFloat();
    maxRange = map.value("maxRange", 78.0f).toFloat();
}

// ============================================================================
// HardwareInfo Implementation
// ============================================================================

QVariantMap HardwareInfo::toVariantMap() const {
    QVariantMap map;
    map["cpuArchitecture"] = cpuArchitecture;
    map["processor"] = processor;
    map["hardware"] = hardware;
    map["modelName"] = modelName;
    map["coreCount"] = coreCount;
    map["gpuRenderer"] = gpuRenderer;
    map["gpuVendor"] = gpuVendor;
    map["openGlVersion"] = openGlVersion;
    map["vulkanVersion"] = vulkanVersion;
    map["totalRam"] = QString::number(totalRam);
    map["heapSize"] = QString::number(heapSize);
    map["largeHeapSize"] = QString::number(largeHeapSize);
    return map;
}

void HardwareInfo::fromVariantMap(const QVariantMap& map) {
    cpuArchitecture = map.value("cpuArchitecture").toString();
    processor = map.value("processor").toString();
    hardware = map.value("hardware").toString();
    modelName = map.value("modelName").toString();
    coreCount = map.value("coreCount", 8).toInt();
    gpuRenderer = map.value("gpuRenderer").toString();
    gpuVendor = map.value("gpuVendor").toString();
    openGlVersion = map.value("openGlVersion").toString();
    vulkanVersion = map.value("vulkanVersion").toString();
    totalRam = map.value("totalRam", "12288000000").toULongLong();
    heapSize = map.value("heapSize", "512000000").toULongLong();
    largeHeapSize = map.value("largeHeapSize", "4096000000").toULongLong();
}

// ============================================================================
// SecurityConfig Implementation
// ============================================================================

QVariantMap SecurityConfig::toVariantMap() const {
    QVariantMap map;
    map["selinuxMode"] = selinuxMode;
    map["verifiedBootState"] = verifiedBootState;
    map["flashLocked"] = flashLocked;
    map["keymasterVersion"] = keymasterVersion;
    map["strongboxAvailable"] = strongboxAvailable;
    map["hasHardwareAttestation"] = hasHardwareAttestation;
    return map;
}

void SecurityConfig::fromVariantMap(const QVariantMap& map) {
    selinuxMode = map.value("selinuxMode", "enforcing").toString();
    verifiedBootState = map.value("verifiedBootState", "green").toString();
    flashLocked = map.value("flashLocked", true).toBool();
    keymasterVersion = map.value("keymasterVersion", 4).toInt();
    strongboxAvailable = map.value("strongboxAvailable", true).toBool();
    hasHardwareAttestation = map.value("hasHardwareAttestation", true).toBool();
}

// ============================================================================
// DeviceProfile Implementation
// ============================================================================

DeviceProfile::DeviceProfile()
    : androidVersion(14)
    , sdkVersion(34)
    , adbPort(5555)
    , vncPort(5900)
    , instanceIndex(0)
{
    id = QUuid::createUuid().toString();
    version = "1.0.0";
    createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    modifiedAt = createdAt;
    createdBy = "system";
}

QJsonObject DeviceProfile::toJson() const {
    QJsonObject root;
    
    // Metadata
    root["id"] = id;
    root["name"] = name;
    root["manufacturer"] = manufacturer;
    root["version"] = version;
    root["tags"] = QJsonArray::fromStringList(tags);
    root["createdAt"] = createdAt;
    root["modifiedAt"] = modifiedAt;
    root["createdBy"] = createdBy;
    
    // Identity
    root["identity"] = QJsonObject::fromVariantMap(identity.toVariantMap());
    
    // MAC
    root["mac"] = QJsonObject::fromVariantMap(mac.toVariantMap());
    
    // Build
    root["build"] = QJsonObject::fromVariantMap(build.toVariantMap());
    
    // Network
    root["network"] = QJsonObject::fromVariantMap(network.toVariantMap());
    
    // SIM
    root["sim"] = QJsonObject::fromVariantMap(sim.toVariantMap());
    
    // GPS
    root["gps"] = QJsonObject::fromVariantMap(gps.toVariantMap());
    
    // Hardware
    root["hardware"] = QJsonObject::fromVariantMap(hardware.toVariantMap());
    
    // Security
    root["security"] = QJsonObject::fromVariantMap(security.toVariantMap());
    
    // Runtime
    root["adbPort"] = adbPort;
    root["vncPort"] = vncPort;
    root["instanceIndex"] = instanceIndex;
    
    return root;
}

void DeviceProfile::fromJson(const QJsonObject& json) {
    // Metadata
    id = json.value("id").toString();
    name = json.value("name").toString();
    manufacturer = json.value("manufacturer").toString();
    version = json.value("version").toString();
    
    QStringList tagList;
    for (const QJsonValue& v : json.value("tags").toArray()) {
        tagList.append(v.toString());
    }
    tags = tagList;
    
    createdAt = json.value("createdAt").toString();
    modifiedAt = json.value("modifiedAt").toString();
    createdBy = json.value("createdBy").toString();
    
    // Identity
    identity.fromVariantMap(json.value("identity").toObject().toVariantMap());
    
    // MAC
    mac.fromVariantMap(json.value("mac").toObject().toVariantMap());
    
    // Build
    build.fromVariantMap(json.value("build").toObject().toVariantMap());
    
    // Network
    network.fromVariantMap(json.value("network").toObject().toVariantMap());
    
    // SIM
    sim.fromVariantMap(json.value("sim").toObject().toVariantMap());
    
    // GPS
    gps.fromVariantMap(json.value("gps").toObject().toVariantMap());
    
    // Hardware
    hardware.fromVariantMap(json.value("hardware").toObject().toVariantMap());
    
    // Security
    security.fromVariantMap(json.value("security").toObject().toVariantMap());
    
    // Runtime
    adbPort = json.value("adbPort", 5555).toInt();
    vncPort = json.value("vncPort", 5900).toInt();
    instanceIndex = json.value("instanceIndex", 0).toInt();
}

bool DeviceProfile::save(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

DeviceProfile DeviceProfile::load(const QString& filePath) {
    DeviceProfile profile;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return profile;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return profile;
    }
    
    profile.fromJson(doc.object());
    return profile;
}

bool DeviceProfile::isValid() const {
    return !name.isEmpty() && !manufacturer.isEmpty() && 
           !build.model.isEmpty() && DeviceProfile::validateIMEI(identity.imei);
}

QStringList DeviceProfile::validationErrors() const {
    QStringList errors;
    
    if (name.isEmpty()) errors.append("Profile name is required");
    if (manufacturer.isEmpty()) errors.append("Manufacturer is required");
    if (build.model.isEmpty()) errors.append("Model is required");
    if (!DeviceProfile::validateIMEI(identity.imei)) {
        errors.append("Invalid IMEI (must be 15 digits with valid Luhn check)");
    }
    
    return errors;
}

// ============================================================================
// IMEI Generation & Validation (Luhn Algorithm)
// ============================================================================

int calculateLuhnCheckDigit(const QString& base) {
    int sum = 0;
    bool alt = true;
    
    for (int i = base.length() - 1; i >= 0; --i) {
        int n = base.mid(i, 1).toInt();
        
        if (alt) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        
        sum += n;
        alt = !alt;
    }
    
    return (10 - (sum % 10)) % 10;
}

bool DeviceProfile::validateIMEI(const QString& imei) {
    if (imei.length() != 15) return false;
    
    for (int i = 0; i < 15; ++i) {
        if (!imei[i].isDigit()) return false;
    }
    
    int checkDigit = calculateLuhnCheckDigit(imei.left(14));
    return imei.right(1).toInt() == checkDigit;
}

QString DeviceProfile::generateIMEI(const QString& tac) {
    Q_ASSERT(tac.length() == 8);
    
    QString randomPart;
    for (int i = 0; i < 6; ++i) {
        randomPart.append(QString::number(QRandomGenerator::global()->bounded(0, 10)));
    }
    
    QString base = tac + randomPart;
    int checkDigit = calculateLuhnCheckDigit(base);
    
    return base + QString::number(checkDigit);
}

QString DeviceProfile::generateSerial(const QString& manufacturer) {
    QString prefix;
    
    if (manufacturer.toLower().contains("samsung")) {
        prefix = "R5CW";
    } else if (manufacturer.toLower().contains("google")) {
        prefix = "GCIJ";
    } else if (manufacturer.toLower().contains("xiaomi")) {
        prefix = "XMI";
    } else {
        prefix = "AND";
    }
    
    QString serial = prefix;
    for (int i = 0; i < 8; ++i) {
        serial.append(QString::number(QRandomGenerator::global()->bounded(0, 16), 16).toUpper());
    }
    
    return serial;
}

QString DeviceProfile::generateAndroidId() {
    QString id;
    for (int i = 0; i < 16; ++i) {
        id.append(QString::number(QRandomGenerator::global()->bounded(0, 16), 16));
    }
    return id;
}

QString DeviceProfile::generateMAC(const QString& oui) {
    QString baseOui = oui.isEmpty() ? "8C:71:F8" : oui;
    QString mac = baseOui + ":";
    
    for (int i = 0; i < 3; ++i) {
        QString byte = QString::number(QRandomGenerator::global()->bounded(0, 256), 16);
        if (byte.length() == 1) byte = "0" + byte;
        mac += byte.toUpper();
        if (i < 2) mac += ":";
    }
    
    return mac;
}

// ============================================================================
// Factory Methods
// ============================================================================

DeviceProfile DeviceProfile::createSamsungS24Ultra() {
    DeviceProfile profile;
    
    profile.name = "Samsung Galaxy S24 Ultra";
    profile.manufacturer = "Samsung";
    profile.tags = {"flagship", "5g", "premium"};
    
    // Build info
    profile.build.brand = "samsung";
    profile.build.manufacturer = "samsung";
    profile.build.model = "SM-S928B";
    profile.build.device = "dm3q";
    profile.build.product = "dm3q";
    profile.build.board = "dm3q";
    profile.build.hardware = "qcom";
    profile.build.bootloader = "S928BXXU1AXXX";
    profile.build.buildId = "UP1A.231005.007";
    profile.build.buildType = "userdebug";
    profile.build.securityPatch = "2024-01-01";
    profile.build.androidVersion = 14;
    profile.build.sdkVersion = 34;
    
    // Hardware
    profile.hardware.cpuArchitecture = "arm64-v8a";
    profile.hardware.processor = "ARM Implementer 88";
    profile.hardware.hardware = "qcom";
    profile.hardware.modelName = "Snapdragon 8 Gen 3";
    profile.hardware.coreCount = 8;
    profile.hardware.gpuRenderer = "Adreno (TM) 750";
    profile.hardware.gpuVendor = "Qualcomm";
    profile.hardware.openGlVersion = "3.2";
    profile.hardware.vulkanVersion = "1.1.269";
    profile.hardware.totalRam = 12288000000ULL;
    profile.hardware.heapSize = 512000000ULL;
    profile.hardware.largeHeapSize = 4096000000ULL;
    
    // Security
    profile.security.selinuxMode = "enforcing";
    profile.security.verifiedBootState = "green";
    profile.security.flashLocked = true;
    profile.security.keymasterVersion = 4;
    profile.security.strongboxAvailable = true;
    profile.security.hasHardwareAttestation = true;
    
    // Generate identity
    profile.identity.imei = generateIMEI("35875109");
    profile.identity.imei2 = generateIMEI("35875108");
    profile.identity.serialNumber = generateSerial("Samsung");
    profile.identity.androidId = generateAndroidId();
    profile.identity.gsfId = QString::number(QRandomGenerator::global()->bounded(1000000000, 9999999999));
    profile.identity.advertisingId = QUuid::createUuid().toString();
    
    // MAC addresses
    profile.mac.wifiMac = generateMAC("8C:71:F8");
    profile.mac.bluetoothMac = generateMAC("8C:71:F8");
    profile.mac.ethernetMac = generateMAC("00:1A:11");
    
    // Network
    profile.network.hostname = "android-dm3q";
    profile.network.dns1 = "8.8.8.8";
    profile.network.dns2 = "8.8.4.4";
    
    // SIM
    profile.sim.carrier = "T-Mobile";
    profile.sim.country = "US";
    profile.sim.mcc = "310";
    profile.sim.mnc = "260";
    profile.sim.iccid = "8961" + QString::number(QRandomGenerator::global()->bounded(100000000000000ULL, 999999999999999ULL));
    profile.sim.imsi = "310260" + QString::number(QRandomGenerator::global()->bounded(100000000, 999999999));
    
    // GPS
    profile.gps.latitude = 37.7749;
    profile.gps.longitude = -122.4194;
    profile.gps.altitude = 10.0;
    profile.gps.accuracy = 5.0;
    profile.gps.provider = "gps";
    
    return profile;
}

DeviceProfile DeviceProfile::createGooglePixel8Pro() {
    DeviceProfile profile;
    
    profile.name = "Google Pixel 8 Pro";
    profile.manufacturer = "Google";
    profile.tags = {"flagship", "5g", "stock-android"};
    
    // Build info
    profile.build.brand = "google";
    profile.build.manufacturer = "Google";
    profile.build.model = "GA04777";
    profile.build.device = "husky";
    profile.build.product = "husky";
    profile.build.board = "husky";
    profile.build.hardware = "gschip";
    profile.build.bootloader = "mainline";
    profile.build.buildId = "UP1A.231005.007";
    profile.build.buildType = "userdebug";
    profile.build.securityPatch = "2024-01-01";
    profile.build.androidVersion = 14;
    profile.build.sdkVersion = 34;
    
    // Hardware
    profile.hardware.cpuArchitecture = "arm64-v8a";
    profile.hardware.processor = "ARM Implementer 65";
    profile.hardware.hardware = "gschip";
    profile.hardware.modelName = "Tensor G3";
    profile.hardware.coreCount = 8;
    profile.hardware.gpuRenderer = "Mali-G715s MC10";
    profile.hardware.gpuVendor = "ARM";
    profile.hardware.openGlVersion = "3.2";
    profile.hardware.vulkanVersion = "1.1.269";
    profile.hardware.totalRam = 12288000000ULL;
    profile.hardware.heapSize = 512000000ULL;
    profile.hardware.largeHeapSize = 4096000000ULL;
    
    // Security
    profile.security.selinuxMode = "enforcing";
    profile.security.verifiedBootState = "green";
    profile.security.flashLocked = true;
    profile.security.keymasterVersion = 4;
    profile.security.strongboxAvailable = true;
    profile.security.hasHardwareAttestation = true;
    
    // Generate identity
    profile.identity.imei = generateIMEI("35746608");
    profile.identity.serialNumber = generateSerial("Google");
    profile.identity.androidId = generateAndroidId();
    profile.identity.gsfId = QString::number(QRandomGenerator::global()->bounded(1000000000, 9999999999));
    profile.identity.advertisingId = QUuid::createUuid().toString();
    
    // MAC addresses
    profile.mac.wifiMac = generateMAC("94:EB:2C");
    profile.mac.bluetoothMac = generateMAC("94:EB:2C");
    profile.mac.ethernetMac = generateMAC("00:1A:11");
    
    // Network
    profile.network.hostname = "android-husky";
    profile.network.dns1 = "8.8.8.8";
    profile.network.dns2 = "8.8.4.4";
    
    // SIM
    profile.sim.carrier = "Google Fi";
    profile.sim.country = "US";
    profile.sim.mcc = "310";
    profile.sim.mnc = "260";
    profile.sim.iccid = "8901260" + QString::number(QRandomGenerator::global()->bounded(10000000000000ULL, 99999999999999ULL));
    profile.sim.imsi = "310260" + QString::number(QRandomGenerator::global()->bounded(100000000, 999999999));
    
    // GPS
    profile.gps.latitude = 37.4220;
    profile.gps.longitude = -122.0841;
    profile.gps.altitude = 10.0;
    profile.gps.accuracy = 5.0;
    profile.gps.provider = "gps";
    
    return profile;
}

DeviceProfile DeviceProfile::createRandom() {
    // Randomly pick a manufacturer and model
    QStringList manufacturers = {"Samsung", "Google", "Xiaomi", "OnePlus", "OPPO", "Vivo"};
    QStringList models = {"Ultra", "Pro", "Plus", "Max", "Standard"};
    
    QString manufacturer = manufacturers[QRandomGenerator::global()->bounded(manufacturers.size())];
    QString modelSuffix = models[QRandomGenerator::global()->bounded(models.size())];
    
    DeviceProfile profile;
    profile.name = QString("%1 Device %2").arg(manufacturer).arg(modelSuffix);
    profile.manufacturer = manufacturer;
    
    // Generate random TAC (first 8 digits of IMEI)
    QString tac = QString::number(QRandomGenerator::global()->bounded(10000000, 99999999));
    
    // Generate identity
    profile.identity.imei = generateIMEI(tac);
    profile.identity.serialNumber = generateSerial(manufacturer);
    profile.identity.androidId = generateAndroidId();
    profile.identity.gsfId = QString::number(QRandomGenerator::global()->bounded(1000000000, 9999999999));
    profile.identity.advertisingId = QUuid::createUuid().toString();
    
    // Build info
    profile.build.brand = manufacturer.toLower();
    profile.build.manufacturer = manufacturer;
    profile.build.model = manufacturer.toUpper() + modelSuffix.toUpper();
    profile.build.device = manufacturer.toLower() + modelSuffix.toLower();
    profile.build.hardware = "qcom";
    profile.build.bootloader = "bootloader";
    profile.build.buildId = "UP1A.231005.007";
    profile.build.buildType = "userdebug";
    profile.build.securityPatch = "2024-01-01";
    profile.build.androidVersion = 14;
    profile.build.sdkVersion = 34;
    
    // MAC addresses
    profile.mac.wifiMac = generateMAC();
    profile.mac.bluetoothMac = generateMAC();
    profile.mac.ethernetMac = generateMAC();
    
    return profile;
}

} // namespace VirtualPhonePro
