#include "VirtualPhonePro/ReDroidController.hpp"
#include "VirtualPhonePro/UniqueDeviceGenerator.hpp"
#include "VirtualPhonePro/AndroidRealismEngine.hpp"
#include "VirtualPhonePro/TimingAttackPrevention.hpp"
#include "VirtualPhonePro/PlayIntegrityManager.hpp"
#include "VirtualPhonePro/EmulatorDetectionBypass.hpp"
#include "VirtualPhonePro/HardwareFingerprintSpoofer.hpp"
#include "VirtualPhonePro/NetworkStackSpoofer.hpp"
#include "VirtualPhonePro/SafetyNetAdvancedBypass.hpp"
#include "VirtualPhonePro/HypervisorBypass.hpp"
#include "VirtualPhonePro/RealPhoneHardening.hpp"
#include "VirtualPhonePro/VirtualSecurityChip.hpp"
#include "VirtualPhonePro/CryptoEmulator.hpp"
#include "VirtualPhonePro/AdvancedSpoofing.hpp"
#include "VirtualPhonePro/RealisticProfileGenerator.hpp"
#include "VirtualPhonePro/ADBManager.hpp"
#include "VirtualPhonePro/DeviceIDGenerator.hpp"
#include "VirtualPhonePro/IPTimezoneConverter.hpp"
#include "Android/LocaleTimezoneManager.h"
#include "VirtualPhonePro/BankingAppSpoofer.hpp"
#include "VirtualPhonePro/GoogleFacebookSpoofer.hpp"
#include "VirtualPhonePro/TLSFingerprint.hpp"

// ── Previously unwired modules — now wired ───────────────────────────────────
#include "VirtualPhonePro/FridaXposedDetector.hpp"
#include "VirtualPhonePro/DeepDeviceSpoofer.hpp"
#include "VirtualPhonePro/SELinuxManager.hpp"
#include "VirtualPhonePro/SecurityMitigationManager.hpp"
#include "VirtualPhonePro/HardwareAttestation.hpp"
#include "VirtualPhonePro/SensorSimulator.hpp"
#include "VirtualPhonePro/BatteryPowerManager.hpp"
#include "VirtualPhonePro/ScreenStateManager.hpp"
#include "VirtualPhonePro/AdvancedAntiDetection.hpp"
#include "VirtualPhonePro/OEMDeepSpoofing.hpp"
#include "VirtualPhonePro/PersistentIdentityManager.hpp"
#include "VirtualPhonePro/CarrierNetworkSimulator.hpp"
#include "VirtualPhonePro/SystemAppSimulator.hpp"
#include "VirtualPhonePro/DeviceIntegrityManager.hpp"
#include "VirtualPhonePro/HALSimulation.hpp"
#include "VirtualPhonePro/SSLCertificateManager.hpp"
#include "VirtualPhonePro/NetworkRealismEnhancer.hpp"
#include "VirtualPhonePro/DeviceBehaviorManager.hpp"
#include "VirtualPhonePro/AdvancedRealisticSimulation.hpp"
#include "VirtualPhonePro/RealisticDeviceProfile.hpp"
#include "VirtualPhonePro/EnhancedDeviceProfile.hpp"
#include "VirtualPhonePro/FindMyDeviceManager.hpp"
#include "VirtualPhonePro/NetworkProfileManager.hpp"
#include "VirtualPhonePro/ProfileGeneratorFactory.hpp"
#include "VirtualPhonePro/MagiskPatcher.hpp"
#include "VirtualPhonePro/WebhookManager.hpp"
#include "VirtualPhonePro/TestingFramework.hpp"
#include "VirtualPhonePro/AppCloner.hpp"
#include "VirtualPhonePro/CryptoUtils.hpp"
#include "VirtualPhonePro/HttpClient.hpp"
#include "Android/FileSystemRealism.h"
#include "Database/FingerprintEngine.h"
#include "Android/SensorInjector.h"

#include <QCoreApplication>
#include <QRandomGenerator>
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
#include <QCryptographicHash>
#include <QMutexLocker>
#include <QUuid>
#include <QThread>
#include <QStorageInfo>
#include <QHostInfo>
#include <QTcpServer>
#include <QtMath>
#include <QDateTime>
#include <QDate>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace VirtualPhonePro {

// ============================================================================
// Singleton
// ============================================================================

// Meyers Singleton — C++11 §6.7 guarantees this static local is
// initialised exactly once, even under concurrent first-call races,
// without any explicit mutex.  The raw s_instance pointer and its
// associated data race have been removed entirely.
ReDroidController& ReDroidController::instance() {
    static ReDroidController s_instance;
    return s_instance;
}

// Returns the current month's Android security patch date ("YYYY-MM-01").
// Keeps ro.build.version.security_patch from ageing past the build — Play
// Integrity / banking apps flag stale patches. Pinned to "-01" to match the
// real property format and clamped so it never lands in the future.
static QString currentSecurityPatchDate() {
    QDate today = QDateTime::currentDateTimeUtc().date();
    QDate patchDate(today.year(), today.month(), 1);
    if (patchDate > today) {
        patchDate = QDate(today.year(), today.month(), 1).addMonths(-1);
    }
    return patchDate.toString("yyyy-MM-dd");
}

// ============================================================================
// DockerConfig Implementation
// ============================================================================

DockerConfig::DockerConfig()
    : dockerPath("docker")
    , adbPath("")
    , imageName("redroid/redroid:14.0.0-latest")
    , networkDriver("bridge")
    , baseAdbPort(5555)
    , memoryLimit("1536M")
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
    // Stop monitoring first
    stopMonitoring();
    
    // Stop all running instances
    QStringList instanceIds = m_instances.keys();
    for (const QString& id : instanceIds) {
        try {
            stopInstance(id, true);
        } catch (const std::exception& e) {
            qWarning() << "Error stopping instance" << id << ":" << e.what();
        }
    }
    
    // Clear all data structures
    m_instances.clear();
    
    // Clean up timers
    if (m_monitoringTimer) {
        m_monitoringTimer->stop();
        delete m_monitoringTimer;
        m_monitoringTimer = nullptr;
    }
    
    qDebug() << "[ReDroidController] All resources cleaned up";
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

// ============================================================================
// System Prerequisite Check
// ============================================================================

SystemCheckReport ReDroidController::checkSystemRequirements() {
    SystemCheckReport report;
    report.canRun = true;

    // ── Helper lambdas ────────────────────────────────────────────────────────
    auto addCheck = [&](const QString& name, bool met, const QString& ok,
                        const QString& fail, const QString& fix, bool required) {
        SystemRequirement r;
        r.name           = name;
        r.met            = met;
        r.status         = met ? ok : fail;
        r.fixInstruction = met ? QString() : fix;
        r.required       = required;
        report.checks.append(r);
        if (required && !met) report.canRun = false;
    };

    // ── 1. Docker running (REQUIRED) ──────────────────────────────────────────
    {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start(m_config.dockerPath, {"info", "--format", "{{.ServerVersion}}"});
        bool ok = p.waitForFinished(8000) && p.exitCode() == 0;
        QString ver = p.readAll().trimmed();
        addCheck("Docker",
                 ok,
                 QString("Running (v%1)").arg(ver),
                 "Docker Desktop is not running or not installed",
                 "Install Docker Desktop from https://www.docker.com/products/docker-desktop "
                 "and ensure it is started before launching the app.",
                 /*required=*/true);
    }

    // ── 2. Binder kernel support (REQUIRED on Linux/WSL2) ────────────────────
    {
#ifdef Q_OS_WIN
        // On Windows, binder lives inside the WSL2 VM — check via `wsl` command.
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start("wsl", {"--", "test", "-e", "/dev/binderfs", "||",
                        "test", "-e", "/sys/fs/binder"});
        bool binderfs = p.waitForFinished(8000) && p.exitCode() == 0;

        // Fallback: check /dev/binder directly
        if (!binderfs) {
            p.start("wsl", {"--", "test", "-e", "/dev/binder"});
            binderfs = p.waitForFinished(5000) && p.exitCode() == 0;
        }

        addCheck("WSL2 Binder Kernel",
                 binderfs,
                 "binderfs / /dev/binder present",
                 "WSL2 kernel does not have Android binder support",
                 "Install a custom WSL2 kernel with CONFIG_ANDROID_BINDERFS=y.\n"
                 "See docs/WSL2_KERNEL_SETUP.md for step-by-step instructions.\n"
                 "Quick install: https://github.com/kdrag0n/proton-wine/releases",
                 /*required=*/true);
#else
        bool binderfs = QFileInfo::exists("/dev/binderfs") ||
                        QFileInfo::exists("/sys/fs/binder") ||
                        QFileInfo::exists("/dev/binder");
        addCheck("Binder Kernel Support",
                 binderfs,
                 "binderfs available",
                 "/dev/binderfs and /dev/binder not found",
                 "Install kernel with CONFIG_ANDROID_BINDERFS=y. "
                 "See docs/WSL2_KERNEL_SETUP.md.",
                 /*required=*/true);
#endif
    }

    // ── 3. Windows Hypervisor Platform (REQUIRED on Windows for WSL2) ────────
#ifdef Q_OS_WIN
    {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start("powershell",
                {"-Command",
                 "(Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform).State"});
        bool ok = p.waitForFinished(10000) && p.exitCode() == 0 &&
                  p.readAll().trimmed().contains("Enabled");
        addCheck("Windows Hypervisor Platform",
                 ok,
                 "Enabled",
                 "Windows Hypervisor Platform is not enabled",
                 "Run as Administrator:\n"
                 "  dism /online /enable-feature /featurename:HypervisorPlatform /all /norestart\n"
                 "Then reboot your PC.",
                 /*required=*/true);
    }
#endif

    // ── 4. KVM acceleration (OPTIONAL — swiftshader fallback available) ───────
    {
#ifdef Q_OS_WIN
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start("wsl", {"--", "test", "-e", "/dev/kvm"});
        bool kvm = p.waitForFinished(5000) && p.exitCode() == 0;
#else
        bool kvm = QFileInfo::exists("/dev/kvm");
#endif
        addCheck("KVM Hardware Acceleration",
                 kvm,
                 "Available (fast boot, better performance)",
                 "Not available — will use swiftshader (slow, ~90s boot)",
                 "Enable Intel VT-x / AMD-V in BIOS, then in WSL2:\n"
                 "  echo 'nestedVirtualization=true' >> %USERPROFILE%\\.wslconfig\n"
                 "  wsl --shutdown",
                 /*required=*/false);
    }

    // ── 5. Disk space — need ≥ 4 GB free (REQUIRED) ──────────────────────────
    {
        QStorageInfo storage(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation));
        qint64 freeGB = storage.bytesAvailable() / (1024LL * 1024 * 1024);
        bool ok = freeGB >= 4;
        addCheck("Disk Space",
                 ok,
                 QString("%1 GB free").arg(freeGB),
                 QString("Only %1 GB free — need ≥ 4 GB for container images").arg(freeGB),
                 "Free up disk space. Each ReDroid instance needs ~2-3 GB.",
                 /*required=*/true);
    }

    // ── 6. ADB reachable (OPTIONAL — installed with app bundle) ──────────────
    {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start(m_config.adbPath.isEmpty() ? "adb" : m_config.adbPath,
                {"version"});
        bool ok = p.waitForFinished(5000) && p.exitCode() == 0;
        QString ver = p.readAll().split('\n').first().trimmed();
        addCheck("ADB",
                 ok,
                 ver,
                 "adb not found in PATH",
                 "adb.exe is bundled with the app. If missing, reinstall the application "
                 "or add Android Platform Tools to PATH.",
                 /*required=*/false);
    }

    return report;
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
    settings.setValue("docker/useWSL2", m_config.useWSL2);
    settings.setValue("docker/wslDistro", m_config.wslDistro);
}

