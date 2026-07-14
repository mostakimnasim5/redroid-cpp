/**
 * @file TLSFingerprint.cpp
 * @brief Advanced TLS/SSL Fingerprinting Implementation
 * @version 2.0.0
 */

#include "VirtualPhonePro/TLSFingerprint.hpp"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>

namespace VirtualPhonePro {

TLSFingerprint* TLSFingerprint::s_instance = nullptr;

TLSFingerprint& TLSFingerprint::instance() {
    if (!s_instance) {
        s_instance = new TLSFingerprint();
    }
    return *s_instance;
}

TLSFingerprint::TLSFingerprint() {
    m_config = getAndroidTLSConfig();
    m_currentJA3 = generateJA3Hash(m_config.cipherSuites,
                                   QVector<quint8>::fromVector(QVector<quint8>() << 23 << 10 << 13 << 11),
                                   m_config.ellipticCurves,
                                   m_config.maxVersion);
    m_currentJA4 = generateJA4Fingerprint(m_config.maxVersion,
                                         m_config.cipherSuites,
                                         QVector<quint8>::fromVector(QVector<quint8>() << 16 << 23));
}

bool TLSFingerprint::initialize(const QString& deviceModel) {
    if (deviceModel.contains("samsung", Qt::CaseInsensitive)) {
        m_config = getSamsungTLSConfig();
    } else if (deviceModel.contains("chrome", Qt::CaseInsensitive)) {
        m_config = getChromeTLSConfig();
    } else {
        m_config = getAndroidTLSConfig();
    }
    
    m_currentJA3 = generateJA3Hash(m_config.cipherSuites,
                                   QVector<quint8>::fromVector(QVector<quint8>() << 23 << 10 << 13 << 11),
                                   m_config.ellipticCurves,
                                   m_config.maxVersion);
    m_currentJA4 = generateJA4Fingerprint(m_config.maxVersion,
                                         m_config.cipherSuites,
                                         QVector<quint8>::fromVector(QVector<quint8>() << 16 << 23));
    
    return true;
}

QString TLSFingerprint::cipherSuitesToString(const QVector<quint16>& suites) {
    QStringList list;
    for (quint16 s : suites) {
        list << QString::number(s);
    }
    return list.join(",");
}

QString TLSFingerprint::extensionsToString(const QVector<quint8>& exts) {
    QStringList list;
    for (quint8 e : exts) {
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

QString TLSFingerprint::generateJA3Hash(const QVector<quint16>& cipherSuites,
                                       const QVector<quint8>& extensions,
                                       const QVector<quint16>& ellipticCurves,
                                       quint16 tlsVersion) {
    // JA3 format: TLSVersion,Random,SessionID,CipherSuites,Extensions,EllipticCurves,ECPointFormats
    
    QStringList components;
    
    // TLS Version
    components << QString::number(tlsVersion);
    
    // Random (32 bytes) - we'll use a placeholder
    components << "0303" + QString("00").repeated(64);
    
    // Session ID (empty for now)
    components << "";
    
    // Cipher Suites
    components << cipherSuitesToString(cipherSuites);
    
    // Extensions
    components << extensionsToString(extensions);
    
    // Elliptic Curves
    components << ellipticCurvesToString(ellipticCurves);
    
    // EC Point Formats
    components << "01"; // uncompressed
    
    QString ja3String = components.join(",");
    
    // Calculate MD5 hash
    QByteArray hash = QCryptographicHash::hash(
        ja3String.toUtf8(), QCryptographicHash::Md5);
    
    return hash.toHex();
}

QString TLSFingerprint::generateJA4Fingerprint(quint16 tlsVersion,
                                              const QVector<quint16>& cipherSuites,
                                              const QVector<quint8>& extensions) {
    // JA4 format: t13d1516h2_8d65c997c30c_02
    // t<version>_<first 2 ciphers>_<last 2 ciphers>_<alpn>_<sni>_<extension count>_<a>,<b>_<extension list>
    
    QString ja4;
    
    // Protocol version
    if (tlsVersion == 0x0304) {
        ja4 = "t13";
    } else {
        ja4 = "t12";
    }
    
    // Version + cipher count + first cipher (first 2 bytes)
    ja4 += "d";
    ja4 += QString::number(cipherSuites.size()).rightJustified(2, '0');
    if (!cipherSuites.isEmpty()) {
        ja4 += QString::number(cipherSuites.first()).rightJustified(4, '0').left(2);
    }
    
    // Last cipher (first 2 bytes)
    if (cipherSuites.size() > 1) {
        ja4 += "h";
        ja4 += QString::number(cipherSuites.last()).rightJustified(4, '0').left(2);
    }
    
    // SNI present (s) or not (i)
    ja4 += "_";
    ja4 += "s";
    
    // ALPN count (2 chars)
    ja4 += "_";
    ja4 += "00";
    
    // Extension count (2 digits)
    ja4 += "_";
    ja4 += QString::number(extensions.size()).rightJustified(2, '0');
    
    // First and last extension (first 2 bytes each)
    if (!extensions.isEmpty()) {
        ja4 += "_";
        ja4 += QString::number(extensions.first()).rightJustified(4, '0').left(2);
        if (extensions.size() > 1) {
            ja4 += QString::number(extensions.last()).rightJustified(4, '0').left(2);
        }
    }
    
    return ja4;
}

OSTLSConfig TLSFingerprint::getTLSConfig() {
    return m_config;
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
