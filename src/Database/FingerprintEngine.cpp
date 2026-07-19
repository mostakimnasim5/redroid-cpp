/**
 * @file FingerprintEngine.cpp
 * @brief Seed-Based Deterministic Fingerprint Generation Implementation
 */

#include "Database/FingerprintEngine.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QUuid>
#include <QDataStream>
#include <QJsonObject>

namespace VirtualPhonePro {

FingerprintEngine* FingerprintEngine::s_instance = nullptr;

FingerprintEngine& FingerprintEngine::instance() {
    if (!s_instance) {
        s_instance = new FingerprintEngine();
    }
    return *s_instance;
}

FingerprintEngine::FingerprintEngine() {
}

// ============================================================================
// Main Generation
// ============================================================================

DeviceFingerprint FingerprintEngine::generateFingerprint(const QString& seed, const FingerprintConfig& config) {
    QMutexLocker locker(&m_mutex);
    
    DeviceFingerprint fp;
    fp.uuid = seed;
    fp.profileId = deriveFromSeed(seed, "profile_id", 32);
    fp.generatedAt = QDateTime::currentMSecsSinceEpoch();
    fp.algorithmVersion = ALGORITHM_VERSION;
    
    // TAC (Type Allocation Code) - first 8 digits of IMEI
    // Based on manufacturer and model
    fp.tac = deriveFromSeed(seed + config.manufacturer + config.model, "tac", 8);
    
    // IMEI Generation with Luhn check digit
    fp.imei1 = generateIMEI(seed, fp.tac);
    fp.imei2 = config.isDualSim ? generateIMEI2(seed) : "";
    
    // Android Identity
    fp.androidId = generateAndroidId(seed);
    fp.gsfId = generateGsFId(seed);
    fp.serialNumber = generateSerial(seed, config.manufacturer);
    fp.bootloaderSerial = deriveFromSeed(seed, "bootloader_serial", 16);
    
    // MAC Addresses - use valid OUI prefixes
    fp.wifiMac = generateMAC(seed, "8C71F8");  // Samsung common OUI
    fp.bluetoothMac = generateMAC(deriveFromSeed(seed, "bt", 0), "001A7D");
    fp.ethernetMac = generateMAC(deriveFromSeed(seed, "eth", 0), "B8:27:EB");  // Raspberry Pi OUI
    
    // SIM Cards
    QString mcc = getMCCForCountry(config.country);
    QString mnc = deriveFromSeed(seed, "mnc", 2);
    fp.iccid1 = generateICCID(seed, mcc);
    fp.iccid2 = config.isDualSim ? generateICCID(deriveFromSeed(seed, "sim2", 0), mcc) : "";
    fp.imsi1 = generateIMSI(seed, mcc, mnc);
    fp.imsi2 = config.isDualSim ? generateIMSI(deriveFromSeed(seed, "sim2", 0), mcc, mnc) : "";
    
    // Hardware Hashes
    fp.hardwareId = hashToHex(QCryptographicHash::hash(
        (seed + config.manufacturer + config.model).toUtf8(), 
        QCryptographicHash::Sha256)).left(32);
    fp.deviceKey = hashToHex(QCryptographicHash::hash(
        (seed + "device_key").toUtf8(), 
        QCryptographicHash::Sha256)).left(32);
    fp.vendorId = deriveFromSeed(seed, "vendor_id", 16);
    
    // Build Components
    fp.buildId = generateBuildId(seed);
    fp.bootloader = deriveFromSeed(seed, "bootloader", 12).toUpper();
    fp.radioVersion = generateRadioVersion(seed);
    
    // Build complete fingerprint string
    fp.fingerprint = generateFingerprintString(fp, config);
    
    // Secure Random Data
    fp.secureId = hashToHex(QCryptographicHash::hash(
        (seed + "secure_id").toUtf8(), 
        QCryptographicHash::Sha256)).left(32);
    fp.authToken = hashToHex(QCryptographicHash::hash(
        (seed + "auth_token").toUtf8(), 
        QCryptographicHash::Sha256)).left(40);
    fp.googleServicesKey = hashToHex(QCryptographicHash::hash(
        (seed + "gms_key").toUtf8(), 
        QCryptographicHash::Sha256)).left(32);
    
    // Model Code
    fp.modelCode = config.model;
    
    // Calculate checksum
    fp.checksum = calculateChecksum(fp);
    
    // Add to collision index
    m_imeiIndex[fp.imei1] = fp.checksum;
    if (!fp.imei2.isEmpty()) m_imeiIndex[fp.imei2] = fp.checksum;
    m_androidIdIndex[fp.androidId] = fp.checksum;
    m_serialIndex[fp.serialNumber] = fp.checksum;
    m_wifiMacIndex[fp.wifiMac] = fp.checksum;
    
    qDebug() << "Generated fingerprint for profile:" << fp.profileId;
    qDebug() << "  IMEI1:" << fp.imei1;
    qDebug() << "  Android ID:" << fp.androidId;
    qDebug() << "  Serial:" << fp.serialNumber;
    
    return fp;
}

DeviceFingerprint FingerprintEngine::generateFromSeed(const QString& seedString) {
    FingerprintConfig defaultConfig;
    defaultConfig.manufacturer = "samsung";
    defaultConfig.model = "SM-S928B";
    defaultConfig.brand = "samsung";
    defaultConfig.device = "dm3q";
    defaultConfig.product = "dm3q";
    defaultConfig.androidVersion = 14;
    defaultConfig.sdkVersion = 34;
    defaultConfig.securityPatch = "2024-01-01";
    defaultConfig.buildType = "user";
    defaultConfig.country = "US";
    defaultConfig.language = "en";
    defaultConfig.carrier = "T-Mobile";
    defaultConfig.isDualSim = false;
    defaultConfig.hasGoogleServices = true;
    
    return generateFingerprint(seedString, defaultConfig);
}

DeviceFingerprint FingerprintEngine::regenerateFingerprint(const QString& seed, const FingerprintConfig& config) {
    return generateFingerprint(seed, config);
}

// ============================================================================
// Internal Generation Methods
// ============================================================================

QString FingerprintEngine::deriveFromSeed(const QString& seed, const QString& purpose, int length) {
    QByteArray data = (seed + purpose).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    
    QString result;
    for (int i = 0; i < hash.size(); ++i) {
        result.append(QString::number(static_cast<unsigned char>(hash[i]), 16).rightJustified(2, '0'));
        if (length > 0 && result.length() >= length) {
            result = result.left(length);
            break;
        }
    }
    
    return result.toUpper();
}

QByteArray FingerprintEngine::hmacSha256(const QString& key, const QString& data) const {
    QMessageAuthenticationCode code(QCryptographicHash::Sha256);
    code.setKey(key.toUtf8());
    code.addData(data.toUtf8());
    return code.result();
}

QString FingerprintEngine::generateIMEI(const QString& seed, const QString& tac) {
    // IMEI format: TAC (8) + SNR (6) + Check Digit (1) = 15 digits
    QString base = tac + deriveFromSeed(seed, "imei1", 6);
    
    // Ensure exactly 14 digits
    while (base.length() < 14) {
        base += "0";
    }
    base = base.left(14);
    
    return base + calculateLuhnCheckDigit(base);
}

QString FingerprintEngine::generateIMEI2(const QString& seed) {
    QString base = deriveFromSeed(seed + "sim2", "imei2", 6);
    // Add different TAC prefix for second IMEI
    QString tac2 = deriveFromSeed(seed, "tac2", 8);
    base = tac2 + base;
    
    while (base.length() < 14) {
        base += "0";
    }
    base = base.left(14);
    
    return base + calculateLuhnCheckDigit(base);
}

QString FingerprintEngine::generateIMSI(const QString& seed, const QString& mcc, const QString& mnc) {
    // IMSI format: MCC (3) + MNC (2-3) + MSIN (9-10) = 15 digits
    QString base = mcc + mnc;
    QString msin = deriveFromSeed(seed, "imsi", 9);
    
    while ((base + msin).length() < 15) {
        msin = "0" + msin;
    }
    
    return (base + msin).left(15);
}

QString FingerprintEngine::generateICCID(const QString& seed, const QString& mcc) {
    // ICCID format: 89 (industry) + Country + Operator + Serial (18) = 20 digits
    QString iccid = "89" + mcc;
    QString serial = deriveFromSeed(seed, "iccid", 16);
    
    // Ensure 20 digits total
    while ((iccid + serial).length() < 20) {
        serial = "0" + serial;
    }
    
    return (iccid + serial).left(20);
}

QString FingerprintEngine::generateMAC(const QString& seed, const QString& prefix) {
    QByteArray hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha256);
    
