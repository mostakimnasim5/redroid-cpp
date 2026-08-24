/**
 * @file LocaleTimezoneManager.h
 * @brief Automatic Locale & Timezone Synchronization
 * @version 2.0.0
 * 
 * Automatically syncs Android locale, timezone, language, and carrier
 * based on the proxy's geolocation data.
 */


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

// What kind of access network the synced instance presents. ISP/residential
// proxies (GUI mode 1) look like a phone on home/office WiFi — no SIM/carrier
// story at all. Mobile proxies (GUI mode 2) look like cellular. Default
// Cellular keeps the existing mode-2 behavior untouched.
enum class SyncNetworkKind { Cellular, WiFi };

// Deterministic WiFi network identity for ISP-proxy (WiFi) instances.
// Derived from the profile-anchored seed — same profile -> same SSID/BSSID
// across reboots; two profiles never share an identity.
struct WifiNetworkConfig {
    QString ssid;
    QString bssid;            // AP MAC, locally administered
    QString gateway;
    QString dns1;
    QString dns2;
    QString frequency;        // "2.4 GHz" or "5 GHz"
    int linkSpeedMbps = 0;
    int signalDbm = 0;
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
    
#ifndef LTM_NO_CONTROLLER
    /**
     * @brief Query geolocation from proxy IP
     */
    bool queryGeolocation(const QString& instanceId);
    
    /**
     * @brief Query geolocation by IP directly
     */
    GeoLocation queryGeoLocationByIP(const QString& ip);
#endif // LTM_NO_CONTROLLER
    
    /**
     * @brief Get current geolocation
     */
    GeoLocation getGeoLocation(const QString& instanceId) const;
    
#ifndef LTM_NO_CONTROLLER
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

    /**
     * @brief Apply a WiFi network identity (ISP/residential proxy mode).
     *
     * Presents the instance as a no-SIM phone on home/office WiFi: mobile
     * data off, WiFi on, SIM-operator props absent, deterministic
     * SSID/BSSID/gateway/frequency applied. Never touches carrier props.
     */
    bool applyWifiNetwork(const QString& instanceId, const WifiNetworkConfig& wifi);
#endif // LTM_NO_CONTROLLER

    /**
     * @brief Derive the deterministic WiFi identity for a profile-anchored
     *        seed (e.g. instanceId). Pure logic — no instance access — so it
     *        is unit-testable in isolation. Same seed -> same config; two
     *        seeds -> two configs.
     */
    WifiNetworkConfig generateWifiNetworkConfig(const QString& seed,
                                                const QString& countryCode) const;
    
    // =========================================================================
    // Auto Sync (Proxy -> Locale)
    // =========================================================================
    
    /**
     * @brief Sync everything based on proxy
     * @param kind  Cellular (mobile proxy, mode 2 — SIM/carrier applied) or
     *              WiFi (ISP/residential proxy, mode 1 — WiFi identity
     *              applied, no SIM/carrier at all).
     * @return true if successful
     */
    bool syncFromProxy(const QString& instanceId,
                       SyncNetworkKind kind = SyncNetworkKind::Cellular);

    /**
     * @brief Re-check the proxy's exit IP and re-sync if it changed.
     *
     * Mobile proxies rotate their exit IP over time. This queries the current
     * exit IP through the same PRIMARY tunnel path as syncFromProxy() (proxy
     * -> ip-api.com, empty target) and, when the exit IP differs from the
     * stored geolocation (or the instance was never synced), re-runs the full
     * syncFromProxy() so carrier/timezone/locale track the new egress.
     *
     * The kind used for the re-sync is the one recorded by the last
     * syncFromProxy() call (stored per instance); pass @p kind only for the
     * first sync of an instance that has no recorded kind yet.
     *
     * @return true if a re-sync was performed and succeeded; false when the
     *         exit IP is unchanged (no-op) or the re-sync failed.
     */
    bool resyncFromProxy(const QString& instanceId,
                         SyncNetworkKind kind = SyncNetworkKind::Cellular);
    
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
     * @param seed  Profile-anchored seed (e.g. instanceId) that deterministically
     *              selects one of the country's carriers. Same seed -> same carrier.
     */
    CarrierConfig getCarrierForLocation(const QString& country, const QString& region,
                                        const QString& seed = QString()) const;
    
    /**
     * @brief Get current state as JSON
     */
    QJsonObject getStateAsJson(const QString& instanceId) const;
    
signals:
    void geoLocationUpdated(const QString& instanceId, const GeoLocation& location);
    void syncCompleted(const QString& instanceId, bool success);
    void error(const QString& instanceId, const QString& message);

private slots:
#ifndef LTM_NO_CONTROLLER
    void onGeoQueryFinished(QNetworkReply* reply);
#endif // LTM_NO_CONTROLLER

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
        SyncNetworkKind networkKind = SyncNetworkKind::Cellular;
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
