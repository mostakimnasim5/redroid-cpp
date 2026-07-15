#include "IPTimezoneConverter.hpp"
#include "Logger.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace AntiDetect {

IPTimezoneConverter::IPTimezoneConverter()
    : m_initialized(false)
{
}

IPTimezoneConverter::~IPTimezoneConverter() {
}

IPTimezoneConverter& IPTimezoneConverter::getInstance() {
    static IPTimezoneConverter instance;
    return instance;
}

bool IPTimezoneConverter::initialize() {
    if (m_initialized) {
        return true;
    }
    
    Logger::getInstance().info("Initializing IP-Timezone Converter...");
    
    initializeCountryDatabase();
    initializeIPGeolocationData();
    
    m_initialized = true;
    Logger::getInstance().info("IP-Timezone Converter initialized with " +
                              std::to_string(m_countryDatabase.size()) + " countries");
    
    return true;
}

void IPTimezoneConverter::initializeCountryDatabase() {
    // Comprehensive country database with timezone, locale, and carrier info
    m_countryDatabase = {
        // Bangladesh & South Asia
        {"BD", {"Asia/Dhaka", "bn_BD", "BD", "Bangladesh", "bn", "Bengali", "470", "Grameenphone", "BDT", "৳", 23.8103f, 90.4125f, "Dhaka", "Asia"}},
        {"BD", {"Asia/Dhaka", "en_BD", "BD", "Bangladesh", "en", "English", "470", "Banglalion", "BDT", "৳", 23.8103f, 90.4125f, "Dhaka", "Asia"}},
        {"IN", {"Asia/Kolkata", "en_IN", "IN", "India", "en", "English", "404", "Airtel", "INR", "₹", 28.6139f, 77.2090f, "New Delhi", "Asia"}},
        {"IN", {"Asia/Kolkata", "hi_IN", "IN", "India", "hi", "Hindi", "404", "Jio", "INR", "₹", 28.6139f, 77.2090f, "New Delhi", "Asia"}},
        {"PK", {"Asia/Karachi", "ur_PK", "PK", "Pakistan", "ur", "Urdu", "410", "Jazz", "PKR", "₨", 33.6844f, 73.0479f, "Islamabad", "Asia"}},
        {"LK", {"Asia/Colombo", "si_LK", "LK", "Sri Lanka", "si", "Sinhala", "413", "Dialog", "LKR", "Rs", 6.9271f, 79.8612f, "Colombo", "Asia"}},
        {"NP", {"Asia/Kathmandu", "ne_NP", "NP", "Nepal", "ne", "Nepali", "429", "Ncell", "NPR", "रू", 27.7172f, 85.3240f, "Kathmandu", "Asia"}},
        
        // USA & North America
        {"US", {"America/New_York", "en_US", "US", "United States", "en", "English", "310", "AT&T", "USD", "$", 40.7128f, -74.0060f, "New York", "North America"}},
        {"US", {"America/Los_Angeles", "en_US", "US", "United States", "en", "English", "310", "T-Mobile", "USD", "$", 34.0522f, -118.2437f, "Los Angeles", "North America"}},
        {"US", {"America/Chicago", "en_US", "US", "United States", "en", "English", "310", "Verizon", "USD", "$", 41.8781f, -87.6298f, "Chicago", "North America"}},
        {"US", {"America/Phoenix", "en_US", "US", "United States", "en", "English", "310", "US Cellular", "USD", "$", 33.4484f, -112.0740f, "Phoenix", "North America"}},
        {"US", {"Pacific/Honolulu", "en_US", "US", "United States", "en", "English", "310", "Hawaiian Telcom", "USD", "$", 21.3069f, -157.8583f, "Honolulu", "North America"}},
        {"CA", {"America/Toronto", "en_CA", "CA", "Canada", "en", "English", "302", "Bell", "CAD", "$", 43.6532f, -79.3832f, "Toronto", "North America"}},
        {"CA", {"America/Vancouver", "en_CA", "CA", "Canada", "en", "English", "302", "Rogers", "CAD", "$", 49.2827f, -123.1207f, "Vancouver", "North America"}},
        {"MX", {"America/Mexico_City", "es_MX", "MX", "Mexico", "es", "Spanish", "334", "Telcel", "MXN", "$", 19.4326f, -99.1332f, "Mexico City", "North America"}},
        
        // Europe
        {"GB", {"Europe/London", "en_GB", "GB", "United Kingdom", "en", "English", "234", "EE", "GBP", "£", 51.5074f, -0.1278f, "London", "Europe"}},
        {"GB", {"Europe/London", "en_GB", "GB", "United Kingdom", "en", "English", "234", "O2", "GBP", "£", 51.5074f, -0.1278f, "London", "Europe"}},
        {"DE", {"Europe/Berlin", "de_DE", "DE", "Germany", "de", "German", "262", "Deutsche Telekom", "EUR", "€", 52.5200f, 13.4050f, "Berlin", "Europe"}},
        {"FR", {"Europe/Paris", "fr_FR", "FR", "France", "fr", "French", "208", "Orange", "EUR", "€", 48.8566f, 2.3522f, "Paris", "Europe"}},
        {"ES", {"Europe/Madrid", "es_ES", "ES", "Spain", "es", "Spanish", "214", "Movistar", "EUR", "€", 40.4168f, -3.7038f, "Madrid", "Europe"}},
        {"IT", {"Europe/Rome", "it_IT", "IT", "Italy", "it", "Italian", "222", "TIM", "EUR", "€", 41.9028f, 12.4964f, "Rome", "Europe"}},
        {"NL", {"Europe/Amsterdam", "nl_NL", "NL", "Netherlands", "nl", "Dutch", "204", "KPN", "EUR", "€", 52.3676f, 4.9041f, "Amsterdam", "Europe"}},
        {"BE", {"Europe/Brussels", "fr_BE", "BE", "Belgium", "fr", "French", "206", "Proximus", "EUR", "€", 50.8503f, 4.3517f, "Brussels", "Europe"}},
        {"CH", {"Europe/Zurich", "de_CH", "CH", "Switzerland", "de", "German", "228", "Swisscom", "CHF", "CHF", 47.3769f, 8.5417f, "Zurich", "Europe"}},
        {"AT", {"Europe/Vienna", "de_AT", "AT", "Austria", "de", "German", "232", "A1", "EUR", "€", 48.2082f, 16.3738f, "Vienna", "Europe"}},
        {"PL", {"Europe/Warsaw", "pl_PL", "PL", "Poland", "pl", "Polish", "260", "Orange", "PLN", "zł", 52.2297f, 21.0122f, "Warsaw", "Europe"}},
        {"SE", {"Europe/Stockholm", "sv_SE", "SE", "Sweden", "sv", "Swedish", "240", "Telia", "SEK", "kr", 59.3293f, 18.0686f, "Stockholm", "Europe"}},
        {"NO", {"Europe/Oslo", "nb_NO", "NO", "Norway", "nb", "Norwegian", "242", "Telenor", "NOK", "kr", 59.9139f, 10.7522f, "Oslo", "Europe"}},
        {"DK", {"Europe/Copenhagen", "da_DK", "DK", "Denmark", "da", "Danish", "238", "TDC", "DKK", "kr", 55.6761f, 12.5683f, "Copenhagen", "Europe"}},
        {"FI", {"Europe/Helsinki", "fi_FI", "FI", "Finland", "fi", "Finnish", "244", "DNA", "EUR", "€", 60.1699f, 24.9384f, "Helsinki", "Europe"}},
        {"IE", {"Europe/Dublin", "en_IE", "IE", "Ireland", "en", "English", "272", "Vodafone", "EUR", "€", 53.3498f, -6.2603f, "Dublin", "Europe"}},
        {"PT", {"Europe/Lisbon", "pt_PT", "PT", "Portugal", "pt", "Portuguese", "268", "MEO", "EUR", "€", 38.7223f, -9.1393f, "Lisbon", "Europe"}},
        {"GR", {"Europe/Athens", "el_GR", "GR", "Greece", "el", "Greek", "202", "Cosmote", "EUR", "€", 37.9838f, 23.7275f, "Athens", "Europe"}},
        {"CZ", {"Europe/Prague", "cs_CZ", "CZ", "Czech Republic", "cs", "Czech", "230", "O2", "CZK", "Kč", 50.0755f, 14.4378f, "Prague", "Europe"}},
        {"HU", {"Europe/Budapest", "hu_HU", "HU", "Hungary", "hu", "Hungarian", "216", "Magyar Telekom", "HUF", "Ft", 47.4979f, 19.0402f, "Budapest", "Europe"}},
        {"RO", {"Europe/Bucharest", "ro_RO", "RO", "Romania", "ro", "Romanian", "226", "Orange", "RON", "lei", 44.4268f, 26.1025f, "Bucharest", "Europe"}},
        {"BG", {"Europe/Sofia", "bg_BG", "BG", "Bulgaria", "bg", "Bulgarian", "284", "Vivacom", "BGN", "лв", 42.6977f, 23.3219f, "Sofia", "Europe"}},
        {"HR", {"Europe/Zagreb", "hr_HR", "HR", "Croatia", "hr", "Croatian", "219", "T-Hrvatski", "EUR", "€", 45.8150f, 15.9819f, "Zagreb", "Europe"}},
        {"UA", {"Europe/Kiev", "uk_UA", "UA", "Ukraine", "uk", "Ukrainian", "255", "Kyivstar", "UAH", "₴", 50.4501f, 30.5234f, "Kiev", "Europe"}},
        {"RU", {"Europe/Moscow", "ru_RU", "RU", "Russia", "ru", "Russian", "250", "Beeline", "RUB", "₽", 55.7558f, 37.6173f, "Moscow", "Europe"}},
        {"TR", {"Europe/Istanbul", "tr_TR", "TR", "Turkey", "tr", "Turkish", "286", "Turkcell", "TRY", "₺", 41.0082f, 28.9784f, "Istanbul", "Europe"}},
        
        // Asia Pacific
        {"JP", {"Asia/Tokyo", "ja_JP", "JP", "Japan", "ja", "Japanese", "440", "NTT Docomo", "JPY", "¥", 35.6762f, 139.6503f, "Tokyo", "Asia"}},
        {"CN", {"Asia/Shanghai", "zh_CN", "CN", "China", "zh", "Chinese", "460", "China Mobile", "CNY", "¥", 31.2304f, 121.4737f, "Shanghai", "Asia"}},
        {"CN", {"Asia/Hong_Kong", "zh_HK", "CN", "Hong Kong", "zh", "Cantonese", "460", "CSL", "HKD", "HK$", 22.3193f, 114.1694f, "Hong Kong", "Asia"}},
        {"TW", {"Asia/Taipei", "zh_TW", "TW", "Taiwan", "zh", "Mandarin", "466", "Chunghwa", "TWD", "NT$", 25.0330f, 121.5654f, "Taipei", "Asia"}},
        {"KR", {"Asia/Seoul", "ko_KR", "KR", "South Korea", "ko", "Korean", "450", "SK Telecom", "KRW", "₩", 37.5665f, 126.9780f, "Seoul", "Asia"}},
        {"SG", {"Asia/Singapore", "en_SG", "SG", "Singapore", "en", "English", "525", "Singtel", "SGD", "S$", 1.3521f, 103.8198f, "Singapore", "Asia"}},
        {"MY", {"Asia/Kuala_Lumpur", "ms_MY", "MY", "Malaysia", "ms", "Malay", "502", "Maxis", "MYR", "RM", 3.1390f, 101.6869f, "Kuala Lumpur", "Asia"}},
        {"TH", {"Asia/Bangkok", "th_TH", "TH", "Thailand", "th", "Thai", "520", "AIS", "THB", "฿", 13.7563f, 100.5018f, "Bangkok", "Asia"}},
        {"VN", {"Asia/Ho_Chi_Minh", "vi_VN", "VN", "Vietnam", "vi", "Vietnamese", "452", "Viettel", "VND", "₫", 10.8231f, 106.6297f, "Ho Chi Minh", "Asia"}},
        {"ID", {"Asia/Jakarta", "id_ID", "ID", "Indonesia", "id", "Indonesian", "510", "Telkomsel", "IDR", "Rp", -6.2088f, 106.8456f, "Jakarta", "Asia"}},
        {"PH", {"Asia/Manila", "fil_PH", "PH", "Philippines", "fil", "Filipino", "515", "Globe", "PHP", "₱", 14.5995f, 120.9842f, "Manila", "Asia"}},
        {"AU", {"Australia/Sydney", "en_AU", "AU", "Australia", "en", "English", "505", "Telstra", "AUD", "$", -33.8688f, 151.2093f, "Sydney", "Oceania"}},
        {"AU", {"Australia/Perth", "en_AU", "AU", "Australia", "en", "English", "505", "Optus", "AUD", "$", -31.9505f, 115.8605f, "Perth", "Oceania"}},
        {"NZ", {"Pacific/Auckland", "en_NZ", "NZ", "New Zealand", "en", "English", "530", "Spark", "NZD", "$", -36.8509f, 174.7645f, "Auckland", "Oceania"}},
        
        // Middle East
        {"AE", {"Asia/Dubai", "ar_AE", "AE", "UAE", "ar", "Arabic", "424", "Etisalat", "AED", "د.إ", 25.2048f, 55.2708f, "Dubai", "Middle East"}},
        {"SA", {"Asia/Riyadh", "ar_SA", "SA", "Saudi Arabia", "ar", "Arabic", "420", "STC", "SAR", "﷼", 23.8859f, 45.0792f, "Riyadh", "Middle East"}},
        {"IL", {"Asia/Jerusalem", "he_IL", "IL", "Israel", "he", "Hebrew", "425", "Partner", "ILS", "₪", 31.7683f, 35.2137f, "Jerusalem", "Middle East"}},
        {"EG", {"Africa/Cairo", "ar_EG", "EG", "Egypt", "ar", "Arabic", "602", "Vodafone", "EGP", "£", 30.0444f, 31.2357f, "Cairo", "Africa"}},
        {"ZA", {"Africa/Johannesburg", "en_ZA", "ZA", "South Africa", "en", "English", "655", "Vodacom", "ZAR", "R", -26.2041f, 28.0473f, "Johannesburg", "Africa"}},
        {"NG", {"Africa/Lagos", "en_NG", "NG", "Nigeria", "en", "English", "621", "MTN", "NGN", "₦", 6.5244f, 3.3792f, "Lagos", "Africa"}},
        {"KE", {"Africa/Nairobi", "sw_KE", "KE", "Kenya", "sw", "Swahili", "639", "Safaricom", "KES", "KSh", -1.2921f, 36.8219f, "Nairobi", "Africa"}},
        {"BR", {"America/Sao_Paulo", "pt_BR", "BR", "Brazil", "pt", "Portuguese", "724", "Vivo", "BRL", "R$", -23.5505f, -46.6333f, "Sao Paulo", "South America"}},
        {"AR", {"America/Argentina/Buenos_Aires", "es_AR", "AR", "Argentina", "es", "Spanish", "722", "Movistar", "ARS", "$", -34.6037f, -58.3816f, "Buenos Aires", "South America"}},
        {"CL", {"America/Santiago", "es_CL", "CL", "Chile", "es", "Spanish", "730", "Entel", "CLP", "$", -33.4489f, -70.6693f, "Santiago", "South America"}},
        {"CO", {"America/Bogota", "es_CO", "CO", "Colombia", "es", "Spanish", "732", "Claro", "COP", "$", 4.7110f, -74.0721f, "Bogota", "South America"}},
        {"PE", {"America/Lima", "es_PE", "PE", "Peru", "es", "Spanish", "716", "Movistar", "PEN", "S/", -12.0464f, -77.0428f, "Lima", "South America"}},
        
        // More countries
        {"GB", {"Europe/London", "cy_GB", "GB", "United Kingdom", "cy", "Welsh", "234", "Three", "GBP", "£", 53.4808f, -2.2426f, "Manchester", "Europe"}},
        {"US", {"America/Denver", "en_US", "US", "United States", "en", "English", "310", "Sprint", "USD", "$", 39.7392f, -104.9903f, "Denver", "North America"}},
        {"US", {"America/Miami", "en_US", "US", "United States", "en", "English", "310", "Cricket", "USD", "$", 25.7617f, -80.1918f, "Miami", "North America"}},
        {"CA", {"America/Montreal", "fr_CA", "CA", "Canada", "fr", "French", "302", "Fido", "CAD", "$", 45.5017f, -73.5673f, "Montreal", "North America"}},
    };
}

