/**
 * @file LocaleTimezoneManager.cpp
 * @brief Automatic Locale & Timezone Synchronization Implementation
 */

#include "Android/LocaleTimezoneManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QDateTime>
#include <QMutexLocker>

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

LocaleTimezoneManager* LocaleTimezoneManager::s_instance = nullptr;

LocaleTimezoneManager& LocaleTimezoneManager::instance() {
    if (!s_instance) {
        s_instance = new LocaleTimezoneManager();
    }
    return *s_instance;
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
    
    qDebug() << "Querying geolocation for proxy:" << proxy.host;
    
    // For SOCKS5/HTTP proxy, we need to use a service that detects the exit IP
    // Since we're using a proxy, the IP we'll query should be the proxy's exit IP
    
    QUrl url(GEO_API_URL + proxy.host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VirtualPhonePro/2.0");
    request.setTimeout(GEO_QUERY_TIMEOUT_MS);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Store instance ID for callback
    reply->setProperty("instanceId", instanceId);
    
    connect(reply, &QNetworkReply::finished, this, &LocaleTimezoneManager::onGeoQueryFinished);
    
    return true;
}

GeoLocation LocaleTimezoneManager::queryGeoLocationByIP(const QString& ip) {
    GeoLocation location;
    location.isValid = false;
    
    QUrl url(GEO_API_URL + ip);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VirtualPhonePro/2.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Synchronous wait
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
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

GeoLocation LocaleTimezoneManager::getGeoLocation(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    
    if (m_instanceStates.contains(instanceId)) {
        return m_instanceStates[instanceId]->geoLocation;
    }
    
    GeoLocation empty;
    empty.isValid = false;
    return empty;
}

void LocaleTimezoneManager::onGeoQueryFinished(QNetworkReply* reply) {
    QString instanceId = reply->property("instanceId").toString();
    
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

// ============================================================================
// Apply Settings
// ============================================================================

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

// ============================================================================
// Auto Sync
// ============================================================================

bool LocaleTimezoneManager::syncFromProxy(const QString& instanceId) {
    qDebug() << "Starting auto-sync from proxy for:" << instanceId;
    
    // Step 1: Query geolocation
    if (!queryGeolocation(instanceId)) {
        return false;
    }
    
    // Wait for geolocation query (synchronous for simplicity)
    QEventLoop loop;
    connect(this, &LocaleTimezoneManager::geoLocationUpdated, &loop, &QEventLoop::quit);
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);  // Timeout
    loop.exec();
    
    // Step 2: Get geolocation
    GeoLocation geo = getGeoLocation(instanceId);
    if (!geo.isValid) {
        qWarning() << "Invalid geolocation for" << instanceId;
        emit error(instanceId, "Could not determine geolocation");
        return false;
    }
    
    // Step 3: Generate locale
    LocaleConfig locale = getLocaleForCountry(geo.countryCode);
    locale.region = geo.countryCode;
    locale.localeString = locale.language + "_" + geo.countryCode;
    
    // Step 4: Get carrier
    CarrierConfig carrier = getCarrierForLocation(geo.country, geo.region);
    carrier.country = geo.country;
    carrier.countryCode = geo.countryCode;
    
    // Step 5: Apply all settings
    applyLocale(instanceId, locale);
    applyTimezone(instanceId, geo.timezone);
    applyCarrier(instanceId, carrier);
    
    // Update state
    {
        QMutexLocker locker(&m_mutex);
        if (m_instanceStates.contains(instanceId)) {
            m_instanceStates[instanceId]->locale = locale;
            m_instanceStates[instanceId]->carrier = carrier;
            m_instanceStates[instanceId]->isSynced = true;
            m_instanceStates[instanceId]->lastSyncTime = QDateTime::currentMSecsSinceEpoch();
        }
    }
    
    qDebug() << "Auto-sync completed for" << instanceId
             << "- Timezone:" << geo.timezone
             << "- Locale:" << locale.localeString
             << "- Carrier:" << carrier.name;
    
    emit syncCompleted(instanceId, true);
    return true;
}

bool LocaleTimezoneManager::syncFromCoordinates(const QString& instanceId, double lat, double lon) {
    // For direct coordinate sync (when proxy is not available)
    
    QString countryCode = getCountryFromCoordinates(lat, lon);
    QString timezone = getTimezoneForCoordinates(lat, lon);
    
    LocaleConfig locale = getLocaleForCountry(countryCode);
    locale.region = countryCode;
    locale.localeString = locale.language + "_" + countryCode;
    
    CarrierConfig carrier = getCarrierForLocation("", "");
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
    locale.countryCode = countryCode;
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

CarrierConfig LocaleTimezoneManager::getCarrierForLocation(const QString& country, const QString& region) const {
    CarrierConfig carrier;
    
    // Default carriers by country
    if (country == "United States" || country == "US") {
        carrier.name = "T-Mobile";
        carrier.shortName = "TMO";
        carrier.mcc = "310";
        carrier.mnc = "260";
        carrier.networkType = "LTE";
    } else if (country == "United Kingdom" || country == "GB") {
        carrier.name = "EE";
        carrier.shortName = "EE";
        carrier.mcc = "234";
        carrier.mnc = "33";
        carrier.networkType = "LTE";
    } else if (country == "Germany" || country == "DE") {
        carrier.name = "Deutsche Telekom";
        carrier.shortName = "DT";
        carrier.mcc = "262";
        carrier.mnc = "01";
        carrier.networkType = "LTE";
    } else if (country == "Japan" || country == "JP") {
        carrier.name = "SoftBank";
        carrier.shortName = "SBT";
        carrier.mcc = "440";
        carrier.mnc = "20";
        carrier.networkType = "5G";
    } else if (country == "India" || country == "IN") {
        carrier.name = "Jio";
        carrier.shortName = "JIO";
        carrier.mcc = "405";
        carrier.mnc = "800";
        carrier.networkType = "4G";
    } else if (country == "Bangladesh" || country == "BD") {
        carrier.name = "Banglalink";
        carrier.shortName = "BL";
        carrier.mcc = "470";
        carrier.mnc = "02";
        carrier.networkType = "4G";
    } else if (country == "China" || country == "CN") {
        carrier.name = "China Mobile";
        carrier.shortName = "CMCC";
        carrier.mcc = "460";
        carrier.mnc = "00";
        carrier.networkType = "5G";
    } else {
        // Default
        carrier.name = "Carrier";
        carrier.shortName = "CAR";
        carrier.mcc = "310";
        carrier.mnc = "260";
        carrier.networkType = "LTE";
    }
    
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
