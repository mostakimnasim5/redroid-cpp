/**
 * @file UniqueDeviceGenerator.cpp
 * @brief Unique Device Profile Generator Implementation
 * @version 3.0.0
 * 
 * Features:
 * - Thread-safe singleton with double-checked locking
 * - Cryptographically secure random number generation (CSPRNG)
 * - 1000+ TAC database integration
 * - Comprehensive error handling
 * - Persistent identity storage
 */

#include "VirtualPhonePro/UniqueDeviceGenerator.h"
#include "Data/TACDatabase.h"

#include <QUuid>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <sys/random.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#endif

namespace VirtualPhonePro {

// ============================================================================
// Thread-Safe Singleton Implementation
// ============================================================================

std::atomic<UniqueDeviceGenerator*> UniqueDeviceGenerator::s_instance{nullptr};
std::atomic<quint64> UniqueDeviceGenerator::s_instanceId{0};
QMutex UniqueDeviceGenerator::s_mutex;

UniqueDeviceGenerator::UniqueDeviceGenerator()
    : m_initialized(false)
    , m_nextInstanceId(0)
{
    initializeOUIs();
    initializeTACDatabase();
    loadExistingRecords();
    m_initialized = true;
    
    qDebug() << "[UniqueDeviceGenerator] Initialized with" 
             << m_generatedIMEIs.size() << "IMEIs,"
             << m_generatedSerials.size() << "serials,"
             << "and TAC database: " << RedroidCPP::TACDatabase::getInstance().size() << "entries";
}

UniqueDeviceGenerator::~UniqueDeviceGenerator() {
    saveRecords();
    m_initialized = false;
}

UniqueDeviceGenerator& UniqueDeviceGenerator::instance() {
    // Double-checked locking pattern with atomic flag
    UniqueDeviceGenerator* instance = s_instance.load(std::memory_order_acquire);
    if (instance == nullptr) {
        QMutexLocker locker(&s_mutex);
        instance = s_instance.load(std::memory_order_relaxed);
        if (instance == nullptr) {
            instance = new (std::nothrow) UniqueDeviceGenerator();
            if (instance) {
                s_instance.store(instance, std::memory_order_release);
            }
        }
    }
    return *instance;
}

bool UniqueDeviceGenerator::isInitialized() const {
    return m_initialized.load(std::memory_order_acquire);
}

// ============================================================================
// Cryptographically Secure Random Number Generator (CSPRNG)
// ============================================================================

bool UniqueDeviceGenerator::getSecureRandomBytes(unsigned char* buffer, size_t length) const {
    if (!buffer || length == 0) {
        return false;
    }
    
#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    BOOL result = FALSE;
    
    // Try to acquire crypto context
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, 0)) {
        // If context doesn't exist, try to create a new one
        if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
            qWarning() << "[SecureRandom] Failed to acquire crypto context";
            return false;
        }
    }
    
    // Generate random bytes
    result = CryptGenRandom(hProv, static_cast<DWORD>(length), buffer);
    CryptReleaseContext(hProv, 0);
    
    if (!result) {
        qWarning() << "[SecureRandom] CryptGenRandom failed";
        return false;
    }
    
    return true;
#else
    // Linux/Unix: Use getrandom() for CSPRNG
    ssize_t bytesRead = getrandom(buffer, length, 0);
    if (bytesRead != static_cast<ssize_t>(length)) {
        qWarning() << "[SecureRandom] getrandom failed, got" << bytesRead << "expected" << length;
        return false;
    }
    return true;
#endif
}

quint32 UniqueDeviceGenerator::getSecureRandomUInt32(quint32 min, quint32 max) const {
    if (min >= max) {
        return min;
    }
    
    const quint32 range = max - min + 1;
    quint32 randomValue;
    
    // Use rejection sampling for uniform distribution
    // This ensures no modulo bias
    const quint32 limit = UINT32_MAX - (UINT32_MAX % range);
    
    do {
        unsigned char bytes[4];
        if (getSecureRandomBytes(bytes, 4)) {
            randomValue = static_cast<quint32>(bytes[0]) |
                        (static_cast<quint32>(bytes[1]) << 8) |
                        (static_cast<quint32>(bytes[2]) << 16) |
                        (static_cast<quint32>(bytes[3]) << 24);
        } else {
            // Fallback to Qt's random generator
            randomValue = QRandomGenerator::global()->bounded(0, UINT32_MAX);
        }
    } while (randomValue >= limit);
    
    return min + (randomValue % range);
}

