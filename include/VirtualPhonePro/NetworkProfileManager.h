/**
 * @file NetworkProfileManager.h
 * @brief Network Profile Manager with Cellular Network Spoofing
 * @version 3.0.0
 * 
 * This module handles two network configuration modes:
 * 1. WITH PROXY (Auto-Sync Mode): Proxy-based traffic routing with
 *    automatic geolocation from API
 * 2. WITHOUT PROXY (Manual Country Mode): Virtual cellular IP with
 *    local dataset-based country selection
 * 
 * Global Feature: Cellular Network Spoofing - Makes Android's network
 * appear as TRANSPORT_CELLULAR instead of TRANSPORT_ETHERNET
 */

#pragma once

#ifndef VIRTUALPHONEPRO_NETWORK_PROFILE_MANAGER_H
#define VIRTUALPHONEPRO_NETWORK_PROFILE_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QUrl>
#include <QHostAddress>
#include <QtConcurrent>

namespace VirtualPhonePro {

// ========================================================================
// ENUMS & CONSTANTS
// ========================================================================

/**
 * @brief Network configuration mode
 */
enum class NetworkConfigMode {
    None = 0,
    WithProxy_AutoSync,    // Mode 1: Proxy with automatic geolocation sync
    WithoutProxy_Manual    // Mode 2: Manual country selection, virtual cellular IP
};

/**
 * @brief Cellular network transport type for spoofing
 */
enum class CellularTransportType {
    LTE,
    NR_NSA,
    NR_SA,
    WCDMA,
    GSM
};

/**
 * @brief Network profile status
 */
enum class NetworkProfileStatus {
    Idle,
    FetchingGeolocation,
    Configuring,
    Applied,
    Error
};

// ========================================================================
// DATA STRUCTURES
// ========================================================================

/**
 * @brief Geolocation data from API response
 */
struct GeolocationData {
    QString status;            // "success" or "fail"
    QString country;           // Country name
    QString countryCode;       // ISO 3166-1 alpha-2 (e.g., "US", "BD")
    QString region;            // State/Region
    QString regionName;        // Full region name
    QString city;              // City name
    QString zip;               // ZIP/Postal code
    double latitude;           // Latitude
    double longitude;          // Longitude
    QString timezone;          // Timezone string (e.g., "America/New_York")
    QString isp;               // ISP name
    QString org;               // Organization
    QString as;                // AS number
    QString query;             // IP address used for query
    
    // Parsed carrier information
    QString carrierName;       // Extracted carrier (e.g., "Grameenphone")
    QString carrierMCC;        // Mobile Country Code (3 digits)
    QString carrierMNC;        // Mobile Network Code (2-3 digits)
    
    bool isValid() const {
        return status == "success" && !country.isEmpty();
    }
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

/**
 * @brief Country data from local dataset
 */
struct CountryData {
    QString name;                  // Full country name
    QString code;                  // ISO 3166-1 alpha-2
    QString region;                // Continent/Region
    QString mcc;                   // Default MCC
    QStringList carriers;          // Available carriers
    
    // Geographic data
    double minLatitude;
    double maxLatitude;
    double minLongitude;
    double maxLongitude;
    QString timezone;               // Primary timezone
    QStringList additionalTimezones;
    
    // Network data
    QStringList dnsServers;         // Country-specific DNS
    QStringList mobileCarriers;    // Major mobile carriers
    
    // Cellular network type weights
    QMap<CellularTransportType, int> networkTypeWeights;
    
    static QList<CountryData> getAllCountries();
    static CountryData getCountry(const QString& code);
};

/**
 * @brief Cellular network configuration
 */
struct CellularNetworkConfig {
    // Transport type
    CellularTransportType transportType;
    QString transportTypeStr;      // "LTE", "5G", etc.
    
    // Operator information
    QString operatorName;          // "AT&T", "Grameenphone"
    QString operatorNumeric;       // MCC + MNC (e.g., "310410")
    int simState;                  // 5 = SIM_STATE_READY
    
    // Network properties
    QString mobileDataEnabled;
    QString dataRoamingEnabled;
    QString networkMode;           // "LTE", "5G", "WCDMA"
    QString preferredNetworkType;  // "9" for LTE (LTE/UMTS/GSM)
    
