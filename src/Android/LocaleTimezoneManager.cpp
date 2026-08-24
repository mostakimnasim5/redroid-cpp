/**
 * @file LocaleTimezoneManager.cpp
 * @brief Automatic Locale & Timezone Synchronization Implementation
 */

#include "Android/LocaleTimezoneManager.h"
#ifndef LTM_NO_CONTROLLER
#include "VirtualPhonePro/ReDroidController.hpp"
#endif

#include <QDebug>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QDateTime>
#include <QMutexLocker>
#include <QTimer>

namespace {

// Blocking geo lookup against ip-api.com. When targetIp is empty, ip-api
// resolves the caller's (exit) IP — this is what makes the proxy-tunnel path
// possible. Returns an invalid GeoLocation on error.
VirtualPhonePro::GeoLocation executeGeoQuery(QNetworkAccessManager& nam,
                                             const QUrl& url,
                                             int timeoutMs) {
    VirtualPhonePro::GeoLocation location;
    location.isValid = false;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VirtualPhonePro/2.0");
    request.setTransferTimeout(timeoutMs);

    QNetworkReply* reply = nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();
        if (json["status"].toString() == "success") {
            location.ipAddress = json["query"].toString();
            location.country = json["country"].toString();
            location.countryCode = json["countryCode"].toString();
            location.region = json["regionName"].toString();
            location.city = json["city"].toString();
            location.postalCode = json["zip"].toString();
            location.timezone = json["timezone"].toString();
            location.latitude = json["lat"].toDouble();
            location.longitude = json["lon"].toDouble();
            location.isp = json["isp"].toString();
            location.org = json["org"].toString();
            location.asn = json["as"].toString();
            location.isValid = true;
            location.queriedAt = QDateTime::currentMSecsSinceEpoch();
        }
    }

    reply->deleteLater();
    return location;
}

} // anonymous namespace

namespace VirtualPhonePro {

// Static timezone mapping
const QMap<QString, QString> LocaleTimezoneManager::COUNTRY_TO_TIMEZONE = {
    {"US", "America/New_York"},
    {"GB", "Europe/London"},
    {"DE", "Europe/Berlin"},
    {"FR", "Europe/Paris"},
    {"JP", "Asia/Tokyo"},
    {"CN", "Asia/Shanghai"},
    {"IN", "Asia/Kolkata"},
    {"KR", "Asia/Seoul"},
    {"AU", "Australia/Sydney"},
    {"CA", "America/Toronto"},
    {"BR", "America/Sao_Paulo"},
    {"RU", "Europe/Moscow"},
    {"AE", "Asia/Dubai"},
    {"SG", "Asia/Singapore"},
    {"BD", "Asia/Dhaka"},
    {"PK", "Asia/Karachi"},
    {"SA", "Asia/Riyadh"},
    {"MX", "America/Mexico_City"},
    {"IT", "Europe/Rome"},
    {"ES", "Europe/Madrid"},
    {"NL", "Europe/Amsterdam"},
    {"SE", "Europe/Stockholm"},
    {"NO", "Europe/Oslo"},
    {"DK", "Europe/Copenhagen"},
    {"FI", "Europe/Helsinki"},
    {"PL", "Europe/Warsaw"},
    {"TH", "Asia/Bangkok"},
    {"VN", "Asia/Ho_Chi_Minh"},
    {"MY", "Asia/Kuala_Lumpur"},
    {"ID", "Asia/Jakarta"},
    {"PH", "Asia/Manila"},
    {"TW", "Asia/Taipei"},
    {"HK", "Asia/Hong_Kong"},
    {"NZ", "Pacific/Auckland"},
    {"ZA", "Africa/Johannesburg"},
    {"EG", "Africa/Cairo"},
    {"NG", "Africa/Lagos"},
    {"KE", "Africa/Nairobi"},
    {"AR", "America/Buenos_Aires"},
    {"CL", "America/Santiago"},
    {"CO", "America/Bogota"},
    {"PE", "America/Lima"},
};

const QMap<QString, QString> LocaleTimezoneManager::COUNTRY_TO_LOCALE = {
    {"US", "en_US"},
    {"GB", "en_GB"},
    {"DE", "de_DE"},
    {"FR", "fr_FR"},
    {"JP", "ja_JP"},
    {"CN", "zh_CN"},
    {"IN", "hi_IN"},
    {"KR", "ko_KR"},
    {"AU", "en_AU"},
    {"CA", "en_CA"},
    {"BR", "pt_BR"},
    {"RU", "ru_RU"},
    {"AE", "ar_AE"},
    {"SG", "en_SG"},
    {"BD", "bn_BD"},
    {"PK", "ur_PK"},
    {"SA", "ar_SA"},
    {"MX", "es_MX"},
    {"IT", "it_IT"},
    {"ES", "es_ES"},
    {"NL", "nl_NL"},
    {"SE", "sv_SE"},
    {"NO", "no_NO"},
    {"DK", "da_DK"},
    {"FI", "fi_FI"},
    {"PL", "pl_PL"},
    {"TH", "th_TH"},
    {"VN", "vi_VN"},
    {"MY", "ms_MY"},
    {"ID", "in_ID"},
    {"PH", "fil_PH"},
    {"TW", "zh_TW"},
    {"HK", "zh_HK"},
    {"NZ", "en_NZ"},
    {"ZA", "en_ZA"},
    {"EG", "ar_EG"},
    {"NG", "en_NG"},
    {"KE", "en_KE"},
    {"AR", "es_AR"},
    {"CL", "es_CL"},
    {"CO", "es_CO"},
    {"PE", "es_PE"},
};

LocaleTimezoneManager& LocaleTimezoneManager::instance() {
    static LocaleTimezoneManager s_instance;
    return s_instance;
}

LocaleTimezoneManager::LocaleTimezoneManager(QObject* parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

LocaleTimezoneManager::~LocaleTimezoneManager() {
    delete m_networkManager;
}

// ============================================================================
// Proxy Configuration
// ============================================================================

bool LocaleTimezoneManager::setProxy(const QString& instanceId, const ProxyInfo& proxy) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instanceStates.contains(instanceId)) {
        m_instanceStates[instanceId] = new InstanceLocaleState();
    }
    