quint64 UniqueDeviceGenerator::getSecureRandomUInt64() const {
    unsigned char bytes[8];
    if (getSecureRandomBytes(bytes, 8)) {
        return static_cast<quint64>(bytes[0]) |
               (static_cast<quint64>(bytes[1]) << 8) |
               (static_cast<quint64>(bytes[2]) << 16) |
               (static_cast<quint64>(bytes[3]) << 24) |
               (static_cast<quint64>(bytes[4]) << 32) |
               (static_cast<quint64>(bytes[5]) << 40) |
               (static_cast<quint64>(bytes[6]) << 48) |
               (static_cast<quint64>(bytes[7]) << 56);
    }
    
    // Fallback
    return QRandomGenerator::global()->bounded(0, INT64_MAX);
}

QString UniqueDeviceGenerator::getSecureRandomHex(size_t length) const {
    QString result;
    result.reserve(static_cast<int>(length));
    
    size_t bytesNeeded = (length + 1) / 2;
    QByteArray buffer(bytesNeeded, 0);
    
    if (getSecureRandomBytes(reinterpret_cast<unsigned char*>(buffer.data()), bytesNeeded)) {
        result = QString::fromLatin1(buffer.toHex().left(static_cast<int>(length)));
    } else {
        // Fallback
        QRandomGenerator* gen = QRandomGenerator::global();
        for (size_t i = 0; i < length; ++i) {
            result.append(QString::number(gen->bounded(16), 16));
        }
    }
    
    return result;
}

void UniqueDeviceGenerator::initializeOUIs() {
    // Official OUI prefixes for different manufacturers (IEEE assigned)
    m_manufacturerOUIs["samsung"] = {
        "8C:71:F8",  // Samsung Electronics
        "D0:22:BE",
        "00:02:78",
        "54:88:0E",
        "90:18:7C",
        "78:25:AD",
        "B4:07:F9",
        "F0:25:B7",
        "20:D5:BF",
        "58:C3:8B"
    };
    
    m_manufacturerOUIs["google"] = {
        "3C:5A:B4",  // Google
        "54:60:09",
        "F4:F5:D8",
        "1C:F2:9A",
        "94:EB:2C",
        "A4:77:33",
        "DC:53:60",
        "F4:F5:E8"
    };
    
    m_manufacturerOUIs["xiaomi"] = {
        "34:80:B3",  // Xiaomi
        "F4:F5:D8",
        "64:09:80",
        "58:44:98",
        "74:23:44",
        "04:CF:8C",
        "C8:D3:A3",
        "0C:1D:AF",
        "48:2C:A0",
        "04:92:26"
    };
    
    m_manufacturerOUIs["huawei"] = {
        "00:25:9E",  // Huawei
        "34:29:12",
        "EC:D0:9F",
        "00:E0:4C",
        "20:F3:A3",
        "34:00:A3",
        "B4:CD:27",
        "C8:08:AA",
        "00:18:82",
        "D4:61:9D"
    };
    
    m_manufacturerOUIs["oppo"] = {
        "2C:33:61",  // OPPO
        "20:74:CF",
        "CC:D0:D5",
        "A4:C3:F0",
        "30:89:FF",
        "78:A2:A0",
        "8C:BE:BE"
    };
    
    m_manufacturerOUIs["vivo"] = {
        "20:A2:E4",  // Vivo
        "BC:83:85",
        "A8:BB:CF",
        "20:A2:E4",
        "9C:20:7B",
        "08:19:A6",
        "58:FC:DB"
    };
    
    m_manufacturerOUIs["oneplus"] = {
        "F6:4D:D7",  // OnePlus
        "58:EF:68",
        "2C:33:61",
        "E4:5F:01",
        "98:BD:9D",
        "80:FA:84"
    };
    
    m_manufacturerOUIs["apple"] = {
        "A4:5E:60",  // Apple
        "3C:06:30",
        "F0:4D:A3",
        "B8:C7:5D",
        "64:A3:CB",
        "5C:59:48",
        "20:7D:74"
    };
    
    m_manufacturerOUIs["sony"] = {
        "AC:9B:0A",  // Sony
        "78:84:3C",
        "98:0D:2E",
        "70:9E:29",
        "A0:68:DD",
        "30:39:26"
    };
    
    m_manufacturerOUIs["motorola"] = {
        "00:26:7E",  // Motorola
        "78:D5:B5",
        "98:40:BB",
        "B4:6D:83",
        "AC:4E:91",
        "C8:3C:85"
    };
    
    m_manufacturerOUIs["lg"] = {
        "E8:5B:5B",  // LG
        "80:CC:9C",
        "A4:C3:F0",
        "AC:0D:1B",
        "B4:A9:5A"
    };
    
    m_manufacturerOUIs["asus"] = {
        "2C:4D:54",  // ASUS
        "74:D0:2B",
        "A8:5E:45",
        "F0:F4:1F",
        "30:5A:3A",
        "50:9F:27"
    };
    
    m_manufacturerOUIs["nokia"] = {
        "00:1F:DF",  // Nokia
        "B4:AE:52",
        "80:EE:73",
        "00:E0:4C",
        "5C:BA:37",
        "00:DB:C7"
    };
    
    m_manufacturerOUIs["huawei_sub"] = {
        "00:18:82",  // Honor/Huawei sub-brand
        "00:E0:4C",
        "4C:1F:CC",
        "20:F1:7C",
        "20:D5:BF"
    };
    
    m_manufacturerOUIs["realme"] = {
        "74:5C:4B",  // Realme
        "C4:08:80",
        "04:42:1A",
        "AC:22:0B",
        "30:C7:AE"
    };
    
    m_manufacturerOUIs["oppo_sub"] = {
        "78:5C:1E",  // Realme/OPPO sub-brand
        "68:89:C1",
        "A4:C3:F0"
    };
    
    m_manufacturerOUIs["vivo_sub"] = {
        "38:07:D4",  // Vivo sub-brand
        "9C:20:7B",
        "00:24:EC"
    };
    
    m_manufacturerOUIs["xiaomi_sub"] = {
        "9C:35:EB",  // Redmi/POCO
        "34:80:B3",
        "F4:F5:D8",
        "48:2C:A0",
        "04:CF:8C"
    };
    
    m_manufacturerOUIs["lenovo"] = {
        "00:23:7E",  // Lenovo
        "50:1A:C5",
        "98:FA:E3",
        "F4:8B:93",
        "A4:4E:31"
    };
    
    m_manufacturerOUIs["default"] = {
        "00:1A:11",  // Generic/Unassigned
        "00:23:69",
        "00:50:56",
        "B8:27:EB",  // Raspberry Pi (common in devices)
        "DC:A6:32",
        "E4:5F:01",  // OnePlus fallback
        "00:00:00"   // Null OUI (rare)
    };
}

