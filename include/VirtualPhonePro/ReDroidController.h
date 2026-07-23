#pragma once

#ifndef VIRTUALPHONEPRO_REDROIDCONTROLLER_H
#define VIRTUALPHONEPRO_REDROIDCONTROLLER_H

#include "VirtualPhonePro/NetworkConfig.h"
#include "VirtualPhonePro/NetworkConfigManager.h"
#include "VirtualPhonePro/DeviceProfile.h"
#include "VirtualPhonePro/AndroidRealismEngine.h"
#include "VirtualPhonePro/TimingAttackPrevention.hpp"

#include <QObject>
#include <QString>
#include <QProcess>
#include <QMap>
#include <QVariantMap>
#include <QPair>
#include <QMutex>
#include <QTimer>
#include <QHostAddress>
#include <QCryptographicHash>

namespace VirtualPhonePro {

/**
 * @brief Container Instance Information
 */
struct InstanceInfo {
    QString instanceId;            // Unique identifier
    QString containerId;           // Docker container ID
    QString containerName;         // Docker container name
    QString imageName;             // Docker image used
    
    InstanceState state;           // Current state
    int adbPort;                   // ADB port on host
    int vncPort;                   // VNC port on host
    int instanceIndex;             // Zero-based index
    
    QString ipAddress;             // Container internal IP
    QString profileId;             // Linked device profile ID
    
    quint64 memoryUsage;           // Current memory in bytes
    quint64 memoryLimit;           // Memory limit in bytes
    double cpuUsage;               // CPU percentage
    
    qint64 startedAt;              // Start timestamp
    qint64 createdAt;              // Creation timestamp
    
    // Connection info
    bool adbConnected;             // ADB connected flag
    bool vncEnabled;               // VNC enabled flag
    
    // Network configuration
    NetworkIsolationConfig networkConfig;
    QString networkName;
};

/**
 * @brief Sensor Data for Mocking
 */
struct SensorData {
    double latitude;
    double longitude;
    double altitude;
    float accuracy;
    
    float accelerometerX;
    float accelerometerY;
    float accelerometerZ;
    
    float gyroscopeX;
    float gyroscopeY;
    float gyroscopeZ;
    
    float magneticX;
    float magneticY;
    float magneticZ;
    
    float speed;
    float bearing;
    
    qint64 timestamp;
};

/**
 * @brief Docker Configuration
 */
struct DockerConfig {
    QString dockerPath;            // Path to docker.exe
    QString adbPath;               // Path to adb.exe
    QString imageName;             // ReDroid image
    QString networkDriver;         // Network driver (bridge, host)
    
    int baseAdbPort;               // Base ADB port (default: 5555)
    int baseVncPort;               // Base VNC port (default: 5900)
    
    QString memoryLimit;           // Memory limit (e.g., "512m")
    int cpuQuota;                  // CPU quota (e.g., 200000 = 2 cores)
    int shmSize;                   // Shared memory size (e.g., 256m)
    
    bool useWSL2;                  // Use WSL2 for Docker
    
    // WSL2 specific
    QString wslDistro;             // WSL2 distribution name
    QString wslMountPrefix;        // WSL2 mount prefix (e.g., "/mnt/c")
    
    DockerConfig();
};

/**
 * @brief Result of an operation
 */
struct OperationResult {
    bool success;
    QString errorMessage;
    QVariantMap data;
    
    OperationResult() : success(false) {}
    OperationResult(bool ok, const QString& error = QString()) 
        : success(ok), errorMessage(error) {}
};

/**
 * @brief ReDroidController - Core Controller for ReDroid Containers
 * 
 * This class manages the lifecycle of ReDroid Android emulator containers
 * on Windows using Docker Desktop. It handles:
 * - Container lifecycle (start, stop, restart)
 * - Device profile application
 * - Sensor data injection
 * - ADB communication
 * - Resource monitoring
 */
class ReDroidController : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static ReDroidController& instance();
    
    /**
     * @brief Destructor
     */
    ~ReDroidController();

    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set Docker configuration
     */
    void setConfig(const DockerConfig& config);
    
    /**
     * @brief Get current configuration
     */
    DockerConfig config() const;
    
    /**
     * @brief Validate Docker installation
     */
    OperationResult validateDocker();
    
    /**
     * @brief Set ADB path (default: app directory + adb.exe)
     */
    void setAdbPath(const QString& path);
    