    m_instanceStates[instanceId]->proxy = proxy;
    m_instanceStates[instanceId]->proxy.isValid = true;
    
    qDebug() << "Proxy set for" << instanceId << ":" << proxy.host << ":" << proxy.port;
    
    return true;
}

bool LocaleTimezoneManager::removeProxy(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    if (m_instanceStates.contains(instanceId)) {
        m_instanceStates[instanceId]->proxy.isValid = false;
        m_instanceStates[instanceId]->proxy.host.clear();
    }
    
    return true;
}

ProxyInfo LocaleTimezoneManager::getProxy(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    
    if (m_instanceStates.contains(instanceId)) {
        return m_instanceStates[instanceId]->proxy;
    }
    
    ProxyInfo empty;
    empty.isValid = false;
    return empty;
}

// ============================================================================
// Geolocation Sync
// ============================================================================

#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::queryGeolocation(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instanceStates.contains(instanceId)) {
        m_instanceStates[instanceId] = new InstanceLocaleState();
    }
    
    ProxyInfo proxy = m_instanceStates[instanceId]->proxy;
    
    if (!proxy.isValid || proxy.host.isEmpty()) {
        qWarning() << "No proxy configured for:" << instanceId;
        emit error(instanceId, "No proxy configured");
        return false;
    }
    
    qDebug() << "Querying geolocation through proxy tunnel:" << proxy.host;

    // Route the lookup THROUGH the proxy itself instead of sending the
    // proxy/host IP to ip-api.com directly. An empty target IP makes
    // ip-api resolve the exit IP seen at the far end of the tunnel —
    // doing it directly would resolve the wrong (host/gateway) location
    // and break country consistency.
    QNetworkProxy qProxy = (proxy.type == "socks5")
        ? QNetworkProxy(QNetworkProxy::Socks5Proxy, proxy.host, proxy.port,
                        proxy.username, proxy.password)
        : QNetworkProxy(QNetworkProxy::HttpProxy, proxy.host, proxy.port,
                        proxy.username, proxy.password);
    m_networkManager->setProxy(qProxy);

    QUrl url(GEO_API_URL);  // empty target -> ip-api resolves our exit IP
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VirtualPhonePro/2.0");
    // Qt6: QNetworkRequest has no setTimeout(); setTransferTimeout() is the
    // supported API (added in Qt 5.15 / present in all Qt6 releases).
    request.setTransferTimeout(GEO_QUERY_TIMEOUT_MS);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Store instance ID for callback
    reply->setProperty("instanceId", instanceId);
    
    // QNetworkReply::finished carries no arguments; onGeoQueryFinished needs
    // the reply, so bridge them with a lambda that captures reply.
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGeoQueryFinished(reply);
    });
    
    return true;
}
#endif // LTM_NO_CONTROLLER


#ifndef LTM_NO_CONTROLLER
GeoLocation LocaleTimezoneManager::queryGeoLocationByIP(const QString& ip) {
    // Direct (un-proxied) lookup for an explicit IP. syncFromProxy() uses
    // this only as a fallback when tunnel-based resolution fails; the
    // primary path resolves the proxy's true exit IP via an empty target.
    QNetworkAccessManager nam;
    nam.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    return executeGeoQuery(nam, QUrl(GEO_API_URL + ip), GEO_QUERY_TIMEOUT_MS);
}
#endif // LTM_NO_CONTROLLER


GeoLocation LocaleTimezoneManager::getGeoLocation(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    
    if (m_instanceStates.contains(instanceId)) {
        return m_instanceStates[instanceId]->geoLocation;
    }
    
    GeoLocation empty;
    empty.isValid = false;
    return empty;
}