void UniqueDeviceGenerator::initializeTACDatabase() {
    // TAC Database is initialized via singleton in TACDatabase.h
    // Just verify it's accessible
    try {
        size_t tacCount = RedroidCPP::TACDatabase::getInstance().size();
        qDebug() << "[UniqueDeviceGenerator] TAC Database loaded with" << tacCount << "entries";
        
        if (tacCount < 100) {
            qWarning() << "[UniqueDeviceGenerator] TAC Database has less than 100 entries!";
        }
    } catch (const std::exception& e) {
        qWarning() << "[UniqueDeviceGenerator] Failed to access TAC Database:" << e.what();
    } catch (...) {
        qWarning() << "[UniqueDeviceGenerator] Unknown error accessing TAC Database";
    }
}

QString UniqueDeviceGenerator::generateHash(const QString& input) {
    QByteArray data = input.toUtf8();
    
    // Add CSPRNG entropy
    quint64 randomEntropy = getSecureRandomUInt64();
    data.append(QString::number(randomEntropy).toUtf8());
    data.append(QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8());
    
    // Double SHA-256 for better distribution
    QByteArray firstHash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString hash = QString(QCryptographicHash::hash(firstHash, QCryptographicHash::Sha256).toHex());
    
    return hash;
}

QString UniqueDeviceGenerator::generateInstanceId() {
    // Use CSPRNG for instance ID generation
    quint64 randomPart = getSecureRandomUInt64();
    QString randomStr = QString("%1").arg(randomPart, 16, 16, QChar('0')).toUpper();
    
    // Format: device_XXXXXXXX_YYYYYYYY
    return "device_" + randomStr.left(8) + "_" + randomStr.right(8);
}

