#include "VirtualPhonePro/ReDroidController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QMutexLocker>
#include <QUuid>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace VirtualPhonePro {

// ============================================================================
// Singleton
// ============================================================================

ReDroidController* ReDroidController::s_instance = nullptr;

ReDroidController& ReDroidController::instance() {
    if (!s_instance) {
        s_instance = new ReDroidController();
    }
    return *s_instance;
}

// ============================================================================
// DockerConfig Implementation
// ============================================================================

DockerConfig::DockerConfig()
    : dockerPath("docker")
    , adbPath("")
    , imageName("ghcr.io/redroid/redroid:14.0.0_google_64only")
    , networkDriver("bridge")
    , baseAdbPort(5555)
    , baseVncPort(5900)
    , memoryLimit("512m")
    , cpuQuota(200000)
    , shmSize(256)
    , useWSL2(false)
    , wslDistro("Ubuntu-22.04")
    , wslMountPrefix("/mnt/c")
{
}

// ============================================================================
// Constructor & Destructor
// ============================================================================

ReDroidController::ReDroidController(QObject* parent)
    : QObject(parent)
    , m_nextAdbPort(5555)
    , m_nextVncPort(5900)
    , m_monitoringTimer(nullptr)
    , m_monitoring(false)
{
    // Set app directory
    m_appDir = QCoreApplication::applicationDirPath();
    
    // Set default ADB path
    QString defaultAdbPath = m_appDir + QDir::separator() + "adb.exe";
    if (QFile::exists(defaultAdbPath)) {
        m_config.adbPath = defaultAdbPath;
    }
    
    // Load configuration
    loadConfiguration();
    
    // Start monitoring timer
    m_monitoringTimer = new QTimer(this);
    connect(m_monitoringTimer, &QTimer::timeout, this, [this]() {
        for (const QString& id : m_instances.keys()) {
            checkInstanceStatus(id);
        }
    });
}

ReDroidController::~ReDroidController() {
    stopMonitoring();
    
    // Stop all running instances
    for (const QString& id : m_instances.keys()) {
        stopInstance(id, true);
    }
}

// ============================================================================
// Configuration
// ============================================================================

void ReDroidController::setConfig(const DockerConfig& config) {
    m_config = config;
    saveConfiguration();
}

DockerConfig ReDroidController::config() const {
    return m_config;
}

void ReDroidController::setAdbPath(const QString& path) {
    m_config.adbPath = path;
}

OperationResult ReDroidController::validateDocker() {
    OperationResult result;
    
    // Check if docker is installed
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    QString dockerExe = m_config.dockerPath;
    
#ifdef Q_OS_WIN32
    // Try to find docker.exe in PATH
    if (dockerExe == "docker") {
        process.start("where", {"docker.exe"});
        if (process.waitForFinished(5000)) {
            QString output = process.readAll().trimmed();
            if (!output.isEmpty()) {
                dockerExe = output.split('\n').first();
            }
        }
    }
#endif
    
    process.start(dockerExe, {"version", "--format", "{{.Server.Version}}"});
    
    if (!process.waitForFinished(10000)) {
        result.errorMessage = "Docker is not running or not installed";
        return result;
    }
    
    if (process.exitCode() != 0) {
        result.errorMessage = QString("Docker error: %1").arg(process.readAll());
        return result;
    }
    
    result.success = true;
    result.data["version"] = process.readAll().trimmed();
    
    return result;
}

void ReDroidController::loadConfiguration() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    configPath += "/config.ini";
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    m_config.dockerPath = settings.value("docker/path", m_config.dockerPath).toString();
    m_config.imageName = settings.value("docker/image", m_config.imageName).toString();
    m_config.memoryLimit = settings.value("docker/memoryLimit", m_config.memoryLimit).toString();
    m_config.cpuQuota = settings.value("docker/cpuQuota", m_config.cpuQuota).toInt();
    m_config.shmSize = settings.value("docker/shmSize", m_config.shmSize).toInt();
    m_config.baseAdbPort = settings.value("docker/baseAdbPort", m_config.baseAdbPort).toInt();
    m_config.baseVncPort = settings.value("docker/baseVncPort", m_config.baseVncPort).toInt();
    m_config.useWSL2 = settings.value("docker/useWSL2", m_config.useWSL2).toBool();
    m_config.wslDistro = settings.value("docker/wslDistro", m_config.wslDistro).toString();
}

void ReDroidController::saveConfiguration() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    configPath += "/config.ini";
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.setValue("docker/path", m_config.dockerPath);
    settings.setValue("docker/image", m_config.imageName);
    settings.setValue("docker/memoryLimit", m_config.memoryLimit);
    settings.setValue("docker/cpuQuota", m_config.cpuQuota);
    settings.setValue("docker/shmSize", m_config.shmSize);
    settings.setValue("docker/baseAdbPort", m_config.baseAdbPort);
    settings.setValue("docker/baseVncPort", m_config.baseVncPort);
    settings.setValue("docker/useWSL2", m_config.useWSL2);
    settings.setValue("docker/wslDistro", m_config.wslDistro);
}

// ============================================================================
// Instance Lifecycle
// ============================================================================

