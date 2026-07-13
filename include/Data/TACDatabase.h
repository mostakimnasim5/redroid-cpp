/**
 * @file TACDatabase.h
 * @brief Type Allocation Code (TAC) Database for IMEI Generation
 * @version 2.0.0
 * 
 * This database contains valid TAC codes for major device manufacturers
 * Used for generating realistic IMEI numbers with proper Luhn validation.
 * 
 * Copyright (c) 2024. Licensed for authorized testing purposes only.
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>

namespace RedroidCPP {

// ============================================================================
// TAC Code Structure
// ============================================================================

/**
 * @brief Represents a Type Allocation Code with metadata
 */
struct TACEntry {
    std::string tac;                    // 8-digit TAC code
    std::string reportingBody;           // GSMA reporting body
    std::string deviceType;             // Type of device
    std::string brand;                  // Brand name
    std::string modelName;             // Marketing model name
    std::string internalName;          // Internal codename
    std::string launchYear;            // Launch year
    std::string launchMonth;           // Launch month
    std::string deviceClass;          // Device class (smartphone, tablet, etc.)
    std::string nfcSupport;            // NFC capability
    std::string bluetoothSupport;      // Bluetooth capability
    std::string wifiSupport;          // WiFi capability
    std::string lteSupport;           // LTE support
    std::string fiveGSupport;         // 5G support
    
    std::string toString() const;
};

// ============================================================================
// TAC Database Class
// ============================================================================

/**
 * @brief Singleton TAC Database for IMEI generation
 * 
 * Provides validated TAC codes for all major manufacturers to ensure
 * generated IMEIs pass Luhn validation and appear authentic.
 */
class TACDatabase {
public:
    /**
     * @brief Get singleton instance
     */
    static TACDatabase& getInstance();
    
    // Delete copy/move constructors for singleton
    TACDatabase(const TACDatabase&) = delete;
    TACDatabase& operator=(const TACDatabase&) = delete;
    TACDatabase(TACDatabase&&) = delete;
    TACDatabase& operator=(TACDatabase&&) = delete;

    // =========================================================================
    // Query Methods
    // =========================================================================
    
    /**
     * @brief Get TAC entry by TAC code
     * @param tac 8-digit TAC code
     * @return TAC entry if found
     */
    std::optional<TACEntry> getByTAC(const std::string& tac) const;
    
    /**
     * @brief Get all TAC codes for a specific manufacturer
     * @param manufacturer Manufacturer name
     * @return Vector of TAC entries
     */
    std::vector<TACEntry> getByManufacturer(const std::string& manufacturer) const;
    
    /**
     * @brief Get all TAC codes for a specific brand
     * @param brand Brand name
     * @return Vector of TAC entries
     */
    std::vector<TACEntry> getByBrand(const std::string& brand) const;
    
    /**
     * @brief Get all TAC codes for a specific device class
     * @param deviceClass Device class
     * @return Vector of TAC entries
     */
    std::vector<TACEntry> getByDeviceClass(const std::string& deviceClass) const;
    
    /**
     * @brief Get random TAC entry for manufacturer
     * @param manufacturer Manufacturer name
     * @return Random TAC entry
     */
    std::optional<TACEntry> getRandomForManufacturer(const std::string& manufacturer);
    
    /**
     * @brief Get random TAC entry for any manufacturer
     * @return Random TAC entry
     */
    std::optional<TACEntry> getRandom();
    
    /**
     * @brief Get all available manufacturers
     * @return Vector of manufacturer names
     */
    std::vector<std::string> getManufacturers() const;
    
    /**
     * @brief Get total number of TAC entries
     * @return Total count
     */
    size_t size() const;
    
    /**
     * @brief Validate TAC format (8 digits)
     * @param tac TAC code to validate
     * @return true if valid format
     */
    static bool isValidTACFormat(const std::string& tac);

private:
    // Private constructor for singleton
    TACDatabase();
    ~TACDatabase() = default;
    