void IPTimezoneConverter::initializeIPGeolocationData() {
    // IP range to country mapping (simplified for demonstration)
    // In production, use a proper IP geolocation database
    
    m_ipRanges = {
        {"1.0.0.0", "AU"},      // APNIC
        {"2.0.0.0", "FR"},      // RIPE
        {"3.0.0.0", "US"},      // Amazon/AWS
        {"5.0.0.0", "EU"},      // RIPE
        {"8.0.0.0", "US"},      // Level3
        {"13.0.0.0", "US"},     // Apple
        {"14.0.0.0", "IN"},     // APNIC
        {"17.0.0.0", "US"},     // Apple
        {"23.0.0.0", "US"},     // ARIN
        {"31.0.0.0", "EU"},     // RIPE
        {"34.0.0.0", "US"},     // Google
        {"35.0.0.0", "US"},     // ARIN
        {"36.0.0.0", "AU"},     // APNIC
        {"37.0.0.0", "EU"},     // RIPE
        {"39.0.0.0", "CN"},     // APNIC
        {"40.0.0.0", "US"},     // AT&T
        {"41.0.0.0", "AF"},     // APNIC
        {"42.0.0.0", "JP"},     // APNIC
        {"43.0.0.0", "JP"},     // APNIC
        {"45.0.0.0", "US"},     // ARIN
        {"46.0.0.0", "EU"},     // RIPE
        {"47.0.0.0", "US"},     // ARIN
        {"48.0.0.0", "US"},     // ARIN
        {"49.0.0.0", "AU"},     // APNIC
        {"50.0.0.0", "US"},     // ARIN
        {"51.0.0.0", "EU"},     // RIPE
        {"52.0.0.0", "US"},     // Amazon
        {"54.0.0.0", "US"},     // Amazon
        {"57.0.0.0", "AU"},     // APNIC
        {"58.0.0.0", "AU"},     // APNIC
        {"59.0.0.0", "AU"},     // APNIC
        {"60.0.0.0", "AU"},     // APNIC
        {"61.0.0.0", "AU"},     // APNIC
        {"62.0.0.0", "EU"},     // RIPE
        {"63.0.0.0", "US"},     // ARIN
        {"64.0.0.0", "US"},     // ARIN
        {"65.0.0.0", "US"},     // ARIN
        {"66.0.0.0", "US"},     // ARIN
        {"67.0.0.0", "US"},     // ARIN
        {"68.0.0.0", "US"},     // ARIN
        {"69.0.0.0", "US"},     // ARIN
        {"70.0.0.0", "US"},     // ARIN
        {"71.0.0.0", "US"},     // ARIN
        {"72.0.0.0", "US"},     // ARIN
        {"73.0.0.0", "US"},     // ARIN
        {"74.0.0.0", "US"},     // ARIN
        {"75.0.0.0", "US"},     // ARIN
        {"76.0.0.0", "US"},     // ARIN
        {"77.0.0.0", "EU"},     // RIPE
        {"78.0.0.0", "EU"},     // RIPE
        {"79.0.0.0", "EU"},     // RIPE
        {"80.0.0.0", "EU"},     // RIPE
        {"81.0.0.0", "EU"},     // RIPE
        {"82.0.0.0", "EU"},     // RIPE
        {"83.0.0.0", "EU"},     // RIPE
        {"84.0.0.0", "EU"},     // RIPE
        {"85.0.0.0", "EU"},     // RIPE
        {"86.0.0.0", "EU"},     // RIPE
        {"87.0.0.0", "EU"},     // RIPE
        {"88.0.0.0", "EU"},     // RIPE
        {"89.0.0.0", "EU"},     // RIPE
        {"90.0.0.0", "EU"},     // RIPE
        {"91.0.0.0", "EU"},     // RIPE
        {"92.0.0.0", "EU"},     // RIPE
        {"93.0.0.0", "EU"},     // RIPE
        {"94.0.0.0", "EU"},     // RIPE
        {"95.0.0.0", "EU"},     // RIPE
        {"96.0.0.0", "US"},     // ARIN
        {"97.0.0.0", "US"},     // ARIN
        {"98.0.0.0", "US"},     // ARIN
        {"99.0.0.0", "US"},     // ARIN
        {"100.0.0.0", "US"},    // ARIN
        {"101.0.0.0", "AU"},    // APNIC
        {"102.0.0.0", "AF"},    // AFRINIC
        {"103.0.0.0", "AU"},    // APNIC
        {"104.0.0.0", "US"},    // ARIN
        {"105.0.0.0", "AF"},    // AFRINIC
        {"106.0.0.0", "AU"},    // APNIC
        {"107.0.0.0", "US"},    // ARIN
        {"108.0.0.0", "US"},    // ARIN
        {"109.0.0.0", "EU"},    // RIPE
        {"110.0.0.0", "AU"},    // APNIC
        {"111.0.0.0", "AU"},    // APNIC
        {"112.0.0.0", "AU"},    // APNIC
        {"113.0.0.0", "AU"},    // APNIC
        {"114.0.0.0", "AU"},    // APNIC
        {"115.0.0.0", "AU"},    // APNIC
        {"116.0.0.0", "AU"},    // APNIC
        {"117.0.0.0", "AU"},    // APNIC
        {"118.0.0.0", "AU"},    // APNIC
        {"119.0.0.0", "AU"},    // APNIC
        {"120.0.0.0", "AU"},    // APNIC
        {"121.0.0.0", "AU"},    // APNIC
        {"122.0.0.0", "AU"},    // APNIC
        {"123.0.0.0", "AU"},    // APNIC
        {"124.0.0.0", "AU"},    // APNIC
        {"125.0.0.0", "AU"},    // APNIC
        {"126.0.0.0", "AU"},    // APNIC
        {"128.0.0.0", "US"},    // ARIN
        {"129.0.0.0", "US"},    // ARIN
        {"130.0.0.0", "US"},    // ARIN
        {"131.0.0.0", "US"},    // ARIN
        {"132.0.0.0", "US"},    // ARIN
        {"133.0.0.0", "JP"},    // APNIC
        {"134.0.0.0", "JP"},    // APNIC
        {"135.0.0.0", "US"},    // APNIC
        {"136.0.0.0", "US"},    // ARIN
        {"137.0.0.0", "US"},    // ARIN
        {"138.0.0.0", "US"},    // ARIN
        {"139.0.0.0", "IN"},    // APNIC
        {"140.0.0.0", "US"},    // ARIN
        {"141.0.0.0", "EU"},    // RIPE
        {"142.0.0.0", "US"},    // ARIN
        {"143.0.0.0", "EU"},    // RIPE
        {"144.0.0.0", "US"},    // ARIN
        {"145.0.0.0", "EU"},    // RIPE
        {"146.0.0.0", "EU"},    // RIPE
        {"147.0.0.0", "US"},    // ARIN
        {"148.0.0.0", "EU"},    // RIPE
        {"149.0.0.0", "US"},    // ARIN
        {"150.0.0.0", "AU"},    // APNIC
        {"151.0.0.0", "EU"},    // RIPE
        {"152.0.0.0", "US"},    // ARIN
        {"153.0.0.0", "AU"},    // APNIC
        {"154.0.0.0", "AF"},    // AFRINIC
        {"155.0.0.0", "US"},    // ARIN
        {"156.0.0.0", "EU"},    // RIPE
        {"157.0.0.0", "US"},    // ARIN
        {"158.0.0.0", "US"},    // ARIN
        {"159.0.0.0", "US"},    // ARIN
        {"160.0.0.0", "US"},    // ARIN
        {"161.0.0.0", "EU"},    // RIPE
        {"162.0.0.0", "US"},    // ARIN
        {"163.0.0.0", "AU"},    // APNIC
        {"164.0.0.0", "US"},    // ARIN
        {"165.0.0.0", "US"},    // ARIN
        {"166.0.0.0", "US"},    // ARIN
        {"167.0.0.0", "US"},    // ARIN
        {"168.0.0.0", "US"},    // ARIN
        {"169.0.0.0", "US"},    // ARIN
        {"170.0.0.0", "US"},    // ARIN
        {"171.0.0.0", "IN"},    // APNIC
        {"172.0.0.0", "US"},    // ARIN
        {"173.0.0.0", "US"},    // ARIN
        {"174.0.0.0", "US"},    // ARIN
        {"175.0.0.0", "AU"},    // APNIC
        {"176.0.0.0", "EU"},    // RIPE
        {"177.0.0.0", "BR"},    // LACNIC
        {"178.0.0.0", "EU"},    // RIPE
        {"179.0.0.0", "MX"},    // LACNIC
        {"180.0.0.0", "AU"},    // APNIC
        {"181.0.0.0", "CL"},    // LACNIC
        {"182.0.0.0", "AU"},    // APNIC
        {"183.0.0.0", "AU"},    // APNIC
        {"184.0.0.0", "US"},    // ARIN
        {"185.0.0.0", "EU"},    // RIPE
        {"186.0.0.0", "BR"},    // LACNIC
        {"187.0.0.0", "MX"},    // LACNIC
        {"188.0.0.0", "EU"},    // RIPE
        {"189.0.0.0", "BR"},    // LACNIC
        {"190.0.0.0", "MX"},    // LACNIC
        {"191.0.0.0", "BR"},    // LACNIC
        {"192.0.0.0", "US"},    // ARIN
        {"193.0.0.0", "EU"},    // RIPE
        {"194.0.0.0", "EU"},    // RIPE
        {"195.0.0.0", "EU"},    // RIPE
        {"196.0.0.0", "AF"},    // AFRINIC
        {"197.0.0.0", "AF"},    // AFRINIC
        {"198.0.0.0", "US"},    // ARIN
        {"199.0.0.0", "US"},    // ARIN
        {"200.0.0.0", "BR"},    // LACNIC
        {"201.0.0.0", "MX"},    // LACNIC
        {"202.0.0.0", "AU"},    // APNIC
        {"203.0.0.0", "AU"},    // APNIC
        {"204.0.0.0", "US"},    // ARIN
        {"205.0.0.0", "US"},    // ARIN
        {"206.0.0.0", "US"},    // ARIN
        {"207.0.0.0", "US"},    // ARIN
        {"208.0.0.0", "US"},    // ARIN
        {"209.0.0.0", "US"},    // ARIN
        {"210.0.0.0", "AU"},    // APNIC
        {"211.0.0.0", "KR"},    // APNIC
        {"212.0.0.0", "EU"},    // RIPE
        {"213.0.0.0", "EU"},    // RIPE
        {"216.0.0.0", "US"},    // ARIN
        {"217.0.0.0", "EU"},    // RIPE
        {"218.0.0.0", "CN"},    // APNIC
        {"219.0.0.0", "JP"},    // APNIC
        {"220.0.0.0", "JP"},    // APNIC
        {"221.0.0.0", "JP"},    // APNIC
        {"222.0.0.0", "JP"},    // APNIC
        {"223.0.0.0", "JP"},    // APNIC
    };
}