bool ReDroidController::startInstance(const QString& instanceId, const DeviceProfile& profile) {
    qDebug() << "Starting instance:" << instanceId;
    
    QMutexLocker locker(&m_instancesMutex);
    
    // Check if already running
    if (m_instances.contains(instanceId)) {
        InstanceInfo& info = m_instances[instanceId];
        if (info.state == InstanceState::Running) {
            qWarning() << "Instance already running:" << instanceId;
            return true;
        }
    }
    
    // Allocate ports
    int adbPort = allocateAdbPort();
    int vncPort = allocateVncPort();
    
    // Create instance info
    InstanceInfo info;
    info.instanceId = instanceId;
    info.containerName = QString("vpp-%1").arg(instanceId);
    info.imageName = m_config.imageName;
    info.state = InstanceState::Starting;
    info.adbPort = adbPort;
    info.vncPort = vncPort;
    info.instanceIndex = m_instances.size();
    info.createdAt = QDateTime::currentMSecsSinceEpoch();
    info.profileId = profile.id;
    info.adbConnected = false;
    info.vncEnabled = true;
    
    // Generate property file path
    QString profileDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(profileDataDir + "/instances/" + instanceId);
    QString propertyFile = profileDataDir + "/instances/" + instanceId + "/device.properties";
    
    // Generate property file
    generatePropertyFile(profile, propertyFile);
    
    // Build docker run command
    QStringList args;
    args << "run" << "-d";
    
    // Container name
    args << "--name" << info.containerName;
    
    // Privileged mode (required for Android)
    args << "--privileged";
    
    // Memory limit
    args << "-m" << m_config.memoryLimit;
    
    // SHM size
    args << "--shm-size" << QString("%1m").arg(m_config.shmSize);
    
    // CPU quota
    args << "--cpu-quota" << QString::number(m_config.cpuQuota);
    
    // Hostname
    args << "-h" << QString("android-%1").arg(instanceId);
    
    // Environment variables for device properties
    args << "-e" << QString("VPP_DEVICE_MANUFACTURER=%1").arg(profile.build.manufacturer);
    args << "-e" << QString("VPP_DEVICE_BRAND=%1").arg(profile.build.brand);
    args << "-e" << QString("VPP_DEVICE_MODEL=%1").arg(profile.build.model);
    args << "-e" << QString("VPP_DEVICE=%1").arg(profile.build.device);
    args << "-e" << QString("VPP_PRODUCT=%1").arg(profile.build.product);
    args << "-e" << QString("VPP_ANDROID_VERSION=%1").arg(profile.build.androidVersion);
    args << "-e" << QString("VPP_BUILD_ID=%1").arg(profile.build.buildId);
    args << "-e" << QString("VPP_BUILD_TYPE=%1").arg(profile.build.buildType);
    args << "-e" << QString("VPP_BOOTLOADER=%1").arg(profile.build.bootloader);
    args << "-e" << QString("VPP_SECURITY_PATCH=%1").arg(profile.build.securityPatch);
    args << "-e" << QString("VPP_IMEI=%1").arg(profile.identity.imei);
    args << "-e" << QString("VPP_IMEI2=%1").arg(profile.identity.imei2);
    args << "-e" << QString("VPP_SERIAL=%1").arg(profile.identity.serialNumber);
    args << "-e" << QString("VPP_ANDROID_ID=%1").arg(profile.identity.androidId);
    args << "-e" << QString("VPP_GSF_ID=%1").arg(profile.identity.gsfId);
    args << "-e" << QString("VPP_WIFI_MAC=%1").arg(profile.mac.wifiMac);
    args << "-e" << QString("VPP_BLUETOOTH_MAC=%1").arg(profile.mac.bluetoothMac);
    args << "-e" << QString("VPP_HOSTNAME=%1").arg(profile.network.hostname);
    args << "-e" << QString("VPP_ICCID=%1").arg(profile.sim.iccid);
    args << "-e" << QString("VPP_IMSI=%1").arg(profile.sim.imsi);
    args << "-e" << QString("VPP_CARRIER=%1").arg(profile.sim.carrier);
    args << "-e" << QString("VPP_MCC=%1").arg(profile.sim.mcc);
    args << "-e" << QString("VPP_MNC=%1").arg(profile.sim.mnc);
    args << "-e" << QString("VPP_GPS_LAT=%1").arg(profile.gps.latitude);
    args << "-e" << QString("VPP_GPS_LON=%1").arg(profile.gps.longitude);
    
    // GPU mode (swiftshader for Windows compatibility)
    args << "-e" << "REDROID_GPU_MODE=swiftshader";
    
    // Redroid configuration
    args << "-e" << "REDROID_CTS=0";
    args << "-e" << "ROG_BOOTANIMATION=false";
    args << "-e" << "ROG_DISABLE_FPS_LIMIT=true";
    
    // Ports
    args << "-p" << QString("%1:5555").arg(adbPort);
    args << "-p" << QString("%1:5900").arg(vncPort);
    
    // Device passthrough
    args << "-v" << QString("%1:/opt/vpp/config/device.properties:ro").arg(propertyFile);
    
    // Labels
    args << "-l" << QString("vpp.instance=%1").arg(instanceId);
    args << "-l" << QString("vpp.profile=%1").arg(profile.id);
    
    // Restart policy
    args << "--restart" << "unless-stopped";
    
    // Image
    args << m_config.imageName;
    
    // Execute docker run
    OperationResult result = executeDocker(args, 60000);
    
    if (!result.success) {
        qCritical() << "Failed to start container:" << result.errorMessage;
        info.state = InstanceState::Error;
        m_instances[instanceId] = info;
        emit error(QString("Failed to start instance: %1").arg(result.errorMessage));
        return false;
    }
    
    // Get container ID from result
    info.containerId = result.data.value("containerId").toString();
    info.ipAddress = result.data.value("ipAddress").toString();
    
    // Wait for container to be running
    int waitCount = 0;
    while (waitCount < 60) {
        QThread::msleep(1000);
        
        // Check if container is running
        QString stateOutput = executeDockerSync({"inspect", "-f", "{{.State.Status}}", info.containerName});
        if (stateOutput.trimmed() == "running") {
            break;
        }
        waitCount++;
    }
    
    // Update instance info
    info.state = InstanceState::Running;
    info.startedAt = QDateTime::currentMSecsSinceEpoch();
    m_instances[instanceId] = info;
    
    // Wait for ADB to be ready
    QThread::msleep(5000);
    
    // Connect ADB
    QString adbSerial = QString("127.0.0.1:%1").arg(adbPort);
    executeAdbSync(instanceId, {"connect", adbSerial}, 15000);
    
    // Store ADB serial
    m_adbSerials[instanceId] = adbSerial;
    
    // Check ADB connection
    QString devicesOutput = executeAdbSync(instanceId, {"devices"});
    info.adbConnected = devicesOutput.contains(adbSerial);
    
    m_instances[instanceId] = info;
    
    // Start monitoring
    if (!m_monitoring) {
        startMonitoring();
    }
    
    qDebug() << "Instance started successfully:" << instanceId;
    
    emit instanceStateChanged(instanceId, InstanceState::Running);
    emit operationCompleted(instanceId, "start", true);
    
    return true;
}

