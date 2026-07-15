/**
 * @file LocaleTimezoneManager.h
 * @brief Automatic Locale & Timezone Synchronization
 * @version 2.0.0
 * 
 * Automatically syncs Android locale, timezone, language, and carrier
 * based on the proxy's geolocation data.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_LOCALE_TIMEZONE_MANAGER_H
#define VIRTUALPHONEPRO_LOCALE_TIMEZONE_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMutex>

namespace VirtualPhonePro {

// Proxy information
struct ProxyInfo {
    QString host;
    int port;
    QString type;              // "socks5" or "http"
    QString username;
    QString password;
    bool isValid;
};

// Geolocation data from proxy
struct GeoLocation {
    QString ipAddress;
    QString country;
    QString countryCode;       // ISO 3166-1 alpha-2 (e.g., "US")
    QString region;
    QString city;
    QString postalCode;
    QString timezone;
    QString timezoneOffset;    // e.g., "+05:30" or "-08:00"
    double latitude;
    double longitude;
    QString isp;
    QString org;
    QString asn;
    bool isValid;
    qint64 queriedAt;
};

// Locale configuration
struct LocaleConfig {
    QString language;          // ISO 639-1 (e.g., "en")
    QString script;            // ISO 15924 (e.g., "Latn")
    QString region;           // ISO 3166-1 alpha-2 (e.g., "US")
    QString localeString;      // Full locale (e.g., "en_US")
    QString displayLanguage;
    QString displayRegion;
};

// Carrier configuration
struct CarrierConfig {
    QString name;
    QString shortName;
    QString mcc;              // Mobile Country Code (3 digits)
    QString mnc;              // Mobile Network Code (2-3 digits)
    QString countryCode;
    QString country;
    QString networkType;       // 4G, 5G, LTE, etc.
};

class LocaleTimezoneManager : public QObject {
    Q_OBJECT

public:
    static LocaleTimezoneManager& instance();
    
    // =========================================================================
    // Proxy Configuration
    // =========================================================================
    
    /**
     * @brief Set proxy for instance
     */
    bool setProxy(const QString& instanceId, const ProxyInfo& proxy);
    
    /**
     * @brief Remove proxy
     */
    bool removeProxy(const QString& instanceId);
    
    /**
     * @brief Get current proxy
     */
    ProxyInfo getProxy(const QString& instanceId) const;
    
    // =========================================================================
    // Geolocation Sync
    // =========================================================================
    
    /**
     * @brief Query geolocation from proxy IP
     */
    bool queryGeolocation(const QString& instanceId);
    
    /**
     * @brief Query geolocation by IP directly
     */
    GeoLocation queryGeoLocationByIP(const QString& ip);
    
    /**
     * @brief Get current geolocation
     */
    GeoLocation getGeoLocation(const QString& instanceId) const;
    
    /**
     * @brief Apply locale settings to instance
     */
    bool applyLocale(const QString& instanceId, const LocaleConfig& locale);
    
    /**
     * @brief Apply timezone to instance
     */
    bool applyTimezone(const QString& instanceId, const QString& timezone);
    
    /**
     * @brief Apply carrier configuration
     */
    bool applyCarrier(const QString& instanceId, const CarrierConfig& carrier);
    
    // =========================================================================
    // Auto Sync (Proxy -> Locale)
    // =========================================================================
    
    /**
     * @brief Sync everything based on proxy
     * @return true if successful
     */
    bool syncFromProxy(const QString& instanceId);
    
    /**
     * @brief Sync using manual coordinates
     */
    bool syncFromCoordinates(const QString& instanceId, double lat, double lon);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get timezone for coordinates
     */
    QString getTimezoneForCoordinates(double lat, double lon) const;
    
    /**
     * @brief Get country code from coordinates
     */
    QString getCountryFromCoordinates(double lat, double lon) const;
    
    /**
     * @brief Get locale for country
     */
    LocaleConfig getLocaleForCountry(const QString& countryCode) const;
    
    /**
     * @brief Get carrier for location
     */
    CarrierConfig getCarrierForLocation(const QString& country, const QString& region) const;
    
    /**
     * @brief Get current state as JSON
     */
    QJsonObject getStateAsJson(const QString& instanceId) const;
    
signals:
    void geoLocationUpdated(const QString& instanceId, const GeoLocation& location);
    void syncCompleted(const QString& instanceId, bool success);
    void error(const QString& instanceId, const QString& message);

private slots:
    void onGeoQueryFinished(QNetworkReply* reply);

private:
    LocaleTimezoneManager(QObject* parent = nullptr);
    ~LocaleTimezoneManager();
    Q_DISABLE_COPY(LocaleTimezoneManager)
    
    // Internal methods
    bool queryGeoLocationInternal(const QString& instanceId);
    QString detectTimezoneFromIP(const QString& ip);
    QString generateLocaleFromCountry(const QString& countryCode);
    QString getMCCForCountry(const QString& countryCode);
    
    // Carrier selection
    QString selectCarrier(const QString& country, const QString& region);
    
    // Instance state
    struct InstanceLocaleState {
        ProxyInfo proxy;
        GeoLocation geoLocation;
        LocaleConfig locale;
        CarrierConfig carrier;
        bool isSynced;
        qint64 lastSyncTime;
    };
    
    QMap<QString, InstanceLocaleState*> m_instanceStates;
    mutable QMutex m_mutex;
    QNetworkAccessManager* m_networkManager;
    
    // Geolocation API (using ip-api.com - free tier)
    static constexpr const char* GEO_API_URL = "http://ip-api.com/json/";
    static constexpr const int GEO_QUERY_TIMEOUT_MS = 10000;
    
    // Static timezone mapping (sample)
    static const QMap<QString, QString> COUNTRY_TO_TIMEZONE;
    static const QMap<QString, QString> COUNTRY_TO_LOCALE;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_LOCALE_TIMEZONE_MANAGER_H