    QString mac = prefix;
    for (int i = 0; i < 3; ++i) {
        mac += ":" + QString::number(static_cast<unsigned char>(hash[i]), 16).rightJustified(2, '0').toUpper();
    }
    
    return mac;
}

QString FingerprintEngine::generateSerial(const QString& seed, const QString& manufacturer) {
    // Manufacturer-specific serial format
    if (manufacturer.toLower().contains("samsung")) {
        // Samsung format: R + 3 digits + 2 letters + 2 digits + letter
        QString s = "R" + deriveFromSeed(seed, "serial", 7);
        return s.toUpper().left(10);
    } else if (manufacturer.toLower().contains("google") || 
               manufacturer.toLower().contains("pixel")) {
        // Google/Pixel format: 4 alphanumeric
        return deriveFromSeed(seed, "serial", 16).toUpper().left(16);
    } else {
        // Generic format
        return deriveFromSeed(seed, "serial", 12).toUpper().left(12);
    }
}

QString FingerprintEngine::generateAndroidId(const QString& seed) {
    // Android ID is 16 characters hex (lowercase)
    return deriveFromSeed(seed, "android_id", 16).toLower();
}

QString FingerprintEngine::generateGsFId(const QString& seed) {
    // GSF ID is typically 10 digits
    return deriveFromSeed(seed, "gsf_id", 10);
}