    // IP configuration
    QString ipAddress;
    QString subnetMask;
    QString gateway;
    QStringList dnsServers;
    
    // Connection state
    QString connectionState;        // "connected", "disconnected"
    int signalStrength;           // -1 to -120 dBm
    int cellId;                    // Cell tower ID
    int lac;                       // Location Area Code
    
    QJsonObject toJson() const;
};

/**
 * @brief TCP/IP fingerprint for network masking
 */
struct TCPFingerprint {
    quint16 windowSize;
    quint16 mss;
    bool windowScaling;
    quint8 sackOk;
    quint8 timestamp;
    quint8 selectiveAck;
    QString mode;              // "normal", "aggressive", "conservative"
    
    static TCPFingerprint getForCountry(const QString& countryCode);
    static TCPFingerprint getDefault();
};

/**
 * @brief DNS configuration for country
 */
struct DNSCOnfig {
    QStringList primary;           // Primary DNS servers
    QStringList secondary;        // Backup DNS
    QStringList doh;              // DNS over HTTPS endpoints
    QStringList dot;              // DNS over TLS endpoints
    
    static DNSCOnfig getForCountry(const QString& countryCode);
    static DNSCOnfig getDefault();
};

/**
 * @brief Complete network profile for an instance
 */
struct NetworkProfile {
    QString profileId;             // UUID
    QString instanceId;            // Target instance
    
    // Mode selection
    NetworkConfigMode mode;
    
    // Proxy configuration (Mode 1)
    QString proxyHost;
    int proxyPort;
    QString proxyUsername;
    QString proxyPassword;
    QString proxyType;             // "socks5" or "http"
    
    // Country selection (Mode 2)
    QString countryCode;
    QString countryName;
    
    // Geolocation (auto-populated)
    GeolocationData geolocation;
    
    // Cellular spoofing
    CellularNetworkConfig cellular;
    
    // TCP/DNS configuration
    TCPFingerprint tcpFingerprint;
    DNSCOnfig dnsConfig;
    
    // Telephony properties for cellular spoofing
    QMap<QString, QString> telephonyProperties;
    
    // Network isolation settings
    bool blockIPv6;
    bool enforceDNS;
    bool spoofWebRTC;
    bool enableFirewall;
    
    // Status
    NetworkProfileStatus status;
    QString lastError;
    qint64 createdAt;
    qint64 appliedAt;
    
    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);
    bool save(const QString& filePath) const;
    static NetworkProfile load(const QString& filePath);
};

// ========================================================================
// NETWORK PROFILE MANAGER CLASS
// ========================================================================

/**
 * @brief NetworkProfileManager - Manages network profiles for ReDroid instances
 * 
 * This class handles:
 * - Proxy-based network configuration with automatic geolocation
 * - Country-based network configuration with virtual cellular IP
 * - Cellular network spoofing (TRANSPORT_CELLULAR)
 * - TCP/IP fingerprint randomization
 * - DNS/DOH/DOT configuration
 * - WebRTC IP spoofing
 */
class NetworkProfileManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static NetworkProfileManager& instance();
    
    // ========================================================================
    // PROFILE MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Create a new network profile
     * @param instanceId Target instance ID
     * @param mode Configuration mode
     * @return Created profile with initial data
     */
    NetworkProfile createProfile(const QString& instanceId, NetworkConfigMode mode);
    
    /**
     * @brief Get existing profile for instance
     * @param instanceId Instance ID
     * @return Profile or null if not exists
     */
    NetworkProfile getProfile(const QString& instanceId) const;
    
    /**
     * @brief Get all profiles
     * @return Map of instanceId -> profile
     */
    QMap<QString, NetworkProfile> getAllProfiles() const;
    
    /**
     * @brief Delete profile for instance
     * @param instanceId Instance ID
     * @return true if deleted
     */
    bool deleteProfile(const QString& instanceId);
    
    /**
     * @brief Save profile to disk
     * @param profile Profile to save
     * @return true if saved
     */
    bool saveProfile(const NetworkProfile& profile);
    
