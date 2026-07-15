#include "DeviceIDGenerator.hpp"
#include "Logger.hpp"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace AntiDetect {

DeviceIDGenerator::DeviceIDGenerator()
    : m_initialized(false)
{
    // Initialize TAC prefixes for different brands
    m_tacPrefixes = {
        {"samsung", "356abc"},      // Samsung TAC prefix
        {"google", "35aabb"},      // Google/Pixel TAC prefix
        {"xiaomi", "869abb"},      // Xiaomi TAC prefix
        {"oneplus", "86aabb"},     // OnePlus TAC prefix
        {"huawei", "861234"},      // Huawei TAC prefix
        {"oppo", "86abcd"},        // OPPO TAC prefix
        {"vivo", "861234"},        // Vivo TAC prefix
        {"realme", "869abc"},      // Realme TAC prefix
        {"asus", "3580ab"},        // ASUS TAC prefix
        {"sony", "356abc"},        // Sony TAC prefix
        {"lg", "354abc"},          // LG TAC prefix
        {"motorola", "350abc"},    // Motorola TAC prefix
        {"nokia", "358abc"},       // Nokia TAC prefix
        {"generic", "869abc"}       // Generic TAC prefix
    };
    
    // Serial number formats by brand
    m_serialFormats = {
        {"samsung", "R{month}{day}{hour}{minute}{random}"},
        {"google", "G{yyyy}{mm}{dd}{random}"},
        {"xiaomi", "{random}{random2}"},
        {"oneplus", "OP{yyyy}{mm}{random}"},
        {"huawei", "HW{random}"},
        {"generic", "{random10}"}
    };
    
    // MAC address OUIs (first 3 bytes) by brand
    m_brandOUIs = {
        {"samsung", "F8:A9:63"},
        {"google", "F4:F5:D8"},
        {"xiaomi", "58:44:98"},
        {"oneplus", "38:2C:4A"},
        {"huawei", "00:25:9E"},
        {"apple", "00:03:93"},
        {"asus", "00:1E:8C"},
        {"sony", "00:1F:E1"},
        {"lg", "00:1F:6B"},
        {"motorola", "00:1E:C0"},
        {"intel", "00:1B:21"},
        {"broadcom", "00:10:18"},
        {"qualcomm", "00:03:7A"},
        {"generic", "AA:BB:CC"}
    };
}

DeviceIDGenerator::~DeviceIDGenerator() {
    shutdown();
}

DeviceIDGenerator& DeviceIDGenerator::getInstance() {
    static DeviceIDGenerator instance;
    return instance;
}

bool DeviceIDGenerator::initialize() {
    if (m_initialized) {
        return true;
    }
    
    Logger::getInstance().info("Initializing Device ID Generator...");
    
    // Seed random number generator
    std::random_device rd;
    m_random.seed(rd());
    
    m_initialized = true;
    Logger::getInstance().info("Device ID Generator initialized");
    
    return true;
}

void DeviceIDGenerator::shutdown() {
    if (!m_initialized) return;
    m_initialized = false;
}

std::string DeviceIDGenerator::generateRandomDigits(size_t count) {
    std::uniform_int_distribution<> dis(0, 9);
    std::stringstream ss;
    for (size_t i = 0; i < count; i++) {
        ss << dis(m_random);
    }
    return ss.str();
}