QString FingerprintEngine::generateFingerprintString(const DeviceFingerprint& fp, const FingerprintConfig& config) {
    // Format: brand/device:version/build:user/release-keys
    return QString("%1/%2/%3:%4/%5/%6:%7/%8-%9")
        .arg(config.brand.toLower())
        .arg(config.device)
        .arg(config.device)
        .arg(config.androidVersion)
        .arg(fp.buildId)
        .arg(fp.bootloader)
        .arg(config.buildType)
        .arg(fp.serialNumber)
        .arg("release-keys");
}

QString FingerprintEngine::generateBuildId(const QString& seed) {
    // Build ID format: UP1A.231005.007 (date-based)
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = QString("UP1A.%1%2.%3")
        .arg(now.date().year())
        .arg(now.date().month(), 2, 10, QChar('0'))
        .arg(deriveFromSeed(seed, "build_id", 3));
    
    return dateStr;
}

QString FingerprintEngine::generateRadioVersion(const QString& seed) {
    return QString("G%1%2%3.%4")
        .arg(deriveFromSeed(seed, "radio", 2))
        .arg(deriveFromSeed(seed + "2", "radio", 2))
        .arg(deriveFromSeed(seed + "3", "radio", 2))
        .arg(deriveFromSeed(seed + "4", "radio", 1));
}

// ============================================================================
// Luhn Algorithm
// ============================================================================

