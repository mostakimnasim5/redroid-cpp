#ifndef IP_TIMEZONE_CONVERTER_HPP
#define IP_TIMEZONE_CONVERTER_HPP

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace VirtualPhonePro {

/**
 * IPTimezoneConverter - IP Address to Timezone/Locale Mapping
 * 
 * Converts IP addresses to appropriate timezone, locale, and region settings.
 * Uses IP geolocation data to determine:
 * - Timezone (e.g., "America/New_York")
 * - Locale (e.g., "en_US")
 * - Country code (e.g., "US")
 * - MCC (Mobile Country Code)
 * - Language
 * 
 * This ensures that device profiles match the IP location to avoid
 * detection by services checking for IP/timezone mismatches.
 */
class IPTimezoneConverter {
public:
    static IPTimezoneConverter& getInstance();
    
    // Initialize
    bool initialize();
    
    // Convert IP to timezone info
    struct LocaleInfo {
        std::string timezone;
        std::string locale;           // en_US, bn_BD, etc.
        std::string countryCode;      // US, BD, etc.
        std::string countryName;      // United States, Bangladesh
        std::string language;         // en, bn, etc.
        std::string languageName;     // English, Bengali
        std::string mcc;             // Mobile Country Code
        std::string carrier;          // Carrier name
        std::string currency;         // USD, BDT
        std::string currencySymbol;   // $, ৳
        float latitude;
        float longitude;
        std::string city;
        std::string region;
    };
    
    LocaleInfo getLocaleFromIP(const std::string& ipAddress);
    LocaleInfo getLocaleFromCountry(const std::string& countryCode);
    
    // Get timezone from IP (simple extraction)
    std::string getTimezoneFromIP(const std::string& ipAddress);
    
    // Region presets
    std::map<std::string, std::string> getRegionSettings(const std::string& region);
    
    // Get all supported regions
    std::vector<std::string> getSupportedRegions();
    
    // IP validation
    bool isValidIPv4(const std::string& ip) const;
    bool isPrivateIP(const std::string& ip) const;
    
    // Get locale info by country code
    LocaleInfo getLocaleByCountryCode(const std::string& countryCode);
    
private:
    IPTimezoneConverter();
    ~IPTimezoneConverter();
    IPTimezoneConverter(const IPTimezoneConverter&) = delete;
    IPTimezoneConverter& operator=(const IPTimezoneConverter&) = delete;
    
    void initializeCountryDatabase();
    void initializeIPGeolocationData();
    
    // Country database: country code -> locale info
    std::map<std::string, LocaleInfo> m_countryDatabase;
    
    // IP range to country mapping (simplified)
    std::vector<std::pair<std::string, std::string>> m_ipRanges; // startIP, countryCode
    
    bool m_initialized;
    
    // Helper methods
    std::string extractCountryFromIP(const std::string& ip);
    uint32_t ipToNumber(const std::string& ip) const;
};

} // namespace VirtualPhonePro

#endif // IP_TIMEZONE_CONVERTER_HPP