bool ReDroidController::stopInstance(const QString& instanceId, bool force) {
    qDebug() << "Stopping instance:" << instanceId << "force:" << force;
    
    QMutexLocker locker(&m_instancesMutex);
    
    if (!m_instances.contains(instanceId)) {
        qWarning() << "Instance not found:" << instanceId;
        return false;
    }
    
    InstanceInfo& info = m_instances[instanceId];
    QString containerName = info.containerName;
    
    // Disconnect ADB first
    QString adbSerial = m_adbSerials.value(instanceId);
    if (!adbSerial.isEmpty()) {
        executeAdbSync(instanceId, {"disconnect", adbSerial});
        m_adbSerials.remove(instanceId);
    }
    
    // Stop container
    QStringList stopArgs = {"stop"};
    if (force) {
        stopArgs << "-t" << "0";
    }
    stopArgs << containerName;
    
    OperationResult result = executeDocker(stopArgs, 30000);
    
    if (!result.success) {
        qWarning() << "Failed to stop container:" << result.errorMessage;
    }
    
    // Remove container
    result = executeDocker({"rm", "-f", containerName}, 10000);
    
    // Update state
    info.state = InstanceState::Stopped;
    m_instances[instanceId] = info;
    
    emit instanceStateChanged(instanceId, InstanceState::Stopped);
    emit operationCompleted(instanceId, "stop", true);
    
    return true;
}

bool ReDroidController::restartInstance(const QString& instanceId) {
    qDebug() << "Restarting instance:" << instanceId;
    
    if (!stopInstance(instanceId, true)) {
        return false;
    }
    
    QThread::msleep(2000);
    
    // Get profile
    DeviceProfile profile;
    if (m_instances.contains(instanceId)) {
        QString profileId = m_instances[instanceId].profileId;
        QString profileDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString profilePath = profileDir + "/profiles/" + profileId + ".json";
        profile = DeviceProfile::load(profilePath);
    }
    
    if (profile.id.isEmpty()) {
        profile = DeviceProfile::createSamsungS24Ultra();
    }
    
    return startInstance(instanceId, profile);
}

bool ReDroidController::deleteInstance(const QString& instanceId) {
    qDebug() << "Deleting instance:" << instanceId;
    
    // Stop first
    stopInstance(instanceId, true);
    
    QMutexLocker locker(&m_instancesMutex);
    
    // Remove from list
    m_instances.remove(instanceId);
    
    // Remove property file
    QString profileDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString propertyFile = profileDataDir + "/instances/" + instanceId + "/device.properties";
    QFile::remove(propertyFile);
    
    return true;
}

bool ReDroidController::pauseInstance(const QString& instanceId) {
    qDebug() << "Pausing instance:" << instanceId;
    
    if (!m_instances.contains(instanceId)) {
        return false;
    }
    
    QString containerName = m_instances[instanceId].containerName;
    OperationResult result = executeDocker({"pause", containerName});
    
    if (result.success) {
        m_instances[instanceId].state = InstanceState::Paused;
        emit instanceStateChanged(instanceId, InstanceState::Paused);
    }
    
    return result.success;
}

bool ReDroidController::resumeInstance(const QString& instanceId) {
    qDebug() << "Resuming instance:" << instanceId;
    
    if (!m_instances.contains(instanceId)) {
        return false;
    }
    
    QString containerName = m_instances[instanceId].containerName;
    OperationResult result = executeDocker({"unpause", containerName});
    
    if (result.success) {
        m_instances[instanceId].state = InstanceState::Running;
        emit instanceStateChanged(instanceId, InstanceState::Running);
    }
    
    return result.success;
}

// ============================================================================
// Instance Queries
// ============================================================================

QList<InstanceInfo> ReDroidController::listInstances() {
    QMutexLocker locker(&m_instancesMutex);
    return m_instances.values();
}

InstanceInfo ReDroidController::getInstanceInfo(const QString& instanceId) const {
    QMutexLocker locker(&m_instancesMutex);
    return m_instances.value(instanceId);
}

bool ReDroidController::instanceExists(const QString& instanceId) const {
    QMutexLocker locker(&m_instancesMutex);
    return m_instances.contains(instanceId);
}

InstanceState ReDroidController::getInstanceState(const QString& instanceId) const {
    QMutexLocker locker(&m_instancesMutex);
    return m_instances.value(instanceId).state;
}

bool ReDroidController::isAdbConnected(const QString& instanceId) const {
    QMutexLocker locker(&m_instancesMutex);
    return m_instances.value(instanceId).adbConnected;
}

// ============================================================================
// Device Profile & Spoofing
// ============================================================================

bool ReDroidController::applyProfile(const QString& instanceId, const DeviceProfile& profile) {
    qDebug() << "Applying profile to instance:" << instanceId;
    
    // Set build properties
    setProperty(instanceId, "ro.product.brand", profile.build.brand);
    setProperty(instanceId, "ro.product.manufacturer", profile.build.manufacturer);
    setProperty(instanceId, "ro.product.model", profile.build.model);
    setProperty(instanceId, "ro.product.device", profile.build.device);
    setProperty(instanceId, "ro.product.name", profile.build.product);
    
    // Set build fingerprint
    QString fingerprint = QString("%1/%2/%3:%4/%5:userdebug/release-keys")
        .arg(profile.build.brand)
        .arg(profile.build.device)
        .arg(profile.build.device)
        .arg(profile.build.androidVersion)
        .arg(profile.build.buildId);
    
    setProperty(instanceId, "ro.build.fingerprint", fingerprint);
    
    // Set identity
    setProperty(instanceId, "ro.serialno", profile.identity.serialNumber);
    setProperty(instanceId, "ro.gsm.device.imei", profile.identity.imei);
    setProperty(instanceId, "ro.android_id", profile.identity.androidId);
    
    // Set network
    setProperty(instanceId, "net.hostname", profile.network.hostname);
    
    // Set verified boot state
    setProperty(instanceId, "ro.boot.verifiedbootstate", "green");
    setProperty(instanceId, "ro.boot.flash.locked", "1");
    
    return true;
}

bool ReDroidController::applyCompleteRealism(const QString& instanceId, const QString& manufacturer, const QString& model) {
    qDebug() << "Applying complete realism for:" << manufacturer << model;
    
    // Initialize and apply Android Realism Engine
    AndroidRealismEngine& engine = AndroidRealismEngine::instance();
    
    // Initialize for this device
    engine.initialize(instanceId, manufacturer, model);
    
    // Apply complete configuration
    bool result = engine.applyCompleteConfiguration(instanceId);
    
    if (result) {
        qDebug() << "Complete realism applied successfully";
    } else {
        qWarning() << "Failed to apply complete realism";
    }
    
    return result;
}

bool ReDroidController::setProperty(const QString& instanceId, const QString& prop, const QString& value) {
    QString cmd = QString("setprop %1 %2").arg(prop).arg(value);
    QString result = executeShell(instanceId, cmd);
    return result.isEmpty() || !result.contains("error");
}