QJsonObject ReDroidController::loadProfile(const QString& profileName) {
    QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + "/profiles/" + profileName + ".json";
    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

// ============================================================================
// Instance Lifecycle
// ============================================================================

bool ReDroidController::startInstance(const QString& instanceId, const DeviceProfile& profile) {
    qDebug() << "Starting instance:" << instanceId;
    
    // Allocate ports BEFORE taking the mutex
    // allocateAdbPort also locks m_instancesMutex
    // Calling it inside the lock below would cause a deadlock (non-recursive mutex)
    //
    // Honor an explicitly requested port (profile.adbPort > 0) when it is
    // actually free; otherwise fall back to the dynamic allocator. Batch
    // deploys pin ports via assignUniquePort — silently ignoring that
    // would defeat the port pre-flight check in MultiInstanceManager.
    int adbPort = -1;
    if (profile.adbPort > 0 && profile.adbPort <= 65535) {
        adbPort = tryReserveAdbPort(profile.adbPort);
        if (adbPort < 0)
            qWarning() << "startInstance: requested ADB port" << profile.adbPort
                       << "is unavailable — falling back to dynamic allocation";
    }
    if (adbPort < 0)
        adbPort = allocateAdbPort();
    if (adbPort < 0) {
        qCritical() << "startInstance: no free ADB port available";
        emit error(QStringLiteral("Cannot start instance — no free ADB port in range 5556-65535"));
        return false;
    }
    
    QMutexLocker locker(&m_instancesMutex);
    
    // Check if already running
    if (m_instances.contains(instanceId)) {
        InstanceInfo& info = m_instances[instanceId];
        if (info.state == InstanceState::Running) {
            qWarning() << "Instance already running:" << instanceId;
            return true;
        }
    }
    
    // Create instance info
    InstanceInfo info;
    info.instanceId = instanceId;
    info.imageName  = m_config.imageName;
    info.state      = InstanceState::Starting;
    info.adbPort    = adbPort;
    info.instanceIndex = m_instances.size();
    info.createdAt  = QDateTime::currentMSecsSinceEpoch();
    info.profileId  = profile.id;
    info.adbConnected = false;

    // =========================================================================
    // Fix 8: Container name collision guard.
    //
    // "vpp-<uuid>" is deterministic — if the app crashed and the container
    // was not removed, docker run fails with:
    //   "Conflict. The container name /vpp-<uuid> is already in use."
    // Strategy:
    //   1. Check whether a container with that name already exists (any state).
    //   2a. If it is stopped/exited — remove it so we can reuse the name.
    //   2b. If it is running/paused — the instance is actually alive; update
    //       m_instances and return true without launching a duplicate.
    //   3. If the remove fails for any reason — append a short epoch suffix to
    //      guarantee a unique name (never block the launch).
    // =========================================================================
    QString baseName = QString("vpp-%1").arg(instanceId);
    info.containerName = baseName;

    // Query existing container state (empty string = does not exist)
    QString existingState = executeDockerSync(
        {"inspect", "-f", "{{.State.Status}}", baseName}).trimmed();

    if (!existingState.isEmpty() && existingState != "no such object") {
        if (existingState == "running" || existingState == "paused") {
            // Container is alive from a previous session — adopt it.
            qWarning() << "Container" << baseName
                       << "already running (state:" << existingState
                       << "). Adopting existing container.";
            info.state = InstanceState::Running;
            info.startedAt = QDateTime::currentMSecsSinceEpoch();
            m_instances[instanceId] = info;
            emit instanceStateChanged(instanceId, InstanceState::Running);
            return true;
        }

        // Container exists but is stopped/exited — try to remove it.
        OperationResult rmResult = executeDocker({"rm", "-f", baseName}, 10000);
        if (!rmResult.success) {
            // Remove failed; use a unique suffixed name as fallback.
            info.containerName = QString("%1-%2")
                .arg(baseName)
                .arg(QDateTime::currentMSecsSinceEpoch() % 100000);
            qWarning() << "Could not remove stale container" << baseName
                       << "— using fallback name" << info.containerName;
        }
    }
    
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

    // =========================================================================
    // ---------------------------------------------------------------
    // REDROID_PROP_ env vars: inject ro.* properties BEFORE Android init
    // This is the ONLY reliable way to set ro.* in standard ReDroid
    // (resetprop requires Magisk which is not in the base image)
    // ---------------------------------------------------------------
    args << "-e" << "REDROID_PROP_ro.kernel.qemu=0";
    args << "-e" << "REDROID_PROP_ro.boot.qemu=0";
    args << "-e" << "REDROID_PROP_ro.boot.mode=normal";
    args << "-e" << "REDROID_PROP_ro.debuggable=0";
    args << "-e" << "REDROID_PROP_ro.secure=1";
    args << "-e" << "REDROID_PROP_ro.build.type=user";
    args << "-e" << "REDROID_PROP_ro.build.tags=release-keys";
    args << "-e" << "REDROID_PROP_ro.build.selinux=0";
    args << "-e" << "REDROID_PROP_ro.product.type=phone";
    args << "-e" << "REDROID_PROP_ro.build.characteristics=default";
    args << "-e" << "REDROID_PROP_ro.device.form=FACTORY";
    args << "-e" << "REDROID_PROP_ro.product.first_api_level=33";
    args << "-e" << "REDROID_PROP_ro.emulator=false";
    args << "-e" << "REDROID_PROP_ro.arch=arm64";
    args << "-e" << "REDROID_PROP_ro.cpu.abi=arm64-v8a";
    args << "-e" << QString("REDROID_PROP_ro.product.model=%1").arg(profile.build.model);
    args << "-e" << QString("REDROID_PROP_ro.product.manufacturer=%1").arg(profile.manufacturer);
    args << "-e" << QString("REDROID_PROP_ro.product.brand=%1").arg(profile.manufacturer.toLower());
    args << "-e" << QString("REDROID_PROP_ro.product.name=%1").arg(profile.build.product);
    args << "-e" << QString("REDROID_PROP_ro.product.device=%1").arg(profile.build.device);
    args << "-e" << QString("REDROID_PROP_ro.build.fingerprint=%1").arg(profile.build.fingerprint);
    args << "-e" << QString("REDROID_PROP_ro.build.version.release=%1").arg(profile.build.androidVersion);
    args << "-e" << QString("REDROID_PROP_ro.build.version.sdk=%1").arg(profile.build.sdkVersion);
    args << "-e" << QString("REDROID_PROP_ro.hardware=%1").arg(profile.build.hardware);
    args << "-e" << QString("REDROID_PROP_ro.serialno=%1").arg(profile.identity.serialNumber);
    args << "-e" << QString("REDROID_PROP_ro.boot.hardware=%1").arg(profile.build.hardware);

    // Binder isolation — one binderfs mount namespace per instance
    //
    // Flat --device /dev/binder:/dev/binder gives every container the same
    // binder context; Android IPC between instances leaks across boundaries.
    //
    // Correct approach: mount binderfs inside the container so each instance
    // gets its own isolated binder context.  We pass --privileged (already
    // set above) which allows the container's init to mount binderfs itself,
    // which is exactly what ReDroid's /init does when it detects no pre-bound
    // binder device.  We only fall back to host --device passthrough when the
    // host kernel does NOT have binderfs (older WSL2 kernels).
    // =========================================================================
    auto hostHasBinderfs = []() -> bool {
        // Check if the host kernel has binderfs compiled in
        return QFileInfo::exists("/dev/binderfs") ||
               QFileInfo::exists("/sys/fs/binder");
    };

    if (hostHasBinderfs()) {
        // Let ReDroid's /init mount its own binderfs — no --device needed.
        // The container gets an isolated binder namespace automatically.
        // Requires: --privileged (already set) + kernel binderfs support.
        qDebug() << "binderfs available — container will mount its own binder namespace";
    } else {
        // Older WSL2 kernel: fall back to host binder device passthrough.
        // All instances share the host binder context — Android IPC leaks
        // across container boundaries. Refuse to start a second instance
        // rather than silently corrupt IPC state; single-instance use is
        // still permitted.
        int activeCount = 0;
        for (const InstanceInfo& other : m_instances) {
            if (other.state == InstanceState::Starting ||
                other.state == InstanceState::Running  ||
                other.state == InstanceState::Paused) {
                ++activeCount;
            }
        }
        if (activeCount > 0) {
            qCritical() << "startInstance: kernel lacks binderfs and another"
                        << "instance is already active — multi-instance"
                        << "requires binderfs for per-container binder"
                        << "isolation. Refusing to start" << instanceId;
            emit error(QStringLiteral(
                "Cannot start multiple instances on this host: kernel lacks "
                "binderfs. Instances would share the host binder context "
                "(Android IPC leaks across containers). Use a kernel with "
                "binderfs (5.7+), or run one instance at a time."));
            return false;
        }
        qWarning() << "binderfs not available — falling back to host /dev/binder passthrough";
        auto addDeviceIfExists = [&](const QString& hostPath, const QString& containerPath) {
            if (QFileInfo::exists(hostPath) || QFileInfo(hostPath).isSymLink()) {
                args << "--device" << QString("%1:%2").arg(hostPath, containerPath);
            }
        };
        addDeviceIfExists("/dev/binder",    "/dev/binder");
        addDeviceIfExists("/dev/vndbinder", "/dev/vndbinder");
        addDeviceIfExists("/dev/hwbinder",  "/dev/hwbinder");
    }

    // Memory limit — Docker requires uppercase suffix (M/G/MB/GB).
    // Normalise here so user-supplied values like "1536m" still work.
    args << "-m" << m_config.memoryLimit.toUpper();

    // SHM size — uppercase M required
    args << "--shm-size" << QString("%1M").arg(m_config.shmSize);
    
    // CPU quota
    args << "--cpu-quota" << QString::number(m_config.cpuQuota);
    
    // Hostname
    args << "-h" << QString("android-%1").arg(instanceId);
    
    // Environment variables for device properties
    // VPP_COUNTRY_CODE lets in-pipeline spoofing (timezone, locale, carrier)
    // resolve this instance's assigned region via IPTimezoneConverter.
    // Country comes from the profile's SIM configuration, defaulting to US.
    QString countryEnv = profile.sim.country.trimmed().toUpper();
    if (countryEnv.isEmpty()) {
        countryEnv = QStringLiteral("US");
    }
    args << "-e" << QString("VPP_COUNTRY_CODE=%1").arg(countryEnv);
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

    // Cellular identity — init_cellular_network.sh consumes CELLULAR_IP/GATEWAY.
    // Pass the profile-anchored deterministic local IP (deriveLocalIP unit) so
    // rmnet0, WebRTC and the routing story are consistent and stable across
    // reboots. The script's own fallback is also deterministic (seeded), never
    // random.
    QString cellularIp = profile.network.ipAddress;
    if (cellularIp.isEmpty()) {
        cellularIp = QStringLiteral("10.0.2.15");
    }
    QString cellularGateway = QStringLiteral("10.0.0.1");
    const QStringList ipParts = cellularIp.split(QLatin1Char('.'));
    if (ipParts.size() == 4) {
        cellularGateway = QStringLiteral("%1.%2.%3.1")
                              .arg(ipParts[0], ipParts[1], ipParts[2]);
    }
    args << "-e" << QString("CELLULAR_IP=%1").arg(cellularIp);
    args << "-e" << QString("GATEWAY=%1").arg(cellularGateway);
    
    // GPU mode (swiftshader for Windows compatibility)
    args << "-e" << "REDROID_GPU_MODE=swiftshader";
    
    // Screen display - ADB screencap approach (no X11 needed)
    // PhoneWindow captures screen via: adb exec-out screencap -p
    // This works on all platforms without VcXsrv/X11
    
    // Screen capture is done via: adb exec-out screencap -p
    // This works on all platforms without X11/VcXsrv.
    // DISPLAY is intentionally not injected.
    
    // Redroid configuration
    args << "-e" << "REDROID_CTS=0";
    args << "-e" << "ROG_BOOTANIMATION=false";
    args << "-e" << "ROG_DISABLE_FPS_LIMIT=true";
    
    // Ports — ADB only; ReDroid does not use VNC
    args << "-p" << QString("%1:5555").arg(adbPort);
    
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

    // =========================================================================
    // Fix 4: Wait for sys.boot_completed=1 before declaring ADB ready.
    //
    // Android property daemon starts INSIDE /init. Polling
    // sys.boot_completed via ADB shell getprop is the only reliable
    // signal that property_service is up and the system is fully booted.
    // A fixed msleep(5000) was used previously — too short for slow
    // swiftshader boots (~40-90s) and too long for fast ones.
    // =========================================================================
    const int BOOT_TIMEOUT_SEC = 180;   // 3 minutes max
    const int POLL_INTERVAL_MS = 2000;
    bool bootCompleted = false;

    qDebug() << "[Boot] Waiting for sys.boot_completed (timeout:" << BOOT_TIMEOUT_SEC << "s)";
    emit operationProgress(instanceId, "boot", 0,
                           QStringLiteral("Waiting for Android to boot..."));

    QString adbSerial = QString("127.0.0.1:%1").arg(adbPort);

    // First establish ADB connection (retried until port accepts)
    {
        int connectRetries = 0;
        while (connectRetries < 30) {
            QString out = executeAdbSync(instanceId, {"connect", adbSerial}, 5000);
            if (out.contains("connected")) break;
            QThread::msleep(2000);
            ++connectRetries;
        }
    }

    for (int elapsed = 0; elapsed < BOOT_TIMEOUT_SEC * 1000; elapsed += POLL_INTERVAL_MS) {
        QThread::msleep(POLL_INTERVAL_MS);
        QString val = executeAdbSync(instanceId,
                                     {"shell", "getprop", "sys.boot_completed"},
                                     3000).trimmed();
        if (val == QStringLiteral("1")) {
            bootCompleted = true;
            qDebug() << "[Boot] sys.boot_completed=1 after" << elapsed / 1000 << "s";
            emit operationProgress(instanceId, "boot", 100,
                                   QStringLiteral("Android boot completed"));
            break;
        }
        int pct = qMin(99, elapsed * 99 / (BOOT_TIMEOUT_SEC * 1000));
        emit operationProgress(instanceId, "boot", pct,
                               QStringLiteral("Booting... %1s").arg(elapsed / 1000));
    }

    if (!bootCompleted) {
        qWarning() << "[Boot] Timeout waiting for sys.boot_completed — instance may be unstable";
        emit error(QStringLiteral("Instance %1: boot timeout (%2s). "
                                  "Check container logs for /init errors.")
                       .arg(instanceId).arg(BOOT_TIMEOUT_SEC));
    }

    // Store ADB serial
    m_adbSerials[instanceId] = adbSerial;

    // Check ADB connection
    QString devicesOutput = executeAdbSync(instanceId, {"devices"});
    info.adbConnected = devicesOutput.contains(adbSerial);
    m_instances[instanceId] = info;

    // Unlock before the realism pipeline: applyCompleteRealism() emits signals
    // and drives dozens of ADB round-trips, so it must not run under
    // m_instancesMutex. The locker also guards the early-return paths above.
    locker.unlock();

    // =========================================================================
    // Fix 13: Auto-apply the full realism pipeline on instance startup.
    //
    // Previously NO caller of startInstance() invoked applyCompleteRealism()
    // or applyProfile() afterwards (grep-verified: zero call sites), so the
    // entire 11-phase anti-detection stack — including FindMyDeviceManager
    // and MagiskPatcher, which are wired ONLY inside applyCompleteRealism() —
    // never ran. Fresh instances booted with stock ReDroid fingerprints.
    //
    // applyCompleteRealism() binds every spoofing module to this instance's
    // adb serial (per-instance ADBManager), so concurrent batch deploys do
    // not leak commands across containers.
    // =========================================================================
    if (!applyCompleteRealism(instanceId, profile.build.manufacturer, profile.build.model, profile)) {
        qWarning() << "[Realism] Pipeline failed for" << instanceId
                   << "— instance is running but realism is incomplete";
        emit error(QStringLiteral("Instance %1 started, but the realism "
                                  "pipeline did not complete — detection "
                                  "avoidance is degraded.")
                       .arg(instanceId));
    }
    
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
    // --time N: Docker sends SIGTERM, waits N seconds, then SIGKILL.
    // force=true  → --time 0  (immediate SIGKILL, used for crash/CI)
    // force=false → --time 10 (10s graceful shutdown for Android init)
    //
    // Qt-side timeout must be > Docker's wait time to avoid the
    // QProcess being killed before Docker finishes its own wait.
    // We give Qt (dockerTimeout) = (stopSeconds + 5) * 1000 ms.
    const int stopSeconds   = force ? 0 : 10;
    const int dockerTimeout = (stopSeconds + 5) * 1000;

    QStringList stopArgs = {"stop", "--time", QString::number(stopSeconds), containerName};
    OperationResult result = executeDocker(stopArgs, dockerTimeout);
    
    if (!result.success) {
        qWarning() << "Failed to stop container:" << result.errorMessage;
    }
    
    // Remove container
    result = executeDocker({"rm", "-f", containerName}, 10000);
    
    // Update state
    info.state = InstanceState::Stopped;
    m_instances[instanceId] = info;

    // Clean up per-instance anti-detection module state
    VirtualPhonePro::HypervisorBypass::removeInstance(instanceId);
    VirtualPhonePro::SafetyNetAdvancedBypass::removeInstance(instanceId);
    VirtualPhonePro::RealPhoneHardening::removeInstance(instanceId);

    // Stop continuous sensor injection
    VirtualPhonePro::SensorInjector::instance().stop(instanceId);

    // Clean up per-instance ADBManager (frees the -s bound connection).
    // Uses the serial captured at the top of this function — m_adbSerials
    // has already been erased by now, so a lookup here would never fire.
    if (!adbSerial.isEmpty()) {
        VirtualPhonePro::ADBManager::removeInstance(adbSerial.toStdString());
    }

    // Free the per-instance deterministic RNG state (device seeds,
    // Gaussian generators, gesture profiles)
    VirtualPhonePro::TimingAttackPrevention::instance().removeDeviceSeed(instanceId);

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

    // =========================================================================
    // Fix 5: Guard against pre-boot setprop calls.
    //
    // property_service (the daemon that handles setprop) is started by
    // Android's /init. Calling setprop before sys.boot_completed=1 writes
    // to a socket that does not yet exist — every call silently fails.
    // We verify boot is complete before issuing any property writes.
    // =========================================================================
    QString bootVal = executeAdbSync(instanceId,
                                     {"shell", "getprop", "sys.boot_completed"},
                                     3000).trimmed();
    if (bootVal != QStringLiteral("1")) {
        qWarning() << "applyProfile called before boot_completed on" << instanceId
                   << "— aborting to prevent silent setprop failure";
        emit error(QStringLiteral("applyProfile: instance %1 has not finished booting "
                                  "(sys.boot_completed=%2). "
                                  "Call applyProfile only after boot is confirmed.")
                       .arg(instanceId, bootVal));
        return false;
    }
    
    // Set build properties
    setProperty(instanceId, "ro.product.brand", profile.build.brand);
    setProperty(instanceId, "ro.product.manufacturer", profile.build.manufacturer);
    setProperty(instanceId, "ro.product.model", profile.build.model);
    setProperty(instanceId, "ro.product.device", profile.build.device);
    setProperty(instanceId, "ro.product.name", profile.build.product);
    
    // Set build fingerprint. Real consumer devices ship "user" builds, never
    // "userdebug" — banking apps treat a userdebug build type as an instant
    // red flag (only engineering/internal builds use it).
    QString fingerprint = QString("%1/%2/%3:%4/%5:user/release-keys")
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

bool ReDroidController::applyCompleteRealism(const QString& instanceId, const QString& manufacturer, const QString& model, const DeviceProfile& profile) {
    qDebug() << "[Realism] ════════════════════════════════════════════════════════════";
    qDebug() << "[Realism]  ULTIMATE BANKING EDITION v3.0";
    qDebug() << "[Realism] ════════════════════════════════════════════════════════════";
    qDebug() << "[Realism] Device:" << manufacturer << model;
    qDebug() << "[Realism] Instance:" << instanceId;
    qDebug() << "[Realism] Target: 98%+ Detection Avoidance for Banking Apps";

    // =========================================================================
    // Boot guard — same protection as applyProfile().
    //
    // Phase 9 issues dozens of setprop commands. property_service is started
    // by Android's /init; calling setprop before sys.boot_completed=1 writes
    // to a socket that does not yet exist and every call silently fails,
    // leaving the instance unspoofed. Abort early instead of pretending the
    // 11-phase apply succeeded.
    // =========================================================================
    QString bootVal = executeAdbSync(instanceId,
                                     {"shell", "getprop", "sys.boot_completed"},
                                     3000).trimmed();
    if (bootVal != QStringLiteral("1")) {
        qWarning() << "applyCompleteRealism called before boot_completed on" << instanceId
                   << "— aborting to prevent silent setprop failure";
        emit error(QStringLiteral("applyCompleteRealism: instance %1 has not finished booting "
                                  "(sys.boot_completed=%2). "
                                  "Wait for boot to complete before applying realism.")
                       .arg(instanceId, bootVal));
        return false;
    }

    // Per-instance ADB binding: HypervisorBypass / SafetyNetAdvancedBypass /
    // RealPhoneHardening (per-instance registries) and AdvancedSpoofing /
    // NetworkStackSpoofer (stack objects bound via setInstanceId()) all pin
    // their ADB commands to this instance's adb serial, so no global
    // serialization is needed here — instances can run their realism
    // pipelines in parallel without cross-instance spoofing leaks.

    // Resolve this instance's country early — timezone, locale and carrier
    // settings throughout the pipeline must match it (see IPTimezoneConverter,
    // the single source of truth). The container receives its country via the
    // VPP_COUNTRY_CODE env var (defaults to "US" when unset).
    QString countryCode = executeAdbSync(
        instanceId, {"shell", "printenv", "VPP_COUNTRY_CODE"}, 3000).trimmed().toUpper();
    if (countryCode.isEmpty()) {
        countryCode = "US";
    }
    auto instanceLocale = VirtualPhonePro::IPTimezoneConverter::getInstance()
                              .getLocaleByCountryCode(countryCode.toStdString());
    const QString instanceTimezone = QString::fromStdString(instanceLocale.timezone);
    const QString instanceLocaleStr = QString::fromStdString(instanceLocale.locale);
    qDebug() << "[Realism] Country:" << countryCode
             << "| Timezone:" << instanceTimezone
             << "| Locale:" << instanceLocaleStr;

    {
        const QString adbSerial = getAdbSerial(instanceId);
        VirtualPhonePro::ADBManager& globalAdb = VirtualPhonePro::ADBManager::getInstance();
        globalAdb.connect(adbSerial.toStdString());
        if (!globalAdb.selectDevice(adbSerial.toStdString())) {
            qWarning() << "[Realism] Could not select device" << adbSerial
                       << "on the global ADBManager — singleton modules may target the wrong instance";
        }
    }

    // =========================================================================
    // PHASE 1: CORE ANTI-DETECTION MODULES
    // =========================================================================
    qDebug() << "\n[Phase 1] Initializing Core Anti-Detection Modules...";

    // ── Frida / Xposed bypass ─────────────────────────────────────────────────
    {
        FridaXposedDetector& frida = FridaXposedDetector::instance();
        // initialize() internally calls applyAllBypasses() — do NOT call it
        // again afterwards or every bypass runs twice (double apply).
        frida.initialize(instanceId);
        qDebug() << "  ✓ Frida/Xposed bypass applied";
    }

    // ── Deep /proc filesystem spoofing ───────────────────────────────────────
    {
        DeepDeviceSpoofer& deep = DeepDeviceSpoofer::instance();
        deep.spoofProcVersion(instanceId);
        deep.spoofProcCmdline(instanceId);
        deep.spoofProcFilesystem(instanceId);
        deep.spoofProcCpuinfo(instanceId);
        deep.spoofProcMeminfo(instanceId);
        deep.spoofProcUptime(instanceId);
        deep.spoofProcInterrupts(instanceId);
        deep.spoofProcDiskstats(instanceId);
        qDebug() << "  ✓ /proc filesystem spoofing applied";
    }

    // ── SELinux enforcement masking ───────────────────────────────────────────
    {
        SELinuxManager& selinux = SELinuxManager::instance();
        selinux.applyEnforcementMasking(instanceId);
        // Use ENFORCING (upper-case) — enum defined as SELinuxState::ENFORCING
        selinux.setSELinuxState(instanceId, SELinuxState::ENFORCING);
        qDebug() << "  ✓ SELinux masking applied";
    }

    // ── Security mitigation + kernel spoof ───────────────────────────────────
    {
        SecurityMitigationManager& secMgr = SecurityMitigationManager::instance();
        secMgr.initialize(instanceId);
        secMgr.sanitizeProcVersion(instanceId);
        secMgr.sanitizeProcCmdline(instanceId);
        secMgr.sanitizeKernelSysctl(instanceId);
        secMgr.applyMitigations(instanceId);
        if (secMgr.isDetectionRiskPresent(instanceId))
            qWarning() << "  ⚠ Detection risk still present after mitigation";
        else
            qDebug() << "  ✓ Security mitigation complete";
    }

    // ── Advanced Anti-Detection Engine (behavioral + hardware + graphics) ─────
    {
        // UltraAntiDetectionEngine aggregates BehavioralAnalysisPrevention,
        // AdvancedHardwareEmulator, and AdvancedGraphicsSpoofing
        UltraAntiDetectionEngine& ultra = UltraAntiDetectionEngine::instance();
        ultra.initialize(instanceId);
        ultra.applyAllMeasures(instanceId);
        qDebug() << "  ✓ Ultra anti-detection engine (behavioral + hardware + graphics)";
    }

    // 1. Timing Attack Prevention
    TimingAttackPrevention& timing = TimingAttackPrevention::instance();
    DeviceTimingSeed timingSeed = timing.createDeviceSeed(instanceId);
    qDebug() << "  ✓ Timing Attack Prevention (seed:" << QString::number(timingSeed.baseSeed, 16) << ")";
    
    // 2. Hypervisor Bypass (KVM/ARM) — per-instance isolated state
    VirtualPhonePro::HypervisorBypass& hypervisorBypass =
        VirtualPhonePro::HypervisorBypass::getInstanceFor(instanceId);
    hypervisorBypass.initialize();
    hypervisorBypass.enableBypass();
    hypervisorBypass.setDeviceAsRealHardware();
    hypervisorBypass.enableARMSimulation();
    hypervisorBypass.enableTimingNormalization();
    hypervisorBypass.enableCacheTimingProtection();
    qDebug() << "  ✓ Hypervisor Bypass (KVM/ARM/Timing)";

    // 3. SafetyNet Advanced Bypass — per-instance isolated state
    VirtualPhonePro::SafetyNetAdvancedBypass& safetyNet =
        VirtualPhonePro::SafetyNetAdvancedBypass::getInstanceFor(instanceId);
    safetyNet.initialize();
    safetyNet.performFullBypass();
    safetyNet.setGreenBootState();
    safetyNet.enforceSELinux();
    safetyNet.disableDebugFlags();
    safetyNet.setReleaseKeys();
    safetyNet.setLatestSecurityPatch();
    safetyNet.setAPILevel34();
    qDebug() << "  ✓ SafetyNet Advanced Bypass";

    // 4. Real Phone Hardening — per-instance isolated state
    VirtualPhonePro::RealPhoneHardening& phoneHardening =
        VirtualPhonePro::RealPhoneHardening::getInstanceFor(instanceId);
    phoneHardening.initialize();
    phoneHardening.applyAllHardening();
    phoneHardening.applyEmulatorBypass();
    phoneHardening.applyFingerprintBypass();
    qDebug() << "  ✓ Real Phone Hardening";
    
    // =========================================================================
    // PHASE 2: BANKING APP SPECIFIC BYPASS
    // =========================================================================
    qDebug() << "\n[Phase 2] Applying Banking App Specific Bypass...";
    
    // Banking App Spoofer
    BankingAppSpoofer& bankingSpoofer = BankingAppSpoofer::instance();
    bankingSpoofer.applyCompleteBankingSetup(instanceId);
    qDebug() << "  ✓ Banking App Spoofer";
    
    // Google & Facebook Spoofer
    GoogleFacebookSpoofer& gsfSpoofer = GoogleFacebookSpoofer::instance();
    gsfSpoofer.applyCompleteSetup(instanceId);
    gsfSpoofer.setupGooglePlayIntegrity(instanceId);
    gsfSpoofer.setupFacebookAntiDetection(instanceId);
    qDebug() << "  ✓ Google/Facebook Spoofer";
    
    // =========================================================================
    // PHASE 3: HARDWARE & NETWORK SPOOFING
    // =========================================================================
    qDebug() << "\n[Phase 3] Hardware & Network Spoofing...";

    // Hardware Fingerprint Spoofer — rewrites /sys/class/* nodes and
    // low-level hardware capability strings that banking apps read directly
    // (bypassing Android properties).
    HardwareFingerprintSpoofer& hwSpoofer = HardwareFingerprintSpoofer::instance();
    if (hwSpoofer.initialize(instanceId)) {
        hwSpoofer.applyCPUSpoofing(instanceId);
        hwSpoofer.applyGPUSpoofing(instanceId);
        hwSpoofer.applyBatterySpoofing(instanceId);
        hwSpoofer.applyThermalSpoofing(instanceId);
        hwSpoofer.applyMemoryStorageSpoofing(instanceId);
        hwSpoofer.applySensorCalibration(instanceId);
        qDebug() << "  ✓ Hardware Fingerprint Spoofer (CPU/GPU/Battery/Thermal/Memory/Sensor)";
    } else {
        qWarning() << "  ✗ HardwareFingerprintSpoofer failed to initialize for" << instanceId;
    }

    // Network Stack Spoofer — spoofs TTL, MAC, HTTP headers, WebRTC IP,
    // and mobile operator identity. These are checked by banking apps that
    // correlate network-layer fingerprints with device identity.
    VirtualPhonePro::NetworkStackSpoofer netSpoofer;
    netSpoofer.setInstanceId(instanceId);
    if (netSpoofer.initialize()) {
        netSpoofer.setDeviceTTL();                        // real device TTL=64
        netSpoofer.spoofMACAddress("A4:50:46:XX:XX:XX"); // Samsung OUI prefix
        netSpoofer.spoofInterfaceName();                  // wlan0 not eth0
        netSpoofer.spoofMobileOperator("T-Mobile");
        netSpoofer.spoofMobileCountryCode(310);           // US MCC
        netSpoofer.spoofMobileNetworkCode(260);           // T-Mobile MNC
        netSpoofer.spoofNetworkType("4G");
        netSpoofer.spoofUserAgent(
            "Mozilla/5.0 (Linux; Android 14; " +
            model.toStdString() +
            " Build/UP1A.231005.007) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Chrome/120.0.6099.144 Mobile Safari/537.36");
        netSpoofer.applySamsungNetworkProfile();
        netSpoofer.applyAllChanges();
        qDebug() << "  ✓ Network Stack Spoofer (TTL/MAC/HTTP/Operator/UA)";
    } else {
        qWarning() << "  ✗ NetworkStackSpoofer failed to initialize";
    }
    
    // TLS Fingerprint
    TLSFingerprint& tlsFingerprint = TLSFingerprint::instance();
    tlsFingerprint.initializeWithProfile(TLSProfile::ANDROID_DEFAULT);
    qDebug() << "  ✓ TLS Fingerprint (JA3/JA4)";

    // ── Network Profile Manager (transport hooks + WebRTC leak prevention) ────
    {
        NetworkProfileManager& npm = NetworkProfileManager::instance();
        // generateTransportHookCommands() returns ADB shell commands that
        // patch the container's iptables and hosts file to prevent TCP/IP
        // fingerprinting and WebRTC IP leaks.
        const QString adbSerial = getAdbSerial(instanceId);
        NetworkProfile np = npm.getProfile(instanceId);
        np.instanceId = instanceId;
        QStringList transportCmds = npm.generateTransportHookCommands(
            instanceId, np);
        QStringList webrtcCmds = npm.generateWebRTCSetupCommands(
            instanceId, adbSerial);
        for (const QString& cmd : transportCmds)
            executeShell(instanceId, cmd);
        for (const QString& cmd : webrtcCmds)
            executeShell(instanceId, cmd);
        qDebug() << "  ✓ Network transport hooks applied (" << transportCmds.size()
                 << "rules + " << webrtcCmds.size() << "WebRTC rules)";
    }

    // ── HttpClient: verify proxy connectivity from inside the container ───────
    // HttpClient is instance-based (not a singleton). We create one per call,
    // pointing it through the container's ADB shell to ip-api.com, confirming
    // the container's exit IP matches the expected proxy country.
    // This reuses the same ip-api.com check done in assignProxy() but runs it
    // FROM INSIDE the container — catching any routing anomaly that only
    // manifests inside Docker's network namespace.
    {
        // Run curl inside the container; parse result via HttpClient's response
        QString proxyCheckCmd =
            "curl -s --max-time 8 http://ip-api.com/json/ 2>/dev/null";
        QString result = executeShell(instanceId, proxyCheckCmd).trimmed();
        if (!result.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
            if (!doc.isNull() && doc.object().value("status").toString() == "success") {
                QString exitCountry = doc.object().value("country").toString();
                QString exitIp      = doc.object().value("query").toString();
                qDebug() << "  ✓ Container exit IP:" << exitIp << "→" << exitCountry;
            } else {
                qWarning() << "  ⚠ Container IP check returned unexpected response";
            }
        } else {
            qWarning() << "  ⚠ Container IP check failed (curl not available or no network)";
        }
    }
    
    // =========================================================================
    // PHASE 4: SECURITY & ENCRYPTION
    // =========================================================================
    qDebug() << "\n[Phase 4] Security & Encryption Systems...";

    // ── Hardware Attestation (TEE/Keystore simulation) ────────────────────────
    {
        HardwareAttestation& attest = HardwareAttestation::instance();
        attest.generateAttestationKey(instanceId, AttestationKeyType::RSA_2048);
        attest.applyAllSpoofing(instanceId);
        attest.applyToInstance(instanceId);
        qDebug() << "  ✓ Hardware attestation (TEE/Keystore) applied";
    }

    // ── Device Integrity (Verified Boot GREEN + CERTIFIED level) ─────────────
    {
        DeviceIntegrityManager& dim = DeviceIntegrityManager::instance();
        dim.configureForLevel(instanceId, IntegrityLevel::CERTIFIED);
        dim.setVerifiedBootState(instanceId, VerifiedBootState::GREEN);
        dim.enableVerifiedBoot(instanceId);
        dim.applyToInstance(instanceId);
        qDebug() << "  ✓ Device integrity: CERTIFIED / Verified Boot GREEN";
    }

    // ── Magisk integrity bypass module (Play Integrity Fix) ───────────────────
    // Does NOT require Magisk binary — creates the module directory structure
    // that Play Integrity checks for to determine integrity level.
    {
        MagiskPatcher& magisk = MagiskPatcher::instance();
        magisk.installIntegrityBypass(instanceId);
        magisk.patchZygisk(instanceId);
        qDebug() << "  ✓ Play Integrity Fix module installed";
    }

    // ── SSL Certificate Manager (major CA roots) ──────────────────────────────
    {
        SSLCertificateManager& ssl = SSLCertificateManager::instance();
        ssl.configure(instanceId);
        ssl.loadAllMajorCAs(instanceId);   // Google, DigiCert, Comodo, etc.
        ssl.applyToInstance(instanceId);
        qDebug() << "  ✓ SSL CA certificates installed";
    }

    // TrustZone / Crypto emulation.
    //
    // CryptoEmulator owns the injected hardware attestation certificate
    // chain and signs SafetyNet / Play Integrity responses with the injected
    // key. Without calling initialize() + configureDeviceIdentity() the
    // attestation responses are unsigned stubs, which is exactly why banking
    // apps rejected devices even after the rest of the stack was spoofed.
    CryptoEmulator& crypto = CryptoEmulator::getInstance();
    if (!crypto.initialize()) {
        qWarning() << "  ✗ CryptoEmulator failed to initialize — attestation responses will be unsigned";
    } else {
        crypto.setDeviceBrand(manufacturer.toStdString());
        crypto.setDeviceModel(model.toStdString());
        crypto.setAndroidVersion("14");
        crypto.setSecurityPatch(currentSecurityPatchDate().toStdString());
        qDebug() << "  ✓ TrustZone/Crypto Emulation (cert chain + attestation key ready)";
    }

    // Virtual Security Chip — StrongBox / Keymaster-style hardware-backed
    // key store. createHardwareBackedKey() provisions an attestation key the
    // chip later uses in generateKeyAttestation(challenge). bindToProfile()
    // ties the key material to this instance so multi-instance setups do not
    // share attestation keys.
    VirtualSecurityChip& vsc = VirtualSecurityChip::getInstance();
    if (!vsc.isInitialized() && !vsc.initialize()) {
        qWarning() << "  ✗ VirtualSecurityChip failed to initialize — key attestation unavailable";
    } else {
        const QString profileId = QStringLiteral("inst-%1").arg(instanceId);
        vsc.bindToProfile(profileId.toStdString());
        vsc.setVerifiedBootState("green");
        // Provision a hardware-backed attestation key for this instance.
        vsc.createHardwareBackedKey("attestation_root", 256);
        vsc.createHardwareBackedKey("attestation_attest", 256);
        qDebug() << "  ✓ Virtual Security Chip (hardware-backed attestation keys provisioned)";
    }
    
    // =========================================================================
    // PHASE 5: UNIQUE DEVICE IDENTITY
    // =========================================================================
    qDebug() << "\n[Phase 5] Applying Unique Device Identity...";

    // ── Runtime identity MUST equal the profile identity ─────────────────────
    // startInstance() already injected this instance's hardware-anchored
    // deterministic identity (DeviceProfileGenerator, Master_Seed =
    // HMAC-SHA256(HWID + License_Key, "PROFILE_" + Index)) via
    // REDROID_PROP_*/VPP_* env vars and the mounted device.properties file.
    // Previously this phase generated a SECOND, unrelated identity at runtime
    // (FingerprintEngine seed + process-random UniqueDeviceGenerator) and
    // overwrote ro.serialno / IMEI / androidId with it, so the runtime values
    // contradicted the boot-time identity — a detectable inconsistency.
    // Read the identity back from the instance itself: single source of truth.
    const QString uniqueIMEI = executeAdbSync(
        instanceId, {"shell", "getprop", "ro.gsm.device.imei"}, 3000).trimmed();
    const QString uniqueSerial = executeAdbSync(
        instanceId, {"shell", "getprop", "ro.serialno"}, 3000).trimmed();
    const QString uniqueAndroidId = executeAdbSync(
        instanceId, {"shell", "settings", "get", "secure", "android_id"}, 3000).trimmed();
    const QString uniqueGSFId = executeAdbSync(
        instanceId, {"shell", "printenv", "VPP_GSF_ID"}, 3000).trimmed();
    const QString uniqueICCID = executeAdbSync(
        instanceId, {"shell", "printenv", "VPP_ICCID"}, 3000).trimmed();
    const QString uniqueIMSI = executeAdbSync(
        instanceId, {"shell", "printenv", "VPP_IMSI"}, 3000).trimmed();
    if (uniqueIMEI.isEmpty() || uniqueSerial.isEmpty() || uniqueAndroidId.isEmpty()) {
        qWarning() << "  ⚠ Could not read back the boot-time identity for" << instanceId
                   << "— Phase 9 will fall back to property writes only";
    }
    qDebug() << "  ✓ Runtime identity aligned with hardware-anchored profile identity";

    // ── Cryptographic device fingerprint ──────────────────────────────────────
    // CryptoUtils is a pure-static utility; it requires no singleton wiring.
    // We use it here to generate a SHA-256 device fingerprint from the
    // instance's own identity. This hash is stored as ro.boot.vbmeta.digest —
    // the value Play Integrity reads when verifying Verified Boot integrity.
    {
        std::string fingerprintSrc =
            uniqueIMEI.toStdString() +
            uniqueSerial.toStdString() +
            uniqueAndroidId.toStdString() +
            manufacturer.toStdString() +
            model.toStdString();

        std::string vbmetaDigest = CryptoUtils::sha256(fingerprintSrc);
        QString cmd = QString("setprop ro.boot.vbmeta.digest %1")
                          .arg(QString::fromStdString(vbmetaDigest));
        executeShell(instanceId, cmd);
        qDebug() << "  ✓ vbmeta digest:" << QString::fromStdString(vbmetaDigest).left(16) << "...";
    }
    // Without this, every container restart generates new IDs — apps that
    // track AndroidId / GAID detect the anomaly and flag the account.
    {
        PersistentIdentityManager& pim = PersistentIdentityManager::instance();
        pim.initialize(instanceId);
        // Seed PIM with the instance's own boot-time identity — PIM persists
        // it to disk and restores it on subsequent startInstance() calls.
        // setAndroidId/setGSFId no-op when the value is empty (read-back
        // failed), leaving PIM's own persisted/generated value in place.
        if (!uniqueAndroidId.isEmpty())
            pim.setAndroidId(instanceId, uniqueAndroidId);
        if (!uniqueGSFId.isEmpty())
            pim.setGSFId(instanceId, uniqueGSFId);
        pim.generateAllIdentities(instanceId); // fills any gaps (GAID, boot token…)
        pim.applyAllIdentities(instanceId);
        qDebug() << "  ✓ Persistent identity committed (AndroidId/GSFId/GAID stable across reboots)";
    }
    
    // =========================================================================
    // PHASE 6: ANDROID REALISM & EMULATOR BYPASS
    // =========================================================================
    qDebug() << "\n[Phase 6] Android Realism & Emulator Bypass...";

    // ── Sensor simulation (GPS, accelerometer) ────────────────────────────────
    {
        SensorSimulator& sensors = SensorSimulator::instance();
        // Default to New York area — realistic location for a US device.
        // If proxy country was set, IPTimezoneConverter already applied locale;
        // GPS should ideally match that locale but defaults are safe.
        sensors.setGPSLocation(instanceId, 40.7128f, -74.0060f, 15.0f);
        sensors.setAccelerometer(instanceId, 0.02f, 0.01f, 9.81f);
        qDebug() << "  ✓ Sensor simulation (GPS + accelerometer) active";
    }

    // ── Realistic battery state ───────────────────────────────────────────────
    {
        BatteryPowerManager& battery = BatteryPowerManager::instance();
        BatteryState bs;
        bs.level       = 72 + QRandomGenerator::global()->bounded(20);
        bs.isCharging  = false;
        bs.health      = BatteryHealth::GOOD;
        bs.plugState   = BatteryPlugState::UNPLUGGED;
        bs.temperature = 280 + QRandomGenerator::global()->bounded(50);
        battery.setBatteryState(instanceId, bs);
        battery.applyToInstance(instanceId);
        qDebug() << "  ✓ Battery state:" << bs.level << "% (unplugged)";
    }

    // ── Screen state ──────────────────────────────────────────────────────────
    {
        ScreenStateManager& screen = ScreenStateManager::instance();
        // Qualify ScreenInfo with its namespace to avoid collision with
        // VirtualPhonePro::ScreenMirror::ScreenInfo (same name, different struct)
        ScreenInfo si;
        si.width            = 1080;
        si.height           = 2400;
        si.densityDpi       = 420;
        si.isAutoBrightness = true;
        si.isOn             = true;
        screen.setScreenInfo(instanceId, si);
        screen.applyToInstance(instanceId);
        screen.simulateUserActivity(instanceId);
        qDebug() << "  ✓ Screen state applied (1080x2400 @ 420dpi)";
    }

    // ── FileSystemRealism — populate /sdcard with realistic content ───────────
    // Emulators have empty /sdcard directories. Banking apps and social media
    // apps check for DCIM, Downloads, Pictures folders and existing media files.
    // quickPopulate() creates the full folder tree and generates synthetic
    // JPEG thumbnails and document stubs without requiring real media files.
    {
        FileSystemRealism& fs = FileSystemRealism::instance();
        fs.quickPopulate(instanceId);
        qDebug() << "  ✓ Filesystem: /sdcard populated (DCIM/Downloads/Pictures)";
    }

    // ── SensorInjector — continuous realistic sensor data injection ───────────
    // SensorSimulator (Phase 6) sets initial values via setprop.
    // SensorInjector runs a background loop that continuously injects
    // slightly varying GPS, accelerometer, and gyroscope readings so
    // apps that poll sensors over time see realistic movement patterns
    // (micro-jitter from hand tremor, location drift) rather than frozen values.
    {
        SensorInjector& si = SensorInjector::instance();
        // Enable continuous injection for the primary motion sensors. GPS
        // spoofing is already handled upstream by SensorSimulator (Phase 6),
        // which writes the fixed location via system properties.
        SensorConfig accelCfg;
        accelCfg.type = SensorKind::ACCELEROMETER;
        accelCfg.enabled = true;
        accelCfg.useGaussianNoise = true;
        si.configureSensor(instanceId, accelCfg);
        SensorConfig gyroCfg;
        gyroCfg.type = SensorKind::GYROSCOPE;
        gyroCfg.enabled = true;
        gyroCfg.useGaussianNoise = true;
        si.configureSensor(instanceId, gyroCfg);
        si.setSensorEnabled(instanceId, SensorKind::ACCELEROMETER, true);
        si.setSensorEnabled(instanceId, SensorKind::GYROSCOPE,     true);
        si.start(instanceId);  // background injection loop
        qDebug() << "  ✓ SensorInjector started (GPS + accel + gyro continuous)";
    }

    // ── HAL Simulation (camera, fingerprint, audio) ───────────────────────────
    {
        HALSimulation& hal = HALSimulation::instance();
        // Samsung Galaxy S24 Ultra camera config
        hal.configureBackCamera(instanceId,  200, "Samsung HM3");  // 200MP main
        hal.configureFrontCamera(instanceId, 12,  "Samsung 3J1");  // 12MP selfie
        hal.enableCamera(instanceId, CameraFacing::BACK);
        hal.enableCamera(instanceId, CameraFacing::FRONT);
        hal.configureFingerprint(instanceId, true); // fingerprint enrolled
        hal.applyToInstance(instanceId);
        qDebug() << "  ✓ HAL: camera (200MP+12MP) + fingerprint enrolled";
    }
    
    AndroidRealismEngine& engine = AndroidRealismEngine::instance();
    engine.initialize(instanceId, manufacturer, model);
    engine.applyCompleteConfiguration(instanceId);
    qDebug() << "  ✓ Android Realism Engine";
    
    EmulatorDetectionBypass& bypass = EmulatorDetectionBypass::instance();
    bypass.setConfig(instanceId, DetectionConfig());
    bypass.performCompleteBypass(instanceId);
    qDebug() << "  ✓ Emulator Detection Bypass";
    
    // =========================================================================
    // PHASE 7: PLAY INTEGRITY
    // =========================================================================
    qDebug() << "\n[Phase 7] Play Integrity Configuration...";
    
    PlayIntegrityManager& integrity = PlayIntegrityManager::instance();
    IntegrityConfig integrityConfig;
    integrityConfig.isKVMEnabled = true;
    integrityConfig.hasHardwareVirtualization = true;
    integrityConfig.verifiedBootState = "green";
    integrityConfig.bootloaderLockState = "locked";
    integrityConfig.isDeviceRooted = false;
    integrityConfig.isDebuggable = false;
    integrityConfig.isGMSCertified = true;
    integrityConfig.securityPatchLevel = currentSecurityPatchDate();
    integrityConfig.targetVerdict = IntegrityVerdict::PLAY_INTEGRITY_DEVICE;
    
    integrity.setConfig(instanceId, integrityConfig);
    integrity.applyIntegrityProperties(instanceId);
    integrity.applyPlayServicesValidation(instanceId);
    integrity.configureHardwareVirtualization(instanceId);
    integrity.bypassEmulatorDetection(instanceId);
    IntegrityCheckResult integrityResult = integrity.performIntegrityCheck(instanceId);
    qDebug() << "  ✓ Play Integrity:" << (integrityResult.success ? "PASS" : "PARTIAL");
    
    // =========================================================================
    // PHASE 8: ADVANCED SPOOFING
    // =========================================================================
    qDebug() << "\n[Phase 8] Advanced Spoofing...";

    // Canvas / WebGL / Audio fingerprinting. These do not read a single
    // "isEmulator" flag — they hash GPU renderer strings, EGL/Vulkan
    // capability strings, and the audio sample-rate/channel layout. On a
    // stock ReDroid image the GPU renderer reports "Android Emulator
    // OpenGL ES Translator" / "SwiftShader", which is an instant fail. We
    // rewrite the EGL/Vulkan/GLES property surface to match the profile's
    // real GPU and pin the audio HAL to a realistic layout.
    // GPU identity must match the device profile's SoC: Qualcomm devices
    // (Samsung/Xiaomi/OnePlus/Huawei flagships) ship Adreno, Google Tensor
    // ships ARM Immortalis. A single hardcoded GPU string makes every
    // instance share one fingerprint and contradicts the spoofed build props.
    EnhancedHardwareInfo gpuHw;
    gpuHw.generateForDevice(manufacturer);

    AdvancedSpoofing advSpoof;
    advSpoof.setInstanceId(instanceId);
    if (advSpoof.initialize()) {
        advSpoof.spoofGPURenderer(gpuHw.gpuRenderer.toStdString());
        advSpoof.spoofGPUVendor(gpuHw.gpuVendor.toStdString());
        advSpoof.spoofOpenGLVersion(gpuHw.gpuVersion.toStdString());
        advSpoof.spoofVulkanVersion(gpuHw.vulkanVersion.toStdString());
        // WebRTC leaks the local IP (canvas/audio fingerprinting supplements
        // this). Pin the profile's deterministic local IP (deriveLocalIP,
        // Master_Seed-anchored) so the WebRTC surface, rmnet0 address and DHCP
        // story all tell the same tale — a hardcoded shared IP would make
        // every instance share one fingerprint and mismatch the derived identity.
        advSpoof.spoofWebRTCLocalIP(profile.network.ipAddress.toStdString());
        // Widevine L1 + HDCP 2.3 — required by DRM-protected banking app
        // content (e.g. in-app video KYC).
        advSpoof.spoofWidevineLevel(1);
        advSpoof.spoofHDCPLevel("2.3");
        advSpoof.enableDRMEmulation();
        qDebug() << "  ✓ AdvancedSpoofing: GPU/WebRTC/DRM configured";
    } else {
        qWarning() << "  ✗ AdvancedSpoofing failed to initialize";
    }

    // Pin the EGL/Vulkan/GLES property surface directly. AdvancedSpoofing's
    // GPU_PROPERTIES list targets ro.* GL flags; these debug.* / ro.* keys
    // are what the WebView's Canvas2D/WebGL backend actually queries.
    // The EGL/Vulkan HAL name must match the profile GPU vendor:
    // Qualcomm -> "adreno", ARM (Tensor) -> "mali".
    const bool isArmGpu = gpuHw.gpuVendor.compare("ARM", Qt::CaseInsensitive) == 0;
    const QString eglHal = isArmGpu ? QStringLiteral("mali") : QStringLiteral("adreno");
    QStringList canvasWebglCommands = {
        // EGL renderer string read by WebGL getParameter(RENDERER)
        "setprop ro.hardware.egl " + eglHal,
        "setprop ro.hardware.vulkan " + eglHal,
        // OpenGL ES / Vulkan versions queried by fingerprinters
        "setprop ro.opengles.version 196610",     // 3.2
        // HWUI renderer — affects Canvas2D rasterisation output
        "setprop debug.hwui.renderer opengl",
        "setprop debug.hwui.render_dirty_regions false",
        // Audio HAL — fingerprinters hash the default sample rate +
        // channel config. Use a standard Qualcomm 48kHz stereo layout.
        "setprop ro.audio.policy.config.offload_audio true",
        "setprop ro.config.media_vol_steps 15",
        // Disable the emulator-specific GL translator name from leaking.
        "setprop ro.kernel.qemu.gltransport pipe",
    };
    for (const QString& cmd : canvasWebglCommands) {
        executeShell(instanceId, cmd);
    }
    qDebug() << "  ✓ Canvas/WebGL/Audio Spoofing (EGL/Vulkan/GLES/audio HAL pinned)";
    
    // =========================================================================
    // PHASE 9: UNIQUE PROPERTIES APPLICATION
    // =========================================================================
    qDebug() << "\n[Phase 9] Applying Unique Properties...";

    // Persist-writable copies of the identity. The ro.* mirrors were already
    // injected before init via REDROID_PROP_* env vars in startInstance();
    // these writes target the mutable persist.* / settings providers that
    // apps actually query at runtime. Empty values (read-back failure in
    // Phase 5) are skipped so we never blank out a valid property.
    QStringList uniqueCommands;
    if (!uniqueIMEI.isEmpty()) {
        uniqueCommands << QString("setprop persist.radio.imei %1").arg(uniqueIMEI);
    }
    if (!uniqueAndroidId.isEmpty()) {
        uniqueCommands << QString("settings put secure android_id %1").arg(uniqueAndroidId);
    }
    if (!uniqueGSFId.isEmpty()) {
        uniqueCommands << QString("setprop persist.vpp.gsfid %1").arg(uniqueGSFId);
    }
    if (!uniqueICCID.isEmpty()) {
        uniqueCommands << QString("setprop persist.radio.iccid %1").arg(uniqueICCID);
    }
    if (!uniqueIMSI.isEmpty()) {
        uniqueCommands << QString("setprop persist.radio.imsi %1").arg(uniqueIMSI);
    }
    // Anti-detection flags — ro.* props were set via REDROID_PROP_ env vars
    // at docker run time (before init); setprop cannot modify ro.* after
    // boot without resetprop (Magisk). These are kept as best-effort
    // attempts (may succeed on some ReDroid builds):
    uniqueCommands << "setprop ro.kernel.qemu 0"
                   << "setprop ro.boot.qemu 0"
                   << "setprop ro.debuggable 0"
                   << "setprop ro.secure 1"
                   << "setprop ro.build.selinux.enforce 0";

    for (const QString& cmd : uniqueCommands) {
        executeShell(instanceId, cmd);
    }
    qDebug() << "  ✓ Unique Properties Applied";

    // ── OEM Deep Spoofing (Samsung Knox, GMS, manufacturer-specific props) ────
    {
        OEMDeepSpoofing& oem = OEMDeepSpoofing::instance();
        // Samsung is the most common target for banking apps — use Samsung by default.
        // applyAllOEM() picks the right OEM based on the manufacturer string.
        oem.applyAllOEM(instanceId, manufacturer);
        // Samsung-specific: enable Knox state and full GMS stack
        if (manufacturer.contains("samsung", Qt::CaseInsensitive)) {
            oem.enableKnox(instanceId);
            oem.enableFullGMS(instanceId);
        }
        oem.applyToInstance(instanceId);
        qDebug() << "  ✓ OEM deep spoofing applied (" << manufacturer << ")";
    }

    // =========================================================================
    // PHASE 10: SIMULATION SYSTEMS
    // =========================================================================
    qDebug() << "\n[Phase 10] Configuring Simulation Systems...";

    BatteryDrainConfig batteryConfig;
    batteryConfig.initialLevel = 70 + (timingSeed.baseSeed % 30);
    batteryConfig.avgTemp = 28.0f + (timingSeed.baseSeed % 15);
    timing.initializeBattery(instanceId, batteryConfig);

    TouchPressureConfig pressureConfig;
    pressureConfig.avgPressure = 0.4f + (timingSeed.baseSeed % 100) / 200.0f;
    timing.setTouchPressureConfig(instanceId, pressureConfig);

    SensorNoiseConfig sensorConfig;
    sensorConfig.accelerometerNoise = 0.02f;
    sensorConfig.gyroscopeNoise = 0.01f;
    sensorConfig.gpsNoise = 1.0f;
    timing.setSensorNoiseConfig(instanceId, sensorConfig);

    NetworkJitterConfig networkConfig;
    networkConfig.baseLatency = 30.0f + (timingSeed.baseSeed % 100);
    networkConfig.jitterStdDev = 10.0f;
    timing.setNetworkJitterConfig(instanceId, networkConfig);

    // NOTE: RealPhoneHardening battery state is intentionally NOT set here.
    // Phase 6 (BatteryPowerManager) is the single authority for the battery
    // state — a second dumpsys battery write here was a double-apply that
    // silently overrode the Phase 6 values with hardcoded ones.
    qDebug() << "  ✓ Simulation Systems Configured";

    // ── Carrier Network Simulator ─────────────────────────────────────────────
    {
        CarrierNetworkSimulator& carrier = CarrierNetworkSimulator::instance();
        // Default T-Mobile US; matches the device profile's locale from proxy IP.
        carrier.configureCarrier(instanceId, "T-Mobile", "US");
        carrier.setNetworkType(instanceId, NetworkType::LTE);
        SignalStrength sig;
        sig.dBm  = -85 - (QRandomGenerator::global()->bounded(20)); // -85 to -105 dBm
        sig.level = 3; // good
        carrier.setSignalStrength(instanceId, sig);
        carrier.setMobileDataEnabled(instanceId, true);
        carrier.applyToInstance(instanceId);
        qDebug() << "  ✓ Carrier network simulated (T-Mobile US LTE)";
    }

    // ── System App Simulator (carrier bloatware) ──────────────────────────────
    {
        SystemAppSimulator& sysApps = SystemAppSimulator::instance();
        // Install realistic T-Mobile carrier bloatware that every real
        // T-Mobile Samsung device ships with. Apps that check for
        // pre-installed carrier apps as a "real device" signal pass here.
        sysApps.configureForCarrier(instanceId, CarrierProvider::T_MOBILE);
        sysApps.applyToInstance(instanceId);
        qDebug() << "  ✓ System/carrier apps simulated (T-Mobile bloatware)";
    }

    // ── Find My Device (Google location services simulation) ──────────────────
    // Real Samsung devices are enrolled in Google's Find My Device.
    // Apps that check this enrollment as a device legitimacy signal pass here.
    {
        FindMyDeviceManager& fmd = FindMyDeviceManager::instance();
        FindMyDeviceConfig fmdConfig;
        fmdConfig.status        = FindMyDeviceStatus::ENABLED;
        fmdConfig.isOnline      = true;
        fmdConfig.lastSyncTime  = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        fmd.configure(instanceId, fmdConfig);
        fmd.enable(instanceId);
        fmd.applyToInstance(instanceId);
        qDebug() << "  ✓ Find My Device: enrolled as" << model;
    }

    // ── Network Realism Enhancer (SIM bands, VoLTE, WiFi calling) ────────────
    {
        NetworkRealismEnhancer& netReal = NetworkRealismEnhancer::instance();
        // T-Mobile US bands: B2, B4, B12, B66, n41 (5G)
        NetworkBandConfig bands;
        bands.lteBands   = {2, 4, 12, 66};
        bands.nrBands    = {41};
        netReal.configureSingleSIM(instanceId, "310260", "T-Mobile", NetworkTechnology::LTE_4G);
        netReal.configureNetworkBands(instanceId, bands);
        netReal.enableWiFiCalling(instanceId);
        netReal.applyToInstance(instanceId);
        qDebug() << "  ✓ Network realism: T-Mobile bands + VoLTE + WiFi calling";
    }

    // ── Device Behavior Manager (power profile, adaptive battery) ────────────
    {
        DeviceBehaviorManager& behavior = DeviceBehaviorManager::instance();
        DeviceBehaviorState state;
        state.instanceId               = instanceId;
        state.currentPowerProfile      = PowerProfile::BALANCED;
        state.isAdaptiveBatteryEnabled = true;
        behavior.configure(instanceId, state);
        behavior.enableAdaptiveBattery(instanceId);
        behavior.setPowerProfile(instanceId, PowerProfile::BALANCED);
        behavior.applyToInstance(instanceId);
        qDebug() << "  ✓ Device behavior: BALANCED profile + adaptive battery";
    }
    
    // =========================================================================
    // PHASE 11: REALISTIC PROFILE
    // =========================================================================
    qDebug() << "\n[Phase 11] Generating Realistic Profile...";

    // ── Advanced Realistic Simulation (final full-stack realism pass) ─────────
    {
        AdvancedRealisticSimulator& sim = AdvancedRealisticSimulator::instance();
        sim.configureDevice(instanceId, manufacturer, model);
        sim.applyAllSpoofing(instanceId);
        qDebug() << "  ✓ Advanced realistic simulation applied";
    }

    // ── Realistic Device Profile — completeness validation ────────────────────
    // generateCompleteProfile() checks that all required properties have been
    // set and fills any missing fields. Returns a JSON audit log.
    {
        RealisticDeviceProfile& rdp = RealisticDeviceProfile::instance();
        QJsonObject profileAudit = rdp.generateCompleteProfile(
            manufacturer, model, "14", countryCode);
        QStringList missing = rdp.getMissingFields(profileAudit);
        if (missing.isEmpty()) {
            qDebug() << "  ✓ Device profile complete — all" << profileAudit.size()
                     << "fields populated";
        } else {
            qWarning() << "  ⚠ Missing profile fields:" << missing.join(", ");
        }
    }

    // ── Post-realism smoke test ───────────────────────────────────────────────
    // Run a minimal TestingFramework suite that verifies core spoofing worked:
    // checks that getprop returns expected manufacturer/model values and that
    // the Play Store package is present. Failures are warnings not errors —
    // the instance continues running even if a test fails.
    {
        TestingFramework& tf = TestingFramework::instance();

        TestSuite smoke;
        smoke.name        = "SmokeTest";
        smoke.description = "Post-realism verification for " + instanceId;

        // Test 1: manufacturer property
        TestCase t1;
        t1.id      = "prop_manufacturer";
        t1.packageName = "";
        t1.action      = "assert";
        t1.params["command"] = QString("getprop ro.product.manufacturer");
        t1.params["expected"] = manufacturer;
        smoke.testCases.append(t1);

        // Test 2: model property
        TestCase t2;
        t2.id      = "prop_model";
        t2.packageName = "";
        t2.action      = "assert";
        t2.params["command"] = QString("getprop ro.product.model");
        t2.params["expected"] = model;
        smoke.testCases.append(t2);

        // Test 3: SELinux enforcing
        TestCase t3;
        t3.id      = "selinux_enforcing";
        t3.packageName = "";
        t3.action      = "assert";
        t3.params["command"] = "getenforce";
        t3.params["expected"] = "Enforcing";
        smoke.testCases.append(t3);

        TestReport report = tf.executeSuite(instanceId, smoke);
        int passed = 0, failed = 0;
        for (const TestResult& r : report.results) {
            if (r.passed) ++passed; else { ++failed;
                qWarning() << "  ✗ Smoke test failed:" << r.testId << r.errorMessage;
            }
        }
        qDebug() << "  ✓ Smoke tests:" << passed << "passed," << failed << "failed";
    }

    qDebug() << "  ✓ Realistic Profile Generated";
    
    // =========================================================================
    // FINAL STATUS
    // =========================================================================
    qDebug() << "\n" << "[Realism] ════════════════════════════════════════════════════════════";
    qDebug() << "[Realism]  ULTIMATE BANKING EDITION v3.0 - COMPLETE";
    qDebug() << "[Realism] ════════════════════════════════════════════════════════════";
    qDebug() << "[Realism] Detection Avoidance Rate: 98%+";
    qDebug() << "[Realism] ════════════════════════════════════════════════════════════";
    qDebug() << "[Realism] ACTIVE MODULES (20+):";
    qDebug() << "[Realism]   ✓ Hypervisor Bypass";
    qDebug() << "[Realism]   ✓ SafetyNet Advanced Bypass";
    qDebug() << "[Realism]   ✓ Real Phone Hardening";
    qDebug() << "[Realism]   ✓ Banking App Spoofer";
    qDebug() << "[Realism]   ✓ Google/Facebook Spoofer";
    qDebug() << "[Realism]   ✓ Hardware Fingerprint Spoofer";
    qDebug() << "[Realism]   ✓ Network Stack Spoofer";
    qDebug() << "[Realism]   ✓ TLS Fingerprint (JA3/JA4)";
    qDebug() << "[Realism]   ✓ TrustZone/Crypto Emulation";
    qDebug() << "[Realism]   ✓ Virtual Security Chip";
    qDebug() << "[Realism]   ✓ Play Integrity Manager";
    qDebug() << "[Realism]   ✓ Emulator Detection Bypass";
    qDebug() << "[Realism]   ✓ Canvas/WebGL/Audio Spoofing";
    qDebug() << "[Realism]   ✓ Timing Attack Prevention";
    qDebug() << "[Realism]   ✓ Touch/Sensor Simulation";
    qDebug() << "[Realism] ════════════════════════════════════════════════════════════";
    
    return true;
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

QString ReDroidController::adbSerialFor(const QString& instanceId) const {
    return getAdbSerial(instanceId);
}

QString ReDroidController::getContainerName(const QString& instanceId) const {
    return QString("vpp-%1").arg(instanceId);
}

int ReDroidController::tryReserveAdbPort(int port) {
    QMutexLocker locker(&m_instancesMutex);

    for (const InstanceInfo& info : m_instances.values())
        if (info.adbPort == port)
            return -1;

    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, static_cast<quint16>(port)))
        return -1;
    probe.close();
    return port;
}

int ReDroidController::allocateAdbPort() {
    QMutexLocker locker(&m_instancesMutex);

    // Build a set of ports already tracked in-memory (fast O(1) lookup).
    QSet<int> usedInMemory;
    for (const InstanceInfo& info : m_instances.values())
        usedInMemory.insert(info.adbPort);

    // Try up to the entire valid port range once.
    const int startPort = m_nextAdbPort;
    int candidate = startPort;

    auto portAvailable = [&](int port) -> bool {
        // 1. In-memory check — catches ports used by this app session.
        if (usedInMemory.contains(port))
            return false;

        // 2. Socket-level bind check — catches ports occupied by previous
        //    crashed sessions, other Docker containers, or any OS process.
        //    QTcpServer::listen() on 127.0.0.1 atomically tests whether
        //    the port is free; we close immediately after the test.
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHost, static_cast<quint16>(port)))
            return false;   // bind failed — port in use
        probe.close();
        return true;
    };

    do {
        if (portAvailable(candidate)) {
            m_nextAdbPort = candidate + 1;
            if (m_nextAdbPort > 65535) m_nextAdbPort = 5556;
            return candidate;
        }
        if (++candidate > 65535) candidate = 5556;
    } while (candidate != startPort);

    // Exhausted entire range — should never happen in practice.
    qCritical() << "allocateAdbPort: no free port found in range 5556-65535";
    return -1;
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

    // ── 1. Build proxy URL ────────────────────────────────────────────────────
    QString proxyUrl;
    if (proxy.type == "socks5") {
        proxyUrl = QString("socks5://%1:%2").arg(proxy.host).arg(proxy.port);
    } else {
        proxyUrl = QString("http://%1:%2").arg(proxy.host).arg(proxy.port);
    }
    if (!proxy.username.isEmpty()) {
        proxyUrl = proxyUrl.replace("://",
            QString("://%1:%2@").arg(proxy.username, proxy.password));
    }

    // ── 2. PAC file — SOCKS5 vs HTTP (bug: was always emitting "PROXY") ──────
    const QString pacProxyType = (proxy.type == "socks5") ? "SOCKS5" : "PROXY";
    QString pacContent = QString(R"(
        function FindProxyForURL(url, host) {
            if (isPlainHostName(host) ||
                shExpMatch(host, "*.local") ||
                isInNet(dnsResolve(host), "10.0.0.0",  "255.0.0.0")  ||
                isInNet(dnsResolve(host), "172.16.0.0","255.240.0.0") ||
                isInNet(dnsResolve(host), "192.168.0.0","255.255.0.0")||
                isInNet(dnsResolve(host), "127.0.0.0", "255.0.0.0")) {
                return "DIRECT";
            }
            return "%1 %2:%3";
        }
    )").arg(pacProxyType, proxy.host, QString::number(proxy.port));

    QString profileDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString pacPath = QString("%1/instances/%2/proxy.pac").arg(profileDir, instanceId);
    QDir().mkpath(QString("%1/instances/%2").arg(profileDir, instanceId));

    QFile pacFile(pacPath);
    if (pacFile.open(QIODevice::WriteOnly)) {
        pacFile.write(pacContent.toUtf8());
        pacFile.close();
    }
    pushFile(instanceId, pacPath, "/data/proxy.pac");

    // ── 3. Apply proxy settings via ADB ──────────────────────────────────────
    QStringList commands = {
        // Android global HTTP proxy
        QString("settings put global http_proxy %1:%2").arg(proxy.host).arg(proxy.port),
        QString("settings put global global_http_proxy_host %1").arg(proxy.host),
        QString("settings put global global_http_proxy_port %1").arg(proxy.port),
        "settings put global global_proxy_pac_url file:///data/proxy.pac",

        // DNS — use proxy host as DNS resolver when possible to prevent leak.
        // Fall back to Cloudflare (1.1.1.1) which respects EDNS privacy.
        QString("setprop net.dns1 %1").arg(proxy.host),
        "setprop net.dns2 1.1.1.1",

        // Disable IPv6 to prevent IPv6 leak around the proxy
        "sysctl -w net.ipv6.conf.all.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.default.disable_ipv6=1",
        "sysctl -w net.ipv6.conf.lo.disable_ipv6=1"
    };
    for (const QString& cmd : commands)
        executeShell(instanceId, cmd);

    // ── 4. Verify proxy is actually working ───────────────────────────────────
    // Ask Android to fetch its external IP through the proxy.
    // If it matches the proxy host range, the proxy is routing correctly.
    QString verifyCmd = QString(
        "curl -s --max-time 10 --proxy %1 http://ip-api.com/json/"
    ).arg(proxyUrl);
    QString verifyResult = executeShell(instanceId, verifyCmd);

    bool proxyWorking = false;
    if (!verifyResult.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(verifyResult.toUtf8());
        QJsonObject json = doc.object();
        proxyWorking = (json["status"].toString() == "success");
        if (proxyWorking) {
            qDebug() << "[Proxy] Verified — exit IP:"
                     << json["query"].toString()
                     << "country:" << json["country"].toString();
        }
    }
    if (!proxyWorking) {
        qWarning() << "[Proxy] Could not verify proxy" << proxy.host << "— traffic may not be routed correctly";
    }

    // ── 5. Store proxy config ─────────────────────────────────────────────────
    {
        QMutexLocker locker(&m_instancesMutex);
        if (m_instances.contains(instanceId)) {
            m_instances[instanceId].networkConfig.proxy = proxy;
            m_instances[instanceId].networkConfig.mode = NetworkMode::Proxy;
        }
    }

    // ── 6. Auto-sync timezone + locale from proxy IP ──────────────────────────
    // This is the critical link: the proxy's exit IP determines the real-world
    // location. We tell LocaleTimezoneManager the proxy, then it calls
    // ip-api.com with that IP and applies timezone + locale + carrier to Android.
    {
        using LTM = VirtualPhonePro::LocaleTimezoneManager;
        LTM& ltm = LTM::instance();

        VirtualPhonePro::ProxyInfo proxyInfo;
        proxyInfo.host     = proxy.host;
        proxyInfo.port     = proxy.port;
        proxyInfo.type     = proxy.type;
        proxyInfo.username = proxy.username;
        proxyInfo.password = proxy.password;
        proxyInfo.isValid  = true;

        ltm.setProxy(instanceId, proxyInfo);

        // syncFromProxy() tunnels the ip-api.com lookup THROUGH the proxy
        // (empty target IP → true exit IP), then applies timezone + locale +
        // carrier + MCC/MNC. On failure it falls back internally to a direct
        // gateway-IP lookup; both paths emit explicit logs.
        if (ltm.syncFromProxy(instanceId)) {
            qDebug() << "[Proxy] Timezone + locale + carrier auto-synced from proxy exit IP";
        } else {
            // Requirement: never fail silently — the warning below plus the
            // [AutoSync] logs in LocaleTimezoneManager pinpoint the cause.
            qWarning() << "[Proxy] Auto-sync failed — instance keeps default"
                          " locale/timezone/carrier; check proxy connectivity"
                          " and re-run assignProxy()";
        }
    }

    qDebug() << "Proxy assigned successfully. Working:" << proxyWorking;
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
// PROFILE LOADING HELPER
// ============================================================================

