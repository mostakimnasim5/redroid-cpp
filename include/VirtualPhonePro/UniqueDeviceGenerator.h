/**
 * @file UniqueDeviceGenerator.h
 * @brief Unique Device Profile Generator
 * @version 2.0.0
 * 
 * Ensures 100% unique device profiles with cryptographic uniqueness.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H
#define VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>

namespace VirtualPhonePro {

/**
 * @brief Unique Device Profile Generator
 * 
 * Generates 100% unique device profiles for each instance.
 * Uses cryptographic hashing and UUID generation for uniqueness.
 */
class UniqueDeviceGenerator {
public:
    static UniqueDeviceGenerator& instance();
    
    // ========================================================================
    // Unique ID Generation
    // ========================================================================
    
    /**
     * @brief Generate unique instance ID
     */
    QString generateInstanceId();
    
    /**
     * @brief Generate unique IMEI
     */
    QString generateUniqueIMEI();
    
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
    
private:
    UniqueDeviceGenerator();
    ~UniqueDeviceGenerator() = default;
    
    // Internal generation helpers
    QString generateHash(const QString& input);
    int calculateLuhnCheckDigit(const QString& base);
    
    // Storage
    QMap<QString, QString> m_generatedIMEIs;
    QMap<QString, QString> m_generatedSerials;
    QMap<QString, QString> m_generatedAndroidIds;
    QMap<QString, QString> m_instanceToIds;
    QMap<QString, QJsonObject> m_idToInstance;
    
    // OUI prefixes for different manufacturers
    QMap<QString, QStringList> m_manufacturerOUIs;
    
    void initializeOUIs();
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_UNIQUE_DEVICE_GENERATOR_H