QString ReDroidController::getProperty(const QString& instanceId, const QString& prop) {
    QString cmd = QString("getprop %1").arg(prop);
    return executeShell(instanceId, cmd).trimmed();
}

QMap<QString, QString> ReDroidController::getAllProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    QString output = executeShell(instanceId, "getprop");
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        // Format: [property]: [value]
        int colonPos = line.indexOf('[');
        int colonEnd = line.lastIndexOf(']');
        
        if (colonPos != -1 && colonEnd != -1) {
            QString prop = line.mid(colonPos + 1, colonEnd - colonPos - 1);
            int valueStart = line.lastIndexOf('[', colonEnd - 1) + 1;
            int valueEnd = line.lastIndexOf(']');
            QString value = line.mid(valueStart, valueEnd - valueStart);
            
            props[prop] = value;
        }
    }
    
    return props;
}

// ============================================================================
// Sensor Data
// ============================================================================

bool ReDroidController::sendSensorData(const QString& instanceId,
                                      double lat, double lon,
                                      double xAccel, double yAccel) {
    SensorData data;
    data.latitude = lat;
    data.longitude = lon;
    data.accelerometerX = xAccel;
    data.accelerometerY = yAccel;
    data.accelerometerZ = 9.81;  // Gravity
    data.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return sendSensorData(instanceId, data);
}

bool ReDroidController::sendSensorData(const QString& instanceId, const SensorData& data) {
    qDebug() << "Sending sensor data to instance:" << instanceId;
    
    // Enable mock location first
    enableMockLocation(instanceId);
    
    // Format: latitude,longitude,altitude,accuracy,speed,bearing,time,provider
    QString location = QString("%1,%2,%3,%4,%5,%6,%7,%8")
        .arg(data.latitude, 0, 'f', 7)
        .arg(data.longitude, 0, 'f', 7)
        .arg(data.altitude, 0, 'f', 2)
        .arg(data.accuracy, 0, 'f', 2)
        .arg(data.speed, 0, 'f', 2)
        .arg(data.bearing, 0, 'f', 2)
        .arg(data.timestamp)
        .arg("gps");
    
    // Send GPS mock via ADB
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", "am", "broadcast",
        "-a", "android.location.GPS_FIX",
        "--ei", "latitude", QString::number(static_cast<int>(data.latitude * 1000000)),
        "--ei", "longitude", QString::number(static_cast<int>(data.longitude * 1000000)),
        "--ei", "accuracy", "5"
    };
    
    executeAdbSync(instanceId, args, 5000);
    
    // Send accelerometer mock via ADB
    QString accelCmd = QString(
        "while true; do "
        "input tap 100 100; "
        "sleep 0.1; "
        "done"
    );
    
    // Alternative: Use app's mock location permission
    QString mockCmd = QString(
        "settings put secure mock_location 1 && "
        "appops set com.android.locationpolicy.MOCK_LOCATION allow && "
        "content insert --uri content://settings/secure "
        "--bind name:s:mock_location "
        "--bind value:s:1"
    );
    
    executeShell(instanceId, mockCmd);
    
    return true;
}

bool ReDroidController::enableMockLocation(const QString& instanceId) {
    QStringList cmds = {
        "settings put secure mock_location 1",
        "settings put global allow_mock_location 1",
        "appops set android.core.mutable_permissions allow"
    };
    
    for (const QString& cmd : cmds) {
        executeShell(instanceId, cmd);
    }
    
    return true;
}

bool ReDroidController::disableMockLocation(const QString& instanceId) {
    QStringList cmds = {
        "settings put secure mock_location 0",
        "settings put global allow_mock_location 0"
    };
    
    for (const QString& cmd : cmds) {
        executeShell(instanceId, cmd);
    }
    
    return true;
}

// ============================================================================
// File Operations
// ============================================================================

bool ReDroidController::pushFile(const QString& instanceId, const QString& localPath, const QString& remotePath) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "push", localPath, remotePath
    };
    
    OperationResult result = executeAdb(instanceId, args, 60000);
    return result.success;
}

bool ReDroidController::pullFile(const QString& instanceId, const QString& remotePath, const QString& localPath) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "pull", remotePath, localPath
    };
    
    OperationResult result = executeAdb(instanceId, args, 60000);
    return result.success;
}

bool ReDroidController::installApk(const QString& instanceId, const QString& apkPath) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "install", "-r", apkPath
    };
    
    OperationResult result = executeAdb(instanceId, args, 120000);
    return result.success;
}

bool ReDroidController::uninstallPackage(const QString& instanceId, const QString& packageName) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "uninstall", packageName
    };
    
    OperationResult result = executeAdb(instanceId, args, 30000);
    return result.success;
}

// ============================================================================
// Screen & Input
// ============================================================================

QByteArray ReDroidController::takeScreenshot(const QString& instanceId) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "exec-out", "screencap", "-p"
    };
    
    OperationResult result = executeAdb(instanceId, args, 30000);
    
    if (result.success) {
        return result.data.value("output").toByteArray();
    }
    
    return QByteArray();
}

bool ReDroidController::tap(const QString& instanceId, int x, int y) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", "input", "tap", QString::number(x), QString::number(y)
    };
    
    OperationResult result = executeAdb(instanceId, args, 5000);
    return result.success;
}

bool ReDroidController::swipe(const QString& instanceId, int x1, int y1, int x2, int y2, int durationMs) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", "input", "swipe",
        QString::number(x1), QString::number(y1),
        QString::number(x2), QString::number(y2),
        QString::number(durationMs)
    };
    
    OperationResult result = executeAdb(instanceId, args, 5000);
    return result.success;
}

bool ReDroidController::inputText(const QString& instanceId, const QString& text) {
    // Escape special characters
    QString escaped = text;
    escaped.replace("\\", "\\\\");
    escaped.replace(" ", "\\ ");
    escaped.replace("(", "\\(");
    escaped.replace(")", "\\)");
    escaped.replace("'", "\\'");
    escaped.replace("\"", "\\\"");
    
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", "input", "text", escaped
    };
    
    OperationResult result = executeAdb(instanceId, args, 5000);
    return result.success;
}

bool ReDroidController::pressKey(const QString& instanceId, int keyCode) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", "input", "keyevent", QString::number(keyCode)
    };
    
    OperationResult result = executeAdb(instanceId, args, 5000);
    return result.success;
}

// ============================================================================
// System
// ============================================================================

