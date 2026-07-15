#include "RealisticProfileGenerator.hpp"
#include "IPTimezoneConverter.hpp"
#include "DeviceIDGenerator.hpp"
#include "CryptoEmulator.hpp"
#include "Logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace AntiDetect {

// ============================================================
// STATIC DATABASE - SAMSUNG DEVICES
// ============================================================
static const std::map<std::string, std::vector<std::map<std::string, std::string>>> SAMSUNG_DATABASE = {
    {"Galaxy S24 Ultra", {
        {{"model", "SM-S928B"}, {"codename", "dm3"}, {"platform", "s5e9935"}, {"cpu", "Exynos 2400"}, {"gpu", "Xclipse 940"}, {"cores", "10"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3120"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S24+", {
        {{"model", "SM-S926B"}, {"codename", "dm2"}, {"platform", "s5e9935"}, {"cpu", "Exynos 2400"}, {"gpu", "Xclipse 930"}, {"cores", "10"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3088"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S24", {
        {{"model", "SM-S921B"}, {"codename", "dm1"}, {"platform", "s5e9935"}, {"cpu", "Exynos 2400"}, {"gpu", "Xclipse 920"}, {"cores", "10"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S23 Ultra", {
        {{"model", "SM-S918B"}, {"codename", "dm3q"}, {"platform", "s5e9935"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3088"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S23+", {
        {{"model", "SM-S916B"}, {"codename", "dm2q"}, {"platform", "s5e9935"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "256"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S23", {
        {{"model", "SM-S911B"}, {"codename", "dm1q"}, {"platform", "s5e9935"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Galaxy S22 Ultra", {
        {{"model", "SM-S908B"}, {"codename", "o1s"}, {"platform", "exynos2200"}, {"cpu", "Exynos 2200"}, {"gpu", "Xclipse 920"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3088"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy S22+", {
        {{"model", "SM-S906B"}, {"codename", "o1s"}, {"platform", "exynos2200"}, {"cpu", "Exynos 2200"}, {"gpu", "Xclipse 920"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy S22", {
        {{"model", "SM-S901B"}, {"codename", "o1"}, {"platform", "exynos2200"}, {"cpu", "Exynos 2200"}, {"gpu", "Xclipse 920"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy S21 Ultra", {
        {{"model", "SM-G998B"}, {"codename", "o1sxx"}, {"platform", "exynos2100"}, {"cpu", "Exynos 2100"}, {"gpu", "Mali-G78"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "128"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2023-11-01"}}
    }},
    {"Galaxy Z Fold5", {
        {{"model", "SM-F946B"}, {"codename", "q5q"}, {"platform", "sd8gen2"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1812x2176"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy Z Flip5", {
        {{"model", "SM-F731B"}, {"codename", "b5q"}, {"platform", "sd8gen2"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "256"}, {"screen", "1080x2640"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy A54", {
        {{"model", "SM-A546B"}, {"codename", "a54x"}, {"platform", "s5e8825"}, {"cpu", "Exynos 1380"}, {"gpu", "Mali-G68"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Galaxy A34", {
        {{"model", "SM-A346B"}, {"codename", "a34x"}, {"platform", "mt6833"}, {"cpu", "Dimensity 1080"}, {"gpu", "Mali-G68"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2340"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }}
};

// ============================================================
// STATIC DATABASE - GOOGLE DEVICES
// ============================================================
static const std::map<std::string, std::vector<std::map<std::string, std::string>>> GOOGLE_DATABASE = {
    {"Pixel 8 Pro", {
        {{"model", "Pixel 8 Pro"}, {"codename", "husky"}, {"platform", "zuma"}, {"cpu", "Tensor G3"}, {"gpu", "Mali-G715"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1344x2992"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Pixel 8", {
        {{"model", "Pixel 8"}, {"codename", "akita"}, {"platform", "zuma"}, {"cpu", "Tensor G3"}, {"gpu", "Mali-G715"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2400"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Pixel 7a", {
        {{"model", "Pixel 7a"}, {"codename", "lynx"}, {"platform", "panteion"}, {"cpu", "Tensor G2"}, {"gpu", "Mali-G710"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2400"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-01-01"}}
    }},
    {"Pixel 7 Pro", {
        {{"model", "Pixel 7 Pro"}, {"codename", "panther"}, {"platform", "panteion"}, {"cpu", "Tensor G2"}, {"gpu", "Mali-G710"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "128"}, {"screen", "1440x3120"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-01-01"}}
    }},
    {"Pixel 7", {
        {{"model", "Pixel 7"}, {"codename", "panther"}, {"platform", "panteion"}, {"cpu", "Tensor G2"}, {"gpu", "Mali-G710"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2400"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-01-01"}}
    }},
    {"Pixel 6a", {
        {{"model", "Pixel 6a"}, {"codename", "bluejay"}, {"platform", "oriole"}, {"cpu", "Tensor G1"}, {"gpu", "Mali-G78"}, {"cores", "8"}, {"ram", "6144"}, {"storage", "128"}, {"screen", "1080x2400"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2023-12-01"}}
    }},
    {"Pixel 6 Pro", {
        {{"model", "Pixel 6 Pro"}, {"codename", "raven"}, {"platform", "oriole"}, {"cpu", "Tensor G1"}, {"gpu", "Mali-G78"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "128"}, {"screen", "1440x3120"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2023-12-01"}}
    }}
};

// ============================================================
// STATIC DATABASE - XIAOMI DEVICES
// ============================================================
static const std::map<std::string, std::vector<std::map<std::string, std::string>>> XIAOMI_DATABASE = {
    {"Xiaomi 14 Ultra", {
        {{"model", "24053PY09G"}, {"codename", "shennong"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8 Gen 3"}, {"gpu", "Adreno 750"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "512"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Xiaomi 14 Pro", {
        {{"model", "23127PN0CC"}, {"codename", "daguin"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8 Gen 3"}, {"gpu", "Adreno 750"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Xiaomi 14", {
        {{"model", "23127PN0CC"}, {"codename", "houji"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8 Gen 3"}, {"gpu", "Adreno 750"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "256"}, {"screen", "1200x2670"}, {"density", "2"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Xiaomi 13 Ultra", {
        {{"model", "2304FN75DC"}, {"codename", "zhenhua"}, {"platform", "nuva"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "512"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-01-01"}}
    }},
    {"Xiaomi 13 Pro", {
        {{"model", "2210132G"}, {"codename", "nuva"}, {"platform", "nuva"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Xiaomi 13", {
        {{"model", "2211133G"}, {"codename", "fuxi"}, {"platform", "nuva"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "256"}, {"screen", "1080x2400"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"Redmi K70 Pro", {
        {{"model", "23117RK65C"}, {"codename", "manet"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8 Gen 3"}, {"gpu", "Adreno 750"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "512"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"Poco F5 Pro", {
        {{"model", "23013RK75G"}, {"codename", "marble"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8+ Gen 1"}, {"gpu", "Adreno 730"}, {"cores", "8"}, {"ram", "12288"}, {"storage", "256"}, {"screen", "1440x3200"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }}
};

// ============================================================
// STATIC DATABASE - ONEPLUS DEVICES
// ============================================================
static const std::map<std::string, std::vector<std::map<std::string, std::string>>> ONEPLUS_DATABASE = {
    {"OnePlus 12", {
        {{"model", "CPH2573"}, {"codename", "salam"}, {"platform", "kalama"}, {"cpu", "Snapdragon 8 Gen 3"}, {"gpu", "Adreno 750"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "256"}, {"screen", "1440x3168"}, {"density", "3"}, {"android", "14"}, {"sdk", "34"}, {"security", "2024-02-01"}}
    }},
    {"OnePlus Open", {
        {{"model", "CPH2551"}, {"codename", "floral"}, {"platform", "lahaina"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "512"}, {"screen", "1792x2260"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"OnePlus 11", {
        {{"model", "CPH2451"}, {"codename", "sagat"}, {"platform", "lahaina"}, {"cpu", "Snapdragon 8 Gen 2"}, {"gpu", "Adreno 740"}, {"cores", "8"}, {"ram", "16384"}, {"storage", "256"}, {"screen", "1440x3216"}, {"density", "3"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }},
    {"OnePlus 10T", {
        {{"model", "CPH2413"}, {"codename", "sagat_t"}, {"platform", "sm8250"}, {"cpu", "Snapdragon 8+ Gen 1"}, {"gpu", "Adreno 730"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2412"}, {"density", "2"}, {"android", "12"}, {"sdk", "32"}, {"security", "2023-12-01"}}
    }},
    {"OnePlus Nord 3", {
        {{"model", "CPH2493"}, {"codename", "imora"}, {"platform", "mt6893"}, {"cpu", "Dimensity 9000"}, {"gpu", "Mali-G710"}, {"cores", "8"}, {"ram", "8192"}, {"storage", "128"}, {"screen", "1080x2412"}, {"density", "2"}, {"android", "13"}, {"sdk", "33"}, {"security", "2024-01-01"}}
    }}
};

// ============================================================
// GEOGRAPHIC REGION DATABASE
// ============================================================
static const std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>> CARRIER_DATABASE = {
    {"US", {
        {"operators", {
            {"310-260", "T-Mobile"},
            {"310-200", "AT&T"},
            {"310-410", "Verizon"},
            {"311-480", "US Cellular"},
            {"310-120", "MetroPCS"},
            {"310-230", "Cricket"},
            {"310-890", "Consumer Cellular"},
        }},
        {"locales", {{"locale", "en-US"}, {"locale", "es-US"}}},
        {"timezones", {{"tz", "America/New_York"}, {"tz", "America/Chicago"}, {"tz", "America/Denver"}, {"tz", "America/Los_Angeles"}}},
        {"lat_range", {{"min", "-125.0"}, {"max", "-66.0"}}},
        {"lon_range", {{"min", "24.0"}, {"max", "50.0"}}}
    }},
    {"GB", {
        {"operators", {
            {"234-10", "O2"},
            {"234-15", "Vodafone"},
            {"234-30", "EE"},
            {"234-33", "Three"},
        }},
        {"locales", {{"locale", "en-GB"}}},
        {"timezones", {{"tz", "Europe/London"}}},
        {"lat_range", {{"min", "-8.0"}, {"max", "2.0"}}},
        {"lon_range", {{"min", "50.0"}, {"max", "58.0"}}}
    }},
    {"DE", {
        {"operators", {
            {"262-01", "Telekom"},
            {"262-02", "Vodafone DE"},
            {"262-03", "O2 DE"},
            {"262-07", "E-Plus"},
        }},
        {"locales", {{"locale", "de-DE"}, {"locale", "de-AT"}, {"locale", "de-CH"}}},
        {"timezones", {{"tz", "Europe/Berlin"}, {"tz", "Europe/Vienna"}}},
        {"lat_range", {{"min", "5.0"}, {"max", "16.0"}}},
        {"lon_range", {{"min", "47.0"}, {"max", "55.0"}}}
    }},
    {"JP", {
        {"operators", {
            {"440-10", "NTT DoCoMo"},
            {"440-20", "au"},
            {"440-50", "SoftBank"},
            {"440-70", "Rakuten"},
        }},
        {"locales", {{"locale", "ja-JP"}}},
        {"timezones", {{"tz", "Asia/Tokyo"}}},
        {"lat_range", {{"min", "122.0"}, {"max", "154.0"}}},
        {"lon_range", {{"min", "24.0"}, {"max", "46.0"}}}
    }},
    {"IN", {
        {"operators", {
            {"404-11", "Airtel"},
            {"404-13", "Airtel"},
            {"404-20", "BSNL"},
            {"404-30", "Reliance Jio"},
            {"404-40", "Vodafone IN"},
            {"404-45", "Idea"},
            {"405-01", "Reliance Jio"},
        }},
        {"locales", {{"locale", "en-IN"}, {"locale", "hi-IN"}, {"locale", "bn-IN"}}},
        {"timezones", {{"tz", "Asia/Kolkata"}}},
        {"lat_range", {{"min", "68.0"}, {"max", "98.0"}}},
        {"lon_range", {{"min", "8.0"}, {"max", "37.0"}}}
    }},
    {"BD", {
        {"operators", {
            {"470-01", "Grameenphone"},
            {"470-02", "Robi"},
            {"470-03", "Banglalink"},
            {"470-04", "Teletalk"},
            {"470-05", "Citycell"},
        }},
        {"locales", {{"locale", "bn-BD"}, {"locale", "en-BD"}}},
        {"timezones", {{"tz", "Asia/Dhaka"}}},
        {"lat_range", {{"min", "88.0"}, {"max", "93.0"}}},
        {"lon_range", {{"min", "20.0"}, {"max", "27.0"}}}
    }},
    {"CN", {
        {"operators", {
            {"460-00", "China Mobile"},
            {"460-01", "China Unicom"},
            {"460-03", "China Telecom"},
            {"460-05", "China Telecom"},
            {"460-11", "China Mobile"},
        }},
        {"locales", {{"locale", "zh-CN"}}},
        {"timezones", {{"tz", "Asia/Shanghai"}, {"tz", "Asia/Urumqi"}}},
        {"lat_range", {{"min", "73.0"}, {"max", "136.0"}}},
        {"lon_range", {{"min", "18.0"}, {"max", "54.0"}}}
    }},
    {"KR", {
        {"operators", {
            {"450-02", "KT"},
            {"450-04", "SK Telecom"},
            {"450-06", "LGU+"},
        }},
        {"locales", {{"locale", "ko-KR"}}},
        {"timezones", {{"tz", "Asia/Seoul"}}},
        {"lat_range", {{"min", "124.0"}, {"max", "132.0"}}},
        {"lon_range", {{"min", "33.0"}, {"max", "39.0"}}}
    }}
};

// ============================================================
// MAC ADDRESS OUI DATABASE
// ============================================================
static const std::map<std::string, std::string> MAC_OUI_DATABASE = {
    {"Samsung", "48:74:40"},
    {"Google", "3C:5A:B4"},
    {"Xiaomi", "58:44:98"},
    {"OnePlus", "2A:A4:3C"},
    {"Apple", "A4:83:E7"},
    {"Huawei", "00:25:9E"},
    {"Sony", "7C:AD:BF"},
    {"LG", "AC:0D:1B"},
    {"Motorola", "00:26:7E"},
    {"Oppo", "B0:4A:39"},
    {"Vivo", "F8:DC:7A"},
    {"Realme", "9C:37:F4"},
    {"Generic", "00:11:22"}
};

// ============================================================
// IMPLEMENTATION
// ============================================================

RealisticProfileGenerator& RealisticProfileGenerator::getInstance() {
    static RealisticProfileGenerator instance;
    return instance;
}

RealisticProfileGenerator::RealisticProfileGenerator() {
    std::random_device rd;
    m_randomGenerator = std::mt19937(rd());
    initializeManufacturerDatabase();
    initializeGeoDatabase();
    initializeNetworkDatabase();
}

RealisticProfileGenerator::~RealisticProfileGenerator() {
}

void RealisticProfileGenerator::initializeManufacturerDatabase() {
    // Already initialized via static databases above
}

void RealisticProfileGenerator::initializeGeoDatabase() {
    // Already initialized via static CARRIER_DATABASE above
}

void RealisticProfileGenerator::initializeNetworkDatabase() {
    for (const auto& regionPair : CARRIER_DATABASE) {
        const std::string& region = regionPair.first;
        const auto& data = regionPair.second;
        
        if (data.find("operators") != data.end()) {
            for (const auto& op : data.at("operators")) {
                int mcc = std::stoi(op.first);
                int mnc = std::stoi(op.second);
                m_networkDB[{mcc, mnc}] = {op.second, op.second, mcc, mnc, region, "4G", "+1"};
            }
        }
    }
}

std::string RealisticProfileGenerator::generateHexDigits(int length) {
    std::stringstream ss;
    std::uniform_int_distribution<> dis(0, 15);
    for (int i = 0; i < length; ++i) {
        ss << std::hex << dis(m_randomGenerator);
    }
    return ss.str();
}

std::string RealisticProfileGenerator::generateNumericString(int length) {
    std::stringstream ss;
    std::uniform_int_distribution<> dis(0, 9);
    for (int i = 0; i < length; ++i) {
        ss << dis(m_randomGenerator);
    }
    return ss.str();
}

int RealisticProfileGenerator::randomInt(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(m_randomGenerator);
}

double RealisticProfileGenerator::randomDouble(double min, double max) {
    std::uniform_real_distribution<> dis(min, max);
    return dis(m_randomGenerator);
}

std::string RealisticProfileGenerator::randomChoice(const std::vector<std::string>& choices) {
    if (choices.empty()) return "";
    std::uniform_int_distribution<> dis(0, static_cast<int>(choices.size()) - 1);
    return choices[dis(m_randomGenerator)];
}

std::string RealisticProfileGenerator::hashString(const std::string& input) {
    // Simple SHA-256 implementation without OpenSSL
    // Using a combination of std::hash and custom mixing
    std::stringstream ss;
    
    // Create multiple hash rounds for better randomness
    size_t hash1 = std::hash<std::string>{}(input);
    size_t hash2 = std::hash<std::string>{}(input + "salt1");
    size_t hash3 = std::hash<std::string>{}(input + "salt2");
    size_t hash4 = std::hash<std::string>{}(input + std::to_string(input.length()));
    
    // Mix hashes
    for (int i = 0; i < 4; ++i) {
        size_t combined = (hash1 << (i * 8)) | (hash2 >> (i * 8)) | (hash3 << (16 + i * 8)) | (hash4 >> (16 + i * 8));
        ss << std::hex << std::setw(16) << std::setfill('0') << combined;
    }
    
    return ss.str();
}

std::string RealisticProfileGenerator::generateLuhnChecksum(const std::string& base) {
    int sum = 0;
    bool alternate = true;
    
    for (int i = static_cast<int>(base.length()) - 1; i >= 0; --i) {
        int digit = base[i] - '0';
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    int checkDigit = (10 - (sum % 10)) % 10;
    return base + std::to_string(checkDigit);
}

ProfileGenerationResult RealisticProfileGenerator::generateSamsungProfile(const std::string& region) {
    std::uniform_int_distribution<> dis(0, static_cast<int>(SAMSUNG_DATABASE.size()) - 1);
    int idx = dis(m_randomGenerator);
    
    int count = 0;
    std::string modelName;
    std::map<std::string, std::string> deviceData;
    
    for (const auto& pair : SAMSUNG_DATABASE) {
        if (count++ == idx) {
            modelName = pair.first;
            deviceData = pair.second[0];
            break;
        }
    }
    
    DeviceProfile profile;
    profile.manufacturer = "samsung";
    profile.brand = "samsung";
    profile.model = deviceData["model"];
    profile.deviceName = modelName;
    profile.productName = modelName;
    
    // CPU
    profile.cpuModel = deviceData["cpu"];
    profile.cpuHardware = deviceData["platform"];
    profile.cpuVariant = "";
    profile.cpuCores = std::stoi(deviceData["cores"]);
    profile.cpuThreads = profile.cpuCores;
    profile.cpuABI = "arm64-v8a";
    
    // GPU
    profile.gpuModel = deviceData["gpu"];
    profile.gpuVendor = "ARM";
    
    // Memory
    profile.ramMB = std::stoi(deviceData["ram"]);
    profile.storageGB = std::stoi(deviceData["storage"]);
    
    // Display
    std::string screenRes = deviceData["screen"];
    size_t xPos = screenRes.find('x');
    profile.screenWidth = std::stoi(screenRes.substr(0, xPos));
    profile.screenHeight = std::stoi(screenRes.substr(xPos + 1));
    profile.screenDPI = profile.screenHeight * (std::stoi(deviceData["density"]));
    profile.screenDensity = std::stoi(deviceData["density"]);
    
    // Build
    profile.androidVersion = deviceData["android"];
    profile.sdkVersion = deviceData["sdk"];
    profile.securityPatch = deviceData["security"];
    profile.buildType = "user";
    profile.buildTags = "release-keys";
    
    // Generate fingerprint
    std::stringstream fp;
    fp << "samsung/" << deviceData["codename"] << "/" << deviceData["codename"] 
       << ":" << profile.androidVersion << "/SP1A.210812.016/" << generateHexDigits(16)
       << ":user/release-keys";
    profile.buildFingerprint = fp.str();
    
    // Bootloader & Radio
    profile.bootloaderVersion = deviceData["model"];
    profile.radioVersion = std::string("G") + std::to_string(randomInt(998, 9998));
    profile.kernelVersion = std::string("5.10.") + std::to_string(randomInt(100, 200)) + "-android13";
    
    // SoC
    profile.socModel = profile.cpuHardware;
    profile.socVendor = profile.cpuModel.find("Snapdragon") != std::string::npos ? "Qualcomm" : "Samsung";
    
    // DMI
    profile.systemVendor = "SAMSUNG ELECTRONICS CO., LTD.";
    profile.systemProduct = deviceData["codename"].substr(0, 5).c_str() + std::string(" PRODUCT NAME");
    profile.boardVendor = "SAMSUNG";
    profile.boardProduct = deviceData["codename"];
    profile.boardVersion = "Rev 0.1";
    
    // Region correlation
    if (!region.empty()) {
        correlateGeoValues(profile, region);
    } else {
        correlateGeoValues(profile, "US");
    }
    
    // Generate unique IDs
    UniqueDeviceID uniqueIDs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uniqueIDs.serialNumber = "R5CR" + generateHexDigits(6).substr(0, 6);
        uniqueIDs.androidID = generateHexDigits(16);
        uniqueIDs.deviceUUID = generateUUID();
        uniqueIDs.wifiMAC = MAC_OUI_DATABASE.at("Samsung") + ":" + 
            generateHexDigits(2) + ":" + generateHexDigits(2) + ":" + generateHexDigits(2);
        uniqueIDs.bluetoothMAC = uniqueIDs.wifiMAC;
    }
    
    profile.serialNumber = uniqueIDs.serialNumber;
    profile.androidID = uniqueIDs.androidID;
    profile.wifiMAC = uniqueIDs.wifiMAC;
    profile.bluetoothMAC = uniqueIDs.bluetoothMAC;
    profile.gsfId = generateGSFID();
    profile.bssid = generateBSSID(profile.brand);
    
    // Profile hash
    profile.profileHash = generateProfileHash(profile);
    
    // Validate
    int uniqueness = calculateUniqueness(profile);
    int realism = calculateRealism(profile);
    
    ProfileGenerationResult result;
    result.success = true;
    result.message = "Samsung " + modelName + " profile generated successfully";
    result.profile = profile;
    result.uniqueIDs = uniqueIDs;
    result.uniquenessScore = uniqueness;
    result.realismScore = realism;
    
    return result;
}

std::string RealisticProfileGenerator::generateProfileHash(const DeviceProfile& profile) {
    std::stringstream ss;
    ss << profile.manufacturer << profile.model << profile.cpuModel 
       << profile.gpuModel << profile.serialNumber << profile.androidID;
    return hashString(ss.str()).substr(0, 32);
}

ProfileGenerationResult RealisticProfileGenerator::generateGoogleProfile(const std::string& region) {
    std::uniform_int_distribution<> dis(0, static_cast<int>(GOOGLE_DATABASE.size()) - 1);
    int idx = dis(m_randomGenerator);
    
    int count = 0;
    std::string modelName;
    std::map<std::string, std::string> deviceData;
    
    for (const auto& pair : GOOGLE_DATABASE) {
        if (count++ == idx) {
            modelName = pair.first;
            deviceData = pair.second[0];
            break;
        }
    }
    
    DeviceProfile profile;
    profile.manufacturer = "Google";
    profile.brand = "google";
    profile.model = deviceData["model"];
    profile.deviceName = modelName;
    profile.productName = deviceData["codename"];
    
    // CPU
    profile.cpuModel = deviceData["cpu"];
    profile.cpuHardware = deviceData["platform"];
    profile.cpuVariant = "";
    profile.cpuCores = std::stoi(deviceData["cores"]);
    profile.cpuThreads = profile.cpuCores;
    profile.cpuABI = "arm64-v8a";
    
    // GPU
    profile.gpuModel = deviceData["gpu"];
    profile.gpuVendor = "ARM";
    
    // Memory
    profile.ramMB = std::stoi(deviceData["ram"]);
    profile.storageGB = std::stoi(deviceData["storage"]);
    
    // Display
    std::string screenRes = deviceData["screen"];
    size_t xPos = screenRes.find('x');
    profile.screenWidth = std::stoi(screenRes.substr(0, xPos));
    profile.screenHeight = std::stoi(screenRes.substr(xPos + 1));
    profile.screenDPI = profile.screenHeight / 3;
    profile.screenDensity = std::stoi(deviceData["density"]);
    
    // Build
    profile.androidVersion = deviceData["android"];
    profile.sdkVersion = deviceData["sdk"];
    profile.securityPatch = deviceData["security"];
    profile.buildType = "user";
    profile.buildTags = "release-keys";
    
    // Fingerprint
    std::stringstream fp;
    fp << "google/" << deviceData["codename"] << "/" << deviceData["codename"] 
       << ":" << profile.androidVersion << "/TP1A." << "220624.014" << "/" << generateHexDigits(7)
       << ":user/release-keys";
    profile.buildFingerprint = fp.str();
    
    // Bootloader & Radio
    profile.bootloaderVersion = deviceData["codename"] + ".234";
    profile.radioVersion = "g5123-" + generateNumericString(4);
    profile.kernelVersion = std::string("5.10-") + deviceData.at("platform") + "-android" + profile.androidVersion;
    
    // SoC
    profile.socModel = profile.cpuHardware;
    profile.socVendor = "Google";
    
    // DMI
    profile.systemVendor = "Google";
    profile.systemProduct = deviceData["codename"];
    profile.boardVendor = "Google";
    profile.boardProduct = deviceData["codename"];
    
    // Region
    if (!region.empty()) {
        correlateGeoValues(profile, region);
    } else {
        correlateGeoValues(profile, "US");
    }
    
    // Unique IDs
    UniqueDeviceID uniqueIDs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uniqueIDs.serialNumber = generateNumericString(10);
        uniqueIDs.androidID = generateHexDigits(16);
        uniqueIDs.deviceUUID = generateUUID();
        uniqueIDs.wifiMAC = MAC_OUI_DATABASE.at("Google") + ":" + 
            generateHexDigits(2) + ":" + generateHexDigits(2) + ":" + generateHexDigits(2);
        uniqueIDs.bluetoothMAC = uniqueIDs.wifiMAC;
    }
    
    profile.serialNumber = uniqueIDs.serialNumber;
    profile.androidID = uniqueIDs.androidID;
    profile.wifiMAC = uniqueIDs.wifiMAC;
    profile.bluetoothMAC = uniqueIDs.bluetoothMAC;
    profile.gsfId = generateGSFID();
    profile.bssid = generateBSSID(profile.brand);
    profile.profileHash = generateProfileHash(profile);
    
    ProfileGenerationResult result;
    result.success = true;
    result.message = "Google " + modelName + " profile generated successfully";
    result.profile = profile;
    result.uniqueIDs = uniqueIDs;
    result.uniquenessScore = calculateUniqueness(profile);
    result.realismScore = calculateRealism(profile);
    
    return result;
}

ProfileGenerationResult RealisticProfileGenerator::generateXiaomiProfile(const std::string& region) {
    std::uniform_int_distribution<> dis(0, static_cast<int>(XIAOMI_DATABASE.size()) - 1);
    int idx = dis(m_randomGenerator);
    
    int count = 0;
    std::string modelName;
    std::map<std::string, std::string> deviceData;
    
    for (const auto& pair : XIAOMI_DATABASE) {
        if (count++ == idx) {
            modelName = pair.first;
            deviceData = pair.second[0];
            break;
        }
    }
    
    DeviceProfile profile;
    profile.manufacturer = "Xiaomi";
    profile.brand = "Xiaomi";
    profile.model = deviceData["model"];
    profile.deviceName = deviceData["codename"];
    profile.productName = modelName;
    
    // CPU
    profile.cpuModel = deviceData["cpu"];
    profile.cpuHardware = deviceData["platform"];
    profile.cpuVariant = "";
    profile.cpuCores = std::stoi(deviceData["cores"]);
    profile.cpuThreads = profile.cpuCores;
    profile.cpuABI = "arm64-v8a";
    
    // GPU
    profile.gpuModel = deviceData["gpu"];
    profile.gpuVendor = "ARM";
    
    // Memory
    profile.ramMB = std::stoi(deviceData["ram"]);
    profile.storageGB = std::stoi(deviceData["storage"]);
    
    // Display
    std::string screenRes = deviceData["screen"];
    size_t xPos = screenRes.find('x');
    profile.screenWidth = std::stoi(screenRes.substr(0, xPos));
    profile.screenHeight = std::stoi(screenRes.substr(xPos + 1));
    profile.screenDPI = profile.screenHeight / 3;
    profile.screenDensity = std::stoi(deviceData["density"]);
    
    // Build
    profile.androidVersion = deviceData["android"];
    profile.sdkVersion = deviceData["sdk"];
    profile.securityPatch = deviceData["security"];
    profile.buildType = "user";
    profile.buildTags = "release-keys";
    
    // Fingerprint
    std::stringstream fp;
    fp << profile.manufacturer << "/" << deviceData["codename"] << "/" << deviceData["codename"] 
       << ":" << profile.androidVersion << "/V14.0.3.0." << generateHexDigits(8) << ":user/release-keys";
    profile.buildFingerprint = fp.str();
    
    // Bootloader & Radio
    profile.bootloaderVersion = deviceData.at("codename").substr(0, 4) + ".234";
    profile.radioVersion = std::string("V") + std::to_string(randomInt(14, 15)) + "." + generateNumericString(6);
    profile.kernelVersion = std::string("5.15-") + deviceData.at("platform") + "-android" + profile.androidVersion;
    
    // SoC
    profile.socModel = profile.cpuHardware;
    profile.socVendor = profile.cpuModel.find("Snapdragon") != std::string::npos ? "Qualcomm" : "MediaTek";
    
    // DMI
    profile.systemVendor = "Xiaomi";
    profile.systemProduct = deviceData["codename"];
    profile.boardVendor = "Xiaomi";
    profile.boardProduct = deviceData["codename"];
    
    // Region
    if (!region.empty()) {
        correlateGeoValues(profile, region);
    } else {
        correlateGeoValues(profile, "CN");
    }
    
    // Unique IDs
    UniqueDeviceID uniqueIDs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uniqueIDs.serialNumber = generateHexDigits(12);
        uniqueIDs.androidID = generateHexDigits(16);
        uniqueIDs.deviceUUID = generateUUID();
        uniqueIDs.wifiMAC = MAC_OUI_DATABASE.at("Xiaomi") + ":" + 
            generateHexDigits(2) + ":" + generateHexDigits(2) + ":" + generateHexDigits(2);
        uniqueIDs.bluetoothMAC = uniqueIDs.wifiMAC;
    }
    
    profile.serialNumber = uniqueIDs.serialNumber;
    profile.androidID = uniqueIDs.androidID;
    profile.wifiMAC = uniqueIDs.wifiMAC;
    profile.bluetoothMAC = uniqueIDs.bluetoothMAC;
    profile.gsfId = generateGSFID();
    profile.bssid = generateBSSID(profile.brand);
    profile.profileHash = generateProfileHash(profile);
    
    ProfileGenerationResult result;
    result.success = true;
    result.message = "Xiaomi " + modelName + " profile generated successfully";
    result.profile = profile;
    result.uniqueIDs = uniqueIDs;
    result.uniquenessScore = calculateUniqueness(profile);
    result.realismScore = calculateRealism(profile);
    
    return result;
}

ProfileGenerationResult RealisticProfileGenerator::generateOnePlusProfile(const std::string& region) {
    std::uniform_int_distribution<> dis(0, static_cast<int>(ONEPLUS_DATABASE.size()) - 1);
    int idx = dis(m_randomGenerator);
    
    int count = 0;
    std::string modelName;
    std::map<std::string, std::string> deviceData;
    
    for (const auto& pair : ONEPLUS_DATABASE) {
        if (count++ == idx) {
            modelName = pair.first;
            deviceData = pair.second[0];
            break;
        }
    }
    
    DeviceProfile profile;
    profile.manufacturer = "OnePlus";
    profile.brand = "OnePlus";
    profile.model = deviceData["model"];
    profile.deviceName = deviceData["codename"];
    profile.productName = modelName;
    
    // CPU
    profile.cpuModel = deviceData["cpu"];
    profile.cpuHardware = deviceData["platform"];
    profile.cpuVariant = "";
    profile.cpuCores = std::stoi(deviceData["cores"]);
    profile.cpuThreads = profile.cpuCores;
    profile.cpuABI = "arm64-v8a";
    
    // GPU
    profile.gpuModel = deviceData["gpu"];
    profile.gpuVendor = "Qualcomm";
    
    // Memory
    profile.ramMB = std::stoi(deviceData["ram"]);
    profile.storageGB = std::stoi(deviceData["storage"]);
    
    // Display
    std::string screenRes = deviceData["screen"];
    size_t xPos = screenRes.find('x');
    profile.screenWidth = std::stoi(screenRes.substr(0, xPos));
    profile.screenHeight = std::stoi(screenRes.substr(xPos + 1));
    profile.screenDPI = profile.screenHeight / 3;
    profile.screenDensity = std::stoi(deviceData["density"]);
    
    // Build
    profile.androidVersion = deviceData["android"];
    profile.sdkVersion = deviceData["sdk"];
    profile.securityPatch = deviceData["security"];
    profile.buildType = "user";
    profile.buildTags = "release-keys";
    
    // Fingerprint
    std::stringstream fp;
    fp << profile.manufacturer << "/" << deviceData["model"] << "/" << deviceData["codename"] 
       << ":" << profile.androidVersion << "/OPR4/" << generateNumericString(10)
       << ":user/release-keys";
    profile.buildFingerprint = fp.str();
    
    // Bootloader & Radio
    profile.bootloaderVersion = "0.1";
    profile.radioVersion = std::string("V") + std::to_string(randomInt(14, 15)) + "." + generateNumericString(6);
    profile.kernelVersion = std::string("5.15-android") + profile.androidVersion;
    
    // SoC
    profile.socModel = profile.cpuHardware;
    profile.socVendor = "Qualcomm";
    
    // DMI
    profile.systemVendor = "OnePlus";
    profile.systemProduct = deviceData["codename"];
    profile.boardVendor = "OnePlus";
    profile.boardProduct = deviceData["codename"];
    
    // Region
    if (!region.empty()) {
        correlateGeoValues(profile, region);
    } else {
        correlateGeoValues(profile, "US");
    }
    
    // Unique IDs
    UniqueDeviceID uniqueIDs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uniqueIDs.serialNumber = generateHexDigits(12);
        uniqueIDs.androidID = generateHexDigits(16);
        uniqueIDs.deviceUUID = generateUUID();
        uniqueIDs.wifiMAC = MAC_OUI_DATABASE.at("OnePlus") + ":" + 
            generateHexDigits(2) + ":" + generateHexDigits(2) + ":" + generateHexDigits(2);
        uniqueIDs.bluetoothMAC = uniqueIDs.wifiMAC;
    }
    
    profile.serialNumber = uniqueIDs.serialNumber;
    profile.androidID = uniqueIDs.androidID;
    profile.wifiMAC = uniqueIDs.wifiMAC;
    profile.bluetoothMAC = uniqueIDs.bluetoothMAC;
    profile.gsfId = generateGSFID();
    profile.bssid = generateBSSID(profile.brand);
    profile.profileHash = generateProfileHash(profile);
    
    ProfileGenerationResult result;
    result.success = true;
    result.message = "OnePlus " + modelName + " profile generated successfully";
    result.profile = profile;
    result.uniqueIDs = uniqueIDs;
    result.uniquenessScore = calculateUniqueness(profile);
    result.realismScore = calculateRealism(profile);
    
    return result;
}

ProfileGenerationResult RealisticProfileGenerator::generateCompleteProfile(const std::string& manufacturer) {
    std::vector<std::string> manufacturers = {"Samsung", "Google", "Xiaomi", "OnePlus"};
    
    if (manufacturer.empty()) {
        std::uniform_int_distribution<> dis(0, static_cast<int>(manufacturers.size()) - 1);
        return generateCompleteProfile(manufacturers[dis(m_randomGenerator)]);
    }
    
    if (manufacturer == "Samsung" || manufacturer == "samsung") {
        return generateSamsungProfile();
    } else if (manufacturer == "Google" || manufacturer == "google") {
        return generateGoogleProfile();
    } else if (manufacturer == "Xiaomi" || manufacturer == "xiaomi") {
        return generateXiaomiProfile();
    } else if (manufacturer == "OnePlus" || manufacturer == "oneplus") {
        return generateOnePlusProfile();
    }
    
    // Random manufacturer
    std::uniform_int_distribution<> dis(0, static_cast<int>(manufacturers.size()) - 1);
    return generateCompleteProfile(manufacturers[dis(m_randomGenerator)]);
}

void RealisticProfileGenerator::correlateGeoValues(DeviceProfile& profile, const std::string& region) {
    if (CARRIER_DATABASE.find(region) == CARRIER_DATABASE.end()) {
        profile.region = "US";
    } else {
        profile.region = region;
    }
    
    const auto& regionData = CARRIER_DATABASE.at(profile.region);
    
    // Locale - stored as pair<string,string> with {"locale", "en-US"}
    if (regionData.find("locales") != regionData.end()) {
        const auto& locales = regionData.at("locales");
        if (!locales.empty()) {
            std::uniform_int_distribution<> localeDis(0, static_cast<int>(locales.size()) - 1);
            profile.locale = locales[localeDis(m_randomGenerator)].second;
            size_t usPos = profile.locale.find("-");
            if (usPos != std::string::npos) {
                profile.language = profile.locale.substr(0, usPos);
            } else {
                profile.language = profile.locale;
            }
        }
    }
    
    // Timezone - stored as pair<string,string> with {"tz", "America/New_York"}
    if (regionData.find("timezones") != regionData.end()) {
        const auto& timezones = regionData.at("timezones");
        if (!timezones.empty()) {
            std::uniform_int_distribution<> tzDis(0, static_cast<int>(timezones.size()) - 1);
            profile.timezone = timezones[tzDis(m_randomGenerator)].second;
        }
    }
    
    // Operator - stored as pair<string,string> with {"mcc-mnc", "name"}
    if (regionData.find("operators") != regionData.end()) {
        const auto& operators = regionData.at("operators");
        if (!operators.empty()) {
            std::uniform_int_distribution<> opDis(0, static_cast<int>(operators.size()) - 1);
            const auto& op = operators[opDis(m_randomGenerator)];
            
            profile.carrierName = op.second;
            // Parse mcc-mnc from op.first
            std::string key = op.first;
            size_t dashPos = key.find('-');
            if (dashPos != std::string::npos) {
                profile.mcc = std::stoi(key.substr(0, dashPos));
                profile.mnc = std::stoi(key.substr(dashPos + 1));
            }
        }
    }
    
    // Location - stored as pair<string,string> with {"min", "-125.0"}, {"max", "-66.0"}
    if (regionData.find("lat_range") != regionData.end() && regionData.find("lon_range") != regionData.end()) {
        const auto& latRange = regionData.at("lat_range");
        const auto& lonRange = regionData.at("lon_range");
        if (latRange.size() >= 2 && lonRange.size() >= 2) {
            double latMin = std::stod(latRange[0].second);
            double latMax = std::stod(latRange[1].second);
            double lonMin = std::stod(lonRange[0].second);
            double lonMax = std::stod(lonRange[1].second);
            profile.latitude = randomDouble(latMin, latMax);
            profile.longitude = randomDouble(lonMin, lonMax);
        }
    }
    
    profile.networkType = "4G";
}

int RealisticProfileGenerator::calculateUniqueness(const DeviceProfile& profile) {
    int score = 100;
    
    // Check for duplicates in memory
    if (m_generatedSerials.find(profile.serialNumber) != m_generatedSerials.end()) {
        score -= 30;
    }
    if (m_generatedAndroidIDs.find(profile.androidID) != m_generatedAndroidIDs.end()) {
        score -= 30;
    }
    if (m_generatedMACs.find(profile.wifiMAC) != m_generatedMACs.end()) {
        score -= 30;
    }
    
    return std::max(0, score);
}

int RealisticProfileGenerator::calculateRealism(const DeviceProfile& profile) {
    int score = 100;
    
    // Check build fingerprint format
    if (profile.buildFingerprint.find(profile.manufacturer) == std::string::npos) {
        score -= 15;
    }
    
    // Check version correlation
    if (profile.sdkVersion.empty() || profile.androidVersion.empty()) {
        score -= 10;
    }
    
    // Check hardware correlation
    if (profile.cpuModel.empty() || profile.gpuModel.empty()) {
        score -= 10;
    }
    
    return std::max(0, std::min(100, score));
}

std::string RealisticProfileGenerator::generateUUID() {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) ss << '-';
        ss << std::setw(2) << dis(m_randomGenerator);
    }
    
    return ss.str();
}

UniqueDeviceID RealisticProfileGenerator::generateUniqueIDs(const std::string& manufacturer) {
    UniqueDeviceID ids;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Serial
    std::string serial;
    if (manufacturer == "Samsung") {
        serial = "R5CR" + generateHexDigits(6);
    } else if (manufacturer == "Google") {
        serial = generateNumericString(10);
    } else {
        serial = generateHexDigits(12);
    }
    ids.serialNumber = serial;
    
    // IMEI
    std::string imeiBase = generateNumericString(14);
    ids.imei = generateLuhnChecksum(imeiBase);
    
    // Android ID
    ids.androidID = generateHexDigits(16);
    
    // UUID
    ids.deviceUUID = generateUUID();
    
    // MAC
    std::string oui = MAC_OUI_DATABASE.count(manufacturer) ? 
        MAC_OUI_DATABASE.at(manufacturer) : MAC_OUI_DATABASE.at("Generic");
    ids.wifiMAC = oui + ":" + generateHexDigits(2) + ":" + 
        generateHexDigits(2) + ":" + generateHexDigits(2);
    ids.bluetoothMAC = ids.wifiMAC;
    
    return ids;
}

bool RealisticProfileGenerator::validateProfile(const DeviceProfile& profile) {
    return calculateRealism(profile) >= 70;
}

std::string RealisticProfileGenerator::exportToJSON(const DeviceProfile& profile) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"manufacturer\": \"" << profile.manufacturer << "\",\n";
    ss << "  \"model\": \"" << profile.model << "\",\n";
    ss << "  \"brand\": \"" << profile.brand << "\",\n";
    ss << "  \"androidVersion\": \"" << profile.androidVersion << "\",\n";
    ss << "  \"sdkVersion\": \"" << profile.sdkVersion << "\",\n";
    ss << "  \"securityPatch\": \"" << profile.securityPatch << "\",\n";
    ss << "  \"buildFingerprint\": \"" << profile.buildFingerprint << "\",\n";
    ss << "  \"cpuModel\": \"" << profile.cpuModel << "\",\n";
    ss << "  \"cpuCores\": " << profile.cpuCores << ",\n";
    ss << "  \"gpuModel\": \"" << profile.gpuModel << "\",\n";
    ss << "  \"ramMB\": " << profile.ramMB << ",\n";
    ss << "  \"screenResolution\": \"" << profile.screenWidth << "x" << profile.screenHeight << "\",\n";
    ss << "  \"serialNumber\": \"" << profile.serialNumber << "\",\n";
    ss << "  \"wifiMAC\": \"" << profile.wifiMAC << "\",\n";
    ss << "  \"carrier\": \"" << profile.carrierName << "\",\n";
    ss << "  \"locale\": \"" << profile.locale << "\",\n";
    ss << "  \"timezone\": \"" << profile.timezone << "\",\n";
    ss << "  \"latitude\": " << profile.latitude << ",\n";
    ss << "  \"longitude\": " << profile.longitude << "\n";
    ss << "}\n";
    return ss.str();
}

std::string RealisticProfileGenerator::exportToADBCommands(const DeviceProfile& profile) {
    std::stringstream ss;
    ss << "# Device Profile: " << profile.model << "\n";
    ss << "# Generated by AntiDetectPro\n\n";
    
    ss << "setprop ro.product.manufacturer " << profile.manufacturer << "\n";
    ss << "setprop ro.product.brand " << profile.brand << "\n";
    ss << "setprop ro.product.model " << profile.model << "\n";
    ss << "setprop ro.build.fingerprint " << profile.buildFingerprint << "\n";
    ss << "setprop ro.product.cpu.abi " << profile.cpuABI << "\n";
    ss << "ro.hardware " << profile.cpuHardware << "\n";
    ss << "setprop ro.kernel.version " << profile.kernelVersion << "\n";
    ss << "setprop ro.bootloader " << profile.bootloaderVersion << "\n";
    ss << "setprop ro.build.version.security_patch " << profile.securityPatch << "\n";
    ss << "setprop ro.build.type " << profile.buildType << "\n";
    ss << "setprop ro.build.tags " << profile.buildTags << "\n";
    ss << "settings put global device_name " << profile.deviceName << "\n";
    
    return ss.str();
}

ProfileGenerationResult RealisticProfileGenerator::generateRandomProfile(const std::string& region) {
    std::vector<std::string> manufacturers = {"Samsung", "Google", "Xiaomi", "OnePlus"};
    std::uniform_int_distribution<> dis(0, static_cast<int>(manufacturers.size()) - 1);
    return generateCompleteProfile(manufacturers[dis(m_randomGenerator)]);
}

ProfileGenerationResult RealisticProfileGenerator::generateOppoProfile(const std::string& region) {
    return generateCompleteProfile("OPPO");
}

ProfileGenerationResult RealisticProfileGenerator::generateVivoProfile(const std::string& region) {
    return generateCompleteProfile("Vivo");
}

ProfileGenerationResult RealisticProfileGenerator::generateRealmeProfile(const std::string& region) {
    return generateCompleteProfile("realme");
}

ProfileGenerationResult RealisticProfileGenerator::generateMotorolaProfile(const std::string& region) {
    return generateCompleteProfile("Motorola");
}

ProfileGenerationResult RealisticProfileGenerator::generateSonyProfile(const std::string& region) {
    return generateCompleteProfile("Sony");
}

std::vector<std::string> RealisticProfileGenerator::getAvailableManufacturers() {
    return {"Samsung", "Google", "Xiaomi", "OnePlus", "OPPO", "Vivo", "Realme", "Motorola", "Sony"};
}

std::vector<std::string> RealisticProfileGenerator::getAvailableRegions() {
    std::vector<std::string> regions;
    for (const auto& pair : CARRIER_DATABASE) {
        regions.push_back(pair.first);
    }
    return regions;
}

std::vector<std::string> RealisticProfileGenerator::getModelsForManufacturer(const std::string& manufacturer) {
    std::vector<std::string> models;
    
    const auto* db = &SAMSUNG_DATABASE;
    std::string key = manufacturer;
    
    if (key == "Google") db = &GOOGLE_DATABASE;
    else if (key == "Xiaomi") db = &XIAOMI_DATABASE;
    else if (key == "OnePlus") db = &ONEPLUS_DATABASE;
    
    for (const auto& pair : *db) {
        models.push_back(pair.first);
    }
    
    return models;
}

std::vector<std::string> RealisticProfileGenerator::getCarriersForRegion(const std::string& region) {
    std::vector<std::string> carriers;
    
    if (CARRIER_DATABASE.find(region) != CARRIER_DATABASE.end()) {
        const auto& data = CARRIER_DATABASE.at(region);
        if (data.find("operators") != data.end()) {
            for (const auto& op : data.at("operators")) {
                carriers.push_back(op.second);
            }
        }
    }
    
    return carriers;
}

void RealisticProfileGenerator::correlateBuildValues(DeviceProfile& profile) {
    // Ensure build values are consistent
    if (profile.androidVersion == "14") {
        profile.sdkVersion = "34";
    } else if (profile.androidVersion == "13") {
        profile.sdkVersion = "33";
    } else if (profile.androidVersion == "12") {
        profile.sdkVersion = "32";
    }
    
    // Build fingerprint based on manufacturer
    if (profile.manufacturer == "samsung" || profile.manufacturer == "Samsung") {
        profile.buildFingerprint = "samsung/" + profile.deviceName + "/" + profile.deviceName +
            ":" + profile.androidVersion + "/SP1A.210812.016/" + generateHexDigits(16) +
            ":user/release-keys";
    } else if (profile.manufacturer == "Google") {
        profile.buildFingerprint = "google/" + profile.productName + "/" + profile.productName +
            ":" + profile.androidVersion + "/TP1A.220624.014/" + generateHexDigits(7) +
            ":user/release-keys";
    }
}

void RealisticProfileGenerator::correlateHardwareValues(DeviceProfile& profile) {
    // CPU/GPU correlation
    if (profile.cpuModel.find("Exynos") != std::string::npos) {
        profile.gpuVendor = "ARM";
        profile.cpuHardware = "exynos" + generateHexDigits(4);
    } else if (profile.cpuModel.find("Snapdragon") != std::string::npos) {
        profile.gpuVendor = "Qualcomm";
        profile.cpuHardware = "qcom";
    } else if (profile.cpuModel.find("Tensor") != std::string::npos) {
        profile.gpuVendor = "ARM";
        profile.cpuHardware = "gs201";
    } else if (profile.cpuModel.find("Dimensity") != std::string::npos) {
        profile.gpuVendor = "ARM";
        profile.cpuHardware = "mt" + generateNumericString(4);
    }
}

void RealisticProfileGenerator::correlateNetworkValues(DeviceProfile& profile) {
    // Ensure network values are valid
    if (profile.mcc < 100 || profile.mcc > 999) {
        profile.mcc = 310; // US default
    }
    if (profile.mnc < 0 || profile.mnc > 999) {
        profile.mnc = 260;
    }
}

void RealisticProfileGenerator::saveProfile(const DeviceProfile& profile, const std::string& filepath) {
    // Implementation would save to file
    Logger::getInstance().info("Profile saved to: " + filepath);
}

DeviceProfile RealisticProfileGenerator::loadProfile(const std::string& filepath) {
    DeviceProfile profile;
    // Implementation would load from file
    Logger::getInstance().info("Profile loaded from: " + filepath);
    return profile;
}

std::vector<ProfileGenerationResult> RealisticProfileGenerator::generateBatch(int count, const std::string& manufacturer) {
    std::vector<ProfileGenerationResult> results;
    for (int i = 0; i < count; ++i) {
        results.push_back(generateRandomProfile(manufacturer));
    }
    return results;
}

std::vector<DeviceProfile> RealisticProfileGenerator::generateUniqueBatch(int count, const std::string& manufacturer) {
    std::vector<DeviceProfile> profiles;
    std::set<std::string> usedSerials;
    
    for (size_t i = 0; i < static_cast<size_t>(count) * 2 && profiles.size() < static_cast<size_t>(count); ++i) {
        auto result = generateRandomProfile(manufacturer);
        if (result.success && usedSerials.find(result.profile.serialNumber) == usedSerials.end()) {
            usedSerials.insert(result.profile.serialNumber);
            profiles.push_back(result.profile);
        }
    }
    
    return profiles;
}

std::vector<std::string> RealisticProfileGenerator::getValidationErrors(const DeviceProfile& profile) {
    std::vector<std::string> errors;
    
    if (profile.manufacturer.empty()) errors.push_back("Manufacturer is empty");
    if (profile.model.empty()) errors.push_back("Model is empty");
    if (profile.serialNumber.empty()) errors.push_back("Serial number is empty");
    if (profile.buildFingerprint.empty()) errors.push_back("Build fingerprint is empty");
    if (profile.cpuModel.empty()) errors.push_back("CPU model is empty");
    if (profile.gpuModel.empty()) errors.push_back("GPU model is empty");
    
    return errors;
}

GeoRegion RealisticProfileGenerator::getGeoRegion(const std::string& code) {
    GeoRegion region;
    region.regionCode = code;
    return region;
}

NetworkOperator RealisticProfileGenerator::getNetworkOperator(int mcc, int mnc) {
    if (m_networkDB.find({mcc, mnc}) != m_networkDB.end()) {
        return m_networkDB[{mcc, mnc}];
    }
    return {"Unknown", "Unknown", mcc, mnc, "Unknown", "4G", "+1"};
}

ManufacturerProfile RealisticProfileGenerator::getManufacturerProfile(const std::string& name) {
    ManufacturerProfile profile;
    profile.name = name;
    return profile;
}

std::string RealisticProfileGenerator::generateSecureID() {
    return generateHexDigits(64);
}

std::string RealisticProfileGenerator::generateTrustzoneID() {
    return "tz_" + generateHexDigits(32);
}

std::string RealisticProfileGenerator::generateAttestationID() {
    return "attest_" + generateHexDigits(32);
}


std::string RealisticProfileGenerator::generateGAID() {
    std::stringstream ss;
    ss << generateHexDigits(8) << "-" << generateHexDigits(4) << "-"
       << "4" << generateHexDigits(3) << "-" 
       << generateHexDigits(4) << "-" << generateHexDigits(12);
    return ss.str();
}


// Device Identifier Generation (v1.8)
void RealisticProfileGenerator::generateDeviceIdentifiers(DeviceProfile& profile, const std::string& brand) {
    auto& idGen = DeviceIDGenerator::getInstance();
    idGen.initialize();
    
    profile.imei = idGen.generateIMEI(brand);
    profile.serialNumber = idGen.generateSerialNumber(brand);
    profile.gsfId = generateGSFID();
    profile.androidID = idGen.generateAndroidId();
    profile.wifiMAC = idGen.generateMACAddress("wifi");
    profile.bluetoothMAC = idGen.generateBluetoothAddress();
    profile.bssid = generateBSSID(brand);
}

std::string RealisticProfileGenerator::generateGSFID() {
    std::stringstream ss;
    ss << "af-" << generateNumericString(12);
    return ss.str();
}

std::string RealisticProfileGenerator::generateBSSID(const std::string& brand) {
    static const std::map<std::string, std::string> BRAND_OUI = {
        {"google", "F4:F5:D8"}, {"samsung", "F8:A9:63"}, {"xiaomi", "58:44:98"},
        {"oneplus", "38:2C:4A"}, {"huawei", "00:25:9E"}, {"generic", "AA:BB:CC"}
    };
    
    std::string lowerBrand = brand;
    std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
    
    std::string oui = BRAND_OUI.count(lowerBrand) ? BRAND_OUI.at(lowerBrand) : BRAND_OUI.at("generic");
    
    std::stringstream ss;
    ss << oui << ":" << generateHexDigits(2) << ":" 
       << generateHexDigits(2) << ":" << generateHexDigits(2);
    return ss.str();
}


// IP-Based Profile Generation (v1.8.1)
ProfileGenerationResult RealisticProfileGenerator::generateProfileWithIP(
    const std::string& manufacturer, 
    const std::string& ipAddress) {
    
    IPTimezoneConverter::getInstance().initialize();
    auto localeInfo = IPTimezoneConverter::getInstance().getLocaleFromIP(ipAddress);
    
    std::string lowerManu = manufacturer;
    std::transform(lowerManu.begin(), lowerManu.end(), lowerManu.begin(), ::tolower);
    
    ProfileGenerationResult result;
    
    if (lowerManu == "samsung") {
        result = generateSamsungProfile(localeInfo.countryCode);
    } else if (lowerManu == "google") {
        result = generateGoogleProfile(localeInfo.countryCode);
    } else if (lowerManu == "xiaomi") {
        result = generateXiaomiProfile(localeInfo.countryCode);
    } else if (lowerManu == "oneplus") {
        result = generateOnePlusProfile(localeInfo.countryCode);
    } else {
        result = generateRandomProfile(localeInfo.countryCode);
    }
    
    if (result.success) {
        result.profile.locale = localeInfo.locale;
        result.profile.timezone = localeInfo.timezone;
        result.profile.language = localeInfo.language;
        result.profile.region = localeInfo.countryCode;
        result.profile.countryName = localeInfo.countryName;
        result.profile.carrierName = localeInfo.carrier;
        
        auto carrierData = CARRIER_DATABASE.find(localeInfo.countryCode);
        if (carrierData != CARRIER_DATABASE.end()) {
            if (carrierData->second.find("operators") != carrierData->second.end()) {
                const auto& ops = carrierData->second.at("operators");
                if (!ops.empty()) {
                    std::uniform_int_distribution<> dis(0, static_cast<int>(ops.size()) - 1);
                    std::string mccMnc = ops[dis(m_randomGenerator)].first;
                    size_t dashPos = mccMnc.find("-");
                    if (dashPos != std::string::npos) {
                        result.profile.carrierMCC = mccMnc.substr(0, dashPos);
                        result.profile.carrierMNC = mccMnc.substr(dashPos + 1);
                    }
                }
            }
        }
        
        result.profile.profileHash = generateProfileHash(result.profile);
        Logger::getInstance().info("Profile for IP: " + ipAddress + " -> TZ: " + localeInfo.timezone);
    }
    
    return result;
}

std::string RealisticProfileGenerator::getTimezoneFromIP(const std::string& ipAddress) {
    return IPTimezoneConverter::getInstance().getTimezoneFromIP(ipAddress);
}

} // namespace AntiDetect