#ifndef LTM_NO_CONTROLLER
void LocaleTimezoneManager::onGeoQueryFinished(QNetworkReply* reply) {
    QString instanceId = reply->property("instanceId").toString();

    // queryGeolocation() routes this request through the instance's proxy on
    // the shared m_networkManager. Reset it once the reply is in so later
    // unrelated requests do not silently inherit the tunnel.
    m_networkManager->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    QMutexLocker locker(&m_mutex);
    
    if (!m_instanceStates.contains(instanceId)) {
        reply->deleteLater();
        return;
    }
    
    InstanceLocaleState* state = m_instanceStates[instanceId];
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();
        
        if (json["status"].toString() == "success") {
            state->geoLocation.ipAddress = json["query"].toString();
            state->geoLocation.country = json["country"].toString();
            state->geoLocation.countryCode = json["countryCode"].toString();
            state->geoLocation.region = json["regionName"].toString();
            state->geoLocation.city = json["city"].toString();
            state->geoLocation.postalCode = json["zip"].toString();
            state->geoLocation.timezone = json["timezone"].toString();
            state->geoLocation.latitude = json["lat"].toDouble();
            state->geoLocation.longitude = json["lon"].toDouble();
            state->geoLocation.isp = json["isp"].toString();
            state->geoLocation.org = json["org"].toString();
            state->geoLocation.asn = json["as"].toString();
            state->geoLocation.isValid = true;
            state->geoLocation.queriedAt = QDateTime::currentMSecsSinceEpoch();
            
            qDebug() << "Geolocation updated for" << instanceId
                     << ":" << state->geoLocation.city << "," << state->geoLocation.country;
            
            emit geoLocationUpdated(instanceId, state->geoLocation);
        } else {
            qWarning() << "Geolocation query failed for" << instanceId;
            emit error(instanceId, "Geolocation query failed");
        }
    } else {
        qWarning() << "Network error for" << instanceId << ":" << reply->errorString();
        emit error(instanceId, reply->errorString());
    }
    
    reply->deleteLater();
}
#endif // LTM_NO_CONTROLLER


// ============================================================================
// Apply Settings
// ============================================================================

#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::applyLocale(const QString& instanceId, const LocaleConfig& locale) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        // Language and locale
        QString("setprop persist.sys.language %1").arg(locale.language),
        QString("setprop persist.sys.country %1").arg(locale.region),
        QString("setprop persist.sys.locale %1").arg(locale.localeString),
        
        // Legacy properties
        QString("setprop ro.product.locale %1").arg(locale.localeString),
        QString("setprop ro.product.locale.language %1").arg(locale.language),
        QString("setprop ro.product.locale.region %1").arg(locale.region),
        
        // User preferences
        "settings put secure locale_preferences_sync 1",
        
        // Input locale
        QString("settings put secure input_methods_priority_list %1").arg(
            locale.language + "-${locale.region}"),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Update carrier country code
    ctrl.executeShell(instanceId,
        QString("setprop persist.sys.country %1").arg(locale.region.toLower()));
    
    qDebug() << "Locale applied for" << instanceId << ":" << locale.localeString;
    return true;
}
#endif // LTM_NO_CONTROLLER


#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::applyTimezone(const QString& instanceId, const QString& timezone) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Calculate timezone offset
    QTimeZone tz(timezone.toUtf8());
    int offsetSeconds = tz.offsetFromUtc(QDateTime::currentDateTimeUtc());
    int hours = offsetSeconds / 3600;
    int minutes = (offsetSeconds % 3600) / 60;
    QString offset = QString("%1%2:%3")
        .arg(offsetSeconds >= 0 ? "+" : "-")
        .arg(qAbs(hours), 2, 10, QChar('0'))
        .arg(qAbs(minutes), 2, 10, QChar('0'));
    
    QStringList commands = {
        // Timezone setting
        QString("setprop persist.sys.timezone %1").arg(timezone),
        QString("setprop user.timezone %1").arg(timezone),
        
        // Timezone data version
        "setprop persist.sys.timezone.auto 0",
        QString("setprop persist.sys.timezone.data.version 2024a"),
        
        // Offset (for apps that use it)
        QString("setprop persist.sys.timezone.offset %1").arg(offset),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Also set via setprop
    ctrl.executeShell(instanceId, "toolbox setprop persist.sys.timezone " + timezone);
    
    qDebug() << "Timezone applied for" << instanceId << ":" << timezone;
    return true;
}
#endif // LTM_NO_CONTROLLER


#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::applyCarrier(const QString& instanceId, const CarrierConfig& carrier) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString numeric = carrier.mcc + carrier.mnc;
    
    QStringList commands = {
        // SIM operator
        QString("setprop gsm.sim.operator.numeric %1").arg(numeric),
        QString("setprop gsm.sim.operator.alpha %1").arg(carrier.name),
        QString("setprop gsm.sim.operator.iso-country %1").arg(carrier.countryCode),
        
        // Network operator
        QString("setprop gsm.operator.numeric %1").arg(numeric),
        QString("setprop gsm.operator.alpha %1").arg(carrier.name),
        QString("setprop gsm.operator.iso-country %1").arg(carrier.countryCode),
        QString("setprop gsm.operator.country %1").arg(carrier.countryCode),
        
        // Network type
        QString("setprop gsm.network.type %1").arg(carrier.networkType),
        
        // Country
        QString("setprop persist.sys.country %1").arg(carrier.countryCode.toLower()),
        QString("setprop ro.product.locale.region %1").arg(carrier.countryCode),
        
        // Mobile country code
        QString("setprop persist.radio.country %1").arg(carrier.countryCode),
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Carrier applied for" << instanceId << ":" << carrier.name 
             << "(" << carrier.mcc << carrier.mnc << ")";
    return true;
}
#endif // LTM_NO_CONTROLLER


// ============================================================================
// Auto Sync
// ============================================================================