bool ReDroidController::rebootInstance(const QString& instanceId, const QString& mode) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "reboot"
    };
    
    if (!mode.isEmpty()) {
        args << mode;
    }
    
    OperationResult result = executeAdb(instanceId, args, 30000);
    
    if (result.success) {
        m_instances[instanceId].state = InstanceState::Starting;
        emit instanceStateChanged(instanceId, InstanceState::Starting);
    }
    
    return result.success;
}

QString ReDroidController::executeShell(const QString& instanceId, const QString& command, int timeoutMs) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "shell", command
    };
    
    OperationResult result = executeAdb(instanceId, args, timeoutMs);
    return result.data.value("output").toString();
}

bool ReDroidController::enableRoot(const QString& instanceId) {
    QStringList args = {
        "-s", getAdbSerial(instanceId),
        "root"
    };
    
    OperationResult result = executeAdb(instanceId, args, 30000);
    
    // Reconnect after root
    QThread::msleep(2000);
    executeAdbSync(instanceId, {"disconnect", getAdbSerial(instanceId)});
    executeAdbSync(instanceId, {"connect", getAdbSerial(instanceId)});
    
    return result.success;
}

QString ReDroidController::getLogs(const QString& instanceId, int tail) {
    QStringList args = {
        "logs", "--tail", QString::number(tail), m_instances[instanceId].containerName
    };
    
    return executeDockerSync(args);
}

// ============================================================================
// Internal Helpers
// ============================================================================

QString ReDroidController::getAdbSerial(const QString& instanceId) const {
    return m_adbSerials.value(instanceId, QString("127.0.0.1:%1").arg(m_instances.value(instanceId).adbPort));
}

QString ReDroidController::getContainerName(const QString& instanceId) const {
    return QString("vpp-%1").arg(instanceId);
}

int ReDroidController::allocateAdbPort() {
    QMutexLocker locker(&m_instancesMutex);
    
    // Find an unused port
    while (true) {
        bool used = false;
        for (const InstanceInfo& info : m_instances.values()) {
            if (info.adbPort == m_nextAdbPort) {
                used = true;
                break;
            }
        }
        
        if (!used) {
            return m_nextAdbPort++;
        }
        
        m_nextAdbPort++;
        
        // Reset if we hit the limit
        if (m_nextAdbPort > 65535) {
            m_nextAdbPort = 5555;
        }
    }
}

int ReDroidController::allocateVncPort() {
    QMutexLocker locker(&m_instancesMutex);
    
    while (true) {
        bool used = false;
        for (const InstanceInfo& info : m_instances.values()) {
            if (info.vncPort == m_nextVncPort) {
                used = true;
                break;
            }
        }
        
        if (!used) {
            return m_nextVncPort++;
        }
        
        m_nextVncPort++;
        
        if (m_nextVncPort > 65535) {
            m_nextVncPort = 5900;
        }
    }
}

QString ReDroidController::convertToWSL2Path(const QString& windowsPath) const {
    QString path = windowsPath;
    
    // Convert backslashes to forward slashes
    path.replace('\\', '/');
    
    // Remove drive letter (e.g., C:)
    if (path.length() >= 3 && path[1] == ':') {
        QString drive = path[0].toUpper();
        path = path.mid(3);
        path = m_config.wslMountPrefix + "/" + drive + "/" + path;
    }
    
    return path;
}

QString ReDroidController::convertToWindowsPath(const QString& wsl2Path) const {
    QString path = wsl2Path;
    
    // Handle /mnt/c/... format
    if (path.startsWith("/mnt/")) {
        path = path.mid(5);  // Remove /mnt/
        int slashPos = path.indexOf('/');
        if (slashPos != -1) {
            QString drive = path.left(slashPos).toUpper();
            path = path.mid(slashPos);
            path.replace('/', '\\');
            path = drive + ":\\" + path;
        }
    }
    
    return path;
}

OperationResult ReDroidController::executeDocker(const QStringList& args, int timeoutMs) {
    OperationResult result;
    
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    qDebug() << "Docker:" << args.join(' ');
    
    process.start(m_config.dockerPath, args);
    
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        result.errorMessage = "Docker command timed out";
        return result;
    }
    
    QString output = process.readAll();
    int exitCode = process.exitCode();
    
    if (exitCode != 0) {
        result.errorMessage = output.isEmpty() ? "Docker command failed" : output;
        return result;
    }
    
    result.success = true;
    result.data["output"] = output;
    
    // Parse container ID from docker run output
    if (args.value(0) == "run") {
        result.data["containerId"] = output.trimmed();
    }
    
    return result;
}

QString ReDroidController::executeDockerSync(const QStringList& args, int timeoutMs) {
    OperationResult result = executeDocker(args, timeoutMs);
    return result.success ? result.data.value("output").toString() : QString();
}

OperationResult ReDroidController::executeAdb(const QString& instanceId, const QStringList& args, int timeoutMs) {
    OperationResult result;
    
    // Find ADB executable
    QString adbExe = m_config.adbPath;
    if (adbExe.isEmpty() || !QFile::exists(adbExe)) {
        adbExe = QCoreApplication::applicationDirPath() + "/adb.exe";
        if (!QFile::exists(adbExe)) {
            adbExe = "adb";  // Try PATH
        }
    }
    
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    qDebug() << "ADB:" << args.join(' ');
    
    process.start(adbExe, args);
    
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        result.errorMessage = "ADB command timed out";
        return result;
    }
    
    QString output = process.readAll();
    int exitCode = process.exitCode();
    
    if (exitCode != 0 && !output.contains("error")) {
        result.errorMessage = output;
        return result;
    }
    
    result.success = true;
    result.data["output"] = output;
    
    return result;
}

QString ReDroidController::executeAdbSync(const QString& instanceId, const QStringList& args, int timeoutMs) {
    OperationResult result = executeAdb(instanceId, args, timeoutMs);
    return result.success ? result.data.value("output").toString() : QString();
}