QString FingerprintEngine::calculateLuhnCheckDigit(const QString& base) const {
    int sum = 0;
    bool alternate = true;
    
    for (int i = base.length() - 1; i >= 0; --i) {
        int digit = base[i].digitValue();
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return QString::number((10 - (sum % 10)) % 10);
}

bool FingerprintEngine::validateIMEI(const QString& imei) const {
    if (imei.length() != 15) return false;
    
    int sum = 0;
    bool alternate = false;
    
    for (int i = imei.length() - 1; i >= 0; --i) {
        int digit = imei[i].digitValue();
        if (digit < 0) return false;
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) digit -= 9;
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

bool FingerprintEngine::validateIMSI(const QString& imsi) const {
    // IMSI should be 15 digits
    if (imsi.length() != 15) return false;
    
    for (const QChar& c : imsi) {
        if (!c.isDigit()) return false;
    }
    
    return true;
}

bool FingerprintEngine::validateICCID(const QString& iccid) const {
    // ICCID should be 19-20 digits
    if (iccid.length() < 19 || iccid.length() > 20) return false;
    
    for (const QChar& c : iccid) {
        if (!c.isDigit()) return false;
    }
    
    return true;
}

// ============================================================================
// Collision Detection
// ============================================================================

bool FingerprintEngine::checkCollision(const DeviceFingerprint& fp) const {
    QMutexLocker locker(&m_mutex);
    
    // Check all primary identifiers
    if (m_imeiIndex.contains(fp.imei1)) {
        qWarning() << "IMEI1 collision detected:" << fp.imei1;
        return true;
    }
    
    if (!fp.imei2.isEmpty() && m_imeiIndex.contains(fp.imei2)) {
        qWarning() << "IMEI2 collision detected:" << fp.imei2;
        return true;
    }
    
    if (m_androidIdIndex.contains(fp.androidId)) {
        qWarning() << "Android ID collision detected:" << fp.androidId;
        return true;
    }
    
    if (m_serialIndex.contains(fp.serialNumber)) {
        qWarning() << "Serial collision detected:" << fp.serialNumber;
        return true;
    }
    
    if (m_wifiMacIndex.contains(fp.wifiMac)) {
        qWarning() << "WiFi MAC collision detected:" << fp.wifiMac;
        return true;
    }
    
    return false;
}

// ============================================================================
// Serialization
// ============================================================================

QJsonObject FingerprintEngine::toJson(const DeviceFingerprint& fp) const {
    QJsonObject json;
    
    json["uuid"] = fp.uuid;
    json["profileId"] = fp.profileId;
    json["imei1"] = fp.imei1;
    json["imei2"] = fp.imei2;
    json["androidId"] = fp.androidId;
    json["gsfId"] = fp.gsfId;
    json["serialNumber"] = fp.serialNumber;
    json["bootloaderSerial"] = fp.bootloaderSerial;
    json["wifiMac"] = fp.wifiMac;
    json["bluetoothMac"] = fp.bluetoothMac;
    json["ethernetMac"] = fp.ethernetMac;
    json["iccid1"] = fp.iccid1;
    json["iccid2"] = fp.iccid2;
    json["imsi1"] = fp.imsi1;
    json["imsi2"] = fp.imsi2;
    json["hardwareId"] = fp.hardwareId;
    json["deviceKey"] = fp.deviceKey;
    json["vendorId"] = fp.vendorId;
    json["fingerprint"] = fp.fingerprint;
    json["buildId"] = fp.buildId;
    json["bootloader"] = fp.bootloader;
    json["radioVersion"] = fp.radioVersion;
    json["secureId"] = fp.secureId;
    json["authToken"] = fp.authToken;
    json["googleServicesKey"] = fp.googleServicesKey;
    json["tac"] = fp.tac;
    json["modelCode"] = fp.modelCode;
    json["generatedAt"] = fp.generatedAt;
    json["algorithmVersion"] = fp.algorithmVersion;
    json["checksum"] = fp.checksum;
    
    return json;
}

DeviceFingerprint FingerprintEngine::fromJson(const QJsonObject& json) const {
    DeviceFingerprint fp;
    
    fp.uuid = json["uuid"].toString();
    fp.profileId = json["profileId"].toString();
    fp.imei1 = json["imei1"].toString();
    fp.imei2 = json["imei2"].toString();
    fp.androidId = json["androidId"].toString();
    fp.gsfId = json["gsfId"].toString();
    fp.serialNumber = json["serialNumber"].toString();
    fp.bootloaderSerial = json["bootloaderSerial"].toString();
    fp.wifiMac = json["wifiMac"].toString();
    fp.bluetoothMac = json["bluetoothMac"].toString();
    fp.ethernetMac = json["ethernetMac"].toString();
    fp.iccid1 = json["iccid1"].toString();
    fp.iccid2 = json["iccid2"].toString();
    fp.imsi1 = json["imsi1"].toString();
    fp.imsi2 = json["imsi2"].toString();
    fp.hardwareId = json["hardwareId"].toString();
    fp.deviceKey = json["deviceKey"].toString();
    fp.vendorId = json["vendorId"].toString();
    fp.fingerprint = json["fingerprint"].toString();
    fp.buildId = json["buildId"].toString();
    fp.bootloader = json["bootloader"].toString();
    fp.radioVersion = json["radioVersion"].toString();
    fp.secureId = json["secureId"].toString();
    fp.authToken = json["authToken"].toString();
    fp.googleServicesKey = json["googleServicesKey"].toString();
    fp.tac = json["tac"].toString();
    fp.modelCode = json["modelCode"].toString();
    fp.generatedAt = json["generatedAt"].toLongLong();
    fp.algorithmVersion = json["algorithmVersion"].toString();
    fp.checksum = json["checksum"].toUInt();
    
    return fp;
}

QByteArray FingerprintEngine::toBinary(const DeviceFingerprint& fp) const {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    
    stream << fp.uuid << fp.profileId << fp.imei1 << fp.imei2
           << fp.androidId << fp.gsfId << fp.serialNumber << fp.bootloaderSerial
           << fp.wifiMac << fp.bluetoothMac << fp.ethernetMac
           << fp.iccid1 << fp.iccid2 << fp.imsi1 << fp.imsi2
           << fp.hardwareId << fp.deviceKey << fp.vendorId
           << fp.fingerprint << fp.buildId << fp.bootloader << fp.radioVersion
           << fp.secureId << fp.authToken << fp.googleServicesKey
           << fp.tac << fp.modelCode
           << fp.generatedAt << fp.algorithmVersion << fp.checksum;
    
    return data;
}

DeviceFingerprint FingerprintEngine::fromBinary(const QByteArray& data) const {
    DeviceFingerprint fp;
    QDataStream stream(data);
    
    stream >> fp.uuid >> fp.profileId >> fp.imei1 >> fp.imei2
           >> fp.androidId >> fp.gsfId >> fp.serialNumber >> fp.bootloaderSerial
           >> fp.wifiMac >> fp.bluetoothMac >> fp.ethernetMac
           >> fp.iccid1 >> fp.iccid2 >> fp.imsi1 >> fp.imsi2
           >> fp.hardwareId >> fp.deviceKey >> fp.vendorId
           >> fp.fingerprint >> fp.buildId >> fp.bootloader >> fp.radioVersion
           >> fp.secureId >> fp.authToken >> fp.googleServicesKey
           >> fp.tac >> fp.modelCode
           >> fp.generatedAt >> fp.algorithmVersion >> fp.checksum;
    
    return fp;
}

// ============================================================================
// Utilities
// ============================================================================

QString FingerprintEngine::generateSeed() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

quint32 FingerprintEngine::calculateChecksum(const DeviceFingerprint& fp) const {
    QString data = fp.uuid + fp.imei1 + fp.androidId + fp.serialNumber + fp.wifiMac;
    QByteArray hash = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5);
    
    quint32 checksum = 0;
    for (int i = 0; i < 4; ++i) {
        checksum = (checksum << 8) | static_cast<unsigned char>(hash[i]);
    }
    
    return checksum;
}

QString FingerprintEngine::hashToHex(const QByteArray& data) const {
    return QString::fromLatin1(data.toHex());
}

QString FingerprintEngine::getMCCForCountry(const QString& country) {
    static QMap<QString, QString> mccMap = {
        {"US", "310"}, {"GB", "234"}, {"DE", "262"}, {"FR", "208"},
        {"JP", "440"}, {"CN", "460"}, {"IN", "404"}, {"KR", "450"},
        {"BD", "470"}, {"PK", "410"}, {"CA", "302"}, {"AU", "505"},
        {"IT", "222"}, {"ES", "214"}, {"BR", "724"}, {"MX", "334"},
        {"RU", "250"}, {"SA", "420"}, {"AE", "424"}, {"SG", "525"}
    };
    
    return mccMap.value(country.toUpper(), "310");
}

} // namespace VirtualPhonePro
