/**
 * @file CommonUtils.hpp
 * @brief Common Utility Functions - Centralized code to eliminate duplication
 * @version 2.0.0
 * 
 * This file contains all common utility functions that were duplicated
 * across multiple modules. All modules should use these functions
 * instead of implementing their own versions.
 */

#ifndef VIRTUALPHONEPRO_COMMONUTILS_HPP
#define VIRTUALPHONEPRO_COMMONUTILS_HPP

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDateTime>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace VirtualPhonePro {
namespace Utils {

// ==============================================================================
// Luhn Algorithm - IMEI Validation
// ==============================================================================

/**
 * @brief Calculate Luhn check digit for a number string
 * @param base Base number string (without check digit)
 * @return Check digit (0-9)
 */
inline int calculateLuhnCheckDigit(const QString& base) {
    int sum = 0;
    bool alternate = true;
    
    for (int i = base.length() - 1; i >= 0; --i) {
        int digit = base[i].digitValue();
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

/**
 * @brief Calculate Luhn check digit (std::string version)
 */
inline int calculateLuhnCheckDigit(const std::string& base) {
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
    
    return (10 - (sum % 10)) % 10;
}

/**
 * @brief Validate IMEI using Luhn algorithm
 * @param imei IMEI string to validate (should be 15 digits)
 * @return true if valid, false otherwise
 */
inline bool validateIMEI(const QString& imei) {
    if (imei.length() != 15) {
        return false;
    }
    
    for (int i = 0; i < 15; ++i) {
        if (!imei[i].isDigit()) {
            return false;
        }
    }
    
    int sum = 0;
    bool alternate = false;
    
    for (int i = 14; i >= 0; --i) {
        int digit = imei[i].digitValue();
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

/**
 * @brief Validate IMEI (std::string version)
 */
inline bool validateIMEI(const std::string& imei) {
    if (imei.length() != 15) {
        return false;
    }
    
    for (char c : imei) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    
    int sum = 0;
    bool alternate = false;
    
    for (int i = 14; i >= 0; --i) {
        int digit = imei[i] - '0';
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

// ==============================================================================
// IMEI Generation
// ==============================================================================

/**
 * @brief Generate a valid IMEI number
 * @param tac Type Allocation Code (first 8 digits)
 * @param randomSeed Optional random seed for reproducibility
 * @return Generated IMEI string
 */
inline QString generateIMEI(const QString& tac, quint64 randomSeed = 0) {
    if (tac.length() < 6) {
        throw std::invalid_argument("TAC must be at least 6 digits");
    }
    
    QString base = tac;
    
    // Ensure base is 14 digits (8 TAC + 6 random)
    while (base.length() < 14) {
        if (randomSeed != 0) {
            base += QString::number((randomSeed + base.length()) % 10);
        } else {
            base += QString::number(QRandomGenerator::global()->bounded(10));
        }
    }
    base = base.left(14);
    
    // Calculate and append Luhn check digit
    int checkDigit = calculateLuhnCheckDigit(base);
    return base + QString::number(checkDigit);
}

/**
 * @brief Generate IMEI with specific TAC prefix
 */
inline std::string generateIMEI(const std::string& tac, quint64 seed = 0) {
    std::string base = tac;
    
    while (base.length() < 14) {
        if (seed != 0) {
            base += std::to_string((seed + base.length()) % 10);
        } else {
            base += std::to_string(QRandomGenerator::global()->bounded(10));
        }
    }
    base = base.substr(0, 14);
    
    int checkDigit = calculateLuhnCheckDigit(base);
    return base + std::to_string(checkDigit);
}

// ==============================================================================
// MAC Address Generation & Validation
// ==============================================================================

/**
 * @brief Generate a random MAC address
 * @param oui Organizationally Unique Identifier (e.g., "8C:71:F8" for Samsung)
 * @return Generated MAC address in XX:XX:XX:XX:XX:XX format
 */
inline QString generateMAC(const QString& oui = "00:00:00") {
    QStringList parts = oui.split(':');
    
    // Ensure we have at least 3 parts (OUI)
    while (parts.size() < 3) {
        parts.append(QString::number(QRandomGenerator::global()->bounded(256), 16).toUpper().rightJustified(2, '0'));
    }
    
    // Generate remaining 3 octets
    for (int i = 3; i < 6; ++i) {
        parts.append(QString::number(QRandomGenerator::global()->bounded(256), 16).toUpper().rightJustified(2, '0'));
    }
    
    return parts.join(':');
}

/**
 * @brief Validate MAC address format
 * @param mac MAC address to validate
 * @return true if valid format, false otherwise
 */
inline bool validateMAC(const QString& mac) {
    QStringList parts = mac.split(':');
    if (parts.size() != 6) {
        return false;
    }
    
    for (const QString& part : parts) {
        if (part.length() != 2) {
            return false;
        }
        bool ok;
        int value = part.toInt(&ok, 16);
        if (!ok || value < 0 || value > 255) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Check if MAC is locally administered (not from real manufacturer)
 * This helps avoid collisions with real device MACs
 */
inline bool isLocallyAdministeredMAC(const QString& mac) {
    if (!validateMAC(mac)) {
        return false;
    }
    
    // Locally administered bit is the second-least-significant bit of the first octet
    bool ok;
    int firstOctet = mac.left(2).toInt(&ok, 16);
    if (!ok) {
        return false;
    }
    
    // Check if second-least-significant bit is set
    return (firstOctet & 0x02) != 0;
}

// ==============================================================================
// Serial Number Generation
// ==============================================================================

/**
 * @brief Generate a device serial number
 * @param manufacturer Manufacturer name
 * @return Generated serial number
 */
inline QString generateSerialNumber(const QString& manufacturer) {
    QByteArray hash = QCryptographicHash::hash(
        (manufacturer + QDateTime::currentDateTime().toString("yyyyMMdd")).toUtf8(),
        QCryptographicHash::Sha256
    );
    
    QString serial;
    for (int i = 0; i < 4; ++i) {
        bool ok;
        int chunk = hash.mid(i * 2, 2).toInt(&ok, 16);
        if (ok) {
            serial += QString::number(chunk % 26 + 'A'); // A-Z
        }
    }
    
    // Add random digits
    for (int i = 0; i < 3; ++i) {
        serial += QString::number(QRandomGenerator::global()->bounded(10));
    }
    
    return serial;
}

/**
 * @brief Generate serial number (std::string version)
 */
inline std::string generateSerialNumber(const std::string& manufacturer) {
    std::stringstream ss;
    std::mt19937 gen(QRandomGenerator::global()->generate());
    std::uniform_int_distribution<> dist(0, 25);
    
    for (int i = 0; i < 4; ++i) {
        ss << static_cast<char>('A' + dist(gen));
    }
    
    std::uniform_int_distribution<> distDigit(0, 9);
    for (int i = 0; i < 3; ++i) {
        ss << distDigit(gen);
    }
    
    return ss.str();
}

// ==============================================================================
// Android ID Generation
// ==============================================================================

/**
 * @brief Generate a random Android ID (16 hex characters)
 * @return Generated Android ID
 */
inline QString generateAndroidId() {
    QString id;
    for (int i = 0; i < 16; ++i) {
        id += QString::number(QRandomGenerator::global()->bounded(16), 16).toUpper();
    }
    return id;
}

/**
 * @brief Generate Android ID (std::string version)
 */
inline std::string generateAndroidIdStr() {
    std::stringstream ss;
    std::mt19937 gen(QRandomGenerator::global()->generate());
    std::uniform_int_distribution<> dist(0, 15);
    
    const char hexChars[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) {
        ss << hexChars[dist(gen)];
    }
    
    return ss.str();
}

// ==============================================================================
// GSF ID (Google Services Framework) Generation
// ==============================================================================

/**
 * @brief Generate a GSF ID (10 digits)
 * @return Generated GSF ID
 */
inline QString generateGSFId() {
    QString id;
    for (int i = 0; i < 10; ++i) {
        id += QString::number(QRandomGenerator::global()->bounded(10));
    }
    return id;
}

// ==============================================================================
// ICCID Generation (SIM Card Identifier)
// ==============================================================================

/**
 * @brief Generate an ICCID (20 digits, with Luhn check)
 * @param mcc Mobile Country Code
 * @param mnc Mobile Network Code
 * @return Generated ICCID
 */
inline QString generateICCID(const QString& mcc = "880", const QString& mnc = "10") {
    QString iccid = mcc + mnc;
    
    // Add 12 random digits to make 17
    while (iccid.length() < 17) {
        iccid += QString::number(QRandomGenerator::global()->bounded(10));
    }
    
    // Calculate and append Luhn check digit
    int checkDigit = calculateLuhnCheckDigit(iccid);
    return iccid + QString::number(checkDigit);
}

// ==============================================================================
// IMSI Generation
// ==============================================================================

/**
 * @brief Generate an IMSI (15 digits)
 * @param mcc Mobile Country Code
 * @param mnc Mobile Network Code
 * @return Generated IMSI
 */
inline QString generateIMSI(const QString& mcc = "880", const QString& mnc = "10") {
    QString imsi = mcc + mnc;
    
    // Add remaining digits (15 - 3 - 2 = 10 more)
    while (imsi.length() < 14) {
        imsi += QString::number(QRandomGenerator::global()->bounded(10));
    }
    
    return imsi;
}

// ==============================================================================
// SHA256 Hashing
// ==============================================================================

/**
 * @brief Generate SHA256 hash of a string
 * @param input Input string
 * @return Hex-encoded SHA256 hash
 */
inline QString sha256(const QString& input) {
    QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex().toUpper();
}

/**
 * @brief Generate SHA256 hash (std::string version)
 */
inline std::string sha256(const std::string& input) {
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(input), QCryptographicHash::Sha256);
    return hash.toHex().toUpper().toStdString();
}

// ==============================================================================
// Exception Handling Helper
// ==============================================================================

/**
 * @brief Safe execute with exception handling
 * @param func Function to execute
 * @param errorMsg Error message to log on failure
 * @param defaultReturn Default value to return on exception
 * @return Function result or defaultReturn on exception
 */
template<typename Func, typename ReturnType>
inline ReturnType safeExecute(Func func, const QString& errorMsg, ReturnType defaultReturn) {
    try {
        return func();
    } catch (const std::exception& e) {
        qWarning() << errorMsg << ":" << e.what();
        return defaultReturn;
    } catch (...) {
        qWarning() << errorMsg << ": Unknown exception";
        return defaultReturn;
    }
}

/**
 * @brief Safe execute with void return
 */
template<typename Func>
inline bool safeExecuteVoid(Func func, const QString& errorMsg) {
    try {
        func();
        return true;
    } catch (const std::exception& e) {
        qWarning() << errorMsg << ":" << e.what();
        return false;
    } catch (...) {
        qWarning() << errorMsg << ": Unknown exception";
        return false;
    }
}

// ==============================================================================
// String Utilities
// ==============================================================================

/**
 * @brief Trim whitespace from both ends
 */
inline QString trim(const QString& str) {
    return str.trimmed();
}

/**
 * @brief Check if string is empty or whitespace only
 */
inline bool isEmpty(const QString& str) {
    return str.trimmed().isEmpty();
}

/**
 * @brief Convert string to uppercase
 */
inline QString toUpper(const QString& str) {
    return str.toUpper();
}

/**
 * @brief Convert string to lowercase
 */
inline QString toLower(const QString& str) {
    return str.toLower();
}

// ==============================================================================
// Format Utilities
// ==============================================================================

/**
 * @brief Format MAC address with colons
 */
inline QString formatMAC(const QString& mac) {
    QString cleaned = mac.remove(':').toUpper();
    if (cleaned.length() != 12) {
        return mac; // Return original if invalid
    }
    
    QString formatted;
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) formatted += ':';
        formatted += cleaned.mid(i, 2);
    }
    return formatted;
}

/**
 * @brief Format phone number
 */
inline QString formatPhoneNumber(const QString& number) {
    QString cleaned = number.remove(QRegExp("[^0-9+]"));
    return cleaned;
}

// ==============================================================================
// Validation Utilities
// ==============================================================================

/**
 * @brief Validate phone number format
 */
inline bool validatePhoneNumber(const QString& phone) {
    QString cleaned = phone.remove(QRegExp("[^0-9+]"));
    return cleaned.length() >= 10 && cleaned.length() <= 15;
}

/**
 * @brief Validate email format
 */
inline bool validateEmail(const QString& email) {
    QRegExp emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return emailRegex.exactMatch(email);
}

/**
 * @brief Validate alphanumeric string
 */
inline bool validateAlphanumeric(const QString& str) {
    for (const QChar& c : str) {
        if (!c.isLetterOrNumber() && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

// ==============================================================================
// Random Utilities
// ==============================================================================

/**
 * @brief Generate random integer in range
 */
inline int randomInt(int min, int max) {
    return QRandomGenerator::global()->bounded(min, max + 1);
}

/**
 * @brief Generate random double in range
 */
inline double randomDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(*QRandomGenerator::global());
}

/**
 * @brief Generate random boolean with probability
 */
inline bool randomBool(double probability = 0.5) {
    return QRandomGenerator::global()->bounded(100) < (probability * 100);
}

// ==============================================================================
// Date/Time Utilities
// ==============================================================================

/**
 * @brief Get current timestamp string
 */
inline QString currentTimestamp() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

/**
 * @brief Get current date string
 */
inline QString currentDate() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd");
}

/**
 * @brief Format duration in seconds to human readable
 */
inline QString formatDuration(int seconds) {
    if (seconds < 60) {
        return QString("%1 seconds").arg(seconds);
    } else if (seconds < 3600) {
        return QString("%1 minutes").arg(seconds / 60);
    } else if (seconds < 86400) {
        return QString("%1 hours").arg(seconds / 3600);
    } else {
        return QString("%1 days").arg(seconds / 86400);
    }
}

} // namespace Utils
} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_COMMONUTILS_HPP
