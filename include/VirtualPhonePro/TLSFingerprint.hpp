/**
 * @file TLSFingerprint.hpp
 * @brief Advanced TLS/SSL Fingerprinting Prevention - Enhanced v3.0
 * 
 * Complete TLS fingerprinting implementation with:
 * - JA3 hash generation and spoofing
 * - JA4 fingerprint support
 * - JA3H (HTTP/2) fingerprinting
 * - Multiple device profiles (Samsung, Pixel, Xiaomi, OnePlus, etc.)
 * - TLS 1.3 extension handling
 * - ALPN spoofing
 * - SNI configuration
 * - Certificate transparency
 */

#pragma once

#ifndef VIRTUALPHONEPRO_TLS_FINGERPRINT_H
#define VIRTUALPHONEPRO_TLS_FINGERPRINT_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>

namespace VirtualPhonePro {

// TLS Extension types (RFC 5246, 6066, 8446)
enum class TLSExtension {
    SERVER_NAME = 0,                    // SNI
    MAX_FRAGMENT_LENGTH = 1,
    CLIENT_CERTIFICATE_URL = 2,
    TRUNCATED_HMAC = 4,
    STATUS_REQUEST = 5,                 // OCSP stapling
    ELLIPTIC_CURVES = 10,              // Supported groups
    EC_POINT_FORMATS = 11,
    SIGNATURE_ALGORITHMS = 13,
    USE_SRTP = 14,
    APPLICATION_LAYER_PROTOCOL = 16,   // ALPN
    SIGNED_CERTIFICATE_TIMESTAMP = 18,  // SCT
    PADDING = 21,
    ENCRYPTED_CLIENT_HELLO = 34,
    SESSION_TICKET = 35,
    TLS_1_3_COMPAT_MODE = 39,
    PSK_KEY_EXCHANGE_MODES = 41,
    RECORD_SIZE_LIMIT = 28,
    QUIC_TRANSPORT_PARAMETERS = 57,
    ENCRYPTED_CLIENT_HELLO_V2 = 65037,
    EARLY_DATA = 5,
    SUPPORTED_VERSIONS = 43,
    COOKIE = 44,
    PSK_BINDERS = 45
};

// TLS Alert codes
enum class TLSAlert {
    CLOSE_NOTIFY = 0,
    UNEXPECTED_MESSAGE = 10,
    BAD_RECORD_MAC = 20,
    DECRYPTION_FAILED = 21,
    RECORD_OVERFLOW = 22,
    DECOMPRESSION_FAILURE = 30,
    HANDSHAKE_FAILURE = 40,
    NO_CERT = 41,
    BAD_CERT = 42,
    UNSUPPORTED_CERT = 43,
    CERT_REVOKED = 44,
    CERT_EXPIRED = 45,
    CERT_UNKNOWN = 46,
    ILLEGAL_PARAMETER = 47,
    UNKNOWN_CA = 48,
    ACCESS_DENIED = 49,
    DECODE_ERROR = 50,
    DECRYPT_ERROR = 51,
    EXPORT_RESTRICTION = 60,
    INTERNAL_ERROR = 80,
    INAPPROPRIATE_FALLBACK = 86,
    USER_CANCELED = 90,
    NO_RENEGOTIATION = 100,
    MISSING_EXTENSION = 109,
    UNSUPPORTED_EXTENSION = 110
};

// Known JA3 hashes for various browsers/apps
struct KnownJA3Hash {
    QString hash;
    QString browser;
    QString version;
    QString os;
};

// HTTP/2 settings (for JA3H)
struct HTTP2Settings {
    quint16 headerTableSize;
    quint32 maxHeaderListSize;
    quint32 initialWindowSize;
    quint16 maxFrameSize;
    quint8 maxConcurrentStreams;
};

// OS-specific TLS configurations
struct OSTLSConfig {
    QString osName;
    QString manufacturer;
    QString browser;
    QString version;
    
