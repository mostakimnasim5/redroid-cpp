/**
 * @file UniqueDeviceGenerator.cpp
 * @brief Unique Device Profile Generator Implementation
 * @version 2.0.0
 */

#include "VirtualPhonePro/UniqueDeviceGenerator.h"

#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

namespace VirtualPhonePro {

UniqueDeviceGenerator* UniqueDeviceGenerator::s_instance = nullptr;

UniqueDeviceGenerator::UniqueDeviceGenerator() {
    initializeOUIs();
    loadExistingRecords();
}

UniqueDeviceGenerator& UniqueDeviceGenerator::instance() {
    if (!s_instance) {
        s_instance = new UniqueDeviceGenerator();
    }
    return *s_instance;
}

void UniqueDeviceGenerator::initializeOUIs() {
    // Official OUI prefixes for different manufacturers
    m_manufacturerOUIs["samsung"] = {
        "8C:71:F8",  // Samsung Electronics
        "D0:22:BE",
        "00:02:78",
        "54:88:0E",
        "90:18:7C"
    };
    
    m_manufacturerOUIs["google"] = {
        "3C:5A:B4",  // Google
        "54:60:09",
        "F4:F5:D8",
        "1C:F2:9A"
    };
    
    m_manufacturerOUIs["xiaomi"] = {
        "34:80:B3",  // Xiaomi
        "F4:F5:D8",
        "64:09:80",
        "58:44:98"
    };
    
    m_manufacturerOUIs["huawei"] = {
        "00:25:9E",  // Huawei
        "34:29:12",
        "EC:D0:9F",
        "00:E0:4C"
    };
    
    m_manufacturerOUIs["oppo"] = {
        "2C:33:61",  // OPPO
        "20:74:CF",
        "CC:D0:D5"
    };
    
    m_manufacturerOUIs["vivo"] = {
        "20:A2:E4",  // Vivo
        "BC:83:85",
        "A8:BB:CF"
    };
    
    m_manufacturerOUIs["oneplus"] = {
        "F6:4D:D7",  // OnePlus
        "58:EF:68",
        "2C:33:61"
    };
    
    m_manufacturerOUIs["default"] = {
        "00:1A:11",  // Generic
        "00:23:69",
        "00:50:56"
    };
}

QString UniqueDeviceGenerator::generateHash(const QString& input) {
    QByteArray data = input.toUtf8();
    data.append(QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8());
    data.append(QString::number(QRandomGenerator::global()->bounded(10000, 99999)).toUtf8());
    
    QString hash = QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    return hash;
}

QString UniqueDeviceGenerator::generateInstanceId() {
    QUuid uuid = QUuid::createUuid();
    QString uuidStr = uuid.toString(QUuid::WithoutBraces);
    
    // Add prefix for readability
    return "device_" + uuidStr.left(8) + "_" + uuidStr.right(8);
}

QString UniqueDeviceGenerator::generateUniqueIMEI() {
    // Generate TAC (Type Allocation Code) - first 8 digits
    // Samsung, Google, Xiaomi TACs
    QStringList samsungTACs = {
        "35875107", "35875108", "35875109", "35746608", "35746609",
        "35673009", "35673010", "35469609", "35469610", "35577509"
    };
    
    QStringList googleTACs = {
        "35746608", "35746609", "35746610", "35746611",
        "35863209", "35863210", "35924909", "35924910"
    };
    
    QStringList xiaomiTACs = {
        "86917102", "86917103", "86917104", "86917105",
        "86831103", "86831104", "86831105", "86831106"
    };
    
    // Select random TAC
    QStringList allTACs = samsungTACs + googleTACs + xiaomiTACs;
    QString tac = allTACs[QRandomGenerator::global()->bounded(allTACs.size())];
    
    // Generate random serial (6 digits)
    QString serial;
    for (int i = 0; i < 6; i++) {
        serial += QString::number(QRandomGenerator::global()->bounded(0, 10));
    }
    
    // Combine TAC + serial
    QString imeiBase = tac + serial;
    
    // Calculate Luhn check digit
    int checkDigit = calculateLuhnCheckDigit(imeiBase);
    QString imei = imeiBase + QString::number(checkDigit);
    
    // Ensure uniqueness
    int attempts = 0;
    while (m_generatedIMEIs.values().contains(imei) && attempts < 100) {
        serial.clear();
        for (int i = 0; i < 6; i++) {
            serial += QString::number(QRandomGenerator::global()->bounded(0, 10));
        }
        imeiBase = tac + serial;
        checkDigit = calculateLuhnCheckDigit(imeiBase);
        imei = imeiBase + QString::number(checkDigit);
        attempts++;
    }
    
    // Store for uniqueness
    QString uniqueKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedIMEIs[uniqueKey] = imei;
    
    return imei;
}

int UniqueDeviceGenerator::calculateLuhnCheckDigit(const QString& base) {
    int sum = 0;
    bool alternate = true;
    
    for (int i = base.length() - 1; i >= 0; i--) {
        int n = base[i].digitValue();
        
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        
        sum += n;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

QString UniqueDeviceGenerator::generateUniqueSerial(const QString& manufacturer) {
    QString serial;
    
    if (manufacturer.toLower() == "samsung") {
        // Samsung format: R + 6 digits + X + 2 digits
        serial = "R" + 
                 QString::number(QRandomGenerator::global()->bounded(100000, 999999)) +
                 "X" +
                 QString::number(QRandomGenerator::global()->bounded(10, 99));
    } 
    else if (manufacturer.toLower() == "google") {
        // Google format: AG + 8 digits
        serial = "AG" + 
                 QString::number(QRandomGenerator::global()->bounded(10000000, 99999999));
    }
    else if (manufacturer.toLower() == "xiaomi") {
        // Xiaomi format: Serial + letters
        serial = QString::number(QRandomGenerator::global()->bounded(10000000, 99999999)) +
                 QString(QChar('A' + QRandomGenerator::global()->bounded(0, 26))) +
                 QString(QChar('A' + QRandomGenerator::global()->bounded(0, 26)));
    }
    else if (manufacturer.toLower() == "huawei") {
        // Huawei format
        serial = QString::number(QRandomGenerator::global()->bounded(100000000000ULL, 999999999999ULL));
    }
    else {
        // Generic format
        serial = QUuid::createUuid().toString(QUuid::WithoutBraces).left(12).toUpper();
    }
    
    // Ensure uniqueness
    int attempts = 0;
    while (m_generatedSerials.values().contains(serial) && attempts < 100) {
        if (manufacturer.toLower() == "samsung") {
            serial = "R" + 
                     QString::number(QRandomGenerator::global()->bounded(100000, 999999)) +
                     "X" +
                     QString::number(QRandomGenerator::global()->bounded(10, 99));
        } else {
            serial = QUuid::createUuid().toString(QUuid::WithoutBraces).left(12).toUpper();
        }
        attempts++;
    }
    
    QString uniqueKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedSerials[uniqueKey] = serial;
    
    return serial;
}

QString UniqueDeviceGenerator::generateUniqueAndroidId() {
    // Android ID is 16 hex characters
    QString id;
    for (int i = 0; i < 16; i++) {
        id += QString::number(QRandomGenerator::global()->bounded(0, 16), 16).toUpper();
    }
    
    // Ensure uniqueness
    int attempts = 0;
    while (m_generatedAndroidIds.values().contains(id) && attempts < 100) {
        id.clear();
        for (int i = 0; i < 16; i++) {
            id += QString::number(QRandomGenerator::global()->bounded(0, 16), 16).toUpper();
        }
        attempts++;
    }
    
    QString uniqueKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedAndroidIds[uniqueKey] = id;
    
    return id;
}

QString UniqueDeviceGenerator::generateUniqueGSFId() {
    // GSF ID is 10 digits
    QString gsfId = QString::number(QRandomGenerator::global()->bounded(1000000000ULL, 9999999999ULL));
    return gsfId;
}

QString UniqueDeviceGenerator::generateUniqueMAC(const QString& oui) {
    QString mac;
    
    if (!oui.isEmpty()) {
        mac = oui;
    } else {
        // Random OUI from default pool
        QStringList defaultOUIs = m_manufacturerOUIs["default"];
        mac = defaultOUIs[QRandomGenerator::global()->bounded(defaultOUIs.size())];
    }
    
    // Generate remaining 3 octets
    for (int i = 0; i < 3; i++) {
        mac += ":" + QString::number(QRandomGenerator::global()->bounded(0, 256), 16)
                                .rightJustified(2, '0').toUpper();
    }
    
    return mac;
}

QString UniqueDeviceGenerator::generateUniqueDeviceKey() {
    QUuid uuid = QUuid::createUuid();
    return uuid.toString(QUuid::WithoutBraces).toUpper();
}

QString UniqueDeviceGenerator::generateUniqueICCID() {
    // ICCID starts with 8961 + operator prefix
    QString iccid = "8961" + 
                    QString::number(QRandomGenerator::global()->bounded(100000000000ULL, 999999999999ULL));
    return iccid;
}

QString UniqueDeviceGenerator::generateUniqueIMSI(const QString& mcc, const QString& mnc) {
    // IMSI = MCC (3 digits) + MNC (2-3 digits) + MSIN (remaining)
    int msinLength = 15 - mcc.length() - mnc.length();
    QString msin;
    
    for (int i = 0; i < msinLength; i++) {
        msin += QString::number(QRandomGenerator::global()->bounded(0, 10));
    }
    
    return mcc + mnc + msin;
}

QString UniqueDeviceGenerator::generateUniqueBluetoothMAC() {
    // Bluetooth MAC (similar to WiFi but different prefix)
    QString mac = "00:1A:7D:";  // Bluetooth SIG assigned OUI
    
    for (int i = 0; i < 3; i++) {
        mac += ":" + QString::number(QRandomGenerator::global()->bounded(0, 256), 16)
                                .rightJustified(2, '0').toUpper();
    }
    
    return mac;
}

QJsonObject UniqueDeviceGenerator::generateCompleteUniqueIdentity(const QString& manufacturer) {
    QJsonObject identity;
    
    // Generate all unique IDs
    identity["instanceId"] = generateInstanceId();
    identity["imei"] = generateUniqueIMEI();
    identity["imei2"] = generateUniqueIMEI();  // Different last digit for dual SIM
    identity["serialNumber"] = generateUniqueSerial(manufacturer);
    identity["androidId"] = generateUniqueAndroidId();
    identity["gsfId"] = generateUniqueGSFId();
    identity["deviceKey"] = generateUniqueDeviceKey();
    identity["wifiMac"] = generateUniqueMAC();
    identity["bluetoothMac"] = generateUniqueBluetoothMAC();
    identity["iccid"] = generateUniqueICCID();
    identity["imsi"] = generateUniqueIMSI("310", "260");  // T-Mobile example
    identity["advertisingId"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Generate unique timestamp
    identity["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    identity["uniqueHash"] = generateHash(identity["instanceId"].toString());
    
    return identity;
}

bool UniqueDeviceGenerator::verifyUniqueness(const QJsonObject& profile) {
    QString imei = profile["imei"].toString();
    QString serial = profile["serialNumber"].toString();
    QString androidId = profile["androidId"].toString();
    
    // Check for duplicates
    bool imeiUnique = !m_generatedIMEIs.values().contains(imei);
    bool serialUnique = !m_generatedSerials.values().contains(serial);
    bool androidIdUnique = !m_generatedAndroidIds.values().contains(androidId);
    
    return imeiUnique && serialUnique && androidIdUnique;
}

QJsonObject UniqueDeviceGenerator::getUniqueIdsForInstance(const QString& instanceId) {
    if (m_instanceToIds.contains(instanceId)) {
        return QJsonObject{{instanceId, m_instanceToIds[instanceId]}};
    }
    return QJsonObject();
}

bool UniqueDeviceGenerator::isIMEIUnique(const QString& imei) {
    return !m_generatedIMEIs.values().contains(imei);
}

bool UniqueDeviceGenerator::isSerialUnique(const QString& serial) {
    return !m_generatedSerials.values().contains(serial);
}

void UniqueDeviceGenerator::registerInstance(const QString& instanceId, const QJsonObject& uniqueIds) {
    m_instanceToIds[instanceId] = uniqueIds["imei"].toString();
    
    QString imeiKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedIMEIs[imeiKey] = uniqueIds["imei"].toString();
    
    QString serialKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedSerials[serialKey] = uniqueIds["serialNumber"].toString();
    
    QString androidKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedAndroidIds[androidKey] = uniqueIds["androidId"].toString();
    
    saveRecords();
}

void UniqueDeviceGenerator::unregisterInstance(const QString& instanceId) {
    m_instanceToIds.remove(instanceId);
    saveRecords();
}

int UniqueDeviceGenerator::getUniqueDeviceCount() {
    return m_instanceToIds.size();
}

void UniqueDeviceGenerator::clearAllRecords() {
    m_generatedIMEIs.clear();
    m_generatedSerials.clear();
    m_generatedAndroidIds.clear();
    m_instanceToIds.clear();
    m_idToInstance.clear();
    saveRecords();
}

void UniqueDeviceGenerator::saveRecords() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString recordsFile = configDir + "/unique_devices.json";
    
    QJsonObject json;
    
    // Save instance mappings
    QJsonObject instances;
    for (auto it = m_instanceToIds.begin(); it != m_instanceToIds.end(); ++it) {
        instances[it.key()] = it.value();
    }
    json["instances"] = instances;
    
    // Save generated IDs
    QJsonArray imeis;
    for (const QString& imei : m_generatedIMEIs.values()) {
        imeis.append(imei);
    }
    json["imeis"] = imeis;
    
    QJsonArray serials;
    for (const QString& serial : m_generatedSerials.values()) {
        serials.append(serial);
    }
    json["serials"] = serials;
    
    QJsonArray androidIds;
    for (const QString& id : m_generatedAndroidIds.values()) {
        androidIds.append(id);
    }
    json["androidIds"] = androidIds;
    
    QFile file(recordsFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
    }
}

void UniqueDeviceGenerator::loadExistingRecords() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString recordsFile = configDir + "/unique_devices.json";
    
    QFile file(recordsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return;
    }
    
    QJsonObject json = doc.object();
    
    // Load instances
    QJsonObject instances = json["instances"].toObject();
    for (auto it = instances.begin(); it != instances.end(); ++it) {
        m_instanceToIds[it.key()] = it.value().toString();
    }
    
    // Load IMEIs
    QJsonArray imeis = json["imeis"].toArray();
    for (int i = 0; i < imeis.size(); ++i) {
        QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_generatedIMEIs[key] = imeis[i].toString();
    }
    
    // Load serials
    QJsonArray serials = json["serials"].toArray();
    for (int i = 0; i < serials.size(); ++i) {
        QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_generatedSerials[key] = serials[i].toString();
    }
    
    // Load Android IDs
    QJsonArray androidIds = json["androidIds"].toArray();
    for (int i = 0; i < androidIds.size(); ++i) {
        QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_generatedAndroidIds[key] = androidIds[i].toString();
    }
}

} // namespace VirtualPhonePro
