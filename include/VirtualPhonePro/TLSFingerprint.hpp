/**
 * @file TLSFingerprint.hpp
 * @brief Advanced TLS/SSL Fingerprinting Prevention
 * @version 2.0.0
 * 
 * Implements JA3/JA4 TLS fingerprinting bypass.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_TLS_FINGERPRINT_H
#define VIRTUALPHONEPRO_TLS_FINGERPRINT_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

namespace VirtualPhonePro {

// TLS Extension types
enum class TLSExtension {
    SERVER_NAME = 0,
    MAX_FRAGMENT_LENGTH = 1,
    CLIENT_CERTIFICATE_URL = 2,
    TRUNCATED_HMAC = 4,
    STATUS_REQUEST = 5,
    ELLIPTIC_CURVES = 10,
    EC_POINT_FORMATS = 11,
    SIGNATURE_ALGORITHMS = 13,
    USE_SRTP = 14,
    APPLICATION_LAYER_PROTOCOL = 16,
    SIGNED_CERTIFICATE_TIMESTAMP = 18,
    PADDING = 21,
    SESSION_TICKET = 35,
    TLS_1_3_COMPAT_MODE = 39,
    PSK_KEY_EXCHANGE_MODES = 41,
    RECORD_SIZE_LIMIT = 28,
    QUIC_TRANSPORT_PARAMETERS = 57,
    ENCRYPTED_CLIENT_HELLO = 65037
};

// OS-specific TLS configurations
struct OSTLSConfig {
    QString osName;
    QString browser;
    QString version;
    
    // TLS Version
    quint16 maxVersion;
    quint16 minVersion;
    
    // Cipher suites
    QVector<quint16> cipherSuites;
    
    // Extensions
    QVector<int> extensions;
    
    // Elliptic curves
    QVector<quint16> ellipticCurves;
    
    // EC point formats
    QVector<quint8> ecPointFormats;
    
    // JA3 string components
    QString ja3Components;
};

class TLSFingerprint {
public:
    static TLSFingerprint& instance();
    
    // Initialize with device profile
    bool initialize(const QString& deviceModel);
    
    // JA3 Hash Generation
    QString generateJA3Hash(const QVector<quint16>& cipherSuites,
                          const QVector<quint8>& extensions,
                          const QVector<quint16>& ellipticCurves,
                          quint16 tlsVersion);
    
    // JA4 Fingerprint Generation
    QString generateJA4Fingerprint(quint16 tlsVersion,
                                  const QVector<quint16>& cipherSuites,
                                  const QVector<quint8>& extensions);
    
    // Get device-specific TLS config
    OSTLSConfig getTLSConfig();
    
    // Get default configs for different devices
    static OSTLSConfig getAndroidTLSConfig();
    static OSTLSConfig getSamsungTLSConfig();
    static OSTLSConfig getChromeTLSConfig();
    
    // Apply TLS configuration to instance
    bool applyToInstance(const QString& instanceId);
    
    // Get current JA3 hash
    QString getCurrentJA3();
    
    // Get current JA4 hash
    QString getCurrentJA4();
    
private:
    TLSFingerprint();
    
    OSTLSConfig m_config;
    QString m_currentJA3;
    QString m_currentJA4;
    
    QString cipherSuitesToString(const QVector<quint16>& suites);
    QString extensionsToString(const QVector<quint8>& exts);
    QString ellipticCurvesToString(const QVector<quint16>& curves);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_TLS_FINGERPRINT_H