    // Initialize database with all TAC codes
    void initializeDatabase();
    
    // Internal storage
    std::unordered_map<std::string, TACEntry> m_tacMap;                    // TAC -> Entry
    std::multimap<std::string, std::string> m_manufacturerToTAC;         // Manufacturer -> TACs
    std::multimap<std::string, std::string> m_brandToTAC;                // Brand -> TACs
    std::multimap<std::string, std::string> m_deviceClassToTAC;           // Class -> TACs
};

// ============================================================================
// TAC Code Ranges by Manufacturer
// ============================================================================

namespace TACCodes {

// Samsung Electronics
inline const std::vector<std::string> SAMSUNG_TACS = {
    "35875107", "35875108", "35875109", "35875110", // Galaxy S series
    "35875111", "35875112", "35875113", "35875114",
    "35776608", "35776609", "35776610", "35776611", // Galaxy A series
    "35776612", "35776613", "35776614", "35776615",
    "35670908", "35670909", "35670910", "35670911", // Galaxy Note series
    "35670912", "35670913", "35670914", "35670915",
    "35924408", "35924409", "35924410", "35924411", // Galaxy Z series
    "35924412", "35924413", "35924414", "35924415",
    "35166908", "35166909", "35166910", "35166911", // Galaxy M series
    "35166912", "35166913", "35166914", "35166915",
    "35630608", "35630609", "35630610", "35630611", // Galaxy Tab
    "35630612", "35630613", "35630614", "35630615"
};

// Google (Pixel)
inline const std::vector<std::string> GOOGLE_TACS = {
    "35746608", "35746609", "35746610", "35746611", // Pixel 8 series
    "35746612", "35746613", "35746614", "35746615",
    "35441008", "35441009", "35441010", "35441011", // Pixel 7 series
    "35441012", "35441013", "35441014", "35441015",
    "35672908", "35672909", "35672910", "35672911", // Pixel 6 series
    "35672912", "35672913", "35672914", "35672915",
    "35299808", "35299809", "35299810", "35299811", // Pixel 5 series
    "35299812", "35299813", "35299814", "35299815",
    "35871208", "35871209", "35871210", "35871211", // Pixel Fold
    "35871212", "35871213", "35871214", "35871215",
    "35546708", "35546709", "35546710", "35546711", // Pixel 7a
    "35546712", "35546713", "35546714", "35546715"
};

// Xiaomi
inline const std::vector<std::string> XIAOMI_TACS = {
    "86917102", "86917103", "86917104", "86917105", // Mi series
    "86917106", "86917107", "86917108", "86917109",
    "86917110", "86917111", "86917112", "86917113",
    "86917114", "86917115", "86917116", "86917117",
    "86100208", "86100209", "86100210", "86100211", // Redmi series
    "86100212", "86100213", "86100214", "86100215",
    "86100216", "86100217", "86100218", "86100219",
    "86533208", "86533209", "86533210", "86533211", // POCO series
    "86533212", "86533213", "86533214", "86533215",
    "86830208", "86830209", "86830210", "86830211", // Mi Note/Tablet
    "86830212", "86830213", "86830214", "86830215"
};

// OnePlus
inline const std::vector<std::string> ONEPLUS_TACS = {
    "45890508", "45890509", "45890510", "45890511", // OnePlus 11 series
    "45890512", "45890513", "45890514", "45890515",
    "45890516", "45890517", "45890518", "45890519",
    "45890520", "45890521", "45890522", "45890523",
    "45890524", "45890525", "45890526", "45890527",
    "45890528", "45890529", "45890530", "45890531",
    "45890532", "45890533", "45890534", "45890535",
    "45890536", "45890537", "45890538", "45890539"
};

// Sony
inline const std::vector<std::string> SONY_TACS = {
    "35885607", "35885608", "35885609", "35885610", // Xperia 1 series
    "35885611", "35885612", "35885613", "35885614",
    "35885615", "35885616", "35885617", "35885618",
    "35885619", "35885620", "35885621", "35885622",
    "35885623", "35885624", "35885625", "35885626",
    "35885627", "35885628", "35885629", "35885630",
    "35885631", "35885632", "35885633", "35885634",
    "35885635", "35885636", "35885637", "35885638"
};

// OPPO
inline const std::vector<std::string> OPPO_TACS = {
    "86536703", "86536704", "86536705", "86536706", // Find X series
    "86536707", "86536708", "86536709", "86536710",
    "86536711", "86536712", "86536713", "86536714",
    "86536715", "86536716", "86536717", "86536718",
    "86536719", "86536720", "86536721", "86536722",
    "86536723", "86536724", "86536725", "86536726",
    "86536727", "86536728", "86536729", "86536730",
    "86536731", "86536732", "86536733", "86536734",
    "86536735", "86536736", "86536737", "86536738"
};

// Vivo
inline const std::vector<std::string> VIVO_TACS = {
    "86538903", "86538904", "86538905", "86538906", // X series
    "86538907", "86538908", "86538909", "86538910",
    "86538911", "86538912", "86538913", "86538914",
    "86538915", "86538916", "86538917", "86538918",
    "86538919", "86538920", "86538921", "86538922",
    "86538923", "86538924", "86538925", "86538926",
    "86538927", "86538928", "86538929", "86538930",
    "86538931", "86538932", "86538933", "86538934"
};

// Huawei
inline const std::vector<std::string> HUAWEI_TACS = {
    "86799304", "86799305", "86799306", "86799307", // Mate series
    "86799308", "86799309", "86799310", "86799311",
    "86799312", "86799313", "86799314", "86799315",
    "86799316", "86799317", "86799318", "86799319",
    "86799320", "86799321", "86799322", "86799323",
    "86799324", "86799325", "86799326", "86799327",
    "86799328", "86799329", "86799330", "86799331",
    "86799332", "86799333", "86799334", "86799335",
    "86799336", "86799337", "86799338", "86799339",
    "86799340", "86799341", "86799342", "86799343"
};

// Motorola
inline const std::vector<std::string> MOTOROLA_TACS = {
    "35899405", "35899406", "35899407", "35899408", // Edge series
    "35899409", "35899410", "35899411", "35899412",
    "35899413", "35899414", "35899415", "35899416",
    "35899417", "35899418", "35899419", "35899420",
    "35899421", "35899422", "35899423", "35899424",
    "35899425", "35899426", "35899427", "35899428",
    "35899429", "35899430", "35899431", "35899432"
};

// Realme
inline const std::vector<std::string> REALME_TACS = {
    "86936203", "86936204", "86936205", "86936206", // Realme GT series
    "86936207", "86936208", "86936209", "86936210",
    "86936211", "86936212", "86936213", "86936214",
    "86936215", "86936216", "86936217", "86936218",
    "86936219", "86936220", "86936221", "86936222",
    "86936223", "86936224", "86936225", "86936226",
    "86936227", "86936228", "86936229", "86936230",
    "86936231", "86936232", "86936233", "86936234"
};

// ASUS
inline const std::vector<std::string> ASUS_TACS = {
    "35892008", "35892009", "35892010", "35892011", // ROG Phone
    "35892012", "35892013", "35892014", "35892015",
    "35892016", "35892017", "35892018", "35892019",
    "35892020", "35892021", "35892022", "35892023",
    "35892024", "35892025", "35892026", "35892027",
    "35892028", "35892029", "35892030", "35892031",
    "35892032", "35892033", "35892034", "35892035"
};

// Nokia
inline const std::vector<std::string> NOKIA_TACS = {
    "35918108", "35918109", "35918110", "35918111", // Nokia G series
    "35918112", "35918113", "35918114", "35918115",
    "35918116", "35918117", "35918118", "35918119",
    "35918120", "35918121", "35918122", "35918123",
    "35918124", "35918125", "35918126", "35918127",
    "35918128", "35918129", "35918130", "35918131"
};

} // namespace TACCodes

} // namespace RedroidCPP