std::string DeviceIDGenerator::bytesToHex(const std::vector<unsigned char>& bytes) {
    std::stringstream ss;
    for (const auto& b : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

int DeviceIDGenerator::calculateLuhnCheckDigit(const std::string& baseNumber) const {
    int sum = 0;
    bool alternate = true;
    
    for (int i = static_cast<int>(baseNumber.length()) - 1; i >= 0; i--) {
        int n = baseNumber[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

bool DeviceIDGenerator::verifyLuhn(const std::string& number) const {
    int sum = 0;
    bool alternate = false;
    
    for (int i = static_cast<int>(number.length()) - 1; i >= 0; i--) {
        if (!isdigit(number[i])) {
            return false;
        }
        int n = number[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (sum % 10 == 0);
}

std::string DeviceIDGenerator::getBrandTAC(const std::string& brand) {
    std::string lowerBrand = brand;
    std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
    
    auto it = m_tacPrefixes.find(lowerBrand);
    if (it != m_tacPrefixes.end()) {
        return it->second;
    }
    
    // Return generic TAC if brand not found
    return m_tacPrefixes["generic"];
}

std::string DeviceIDGenerator::generateIMEI(const std::string& brand) {
    // Get TAC (Type Allocation Code) - first 8 digits
    std::string tac = getBrandTAC(brand);
    
    // Add 6 random digits to make 14 digits
    std::string serial = generateRandomDigits(6);
    std::string baseIMEI = tac + serial;
    
    // Calculate and append Luhn check digit
    int checkDigit = calculateLuhnCheckDigit(baseIMEI);
    std::string imei = baseIMEI + std::to_string(checkDigit);
    
    return imei;
}

std::string DeviceIDGenerator::generateValidIMEI(const std::string& baseIMEI) {
    if (baseIMEI.length() >= 14) {
        std::string base = baseIMEI.substr(0, 14);
        int checkDigit = calculateLuhnCheckDigit(base);
        return base + std::to_string(checkDigit);
    }
    return generateIMEI("generic");
}

bool DeviceIDGenerator::validateIMEI(const std::string& imei) const {
    if (imei.length() != 15) {
        return false;
    }
    return verifyLuhn(imei);
}

std::string DeviceIDGenerator::generateSamsungSerial() {
    // Samsung serial format: R + 2-digit year + 2-digit month + 2-digit day + 4 random
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    
    std::stringstream ss;
    ss << "R";
    ss << std::setw(2) << std::setfill('0') << (tm->tm_year % 100);
    ss << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1);
    ss << std::setw(2) << std::setfill('0') << tm->tm_mday;
    ss << generateRandomDigits(4);
    
    return ss.str();
}

std::string DeviceIDGenerator::generateGoogleSerial() {
    // Google/Pixel serial format: YearMonthDay + 6 random
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    
    std::stringstream ss;
    ss << "G";
    ss << std::setw(4) << std::setfill('0') << (tm->tm_year + 1900);
    ss << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1);
    ss << std::setw(2) << std::setfill('0') << tm->tm_mday;
    ss << generateRandomDigits(6);
    
    return ss.str();
}

std::string DeviceIDGenerator::generateXiaomiSerial() {
    // Xiaomi serial: 13 alphanumeric characters
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::stringstream ss;
    for (int i = 0; i < 13; i++) {
        ss << chars[dis(m_random)];
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateOnePlusSerial() {
    // OnePlus serial: OP + YearMonth + 6 random
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    
    std::stringstream ss;
    ss << "OP";
    ss << std::setw(4) << std::setfill('0') << (tm->tm_year + 1900);
    ss << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1);
    ss << generateRandomDigits(6);
    
    return ss.str();
}

std::string DeviceIDGenerator::generateHuaweiSerial() {
    // Huawei serial: HW + 13 alphanumeric
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::stringstream ss;
    ss << "HW";
    for (int i = 0; i < 13; i++) {
        ss << chars[dis(m_random)];
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateSerialNumber(const std::string& brand) {
    std::string lowerBrand = brand;
    std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
    
    if (lowerBrand.find("samsung") != std::string::npos) {
        return generateSamsungSerial();
    } else if (lowerBrand.find("google") != std::string::npos || lowerBrand.find("pixel") != std::string::npos) {
        return generateGoogleSerial();
    } else if (lowerBrand.find("xiaomi") != std::string::npos || lowerBrand.find("redmi") != std::string::npos) {
        return generateXiaomiSerial();
    } else if (lowerBrand.find("oneplus") != std::string::npos) {
        return generateOnePlusSerial();
    } else if (lowerBrand.find("huawei") != std::string::npos || lowerBrand.find("honor") != std::string::npos) {
        return generateHuaweiSerial();
    }
    
    // Default: alphanumeric serial
    return generateRandomSerial(10);
}

std::string DeviceIDGenerator::generateRandomSerial(size_t length) {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << chars[dis(m_random)];
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateGSFId() {
    // GSF ID format: "af-" + 12 digits
    std::stringstream ss;
    ss << "af-" << generateRandomDigits(12);
    return ss.str();
}

std::string DeviceIDGenerator::generateAndroidId() {
    // Android ID: 16 hex characters (lowercase)
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 16; i++) {
        std::uniform_int_distribution<> dis(0, 15);
        int val = dis(m_random);
        ss << std::setw(1) << val;
    }
    
    std::string result = ss.str();
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string DeviceIDGenerator::generateRandomMAC() {
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;
    
    for (int i = 0; i < 6; i++) {
        if (i > 0) ss << ":";
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(m_random);
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateMACAddress(const std::string& type) {
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    
    // Use generic OUI
    std::string oui = m_brandOUIs["generic"];
    
    // Add 3 random bytes
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;
    ss << oui << ":";
    for (int i = 0; i < 3; i++) {
        if (i > 0) ss << ":";
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(m_random);
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateBSSID(const std::string& brand) {
    std::string lowerBrand = brand;
    std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
    
    std::string oui;
    auto it = m_brandOUIs.find(lowerBrand);
    if (it != m_brandOUIs.end()) {
        oui = it->second;
    } else {
        oui = m_brandOUIs["generic"];
    }
    
    // BSSID format: XX:XX:XX:XX:XX:XX
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;
    ss << oui << ":";
    for (int i = 0; i < 3; i++) {
        if (i > 0) ss << ":";
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(m_random);
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateRandomSSID() {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::stringstream ss;
    int length = 8 + dis(m_random) % 8; // 8-15 characters
    
    for (int i = 0; i < length; i++) {
        ss << chars[dis(m_random)];
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateBluetoothAddress() {
    // Bluetooth address format: XX:XX:XX:XX:XX:XX
    // Local Bluetooth addresses are in form XX:XX:XX:XX:XX:XX where X is even
    std::uniform_int_distribution<> dis(0, 126); // Even numbers only (0-126)
    
    std::stringstream ss;
    for (int i = 0; i < 6; i++) {
        if (i > 0) ss << ":";
        int val = dis(m_random) * 2; // Make it even
        ss << std::hex << std::setw(2) << std::setfill('0') << val;
    }
    
    return ss.str();
}

std::string DeviceIDGenerator::generateProfileBoundId(const std::string& profileId) {
    // Generate deterministic but unique ID based on profile
    std::stringstream ss;
    ss << profileId << "-" << generateRandomDigits(8);
    return ss.str();
}

void DeviceIDGenerator::setBrandTACPrefix(const std::string& brand, const std::string& prefix) {
    std::string lowerBrand = brand;
    std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
    m_tacPrefixes[lowerBrand] = prefix;
}

std::map<std::string, std::string> DeviceIDGenerator::generateCompleteDeviceIdentity(
    const std::string& brand, const std::string& model) {
    
    std::map<std::string, std::string> identity;
    
    // Generate all IDs
    identity["imei"] = generateIMEI(brand);
    identity["serial_number"] = generateSerialNumber(brand);
    identity["gsf_id"] = generateGSFId();
    identity["android_id"] = generateAndroidId();
    identity["wifi_mac"] = generateMACAddress("wifi");
    identity["bluetooth_mac"] = generateBluetoothAddress();
    identity["bssid"] = generateBSSID(brand);
    
    return identity;
}

} // namespace AntiDetect