    // TLS Version
    quint16 maxVersion;    // TLS 1.3 = 0x0304
    quint16 minVersion;    // TLS 1.0 = 0x0301
    
    // TLS 1.3 cipher suites (IANA assigned)
    QVector<quint16> cipherSuites;
    
    // TLS extensions
    QVector<quint16> extensions;
    
    // Elliptic curves / supported groups (TLS 1.3)
    QVector<quint16> ellipticCurves;
    
    // EC point formats
    QVector<quint8> ecPointFormats;
    
    // ALPN protocols
    QStringList alpnProtocols;
    
    // SNI hostname pattern
    QString sniPattern;
    
    // HTTP/2 settings (for JA3H)
    HTTP2Settings http2Settings;
    
    // Pre-computed JA3 hash
    QString ja3Hash;
    
    // Pre-computed JA4 fingerprint
    QString ja4Fingerprint;
    
    // JA3H hash for HTTP/2
    QString ja3hHash;
};

// Profile presets for common devices
enum class TLSProfile {
    ANDROID_DEFAULT,
    SAMSUNG_GALAXY,
    GOOGLE_PIXEL,
    XIAOMI,
    ONEPLUS,
    HUAWEI,
    OPPO,
    VIVO,
    REALME,
    MOTOROLA,
    LG,
    SONY,
    ASUS,
    CHROME_DESKTOP,
    FIREFOX_DESKTOP,
    SAFARI_DESKTOP,
    CUSTOM
};

class TLSFingerprint {
public:
    static TLSFingerprint& instance();
    
    // ========================================================================
    // INITIALIZATION & CONFIGURATION
    // ========================================================================
    
    /**
     * @brief Initialize with device model name (auto-detect profile)
     */
    bool initialize(const QString& deviceModel);
    
    /**
     * @brief Initialize with specific profile
     */
    bool initializeWithProfile(TLSProfile profile);
    
    /**
     * @brief Get current configuration
     */
    OSTLSConfig getTLSConfig() const;
    
    /**
     * @brief Set custom TLS configuration
     */
    void setTLSConfig(const OSTLSConfig& config);
    
    /**
     * @brief Reset to default Android configuration
     */
    void resetToDefault();
    
    // ========================================================================
    // JA3 FINGERPRINTING
    // ========================================================================
    
    /**
     * @brief Generate JA3 hash from cipher suites, extensions, and curves
     * 
     * JA3 format: TLSVersion,Random,SessionID,CipherSuites,Extensions,
     *             EllipticCurves,ECPointFormats
     */
    QString generateJA3Hash(const QVector<quint16>& cipherSuites,
                           const QVector<quint16>& extensions,
                           const QVector<quint16>& ellipticCurves,
                           quint16 tlsVersion);
    
    /**
     * @brief Generate JA3 hash from current configuration
     */
    QString generateJA3Hash();
    
    /**
     * @brief Get current JA3 hash
     */
    QString getCurrentJA3() const;
    QString getCurrentJA3();
    
    /**
     * @brief Set JA3 hash (for spoofing)
     */
    void setJA3Hash(const QString& hash);
    
    // ========================================================================
    // JA4 FINGERPRINTING
    // ========================================================================
    
    /**
     * @brief Generate JA4 fingerprint
     * 
     * JA4 format: t13d1516h2_8d65c997c30c_02
     * t<protocol>_<tls_version><cipher_count>_<first_cipher>...
     */
    QString generateJA4Fingerprint(quint16 tlsVersion,
                                   const QVector<quint16>& cipherSuites,
                                   const QVector<quint16>& extensions);
    
    /**
     * @brief Generate JA4 fingerprint from current configuration
     */
    QString generateJA4Fingerprint();
    
    /**
     * @brief Get current JA4 fingerprint
     */
    QString getCurrentJA4() const;
    QString getCurrentJA4();
    
    /**
     * @brief Set JA4 fingerprint (for spoofing)
     */
    void setJA4Fingerprint(const QString& fingerprint);
    