#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::syncFromProxy(const QString& instanceId) {
    qDebug() << "Starting auto-sync from proxy for:" << instanceId;

    ProxyInfo proxy = getProxy(instanceId);
    if (!proxy.isValid || proxy.host.isEmpty()) {
        qWarning() << "[AutoSync] No proxy configured for" << instanceId
                   << "— cannot sync";
        emit error(instanceId, "No proxy configured");
        return false;
    }

    // PRIMARY PATH — tunnel the lookup through the configured proxy so
    // ip-api.com sees the *exit IP*. Querying proxy.host directly from the
    // host would resolve the wrong (gateway/host) location.
    QNetworkProxy qProxy = (proxy.type == "socks5")
        ? QNetworkProxy(QNetworkProxy::Socks5Proxy, proxy.host, proxy.port,
                        proxy.username, proxy.password)
        : QNetworkProxy(QNetworkProxy::HttpProxy, proxy.host, proxy.port,
                        proxy.username, proxy.password);

    GeoLocation geo;
    {
        QNetworkAccessManager nam;
        nam.setProxy(qProxy);
        QUrl url{QString(GEO_API_URL)};  // empty target -> exit IP
        geo = executeGeoQuery(nam, url, GEO_QUERY_TIMEOUT_MS);
    }

    if (geo.isValid) {
        qDebug() << "[AutoSync] Geolocation resolved via proxy tunnel:"
                 << geo.ipAddress << "→" << geo.country;
    } else {
        // FALLBACK — tunnel failed; fall back to a direct geo lookup of the
        // gateway IP instead of failing silently. Loud log either way.
        qWarning() << "[AutoSync] Proxy-tunnel geolocation failed for" << instanceId
                   << "— falling back to gateway IP lookup";
        QNetworkAccessManager nam;
        nam.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        QUrl url{QString(GEO_API_URL) + proxy.host};
        geo = executeGeoQuery(nam, url, GEO_QUERY_TIMEOUT_MS);
        if (!geo.isValid) {
            qWarning() << "[AutoSync] Geolocation resolution failed for" << instanceId
                       << "— instance keeps default locale/timezone/carrier";
            emit error(instanceId, "Could not resolve geolocation via proxy");
            return false;
        }
        qWarning() << "[AutoSync] Using gateway-IP geolocation fallback:"
                   << geo.ipAddress << "→" << geo.country << "(accuracy degraded)";
    }

    // Generate locale + carrier from the resolved country
    LocaleConfig locale = getLocaleForCountry(geo.countryCode);
    locale.region = geo.countryCode;
    locale.localeString = locale.language + "_" + geo.countryCode;

    CarrierConfig carrier = getCarrierForLocation(geo.countryCode, geo.region, instanceId);
    carrier.country = geo.country;
    carrier.countryCode = geo.countryCode;

    applyLocale(instanceId, locale);
    applyTimezone(instanceId, geo.timezone);
    applyCarrier(instanceId, carrier);

    // Update state
    {
        QMutexLocker locker(&m_mutex);
        if (m_instanceStates.contains(instanceId)) {
            m_instanceStates[instanceId]->geoLocation = geo;
            m_instanceStates[instanceId]->locale = locale;
            m_instanceStates[instanceId]->carrier = carrier;
            m_instanceStates[instanceId]->isSynced = true;
            m_instanceStates[instanceId]->lastSyncTime = QDateTime::currentMSecsSinceEpoch();
        }
    }

    qDebug() << "Auto-sync completed for" << instanceId
             << "- Timezone:" << geo.timezone
             << "- Locale:" << locale.localeString
             << "- Carrier:" << carrier.name
             << "- MCC/MNC:" << carrier.mcc << "/" << carrier.mnc;

    emit geoLocationUpdated(instanceId, geo);
    emit syncCompleted(instanceId, true);
    return true;
}
#endif // LTM_NO_CONTROLLER




#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::resyncFromProxy(const QString& instanceId) {
    qDebug() << "[ReSync] Checking proxy exit IP for rotation:" << instanceId;

    ProxyInfo proxy = getProxy(instanceId);
    if (!proxy.isValid || proxy.host.isEmpty()) {
        qWarning() << "[ReSync] No proxy configured for" << instanceId
                   << "— nothing to re-sync";
        return false;
    }

    // PRIMARY tunnel path — same as syncFromProxy(): query ip-api.com THROUGH
    // the proxy with an empty target so the true exit IP is resolved.
    QNetworkProxy qProxy = (proxy.type == "socks5")
        ? QNetworkProxy(QNetworkProxy::Socks5Proxy, proxy.host, proxy.port,
                        proxy.username, proxy.password)
        : QNetworkProxy(QNetworkProxy::HttpProxy, proxy.host, proxy.port,
                        proxy.username, proxy.password);

    GeoLocation current;
    {
        QNetworkAccessManager nam;
        nam.setProxy(qProxy);
        QUrl url{QString(GEO_API_URL)};  // empty target -> exit IP
        current = executeGeoQuery(nam, url, GEO_QUERY_TIMEOUT_MS);
    }

    if (!current.isValid) {
        qWarning() << "[ReSync] Could not resolve current exit IP through proxy"
                   << "tunnel for" << instanceId << "— keeping existing identity";
        return false;
    }

    const GeoLocation stored = getGeoLocation(instanceId);
    const bool neverSynced = !stored.isValid;
    const bool rotated = stored.isValid
        && !current.ipAddress.isEmpty()
        && current.ipAddress != stored.ipAddress;

    if (!neverSynced && !rotated) {
        qDebug() << "[ReSync] Exit IP unchanged (" << current.ipAddress
                 << ") — identity stays as-is";
        return false;  // no-op, nothing to re-apply
    }

    qDebug() << "[ReSync]" << (neverSynced ? "Never synced" : "Exit IP rotated")
             << "for" << instanceId << "— re-running full sync"
             << "(" << stored.ipAddress << "->" << current.ipAddress << ")";

    return syncFromProxy(instanceId);
}
#endif // LTM_NO_CONTROLLER


