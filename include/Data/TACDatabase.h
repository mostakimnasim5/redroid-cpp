/**
 * @file TACDatabase.h
 * @brief Type Allocation Code (TAC) Database for IMEI Generation
 * @version 3.0.0
 * 
 * This database contains valid TAC codes for major device manufacturers
 * Used for generating realistic IMEI numbers with proper Luhn validation.
 * 
 * Features:
 * - 1000+ TAC entries covering 50+ manufacturers
 * - Thread-safe access with shared_mutex protection
 * - CSPRNG for random selection
 * - Comprehensive device metadata
 * 
 * Copyright (c) 2024. Licensed for authorized testing purposes only.
 */
#pragma once

#ifndef REDROIDCPP_TAC_DATABASE_H
#define REDROIDCPP_TAC_DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>
#include <memory>

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
// TAC Database Class (PIMPL Pattern)
// ============================================================================

/**
 * @brief Singleton TAC Database for IMEI generation
 * 
 * Provides validated TAC codes for all major manufacturers to ensure
 * generated IMEIs pass Luhn validation and appear authentic.
 * 
 * Thread Safety:
 * - Uses shared_mutex for read-write lock separation
 * - All public methods are thread-safe
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
    // Forward declaration of Impl (PIMPL pattern)
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    // Private constructor for singleton
    TACDatabase();
    ~TACDatabase();
};

} // namespace RedroidCPP

#endif // REDROIDCPP_TAC_DATABASE_H
