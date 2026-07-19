/**
 * @file UniqueDeviceGenerator.h
 * @brief Unique Device Profile Generator
 * @version 3.0.0
 *
 * Features:
 * - Thread-safe singleton with double-checked locking
 * - Cryptographically secure random number generation (CSPRNG)
 * - Integration with 1000+ TAC database
 * - Comprehensive error handling
 * - Persistent identity storage
 */

#pragma once

#ifndef VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H
#define VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QtGlobal>
#include <atomic>

namespace VirtualPhonePro {

/**
 * @brief Unique Device Profile Generator
 *
 * Generates 100% unique device profiles for each instance.
 * Uses cryptographic hashing and UUID generation for uniqueness.
 *
 * Thread Safety:
 * - Singleton uses double-checked locking with atomic flag
 * - All ID generation methods are thread-safe
 * - Mutex protection for ID storage access
 */
class UniqueDeviceGenerator {
public:
    /**
     * @brief Get thread-safe singleton instance
     */
    static UniqueDeviceGenerator& instance();

    /**
     * @brief Check if generator is initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Unique ID Generation
    // ========================================================================

    /**
     * @brief Generate unique instance ID
     */
    QString generateInstanceId();

    /**
     * @brief Generate unique IMEI for specific manufacturer
     * @param manufacturer Device manufacturer name (optional, defaults to random)
     */
    QString generateUniqueIMEI(const QString& manufacturer = QString());

    /**
     * @brief Generate unique serial number
     */
    QString generateUniqueSerial(const QString& manufacturer);

    /**
     * @brief Generate unique Android ID
     */
    QString generateUniqueAndroidId();

    /**
     * @brief Generate unique GSF ID
     */
    QString generateUniqueGSFId();

    /**
     * @brief Generate unique MAC address
     */
    QString generateUniqueMAC(const QString& oui = QString());

    /**
     * @brief Generate unique device key
     */
    QString generateUniqueDeviceKey();

    /**
     * @brief Generate unique ICCID
     */
    QString generateUniqueICCID();

    /**
     * @brief Generate unique IMSI
     */
    QString generateUniqueIMSI(const QString& mcc, const QString& mnc);

    /**
     * @brief Generate unique Bluetooth MAC
     */
    QString generateUniqueBluetoothMAC();

    // ========================================================================
    // Complete Unique Profile
    // ========================================================================

    /**
     * @brief Generate complete unique device identity
     */
    QJsonObject generateCompleteUniqueIdentity(const QString& manufacturer);

    /**
     * @brief Verify uniqueness of a profile
     */
    bool verifyUniqueness(const QJsonObject& profile);

    /**
     * @brief Get all generated unique IDs for an instance
     */
    QJsonObject getUniqueIdsForInstance(const QString& instanceId);

    /**
     * @brief Check if IMEI already exists
     */
    bool isIMEIUnique(const QString& imei);

    /**
     * @brief Check if serial already exists
     */
    bool isSerialUnique(const QString& serial);

    // ========================================================================
    // Instance Management
    // ========================================================================

    /**
     * @brief Register an instance with its unique IDs
     */
    void registerInstance(const QString& instanceId, const QJsonObject& uniqueIds);

    /**
     * @brief Unregister an instance
     */
    void unregisterInstance(const QString& instanceId);

    /**
     * @brief Get count of all unique devices
     */
    int getUniqueDeviceCount();

    /**
     * @brief Clear all uniqueness records
     */
    void clearAllRecords();

    /**
     * @brief Generate hash for a profile (public wrapper)
     */
    QString hashProfile(const QString& input);

private:
    // Thread-safe singleton implementation
    static std::atomic<UniqueDeviceGenerator*> s_instance;
    static std::atomic<quint64> s_instanceId;
    static QMutex s_mutex;

    UniqueDeviceGenerator();
    ~UniqueDeviceGenerator();

    // Disable copy/move for singleton
    Q_DISABLE_COPY_MOVE(UniqueDeviceGenerator)

    // Internal generation helpers
    QString generateHash(const QString& input);
    int calculateLuhnCheckDigit(const QString& base);
    QStringList getTACListForManufacturer(const QString& manufacturer) const;
    QString generateFallbackTAC() const;

    // CSPRNG (Cryptographically Secure Random Number Generator)
    bool getSecureRandomBytes(unsigned char* buffer, size_t length) const;
    quint32 getSecureRandomUInt32(quint32 min, quint32 max) const;
    quint64 getSecureRandomUInt64() const;
    QString getSecureRandomHex(size_t length) const;

    // Persistence
    void saveRecords();
    void loadExistingRecords();
    void initializeTACDatabase();

    // Initialization state
    std::atomic<bool> m_initialized;

    // Thread-safe storage with mutex
    mutable QMutex m_idMutex;
    QMap<QString, QString> m_generatedIMEIs;
    QMap<QString, QString> m_generatedSerials;
    QMap<QString, QString> m_generatedAndroidIds;
    QMap<QString, QString> m_instanceToIds;
    QMap<QString, QJsonObject> m_idToInstance;

    // Next instance ID counter
    quint64 m_nextInstanceId;

    // OUI prefixes for different manufacturers
    QMap<QString, QStringList> m_manufacturerOUIs;

    void initializeOUIs();
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H