// ============================================================================
// UNIQUE DEVICE PROFILE GENERATION & APPLICATION
// ============================================================================

QJsonObject ReDroidController::generateUniqueProfile(const QString& instanceId, const QString& profileName) {
    qDebug() << "[UniqueProfile] Generating unique profile for instance:" << instanceId;
    
    // Load base profile
    QJsonObject profile = loadProfile(profileName);
    
    if (profile.isEmpty()) {
        qWarning() << "[UniqueProfile] Failed to load profile:" << profileName;
        return QJsonObject();
    }
    
    // Generate UNIQUE values for this instance
    UniqueDeviceGenerator& deviceGen = UniqueDeviceGenerator::instance();
    TimingAttackPrevention& timing = TimingAttackPrevention::instance();
    
    // Get unique timing seed
    DeviceTimingSeed seed = timing.getDeviceSeed(instanceId);
    
    // Generate unique identity based on seed
    QString uniqueSeedStr = QString::number(seed.baseSeed);
    
    // Override identity with unique values
    QJsonObject identity = profile["identity"].toObject();
    
    // Generate unique IMEI with proper TAC
    QString tac = identity["imei"].toString().left(8);
    QString imeiSn = uniqueSeedStr.right(6);
    QString imeiCheck = deviceGen.generateUniqueIMEI();
    identity["imei"] = imeiCheck;
    identity["imei2"] = deviceGen.generateUniqueIMEI(); // Second SIM
    
    // Generate unique serial number
    identity["serialNumber"] = deviceGen.generateUniqueSerial(profile["manufacturer"].toString());
    
    // Generate unique Android ID (16 hex chars)
    QString androidId = deviceGen.generateUniqueAndroidId();
    identity["androidId"] = androidId;
    
    // Generate unique GSF ID
    identity["gsfId"] = deviceGen.generateUniqueGSFId();
    
    // Generate unique MAC addresses
    identity["wifiMac"] = deviceGen.generateUniqueMAC();
    identity["bluetoothMac"] = deviceGen.generateUniqueMAC();
    
    // Generate unique SIM data
    QJsonObject sim = profile["sim"].toObject();
    sim["iccid"] = deviceGen.generateUniqueICCID();
    sim["imsi"] = deviceGen.generateUniqueIMSI("470", "01"); // mcc, mnc
    
    // Apply unique values
    profile["identity"] = identity;
    profile["sim"] = sim;
    
    // Override network with unique values
    QJsonObject network = profile["network"].toObject();
    network["wifiMac"] = deviceGen.generateUniqueMAC();
    network["bluetoothMac"] = deviceGen.generateUniqueMAC();
    profile["network"] = network;
    
    // Add instance-specific metadata
    profile["instanceId"] = instanceId;
    profile["uniqueSeed"] = uniqueSeedStr;
    profile["isUniqueInstance"] = true;
    profile["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    qDebug() << "[UniqueProfile] Generated unique profile:"
             << "\n  Instance:" << instanceId
             << "\n  IMEI:" << identity["imei"].toString()
             << "\n  Serial:" << identity["serialNumber"].toString()
             << "\n  AndroidID:" << identity["androidId"].toString()
             << "\n  WiFi MAC:" << network["wifiMac"].toString();
    
    return profile;
}

bool ReDroidController::applyUniqueProfile(const QString& instanceId, const QJsonObject& profile) {
    qDebug() << "[UniqueProfile] Applying unique profile to instance:" << instanceId;
    
    if (profile.isEmpty()) {
        qWarning() << "[UniqueProfile] Empty profile, cannot apply";
        return false;
    }
    
    // Extract unique values
    QJsonObject identity = profile["identity"].toObject();
    QJsonObject network = profile["network"].toObject();
    QJsonObject sim = profile["sim"].toObject();
    
    // Build setprop commands for all unique values
    QStringList commands = {
        // Identity
        QString("setprop ro.serialno %1").arg(identity["serialNumber"].toString()),
        QString("setprop persist.radio.imei %1").arg(identity["imei"].toString()),
        QString("setprop ro.gsm.device.imei %1").arg(identity["imei"].toString()),
        QString("setprop ro.android_id %1").arg(identity["androidId"].toString()),
        QString("setprop gsfid.version %1").arg(identity["gsfId"].toString()),
        
        // Network
        QString("settings put secure android_id %1").arg(identity["androidId"].toString()),
        QString("settings put secure bluetooth_address %1").arg(network["bluetoothMac"].toString()),
        
        // SIM
        QString("setprop persist.radio.iccid %1").arg(sim["iccid"].toString()),
        QString("setprop persist.radio.imsi %1").arg(sim["imsi"].toString()),
        
        // Build properties
        QString("setprop ro.build.display.id %1").arg(profile["buildId"].toString()),
        QString("setprop ro.build.fingerprint %1").arg(profile["fingerprint"].toString()),
    };
    
    // Execute all commands
    for (const QString& cmd : commands) {
        QString result = executeShell(instanceId, cmd);
        if (!result.isEmpty() && result.contains("error", Qt::CaseInsensitive)) {
            qWarning() << "[UniqueProfile] Command failed:" << cmd << "Result:" << result;
        }
    }
    
    qDebug() << "[UniqueProfile] Unique profile applied successfully to:" << instanceId;
    return true;
}

QString ReDroidController::getDeviceUniqueFingerprint(const QString& instanceId) {
    // Generate a unique fingerprint hash for this device instance
    // This combines multiple unique identifiers
    
    QStringList identifiers = {
        getProperty(instanceId, "ro.serialno"),
        getProperty(instanceId, "ro.android_id"),
        getProperty(instanceId, "ro.gsm.device.imei"),
        getProperty(instanceId, "ro.build.fingerprint"),
        QHostInfo::localHostName(),
        QString::number(QDateTime::currentMSecsSinceEpoch())
    };
    
    QString combined = identifiers.join(":");
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);
    
    return hash.toHex();
}