    /**
     * @brief Load configuration from file
     */
    void loadConfiguration();
    
    /**
     * @brief Save configuration to file
     */
    void saveConfiguration();
    
    // =========================================================================
    // Instance Lifecycle
    // =========================================================================
    
    /**
     * @brief Start a new ReDroid instance
     * @param instanceId Unique instance identifier
     * @param profile Device profile to apply
     * @return true if started successfully
     */
    bool startInstance(const QString& instanceId, const DeviceProfile& profile);
    
    /**
     * @brief Stop a running instance
     * @param instanceId Instance to stop
     * @param force Force stop (SIGKILL)
     * @return true if stopped successfully
     */
    bool stopInstance(const QString& instanceId, bool force = false);
    
    /**
     * @brief Restart an instance
     * @param instanceId Instance to restart
     * @return true if restarted successfully
     */
    bool restartInstance(const QString& instanceId);
    
    /**
     * @brief Delete an instance completely
     * @param instanceId Instance to delete
     * @return true if deleted successfully
     */
    bool deleteInstance(const QString& instanceId);
    
    /**
     * @brief Pause an instance
     * @param instanceId Instance to pause
     * @return true if paused successfully
     */
    bool pauseInstance(const QString& instanceId);
    
    /**
     * @brief Resume a paused instance
     * @param instanceId Instance to resume
     * @return true if resumed successfully
     */
    bool resumeInstance(const QString& instanceId);

    // =========================================================================
    // Instance Queries
    // =========================================================================
    
    /**
     * @brief Get list of all instances
     */
    QList<InstanceInfo> listInstances();
    
    /**
     * @brief Get info for specific instance
     */
    InstanceInfo getInstanceInfo(const QString& instanceId) const;
    
    /**
     * @brief Check if instance exists
     */
    bool instanceExists(const QString& instanceId) const;
    
    /**
     * @brief Get instance state
     */
    InstanceState getInstanceState(const QString& instanceId) const;
    
    /**
     * @brief Check if ADB is connected to instance
     */
    bool isAdbConnected(const QString& instanceId) const;

    // =========================================================================
    // Device Profile & Spoofing
    // =========================================================================
    
    /**
     * @brief Apply device profile to running instance
     * @param instanceId Target instance
     * @param profile Profile to apply
     * @return true if applied successfully
     */
    bool applyProfile(const QString& instanceId, const DeviceProfile& profile);
    
    /**
     * @brief Apply complete realism configuration for 100% realistic Android
     * @param instanceId Target instance
     * @param manufacturer Device manufacturer
     * @param model Device model
     * @return true if applied successfully
     */
    bool applyCompleteRealism(const QString& instanceId, const QString& manufacturer, const QString& model);
    
    /**
     * @brief Apply individual property
     */
    bool setProperty(const QString& instanceId, const QString& prop, const QString& value);
    
    /**
     * @brief Get property value
     */
    QString getProperty(const QString& instanceId, const QString& prop);
    
    /**
     * @brief Get all properties from instance
     */
    QMap<QString, QString> getAllProperties(const QString& instanceId);

    // =========================================================================
    // Sensor Data
    // =========================================================================
    
    /**
     * @brief Send mock GPS and sensor data
     * @param instanceId Target instance
     * @param lat Latitude
     * @param lon Longitude
     * @param xAccel X-axis accelerometer
     * @param yAccel Y-axis accelerometer
     * @return true if sent successfully
     */
    bool sendSensorData(const QString& instanceId, 
                        double lat, double lon,
                        double xAccel = 0.0, double yAccel = 0.0);
    
    /**
     * @brief Send complete sensor data
     */
    bool sendSensorData(const QString& instanceId, const SensorData& data);
    
    /**
     * @brief Enable mock location mode
     */
    bool enableMockLocation(const QString& instanceId);
    
    /**
     * @brief Disable mock location mode
     */
    bool disableMockLocation(const QString& instanceId);

    // =========================================================================
    // File Operations
    // =========================================================================
    
    /**
     * @brief Push file to instance
     */
    bool pushFile(const QString& instanceId, const QString& localPath, const QString& remotePath);
    
    /**
     * @brief Pull file from instance
     */
    bool pullFile(const QString& instanceId, const QString& remotePath, const QString& localPath);
    