#ifndef LTM_NO_CONTROLLER
bool LocaleTimezoneManager::syncFromCoordinates(const QString& instanceId, double lat, double lon) {
    // For direct coordinate sync (when proxy is not available)
    
    QString countryCode = getCountryFromCoordinates(lat, lon);
    QString timezone = getTimezoneForCoordinates(lat, lon);
    
    LocaleConfig locale = getLocaleForCountry(countryCode);
    locale.region = countryCode;
    locale.localeString = locale.language + "_" + countryCode;
    
    CarrierConfig carrier = getCarrierForLocation("", "", instanceId);
    carrier.countryCode = countryCode;
    
    applyLocale(instanceId, locale);
    applyTimezone(instanceId, timezone);
    applyCarrier(instanceId, carrier);
    
    // Update state
    {
        QMutexLocker locker(&m_mutex);
        if (m_instanceStates.contains(instanceId)) {
            m_instanceStates[instanceId]->locale = locale;
            m_instanceStates[instanceId]->carrier = carrier;
            m_instanceStates[instanceId]->geoLocation.latitude = lat;
            m_instanceStates[instanceId]->geoLocation.longitude = lon;
            m_instanceStates[instanceId]->geoLocation.timezone = timezone;
            m_instanceStates[instanceId]->isSynced = true;
        }
    }
    
    return true;
}
#endif // LTM_NO_CONTROLLER


// ============================================================================
// Utility Methods
// ============================================================================

QString LocaleTimezoneManager::getTimezoneForCoordinates(double lat, double lon) const {
    // Simplified timezone lookup by country
    // In production, use a proper timezone database
    
    // Default timezone based on rough longitude
    int timezoneIndex = static_cast<int>((lon + 180) / 15);
    int offsetHours = timezoneIndex - 12;  // -12 to +12
    
    // Common timezones
    static QMap<int, QString> offsetTimezones = {
        {-12, "Pacific/Baker_Island"},
        {-11, "Pacific/Samoa"},
        {-10, "Pacific/Honolulu"},
        {-9, "America/Anchorage"},
        {-8, "America/Los_Angeles"},
        {-7, "America/Denver"},
        {-6, "America/Chicago"},
        {-5, "America/New_York"},
        {-4, "America/Halifax"},
        {-3, "America/Sao_Paulo"},
        {-2, "Atlantic/South_Georgia"},
        {-1, "Atlantic/Azores"},
        {0, "Europe/London"},
        {1, "Europe/Paris"},
        {2, "Europe/Helsinki"},
        {3, "Europe/Moscow"},
        {4, "Asia/Dubai"},
        {5, "Asia/Kolkata"},
        {6, "Asia/Dhaka"},
        {7, "Asia/Bangkok"},
        {8, "Asia/Shanghai"},
        {9, "Asia/Tokyo"},
        {10, "Australia/Sydney"},
        {11, "Pacific/Noumea"},
        {12, "Pacific/Auckland"},
    };
    
    return offsetTimezones.value(offsetHours, "UTC");
}

QString LocaleTimezoneManager::getCountryFromCoordinates(double lat, double lon) const {
    // Simplified country detection by coordinates
    // In production, use reverse geocoding
    
    // US
    if (lat >= 24 && lat <= 50 && lon >= -125 && lon <= -66) return "US";
    // UK
    if (lat >= 49 && lat <= 61 && lon >= -8 && lon <= 2) return "GB";
    // Germany
    if (lat >= 47 && lat <= 55 && lon >= 5 && lon <= 15) return "DE";
    // France
    if (lat >= 41 && lat <= 51 && lon >= -5 && lon <= 9) return "FR";
    // Japan
    if (lat >= 24 && lat <= 46 && lon >= 123 && lon <= 146) return "JP";
    // China
    if (lat >= 18 && lat <= 54 && lon >= 73 && lon <= 135) return "CN";
    // India
    if (lat >= 6 && lat <= 36 && lon >= 68 && lon <= 97) return "IN";
    // South Korea
    if (lat >= 33 && lat <= 39 && lon >= 124 && lon <= 132) return "KR";
    // Australia
    if (lat >= -44 && lat <= -10 && lon >= 112 && lon <= 155) return "AU";
    // Brazil
    if (lat >= -34 && lat <= 5 && lon >= -74 && lon <= -34) return "BR";
    // Russia
    if (lat >= 41 && lat <= 82 && lon >= 19 && lon <= 180) return "RU";
    // UAE
    if (lat >= 22 && lat <= 27 && lon >= 51 && lon <= 57) return "AE";
    // Singapore
    if (lat >= 1 && lat <= 2 && lon >= 103 && lon <= 105) return "SG";
    // Bangladesh
    if (lat >= 20 && lat <= 27 && lon >= 88 && lon <= 93) return "BD";
    
    return "US";  // Default
}