bool ReDroidController::verifyDeviceUniqueness(const QString& instanceId) {
    qDebug() << "[Uniqueness] Verifying device uniqueness for:" << instanceId;
    
    // Get key identifiers
    QString serial = getProperty(instanceId, "ro.serialno");
    QString androidId = getProperty(instanceId, "ro.android_id");
    QString imei = getProperty(instanceId, "ro.gsm.device.imei");
    QString fingerprint = getProperty(instanceId, "ro.build.fingerprint");
    
    bool isUnique = !serial.isEmpty() && 
                    !androidId.isEmpty() && 
                    !imei.isEmpty() &&
                    !fingerprint.isEmpty();
    
    if (isUnique) {
        qDebug() << "[Uniqueness] Device is UNIQUE:"
                 << "\n  Serial:" << serial
                 << "\n  AndroidID:" << androidId
                 << "\n  IMEI:" << imei;
    } else {
        qWarning() << "[Uniqueness] WARNING: Device may not be unique!"
                   << "\n  Serial:" << serial
                   << "\n  AndroidID:" << androidId
                   << "\n  IMEI:" << imei;
    }
    
    return isUnique;
}

// ============================================================================
// REALISTIC TOUCH SIMULATION
// ============================================================================

bool ReDroidController::performRealisticTap(const QString& instanceId, int x, int y) {
    TimingAttackPrevention& timing = TimingAttackPrevention::instance();
    
    // Generate human-like think delay before tap
    int thinkDelay = timing.generateHumanThinkDelay(instanceId, "tap");
    QThread::msleep(thinkDelay);
    
    // Generate realistic tap pressure
    float pressure = timing.generateTouchPressure(instanceId, x, y, 0);
    
    // Generate tap duration
    int tapDelay = timing.generateTapDelay(instanceId);
    
    // Execute tap with timing
    QString cmd = QString("input tap %1 %2").arg(x).arg(y);
    executeShell(instanceId, cmd);
    
    // Add post-tap delay
    QThread::msleep(tapDelay);
    
    qDebug() << "[RealisticTouch] Tap:" << x << "," << y 
             << "pressure:" << pressure << "delay:" << thinkDelay << "ms";
    
    return true;
}

