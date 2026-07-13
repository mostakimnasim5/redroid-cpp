/**
 * @file DeviceProfile.cpp
 * @brief Implementation of Complete Device Profile
 * @version 3.0.0
 * 
 * Professional implementation of all device profile components.
 */

#include "Core/DeviceProfile.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <ctime>

namespace RedroidCPP {

// ============================================================================
// Utility Functions
// ============================================================================

namespace {
std::string generateUUID() {
    static const char hexChars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::string uuid;
    for (int i = 0; i < 32; ++i) {
        if (i == 8 || i == 12 || i == 16 || i == 20) uuid += '-';
        uuid += hexChars[dis(gen)];
    }
    return uuid;
}

std::string generateHex(int length) {
    static const char hexChars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += hexChars[dis(gen)];
    }
    return result;
}

std::string generateNumeric(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += std::to_string(dis(gen));
    }
    return result;
}

std::string formatMAC(const std::string& mac) {
    std::string result;
    for (size_t i = 0; i < mac.length(); ++i) {
        if (i > 0 && i % 2 == 0 && mac[i] != ':') {
            result += ':';
        }
        if (mac[i] != ':') {
            result += static_cast<char>(std::toupper(mac[i]));
        }
    }
    return result;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string getCurrentDateUTC() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%a %b %d %H:%M:%S UTC %Y");
    return oss.str();
}
}

// ============================================================================
// IMEI Generator Implementation
// ============================================================================