QString ReDroidController::generatePropertyFile(const DeviceProfile& profile, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream out(&file);
    
    // Device Identity
    out << "RO_PRODUCT_BRAND=" << profile.build.brand << "\n";
    out << "RO_PRODUCT_MANUFACTURER=" << profile.build.manufacturer << "\n";
    out << "RO_PRODUCT_MODEL=" << profile.build.model << "\n";
    out << "RO_PRODUCT_DEVICE=" << profile.build.device << "\n";
    out << "RO_PRODUCT_NAME=" << profile.build.product << "\n";
    out << "RO_PRODUCT_BOARD=" << profile.build.board << "\n";
    
    // Build Properties
    out << "RO_BUILD_VERSION_RELEASE=" << profile.build.androidVersion << "\n";
    out << "RO_BUILD_VERSION_SDK=" << profile.build.sdkVersion << "\n";
    out << "RO_BUILD_ID=" << profile.build.buildId << "\n";
    out << "RO_BUILD_TYPE=" << profile.build.buildType << "\n";
    out << "RO_BUILD_VERSION_SECURITY_PATCH=" << profile.build.securityPatch << "\n";
    out << "RO_BOOTLOADER=" << profile.build.bootloader << "\n";
    
    // Device Identity
    out << "RO_GSM_DEVICE_IMEI=" << profile.identity.imei << "\n";
    out << "RO_GSM_DEVICE_IMEI2=" << profile.identity.imei2 << "\n";
    out << "RO_SERIALNO=" << profile.identity.serialNumber << "\n";
    out << "RO_ANDROID_ID=" << profile.identity.androidId << "\n";
    out << "RO_GSF_ID=" << profile.identity.gsfId << "\n";
    
    // Network
    out << "NET_WIFI_MAC=" << profile.mac.wifiMac << "\n";
    out << "NET_BLUETOOTH_MAC=" << profile.mac.bluetoothMac << "\n";
    out << "NET_ETHERNET_MAC=" << profile.mac.ethernetMac << "\n";
    out << "NET_HOSTNAME=" << profile.network.hostname << "\n";
    
    // SIM
    out << "RO_SIM_ICCID=" << profile.sim.iccid << "\n";
    out << "RO_GSM_SIM_IMSI=" << profile.sim.imsi << "\n";
    out << "RO_SIM_OPERATOR=" << profile.sim.carrier << "\n";
    out << "RO_SIM_OPERATOR_NUMERIC=" << profile.sim.mcc << profile.sim.mnc << "\n";
    
    // GPS
    out << "GPS_LAT=" << profile.gps.latitude << "\n";
    out << "GPS_LON=" << profile.gps.longitude << "\n";
    
    // Hardware
    out << "RO_HARDWARE=" << profile.build.hardware << "\n";
    out << "RO_ARCH=arm64\n";
    
    file.close();
    
    return filePath;
}

// ============================================================================
// Monitoring
// ============================================================================

void ReDroidController::startMonitoring() {
    if (m_monitoring) return;
    
    m_monitoring = true;
    m_monitoringTimer->start(5000);  // Check every 5 seconds
}

void ReDroidController::stopMonitoring() {
    m_monitoring = false;
    m_monitoringTimer->stop();
}

void ReDroidController::checkInstanceStatus(const QString& instanceId) {
    if (!m_instances.contains(instanceId)) {
        return;
    }
    
    InstanceInfo& info = m_instances[instanceId];
    
    // Check container status
    QString stateOutput = executeDockerSync({
        "inspect", "-f", "{{.State.Status}}", info.containerName
    });
    
    InstanceState newState = stateFromString(stateOutput.trimmed());
    
    if (newState != info.state) {
        info.state = newState;
        emit instanceStateChanged(instanceId, newState);
    }
    
    // Check ADB connection
    if (info.state == InstanceState::Running) {
        QString devicesOutput = executeAdbSync(instanceId, {"devices"});
        bool connected = devicesOutput.contains(getAdbSerial(instanceId));
        
        if (connected != info.adbConnected) {
            info.adbConnected = connected;
            emit adbConnectionChanged(instanceId, connected);
        }
        
        // Get container IP
        QString ipOutput = executeDockerSync({
            "inspect", "-f", "{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}",
            info.containerName
        });
        info.ipAddress = ipOutput.trimmed();
    }
}

// ============================================================================
// Network Isolation Implementation
// ============================================================================

bool ReDroidController::createIsolatedNetwork(const QString& instanceId, const QString& subnet) {
    qDebug() << "Creating isolated network for instance:" << instanceId;
    
    QString networkName = QString("vpp-network-%1").arg(instanceId);
    
    // Build docker network create command
    QStringList args = {
        "network", "create",
        "--driver", "bridge",
        "--subnet", subnet,
        "--ipam-driver", "default",
        "--opt", QString("com.docker.network.bridge.name=%1").arg(networkName),
        "--opt", "com.docker.network.bridge.enable_icc=true",
        "--opt", "com.docker.network.bridge.enable_ip_masquerade=true",
        "--dns", "8.8.8.8",
        "--dns", "1.1.1.1",
        networkName
    };
    
    OperationResult result = executeDocker(args, 30000);
    
    if (result.success) {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkName = networkName;
            m_instances[instanceId].networkConfig.networkName = networkName;
            m_instances[instanceId].networkConfig.subnet = subnet;
            m_instances[instanceId].networkConfig.mode = NetworkMode::IsolatedBridge;
        }
        qDebug() << "Isolated network created:" << networkName;
    }
    
    return result.success;
}

bool ReDroidController::deleteIsolatedNetwork(const QString& instanceId) {
    qDebug() << "Deleting isolated network for instance:" << instanceId;
    
    QString networkName;
    
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            networkName = m_instances[instanceId].networkName;
        }
    }
    
    if (networkName.isEmpty()) {
        return true;  // Already deleted
    }
    
    OperationResult result = executeDocker({"network", "rm", networkName}, 10000);
    
    if (result.success) {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkName.clear();
        }
        qDebug() << "Isolated network deleted:" << networkName;
    }
    
    return result.success;
}