    /**
     * @brief Install APK
     */
    bool installApk(const QString& instanceId, const QString& apkPath);
    
    /**
     * @brief Uninstall package
     */
    bool uninstallPackage(const QString& instanceId, const QString& packageName);

    // =========================================================================
    // Screen & Input
    // =========================================================================
    
    /**
     * @brief Take screenshot
     */
    QByteArray takeScreenshot(const QString& instanceId);
    
    /**
     * @brief Tap at coordinates
     */
    bool tap(const QString& instanceId, int x, int y);
    
    /**
     * @brief Swipe gesture
     */
    bool swipe(const QString& instanceId, int x1, int y1, int x2, int y2, int durationMs = 300);
    
    /**
     * @brief Input text
     */
    bool inputText(const QString& instanceId, const QString& text);
    
    /**
     * @brief Press key event
     */
    bool pressKey(const QString& instanceId, int keyCode);

    // =========================================================================
    // System
    // =========================================================================
    
    /**
     * @brief Reboot instance
     */
    bool rebootInstance(const QString& instanceId, const QString& mode = QString());
    
    /**
     * @brief Execute shell command
     */
    QString executeShell(const QString& instanceId, const QString& command, int timeoutMs = 30000);
    
    /**
     * @brief Enable root access
     */
    bool enableRoot(const QString& instanceId);
    
    /**
     * @brief Get instance logs
     */
    QString getLogs(const QString& instanceId, int tail = 100);

    // =========================================================================
    // Network Isolation
    // =========================================================================
    
    /**
     * @brief Configure network isolation for an instance
     * @param instanceId Target instance
     * @param config Network isolation configuration
     * @return true if configured successfully
     */
    bool configureNetworkIsolation(const QString& instanceId, 
                                   const NetworkIsolationConfig& config);
    
    /**
     * @brief Create isolated Docker network for an instance
     * @param instanceId Target instance
     * @param subnet Network subnet (e.g., "172.28.1.0/24")
     * @return true if created successfully
     */
    bool createIsolatedNetwork(const QString& instanceId, const QString& subnet);
    
    /**
     * @brief Delete isolated network for an instance
     * @param instanceId Target instance
     * @return true if deleted successfully
     */
    bool deleteIsolatedNetwork(const QString& instanceId);
    
    /**
     * @brief Assign proxy to instance
     * @param instanceId Target instance
     * @param proxy Proxy configuration
     * @return true if assigned successfully
     */
    bool assignProxy(const QString& instanceId, const ProxyConfig& proxy);
    
    /**
     * @brief Remove proxy from instance
     * @param instanceId Target instance
     * @return true if removed successfully
     */
    bool removeProxy(const QString& instanceId);
    
    /**
     * @brief Setup VPN for instance
     * @param instanceId Target instance
     * @param vpn VPN configuration
     * @return true if setup successfully
     */
    bool setupVPN(const QString& instanceId, const VPNConfig& vpn);
    
    /**
     * @brief Prevent network leaks (MAC, IP, DNS)
     * @param instanceId Target instance
     * @return true if applied successfully
     */
    bool applyLeakPrevention(const QString& instanceId);
    
    /**
     * @brief Block IPv6 traffic
     * @param instanceId Target instance
     * @return true if blocked successfully
     */
    bool blockIPv6(const QString& instanceId);
    
    /**
     * @brief Configure DNS servers
     * @param instanceId Target instance
     * @param dnsServers List of DNS servers
     * @return true if configured successfully
     */
    bool configureDNS(const QString& instanceId, const QList<QString>& dnsServers);
    
    /**
     * @brief Get current network info for instance
     * @param instanceId Target instance
     * @return Network information JSON
     */
    QString getNetworkInfo(const QString& instanceId);
    
    /**
     * @brief Test for network leaks
     * @param instanceId Target instance
     * @return true if leaks detected
     */
    bool testForLeaks(const QString& instanceId);

signals:
    /**
     * @brief Emitted when instance state changes
     */
    void instanceStateChanged(const QString& instanceId, InstanceState state);
    
    /**
     * @brief Emitted when ADB connection state changes
     */
    void adbConnectionChanged(const QString& instanceId, bool connected);
    
    /**
     * @brief Emitted when an error occurs
     */
    void error(const QString& message);
    