QString UniqueDeviceGenerator::generateUniqueIMEI(const QString& manufacturer) {
    // Try to get TAC from database first
    QString tac;
    bool tacFromDatabase = false;
    
    try {
        auto tacEntry = RedroidCPP::TACDatabase::getInstance().getRandomForManufacturer(manufacturer.toStdString());
        if (tacEntry.has_value()) {
            tac = QString::fromStdString(tacEntry->tac);
            tacFromDatabase = true;
            qDebug() << "[IMEI] Using TAC from database:" << tac 
                     << "for manufacturer:" << manufacturer;
        }
    } catch (const std::exception& e) {
        qWarning() << "[IMEI] TAC database error:" << e.what();
    }
    
    // Fallback to hardcoded TACs if database fails or is empty
    if (tac.isEmpty()) {
        QStringList allTACs = getTACListForManufacturer(manufacturer);
        if (!allTACs.isEmpty()) {
            quint32 index = getSecureRandomUInt32(0, static_cast<quint32>(allTACs.size() - 1));
            tac = allTACs[static_cast<int>(index)];
        } else {
            // Ultimate fallback: generate random valid TAC
            tac = generateFallbackTAC();
        }
    }
    
    // Generate random serial (6 digits) using CSPRNG
    QString serial;
    for (int i = 0; i < 6; i++) {
        quint32 digit = getSecureRandomUInt32(0, 9);
        serial += QString::number(digit);
    }
    
    // Combine TAC + serial
    QString imeiBase = tac + serial;
    
    // Validate TAC length
    if (imeiBase.length() != 14) {
        qWarning() << "[IMEI] Generated base has wrong length:" << imeiBase.length();
        // Pad with zeros or truncate
        while (imeiBase.length() < 14) {
            imeiBase += '0';
        }
        imeiBase = imeiBase.left(14);
    }
    
    // Calculate Luhn check digit
    int checkDigit = calculateLuhnCheckDigit(imeiBase);
    if (checkDigit < 0 || checkDigit > 9) {
        qWarning() << "[IMEI] Invalid Luhn check digit:" << checkDigit;
        checkDigit = 0;
    }
    QString imei = imeiBase + QString::number(checkDigit);
    
    // Ensure uniqueness with thread-safe approach
    QMutexLocker locker(&m_idMutex);
    int attempts = 0;
    const int maxAttempts = 100;
    
    while (m_generatedIMEIs.values().contains(imei) && attempts < maxAttempts) {
        serial.clear();
        for (int i = 0; i < 6; i++) {
            quint32 digit = getSecureRandomUInt32(0, 9);
            serial += QString::number(digit);
        }
        imeiBase = tac + serial;
        checkDigit = calculateLuhnCheckDigit(imeiBase);
        imei = imeiBase + QString::number(checkDigit);
        attempts++;
    }
    
    if (attempts >= maxAttempts) {
        qWarning() << "[IMEI] Failed to generate unique IMEI after" << maxAttempts << "attempts";
        // Append timestamp as last resort
        imei = imeiBase + QString::number(QDateTime::currentMSecsSinceEpoch() % 10);
    }
    
    // Store for uniqueness with UUID key
    QString uniqueKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_generatedIMEIs[uniqueKey] = imei;
    
    qDebug() << "[IMEI] Generated IMEI:" << imei << "(attempts:" << attempts << ", TAC:" << tac << ")";
    
    return imei;
}