bool ReDroidController::assignProxy(const QString& instanceId, const ProxyConfig& proxy) {
    if (!proxy.isValid()) {
        qWarning() << "Invalid proxy configuration";
        return false;
    }
    
    qDebug() << "Assigning proxy to instance:" << instanceId;
    
    // Build proxy URL
    QString proxyUrl;
    if (proxy.type == "socks5") {
        proxyUrl = QString("socks5://%1:%2").arg(proxy.host).arg(proxy.port);
    } else {
        proxyUrl = QString("http://%1:%2").arg(proxy.host).arg(proxy.port);
    }
    
    if (!proxy.username.isEmpty()) {
        proxyUrl = proxyUrl.replace("://", 
            QString("://%1:%2@").arg(proxy.username).arg(proxy.password));
    }
    
    // Generate PAC file content
    QString pacContent = QString(R"(
        function FindProxyForURL(url, host) {
            if (isPlainHostName(host) || 
                shExpMatch(host, "*.local") ||
                isInNet(dnsResolve(host), "10.0.0.0", "255.0.0.0") ||
                isInNet(dnsResolve(host), "172.16.0.0", "255.240.0.0") ||
                isInNet(dnsResolve(host), "192.168.0.0", "255.255.0.0") ||
                isInNet(dnsResolve(host), "127.0.0.0", "255.0.0.0")) {
                return "DIRECT";
            }
            return "PROXY %1:%2";
        }
    )").arg(proxy.host).arg(proxy.port);
    
    // Save PAC file locally
    QString profileDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString pacPath = QString("%1/instances/%2/proxy.pac").arg(profileDir).arg(instanceId);
    
    QDir().mkpath(QString("%1/instances/%2").arg(profileDir).arg(instanceId));
    
    QFile pacFile(pacPath);
    if (pacFile.open(QIODevice::WriteOnly)) {
        pacFile.write(pacContent.toUtf8());
        pacFile.close();
    }
    
    // Push PAC file to container
    pushFile(instanceId, pacPath, "/data/proxy.pac");
    
    // Apply proxy settings via ADB
    QStringList commands = {
        // Set global HTTP proxy
        QString("settings put global http_proxy %1 %2").arg(proxy.host).arg(proxy.port),
        QString("settings put global global_http_proxy_host %1").arg(proxy.host),
        QString("settings put global global_http_proxy_port %1").arg(proxy.port),
        QString("settings put global global_proxy_pac_url file:///data/proxy.pac"),
        
        // Configure DNS to prevent leaks
        "setprop net.dns1 8.8.8.8",
        "setprop net.dns2 1.1.1.1",
        
        // Disable IPv6
        "sysctl -w net.ipv6.conf.all.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.lo.disable_ipv6=1"
    };
    
    for (const QString& cmd : commands) {
        executeShell(instanceId, cmd);
    }
    
    // Store proxy config
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkConfig.proxy = proxy;
            m_instances[instanceId].networkConfig.mode = NetworkMode::Proxy;
        }
    }
    
    qDebug() << "Proxy assigned successfully";
    return true;
}

bool ReDroidController::removeProxy(const QString& instanceId) {
    qDebug() << "Removing proxy from instance:" << instanceId;
    
    QStringList commands = {
        // Remove global proxy
        "settings delete global http_proxy",
        "settings delete global global_http_proxy_host",
        "settings delete global global_http_proxy_port",
        "settings delete global global_proxy_pac_url",
        
        // Reset DNS
        "setprop net.dns1 8.8.8.8",
        "setprop net.dns2 8.8.4.4",
        
        // Re-enable IPv6
        "sysctl -w net.ipv6.conf.all.disable_ipv6=0",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=0"
    };
    
    for (const QString& cmd : commands) {
        executeShell(instanceId, cmd);
    }
    
    // Clear proxy config
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkConfig.mode = NetworkMode::Default;
        }
    }
    
    return true;
}

bool ReDroidController::configureNetworkIsolation(const QString& instanceId,
                                                 const NetworkIsolationConfig& config) {
    qDebug() << "Configuring network isolation for instance:" << instanceId;
    
    // Create isolated network if needed
    if (config.mode == NetworkMode::IsolatedBridge || 
        config.mode == NetworkMode::Proxy ||
        config.mode == NetworkMode::VPN) {
        
        if (!config.subnet.isEmpty()) {
            if (!createIsolatedNetwork(instanceId, config.subnet)) {
                qWarning() << "Failed to create isolated network";
            }
        }
    }
    
    // Assign proxy if configured
    if (config.mode == NetworkMode::Proxy && config.proxy.isValid()) {
        assignProxy(instanceId, config.proxy);
    }
    
    // Apply leak prevention
    applyLeakPrevention(instanceId);
    
    // Block IPv6 if configured
    if (config.blockIPv6) {
        blockIPv6(instanceId);
    }
    
    // Configure DNS
    if (!config.dnsServers.isEmpty()) {
        configureDNS(instanceId, config.dnsServers);
    }
    
    // Store configuration
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkConfig = config;
        }
    }
    
    qDebug() << "Network isolation configured successfully";
    return true;
}

bool ReDroidController::setupVPN(const QString& instanceId, const VPNConfig& vpn) {
    qDebug() << "Setting up VPN for instance:" << instanceId;
    
    // VPN setup requires additional container for WireGuard/OpenVPN
    // This is a placeholder for VPN configuration
    
    qWarning() << "VPN setup requires additional VPN container configuration";
    
    // Store VPN config
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkConfig.vpn = vpn;
            m_instances[instanceId].networkConfig.mode = NetworkMode::VPN;
        }
    }
    
    return false;  // Requires additional VPN container setup
}

bool ReDroidController::applyLeakPrevention(const QString& instanceId) {
    qDebug() << "Applying leak prevention for instance:" << instanceId;
    
    QStringList commands = {
        // Clear ARP cache
        "ip neigh flush all",
        
        // Clear routing cache
        "ip route flush cache",
        
        // Reset hostname to generic
        "hostname android-" + QString::number(QRandomGenerator::global()->bounded(1000, 9999)),
        
        // Clear any IPv6 routes
        "ip -6 route flush cache",
        
        // Disable IPv6 router advertisements
        "sysctl -w net.ipv6.conf.all.accept_ra=0",
        "sysctl -w net.ipv6.conf.default.accept_ra=0"
    };
    
    for (const QString& cmd : commands) {
        executeShell(instanceId, cmd);
    }
    
    // Block STUN to prevent WebRTC leaks
    QStringList stunBlocks = {
        // Block Google STUN
        "iptables -A OUTPUT -p udp --dport 19302 -j DROP",
        // Block standard STUN
        "iptables -A OUTPUT -p udp --dport 3478 -j DROP"
    };
    
    for (const QString& cmd : stunBlocks) {
        executeShell(instanceId, cmd);
    }
    
    qDebug() << "Leak prevention applied";
    return true;
}

bool ReDroidController::blockIPv6(const QString& instanceId) {
    qDebug() << "Blocking IPv6 for instance:" << instanceId;
    
    QStringList commands = {
        "sysctl -w net.ipv6.conf.all.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.lo.disable_ipv6=1"
    };
    
    for (const QString& cmd : commands) {
        executeShell(instanceId, cmd);
    }
    
    return true;
}

