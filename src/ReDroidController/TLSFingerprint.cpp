/**
 * @file TLSFingerprint.cpp
 * @brief Advanced TLS/SSL Fingerprinting Implementation - Enhanced v3.0
 * 
 * Complete TLS fingerprinting with:
 * - JA3/JA4/JA3H hash generation
 * - Multiple device profiles (Samsung, Pixel, Xiaomi, OnePlus, etc.)
 * - TLS 1.3 cipher suites and extensions
 * - HTTP/2 settings for JA3H
 */

#include "VirtualPhonePro/TLSFingerprint.hpp"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>

namespace VirtualPhonePro {

// ========================================================================
// STATIC HELPER FUNCTIONS
// ========================================================================

static QString joinInts(const QVector<quint16>& values, const QString& delimiter) {
    QStringList strList;
    for (quint16 v : values) {
        strList << QString::number(v);
    }
    return strList.join(delimiter);
}

// ========================================================================
// SINGLETON INITIALIZATION
// ========================================================================

TLSFingerprint* TLSFingerprint::s_instance = nullptr;

TLSFingerprint& TLSFingerprint::instance() {
    if (!s_instance) {
        s_instance = new TLSFingerprint();
    }
    return *s_instance;
}

TLSFingerprint::TLSFingerprint() {
    m_config = getAndroidTLSConfig();
    initializeAllProfiles();
    
    // Generate fingerprints
    m_currentJA3 = generateJA3Hash();
    m_currentJA4 = generateJA4Fingerprint();
    m_currentJA3H = generateJA3HHash(m_config.cipherSuites, 
                                       m_config.extensions, 
                                       m_config.http2Settings);
    
    qDebug() << "[TLSFingerprint] Initialized with Android TLS config";
}

bool TLSFingerprint::initialize(const QString& deviceModel) {
    DeviceProfile profile = DeviceProfile::ANDROID_DEFAULT;
    QString model = deviceModel.toLower();
    
    if (model.contains("samsung") || model.contains("galaxy")) {
        profile = DeviceProfile::SAMSUNG_GALAXY;
    } else if (model.contains("pixel")) {
        profile = DeviceProfile::GOOGLE_PIXEL;
    } else if (model.contains("xiaomi") || model.contains("redmi") || model.contains("poco")) {
        profile = DeviceProfile::XIAOMI;
    } else if (model.contains("oneplus")) {
        profile = DeviceProfile::ONEPLUS;
    } else if (model.contains("huawei") || model.contains("honor")) {
        profile = DeviceProfile::HUAWEI;
    } else if (model.contains("oppo") || model.contains("realme")) {
        profile = DeviceProfile::OPPO;
    } else if (model.contains("vivo")) {
        profile = DeviceProfile::VIVO;
    } else if (model.contains("moto") || model.contains("motorola")) {
        profile = DeviceProfile::MOTOROLA;
    } else if (model.contains("chrome")) {
        profile = DeviceProfile::CHROME_DESKTOP;
    }
    
    return initializeWithProfile(profile);
}

bool TLSFingerprint::initializeWithProfile(DeviceProfile profile) {
    // Check cache first
    if (m_profileCache.contains(profile)) {
        m_config = m_profileCache[profile];
    } else {
        m_config = getConfigForProfile(profile);
        cacheProfile(profile);
    }
    
    // Generate fingerprints
    m_currentJA3 = generateJA3Hash();
    m_currentJA4 = generateJA4Fingerprint();
    m_currentJA3H = generateJA3HHash(m_config.cipherSuites,
                                      m_config.extensions,
                                      m_config.http2Settings);
    
    qDebug() << "[TLSFingerprint] Initialized with profile:" << static_cast<int>(profile)
             << "JA3:" << m_currentJA3.left(16) << "..."
             << "JA4:" << m_currentJA4;
    
    return true;
}

void TLSFingerprint::resetToDefault() {
    initializeWithProfile(DeviceProfile::ANDROID_DEFAULT);
}

void TLSFingerprint::setTLSConfig(const OSTLSConfig& config) {
    m_config = config;
    m_currentJA3 = generateJA3Hash();
    m_currentJA4 = generateJA4Fingerprint();
    m_currentJA3H = generateJA3HHash(m_config.cipherSuites,
                                      m_config.extensions,
                                      m_config.http2Settings);
}

OSTLSConfig TLSFingerprint::getTLSConfig() const {
    return m_config;
}

QString TLSFingerprint::getCurrentJA3() const {
    return m_currentJA3;
}

QString TLSFingerprint::getCurrentJA4() const {
    return m_currentJA4;
}

QString TLSFingerprint::getCurrentJA3H() const {
    return m_currentJA3H;
}

void TLSFingerprint::setJA3Hash(const QString& hash) {
    m_currentJA3 = hash;
    qDebug() << "[TLSFingerprint] JA3 hash spoofed to:" << hash;
}

void TLSFingerprint::setJA4Fingerprint(const QString& fingerprint) {
    m_currentJA4 = fingerprint;
    qDebug() << "[TLSFingerprint] JA4 fingerprint spoofed to:" << fingerprint;
}

// ========================================================================
// JA3/JA4/JA3H GENERATION
// ========================================================================

QString TLSFingerprint::cipherSuitesToString(const QVector<quint16>& suites) {
    QStringList list;
    for (quint16 s : suites) {
        list << QString::number(s);
    }
    return list.join(",");
}

QString TLSFingerprint::cipherSuitesToHex(const QVector<quint16>& suites) {
    QStringList list;
    for (quint16 s : suites) {
        list << QString("%1").arg(s, 4, 16, QChar('0'));
    }
    return list.join("-");
}

QString TLSFingerprint::extensionsToString(const QVector<quint16>& exts) {
    QStringList list;
    for (quint16 e : exts) {
        list << QString::number(e);
    }
    return list.join(",");
}

QString TLSFingerprint::ellipticCurvesToString(const QVector<quint16>& curves) {
    QStringList list;
    for (quint16 c : curves) {
        list << QString::number(c);
    }
    return list.join(",");
}

QString TLSFingerprint::buildJA3String(quint16 tlsVersion,
                                       const QVector<quint16>& cipherSuites,
                                       const QVector<quint16>& extensions,
                                       const QVector<quint16>& ellipticCurves) {
    // JA3 format: TLSVersion,Random,SessionID,CipherSuites,Extensions,EllipticCurves,ECPointFormats
    
    QStringList components;
    
    // TLS Version (hex)
    components << QString::number(tlsVersion, 16).toUpper();
    
    // Random (32 bytes as hex) - normally from ClientHello
    // Using placeholder that will be replaced at runtime
    components << "random-placeholder";
    
    // Session ID (empty for new connections)
    components << "";
    
    // Cipher Suites (comma-separated)
    components << cipherSuitesToString(cipherSuites);
    
    // Extensions (comma-separated)
    components << extensionsToString(extensions);
    
    // Elliptic Curves
    components << ellipticCurvesToString(ellipticCurves);
    
    // EC Point Formats (uncompressed only)
    components << "0";
    
    return components.join(",");
}

QString TLSFingerprint::generateJA3Hash(const QVector<quint16>& cipherSuites,
                                       const QVector<quint16>& extensions,
                                       const QVector<quint16>& ellipticCurves,
                                       quint16 tlsVersion) {
    QString ja3String = buildJA3String(tlsVersion, cipherSuites, extensions, ellipticCurves);
    
    // Calculate MD5 hash
    QByteArray hash = QCryptographicHash::hash(
        ja3String.toUtf8(), QCryptographicHash::Md5);
    
    return hash.toHex();
}

QString TLSFingerprint::generateJA3Hash() {
    return generateJA3Hash(m_config.cipherSuites, 
                          m_config.extensions, 
                          m_config.ellipticCurves, 
                          m_config.maxVersion);
}

QString TLSFingerprint::generateJA4Fingerprint(quint16 tlsVersion,
                                              const QVector<quint16>& cipherSuites,
                                              const QVector<quint16>& extensions) {
    // JA4 format: t13d1516h2_8d65c997c30c_02
    // Components: protocol_tls_version + cipher_count + first_cipher + last_cipher
    //           _sni_ + alpn_ + extension_count_ + first_extension + last_extension
    
    QString ja4;
    
    // Protocol type (t = TLS, q = QUIC)
    ja4 = "t";
    
    // TLS version (13 = TLS 1.3, 12 = TLS 1.2)
    if (tlsVersion == 0x0304) {
        ja4 += "13";
    } else if (tlsVersion == 0x0303) {
        ja4 += "12";
    } else {
        ja4 += "11";  // TLS 1.0/1.1 (legacy)
    }
    
    // Cipher count (2 digits)
    ja4 += "d";
    ja4 += QString::number(cipherSuites.size()).rightJustified(2, '0');
    
    // First cipher suite (first 2 bytes, in hex)
    if (!cipherSuites.isEmpty()) {
        QString cipherHex = QString::number(cipherSuites.first(), 16).toUpper();
        ja4 += cipherHex.rightJustified(4, '0').left(2);
    }
    
    // Last cipher suite (first 2 bytes)
    if (cipherSuites.size() > 1) {
        ja4 += "h";
        QString cipherHex = QString::number(cipherSuites.last(), 16).toUpper();
        ja4 += cipherHex.rightJustified(4, '0').left(2);
    } else {
        ja4 += "h_";  // No second cipher
    }
    
    // SNI indicator (s = present, i = not present)
    ja4 += "_";
    ja4 += m_config.sniPattern.isEmpty() ? "i" : "s";
    
    // ALPN count (2 chars, lowercase = 2 protocols)
    ja4 += "_";
    if (m_config.alpnProtocols.isEmpty()) {
        ja4 += "00";
    } else {
        ja4 += "0" + QString::number(m_config.alpnProtocols.size());
    }
    
    // Extension count (2 digits)
    ja4 += "_";
    ja4 += QString::number(extensions.size()).rightJustified(2, '0');
    
    // First and last extension codes
    if (!extensions.isEmpty()) {
        ja4 += "_";
        ja4 += QString::number(extensions.first(), 16).toUpper().rightJustified(2, '0');
        if (extensions.size() > 1) {
            ja4 += QString::number(extensions.last(), 16).toUpper().rightJustified(2, '0');
        }
    }
    
    return ja4;
}

QString TLSFingerprint::generateJA4Fingerprint() {
    return generateJA4Fingerprint(m_config.maxVersion, 
                                m_config.cipherSuites, 
                                m_config.extensions);
}

QString TLSFingerprint::generateJA3HHash(const QVector<quint16>& http2Ciphers,
                                       const QVector<quint16>& extensions,
                                       const HTTP2Settings& settings) {
    // JA3H format: HTTP/2 settings + window updates + frame size
    
    QStringList components;
    
    // HTTP/2 SETTINGS frame
    // Format: settings_option:value,settings_option:value,...
    QStringList settingsList;
    settingsList << QString("1:%1").arg(settings.headerTableSize);
    settingsList << QString("3:%1").arg(settings.maxConcurrentStreams);
    settingsList << QString("4:%1").arg(settings.initialWindowSize);
    settingsList << QString("5:%1").arg(settings.maxFrameSize);
    
    components << settingsList.join(",");
    
    // HTTP/2 Window Update frames (in first 3 packets)
    components << "0";  // Number of window updates in first 3 packets
    
    // Frame size limit
    components << QString::number(settings.maxFrameSize);
    
    QString ja3hString = components.join(",");
    
    // Calculate MD5 hash
    QByteArray hash = QCryptographicHash::hash(
        ja3hString.toUtf8(), QCryptographicHash::Md5);
    
    return hash.toHex();
}

OSTLSConfig TLSFingerprint::getAndroidTLSConfig() {
    OSTLSConfig config;
    config.osName = "Android";
    config.browser = "Chrome";
    config.version = "14";
    
    // TLS 1.3
    config.maxVersion = 0x0304;
    config.minVersion = 0x0301;
    
    // Android cipher suites
    config.cipherSuites = {
        0x1301, // TLS_AES_128_GCM_SHA256
        0x1302, // TLS_AES_256_GCM_SHA384
        0x1303, // TLS_CHACHA20_POLY1305_SHA256
        0xC02C, // ECDHE_ECDSA_AES_256_GCM_SHA384
        0xC02B, // ECDHE_ECDSA_AES_128_GCM_SHA256
        0xC0AD, // ECDHE_ECDSA_AES_256_GCM_SHA384
        0xC0AC, // ECDHE_ECDSA_AES_128_GCM_SHA256
        0xCCA9, // ECDHE_ECDSA_CHACHA20_POLY1305_SHA256
        0xC02F, // ECDHE_RSA_AES_256_GCM_SHA384
        0xC02E, // ECDHE_RSA_AES_128_GCM_SHA256
        0xC0A9, // ECDHE_RSA_CHACHA20_POLY1305_SHA256
        0xC024, // ECDHE_ECDSA_AES_256_CBC_SHA384
        0xC023, // ECDHE_ECDSA_AES_128_CBC_SHA256
        0xC028, // ECDHE_RSA_AES_256_CBC_SHA384
        0xC027, // ECDHE_RSA_AES_128_CBC_SHA256
        0x006B, // RSA_AES_256_CBC_SHA
        0x003F  // RSA_AES_128_CBC_SHA
    };
    
    config.extensions = {23, 43, 45, 16};
    config.ellipticCurves = {29, 23, 30, 25, 24};
    config.ecPointFormats = {0};
    
    return config;
}

OSTLSConfig TLSFingerprint::getSamsungTLSConfig() {
    OSTLSConfig config = getAndroidTLSConfig();
    config.osName = "Samsung";
    config.browser = "Samsung Internet";
    config.version = "22.0";
    return config;
}

OSTLSConfig TLSFingerprint::getChromeTLSConfig() {
    OSTLSConfig config;
    config.osName = "Windows";
    config.browser = "Chrome";
    config.version = "120";
    
    config.maxVersion = 0x0304;
    config.minVersion = 0x0301;
    
    config.cipherSuites = {
        0x1301, 0x1302, 0x1303,
        0xC02C, 0xC02B, 0xC02F, 0xC02E,
        0xCCA9, 0xCCA8, 0xC0A9, 0xC0A8,
        0xC024, 0xC023, 0xC028, 0xC027
    };
    
    config.extensions = {23, 43, 45, 16, 5, 13, 10};
    config.ellipticCurves = {29, 23, 30, 25, 24, 27, 28};
    config.ecPointFormats = {0};
    
    return config;
}

bool TLSFingerprint::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Apply TLS-related properties
    QStringList commands = {
        "setprop net.ssl.version TLSv1.3",
        "setprop net.ssl.cipher_suites " + cipherSuitesToString(m_config.cipherSuites),
        "setprop debug.tls.ja3 " + m_currentJA3,
        "setprop debug.tls.ja4 " + m_currentJA4
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

QString TLSFingerprint::getCurrentJA3() {
    return m_currentJA3;
}

QString TLSFingerprint::getCurrentJA4() {
    return m_currentJA4;
}

} // namespace VirtualPhonePro