    /**
     * @brief Load profile from disk
     * @param instanceId Instance ID
     * @return Loaded profile
     */
    NetworkProfile loadProfile(const QString& instanceId);
    
    // ========================================================================
    // MODE 1: PROXY WITH AUTO-SYNC
    // ========================================================================
    
    /**
     * @brief Configure profile with proxy (Mode 1)
     * @param instanceId Target instance
     * @param proxyString Proxy in format "host:port" or "host:port:user:pass"
     * @param proxyType "socks5" or "http"
     * @return Updated profile
     */
    NetworkProfile configureWithProxy(const QString& instanceId,
                                     const QString& proxyString,
                                     const QString& proxyType = "socks5");
    
    /**
     * @brief Fetch geolocation data through proxy
     * @param instanceId Target instance
     * @param proxyConfig Proxy configuration
     * @param useProxyForQuery Use proxy to query the API
     * @return Geolocation data
     */
    GeolocationData fetchGeolocation(const ProxyConfig& proxyConfig,
                                     bool useProxyForQuery = true);
    
    /**
     * @brief Auto-sync profile data from geolocation
     * @param instanceId Target instance
     * @return Updated profile
     */
    NetworkProfile autoSyncFromGeolocation(const QString& instanceId);
    
    // ========================================================================
    // MODE 2: WITHOUT PROXY (MANUAL COUNTRY)
    // ========================================================================
    
    /**
     * @brief Configure profile without proxy (Mode 2)
     * @param instanceId Target instance
     * @param countryCode ISO country code (e.g., "US", "BD")
     * @return Updated profile
     */
    NetworkProfile configureWithoutProxy(const QString& instanceId,
                                        const QString& countryCode);
    
    /**
     * @brief Generate random GPS coordinates for country
     * @param countryCode Country code
     * @return Generated coordinates
     */
    QPair<double, double> generateCountryCoordinates(const QString& countryCode);
    
    /**
     * @brief Get available countries for selection
     * @return List of country data
     */
    QList<CountryData> getAvailableCountries() const;
    
    /**
     * @brief Get carrier list for country
     * @param countryCode Country code
     * @return List of carrier names
     */
    QStringList getCarriersForCountry(const QString& countryCode) const;
    
    // ========================================================================
    // CELLULAR NETWORK SPOOFING
    // ========================================================================
    
    /**
     * @brief Generate cellular network configuration
     * @param profile Network profile
     * @return Cellular config
     */
    CellularNetworkConfig generateCellularConfig(const NetworkProfile& profile);
    
    /**
     * @brief Generate telephony properties for cellular spoofing
     * @param profile Network profile
     * @return Map of property -> value
     */
    QMap<QString, QString> generateTelephonyProperties(const NetworkProfile& profile);
    
    /**
     * @brief Generate network transport hook commands
     * @param instanceId Target instance
     * @param profile Network profile
     * @return List of shell commands
     */
    QStringList generateTransportHookCommands(const QString& instanceId,
                                             const NetworkProfile& profile);
    
    // ========================================================================
    // NETWORK SPOOFING
    // ========================================================================
    
    /**
     * @brief Generate TCP/IP fingerprint for country
     * @param countryCode Country code
     * @return TCP fingerprint
     */
    TCPFingerprint generateTCPFingerprint(const QString& countryCode);
    
    /**
     * @brief Generate DNS configuration for country
     * @param countryCode Country code
     * @return DNS configuration
     */
    DNSCOnfig generateDNSConfig(const QString& countryCode);
    
    /**
     * @brief Generate WebRTC configuration for IP spoofing
     * @param instanceId Target instance
     * @param localIP Local IP to report
     * @return Shell commands for WebRTC setup
     */
    QStringList generateWebRTCSetupCommands(const QString& instanceId,
                                           const QString& localIP);
    
    // ========================================================================
    // PROFILE APPLICATION
    // ========================================================================
    
    /**
     * @brief Apply network profile to instance
     * @param profile Network profile to apply
     * @return true if applied successfully
     */
    bool applyProfile(const NetworkProfile& profile);
    