int IMEIGenerator::calculateLuhnCheckDigit(const std::string& base) {
    int sum = 0;
    bool alternate = true;
    
    for (int i = static_cast<int>(base.length()) - 1; i >= 0; --i) {
        int n = base[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n = (n % 10) + 1;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

std::string IMEIGenerator::generate(const std::string& tac) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    
    std::string imei = tac;
    for (int i = 0; i < 6; ++i) {
        imei += std::to_string(dis(gen));
    }
    
    int checkDigit = calculateLuhnCheckDigit(imei);
    imei += std::to_string(checkDigit);
    
    return imei;
}

bool IMEIGenerator::validate(const std::string& imei) {
    if (imei.length() != 15) return false;
    
    for (char c : imei) {
        if (!std::isdigit(c)) return false;
    }
    
    int sum = 0;
    bool alternate = true;
    for (int i = 14; i >= 0; --i) {
        int n = imei[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n = (n % 10) + 1;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

// ============================================================================
// MAC Generator Implementation
// ============================================================================

std::string MACGenerator::getOUIForManufacturer(const std::string& manufacturer) {
    static const std::map<std::string, std::string> ouiMap = {
        {"Samsung", "8C:71:F8"},
        {"Google", "94:EB:2C"},
        {"Xiaomi", "64:09:80"},
        {"OnePlus", "F8:39:6B"},
        {"Sony", "AC:2B:6E"},
        {"OPPO", "88:C9:D0"},
        {"Vivo", "2C:33:61"},
        {"Huawei", "20:F3:A3"},
        {"Motorola", "A4:39:A6"},
        {"Realme", "88:C9:D0"},
        {"ASUS", "04:D3:C2"},
        {"Nokia", "00:27:1A"},
        {"Apple", "00:26:08"},
        {"LG", "AC:9B:0A"},
        {"HTC", "64:A7:69"},
        {"BlackBerry", "98:9E:63"}
    };
    
    auto it = ouiMap.find(manufacturer);
    if (it != ouiMap.end()) {
        return it->second;
    }
    return "00:00:00";
}

std::string MACGenerator::generate(const std::string& oui) {
    std::string mac = oui;
    mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
    
    std::random_device rd;
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < 3; ++i) {
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << dis(rd);
        mac += oss.str();
    }
    
    return formatMAC(mac);
}

// ============================================================================
// TAC Database Implementation
// ============================================================================

TACDatabase::TACDatabase() {
    initializeDatabase();
}

void TACDatabase::initializeDatabase() {
    // Samsung
    addTAC("35875109", "Samsung", "Galaxy S24 Ultra", "dm3q", "Smartphone", "High-End", "2024", "01");
    addTAC("35875108", "Samsung", "Galaxy S24+", "z3s", "Smartphone", "High-End", "2024", "01");
    addTAC("35875107", "Samsung", "Galaxy S24", "z4s", "Smartphone", "High-End", "2024", "01");
    addTAC("35776608", "Samsung", "Galaxy S23 Ultra", "dm3", "Smartphone", "High-End", "2023", "02");
    addTAC("35776609", "Samsung", "Galaxy S23+", "z3", "Smartphone", "High-End", "2023", "02");
    addTAC("35776610", "Samsung", "Galaxy S23", "z4", "Smartphone", "High-End", "2023", "02");
    addTAC("35924408", "Samsung", "Galaxy Z Fold5", "q5", "Foldable", "Premium", "2023", "08");
    addTAC("35924409", "Samsung", "Galaxy Z Flip5", "q5s", "Foldable", "Premium", "2023", "08");
    addTAC("35166908", "Samsung", "Galaxy A54", "a5x", "Smartphone", "Mid-Range", "2023", "03");
    addTAC("35166909", "Samsung", "Galaxy A34", "a3x", "Smartphone", "Mid-Range", "2023", "03");
    
    // Google
    addTAC("35746608", "Google", "Pixel 8 Pro", "husky", "Smartphone", "High-End", "2023", "10");
    addTAC("35746609", "Google", "Pixel 8", "shiba", "Smartphone", "High-End", "2023", "10");
    addTAC("35441008", "Google", "Pixel 7 Pro", "cheetah", "Smartphone", "High-End", "2022", "10");
    addTAC("35441009", "Google", "Pixel 7", "panther", "Smartphone", "High-End", "2022", "10");
    addTAC("35672908", "Google", "Pixel 6 Pro", "raven", "Smartphone", "High-End", "2021", "10");
    addTAC("35672909", "Google", "Pixel 6", "oriole", "Smartphone", "High-End", "2021", "10");
    addTAC("35871208", "Google", "Pixel Fold", "felix", "Foldable", "Premium", "2023", "06");
    addTAC("35546708", "Google", "Pixel 7a", "lynx", "Smartphone", "Mid-Range", "2023", "05");
    
    // Xiaomi
    addTAC("86917102", "Xiaomi", "Xiaomi 14 Pro", "sm8550", "Smartphone", "High-End", "2024", "02");
    addTAC("86917103", "Xiaomi", "Xiaomi 14", "sm8550", "Smartphone", "High-End", "2024", "02");
    addTAC("86917104", "Xiaomi", "Xiaomi 13 Pro", "sm8550", "Smartphone", "High-End", "2023", "12");
    addTAC("86917105", "Xiaomi", "Xiaomi 13", "sm8550", "Smartphone", "High-End", "2023", "12");
    addTAC("86100208", "Redmi", "Redmi K70 Pro", "m20", "Smartphone", "High-End Gaming", "2024", "01");
    addTAC("86100209", "Redmi", "Redmi K70", "m20", "Smartphone", "High-End Gaming", "2024", "01");
    addTAC("86533208", "Xiaomi", "POCO F6 Pro", "poco_f", "Smartphone", "Gaming", "2024", "05");
    addTAC("86533209", "Xiaomi", "POCO F6", "poco_f", "Smartphone", "Gaming", "2024", "05");
    
    // OnePlus
    addTAC("45890508", "OnePlus", "OnePlus 12", "OP595", "Smartphone", "High-End", "2024", "01");
    addTAC("45890509", "OnePlus", "OnePlus 12R", "OP595", "Smartphone", "Mid-Range", "2024", "01");
    addTAC("45890510", "OnePlus", "OnePlus 11", "OP555", "Smartphone", "High-End", "2023", "01");
    addTAC("45890511", "OnePlus", "OnePlus 10T", "OP541", "Smartphone", "High-End", "2022", "08");
    
    // OPPO
    addTAC("86536703", "OPPO", "Find X7 Pro", "CPH259", "Smartphone", "High-End", "2024", "01");
    addTAC("86536704", "OPPO", "Find X6 Pro", "CPH255", "Smartphone", "High-End", "2023", "03");
    addTAC("86536705", "OPPO", "Reno 11 Pro", "CPH2599", "Smartphone", "Mid-High", "2024", "01");
    addTAC("86536706", "OPPO", "Reno 10 Pro", "CPH254", "Smartphone", "Mid-High", "2023", "05");
    
    // Vivo
    addTAC("86538903", "Vivo", "X100 Pro", "V2245", "Smartphone", "High-End Camera", "2023", "11");
    addTAC("86538904", "Vivo", "X90 Pro+", "V2227A", "Smartphone", "High-End Camera", "2022", "11");
    addTAC("86538905", "Vivo", "V30 Pro", "V2318", "Smartphone", "Mid-High", "2024", "02");
    addTAC("86538906", "Vivo", "V27 Pro", "V2230", "Smartphone", "Mid-Range", "2023", "03");
    
    // Huawei
    addTAC("86799304", "Huawei", "Mate 60 Pro", "ALN-NX9", "Smartphone", "High-End", "2023", "08");
    addTAC("86799305", "Huawei", "P60 Pro", "LNA-NX9", "Smartphone", "High-End Camera", "2023", "03");
    addTAC("86799306", "Huawei", "Mate X5", "ALT-NX9", "Foldable", "Premium", "2023", "09");
    addTAC("86799307", "Huawei", "P50 Pro", "JAD-LX9", "Smartphone", "High-End Camera", "2021", "08");
    
    // Motorola
    addTAC("35899405", "Motorola", "Edge 50 Ultra", "XT240", "Smartphone", "High-End", "2024", "04");
    addTAC("35899406", "Motorola", "Edge 40 Pro", "XT230", "Smartphone", "High-End", "2023", "05");
    addTAC("35899407", "Motorola", "Edge 30 Ultra", "XT220", "Smartphone", "High-End", "2022", "09");
    addTAC("35899408", "Motorola", "Moto G84", "XT234", "Smartphone", "Mid-Range", "2023", "09");
    
    // Sony
    addTAC("35885607", "Sony", "Xperia 1 V", "PDX-245", "Smartphone", "High-End", "2023", "06");
    addTAC("35885608", "Sony", "Xperia 5 V", "PDX-245s", "Smartphone", "High-End Compact", "2023", "09");
    addTAC("35885609", "Sony", "Xperia 1 IV", "PDX-244", "Smartphone", "High-End", "2022", "06");
    addTAC("35885610", "Sony", "Xperia 10 V", "XQ-DC72", "Smartphone", "Mid-Range", "2023", "06");
    
    // ASUS
    addTAC("35892008", "ASUS", "ROG Phone 8 Pro", "AI2401", "Smartphone", "Gaming Flagship", "2024", "01");
    addTAC("35892009", "ASUS", "ROG Phone 7 Ultimate", "AI2205", "Smartphone", "Gaming Flagship", "2023", "04");
    addTAC("35892010", "ASUS", "Zenfone 10", "AI2302", "Smartphone", "Compact High-End", "2023", "06");
    addTAC("35892011", "ASUS", "ROG Phone 6D", "AI2203", "Smartphone", "Gaming", "2022", "12");
    
    // Realme
    addTAC("86936203", "Realme", "realme GT 5 Pro", "RMX388", "Smartphone", "High-End Gaming", "2023", "12");
    addTAC("86936204", "Realme", "realme GT 3", "RMX369", "Smartphone", "High-End Gaming", "2023", "03");
    addTAC("86936205", "Realme", "realme 11 Pro+", "RMX374", "Smartphone", "Mid-High", "2023", "09");
    addTAC("86936206", "Realme", "realme C67", "RMX389", "Smartphone", "Budget", "2023", "12");
    
    // Nokia
    addTAC("35918108", "Nokia", "Nokia G42", "TA-158", "Smartphone", "Mid-Range", "2023", "06");
    addTAC("35918109", "Nokia", "Nokia X30", "TA-145", "Smartphone", "Mid-Range", "2022", "09");
    addTAC("35918110", "Nokia", "Nokia G60 5G", "TA-147", "Smartphone", "Mid-Range 5G", "2022", "09");
    addTAC("35918111", "Nokia", "Nokia C32", "TA-155", "Smartphone", "Budget", "2023", "03");
}

void TACDatabase::addTAC(const std::string& tac, const std::string& brand,
                          const std::string& model, const std::string& internal,
                          const std::string& type, const std::string& cls,
                          const std::string& year, const std::string& month) {
    TACEntry entry;
    entry.tac = tac;
    entry.brand = brand;
    entry.modelName = model;
    entry.internalName = internal;
    entry.deviceType = type;
    entry.deviceClass = cls;
    entry.launchYear = year;
    entry.launchMonth = month;
    
    m_tacMap[tac] = entry;
    m_manufacturerTACs[brand].push_back(tac);
}

TACDatabase& TACDatabase::getInstance() {
    static TACDatabase instance;
    return instance;
}

std::optional<TACEntry> TACDatabase::getByTAC(const std::string& tac) const {
    auto it = m_tacMap.find(tac);
    if (it != m_tacMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<TACEntry> TACDatabase::getRandomForManufacturer(const std::string& manufacturer) {
    auto it = m_manufacturerTACs.find(manufacturer);
    if (it == m_manufacturerTACs.end()) {
        return getRandom();
    }
    
    const auto& tacs = it->second;
    if (tacs.empty()) {
        return std::nullopt;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, tacs.size() - 1);
    
    return m_tacMap.at(tacs[dis(gen)]);
}

std::vector<TACEntry> TACDatabase::getByManufacturer(const std::string& manufacturer) {
    std::vector<TACEntry> entries;
    auto it = m_manufacturerTACs.find(manufacturer);
    if (it != m_manufacturerTACs.end()) {
        for (const auto& tac : it->second) {
            entries.push_back(m_tacMap.at(tac));
        }
    }
    return entries;
}

std::vector<std::string> TACDatabase::getManufacturers() const {
    std::vector<std::string> manufacturers;
    for (const auto& pair : m_manufacturerTACs) {
        manufacturers.push_back(pair.first);
    }
    std::sort(manufacturers.begin(), manufacturers.end());
    return manufacturers;
}

std::optional<TACEntry> TACDatabase::getRandom() {
    if (m_tacMap.empty()) {
        return std::nullopt;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, m_tacMap.size() - 1);
    
    auto it = m_tacMap.begin();
    std::advance(it, dis(gen));
    return it->second;
}

// ============================================================================
// Carrier Database Implementation
// ============================================================================

CarrierDatabase::CarrierDatabase() {
    initialize();
}

void CarrierDatabase::initialize() {
    // US Carriers
    m_carriers.push_back({"T-Mobile", "US", "310", "260"});
    m_carriers.push_back({"AT&T", "US", "310", "410"});
    m_carriers.push_back({"Verizon", "US", "311", "480"});
    m_carriers.push_back({"T-Mobile", "US", "310", "260"});
    m_carriers.push_back({"Sprint", "US", "310", "120"});
    m_carriers.push_back({"US Cellular", "US", "311", "350"});
    
    // UK Carriers
    m_carriers.push_back({"Vodafone", "UK", "234", "15"});
    m_carriers.push_back({"O2", "UK", "234", "10"});
    m_carriers.push_back({"EE", "UK", "234", "30"});
    m_carriers.push_back({"Three", "UK", "234", "20"});
    
    // Europe
    m_carriers.push_back({"Deutsche Telekom", "DE", "262", "01"});
    m_carriers.push_back({"Vodafone DE", "DE", "262", "02"});
    m_carriers.push_back({"O2 DE", "DE", "262", "07"});
    m_carriers.push_back({"Orange FR", "FR", "208", "01"});
    m_carriers.push_back({"SFR", "FR", "208", "10"});
    m_carriers.push_back({"Bouygues", "FR", "208", "20"});
    
    // Asia
    m_carriers.push_back({"Grameenphone", "BD", "470", "01"});
    m_carriers.push_back({"Robi", "BD", "470", "04"});
    m_carriers.push_back({"Banglalink", "BD", "470", "07"});
    m_carriers.push_back({"Airtel", "BD", "470", "06"});
    m_carriers.push_back({"Jazz", "PK", "410", "06"});
    m_carriers.push_back({"Warid", "PK", "410", "03"});
    
    // Others
    m_carriers.push_back({"Telia", "SE", "240", "01"});
    m_carriers.push_back({"Telenor", "NO", "242", "02"});
    m_carriers.push_back({"Swisscom", "CH", "228", "01"});
    m_carriers.push_back({"Optus", "AU", "505", "02"});
    m_carriers.push_back({"Telstra", "AU", "505", "01"});
}

CarrierDatabase& CarrierDatabase::getInstance() {
    static CarrierDatabase instance;
    return instance;
}

CarrierInfo CarrierDatabase::getRandom() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, m_carriers.size() - 1);
    return m_carriers[dis(gen)];
}

std::vector<CarrierInfo> CarrierDatabase::getByCountry(const std::string& country) {
    std::vector<CarrierInfo> result;
    for (const auto& carrier : m_carriers) {
        if (carrier.country == country) {
            result.push_back(carrier);
        }
    }
    return result;
}

// ============================================================================
// Device Identity Implementation
// ============================================================================

void DeviceIdentity::generate(const std::string& tac, const std::string& manufacturer) {
    // Generate IMEI with Luhn validation
    imei = IMEIGenerator::generate(tac);
    imei2 = IMEIGenerator::generate(tac);
    
    // Generate Serial Number based on manufacturer
    if (manufacturer == "Samsung") {
        static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::string serial;
        for (int i = 0; i < 12; ++i) {
            serial += chars[rd() % 36];
        }
        serialNumber = serial;
    } else if (manufacturer == "Google") {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10000000, 99999999);
        serialNumber = "R" + std::to_string(dis(gen));
    } else if (manufacturer == "Xiaomi") {
        static const char hexChars[] = "0123456789ABCDEF";
        std::random_device rd;
        std::string serial;
        for (int i = 0; i < 12; ++i) {
            serial += hexChars[rd() % 16];
        }
        serialNumber = serial;
    } else {
        serialNumber = generateNumeric(12);
    }
    
    // Generate Android ID (16 hex chars)
    androidId = generateHex(16);
    
    // Generate GSF ID (10 digits)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000000000, 9999999999);
    gsfId = std::to_string(dis(gen));
    
    // Generate Advertising ID (AAID) - 36 chars with dashes
    advertisingId = generateUUID();
}

bool DeviceIdentity::validate() const {
    if (!IMEIGenerator::validate(imei)) return false;
    if (imei.length() != 15) return false;
    if (androidId.length() != 16) return false;
    if (serialNumber.empty()) return false;
    return true;
}

std::map<std::string, std::string> DeviceIdentity::toMap() const {
    return {
        {"ro.gsm.deviceId", imei},
        {"ro.gsm.deviceId2", imei2},
        {"ro.serialno", serialNumber},
        {"ro.androidId", androidId},
        {"com.google.gsf.deviceid", gsfId},
        {"advertising_id", advertisingId}
    };
}

// ============================================================================
// MAC Addresses Implementation
// ============================================================================

void MACAddresses::generate(const std::string& manufacturer) {
    std::string oui = MACGenerator::getOUIForManufacturer(manufacturer);
    
    wifiMac = MACGenerator::generate(oui);
    bluetoothMac = MACGenerator::generate(oui);
    ethernetMac = MACGenerator::generate(oui);
    
    wifiInterface = "wlan0";
    ethernetInterface = "eth0";
}

bool MACAddresses::isValid() const {
    if (wifiMac.length() != 17) return false;
    if (bluetoothMac.length() != 17) return false;
    return true;
}

std::map<std::string, std::string> MACAddresses::toMap() const {
    return {
        {"wifi.interface", wifiInterface},
        {"wifi.mac", wifiMac},
        {"bluetooth.interface", "bt0"},
        {"bluetooth.address", bluetoothMac},
        {"ethernet.interface", ethernetInterface},
        {"ethernet.mac", ethernetMac}
    };
}

// ============================================================================
// SIM Configuration Implementation
// ============================================================================

void SIMConfiguration::generate() {
    auto& carrierDb = CarrierDatabase::getInstance();
    CarrierInfo carrier = carrierDb.getRandom();
    
    carrierName = carrier.name;
    carrierCountry = carrier.country;
    mcc = carrier.mcc;
    mnc = carrier.mnc;
    
    // Generate ICCIDs (20 digits)
    std::string iccidPrefix = "89" + mcc + mnc;
    iccid1 = iccidPrefix + generateNumeric(13);
    iccid2 = iccidPrefix + generateNumeric(13);
    
    // Generate IMSIs (15 digits)
    std::string imsiPrefix = mcc + mnc;
    imsi1 = imsiPrefix + generateNumeric(9);
    imsi2 = imsiPrefix + generateNumeric(9);
    
    isMultiSIM = false;
    simCount = 1;
    simState = "READY";
    networkType = "LTE";
    phoneType = "GSM";
}

std::map<std::string, std::string> SIMConfiguration::toMap() const {
    std::map<std::string, std::string> result = {
        {"iccid", iccid1},
        {"sim.operator.numeric", mcc + mnc},
        {"sim.operator.alpha", carrierName},
        {"sim.country.iso-country", carrierCountry},
        {"gsm.sim.operator.numeric", mcc + mnc},
        {"gsm.sim.operator.alpha", carrierName},
        {"network.type", networkType},
        {"phone.type", phoneType}
    };
    
    if (isMultiSIM) {
        result["iccid_2"] = iccid2;
    }
    
    return result;
}

// ============================================================================
// CPU Information Implementation
// ============================================================================

void CPUInformation::generate(const std::string& deviceClass) {
    architecture = "arm64-v8a";
    cpuAbiList = "arm64-v8a, armeabi-v7a, armeabi";
    cpuAbi2List = "";
    
    if (deviceClass == "High-End" || deviceClass == "Gaming Flagship") {
        hardware = "qcom";
        board = "taro";
        modelName = "Snapdragon 8 Gen 3";
        modelShort = "SM8650";
        coreCount = 8;
        coreCountBig = 1;
        coreCountMid = 5;
        coreCountLittle = 2;
        cpuMaxFreq = 3300000000ULL;
        cpuMinFreq = 300000000ULL;
        bogoMips = 480.0;
    } else if (deviceClass == "High-End Gaming") {
        hardware = "qcom";
        board = "kalama";
        modelName = "Snapdragon 8 Gen 2";
        modelShort = "SM8550";
        coreCount = 8;
        coreCountBig = 1;
        coreCountMid = 4;
        coreCountLittle = 3;
        cpuMaxFreq = 3200000000ULL;
        cpuMinFreq = 300000000ULL;
        bogoMips = 450.0;
    } else if (deviceClass == "Mid-Range" || deviceClass == "Mid-High") {
        hardware = "qcom";
        board = "lahaina";
        modelName = "Snapdragon 7+ Gen 2";
        modelShort = "SM7475";
        coreCount = 8;
        coreCountBig = 1;
        coreCountMid = 3;
        coreCountLittle = 4;
        cpuMaxFreq = 2800000000ULL;
        cpuMinFreq = 300000000ULL;
        bogoMips = 380.0;
    } else {
        hardware = "qcom";
        board = "bengal";
        modelName = "Snapdragon 680";
        modelShort = "SM6225";
        coreCount = 8;
        coreCountBig = 4;
        coreCountMid = 0;
        coreCountLittle = 4;
        cpuMaxFreq = 2400000000ULL;
        cpuMinFreq = 300000000ULL;
        bogoMips = 245.0;
    }
    
    processor = "ARM Implementer 88 -> Qualcomm Technologies, Inc.";
    hasAES = true;
    hasNEON = true;
    hasVFP = true;
    hasARMv8 = true;
}

std::map<std::string, std::string> CPUInformation::toMap() const {
    return {
        {"ro.board.platform", hardware},
        {"ro.arch", architecture},
        {"ro.hardware", hardware},
        {"ro.product.cpu.abilist", cpuAbiList},
        {"ro.product.cpu.abilist64", "arm64-v8a"},
        {"ro.product.cpu.abilist32", "armeabi-v7a, armeabi"},
        {"dalvik.vm.isa.arm64.variant", "cortex-a710"},
        {"dalvik.vm.isa.arm64.features", "default"},
        {"dalvik.vm.isa.arm.variant", "cortex-a710"},
        {"dalvik.vm.isa.arm.features", "default"}
    };
}

// ============================================================================
// GPU Information Implementation
// ============================================================================

void GPUInformation::generate(const std::string& gpuModel) {
    if (gpuModel.find("8 Gen 3") != std::string::npos) {
        renderer = "Adreno (TM) 750";
        vendor = "Qualcomm";
        version = "OpenGL ES 3.2 V@0533.0 (Adreno (TM) 750)";
        vulkanVersion = "1.1.279";
        vulkanConformance = "1.3.2";
        glEsVersion = "3.2";
        glEsRenderer = renderer;
    } else if (gpuModel.find("8 Gen 2") != std::string::npos) {
        renderer = "Adreno (TM) 740";
        vendor = "Qualcomm";
        version = "OpenGL ES 3.2 V@0523.0 (Adreno (TM) 740)";
        vulkanVersion = "1.1.279";
        vulkanConformance = "1.3.2";
        glEsVersion = "3.2";
        glEsRenderer = renderer;
    } else {
        renderer = "Adreno (TM) 619";
        vendor = "Qualcomm";
        version = "OpenGL ES 3.2 V@0302.0 (Adreno (TM) 619)";
        vulkanVersion = "1.1.218";
        vulkanConformance = "1.3.0";
        glEsVersion = "3.2";
        glEsRenderer = renderer;
    }
    
    extensions = {
        "GL_OES_EGL_image",
        "GL_OES_EGL_image_external",
        "GL_OES_EGL_sync",
        "GL_OES_vertex_half_float",
        "GL_OES_framebuffer_object",
        "GL_OES_rgb8_rgba8",
        "GL_OES_depth24",
        "GL_OES_depth32",
        "GL_OES_texture_stencil8",
        "GL_EXT_texture_filter_anisotropic",
        "GL_EXT_multisampled_render_to_texture"
    };
}

std::map<std::string, std::string> GPUInformation::toMap() const {
    return {
        {"ro.hardware.gpu", renderer},
        {"ro.opengles.version", glEsVersion},
        {"ro.opengles.version.gl_es", glEsVersion},
        {"ro.opengles.version.gles_es", glEsVersion},
        {"ro.popenGLESversion", glEsVersion}
    };
}

// ============================================================================
// Memory Information Implementation
// ============================================================================

void MemoryInformation::generate() {
    totalRAMGB = 12;
    totalRAMMB = 12288;
    totalRAM = totalRAMMB * 1024ULL * 1024ULL;
    availableRAM = totalRAM / 2;
    
    lowMemoryThreshold = 1024 * 1024 * 1024; // 1GB
    threshold = lowMemoryThreshold;
    
    isLowRamDevice = false;
    hardwarePageSize = 4096;
    
    largeHeapEnabled = true;
    largeHeapSize = 805306368; // 768MB
    
    if (totalRAMGB >= 12) {
        largeHeapSize = 805306368;  // 768MB
    } else if (totalRAMGB >= 8) {
        largeHeapSize = 536870912; // 512MB
    } else {
        largeHeapSize = 268435456;  // 256MB
    }
}

std::map<std::string, std::string> MemoryInformation::toMap() const {
    std::map<std::string, std::string> result = {
        {"dalvik.vm.heapgrowthlimit", std::to_string(largeHeapSize)},
        {"dalvik.vm.heapmaxsize", std::to_string(largeHeapSize * 2)},
        {"dalvik.vm.heapsize", std::to_string(largeHeapSize * 2)},
        {"dalvik.vm.heapstartsize", "8388608"},
        {"dalvik.vm.lowmemorymode", isLowRamDevice ? "true" : "false"},
        {"ro.config.low_ram", isLowRamDevice ? "true" : "false"}
    };
    return result;
}

// ============================================================================
// Battery Information Implementation
// ============================================================================

void BatteryInformation::generate() {
    status = "good";
    health = "good";
    plugType = "none";
    level = 100;
    temperature = 250; // 25.0°C
    voltage = 4200;
    currentNow = 0;
    currentAvg = 0;
    present = true;
    technology = "Li-ion";
    capacity = 4500;
    batteryScale = 100;
    chargeCounter = 0;
    energyCounter = 0;
}

std::map<std::string, std::string> BatteryInformation::toMap() const {
    return {
        {"ro.battery.capacity", std::to_string(capacity)},
        {"ro.batterytechnology", technology},
        {"persist.battery.capacity", std::to_string(capacity)}
    };
}

// ============================================================================
// Build Information Implementation
// ============================================================================

void BuildInformation::generate(const std::string& manufacturer,
                               const std::string& model,
                               const std::string& codename,
                               const std::string& androidVersion) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Build ID generation
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss;
    oss << "UP1A." << std::uppercase << std::hex << (timestamp & 0xFFFFFFF);
    buildId = oss.str();
    
    // Security patch based on Android version
    if (androidVersion == "16" || androidVersion == "15") {
        buildVersionSecurityPatch = "2025-06-01";
    } else if (androidVersion == "14") {
        buildVersionSecurityPatch = "2024-11-01";
    } else {
        buildVersionSecurityPatch = "2024-06-01";
    }
    
    buildDisplay = buildId;
    buildType = "user";
    buildTags = "release-keys";
    buildVersionRelease = androidVersion;
    buildVersionCodename = "REL";
    buildVersionSDK = (androidVersion == "16") ? "36" :
                      (androidVersion == "15") ? "35" :
                      (androidVersion == "14") ? "34" : "33";
    
    // Build device properties
    buildBrand = manufacturer;
    buildManufacturer = manufacturer;
    buildDevice = codename;
    buildProduct = model;
    buildHardware = codename;
    buildModel = model;
    buildName = model;
    
    // Bootloader
    std::ostringstream blOss;
    blOss << "M" << codename.substr(0, 3) << (1000 + rd() % 9000);
    bootloader = blOss.str();
    
    // Radio/Baseband
    std::ostringstream radioOss;
    radioOss << "MPSS." << (4 + rd() % 3) << "." << (10 + rd() % 40) << "." 
             << (100 + rd() % 400);
    radioVersion = radioOss.str();
    
    // Build time
    buildTime = std::time(nullptr);
    buildDate = getCurrentDateUTC();
    buildDateUTC = buildDate;
    
    // Generate fingerprint
    std::ostringstream fpOss;
    fpOss << manufacturer << "/" << manufacturer << "/" << codename << ":"
          << androidVersion << "/" << buildId << "/" << buildId << ":"
          << buildType << "/" << buildTags;
    fingerprint = fpOss.str();
}

std::map<std::string, std::string> BuildInformation::toMap() const {
    return {
        {"ro.build.id", buildId},
        {"ro.build.display.id", buildDisplay},
        {"ro.build.type", buildType},
        {"ro.build.tags", buildTags},
        {"ro.build.version.release", buildVersionRelease},
        {"ro.build.version.codename", buildVersionCodename},
        {"ro.build.version.sdk", buildVersionSDK},
        {"ro.build.version.security_patch", buildVersionSecurityPatch},
        {"ro.build.fingerprint", fingerprint},
        {"ro.build.date", buildDate},
        {"ro.build.date.utc", std::to_string(buildTime)},
        {"ro.product.brand", buildBrand},
        {"ro.product.manufacturer", buildManufacturer},
        {"ro.product.device", buildDevice},
        {"ro.product.model", buildModel},
        {"ro.product.name", buildProduct},
        {"ro.hardware", buildHardware},
        {"ro.bootloader", bootloader},
        {"ro.radio.version", radioVersion}
    };
}

// ============================================================================
// Verified Boot Information Implementation
// ============================================================================

void VerifiedBootInformation::generate() {
    verifiedBootState = "green";
    verifiedBootLocked = "true";
    verifiedBootHardlocked = "true";
    verificationBootEnabled = "true";
    verificationBoot = "true";
    
    // Generate VBMeta digest (64 hex characters)
    vbmetaDigest = generateHex(64);
    vbmetaVersion = "1.0";
    
    oemLockState = "locked";
    oemUnlockAllowed = "false";
}

std::map<std::string, std::string> VerifiedBootInformation::toMap() const {
    return {
        {"ro.boot.verifiedbootstate", verifiedBootState},
        {"ro.verifiedbootstate", verifiedBootState},
        {"ro.boot.verification.enabled", verificationBootEnabled},
        {"ro.verification.boot", verificationBoot},
        {"ro.boot.locked", verifiedBootLocked},
        {"ro.boot.hardlocked", verifiedBootHardlocked},
        {"ro.oem.lock.state", oemLockState},
        {"ro.oem.unlock.supported", oemUnlockAllowed},
        {"ro.boot.vbmeta.digest", vbmetaDigest}
    };
}

// ============================================================================
// Network Configuration Implementation
// ============================================================================

void NetworkConfiguration::generate(const std::string& deviceName) {
    hostname = deviceName;
    dhcpHostname = deviceName;
    
    // Generate IP address (10.0.x.x range for internal)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(2, 254);
    
    std::ostringstream ipOss;
    ipOss << "10.0." << dis(gen) << "." << dis(gen);
    ipAddress = ipOss.str();
    
    netmask = "255.255.255.0";
    std::ostringstream gwOss;
    gwOss << "10.0." << dis(gen) << ".1";
    gateway = gwOss.str();
    
    dns1 = "8.8.8.8";
    dns2 = "8.8.4.4";
    
    // TCP buffer settings
    tcpRmemMin = "4096";
    tcpRmemDefault = "131072";
    tcpRmemMax = "131072";
    tcpWmemMin = "4096";
    tcpWmemDefault = "131072";
    tcpWmemMax = "131072";
    tcpCongestion = "cubic";
    tcpCongestionDefault = "cubic";
    
    dnsSearchDomains = "";
    dnsServers = {"8.8.8.8", "8.8.4.4"};
    
    networkInterface = "wlan0";
    domain = "";
    domainLookup = "true";
}

std::map<std::string, std::string> NetworkConfiguration::toMap() const {
    return {
        {"net.hostname", hostname},
        {"net.dns1", dns1},
        {"net.dns2", dns2},
        {"net.tcp.rmem", tcpRmemDefault},
        {"net.tcp.wmem", tcpWmemDefault},
        {"net.tcp.congestion", tcpCongestion}
    };
}

// ============================================================================
// GPS Configuration Implementation
// ============================================================================

void GPSConfiguration::generate() {
    // Default location (San Francisco area)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-0.5, 0.5);
    
    latitude = 37.7749 + dis(gen);
    longitude = -122.4194 + dis(gen);
    altitude = 10.0 + dis(gen) * 5;
    accuracy = 3.0 + dis(gen) * 2;
    altitudeAccuracy = accuracy * 0.5;
    speed = 0.0;
    bearing = 0.0;
    
    gpsVersion = "1.0";
    gpsSize = "1040384";
    gpsFlags = "1";
    
    supportsGPS = true;
    supportsGLONASS = true;
    supportsBEIDOU = true;
    supportsGALILEO = true;
    supportsQZSS = true;
    
    gpsAccuracy = "3.0";
    gpsSensitivity = "0.0";
    gpsVendor = "Qualcomm";
    gpsModel = "gps_vendor";
    
    std::ostringstream locOss;
    locOss << std::fixed << std::setprecision(6) << latitude << "," << longitude;
    lastLocation = locOss.str();
    
    locationMode = "high_accuracy";
}

std::map<std::string, std::string> GPSConfiguration::toMap() const {
    return {
        {"ro.location.provider", "gps"},
        {"ro.gps.sensor_used", "0"},
        {"ro.gps.supports_galileo", supportsGALILEO ? "true" : "false"},
        {"ro.gps.supports_beidou", supportsBEIDOU ? "true" : "false"},
        {"ro.gps.supports_glonass", supportsGLONASS ? "true" : "false"}
    };
}

// ============================================================================
// Sensors Configuration Implementation
// ============================================================================

void SensorsConfiguration::generate(const std::string& deviceClass) {
    availableSensors = {
        "Accelerometer Sensor",
        "Gyroscope Sensor",
        "Magnetic Field Sensor",
        "Proximity Sensor",
        "Light Sensor",
        "Pressure Sensor",
        "Step Counter Sensor",
        "Step Detector Sensor",
        "Haptic Feedback Consumer"
    };
    
    // Accelerometer
    accelerometer.name = "STMicro Accelerometer";
    accelerometer.vendor = "STMicroelectronics";
    accelerometer.version = "1";
    accelerometer.type = "1";
    accelerometer.maxRange = deviceClass == "High-End" ? "39.226600" : "19.613300";
    accelerometer.resolution = "0.001200";
    accelerometer.power = "0.130000";
    accelerometer.maxDelay = "200000";
    accelerometer.minDelay = "5000";
    accelerometer.x = 0.0f;
    accelerometer.y = 0.0f;
    accelerometer.z = 9.81f;
    
    // Gyroscope
    gyroscope.name = "STMicro Gyroscope";
    gyroscope.vendor = "STMicroelectronics";
    gyroscope.version = "1";
    gyroscope.type = "4";
    gyroscope.maxRange = "4000.000000";
    gyroscope.resolution = "0.001220";
    gyroscope.power = "0.260000";
    gyroscope.x = 0.0f;
    gyroscope.y = 0.0f;
    gyroscope.z = 0.0f;
    
    // Magnetic Field
    magneticField.name = "STMicro Magnetic Field";
    magneticField.vendor = "STMicroelectronics";
    magneticField.version = "1";
    magneticField.type = "2";
    magneticField.maxRange = "49.999992";
    magneticField.resolution = "0.001500";
    magneticField.power = "0.250000";
    magneticField.x = 25.0f;
    magneticField.y = -15.0f;
    magneticField.z = 45.0f;
    
    // Barometer
    barometer.name = "BMP280 Pressure";
    barometer.vendor = "Bosch";
    barometer.version = "1";
    barometer.type = "6";
    barometer.maxRange = "1100.000000";
    barometer.resolution = "0.001000";
    barometer.power = "0.001400";
    barometer.pressure = 1013.25f;
    
    // Light
    light.name = "STMicro Light Sensor";
    light.vendor = "STMicroelectronics";
    light.version = "1";
    light.type = "5";
    light.maxRange = "4300.000000";
    light.resolution = "1.000000";
    light.power = "0.150000";
    light.lux = 300.0f;
    
    // Proximity
    proximity.name = "STMicro Proximity";
    proximity.vendor = "STMicroelectronics";
    proximity.version = "1";
    proximity.type = "3";
    proximity.maxRange = "5.000000";
    proximity.resolution = "1.000000";
    proximity.power = "0.180000";
    proximity.distance = 0.0f;
    
    // Step Counter
    stepCounter.name = "Step Counter";
    stepCounter.vendor = "STMicroelectronics";
    stepCounter.version = "1";
    stepCounter.type = "19";
    stepCounter.steps = 0;
    stepCounter.stepDetected = false;
    
    // Haptic
    haptic.name = "Haptic Feedback";
    haptic.maxAmplitude = 255;
    haptic.defaultAmplitude = 128;
}

std::map<std::string, std::string> SensorsConfiguration::toMap() const {
    return {
        {"ro.sensor.kind.accelerometer", accelerometer.name},
        {"ro.sensor.kind.gyroscope", gyroscope.name},
        {"ro.sensor.kind.magnetic", magneticField.name},
        {"ro.sensor.kind.proximity", proximity.name},
        {"ro.sensor.kind.light", light.name}
    };
}

// ============================================================================
// Security Configuration Implementation
// ============================================================================

void SecurityConfiguration::generate(const std::string& manufacturer) {
    // SELinux - always enforcing on modern devices
    selinuxStatus = "Enforcing";
    selinuxEnforcing = "Enforcing";
    selinuxMode = "Enforcing";
    
    // Hardware attestation
    attestationEnabled = "true";
    attestationStatus = "true";
    keymasterVersion = "4.1";
    keymasterSecurityLevel = "SOFTWARE";
    
    gatekeeperVersion = "1.0";
    gatekeeperSecurityLevel = "SOFTWARE";
    
    // Strongbox
    hasStrongbox = true;
    strongboxVersion = "1.0";
    strongboxSecurityLevel = "STRONGBOX";
    
    // Trusty
    trustyVersion = "1.0";
    
    // Hardware-backed keys
    hardwareBackedKeys = "true";
    cryptoSupported = "true";
    
    // SafetyNet
    safetyNetEnabled = "true";
    playIntegrityToken = "";
    
    // KNOX (Samsung specific)
    if (manufacturer == "Samsung") {
        knoxVersion = "3.9";
        knoxId = generateHex(32);
        knoxGuardVersion = "2.0";
        odeMode = "true";
        secureStorageEnabled = "true";
        teeVersion = "2.0";
    } else {
        knoxVersion = "";
        knoxId = "";
        knoxGuardVersion = "";
        odeMode = "false";
        secureStorageEnabled = "false";
        teeVersion = "1.0";
    }
    
    // Verified Boot Key
    verifiedBootKeyHash = generateHex(64);
    roBootloader = "unlocked";
}

std::map<std::string, std::string> SecurityConfiguration::toMap() const {
    std::map<std::string, std::string> result = {
        {"ro.build.selinux.enforce", selinuxEnforcing},
        {"ro.build.selinux.status", selinuxStatus},
        {"ro.security.vndmk", keymasterVersion},
        {"ro.hardware.keystore", keymasterVersion},
        {"ro.hardware.gatekeeper", gatekeeperVersion},
        {"ro.crypto.state", "unencrypted"},
        {"ro.crypto.tima.proprietary", "false"},
        {"ro.crypto.hw_decrypt", cryptoSupported},
        {"ro.hardware.backed_keys", hardwareBackedKeys},
        {"ro.attestation.enabled", attestationEnabled}
    };
    
    if (!knoxVersion.empty()) {
        result["ro.config.knox"] = knoxVersion;
        result["ro Knox.version"] = knoxVersion;
    }
    
    return result;
}

// ============================================================================
// Android Version Implementation
// ============================================================================

AndroidVersion AndroidVersion::getLatest() {
    AndroidVersion v;
    v.codename = "VanillaIceCream";
    v.versionNumber = "16";
    v.apiLevel = "36";
    v.releaseDate = "2025-03";
    v.securityPatch = "2025-06-01";
    v.buildType = "user";
    v.buildTags = "release-keys";
    return v;
}

AndroidVersion AndroidVersion::fromString(const std::string& version) {
    AndroidVersion v;
    
    if (version == "16" || version == "16.0" || version.find("16") == 0) {
        v.codename = "VanillaIceCream";
        v.versionNumber = "16";
        v.apiLevel = "36";
        v.securityPatch = "2025-06-01";
    } else if (version == "15" || version == "15.0" || version.find("15") == 0) {
        v.codename = "UpsideDownCake";
        v.versionNumber = "15";
        v.apiLevel = "35";
        v.securityPatch = "2024-11-01";
    } else if (version == "14" || version == "14.0" || version.find("14") == 0) {
        v.codename = "UpsideDownCake";
        v.versionNumber = "14";
        v.apiLevel = "34";
        v.securityPatch = "2024-06-01";
    } else {
        v.codename = "Tiramisu";
        v.versionNumber = "13";
        v.apiLevel = "33";
        v.securityPatch = "2024-02-01";
    }
    
    v.releaseDate = "2023-08";
    v.buildType = "user";
    v.buildTags = "release-keys";
    
    return v;
}

// ============================================================================
// Display Configuration Implementation
// ============================================================================

void DisplayConfiguration::generate(const std::string& deviceClass) {
    if (deviceClass == "High-End" || deviceClass == "Gaming Flagship") {
        widthPixels = 1440;
        heightPixels = 3120;
        densityDPI = 560;
        densityValue = 3.5f;
        fps = 120;
        vsync = 120;
    } else if (deviceClass == "Premium" || deviceClass == "High-End Compact") {
        widthPixels = 1080;
        heightPixels = 2520;
        densityDPI = 420;
        densityValue = 2.625f;
        fps = 90;
        vsync = 90;
    } else if (deviceClass == "Gaming") {
        widthPixels = 1080;
        heightPixels = 2400;
        densityDPI = 400;
        densityValue = 2.5f;
        fps = 144;
        vsync = 144;
    } else if (deviceClass == "Mid-Range" || deviceClass == "Mid-High") {
        widthPixels = 1080;
        heightPixels = 2400;
        densityDPI = 400;
        densityValue = 2.5f;
        fps = 90;
        vsync = 90;
    } else {
        widthPixels = 720;
        heightPixels = 1600;
        densityDPI = 320;
        densityValue = 2.0f;
        fps = 60;
        vsync = 60;
    }
    
    hdrCapabilities = "HDR10, HDR10+, DolbyVision, HLG";
    wideColorGamut = "sRGB, Display P3, Adobe RGB";
    
    // Calculate
    if (densityDPI <= 120) densityBucket = 120;
    else if (densityDPI <= 160) densityBucket = 160;
    else if (densityDPI <= 240) densityBucket = 240;
    else if (densityDPI <= 320) densityBucket = 320;
    else if (densityDPI <= 480) densityBucket = 480;
    else densityBucket = 640;
    
    smallestWidth = std::min(widthPixels, heightPixels) / (densityDPI / 160);
}

std::map<std::string, std::string> DisplayConfiguration::toMap() const {
    return {
        {"ro.sf.lcd_density", std::to_string(densityDPI)},
        {"ro.sf.lcd_density_bucket", std::to_string(densityBucket)},
        {"ro.sf.primary_lcd_density", std::to_string(densityDPI)}
    };
}

// ============================================================================
// DeviceProfile Implementation
// ============================================================================

DeviceProfile::DeviceProfile() {
    initialize();
}

DeviceProfile::DeviceProfile(const std::string& manufacturer) {
    initialize();
    generate(manufacturer);
}

DeviceProfile::DeviceProfile(const DeviceProfile& other)
    : identity(other.identity)
    , macAddresses(other.macAddresses)
    , sim(other.sim)
    , cpu(other.cpu)
    , gpu(other.gpu)
    , memory(other.memory)
    , battery(other.battery)
    , display(other.display)
    , build(other.build)
    , verifiedBoot(other.verifiedBoot)
    , network(other.network)
    , gps(other.gps)
    , sensors(other.sensors)
    , security(other.security)
    , androidVersion(other.androidVersion) {
    profileId = other.profileId;
    profileName = other.profileName;
    createdAt = other.createdAt;
    modifiedAt = other.modifiedAt;
    manufacturer = other.manufacturer;
    model = other.model;
    codename = other.codename;
    deviceClass = other.deviceClass;
    tac = other.tac;
    tacModel = other.tacModel;
    tacInternalName = other.tacInternalName;
}

DeviceProfile& DeviceProfile::operator=(const DeviceProfile& other) {
    if (this != &other) {
        identity = other.identity;
        macAddresses = other.macAddresses;
        sim = other.sim;
        cpu = other.cpu;
        gpu = other.gpu;
        memory = other.memory;
        battery = other.battery;
        display = other.display;
        build = other.build;
        verifiedBoot = other.verifiedBoot;
        network = other.network;
        gps = other.gps;
        sensors = other.sensors;
        security = other.security;
        androidVersion = other.androidVersion;
        
        profileId = other.profileId;
        profileName = other.profileName;
        createdAt = other.createdAt;
        modifiedAt = other.modifiedAt;
        manufacturer = other.manufacturer;
        model = other.model;
        codename = other.codename;
        deviceClass = other.deviceClass;
        tac = other.tac;
        tacModel = other.tacModel;
        tacInternalName = other.tacInternalName;
    }
    return *this;
}

void DeviceProfile::initialize() {
    profileId = generateUniqueId();
    profileName = "";
    createdAt = getCurrentTimestamp();
    modifiedAt = createdAt;
    manufacturer = "";
    model = "";
    codename = "";
    deviceClass = "High-End";
    tac = "";
    tacModel = "";
    tacInternalName = "";
}

std::string DeviceProfile::generateUniqueId() {
    return "dev_" + generateUUID().substr(0, 8);
}

std::string DeviceProfile::getCurrentTimestamp() {
    return ::RedroidCPP::getCurrentTimestamp();
}

void DeviceProfile::generate(const std::string& mfg, const std::string& modelName, 
                            const std::string& androidVer) {
    manufacturer = mfg;
    
    // Get TAC entry
    auto& tacDb = TACDatabase::getInstance();
    auto tacEntryOpt = tacDb.getRandomForManufacturer(mfg);
    
    TACEntry tacEntry;
    if (tacEntryOpt) {
        tacEntry = *tacEntryOpt;
    } else {
        tacEntry.tac = "35875109";
        tacEntry.brand = "Samsung";
        tacEntry.modelName = "Galaxy S24";
        tacEntry.internalName = "dm3q";
        tacEntry.deviceType = "Smartphone";
        tacEntry.deviceClass = "High-End";
        tacEntry.launchYear = "2024";
        tacEntry.launchMonth = "01";
    }
    
    tac = tacEntry.tac;
    tacModel = tacEntry.modelName;
    tacInternalName = tacEntry.internalName;
    deviceClass = tacEntry.deviceClass;
    
    if (modelName.empty()) {
        model = tacEntry.modelName;
    } else {
        model = modelName;
    }
    codename = tacEntry.internalName;
    
    profileName = manufacturer + " " + model;
    modifiedAt = getCurrentTimestamp();
    
    // Android Version
    if (androidVer.empty()) {
        androidVersion = AndroidVersion::getLatest();
    } else {
        androidVersion = AndroidVersion::fromString(androidVer);
    }
    
    // Generate all components
    identity.generate(tac, manufacturer);
    macAddresses.generate(manufacturer);
    sim.generate();
    cpu.generate(deviceClass);
    gpu.generate(cpu.modelName);
    memory.generate();
    battery.generate();
    display.generate(deviceClass);
    build.generate(manufacturer, model, codename, androidVersion.versionNumber);
    verifiedBoot.generate();
    network.generate(profileName);
    gps.generate();
    sensors.generate(deviceClass);
    security.generate(manufacturer);
}

std::string DeviceProfile::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"profile_id\": \"" << profileId << "\",\n";
    oss << "  \"profile_name\": \"" << profileName << "\",\n";
    oss << "  \"manufacturer\": \"" << manufacturer << "\",\n";
    oss << "  \"model\": \"" << model << "\",\n";
    oss << "  \"android_version\": \"" << androidVersion.versionNumber << "\",\n";
    oss << "  \"identity\": {\n";
    oss << "    \"imei\": \"" << identity.imei << "\",\n";
    oss << "    \"imei2\": \"" << identity.imei2 << "\",\n";
    oss << "    \"serial\": \"" << identity.serialNumber << "\",\n";
    oss << "    \"android_id\": \"" << identity.androidId << "\",\n";
    oss << "    \"gsf_id\": \"" << identity.gsfId << "\",\n";
    oss << "    \"advertising_id\": \"" << identity.advertisingId << "\"\n";
    oss << "  },\n";
    oss << "  \"network\": {\n";
    oss << "    \"wifi_mac\": \"" << macAddresses.wifiMac << "\",\n";
    oss << "    \"bluetooth_mac\": \"" << macAddresses.bluetoothMac << "\"\n";
    oss << "  },\n";
    oss << "  \"build\": {\n";
    oss << "    \"fingerprint\": \"" << build.fingerprint << "\"\n";
    oss << "  }\n";
    oss << "}";
    return oss.str();
}

bool DeviceProfile::fromJSON(const std::string& json) {
    // Simplified - in production use JSON library
    return !json.empty();
}

bool DeviceProfile::save(const std::string& filepath) const {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        file << toJSON();
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool DeviceProfile::load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return fromJSON(buffer.str());
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::string> DeviceProfile::getAllProperties() const {
    std::map<std::string, std::string> props;
    
    auto identityMap = identity.toMap();
    props.insert(identityMap.begin(), identityMap.end());
    
    auto macMap = macAddresses.toMap();
    props.insert(macMap.begin(), macMap.end());
    
    auto simMap = sim.toMap();
    props.insert(simMap.begin(), simMap.end());
    
    auto cpuMap = cpu.toMap();
    props.insert(cpuMap.begin(), cpuMap.end());
    
    auto gpuMap = gpu.toMap();
    props.insert(gpuMap.begin(), gpuMap.end());
    
    auto memoryMap = memory.toMap();
    props.insert(memoryMap.begin(), memoryMap.end());
    
    auto batteryMap = battery.toMap();
    props.insert(batteryMap.begin(), batteryMap.end());
    
    auto displayMap = display.toMap();
    props.insert(displayMap.begin(), displayMap.end());
    
    auto buildMap = build.toMap();
    props.insert(buildMap.begin(), buildMap.end());
    
    auto verifiedBootMap = verifiedBoot.toMap();
    props.insert(verifiedBootMap.begin(), verifiedBootMap.end());
    
    auto networkMap = network.toMap();
    props.insert(networkMap.begin(), networkMap.end());
    
    auto gpsMap = gps.toMap();
    props.insert(gpsMap.begin(), gpsMap.end());
    
    auto sensorsMap = sensors.toMap();
    props.insert(sensorsMap.begin(), sensorsMap.end());
    
    auto securityMap = security.toMap();
    props.insert(securityMap.begin(), securityMap.end());
    
    return props;
}

std::map<std::string, std::map<std::string, std::string>> 
DeviceProfile::getPropertiesByCategory() const {
    std::map<std::string, std::map<std::string, std::string>> categories;
    
    categories["identity"] = identity.toMap();
    categories["mac_addresses"] = macAddresses.toMap();
    categories["sim"] = sim.toMap();
    categories["cpu"] = cpu.toMap();
    categories["gpu"] = gpu.toMap();
    categories["memory"] = memory.toMap();
    categories["battery"] = battery.toMap();
    categories["display"] = display.toMap();
    categories["build"] = build.toMap();
    categories["verified_boot"] = verifiedBoot.toMap();
    categories["network"] = network.toMap();
    categories["gps"] = gps.toMap();
    categories["sensors"] = sensors.toMap();
    categories["security"] = security.toMap();
    
    return categories;
}

void DeviceProfile::print() const {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                         DEVICE PROFILE - " << profileName << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════\n";
    
    // Basic Info
    std::cout << "\n" << "[ BASIC INFO ]\n";
    std::cout << "  ID:           " << profileId << "\n";
    std::cout << "  Manufacturer:  " << manufacturer << "\n";
    std::cout << "  Model:       " << model << "\n";
    std::cout << "  Codename:     " << codename << "\n";
    std::cout << "  Android:      " << androidVersion.codename << " (" << androidVersion.versionNumber << ")\n";
    std::cout << "  Class:        " << deviceClass << "\n";
    
    // Device Identity
    std::cout << "\n" << "[ DEVICE IDENTITY ]\n";
    std::cout << "  IMEI:         " << identity.imei << "\n";
    std::cout << "  IMEI2:        " << identity.imei2 << "\n";
    std::cout << "  Serial:       " << identity.serialNumber << "\n";
    std::cout << "  Android ID:   " << identity.androidId << "\n";
    std::cout << "  GSF ID:      " << identity.gsfId << "\n";
    std::cout << "  AAID:        " << identity.advertisingId << "\n";
    
    // MAC Addresses
    std::cout << "\n" << "[ MAC ADDRESSES ]\n";
    std::cout << "  WiFi:         " << macAddresses.wifiMac << "\n";
    std::cout << "  Bluetooth:    " << macAddresses.bluetoothMac << "\n";
    std::cout << "  Ethernet:    " << macAddresses.ethernetMac << "\n";
    
    // SIM
    std::cout << "\n" << "[ SIM ]\n";
    std::cout << "  ICCID:        " << sim.iccid1 << "\n";
    std::cout << "  IMSI:        " << sim.imsi1 << "\n";
    std::cout << "  Carrier:     " << sim.carrierName << " (" << sim.carrierCountry << ")\n";
    std::cout << "  MCC/MNC:     " << sim.mcc << "/" << sim.mnc << "\n";
    
    // Display
    std::cout << "\n" << "[ DISPLAY ]\n";
    std::cout << "  Resolution:  " << display.widthPixels << "x" << display.heightPixels << "\n";
    std::cout << "  DPI:         " << display.densityDPI << "\n";
    std::cout << "  FPS:         " << display.fps << "\n";
    
    // Build
    std::cout << "\n" << "[ BUILD ]\n";
    std::cout << "  Fingerprint:  " << build.fingerprint << "\n";
    std::cout << "  Bootloader:   " << build.bootloader << "\n";
    std::cout << "  Build ID:     " << build.buildId << "\n";
    std::cout << "  Security Patch: " << build.buildVersionSecurityPatch << "\n";
    
    // Verified Boot
    std::cout << "\n" << "[ VERIFIED BOOT ]\n";
    std::cout << "  State:        " << verifiedBoot.verifiedBootState << "\n";
    std::cout << "  Locked:       " << verifiedBoot.verifiedBootLocked << "\n";
    std::cout << "  VBMeta Digest: " << verifiedBoot.vbmetaDigest.substr(0, 32) << "...\n";
    
    // Security
    std::cout << "\n" << "[ SECURITY ]\n";
    std::cout << "  SELinux:      " << security.selinuxEnforcing << "\n";
    std::cout << "  Keymaster:    " << security.keymasterVersion << "\n";
    std::cout << "  Strongbox:    " << (security.hasStrongbox ? "Yes" : "No") << "\n";
    if (!security.knoxVersion.empty()) {
        std::cout << "  KNOX:        " << security.knoxVersion << "\n";
    }
    
    // GPS
    std::cout << "\n" << "[ GPS ]\n";
    std::cout << "  Latitude:     " << std::fixed << std::setprecision(6) << gps.latitude << "\n";
    std::cout << "  Longitude:    " << gps.longitude << "\n";
    std::cout << "  Altitude:     " << gps.altitude << "m\n";
    std::cout << "  Accuracy:     " << gps.accuracy << "m\n";
    
    // Sensors
    std::cout << "\n" << "[ SENSORS ]\n";
    std::cout << "  Accelerometer: " << sensors.accelerometer.name << "\n";
    std::cout << "  Gyroscope:     " << sensors.gyroscope.name << "\n";
    std::cout << "  Light:         " << sensors.light.name << "\n";
    std::cout << "  Proximity:     " << sensors.proximity.name << "\n";
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════════\n";
}

std::string DeviceProfile::summary() const {
    std::ostringstream oss;
    oss << manufacturer << " " << model 
        << " | IMEI: " << identity.imei.substr(0, 8) << "****"
        << " | Android " << androidVersion.versionNumber;
    return oss.str();
}

bool DeviceProfile::validate() const {
    return identity.validate();
}

} // namespace RedroidCPP
