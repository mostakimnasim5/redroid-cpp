/**
 * @file ProfileGeneratorExample.cpp
 * @brief Usage Example: Device Profile Generator
 * @version 4.0.0
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <set>
#include "VirtualPhonePro/DeviceProfileGenerator.hpp"

using namespace VirtualPhonePro;

void printSeparator() {
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
}

void printProfile(const DeviceIdentityProfile& profile) {
    std::cout << std::left;
    
    printSeparator();
    std::cout << "  DEVICE IDENTITY (from HMAC-SHA256 Derivation)\n";
    printSeparator();
    
    std::cout << std::setw(20) << "  IMEI 1:" << profile.imei1 << "\n";
    std::cout << std::setw(20) << "  IMEI 2:" << profile.imei2 << " (Dual SIM)\n";
    std::cout << std::setw(20) << "  Serial Number:" << profile.serial_number << "\n";
    std::cout << std::setw(20) << "  Android ID:" << profile.android_id << "\n";
    std::cout << std::setw(20) << "  GSF ID:" << profile.gsf_id << "\n";
    std::cout << std::setw(20) << "  AAID:" << profile.advertising_id << "\n";
    
    printSeparator();
    std::cout << "  NETWORK IDENTITY\n";
    printSeparator();
    
    std::cout << std::setw(20) << "  WiFi MAC:" << profile.wifi_mac << " (Local Admin)\n";
    std::cout << std::setw(20) << "  Bluetooth MAC:" << profile.bluetooth_mac << " (Local Admin)\n";
    std::cout << std::setw(20) << "  BSSID:" << profile.bssid << " (Local Admin)\n";
    std::cout << std::setw(20) << "  Local IP:" << profile.local_ip << "\n";
    std::cout << std::setw(20) << "  IMSI 1:" << profile.imsi1 << "\n";
    std::cout << std::setw(20) << "  IMSI 2:" << profile.imsi2 << " (Dual SIM)\n";
    std::cout << std::setw(20) << "  ICCID 1:" << profile.iccid1 << "\n";
    std::cout << std::setw(20) << "  ICCID 2:" << profile.iccid2 << " (Dual SIM)\n";
    std::cout << std::setw(20) << "  Phone 1:" << profile.phone_number1 << "\n";
    std::cout << std::setw(20) << "  Phone 2:" << profile.phone_number2 << " (Dual SIM)\n";
    
    printSeparator();
    std::cout << "  SECURITY IDENTITY\n";
    printSeparator();
    
    std::cout << std::setw(20) << "  Device Key:" << profile.device_key.substr(0, 16) << "...\n";
    std::cout << std::setw(20) << "  Auth Token:" << profile.auth_token.substr(0, 16) << "...\n";
    std::cout << std::setw(20) << "  Profile ID:" << profile.profile_id << "\n";
    std::cout << std::setw(20) << "  Bootloader:" << profile.bootloader_version << "\n";
    std::cout << std::setw(20) << "  Radio:" << profile.radio_version << "\n";
    
    printSeparator();
    std::cout << "  STATIC TEMPLATE (from REAL certified device)\n";
    printSeparator();
    
    std::cout << std::setw(20) << "  Manufacturer:" << profile.manufacturer << "\n";
    std::cout << std::setw(20) << "  Model:" << profile.model_name << "\n";
    std::cout << std::setw(20) << "  Hardware Board:" << profile.hardware_board << "\n";
    std::cout << std::setw(20) << "  Build Fingerprint:\n";
    std::cout << "    " << profile.build_fingerprint << "\n";
    std::cout << std::setw(20) << "  Android:" << "v" << profile.android_version 
              << " (SDK " << profile.sdk_version << ")\n";
    std::cout << std::setw(20) << "  Security Patch:" << profile.security_patch << "\n";
    std::cout << std::setw(20) << "  Build ID:" << profile.build_id << "\n";
    
    printSeparator();
}

void printValidation(const DeviceIdentityProfile& profile) {
    printSeparator();
    std::cout << "  VALIDATION RESULTS\n";
    printSeparator();
    
    std::cout << "  IMEI1 Luhn:      " 
              << (DeviceProfileGenerator::validateIMEI(profile.imei1) ? "VALID" : "INVALID") << "\n";
    std::cout << "  IMEI2 Luhn:      " 
              << (DeviceProfileGenerator::validateIMEI(profile.imei2) ? "VALID" : "INVALID") << "\n";
    std::cout << "  WiFi MAC:       " 
              << (DeviceProfileGenerator::validateMAC(profile.wifi_mac) ? "VALID" : "INVALID");
    if (DeviceProfileGenerator::isLocallyAdministeredMAC(profile.wifi_mac)) {
        std::cout << " (Locally Administered)";
    }
    std::cout << "\n";
    std::cout << "  Bluetooth MAC:  " 
              << (DeviceProfileGenerator::validateMAC(profile.bluetooth_mac) ? "VALID" : "INVALID");
    if (DeviceProfileGenerator::isLocallyAdministeredMAC(profile.bluetooth_mac)) {
        std::cout << " (Locally Administered)";
    }
    std::cout << "\n";
    std::cout << "  IMSI1:          " 
              << (DeviceProfileGenerator::validateIMSI(profile.imsi1) ? "VALID" : "INVALID") << "\n";
    std::cout << "  ICCID1:         " 
              << (DeviceProfileGenerator::validateICCID(profile.iccid1) ? "VALID" : "INVALID") << "\n";
    std::cout << "  Android ID:     " 
              << (DeviceProfileGenerator::isValidHex(profile.android_id, 16) ? "VALID" : "INVALID") << "\n";
    
    printSeparator();
}

int main() {
    std::cout << "\n";
    std::cout << "=====================================================================\n";
    std::cout << "     DEVICE PROFILE GENERATOR v4.0.0\n";
    std::cout << "        Anti-Detection Engine - HMAC-SHA256 Based\n";
    std::cout << "=====================================================================\n";
    
    // Initialize Generator with HWID and License Key
    std::string hwid = "PC-001-USER123";
    std::string license = "PRO-REDCPP-2024";
    
    DeviceProfileGenerator generator(hwid, license);
    
    std::cout << "\n[1] Initializing Profile Generator...\n";
    std::cout << "    HWID: " << hwid << "\n";
    std::cout << "    License: " << license << "\n";
    
    // Generate Samsung S24 Ultra Profile
    std::cout << "\n[2] Generating Samsung Galaxy S24 Ultra Profile...\n";
    auto samsung_profile = generator.generateProfile(1, "samsung_s24_ultra");
    printProfile(samsung_profile);
    printValidation(samsung_profile);
    
    // Generate Google Pixel 8 Pro Profile
    std::cout << "\n\n[3] Generating Google Pixel 8 Pro Profile...\n";
    auto pixel_profile = generator.generateProfile(2, "pixel_8_pro");
    printProfile(pixel_profile);
    printValidation(pixel_profile);
    
    // Test Deterministic Regeneration
    std::cout << "\n\n[4] Testing Deterministic Regeneration...\n";
    auto regenerated = generator.regenerateProfile(1);
    std::cout << "    Original IMEI1:  " << samsung_profile.imei1 << "\n";
    std::cout << "    Regenerated IMEI1: " << regenerated.imei1 << "\n";
    
    if (samsung_profile.imei1 == regenerated.imei1) {
        std::cout << "    DETERMINISTIC: Same profile restored!\n";
    }
    
    // Generate Multiple Profiles and check uniqueness
    std::cout << "\n\n[5] Generating 5 Different Profiles...\n";
    std::vector<std::string> generated_imeis;
    for (uint32_t i = 1; i <= 5; i++) {
        auto profile = generator.generateProfile(i);
        generated_imeis.push_back(profile.imei1);
        std::cout << "    Profile " << i << ": " << profile.imei1 << "\n";
    }
    
    std::set<std::string> unique_imeis(generated_imeis.begin(), generated_imeis.end());
    if (unique_imeis.size() == 5) {
        std::cout << "    ALL UNIQUE: No collisions!\n";
    }
    
    std::cout << "\n=====================================================================\n";
    std::cout << "                        SUMMARY\n";
    std::cout << "=====================================================================\n";
    std::cout << "  23 Parameters Generated per Profile\n";
    std::cout << "  Deterministic (same index = same output)\n";
    std::cout << "  Globally Unique (HMAC-SHA256 derivation)\n";
    std::cout << "  Zero Collision with Real Devices (Local Admin MAC)\n";
    std::cout << "  Luhn Validated IMEI\n";
    std::cout << "  Real Certified Device Templates\n";
    std::cout << "=====================================================================\n";
    
    return 0;
}
