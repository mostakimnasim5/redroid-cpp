/**
 * @file SSLCertificateManager.cpp
 * @brief Preloaded SSL Certificate Manager Implementation
 */

#include "VirtualPhonePro/SSLCertificateManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>

namespace VirtualPhonePro {

SSLCertificateManager* SSLCertificateManager::s_instance = nullptr;

SSLCertificateManager& SSLCertificateManager::instance() {
    if (!s_instance) {
        s_instance = new SSLCertificateManager();
    }
    return *s_instance;
}

SSLCertificateManager::SSLCertificateManager() {
    initializePredefinedCertificates();
}

// ============================================================================
// Configuration
// ============================================================================

bool SSLCertificateManager::configure(const QString& instanceId) {
    CertificateStore store;
    store.instanceId = instanceId;
    store.totalCertificates = 0;
    store.trustedCertificates = 0;
    store.expiredCertificates = 0;
    store.revokedCertificates = 0;
    
    m_stores[instanceId] = store;
    
    qDebug() << "Configured certificate store for instance:" << instanceId;
    
    return applyToInstance(instanceId);
}

bool SSLCertificateManager::applyToInstance(const QString& instanceId) {
    if (!m_stores.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    const CertificateStore& store = m_stores[instanceId];
    
    QStringList commands = {
        // Enable system CA certificates
        "settings put global cert_locator_install_enabled 1",
        "settings put global install_non_market_apps 0",
        
        // Set up trust management
        "cmd trust store update",
    };
    
    // If we have certificates, set up trust store
    if (store.trustedCertificates > 0) {
        commands += "setprop ro.trust_lib.enable true";
    }
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Certificate store applied to instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Certificate Management
// ============================================================================

bool SSLCertificateManager::addCACertificate(const QString& instanceId, const CACertificate& cert) {
    if (!m_stores.contains(instanceId)) {
        configure(instanceId);
    }
    
    CertificateStore& store = m_stores[instanceId];
    
    if (cert.type == CertificateType::ROOT_CA) {
        store.rootCAs[cert.serialNumber] = cert;
    } else if (cert.type == CertificateType::INTERMEDIATE_CA) {
        store.intermediateCAs[cert.serialNumber] = cert;
    } else {
        store.serverCerts[cert.serialNumber] = cert;
    }
    
    store.totalCertificates++;
    if (cert.isTrusted) {
        store.trustedCertificates++;
    }
    if (cert.status == CertificateStatus::EXPIRED) {
        store.expiredCertificates++;
    }
    if (cert.status == CertificateStatus::REVOKED) {
        store.revokedCertificates++;
    }
    
    qDebug() << "Added certificate:" << cert.commonName << "for instance:" << instanceId;
    
    return true;
}

bool SSLCertificateManager::removeCACertificate(const QString& instanceId, const QString& certId) {
    if (!m_stores.contains(instanceId)) {
        return false;
    }
    
    CertificateStore& store = m_stores[instanceId];
    
    bool removed = false;
    
    if (store.rootCAs.contains(certId)) {
        store.totalCertificates--;
        if (store.rootCAs[certId].isTrusted) {
            store.trustedCertificates--;
        }
        store.rootCAs.remove(certId);
        removed = true;
    }
    
    if (store.intermediateCAs.contains(certId)) {
        store.totalCertificates--;
        if (store.intermediateCAs[certId].isTrusted) {
            store.trustedCertificates--;
        }
        store.intermediateCAs.remove(certId);
        removed = true;
    }
    
    return removed;
}

CACertificate SSLCertificateManager::getCACertificate(const QString& instanceId, 
                                                       const QString& certId) const {
    CACertificate defaultCert;
    
    if (m_stores.contains(instanceId)) {
        const CertificateStore& store = m_stores[instanceId];
        
        if (store.rootCAs.contains(certId)) {
            return store.rootCAs[certId];
        }
        if (store.intermediateCAs.contains(certId)) {
            return store.intermediateCAs[certId];
        }
    }
    
    return defaultCert;
}

QList<CACertificate> SSLCertificateManager::getAllCACertificates(const QString& instanceId) const {
    QList<CACertificate> certs;
    
    if (m_stores.contains(instanceId)) {
        const CertificateStore& store = m_stores[instanceId];
        
        for (const auto& cert : store.rootCAs) {
            certs.append(cert);
        }
        for (const auto& cert : store.intermediateCAs) {
            certs.append(cert);
        }
    }
    
    return certs;
}

// ============================================================================
// Certificate Providers
// ============================================================================

bool SSLCertificateManager::loadPredefinedCAs(const QString& instanceId, CAProvider provider) {
    CACertificate rootCA = generatePredefinedCertificate(provider, CertificateType::ROOT_CA);
    
    addCACertificate(instanceId, rootCA);
    
    // Add intermediate CA
    CACertificate intermediateCA = rootCA;
    intermediateCA.type = CertificateType::INTERMEDIATE_CA;
    intermediateCA.commonName = "Intermediate " + rootCA.commonName;
    intermediateCA.serialNumber = generateRandomSerial();
    intermediateCA.authorityKeyId = rootCA.subjectKeyId;
    intermediateCA.isTrusted = true;
    
    addCACertificate(instanceId, intermediateCA);
    
    qDebug() << "Loaded predefined CAs for provider:" << providerToString(provider);
    
    return true;
}

bool SSLCertificateManager::loadAllMajorCAs(const QString& instanceId) {
    if (!m_stores.contains(instanceId)) {
        configure(instanceId);
    }
    
    QList<CAProvider> majorProviders = {
        CAProvider::DIGICERT,
        CAProvider::COMODO,
        CAProvider::GO_DADDY,
        CAProvider::GLOBAL_SIGN,
        CAProvider::SYMANTEC,
        CAProvider::ISRG,
        CAProvider::AMAZON,
        CAProvider::MICROSOFT,
        CAProvider::GOOGLE,
    };
    
    for (CAProvider provider : majorProviders) {
        loadPredefinedCAs(instanceId, provider);
    }
    
    qDebug() << "Loaded all major CA certificates for instance:" << instanceId;
    
    return true;
}

bool SSLCertificateManager::loadOEMCertificates(const QString& instanceId, const QString& oemType) {
    if (!m_stores.contains(instanceId)) {
        configure(instanceId);
    }
    
    QString oemLower = oemType.toLower();
    
    if (oemLower == "samsung") {
        loadPredefinedCAs(instanceId, CAProvider::SAMSUNG);
    } else if (oemLower == "huawei") {
        loadPredefinedCAs(instanceId, CAProvider::HUAWEI);
    } else if (oemLower == "xiaomi") {
        loadPredefinedCAs(instanceId, CAProvider::XIAOMI);
    } else if (oemLower == "google") {
        loadPredefinedCAs(instanceId, CAProvider::GOOGLE);
    }
    
    qDebug() << "Loaded OEM certificates for" << oemType << "instance:" << instanceId;
    
    return true;
}

bool SSLCertificateManager::loadManufacturerCertificates(const QString& instanceId) {
    if (!m_stores.contains(instanceId)) {
        configure(instanceId);
    }
    
    // Load device manufacturer certificates
    QList<CAProvider> manufacturers = {
        CAProvider::SAMSUNG,
        CAProvider::HUAWEI,
        CAProvider::XIAOMI,
    };
    
    for (CAProvider provider : manufacturers) {
        CACertificate cert = generatePredefinedCertificate(provider, CertificateType::ROOT_CA);
        cert.isTrusted = true;
        addCACertificate(instanceId, cert);
    }
    
    qDebug() << "Loaded manufacturer certificates for instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Certificate Validation
// ============================================================================

CertificateStatus SSLCertificateManager::validateCertificate(const QString& instanceId, 
                                                            const QString& certId) {
    CACertificate cert = getCACertificate(instanceId, certId);
    return cert.status;
}

bool SSLCertificateManager::isCertificateTrusted(const QString& instanceId, 
                                                   const QString& certId) {
    CACertificate cert = getCACertificate(instanceId, certId);
    return cert.isTrusted && cert.status == CertificateStatus::VALID;
}

bool SSLCertificateManager::isCertificateExpired(const QString& instanceId, 
                                                   const QString& certId) {
    CACertificate cert = getCACertificate(instanceId, certId);
    
    QDateTime now = QDateTime::currentDateTime();
    return now > cert.notAfter || now < cert.notBefore;
}

QList<CACertificate> SSLCertificateManager::getCertificateChain(const QString& instanceId, 
                                                               const QString& certId) {
    QList<CACertificate> chain;
    
    CACertificate serverCert = getCACertificate(instanceId, certId);
    if (!serverCert.serialNumber.isEmpty()) {
        chain.append(serverCert);
        
        // Find intermediate CA
        if (!serverCert.authorityKeyId.isEmpty() && m_stores.contains(instanceId)) {
            const CertificateStore& store = m_stores[instanceId];
            for (const auto& cert : store.intermediateCAs) {
                if (cert.subjectKeyId == serverCert.authorityKeyId) {
                    chain.append(cert);
                    break;
                }
            }
        }
        
        // Find root CA
        if (!serverCert.authorityKeyId.isEmpty() && m_stores.contains(instanceId)) {
            const CertificateStore& store = m_stores[instanceId];
            for (const auto& cert : store.rootCAs) {
                if (cert.subjectKeyId == serverCert.authorityKeyId) {
                    chain.append(cert);
                    break;
                }
            }
        }
    }
    
    return chain;
}

// ============================================================================
// Certificate Export
// ============================================================================

QString SSLCertificateManager::exportCertificateToPEM(const QString& instanceId, 
                                                      const QString& certId) {
    CACertificate cert = getCACertificate(instanceId, certId);
    
    if (cert.serialNumber.isEmpty()) {
        return QString();
    }
    
    QString pem = QString(
        "-----BEGIN CERTIFICATE-----\n"
        "%1\n"
        "-----END CERTIFICATE-----\n"
        "Subject: %2\n"
        "Issuer: %3\n"
        "Valid: %4 - %5\n"
        "Serial: %6\n"
        "Fingerprint: %7\n"
    ).arg(
        cert.fingerprint,
        cert.commonName,
        cert.organization,
        cert.notBefore.toString(Qt::ISODate),
        cert.notAfter.toString(Qt::ISODate),
        cert.serialNumber,
        cert.fingerprint
    );
    
    return pem;
}

QStringList SSLCertificateManager::exportAllCertificates(const QString& instanceId) {
    QStringList exports;
    
    if (m_stores.contains(instanceId)) {
        const CertificateStore& store = m_stores[instanceId];
        
        for (const auto& cert : store.rootCAs) {
            exports.append(cert.commonName + " (Root CA)");
        }
        for (const auto& cert : store.intermediateCAs) {
            exports.append(cert.commonName + " (Intermediate)");
        }
        for (const auto& cert : store.serverCerts) {
            exports.append(cert.commonName + " (Server)");
        }
    }
    
    return exports;
}

QString SSLCertificateManager::getCertificateFingerprint(const QString& instanceId, 
                                                          const QString& certId) {
    CACertificate cert = getCACertificate(instanceId, certId);
    return cert.fingerprint;
}

// ============================================================================
// Trust Store
// ============================================================================

bool SSLCertificateManager::trustCertificate(const QString& instanceId, const QString& certId) {
    if (!m_stores.contains(instanceId)) {
        return false;
    }
    
    CertificateStore& store = m_stores[instanceId];
    
    if (store.rootCAs.contains(certId)) {
        store.rootCAs[certId].isTrusted = true;
        store.rootCAs[certId].status = CertificateStatus::VALID;
        store.trustedCertificates++;
        return true;
    }
    
    if (store.intermediateCAs.contains(certId)) {
        store.intermediateCAs[certId].isTrusted = true;
        store.intermediateCAs[certId].status = CertificateStatus::VALID;
        store.trustedCertificates++;
        return true;
    }
    
    return false;
}

bool SSLCertificateManager::untrustCertificate(const QString& instanceId, const QString& certId) {
    if (!m_stores.contains(instanceId)) {
        return false;
    }
    
    CertificateStore& store = m_stores[instanceId];
    
    if (store.rootCAs.contains(certId)) {
        store.rootCAs[certId].isTrusted = false;
        store.trustedCertificates = qMax(0, store.trustedCertificates - 1);
        return true;
    }
    
    if (store.intermediateCAs.contains(certId)) {
        store.intermediateCAs[certId].isTrusted = false;
        store.trustedCertificates = qMax(0, store.trustedCertificates - 1);
        return true;
    }
    
    return false;
}

QJsonObject SSLCertificateManager::getTrustStoreSummary(const QString& instanceId) const {
    QJsonObject summary;
    
    if (m_stores.contains(instanceId)) {
        const CertificateStore& store = m_stores[instanceId];
        
        summary["totalCertificates"] = store.totalCertificates;
        summary["trustedCertificates"] = store.trustedCertificates;
        summary["expiredCertificates"] = store.expiredCertificates;
        summary["revokedCertificates"] = store.revokedCertificates;
        summary["rootCAs"] = store.rootCAs.size();
        summary["intermediateCAs"] = store.intermediateCAs.size();
        summary["serverCerts"] = store.serverCerts.size();
    }
    
    return summary;
}

// ============================================================================
// System Certificate Installation
// ============================================================================

bool SSLCertificateManager::installSystemCertificate(const QString& instanceId, 
                                                     const QString& certPath) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("cp %1 /system/etc/security/cacerts/").arg(certPath),
        "chmod 644 /system/etc/security/cacerts/*.pem",
        "reboot",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Installed system certificate from" << certPath;
    
    return true;
}

bool SSLCertificateManager::installUserCertificate(const QString& instanceId, 
                                                   const QString& certPath) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("cp %1 /data/misc/user/0/cacerts-added/").arg(certPath),
        "chmod 600 /data/misc/user/0/cacerts-added/*.pem",
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Installed user certificate from" << certPath;
    
    return true;
}

bool SSLCertificateManager::clearUserCertificates(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "rm -rf /data/misc/user/0/cacerts-added/*");
    
    qDebug() << "Cleared user certificates for instance:" << instanceId;
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

CertificateStore SSLCertificateManager::getCertificateStore(const QString& instanceId) const {
    CertificateStore defaultStore;
    defaultStore.instanceId = instanceId;
    
    if (m_stores.contains(instanceId)) {
        return m_stores[instanceId];
    }
    
    return defaultStore;
}

QJsonObject SSLCertificateManager::getCertificateStoreJSON(const QString& instanceId) const {
    QJsonObject json;
    
    if (m_stores.contains(instanceId)) {
        const CertificateStore& store = m_stores[instanceId];
        
        QJsonArray rootCAs;
        for (const auto& cert : store.rootCAs) {
            QJsonObject certJson;
            certJson["id"] = cert.serialNumber;
            certJson["name"] = cert.commonName;
            certJson["org"] = cert.organization;
            certJson["validUntil"] = cert.notAfter.toString(Qt::ISODate);
            certJson["trusted"] = cert.isTrusted;
            certJson["type"] = "Root CA";
            rootCAs.append(certJson);
        }
        json["rootCAs"] = rootCAs;
        
        QJsonArray intermediateCAs;
        for (const auto& cert : store.intermediateCAs) {
            QJsonObject certJson;
            certJson["id"] = cert.serialNumber;
            certJson["name"] = cert.commonName;
            certJson["org"] = cert.organization;
            certJson["validUntil"] = cert.notAfter.toString(Qt::ISODate);
            certJson["trusted"] = cert.isTrusted;
            certJson["type"] = "Intermediate CA";
            intermediateCAs.append(certJson);
        }
        json["intermediateCAs"] = intermediateCAs;
        
        json["totalCertificates"] = store.totalCertificates;
        json["trustedCertificates"] = store.trustedCertificates;
    }
    
    return json;
}

bool SSLCertificateManager::reset(const QString& instanceId) {
    if (m_stores.contains(instanceId)) {
        m_stores.remove(instanceId);
        return true;
    }
    return false;
}

// ============================================================================
// Private Helpers
// ============================================================================

void SSLCertificateManager::initializePredefinedCertificates() {
    // Add some predefined certificates from major providers
    QList<CAProvider> providers = {
        CAProvider::DIGICERT,
        CAProvider::COMODO,
        CAProvider::GO_DADDY,
        CAProvider::GLOBAL_SIGN,
        CAProvider::SYMANTEC,
        CAProvider::ISRG,
        CAProvider::AMAZON,
        CAProvider::MICROSOFT,
        CAProvider::GOOGLE,
    };
    
    for (CAProvider provider : providers) {
        m_predefinedCertificates.append(generatePredefinedCertificate(provider, CertificateType::ROOT_CA));
    }
}

CACertificate SSLCertificateManager::generatePredefinedCertificate(CAProvider provider, 
                                                                   CertificateType type) {
    CACertificate cert;
    cert.type = type;
    cert.status = CertificateStatus::VALID;
    cert.isTrusted = true;
    cert.isUserAdded = false;
    
    // Set properties based on provider
    switch (provider) {
        case CAProvider::DIGICERT:
            cert.commonName = "DigiCert Global Root G2";
            cert.organization = "DigiCert Inc";
            cert.country = "US";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::COMODO:
            cert.commonName = "COMODO RSA Certification Authority";
            cert.organization = "COMODO CA Limited";
            cert.country = "GB";
            cert.keySize = 4096;
            cert.signatureAlgorithm = "SHA384withRSA";
            break;
        case CAProvider::GO_DADDY:
            cert.commonName = "Go Daddy Root Certificate Authority";
            cert.organization = "GoDaddy.com, Inc.";
            cert.country = "US";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::GLOBAL_SIGN:
            cert.commonName = "GlobalSign Root CA";
            cert.organization = "GlobalSign";
            cert.country = "BE";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::SYMANTEC:
            cert.commonName = "VeriSign Class 3 Public Primary CA";
            cert.organization = "VeriSign, Inc.";
            cert.country = "US";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA1withRSA";
            break;
        case CAProvider::ISRG:
            cert.commonName = "ISRG Root X1";
            cert.organization = "Internet Security Research Group";
            cert.country = "US";
            cert.keySize = 4096;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::AMAZON:
            cert.commonName = "Amazon Root CA 1";
            cert.organization = "Amazon";
            cert.country = "US";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::MICROSOFT:
            cert.commonName = "Microsoft RSA Root Certificate Authority";
            cert.organization = "Microsoft Corporation";
            cert.country = "US";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::GOOGLE:
            cert.commonName = "GTS Root R1";
            cert.organization = "Google Trust Services LLC";
            cert.country = "US";
            cert.keySize = 4096;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::SAMSUNG:
            cert.commonName = "Samsung Root CA";
            cert.organization = "Samsung Electronics Co., Ltd.";
            cert.country = "KR";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::HUAWEI:
            cert.commonName = "Huawei Root CA";
            cert.organization = "Huawei Technologies Co., Ltd.";
            cert.country = "CN";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        case CAProvider::XIAOMI:
            cert.commonName = "Xiaomi Root CA";
            cert.organization = "Xiaomi Inc.";
            cert.country = "CN";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
        default:
            cert.commonName = "Custom Root CA";
            cert.organization = "Custom Organization";
            cert.keySize = 2048;
            cert.signatureAlgorithm = "SHA256withRSA";
            break;
    }
    
    // Set validity
    cert.notBefore = QDateTime::currentDateTime().addYears(-5);
    cert.notAfter = QDateTime::currentDateTime().addYears(15);
    
    // Generate serial and fingerprint
    cert.serialNumber = generateRandomSerial();
    cert.fingerprint = generateCertificateFingerprint(cert.serialNumber, cert.organization);
    cert.subjectKeyId = cert.fingerprint.left(40);
    
    return cert;
}

QString SSLCertificateManager::generateCertificateFingerprint(const QString& serial, 
                                                               const QString& issuer) {
    QByteArray data = (serial + issuer).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return hash.toHex();
}

QString SSLCertificateManager::providerToString(CAProvider provider) const {
    switch (provider) {
        case CAProvider::DIGICERT: return "DigiCert";
        case CAProvider::COMODO: return "Comodo";
        case CAProvider::GO_DADDY: return "GoDaddy";
        case CAProvider::GLOBAL_SIGN: return "GlobalSign";
        case CAProvider::SYMANTEC: return "Symantec";
        case CAProvider::ISRG: return "ISRG (Let's Encrypt)";
        case CAProvider::AMAZON: return "Amazon";
        case CAProvider::MICROSOFT: return "Microsoft";
        case CAProvider::GOOGLE: return "Google";
        case CAProvider::SAMSUNG: return "Samsung";
        case CAProvider::HUAWEI: return "Huawei";
        case CAProvider::XIAOMI: return "Xiaomi";
        default: return "Custom";
    }
}

QString SSLCertificateManager::certificateTypeToString(CertificateType type) const {
    switch (type) {
        case CertificateType::ROOT_CA: return "Root CA";
        case CertificateType::INTERMEDIATE_CA: return "Intermediate CA";
        case CertificateType::SERVER_CERT: return "Server Certificate";
        case CertificateType::CLIENT_CERT: return "Client Certificate";
        case CertificateType::WIREGUARD: return "WireGuard";
        default: return "Unknown";
    }
}

QString SSLCertificateManager::generateRandomSerial() {
    QString serial;
    for (int i = 0; i < 16; i++) {
        serial += QString::number(QRandomGenerator::global()->bounded(0, 16), 16).toUpper();
    }
    return serial;
}

} // namespace VirtualPhonePro