IPTimezoneConverter::LocaleInfo IPTimezoneConverter::getLocaleFromIP(const std::string& ipAddress) {
    if (!m_initialized) {
        initialize();
    }
    
    std::string countryCode = extractCountryFromIP(ipAddress);
    return getLocaleByCountryCode(countryCode);
}

std::string IPTimezoneConverter::extractCountryFromIP(const std::string& ip) {
    if (!isValidIPv4(ip)) {
        return "US"; // Default to US for invalid IPs
    }
    
    uint32_t ipNum = ipToNumber(ip);
    
    // Find matching range
    for (const auto& range : m_ipRanges) {
        uint32_t rangeStart = ipToNumber(range.first);
        if (ipNum >= rangeStart) {
            return range.second;
        }
    }
    
    return "US"; // Default
}

std::string IPTimezoneConverter::getTimezoneFromIP(const std::string& ipAddress) {
    LocaleInfo info = getLocaleFromIP(ipAddress);
    return info.timezone;
}

IPTimezoneConverter::LocaleInfo IPTimezoneConverter::getLocaleByCountryCode(const std::string& countryCode) {
    auto it = m_countryDatabase.find(countryCode);
    if (it != m_countryDatabase.end()) {
        return it->second;
    }
    
    // Return default US locale
    return m_countryDatabase.at("US");
}