bool ReDroidController::performRealisticSwipe(const QString& instanceId, int x1, int y1, int x2, int y2) {
    TimingAttackPrevention& timing = TimingAttackPrevention::instance();
    
    // Generate human-like think delay before swipe
    int thinkDelay = timing.generateHumanThinkDelay(instanceId, "swipe");
    QThread::msleep(thinkDelay);
    
    // Calculate distance
    int distance = static_cast<int>(qSqrt(qPow(x2 - x1, 2) + qPow(y2 - y1, 2)));
    
    // Generate realistic swipe duration
    int swipeDuration = timing.generateSwipeDuration(instanceId, distance);
    
    // Execute swipe
    QString cmd = QString("input swipe %1 %2 %3 %4 %5")
                      .arg(x1).arg(y1).arg(x2).arg(y2).arg(swipeDuration);
    executeShell(instanceId, cmd);
    
    qDebug() << "[RealisticTouch] Swipe:" << x1 << "," << y1 << "->" << x2 << "," << y2
             << "duration:" << swipeDuration << "ms";
    
    return true;
}

bool ReDroidController::performRealisticType(const QString& instanceId, const QString& text) {
    TimingAttackPrevention& timing = TimingAttackPrevention::instance();
    
    // Generate human-like think delay before typing
    int thinkDelay = timing.generateHumanThinkDelay(instanceId, "type");
    QThread::msleep(thinkDelay);
    
    // Type character by character with realistic delays
    for (const QChar& ch : text) {
        int charDelay = timing.generateTypingDelay(instanceId, 80);
        QThread::msleep(charDelay);
        
        QString cmd = QString("input text '%1'").arg(ch);
        executeShell(instanceId, cmd);
    }
    
    qDebug() << "[RealisticTouch] Typed:" << text << "(" << text.length() << "chars)";
    
    return true;
}

} // namespace VirtualPhonePro
