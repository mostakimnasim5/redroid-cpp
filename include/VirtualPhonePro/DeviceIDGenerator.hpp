#ifndef DEVICE_ID_GENERATOR_HPP
#define DEVICE_ID_GENERATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <random>

namespace AntiDetect {

/**
 * DeviceIDGenerator - Generates valid device identifiers
 * 
 * Creates properly formatted and validated device IDs:
 * - IMEI (15 digits with Luhn check digit)
 * - Serial Number (alphanumeric)
 * - GSF ID (Google Services Framework ID)
 * - Android ID (64-bit hex)
 * - MAC Address
 * - WiFi MAC Address
 * 
 * Features:
 * - Luhn algorithm validation for IMEI
 * - Realistic brand/model specific formats
 * - Random but valid generation
 */
class DeviceIDGenerator {
public:
    static DeviceIDGenerator& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // IMEI Generation (15 digits)
    std::string generateIMEI(const std::string& brand = "samsung");
    std::string generateValidIMEI(const std::string& baseIMEI);
    bool validateIMEI(const std::string& imei) const;
    
    // Serial Number Generation
    std::string generateSerialNumber(const std::string& brand = "samsung");
    std::string generateRandomSerial(size_t length = 10);
    
    // GSF ID Generation
    std::string generateGSFId();
    
    // Android ID Generation
    std::string generateAndroidId();
    
    // MAC Address Generation
    std::string generateMACAddress(const std::string& type = "wifi");
    std::string generateRandomMAC();
    
    // WiFi BSSID Generation
    std::string generateBSSID(const std::string& brand = "samsung");
    
    // WiFi SSID (fake networks)
    std::string generateRandomSSID();
    
    // Bluetooth Address
    std::string generateBluetoothAddress();
    
    // Device Identity Set (complete set)
    std::map<std::string, std::string> generateCompleteDeviceIdentity(
        const std::string& brand = "samsung",
        const std::string& model = "");
    
    // Profile-bound ID generation
    std::string generateProfileBoundId(const std::string& profileId);
    
    // Set brand-specific TAC prefix
    void setBrandTACPrefix(const std::string& brand, const std::string& prefix);
    
private:
    DeviceIDGenerator();
    ~DeviceIDGenerator();
    DeviceIDGenerator(const DeviceIDGenerator&) = delete;
    DeviceIDGenerator& operator=(const DeviceIDGenerator&) = delete;
    
    int calculateLuhnCheckDigit(const std::string& baseNumber) const;
    bool verifyLuhn(const std::string& number) const;
    
    std::string generateRandomDigits(size_t count);
    std::string bytesToHex(const std::vector<unsigned char>& bytes);
    
    std::string generateSamsungSerial();
    std::string generateGoogleSerial();
    std::string generateXiaomiSerial();
    std::string generateOnePlusSerial();
    std::string generateHuaweiSerial();
    
    std::string getBrandTAC(const std::string& brand);
    
    // Brand-specific prefixes
    std::map<std::string, std::string> m_tacPrefixes;
    std::map<std::string, std::string> m_serialFormats;
    
    // Random engine
    std::mt19937 m_random;
    bool m_initialized;
    
    // Brand OUIs for MAC addresses
    std::map<std::string, std::string> m_brandOUIs;
};

} // namespace AntiDetect

#endif // DEVICE_ID_GENERATOR_HPP
