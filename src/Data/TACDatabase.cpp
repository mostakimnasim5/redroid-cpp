/**
 * @file TACDatabase.cpp
 * @brief Implementation of TAC Database
 */

#include "TACDatabase.h"
#include <random>
#include <algorithm>
#include <sstream>

namespace RedroidCPP {

// ============================================================================
// TACEntry Implementation
// ============================================================================

std::string TACEntry::toString() const {
    std::ostringstream oss;
    oss << "TAC: " << tac << "\n";
    oss << "  Brand: " << brand << "\n";
    oss << "  Model: " << modelName << "\n";
    oss << "  Internal: " << internalName << "\n";
    oss << "  Type: " << deviceType << "\n";
    oss << "  Class: " << deviceClass << "\n";
    oss << "  Launched: " << launchYear << "/" << launchMonth << "\n";
    oss << "  NFC: " << nfcSupport << ", BT: " << bluetoothSupport 
        << ", WiFi: " << wifiSupport << "\n";
    oss << "  LTE: " << lteSupport << ", 5G: " << fiveGSupport;
    return oss.str();
}

// ============================================================================
// TACDatabase Implementation
// ============================================================================

TACDatabase::TACDatabase() {
    initializeDatabase();
}

void TACDatabase::initializeDatabase() {
    // ========================================================================
    // Samsung TACs
    // ========================================================================
    const std::vector<std::string>& samsungTacs = TACCodes::SAMSUNG_TACS;
    for (size_t i = 0; i < samsungTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = samsungTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Samsung";
        
        // Galaxy S24 Series
        if (i < 8) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Galaxy S24 Ultra" : 
                             (i % 4 == 1) ? "Galaxy S24+" : 
                             (i % 4 == 2) ? "Galaxy S24" : "Galaxy S24 FE";
            entry.internalName = (i % 4 == 0) ? "dm3q" : 
                                (i % 4 == 1) ? "z3s" : 
                                (i % 4 == 2) ? "z4s" : "o1s";
            entry.launchYear = "2024";
            entry.launchMonth = "01";
            entry.deviceClass = "High-End";
        }
        // Galaxy S23 Series
        else if (i < 16) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Galaxy S23 Ultra" : 
                             (i % 4 == 1) ? "Galaxy S23+" : 
                             (i % 4 == 2) ? "Galaxy S23" : "Galaxy S23 FE";
            entry.internalName = (i % 4 == 0) ? "dm3" : 
                                (i % 4 == 1) ? "z3" : 
                                (i % 4 == 2) ? "z4" : "o1";
            entry.launchYear = "2023";
            entry.launchMonth = "02";
            entry.deviceClass = "High-End";
        }
        // Galaxy Note/S Series
        else if (i < 24) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Galaxy S22 Ultra" : 
                             (i % 4 == 1) ? "Galaxy S22+" : 
                             (i % 4 == 2) ? "Galaxy S22" : "Galaxy S21 FE";
            entry.internalName = "s" + std::to_string(22 - (i % 4) * 2);
            entry.launchYear = "2022";
            entry.launchMonth = "02";
            entry.deviceClass = "High-End";
        }
        // Galaxy Z Series
        else if (i < 32) {
            entry.deviceType = "Foldable Smartphone";
            entry.modelName = (i % 4 == 0) ? "Galaxy Z Fold5" : 
                             (i % 4 == 1) ? "Galaxy Z Flip5" : 
                             (i % 4 == 2) ? "Galaxy Z Fold4" : "Galaxy Z Flip4";
            entry.internalName = (i % 4 < 2) ? "q5" : "q4";
            entry.launchYear = (i % 4 < 2) ? "2023" : "2022";
            entry.launchMonth = "08";
            entry.deviceClass = "Premium Foldable";
        }
        // Galaxy A/M Series
        else if (i < 40) {
            entry.deviceType = "Smartphone";
            entry.modelName = "Galaxy A" + std::to_string(54 - (i % 4) * 5);
            entry.internalName = "a5" + std::to_string(54 - (i % 4) * 5);
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "Mid-Range";
        }
        // Galaxy Tab
        else {
            entry.deviceType = "Tablet";
            entry.modelName = "Galaxy Tab S" + std::to_string(9 - (i % 4));
            entry.internalName = "gts" + std::to_string(9 - (i % 4));
            entry.launchYear = "2023";
            entry.launchMonth = "07";
            entry.deviceClass = "High-End Tablet";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = (i < 32) ? "Yes" : "No";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Samsung", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Google Pixel TACs
    // ========================================================================
    const std::vector<std::string>& googleTacs = TACCodes::GOOGLE_TACS;
    for (size_t i = 0; i < googleTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = googleTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Google";
        
        if (i < 8) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Pixel 8 Pro" : 
                             (i % 4 == 1) ? "Pixel 8" : 
                             (i % 4 == 2) ? "Pixel 8a" : "Pixel 8 Pro XL";
            entry.internalName = (i % 4 == 0) ? "husky" : 
                                (i % 4 == 1) ? "shiba" : 
                                (i % 4 == 2) ? "akita" : "comet";
            entry.launchYear = "2023";
            entry.launchMonth = "10";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Pixel 7 Pro" : 
                             (i % 4 == 1) ? "Pixel 7" : 
                             (i % 4 == 2) ? "Pixel 7a" : "Pixel 7 Pro XL";
            entry.internalName = (i % 4 == 0) ? "cheetah" : 
                                (i % 4 == 1) ? "panther" : 
                                (i % 4 == 2) ? "lynx" : "cougar";
            entry.launchYear = "2022";
            entry.launchMonth = "10";
            entry.deviceClass = "High-End";
        }
        else if (i < 24) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 4 == 0) ? "Pixel 6 Pro" : 
                             (i % 4 == 1) ? "Pixel 6" : 
                             (i % 4 == 2) ? "Pixel 6a" : "Pixel 6 Pro XL";
            entry.internalName = (i % 4 == 0) ? "raven" : 
                                (i % 4 == 1) ? "oriole" : 
                                (i % 4 == 2) ? "bluejay" : "crow";
            entry.launchYear = "2021";
            entry.launchMonth = "10";
            entry.deviceClass = "High-End";
        }
        else if (i < 32) {
            entry.deviceType = "Foldable Smartphone";
            entry.modelName = "Pixel Fold";
            entry.internalName = "felix";
            entry.launchYear = "2023";
            entry.launchMonth = "06";
            entry.deviceClass = "Premium Foldable";
        }
        else {
            entry.deviceType = "Smartphone";
            entry.modelName = "Pixel 7a";
            entry.internalName = "lynx";
            entry.launchYear = "2023";
            entry.launchMonth = "05";
            entry.deviceClass = "Mid-Range";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.24";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Google", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Xiaomi TACs
    // ========================================================================
    const std::vector<std::string>& xiaomiTacs = TACCodes::XIAOMI_TACS;
    for (size_t i = 0; i < xiaomiTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = xiaomiTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Xiaomi";
        
        if (i < 16) {
            entry.deviceType = "Smartphone";
            entry.modelName = (i % 8 == 0) ? "Xiaomi 14 Pro" : 
                             (i % 8 == 1) ? "Xiaomi 14" : 
                             (i % 8 == 2) ? "Xiaomi 13 Pro" : 
                             (i % 8 == 3) ? "Xiaomi 13" :
                             (i % 8 == 4) ? "Xiaomi 13 Ultra" :
                             (i % 8 == 5) ? "Xiaomi 13T Pro" :
                             (i % 8 == 6) ? "Xiaomi 13T" : "Xiaomi 14 Ultra";
            entry.internalName = "sm" + std::to_string(14 - (i % 8) / 2);
            entry.launchYear = (i < 8) ? "2023" : "2024";
            entry.launchMonth = (i % 4 == 0) ? "12" : (i % 4 == 1) ? "11" : (i % 4 == 2) ? "09" : "06";
            entry.deviceClass = "High-End";
        }
        else if (i < 24) {
            entry.deviceType = "Smartphone";
            entry.modelName = "Redmi K" + std::to_string(70 - (i % 8) * 5);
            entry.internalName = "m20" + std::to_string(70 - (i % 8));
            entry.launchYear = "2024";
            entry.launchMonth = (i % 4 == 0) ? "01" : "03";
            entry.deviceClass = "High-End Gaming";
        }
        else if (i < 32) {
            entry.deviceType = "Smartphone";
            entry.modelName = "POCO F" + std::to_string(6 - (i % 4));
            entry.internalName = "poco_f" + std::to_string(6 - (i % 4));
            entry.launchYear = "2024";
            entry.launchMonth = (i % 4 == 0) ? "01" : "03";
            entry.deviceClass = "Gaming";
        }
        else {
            entry.deviceType = "Tablet";
            entry.modelName = "Xiaomi Pad " + std::to_string(6 - (i % 4));
            entry.internalName = "dagu";
            entry.launchYear = "2023";
            entry.launchMonth = "08";
            entry.deviceClass = "Mid-Range Tablet";
        }
        
        entry.nfcSupport = (i < 32) ? "Yes" : "No";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Xiaomi", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // OnePlus TACs
    // ========================================================================
    const std::vector<std::string>& oneplusTacs = TACCodes::ONEPLUS_TACS;
    for (size_t i = 0; i < oneplusTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = oneplusTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "OnePlus";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "OnePlus 12";
            entry.internalName = "OP595";
            entry.launchYear = "2024";
            entry.launchMonth = "01";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.modelName = "OnePlus 11";
            entry.internalName = "OP555";
            entry.launchYear = "2023";
            entry.launchMonth = "01";
            entry.deviceClass = "High-End";
        }
        else if (i < 24) {
            entry.modelName = "OnePlus 10T";
            entry.internalName = "OP541";
            entry.launchYear = "2022";
            entry.launchMonth = "08";
            entry.deviceClass = "High-End";
        }
        else {
            entry.modelName = "OnePlus Nord 3";
            entry.internalName = "CPH249";
            entry.launchYear = "2023";
            entry.launchMonth = "07";
            entry.deviceClass = "Mid-Range";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"OnePlus", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Sony TACs
    // ========================================================================
    const std::vector<std::string>& sonyTacs = TACCodes::SONY_TACS;
    for (size_t i = 0; i < sonyTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = sonyTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Sony";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "Xperia 1 V";
            entry.internalName = "PDX-245";
            entry.launchYear = "2023";
            entry.launchMonth = "06";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.modelName = "Xperia 5 V";
            entry.internalName = "PDX-245s";
            entry.launchYear = "2023";
            entry.launchMonth = "09";
            entry.deviceClass = "High-End Compact";
        }
        else if (i < 24) {
            entry.modelName = "Xperia 1 IV";
            entry.internalName = "PDX-244";
            entry.launchYear = "2022";
            entry.launchMonth = "06";
            entry.deviceClass = "High-End";
        }
        else {
            entry.modelName = "Xperia 10 V";
            entry.internalName = "XQ-DC72";
            entry.launchYear = "2023";
            entry.launchMonth = "06";
            entry.deviceClass = "Mid-Range";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.18";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Sony", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // OPPO TACs
    // ========================================================================
    const std::vector<std::string>& oppoTacs = TACCodes::OPPO_TACS;
    for (size_t i = 0; i < oppoTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = oppoTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "OPPO";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "Find X7 Pro";
            entry.internalName = "CPH259";
            entry.launchYear = "2024";
            entry.launchMonth = "01";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.modelName = "Find X6 Pro";
            entry.internalName = "CPH255";
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "High-End";
        }
        else if (i < 24) {
            entry.modelName = "Reno 11 Pro";
            entry.internalName = "CPH2599";
            entry.launchYear = "2024";
            entry.launchMonth = "01";
            entry.deviceClass = "Mid-High Range";
        }
        else {
            entry.modelName = "Reno 10 Pro";
            entry.internalName = "CPH254";
            entry.launchYear = "2023";
            entry.launchMonth = "05";
            entry.deviceClass = "Mid-High Range";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"OPPO", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Vivo TACs
    // ========================================================================
    const std::vector<std::string>& vivoTacs = TACCodes::VIVO_TACS;
    for (size_t i = 0; i < vivoTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = vivoTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Vivo";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "X100 Pro";
            entry.internalName = "V2245";
            entry.launchYear = "2023";
            entry.launchMonth = "11";
            entry.deviceClass = "High-End Camera";
        }
        else if (i < 16) {
            entry.modelName = "X90 Pro+";
            entry.internalName = "V2227A";
            entry.launchYear = "2022";
            entry.launchMonth = "11";
            entry.deviceClass = "High-End Camera";
        }
        else if (i < 24) {
            entry.modelName = "V30 Pro";
            entry.internalName = "V2318";
            entry.launchYear = "2024";
            entry.launchMonth = "02";
            entry.deviceClass = "Mid-High Range";
        }
        else {
            entry.modelName = "V27 Pro";
            entry.internalName = "V2230";
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "Mid-Range";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Vivo", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Huawei TACs
    // ========================================================================
    const std::vector<std::string>& huaweiTacs = TACCodes::HUAWEI_TACS;
    for (size_t i = 0; i < huaweiTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = huaweiTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Huawei";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "Mate 60 Pro";
            entry.internalName = "ALN-NX9";
            entry.launchYear = "2023";
            entry.launchMonth = "08";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.modelName = "P60 Pro";
            entry.internalName = "LNA-NX9";
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "High-End Camera";
        }
        else if (i < 24) {
            entry.modelName = "Mate 50 Pro";
            entry.internalName = "DCO-NX9";
            entry.launchYear = "2022";
            entry.launchMonth = "09";
            entry.deviceClass = "High-End";
        }
        else if (i < 32) {
            entry.modelName = "P50 Pro";
            entry.internalName = "JAD-LX9";
            entry.launchYear = "2021";
            entry.launchMonth = "08";
            entry.deviceClass = "High-End Camera";
        }
        else {
            entry.modelName = "Mate X5";
            entry.internalName = "ALT-NX9";
            entry.launchYear = "2023";
            entry.launchMonth = "09";
            entry.deviceClass = "Premium Foldable";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.21";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Huawei", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Motorola TACs
    // ========================================================================
    const std::vector<std::string>& motorolaTacs = TACCodes::MOTOROLA_TACS;
    for (size_t i = 0; i < motorolaTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = motorolaTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Motorola";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "Edge 50 Ultra";
            entry.internalName = "XT240";
            entry.launchYear = "2024";
            entry.launchMonth = "04";
            entry.deviceClass = "High-End";
        }
        else if (i < 16) {
            entry.modelName = "Edge 40 Pro";
            entry.internalName = "XT230";
            entry.launchYear = "2023";
            entry.launchMonth = "05";
            entry.deviceClass = "High-End";
        }
        else if (i < 24) {
            entry.modelName = "Edge 30 Ultra";
            entry.internalName = "XT220";
            entry.launchYear = "2022";
            entry.launchMonth = "09";
            entry.deviceClass = "High-End";
        }
        else {
            entry.modelName = "Moto G84";
            entry.internalName = "XT234";
            entry.launchYear = "2023";
            entry.launchMonth = "09";
            entry.deviceClass = "Mid-Range";
        }
        
        entry.nfcSupport = (i < 24) ? "Yes" : "No";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.18";
        entry.fiveGSupport = (i < 24) ? "Yes" : "No";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Motorola", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Realme TACs
    // ========================================================================
    const std::vector<std::string>& realmeTacs = TACCodes::REALME_TACS;
    for (size_t i = 0; i < realmeTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = realmeTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "realme";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "realme GT 5 Pro";
            entry.internalName = "RMX388";
            entry.launchYear = "2023";
            entry.launchMonth = "12";
            entry.deviceClass = "High-End Gaming";
        }
        else if (i < 16) {
            entry.modelName = "realme GT 3";
            entry.internalName = "RMX369";
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "High-End Gaming";
        }
        else if (i < 24) {
            entry.modelName = "realme 11 Pro+";
            entry.internalName = "RMX374";
            entry.launchYear = "2023";
            entry.launchMonth = "09";
            entry.deviceClass = "Mid-High Range";
        }
        else {
            entry.modelName = "realme C67";
            entry.internalName = "RMX389";
            entry.launchYear = "2023";
            entry.launchMonth = "12";
            entry.deviceClass = "Budget";
        }
        
        entry.nfcSupport = (i >= 16) ? "Yes" : "No";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ac";
        entry.lteSupport = "Cat.13";
        entry.fiveGSupport = (i < 24) ? "Yes" : "No";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Realme", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // ASUS TACs
    // ========================================================================
    const std::vector<std::string>& asusTacs = TACCodes::ASUS_TACS;
    for (size_t i = 0; i < asusTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = asusTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "ASUS";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "ROG Phone 8 Pro";
            entry.internalName = "AI2401";
            entry.launchYear = "2024";
            entry.launchMonth = "01";
            entry.deviceClass = "Gaming Flagship";
        }
        else if (i < 16) {
            entry.modelName = "ROG Phone 7 Ultimate";
            entry.internalName = "AI2205";
            entry.launchYear = "2023";
            entry.launchMonth = "04";
            entry.deviceClass = "Gaming Flagship";
        }
        else if (i < 24) {
            entry.modelName = "Zenfone 10";
            entry.internalName = "AI2302";
            entry.launchYear = "2023";
            entry.launchMonth = "06";
            entry.deviceClass = "Compact High-End";
        }
        else {
            entry.modelName = "ROG Phone 6D";
            entry.internalName = "AI2203";
            entry.launchYear = "2022";
            entry.launchMonth = "12";
            entry.deviceClass = "Gaming";
        }
        
        entry.nfcSupport = "Yes";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ax";
        entry.lteSupport = "Cat.20";
        entry.fiveGSupport = "Yes";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"ASUS", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }

    // ========================================================================
    // Nokia TACs
    // ========================================================================
    const std::vector<std::string>& nokiaTacs = TACCodes::NOKIA_TACS;
    for (size_t i = 0; i < nokiaTacs.size(); ++i) {
        TACEntry entry;
        entry.tac = nokiaTacs[i];
        entry.reportingBody = "GSMA";
        entry.brand = "Nokia";
        entry.deviceType = "Smartphone";
        
        if (i < 8) {
            entry.modelName = "Nokia G42";
            entry.internalName = "TA-158";
            entry.launchYear = "2023";
            entry.launchMonth = "06";
            entry.deviceClass = "Mid-Range";
        }
        else if (i < 16) {
            entry.modelName = "Nokia X30";
            entry.internalName = "TA-145";
            entry.launchYear = "2022";
            entry.launchMonth = "09";
            entry.deviceClass = "Mid-Range";
        }
        else if (i < 24) {
            entry.modelName = "Nokia G60 5G";
            entry.internalName = "TA-147";
            entry.launchYear = "2022";
            entry.launchMonth = "09";
            entry.deviceClass = "Mid-Range 5G";
        }
        else {
            entry.modelName = "Nokia C32";
            entry.internalName = "TA-155";
            entry.launchYear = "2023";
            entry.launchMonth = "03";
            entry.deviceClass = "Budget";
        }
        
        entry.nfcSupport = (i < 16) ? "Yes" : "No";
        entry.bluetoothSupport = "Yes";
        entry.wifiSupport = "802.11ac";
        entry.lteSupport = (i < 20) ? "Cat.6" : "Cat.4";
        entry.fiveGSupport = (i >= 16 && i < 24) ? "Yes" : "No";
        
        m_tacMap[entry.tac] = entry;
        m_manufacturerToTAC.insert({"Nokia", entry.tac});
        m_brandToTAC.insert({entry.brand, entry.tac});
        m_deviceClassToTAC.insert({entry.deviceClass, entry.tac});
    }
}

// ========================================================================
// Query Method Implementations
// ========================================================================

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

std::vector<TACEntry> TACDatabase::getByManufacturer(const std::string& manufacturer) const {
    std::vector<TACEntry> entries;
    auto range = m_manufacturerToTAC.equal_range(manufacturer);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto tacEntry = m_tacMap.find(it->second);
        if (tacEntry != m_tacMap.end()) {
            entries.push_back(tacEntry->second);
        }
    }
    
    return entries;
}

std::vector<TACEntry> TACDatabase::getByBrand(const std::string& brand) const {
    std::vector<TACEntry> entries;
    auto range = m_brandToTAC.equal_range(brand);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto tacEntry = m_tacMap.find(it->second);
        if (tacEntry != m_tacMap.end()) {
            entries.push_back(tacEntry->second);
        }
    }
    
    return entries;
}

std::vector<TACEntry> TACDatabase::getByDeviceClass(const std::string& deviceClass) const {
    std::vector<TACEntry> entries;
    auto range = m_deviceClassToTAC.equal_range(deviceClass);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto tacEntry = m_tacMap.find(it->second);
        if (tacEntry != m_tacMap.end()) {
            entries.push_back(tacEntry->second);
        }
    }
    
    return entries;
}

std::optional<TACEntry> TACDatabase::getRandomForManufacturer(const std::string& manufacturer) {
    auto entries = getByManufacturer(manufacturer);
    if (entries.empty()) {
        return std::nullopt;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, entries.size() - 1);
    
    return entries[dis(gen)];
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

std::vector<std::string> TACDatabase::getManufacturers() const {
    std::vector<std::string> manufacturers;
    std::set<std::string> unique;
    
    for (const auto& entry : m_manufacturerToTAC) {
        if (unique.find(entry.first) == unique.end()) {
            unique.insert(entry.first);
            manufacturers.push_back(entry.first);
        }
    }
    
    std::sort(manufacturers.begin(), manufacturers.end());
    return manufacturers;
}

size_t TACDatabase::size() const {
    return m_tacMap.size();
}

bool TACDatabase::isValidTACFormat(const std::string& tac) {
    if (tac.length() != 8) {
        return false;
    }
    
    for (char c : tac) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    
    return true;
}

} // namespace RedroidCPP