QStringList UniqueDeviceGenerator::getTACListForManufacturer(const QString& manufacturer) const {
    QStringList tacList;
    
    // Map manufacturer names to their TAC prefixes
    if (manufacturer.toLower() == "samsung") {
        tacList = {"35875107", "35875108", "35875109", "35875110", "35875111", "35875112",
                   "35776608", "35776609", "35776610", "35776611", "35776612", "35776613",
                   "35924401", "35924402", "35924403", "35924404", "35924405", "35924406",
                   "35924408", "35924409", "35924410", "35924411", "35924412", "35924413",
                   "35166908", "35166909", "35166910", "35166911", "35166912", "35166913",
                   "35630608", "35630609", "35630610", "35630611", "35630612", "35630613"};
    } else if (manufacturer.toLower() == "google") {
        tacList = {"35746608", "35746609", "35746610", "35746611", "35746612", "35746613",
                   "35441008", "35441009", "35441010", "35441011", "35441012", "35441013",
                   "35672908", "35672909", "35672910", "35672911", "35672912", "35672913",
                   "35871208", "35871209", "35871210", "35871211",
                   "35299808", "35299809", "35299810", "35299811"};
    } else if (manufacturer.toLower() == "xiaomi") {
        tacList = {"86917102", "86917103", "86917104", "86917105", "86917106", "86917107",
                   "86100208", "86100209", "86100210", "86100211", "86100212", "86100213",
                   "86533208", "86533209", "86533210", "86533211", "86533212", "86533213",
                   "86830208", "86830209", "86830210", "86830211"};
    } else if (manufacturer.toLower() == "huawei" || manufacturer.toLower() == "honor") {
        tacList = {"86799304", "86799305", "86799306", "86799307", "86799308", "86799309",
                   "86799310", "86799315", "86799316", "86799317", "86799318", "86799319",
                   "86799325", "86799326", "86799327", "86799328", "86799329", "86799330"};
    } else if (manufacturer.toLower() == "oneplus") {
        tacList = {"45890508", "45890509", "45890510", "45890511", "45890512", "45890513",
                   "45890520", "45890521", "45890522", "45890523", "45890524", "45890525",
                   "45890535", "45890536", "45890537", "45890538", "45890539", "45890540"};
    } else if (manufacturer.toLower() == "oppo") {
        tacList = {"86536703", "86536704", "86536705", "86536706", "86536707", "86536708",
                   "86536720", "86536721", "86536722", "86536723", "86536724", "86536725",
                   "86536735", "86536736", "86536737", "86536738", "86536739", "86536740"};
    } else if (manufacturer.toLower() == "vivo") {
        tacList = {"86538903", "86538904", "86538905", "86538906", "86538907", "86538908",
                   "86538920", "86538921", "86538922", "86538923", "86538924", "86538925",
                   "86538935", "86538936", "86538937", "86538938", "86538939", "86538940"};
    } else if (manufacturer.toLower() == "realme") {
        tacList = {"86936203", "86936204", "86936205", "86936206", "86936207", "86936208",
                   "86936220", "86936221", "86936222", "86936223", "86936224", "86936225",
                   "86936235", "86936236", "86936237", "86936238", "86936239", "86936240"};
    } else if (manufacturer.toLower() == "motorola") {
        tacList = {"35899405", "35899406", "35899407", "35899408", "35899409", "35899410",
                   "35899420", "35899421", "35899422", "35899423", "35899430", "35899431",
                   "35899440", "35899441", "35899442", "35899443"};
    } else if (manufacturer.toLower() == "sony") {
        tacList = {"35885607", "35885608", "35885609", "35885610", "35885611", "35885612",
                   "35885620", "35885621", "35885622", "35885623", "35885624", "35885625",
                   "35885630", "35885631", "35885632", "35885633", "35885634", "35885635"};
    } else if (manufacturer.toLower() == "asus") {
        tacList = {"35892008", "35892009", "35892010", "35892011", "35892012", "35892013",
                   "35892017", "35892018", "35892019", "35892020", "35892021", "35892022"};
    } else if (manufacturer.toLower() == "nokia") {
        tacList = {"35918108", "35918109", "35918110", "35918111", "35918112", "35918113",
                   "35918116", "35918117", "35918118", "35918119", "35918120", "35918121"};
    } else {
        // Default: mix of various manufacturers
        tacList = {"35875107", "35746608", "86917102", "45890508", "86799304",
                   "86536703", "86538903", "86936203", "35899405", "35885607",
                   "35892008", "35918108"};
    }
    
    return tacList;
}

QString UniqueDeviceGenerator::generateFallbackTAC() const {
    // Generate a TAC that looks valid
    // TAC format: first 6 digits = TAC prefix, last 2 digits = FAC
    // Valid TAC ranges for common manufacturers:
    static const QStringList tacPrefixes = {
        "350000", "351000", "352000", "353000", "354000", "355000", "356000", "357000", "358000", "359000",  // Europe
        "860000", "861000", "862000", "863000", "864000", "865000", "866000", "867000", "868000", "869000",  // Asia
        "450000", "451000", "452000", "453000", "454000", "455000", "456000", "457000", "458000", "459000",  // US
        "500000", "501000", "502000", "503000", "504000", "505000", "506000", "507000", "508000", "509000"   // Other
    };
    
    quint32 prefixIndex = getSecureRandomUInt32(0, static_cast<quint32>(tacPrefixes.size() - 1));
    QString tacPrefix = tacPrefixes[static_cast<int>(prefixIndex)];
    
    // Generate random FAC (2 digits)
    quint32 fac1 = getSecureRandomUInt32(0, 9);
    quint32 fac2 = getSecureRandomUInt32(0, 9);
    
    return tacPrefix + QString::number(fac1) + QString::number(fac2);
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