    // ========================================================================
    // JA3H FINGERPRINTING (HTTP/2)
    // ========================================================================
    
    /**
     * @brief Generate JA3H hash for HTTP/2 connections
     */
    QString generateJA3HHash(const QVector<quint16>& http2Ciphers,
                            const QVector<quint16>& extensions,
                            const HTTP2Settings& settings);
    
    /**
     * @brief Get current JA3H hash
     */
    QString getCurrentJA3H() const;
    
    // ========================================================================
    // DEVICE PROFILE CONFIGS
    // ========================================================================
    
    /**
     * @brief Get TLS configuration for Android (default)
     */
    static OSTLSConfig getAndroidTLSConfig();
    
    /**
     * @brief Get TLS configuration for Samsung devices
     */
    static OSTLSConfig getSamsungTLSConfig();
    
    /**
     * @brief Get TLS configuration for Google Pixel devices
     */
    static OSTLSConfig getPixelTLSConfig();
    
    /**
     * @brief Get TLS configuration for Xiaomi devices
     */
    static OSTLSConfig getXiaomiTLSConfig();
    
    /**
     * @brief Get TLS configuration for OnePlus devices
     */
    static OSTLSConfig getOnePlusTLSConfig();
    
    /**
     * @brief Get TLS configuration for Huawei devices
     */
    static OSTLSConfig getHuaweiTLSConfig();
    
    /**
     * @brief Get TLS configuration for Chrome Desktop
     */
    static OSTLSConfig getChromeTLSConfig();
    
    /**
     * @brief Get TLS configuration for Firefox Desktop
     */
    static OSTLSConfig getFirefoxTLSConfig();
    
    /**
     * @brief Get TLS configuration for Safari Desktop
     */
    static OSTLSConfig getSafariTLSConfig();
    
    /**
     * @brief Get TLS configuration by profile enum
     */
    static OSTLSConfig getConfigForProfile(TLSProfile profile);
    
    // ========================================================================
    // LOOKUP & UTILITIES
    // ========================================================================
    
    /**
     * @brief Identify browser from JA3 hash
     */
    QString identifyBrowserFromJA3(const QString& ja3Hash);
    
    /**
     * @brief Get list of known JA3 hashes
     */
    QVector<KnownJA3Hash> getKnownJA3Hashes();
    
    /**
     * @brief Convert cipher suites to hex string (for JA3)
     */
    static QString cipherSuitesToHex(const QVector<quint16>& suites);
    
    /**
     * @brief Convert cipher suites to comma-separated string
     */
    static QString cipherSuitesToString(const QVector<quint16>& suites);
    
    /**
     * @brief Convert extensions to string (for JA3)
     */
    static QString extensionsToString(const QVector<quint16>& exts);
    
    /**
     * @brief Convert elliptic curves to string (for JA3)
     */
    static QString ellipticCurvesToString(const QVector<quint16>& curves);
    
    // ========================================================================
    // INSTANCE APPLICATION
    // ========================================================================
    
    /**
     * @brief Apply TLS configuration to Android instance
     */
    bool applyToInstance(const QString& instanceId);
    
    /**
     * @brief Get all TLS-related properties for instance
     */
    QJsonObject getTLSProperties(const QString& instanceId);
    
private:
    static TLSFingerprint* s_instance;
    TLSFingerprint();
    
    OSTLSConfig m_config;
    QString m_currentJA3;
    QString m_currentJA4;
    QString m_currentJA3H;
    
    QMap<TLSProfile, OSTLSConfig> m_profileCache;
    
    // Helper methods
    QString buildJA3String(quint16 tlsVersion,
                          const QVector<quint16>& cipherSuites,
                          const QVector<quint16>& extensions,
                          const QVector<quint16>& ellipticCurves);
    
    void cacheProfile(TLSProfile profile);
    void initializeAllProfiles();

    static TLSFingerprint* s_instance;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_TLS_FINGERPRINT_H