    /**
     * @brief Emitted when operation completes
     */
    void operationCompleted(const QString& instanceId, const QString& operation, bool success);

private:
    explicit ReDroidController(QObject* parent = nullptr);
    Q_DISABLE_COPY(ReDroidController)
    
    // Internal helpers
    QJsonObject loadProfile(const QString& profileName);
    QString getAdbSerial(const QString& instanceId) const;
    QString getContainerName(const QString& instanceId) const;
    int allocateAdbPort();
    int allocateVncPort();
    
    QString convertToWSL2Path(const QString& windowsPath) const;
    QString convertToWindowsPath(const QString& wsl2Path) const;
    
    // Docker command execution
    OperationResult executeDocker(const QStringList& args, int timeoutMs = 30000);
    QString executeDockerSync(const QStringList& args, int timeoutMs = 30000);
    
    // ADB command execution
    OperationResult executeAdb(const QString& instanceId, const QStringList& args, int timeoutMs = 30000);
    QString executeAdbSync(const QString& instanceId, const QStringList& args, int timeoutMs = 30000);
    
    // Property file generation
    QString generatePropertyFile(const DeviceProfile& profile, const QString& filePath);
    
    // Instance monitoring
    void startMonitoring();
    void stopMonitoring();
    void checkInstanceStatus(const QString& instanceId);
    
    // Singleton
    static ReDroidController* s_instance;
    
    // Configuration
    DockerConfig m_config;
    
    // Instance management
    QMap<QString, InstanceInfo> m_instances;
    mutable QMutex m_instancesMutex;
    int m_nextAdbPort;
    int m_nextVncPort;
    
    // Monitoring
    QTimer* m_monitoringTimer;
    bool m_monitoring;
    
    // ADB connection pool
    QMap<QString, QString> m_adbSerials;  // instanceId -> serial
    
    // App paths
    QString m_appDir;

public:
    // ========================================================================
    // UNIQUE DEVICE PROFILE SYSTEM
    // ========================================================================
    
    /**
     * @brief Generate a unique profile for a specific device instance
     * @param instanceId The device instance ID
     * @param profileName Base profile name to use
     * @return JSON object with unique values
     */
    QJsonObject generateUniqueProfile(const QString& instanceId, const QString& profileName);
    
    /**
     * @brief Apply unique profile values to a device
     * @param instanceId The device instance ID
     * @param profile JSON profile with unique values
     * @return Success status
     */
    bool applyUniqueProfile(const QString& instanceId, const QJsonObject& profile);
    
    /**
     * @brief Get unique fingerprint hash for this device
     * @param instanceId The device instance ID
     * @return SHA256 hash of device identifiers
     */
    QString getDeviceUniqueFingerprint(const QString& instanceId);
    
    /**
     * @brief Verify that a device has unique identifiers
     * @param instanceId The device instance ID
     * @return True if device appears unique
     */
    bool verifyDeviceUniqueness(const QString& instanceId);
    
    // ========================================================================
    // REALISTIC TOUCH SIMULATION
    // ========================================================================
    
    /**
     * @brief Perform a realistic tap with human-like timing
     * @param instanceId The device instance ID
     * @param x X coordinate
     * @param y Y coordinate
     * @return Success status
     */
    bool performRealisticTap(const QString& instanceId, int x, int y);
    
    /**
     * @brief Perform a realistic swipe with human-like timing
     * @param instanceId The device instance ID
     * @param x1 Start X coordinate
     * @param y1 Start Y coordinate
     * @param x2 End X coordinate
     * @param y2 End Y coordinate
     * @return Success status
     */
    bool performRealisticSwipe(const QString& instanceId, int x1, int y1, int x2, int y2);
    
    /**
     * @brief Type text with realistic human-like timing
     * @param instanceId The device instance ID
     * @param text Text to type
     * @return Success status
     */
    bool performRealisticType(const QString& instanceId, const QString& text);
};

/**
 * @brief Key event codes
 */
namespace KeyCode {
    constexpr int HOME = 3;
    constexpr int BACK = 4;
    constexpr int MENU = 82;
    constexpr int POWER = 26;
    constexpr int VOLUME_UP = 24;
    constexpr int VOLUME_DOWN = 25;
    constexpr int ENTER = 66;
    constexpr int DELETE = 67;
    constexpr int SPACE = 62;
}

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_REDROIDCONTROLLER_H