bool ReDroidController::configureDNS(const QString& instanceId, const QList<QString>& dnsServers) {
    qDebug() << "Configuring DNS for instance:" << instanceId;
    
    int index = 1;
    for (const QString& dns : dnsServers) {
        QString cmd = QString("setprop net.dns%1 %2").arg(index).arg(dns);
        executeShell(instanceId, cmd);
        
        // Also set persist properties
        QString persistCmd = QString("setprop persist.net.dns%1 %2").arg(index).arg(dns);
        executeShell(instanceId, persistCmd);
        
        index++;
    }
    
    // Flush DNS cache
    executeShell(instanceId, "ndc resolver flushif wlan0");
    executeShell(instanceId, "ndc resolver flushif eth0");
    
    return true;
}

QString ReDroidController::getNetworkInfo(const QString& instanceId) {
    QStringList commands = {
        "ip addr show",
        "ip route show",
        "cat /etc/resolv.conf",
        "getprop net.hostname",
        "getprop net.dns1",
        "getprop net.dns2"
    };
    
    QStringList outputs;
    for (const QString& cmd : commands) {
        outputs.append(QString("\n=== %1 ===\n%2").arg(cmd).arg(executeShell(instanceId, cmd)));
    }
    
    return outputs.join("\n");
}

bool ReDroidController::testForLeaks(const QString& instanceId) {
    qDebug() << "Testing for network leaks in instance:" << instanceId;
    
    bool leakDetected = false;
    
    // Check for IPv6 leak
    QString ipv6Result = executeShell(instanceId, "curl -s -6 https://ifconfig.me 2>/dev/null");
    if (!ipv6Result.trimmed().isEmpty()) {
        qWarning() << "IPv6 leak detected! IP:" << ipv6Result;
        leakDetected = true;
    }
    
    // Check for DNS leak using external service
    QString dnsLeakResult = executeShell(instanceId, 
        "curl -s https://browserleaks.com/dns 2>/dev/null || echo 'check_failed'");
    if (dnsLeakResult.contains("check_failed")) {
        qWarning() << "DNS leak test could not be completed";
    }
    
    return leakDetected;
}

// ============================================================================
// Android Studio Emulator Support (Windows)
// ============================================================================

bool ReDroidController::isAdbAvailable() {
    QString adbPath = m_config.adbPath;
    
    // Try to find adb
    if (adbPath.isEmpty() || adbPath == "adb") {
        adbPath = findAdbPath();
    }
    
    if (adbPath.isEmpty()) {
        return false;
    }
    
    QProcess process;
    process.start(adbPath, {"version"});
    bool available = process.waitForFinished(5000);
    
    if (available && process.exitCode() == 0) {
        qDebug() << "ADB found at:" << adbPath;
        return true;
    }
    
    return false;
}

QString ReDroidController::findAdbPath() {
    QStringList possiblePaths;
    
#ifdef Q_OS_WIN32
    // Common Android SDK locations on Windows
    QStringList sdkPaths = {
        qEnvironmentVariable("LOCALAPPDATA") + "/Android/Sdk/platform-tools/adb.exe",
        qEnvironmentVariable("USERPROFILE") + "/Android/Sdk/platform-tools/adb.exe",
        "C:/Users/" + qEnvironmentVariable("USERNAME") + "/AppData/Local/Android/Sdk/platform-tools/adb.exe",
        "C:/Android/Sdk/platform-tools/adb.exe",
        "C:/Program Files/Android/Sdk/platform-tools/adb.exe",
    };
    
    for (const QString& path : sdkPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    // Try to find in PATH
    QProcess process;
    process.start("where", {"adb.exe"});
    if (process.waitForFinished(3000)) {
        QString output = process.readAll().trimmed();
        if (!output.isEmpty()) {
            return output.split('\n').first();
        }
    }
#else
    // Linux/Mac
    QStringList paths = {"/usr/bin/adb", "/usr/local/bin/adb", "/opt/android-sdk/platform-tools/adb"};
    for (const QString& path : paths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
#endif
    
    return "adb"; // Fallback to PATH
}

QStringList ReDroidController::detectAndroidStudioEmulators() {
    QStringList emulators;
    
    // Ensure ADB path is set
    if (m_config.adbPath.isEmpty()) {
        m_config.adbPath = findAdbPath();
    }
    
    // First, try adb devices
    QProcess process;
    process.start(m_config.adbPath, {"devices"});
    if (!process.waitForFinished(5000)) {
        qWarning() << "ADB devices command timed out";
        return emulators;
    }
    
    QString output = process.readAll();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("List")) continue;
        
        // Format: emulator-5554    device
        QStringList parts = trimmed.split(QRegExp("\\s+"));
        if (parts.size() >= 2 && parts[1] == "device") {
            QString serial = parts[0];
            if (serial.startsWith("emulator-")) {
                emulators.append(serial);
                qDebug() << "Found Android Studio Emulator:" << serial;
            }
        }
    }
    
    return emulators;
}

QString ReDroidController::connectToAndroidStudioEmulator() {
    // First try to find already running emulators
    QStringList runningEmulators = detectAndroidStudioEmulators();
    
    if (!runningEmulators.isEmpty()) {
        QString emulator = runningEmulators.first();
        qDebug() << "Found running emulator:" << emulator;
        
        InstanceInfo info;
        info.instanceId = emulator;
        info.serial = emulator;
        info.status = InstanceStatus::RUNNING;
        info.adbPort = emulator.replace("emulator-", "").toInt();
        
        QMutexLocker locker(&m_instancesMutex);
        m_instances[emulator] = info;
        m_adbSerials[emulator] = emulator;
        
        return emulator;
    }
    
    // If no emulator is running, try to connect to common ports
    QStringList ports = {"5554", "5556", "5558", "5560"};
    for (const QString& port : ports) {
        QString serial = QString("emulator-%1").arg(port);
        QString result = executeCommand(QString("connect localhost:%1").arg(port));
        
        if (!result.contains("failed") && !result.contains("error")) {
            qDebug() << "Connected to emulator at port" << port;
            
            InstanceInfo info;
            info.instanceId = serial;
            info.serial = serial;
            info.status = InstanceStatus::RUNNING;
            info.adbPort = port.toInt();
            
            QMutexLocker locker(&m_instancesMutex);
            m_instances[serial] = info;
            m_adbSerials[serial] = serial;
            
            return serial;
        }
    }
    
    qWarning() << "No Android Studio Emulator found or could not connect";
    return QString();
}

void ReDroidController::setAndroidStudioAdbPath(const QString& path) {
    m_config.adbPath = path;
    saveConfiguration();
    qDebug() << "Android Studio ADB path set to:" << path;
}

} // namespace VirtualPhonePro