IPTimezoneConverter::LocaleInfo IPTimezoneConverter::getLocaleFromCountry(const std::string& countryCode) {
    return getLocaleByCountryCode(countryCode);
}

std::map<std::string, std::string> IPTimezoneConverter::getRegionSettings(const std::string& region) {
    std::map<std::string, std::string> settings;
    
    if (region == "US" || region == "North America") {
        settings["timezone"] = "America/New_York";
        settings["locale"] = "en_US";
        settings["language"] = "en";
        settings["currency"] = "USD";
        settings["mcc"] = "310";
    } else if (region == "Europe") {
        settings["timezone"] = "Europe/London";
        settings["locale"] = "en_GB";
        settings["language"] = "en";
        settings["currency"] = "EUR";
        settings["mcc"] = "234";
    } else if (region == "Asia") {
        settings["timezone"] = "Asia/Singapore";
        settings["locale"] = "en_SG";
        settings["language"] = "en";
        settings["currency"] = "SGD";
        settings["mcc"] = "525";
    }
    
    return settings;
}

std::vector<std::string> IPTimezoneConverter::getSupportedRegions() {
    std::vector<std::string> regions;
    
    for (const auto& pair : m_countryDatabase) {
        if (std::find(regions.begin(), regions.end(), pair.second.region) == regions.end()) {
            regions.push_back(pair.second.region);
        }
    }
    
    return regions;
}