LocaleConfig LocaleTimezoneManager::getLocaleForCountry(const QString& countryCode) const {
    LocaleConfig locale;
    // LocaleConfig has no countryCode field; 'region' holds the ISO 3166-1
    // alpha-2 country code (set on the next line).
    locale.language = "en";
    locale.region = countryCode;
    
    if (COUNTRY_TO_LOCALE.contains(countryCode)) {
        QString loc = COUNTRY_TO_LOCALE[countryCode];
        QStringList parts = loc.split("_");
        if (parts.size() >= 2) {
            locale.language = parts[0];
            locale.region = parts[1];
        }
    } else {
        locale.language = "en";
        locale.region = countryCode;
    }
    
    locale.localeString = locale.language + "_" + locale.region;
    
    return locale;
}

CarrierConfig LocaleTimezoneManager::getCarrierForLocation(const QString& country, const QString& region, const QString& seed) const {
    (void)region; // region intentionally unused — lookup is by country code

    // Multi-carrier table: several realistic carriers per country, each with a
    // documented MCC/MNC pair (wrong combinations raise detection risk). The
    // caller passes a profile-anchored seed (instanceId) so the SAME profile
    // always gets the SAME carrier while different profiles in the same
    // country see different operators — never process-random.
    struct CarrierEntry {
        const char* name;
        const char* shortName;
        const char* mcc;
        const char* mnc;
        const char* networkType;
    };

    static const QMap<QString, QVector<CarrierEntry>> COUNTRY_CARRIERS = {
        {"US", {{"T-Mobile", "TMO", "310", "260", "LTE"},
                {"AT&T", "ATT", "310", "410", "LTE"},
                {"Verizon", "VZ", "311", "480", "5G"},
                {"US Cellular", "USC", "311", "580", "LTE"}}},
        {"GB", {{"EE", "EE", "234", "33", "5G"},
                {"O2", "O2", "234", "10", "LTE"},
                {"Vodafone UK", "VOD", "234", "15", "LTE"},
                {"Three UK", "THREE", "234", "20", "LTE"}}},
        {"DE", {{"Deutsche Telekom", "DT", "262", "01", "5G"},
                {"Vodafone DE", "VFD", "262", "02", "LTE"},
                {"O2 DE", "O2D", "262", "07", "LTE"}}},
        {"FR", {{"Orange", "ORA", "208", "01", "LTE"},
                {"Bouygues", "BYG", "208", "20", "LTE"},
                {"SFR", "SFR", "208", "10", "LTE"},
                {"Free Mobile", "FREE", "208", "15", "LTE"}}},
        {"JP", {{"SoftBank", "SBT", "440", "20", "5G"},
                {"NTT DOCOMO", "DCM", "440", "10", "5G"},
                {"au KDDI", "AU", "440", "50", "5G"},
                {"Rakuten Mobile", "RAK", "440", "11", "5G"}}},
        {"KR", {{"SK Telecom", "SKT", "450", "05", "5G"},
                {"KT", "KT", "450", "08", "5G"},
                {"LG U+", "LGU", "450", "06", "5G"}}},
        {"IN", {{"Jio", "JIO", "405", "874", "4G"},
                {"Airtel", "AIR", "404", "10", "4G"},
                {"Vi", "VI", "405", "66", "4G"}}},
        {"BD", {{"Grameenphone", "GP", "470", "01", "4G"},
                {"Robi", "ROBI", "470", "02", "4G"},
                {"Banglalink", "BL", "470", "03", "4G"},
                {"Teletalk", "TT", "470", "04", "4G"}}},
        {"CN", {{"China Mobile", "CMCC", "460", "00", "5G"},
                {"China Unicom", "CUC", "460", "01", "LTE"},
                {"China Telecom", "CT", "460", "03", "LTE"}}},
        {"AU", {{"Telstra", "Telstra", "505", "01", "5G"},
                {"Optus", "Optus", "505", "02", "LTE"},
                {"Vodafone AU", "VFA", "505", "03", "LTE"}}},
        {"CA", {{"Rogers", "Rogers", "302", "720", "5G"},
                {"Bell", "Bell", "302", "610", "LTE"},
                {"Telus", "Telus", "302", "220", "LTE"}}},
        {"BR", {{"Vivo", "Vivo", "724", "06", "4G"},
                {"Claro BR", "Claro", "724", "05", "4G"},
                {"TIM BR", "TIM", "724", "02", "4G"}}},
        {"RU", {{"MTS", "MTS", "250", "01", "LTE"},
                {"MegaFon", "MGF", "250", "02", "LTE"},
                {"Beeline", "BEE", "250", "99", "LTE"},
                {"Tele2 RU", "T2", "250", "20", "LTE"}}},
        {"AE", {{"Etisalat", "etisalat", "424", "02", "5G"},
                {"du", "du", "424", "03", "LTE"}}},
        {"SG", {{"Singtel", "Singtel", "525", "01", "5G"},
                {"StarHub", "StarHub", "525", "05", "LTE"},
                {"M1", "M1", "525", "03", "LTE"}}},
        {"PK", {{"Jazz", "Jazz", "410", "01", "4G"},
                {"Telenor PK", "Telenor", "410", "06", "4G"},
                {"Zong", "Zong", "410", "04", "4G"},
                {"Ufone", "Ufone", "410", "03", "4G"}}},
        {"SA", {{"STC", "STC", "420", "01", "5G"},
                {"Mobily", "Mobily", "420", "03", "LTE"},
                {"Zain SA", "Zain", "420", "04", "LTE"}}},
        {"MX", {{"Telcel", "Telcel", "334", "20", "4G"},
                {"Movistar MX", "Movistar", "334", "03", "4G"},
                {"AT&T MX", "ATT", "334", "50", "4G"}}},
        {"IT", {{"TIM", "TIM", "222", "01", "LTE"},
                {"Vodafone IT", "VOD", "222", "10", "LTE"},
                {"Wind Tre", "WIND", "222", "88", "LTE"},
                {"Iliad IT", "ILIAD", "222", "50", "LTE"}}},
        {"ES", {{"Movistar", "Movistar", "214", "07", "LTE"},
                {"Vodafone ES", "VFE", "214", "01", "LTE"},
                {"Orange ES", "ORA", "214", "03", "LTE"}}},
        {"NL", {{"KPN", "KPN", "204", "08", "LTE"},
                {"Vodafone NL", "VOD", "204", "04", "LTE"},
                {"T-Mobile NL", "TMO", "204", "16", "LTE"}}},
        {"SE", {{"Telia", "Telia", "240", "01", "LTE"},
                {"Tele2 SE", "Tele2", "240", "07", "LTE"},
                {"Telenor SE", "Telenor", "240", "08", "LTE"},
                {"Tre SE", "3", "240", "02", "LTE"}}},
        {"NO", {{"Telenor", "Telenor", "242", "01", "LTE"},
                {"Telia NO", "Telia", "242", "02", "LTE"},
                {"Ice", "Ice", "242", "14", "LTE"}}},
        {"DK", {{"TDC", "TDC", "238", "01", "LTE"},
                {"Telenor DK", "Telenor", "238", "02", "LTE"},
                {"3 DK", "3", "238", "06", "LTE"}}},
        {"FI", {{"Elisa", "Elisa", "244", "05", "LTE"},
                {"DNA", "DNA", "244", "03", "LTE"},
                {"Telia FI", "Telia", "244", "91", "LTE"}}},
        {"PL", {{"Play", "Play", "260", "06", "4G"},
                {"Orange PL", "ORA", "260", "03", "LTE"},
                {"Plus", "Plus", "260", "01", "LTE"},
                {"T-Mobile PL", "TMO", "260", "02", "LTE"}}},
        {"TH", {{"AIS", "AIS", "520", "01", "4G"},
                {"dtac", "dtac", "520", "05", "4G"},
                {"TrueMove H", "TRUE", "520", "04", "4G"}}},
        {"VN", {{"Viettel", "Viettel", "452", "04", "4G"},
                {"Vinaphone", "VNA", "452", "02", "4G"},
                {"Mobifone", "MBF", "452", "01", "4G"}}},
        {"MY", {{"Maxis", "Maxis", "502", "12", "4G"},
                {"Digi", "Digi", "502", "16", "4G"},
                {"Celcom", "Celcom", "502", "19", "4G"}}},
        {"ID", {{"Telkomsel", "Telkomsel", "510", "10", "4G"},
                {"Indosat", "ISAT", "510", "01", "4G"},
                {"XL Axiata", "XL", "510", "11", "4G"}}},
        {"PH", {{"Globe", "Globe", "515", "02", "4G"},
                {"Smart", "Smart", "515", "03", "4G"},
                {"DITO", "DITO", "515", "66", "4G"}}},
        {"TW", {{"Chunghwa", "CHT", "466", "92", "4G"},
                {"Taiwan Mobile", "TWM", "466", "97", "4G"},
                {"FarEasTone", "FET", "466", "01", "4G"}}},
        {"HK", {{"SmarTone", "SmarTone", "454", "06", "4G"},
                {"CSL", "CSL", "454", "00", "4G"},
                {"3 HK", "3", "454", "03", "4G"}}},
        {"NZ", {{"Vodafone NZ", "VF", "530", "01", "4G"},
                {"Spark", "Spark", "530", "05", "4G"},
                {"2degrees", "2DEG", "530", "24", "4G"}}},
        {"ZA", {{"Vodacom", "Vodacom", "655", "01", "4G"},
                {"MTN ZA", "MTN", "655", "10", "4G"},
                {"Cell C", "CellC", "655", "07", "4G"},
                {"Telkom ZA", "Telkom", "655", "02", "4G"}}},
        {"EG", {{"Orange Egypt", "Orange", "602", "01", "4G"},
                {"Vodafone EG", "VF", "602", "02", "4G"},
                {"Etisalat EG", "ETI", "602", "03", "4G"}}},
        {"NG", {{"MTN Nigeria", "MTN", "621", "30", "4G"},
                {"Airtel NG", "Airtel", "621", "20", "4G"},
                {"Glo", "Glo", "621", "50", "4G"}}},
        {"KE", {{"Safaricom", "Safaricom", "639", "02", "4G"},
                {"Airtel KE", "Airtel", "639", "03", "4G"},
                {"Telkom KE", "Telkom", "639", "07", "4G"}}},
        {"AR", {{"Movistar Argentina", "Movistar", "722", "070", "4G"},
                {"Claro AR", "Claro", "722", "310", "4G"},
                {"Personal", "Personal", "722", "341", "4G"}}},
        {"CL", {{"Movistar Chile", "Movistar", "730", "02", "4G"},
                {"Entel", "Entel", "730", "01", "4G"},
                {"Claro CL", "Claro", "730", "03", "4G"}}},
        {"CO", {{"Claro Colombia", "Claro", "732", "101", "4G"},
                {"Movistar CO", "Movistar", "732", "123", "4G"},
                {"Tigo", "Tigo", "732", "111", "4G"}}},
        {"PE", {{"Movistar Peru", "Movistar", "716", "06", "4G"},
                {"Claro PE", "Claro", "716", "10", "4G"},
                {"Entel PE", "Entel", "716", "17", "4G"}}},
    };

    // Full country names (as returned by ip-api) -> ISO code, so legacy
    // name-based callers still resolve correctly.
    static const QMap<QString, QString> NAME_TO_CODE = {
        {"United States", "US"},      {"United Kingdom", "GB"},
        {"Germany", "DE"},            {"France", "FR"},
        {"Japan", "JP"},              {"South Korea", "KR"},
        {"India", "IN"},              {"Bangladesh", "BD"},
        {"China", "CN"},              {"Australia", "AU"},
        {"Canada", "CA"},             {"Brazil", "BR"},
        {"Russia", "RU"},             {"United Arab Emirates", "AE"},
        {"Singapore", "SG"},          {"Pakistan", "PK"},
        {"Saudi Arabia", "SA"},       {"Mexico", "MX"},
        {"Italy", "IT"},              {"Spain", "ES"},
        {"Netherlands", "NL"},        {"Sweden", "SE"},
        {"Norway", "NO"},             {"Denmark", "DK"},
        {"Finland", "FI"},            {"Poland", "PL"},
        {"Thailand", "TH"},           {"Vietnam", "VN"},
        {"Malaysia", "MY"},           {"Indonesia", "ID"},
        {"Philippines", "PH"},        {"Taiwan", "TW"},
        {"Hong Kong", "HK"},          {"New Zealand", "NZ"},
        {"South Africa", "ZA"},       {"Egypt", "EG"},
        {"Nigeria", "NG"},            {"Kenya", "KE"},
        {"Argentina", "AR"},          {"Chile", "CL"},
        {"Colombia", "CO"},           {"Peru", "PE"},
    };

    QString code = country.trimmed();
    if (code.length() != 2) {
        auto it = NAME_TO_CODE.find(code);
        if (it != NAME_TO_CODE.end()) code = it.value();
    }
    code = code.toUpper();

    auto it = COUNTRY_CARRIERS.find(code);
    if (it == COUNTRY_CARRIERS.end()) {
        qWarning() << "[AutoSync] No carrier mapping for country" << country
                   << "— using generic placeholder carrier (310/260)";
        CarrierConfig carrier;
        carrier.name = "Carrier";
        carrier.shortName = "CAR";
        carrier.mcc = "310";
        carrier.mnc = "260";
        carrier.networkType = "LTE";
        return carrier;
    }

    const QVector<CarrierEntry>& carriers = it.value();

    // Profile-anchored deterministic selection: FNV-1a of the seed (instanceId)
    // picks the carrier index. Same profile -> same carrier every time;
    // different profiles in the same country spread across operators.
    const QString selSeed = !seed.isEmpty() ? seed : code;
    quint32 h = 2166136261u;  // FNV-1a offset basis
    for (const QChar c : selSeed) {
        h ^= c.unicode();
        h *= 16777619u;       // FNV prime
    }
    const CarrierEntry& e = carriers.at(static_cast<int>(h % carriers.size()));

    CarrierConfig carrier;
    carrier.name = QString::fromLatin1(e.name);
    carrier.shortName = QString::fromLatin1(e.shortName);
    carrier.mcc = QString::fromLatin1(e.mcc);
    carrier.mnc = QString::fromLatin1(e.mnc);
    carrier.networkType = QString::fromLatin1(e.networkType);
    return carrier;
}