    /**
     * @brief Apply profile to instance (async)
     * @param profile Network profile
     * @note Emits profileApplied signal on completion
     */
    void applyProfileAsync(const NetworkProfile& profile);
    
    /**
     * @brief Remove network profile from instance
     * @param instanceId Instance ID
     * @return true if removed
     */
    bool removeProfile(const QString& instanceId);
    
    /**
     * @brief Verify profile is correctly applied
     * @param instanceId Instance ID
     * @return Verification result
     */
    QJsonObject verifyProfile(const QString& instanceId);
    
    // ========================================================================
    // UTILITY
    // ========================================================================
    
    /**
     * @brief Generate init script for container
     * @param profile Network profile
     * @return Shell script content
     */
    QString generateInitScript(const NetworkProfile& profile);
    
    /**
     * @brief Parse proxy string
     * @param proxyString Proxy in format "host:port" or "host:port:user:pass"
     * @return Parsed proxy config
     */
    ProxyConfig parseProxyString(const QString& proxyString);
    
    /**
     * @brief Extract carrier info from ISP name
     * @param ispName ISP name
     * @return Carrier information
     */
    QPair<QString, QString> extractCarrierInfo(const QString& ispName);
    
    /**
     * @brief Validate country code
     * @param code Country code
     * @return true if valid
     */
    bool isValidCountryCode(const QString& code) const;
    
    /**
     * @brief Get country flag emoji
     * @param countryCode Country code
     * @return Flag emoji
     */
    QString getCountryFlag(const QString& countryCode) const;

signals:
    /**
     * @brief Emitted when profile status changes
     */
    void profileStatusChanged(const QString& instanceId, NetworkProfileStatus status);
    
    /**
     * @brief Emitted when profile is applied
     */
    void profileApplied(const QString& instanceId, bool success, const QString& error);
    
    /**
     * @brief Emitted when geolocation is fetched
     */
    void geolocationFetched(const QString& instanceId, const GeolocationData& data);
    
    /**
     * @brief Emitted during profile creation
     */
    void progressUpdated(const QString& instanceId, int percent, const QString& message);
    
    /**
     * @brief Emitted on error
     */
    void error(const QString& message);

private:
    explicit NetworkProfileManager(QObject* parent = nullptr);
    ~NetworkProfileManager();
    Q_DISABLE_COPY(NetworkProfileManager)
    
    // Internal helpers
    GeolocationData fetchGeolocationFromAPI(const QString& proxyHost,
                                           int proxyPort,
                                           const QString& proxyUser = QString(),
                                           const QString& proxyPass = QString());
    
    QString generateMCCFromCountry(const QString& countryCode);
    QString generateMNCForCarrier(const QString& carrierName, const QString& mcc);
    
    QStringList buildCellularSetupCommands(const NetworkProfile& profile);
    QStringList buildProxyRoutingCommands(const NetworkProfile& profile);
    
    bool executeShellCommand(const QString& command, QString& output, int timeoutMs = 30000);
    bool pushScriptToInstance(const QString& instanceId, const QString& localPath, const QString& remotePath);
    bool executeScriptOnInstance(const QString& instanceId, const QString& scriptPath);
    
    QString getCountryTimezone(const QString& countryCode);
    QPair<QString, QString> getCountryCarrier(const QString& countryCode);
    
    void initializeCountryDataset();
    void saveProfilesToDisk();
    void loadProfilesFromDisk();
    
    // Singleton
    static NetworkProfileManager* s_instance;
    
    // State
    QNetworkAccessManager* m_networkManager;
    QMap<QString, NetworkProfile> m_profiles;
    QList<CountryData> m_countryDataset;
    QMutex m_mutex;
    QMutex m_networkMutex;
    
    // Configuration
    QString m_profilesDir;
    QString m_scriptsDir;
    
    // API endpoints
    const QString GEOLOCATION_API_URL = "http://ip-api.com/json";
    const QString GEOLOCATION_FALLBACK_URL = "https://ipapi.co/json";
    
    // Internal state
    bool m_initialized;
    QMap<QString, QString> m_pendingOperations;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_NETWORK_PROFILE_MANAGER_H