bool IPTimezoneConverter::isValidIPv4(const std::string& ip) const {
    int dots = 0;
    int num = 0;
    
    for (char c : ip) {
        if (c == '.') {
            if (num > 255) return false;
            dots++;
            num = 0;
        } else if (isdigit(c)) {
            num = num * 10 + (c - '0');
            if (num > 255) return false;
        } else {
            return false;
        }
    }
    
    return dots == 3;
}

bool IPTimezoneConverter::isPrivateIP(const std::string& ip) const {
    if (ip.rfind("10.", 0) == 0) return true;
    if (ip.rfind("172.16.", 0) == 0) return true;
    if (ip.rfind("192.168.", 0) == 0) return true;
    if (ip.rfind("127.", 0) == 0) return true;
    if (ip == "localhost") return true;
    
    return false;
}

uint32_t IPTimezoneConverter::ipToNumber(const std::string& ip) const {
    uint32_t result = 0;
    int octet = 0;
    uint32_t multiplier = 1;
    
    for (int i = static_cast<int>(ip.length()) - 1; i >= 0; i--) {
        if (ip[i] == '.') {
            result += octet * multiplier;
            multiplier *= 256;
            octet = 0;
        } else {
            octet = octet * 10 + (ip[i] - '0');
        }
    }
    
    result += octet * multiplier;
    return result;
}

} // namespace AntiDetect
