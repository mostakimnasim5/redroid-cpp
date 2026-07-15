/**
 * @file SSLCertificateManager.h
 * @brief Preloaded SSL Certificate Manager
 * @version 3.0.0
 * 
 * Provides realistic SSL certificate simulation:
 * - Preloaded CA certificates
 * - Root certificate authorities
 * - Intermediate certificates
 * - Domain-specific certificates
 * - Certificate pinning simulation
 */

#pragma once

#ifndef VIRTUALPHONEPRO_SSL_CERTIFICATE_MANAGER_H
#define VIRTUALPHONEPRO_SSL_CERTIFICATE_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include <QStringList>

namespace VirtualPhonePro {

// Certificate Type
enum class CertificateType {
    ROOT_CA,
    INTERMEDIATE_CA,
    SERVER_CERT,
    CLIENT_CERT,
    WIREGUARD
};

// Certificate Status
enum class CertificateStatus {
    VALID,
    EXPIRED,
    REVOKED,
    UNKNOWN
};

// CA Certificate Info
struct CACertificate {
    QString commonName;
    QString organization;
    QString organizationalUnit;
    QString country;
    QString serialNumber;
    QString subjectKeyId;
    QString authorityKeyId;
    QDateTime notBefore;
    QDateTime notAfter;
    QString fingerprint;
    QString signatureAlgorithm;
    int keySize;
    CertificateType type;
    CertificateStatus status;
    bool isTrusted;
    bool isUserAdded;
};

// Certificate Store
struct CertificateStore {
    QString instanceId;
    QMap<QString, CACertificate> rootCAs;
    QMap<QString, CACertificate> intermediateCAs;
    QMap<QString, CACertificate> serverCerts;
    QMap<QString, CACertificate> clientCerts;
    int totalCertificates;
    int trustedCertificates;
    int expiredCertificates;
    int revokedCertificates;
};

// Predefined CA Providers
enum class CAProvider {
    DIGICERT,
    COMODO,
    GO_DADDY,
    GLOBAL_SIGN,
    SYMANTEC,
    ISRG,           // Let's Encrypt
    AMAZON,
    MICROSOFT,
    GOOGLE,
    SAMSUNG,
    HUAWEI,
    XIAOMI,
    CUSTOM
};

class SSLCertificateManager {
public:
    static SSLCertificateManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure certificate store
     */
    bool configure(const QString& instanceId);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Certificate Management
    // =========================================================================
    
    /**
     * @brief Add a CA certificate
     */
    bool addCACertificate(const QString& instanceId, const CACertificate& cert);
    
    /**
     * @brief Remove a CA certificate
     */
    bool removeCACertificate(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Get CA certificate
     */
    CACertificate getCACertificate(const QString& instanceId, const QString& certId) const;
    
    /**
     * @brief Get all CA certificates
     */
    QList<CACertificate> getAllCACertificates(const QString& instanceId) const;
    
    // =========================================================================
    // Certificate Providers
    // =========================================================================
    
    /**
     * @brief Load predefined CA certificates
     */
    bool loadPredefinedCAs(const QString& instanceId, CAProvider provider);
    
    /**
     * @brief Load all major CA providers
     */
    bool loadAllMajorCAs(const QString& instanceId);
    
    /**
     * @brief Load OEM-specific certificates
     */
    bool loadOEMCertificates(const QString& instanceId, const QString& oemType);
    
    /**
     * @brief Load device manufacturer certificates
     */
    bool loadManufacturerCertificates(const QString& instanceId);
    
    // =========================================================================
    // Certificate Validation
    // =========================================================================
    
    /**
     * @brief Validate certificate
     */
    CertificateStatus validateCertificate(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Check if certificate is trusted
     */
    bool isCertificateTrusted(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Check expiration
     */
    bool isCertificateExpired(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Get certificate chain
     */
    QList<CACertificate> getCertificateChain(const QString& instanceId, const QString& certId);
    
    // =========================================================================
    // Certificate Export
    // =========================================================================
    
    /**
     * @brief Export certificate to PEM
     */
    QString exportCertificateToPEM(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Export all certificates
     */
    QStringList exportAllCertificates(const QString& instanceId);
    
    /**
     * @brief Get certificate fingerprint
     */
    QString getCertificateFingerprint(const QString& instanceId, const QString& certId);
    
    // =========================================================================
    // Trust Store
    // =========================================================================
    
    /**
     * @brief Trust a certificate
     */
    bool trustCertificate(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Untrust a certificate
     */
    bool untrustCertificate(const QString& instanceId, const QString& certId);
    
    /**
     * @brief Get trust store summary
     */
    QJsonObject getTrustStoreSummary(const QString& instanceId) const;
    
    // =========================================================================
    // System Certificate Installation
    // =========================================================================
    
    /**
     * @brief Install system certificate
     */
    bool installSystemCertificate(const QString& instanceId, const QString& certPath);
    
    /**
     * @brief Install user certificate
     */
    bool installUserCertificate(const QString& instanceId, const QString& certPath);
    
    /**
     /**
     * @brief Clear user certificates
     */
    bool clearUserCertificates(const QString& instanceId);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get certificate store
     */
    CertificateStore getCertificateStore(const QString& instanceId) const;
    
    /**
     * @brief Get certificate store as JSON
     */
    QJsonObject getCertificateStoreJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
private:
    SSLCertificateManager();
    
    // Helper methods
    void initializePredefinedCertificates();
    CACertificate generatePredefinedCertificate(CAProvider provider, CertificateType type);
    QString generateCertificateFingerprint(const QString& serial, const QString& issuer);
    QString providerToString(CAProvider provider) const;
    QString certificateTypeToString(CertificateType type) const;
    QString generateRandomSerial();
    
    QMap<QString, CertificateStore> m_stores;
    QList<CACertificate> m_predefinedCertificates;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SSL_CERTIFICATE_MANAGER_H