QJsonObject LocaleTimezoneManager::getStateAsJson(const QString& instanceId) const {
    QJsonObject state;
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_instanceStates.contains(instanceId)) {
        return state;
    }
    
    InstanceLocaleState* s = m_instanceStates[instanceId];
    
    state["synced"] = s->isSynced;
    state["lastSyncTime"] = s->lastSyncTime;
    
    QJsonObject proxy;
    proxy["host"] = s->proxy.host;
    proxy["port"] = s->proxy.port;
    proxy["type"] = s->proxy.type;
    proxy["valid"] = s->proxy.isValid;
    state["proxy"] = proxy;
    
    QJsonObject geo;
    geo["country"] = s->geoLocation.country;
    geo["countryCode"] = s->geoLocation.countryCode;
    geo["city"] = s->geoLocation.city;
    geo["timezone"] = s->geoLocation.timezone;
    geo["latitude"] = s->geoLocation.latitude;
    geo["longitude"] = s->geoLocation.longitude;
    geo["valid"] = s->geoLocation.isValid;
    state["geolocation"] = geo;
    
    QJsonObject locale;
    locale["language"] = s->locale.language;
    locale["region"] = s->locale.region;
    locale["localeString"] = s->locale.localeString;
    state["locale"] = locale;
    
    QJsonObject carrier;
    carrier["name"] = s->carrier.name;
    carrier["mcc"] = s->carrier.mcc;
    carrier["mnc"] = s->carrier.mnc;
    state["carrier"] = carrier;
    
    return state;
}

} // namespace VirtualPhonePro
