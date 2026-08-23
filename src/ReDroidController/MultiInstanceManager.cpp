// Windows headers MUST be the very first include to prevent Qt macro conflicts
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

// Undef Windows macros that conflict with our variable names
#ifdef HOME
#undef HOME
#endif
#ifdef BACK
#undef BACK
#endif
#ifdef DELETE
#undef DELETE
#endif
#ifdef ENTER
#undef ENTER
#endif
#ifdef SPACE
#undef SPACE
#endif
#endif

#include "VirtualPhonePro/MultiInstanceManager.hpp"

#include "VirtualPhonePro/ReDroidController.hpp"
#include "VirtualPhonePro/ProfileGeneratorFactory.hpp"

#include <QThread>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTcpServer>
#include <QHostAddress>
#include <cstring>

namespace VirtualPhonePro {

MultiInstanceManager& MultiInstanceManager::instance() {
    static MultiInstanceManager s_instance;
    return s_instance;
}

namespace {

// System memory in MB, or -1 when undetectable (callers then skip the
// check rather than guessing). Cross-platform: /proc/meminfo on Linux,
// GlobalMemoryStatusEx on Windows.
qint64 readMeminfoMB(const char* key) {
#ifdef _WIN32
    MEMORYSTATUSEX memStatus{};
    memStatus.dwLength = sizeof(memStatus);
    if (!GlobalMemoryStatusEx(&memStatus))
        return -1;
    if (std::strcmp(key, "MemTotal") == 0)
        return static_cast<qint64>(memStatus.ullTotalPhys / (1024 * 1024));
    return static_cast<qint64>(memStatus.ullAvailPhys / (1024 * 1024));
#else
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray content = f.readAll();
    const QList<QByteArray> lines = content.split('\n');
    for (const QByteArray& line : lines) {
        if (line.startsWith(key)) {
            // Format: "MemAvailable:   12345678 kB"
            const QList<QByteArray> parts = line.simplified().split(' ');
            if (parts.size() >= 2)
                return parts.at(1).toLongLong() / 1024;  // kB -> MB
        }
    }
    return -1;
#endif
}

qint64 systemTotalMemoryMB()     { return readMeminfoMB("MemTotal"); }
qint64 systemAvailableMemoryMB() { return readMeminfoMB("MemAvailable"); }

// Socket-level probe: true when nothing on this host is bound to the port.
bool isHostPortFree(int port) {
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, static_cast<quint16>(port)))
        return false;
    probe.close();
    return true;
}

} // namespace

MultiInstanceManager::MultiInstanceManager(QObject* parent)
    : QObject(parent)
    , m_maxConcurrentInstances(10)
    , m_maxMemoryPerInstance(1536)  // Synced with ReDroidController default
    , m_nextAvailablePort(5555)
{
    m_threadPool.setMaxThreadCount(m_maxConcurrentInstances);
}

std::optional<DeviceProfile> MultiInstanceManager::cloneProfile(const DeviceProfile& base,
                                               const QString& instanceId, int index) {
    DeviceProfile profile = base;

    // Generate unique ID
    profile.id = QUuid::createUuid().toString();
    profile.name = QString("%1 #%2").arg(base.name).arg(index + 1);
    profile.instanceIndex = index;

    // Identity comes from the same hardware-anchored deterministic engine
    // the GUI single-create path uses (Master_Seed = HMAC-SHA256(HWID +
    // License_Key, "PROFILE_" + Index)). Each instance consumes a fresh
    // persisted profile index, so seeds never overlap — uniqueness is
    // deterministic, not probabilistic. No process-random sources.
    const HardwareAnchoredIdentity identity = generateUniqueHardwareAnchoredIdentity();
    if (!identity.ok) {
        qCritical() << "cloneProfile: could not allocate a unique deterministic"
                       " identity for" << instanceId
                    << "(profile-index space exhausted or persistence failure)";
        return std::nullopt;
    }
    applyIdentityToDeviceProfile(profile, identity.identity);

    // The engine derives no ethernet MAC, but the field is consumed per
    // container (NET_ETHERNET_MAC), so sharing the base value across clones
    // would duplicate it. Derive it deterministically from engine material:
    // first 6 bytes of the device key, forced locally-administered unicast.
    const QByteArray keyBytes = QByteArray::fromHex(
        QByteArray::fromStdString(identity.identity.device_key));
    if (keyBytes.size() >= 6) {
        QByteArray mac = keyBytes.left(6);
        mac[0] = static_cast<char>((static_cast<quint8>(mac[0]) | 0x02) & 0xFE);
        profile.mac.ethernetMac = QString::fromLatin1(mac.toHex(':')).toUpper();
    }

    // Record the issued identity so future allocations (GUI or batch) can
    // detect collisions against this instance.
    registerIssuedIdentity(instanceId, profile);

    return profile;
}

QString MultiInstanceManager::generateUniqueId(const QString& prefix, int index) {
    return QString("%1-%2").arg(prefix).arg(index + 1, 3, 10, QChar('0'));
}

void MultiInstanceManager::updateResourceTracking(const QString& instanceId, bool added) {
    QMutexLocker locker(&m_mutex);
    
    if (added) {
        m_deployedInstances.insert(instanceId);
    } else {
        m_deployedInstances.remove(instanceId);
    }
}

BatchStatus MultiInstanceManager::deployBatch(const InstanceDeployConfig& config) {
    BatchStatus status;
    status.total = config.count;

    // Sweep containers left behind by crashed sessions before judging
    // capacity — stale vpp-* containers otherwise hold ports and names.
    const int swept = cleanupStaleContainers();
    if (swept > 0)
        qDebug() << "deployBatch: removed" << swept << "stale container(s)";

    if (!canDeploy(config.count)) {
        const QString msg = QStringLiteral(
            "Resource limit reached: %1 concurrent-instance cap or "
            "insufficient free RAM for %2 new instance(s).")
            .arg(m_maxConcurrentInstances).arg(config.count);
        emit resourceWarning(msg);
        status.failed = config.count;
        status.errors.append(msg);
        return status;
    }

    // RAM overcommit guard with an explicit, actionable error (canDeploy()
    // above only returns a bool).
    const qint64 freeMB = systemAvailableMemoryMB();
    const qint64 needMB = static_cast<qint64>(config.count) * m_maxMemoryPerInstance;
    if (freeMB >= 0 && freeMB < needMB) {
        const QString msg = QStringLiteral(
            "Not enough free RAM: need %1 MB for %2 instance(s) "
            "(%3 MB each) but only %4 MB available.")
            .arg(needMB).arg(config.count)
            .arg(m_maxMemoryPerInstance).arg(freeMB);
        qCritical() << "deployBatch:" << msg;
        emit resourceWarning(msg);
        status.failed = config.count;
        status.errors.append(msg);
        return status;
    }

    // Pre-flight the requested port range when the caller pins ports:
    // a busy port now would otherwise surface as an obscure docker
    // "port is already allocated" failure mid-launch.
    if (config.assignUniquePort) {
        for (int i = 0; i < config.count; ++i) {
            const int port = config.portStart + (i * 2);
            if (!isHostPortFree(port)) {
                const QString msg = QStringLiteral(
                    "Requested ADB port %1 is already in use on the host.")
                    .arg(port);
                qCritical() << "deployBatch:" << msg;
                emit resourceWarning(msg);
                status.failed = config.count;
                status.errors.append(msg);
                return status;
            }
        }
    }

    qDebug() << "Batch deploy:" << config.count << "instances (parallel)";
    const QString batchId = QUuid::createUuid().toString();

    // ── 1. Pre-allocate instanceId + profile list ─────────────────────────────
    // Do this synchronously so every instance has a unique identity before any
    // container starts. cloneProfile() derives the full identity from the
    // hardware-anchored deterministic engine (same as the GUI single-create
    // path) and consumes one persisted profile index per instance.
    struct LaunchItem {
        QString       instanceId;
        DeviceProfile profile;
    };
    QList<LaunchItem> items;
    items.reserve(config.count);

    for (int i = 0; i < config.count; ++i) {
        LaunchItem item;
        item.instanceId = generateUniqueId(config.profilePrefix, i);
        std::optional<DeviceProfile> profile =
            cloneProfile(config.baseProfile, item.instanceId, i);
        if (!profile) {
            const QString msg = QStringLiteral(
                "Could not allocate a unique deterministic identity for %1 "
                "(profile-index space exhausted or persistence failure). "
                "Aborting batch before any container starts.")
                .arg(item.instanceId);
            qCritical() << "deployBatch:" << msg;
            emit resourceWarning(msg);
            status.failed = config.count;
            status.errors.append(msg);
            return status;
        }
        item.profile = std::move(*profile);
        if (config.assignUniquePort)
            item.profile.adbPort = config.portStart + (i * 2);
        items.append(item);
    }

    // ── 2. Launch all containers in parallel ──────────────────────────────────
    // startInstance() is thread-safe (internal QMutexLocker on m_instancesMutex,
    // socket-level port probe, binderfs isolation per container).
    // applyCompleteRealism() pins every spoofing module to the instance's own
    // adb serial (per-instance ADBManager), so parallel pipelines cannot leak
    // commands across containers.
    QMutex           resultMutex;
    std::atomic<int> completed{0};
    std::atomic<int> failed{0};

    ReDroidController& ctrl = ReDroidController::instance();

    QList<QFuture<void>> futures;
    futures.reserve(items.size());

    for (const LaunchItem& item : items) {
        QFuture<void> f = QtConcurrent::run([&, item]() {
            bool ok = ctrl.startInstance(item.instanceId, item.profile);

            {
                QMutexLocker lk(&resultMutex);
                status.instanceResults[item.instanceId] = ok;
                if (ok) {
                    updateResourceTracking(item.instanceId, true);
                } else {
                    ++failed;
                    status.errors.append(
                        QString("Failed to deploy %1").arg(item.instanceId));
                }
            }

            int done = ++completed;
            emit batchProgress(batchId, done, config.count);
        });
        futures.append(f);

        // Optional stagger — reduces simultaneous Docker API load.
        // config.delayBetween == 0 means fully parallel (no stagger).
        if (config.delayBetween > 0 && &item != &items.last())
            QThread::msleep(config.delayBetween);
    }

    // ── 3. Wait for all futures ───────────────────────────────────────────────
    for (QFuture<void>& f : futures)
        f.waitForFinished();

    status.completed = completed.load();
    status.failed    = failed.load();
    status.running   = status.completed - status.failed;

    qDebug() << "Batch deploy complete:"
             << status.completed - status.failed << "OK,"
             << status.failed << "failed";

    emit batchCompleted(batchId, status);
    return status;
}

BatchStatus MultiInstanceManager::startBatch(const QStringList& instanceIds) {
    BatchStatus status;
    status.total = instanceIds.size();
    
    qDebug() << "Starting batch start of" << instanceIds.size() << "instances";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    for (const QString& instanceId : instanceIds) {
        // Load profile
        QString profilePath = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
        profilePath += "/profiles/" + ctrl.getInstanceInfo(instanceId).profileId + ".json";
        DeviceProfile profile = DeviceProfile::load(profilePath);
        
        if (profile.id.isEmpty()) {
            profile = DeviceProfile::createSamsungS24Ultra();
        }
        
        bool success = ctrl.startInstance(instanceId, profile);
        
        status.completed++;
        status.instanceResults[instanceId] = success;
        
        if (!success) {
            status.failed++;
            status.errors.append(QString("Failed to start %1").arg(instanceId));
        }
        
        emit instanceStateChanged(instanceId, InstanceState::Running);
    }
    
    return status;
}

BatchStatus MultiInstanceManager::stopBatch(const QStringList& instanceIds) {
    BatchStatus status;
    status.total = instanceIds.size();
    
    qDebug() << "Stopping batch of" << instanceIds.size() << "instances";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    for (const QString& instanceId : instanceIds) {
        bool success = ctrl.stopInstance(instanceId);
        
        status.completed++;
        status.instanceResults[instanceId] = success;
        
        if (!success) {
            status.failed++;
            status.errors.append(QString("Failed to stop %1").arg(instanceId));
        }
        
        emit instanceStateChanged(instanceId, InstanceState::Stopped);
    }
    
    return status;
}

BatchStatus MultiInstanceManager::deleteBatch(const QStringList& instanceIds) {
    BatchStatus status;
    status.total = instanceIds.size();
    
    qDebug() << "Deleting batch of" << instanceIds.size() << "instances";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    for (const QString& instanceId : instanceIds) {
        bool success = ctrl.deleteInstance(instanceId);
        
        status.completed++;
        status.instanceResults[instanceId] = success;
        
        if (success) {
            updateResourceTracking(instanceId, false);
        } else {
            status.failed++;
            status.errors.append(QString("Failed to delete %1").arg(instanceId));
        }
    }
    
    return status;
}

BatchStatus MultiInstanceManager::restartBatch(const QStringList& instanceIds) {
    BatchStatus status;
    status.total = instanceIds.size();
    
    qDebug() << "Restarting batch of" << instanceIds.size() << "instances";
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    for (const QString& instanceId : instanceIds) {
        bool success = ctrl.restartInstance(instanceId);
        
        status.completed++;
        status.instanceResults[instanceId] = success;
        
        if (!success) {
            status.failed++;
            status.errors.append(QString("Failed to restart %1").arg(instanceId));
        }
        
        emit instanceStateChanged(instanceId, InstanceState::Running);
    }
    
    return status;
}

// ============================================================================
// Instance Group Management
// ============================================================================

bool MultiInstanceManager::createGroup(const QString& groupName, const QStringList& instanceIds) {
    QMutexLocker locker(&m_mutex);
    
    if (m_groups.contains(groupName)) {
        qWarning() << "Group already exists:" << groupName;
        return false;
    }
    
    m_groups[groupName] = instanceIds;
    qDebug() << "Created group" << groupName << "with" << instanceIds.size() << "instances";
    
    return true;
}

bool MultiInstanceManager::deleteGroup(const QString& groupName) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_groups.contains(groupName)) {
        return false;
    }
    
    m_groups.remove(groupName);
    qDebug() << "Deleted group:" << groupName;
    
    return true;
}

QStringList MultiInstanceManager::getGroupInstances(const QString& groupName) const {
    return m_groups.value(groupName);
}

BatchStatus MultiInstanceManager::startGroup(const QString& groupName) {
    QStringList instances = getGroupInstances(groupName);
    if (instances.isEmpty()) {
        return BatchStatus{0, 0, 0, 0, {"Group not found or empty"}, {}};
    }
    return startBatch(instances);
}

BatchStatus MultiInstanceManager::stopGroup(const QString& groupName) {
    QStringList instances = getGroupInstances(groupName);
    if (instances.isEmpty()) {
        return BatchStatus{0, 0, 0, 0, {"Group not found or empty"}, {}};
    }
    return stopBatch(instances);
}

QStringList MultiInstanceManager::getAllGroups() const {
    return m_groups.keys();
}

// ============================================================================
// Resource Management
// ============================================================================

QMap<QString, int> MultiInstanceManager::getResourceSummary() const {
    QMap<QString, int> summary;
    
    ReDroidController& ctrl = ReDroidController::instance();
    QList<InstanceInfo> instances = ctrl.listInstances();
    
    summary["total"] = instances.size();
    summary["running"] = 0;
    summary["stopped"] = 0;
    summary["error"] = 0;
    
    quint64 totalMemory = 0;
    quint64 usedMemory = 0;
    
    for (const InstanceInfo& info : instances) {
        switch (info.state) {
            case InstanceState::Running:
                summary["running"]++;
                usedMemory += info.memoryUsage;
                break;
            case InstanceState::Stopped:
                summary["stopped"]++;
                break;
            case InstanceState::Error:
                summary["error"]++;
                break;
            default:
                break;
        }
        totalMemory += info.memoryLimit;
    }
    
    summary["totalMemoryMB"] = totalMemory / (1024 * 1024);
    summary["usedMemoryMB"] = usedMemory / (1024 * 1024);
    summary["availableSlots"] = m_maxConcurrentInstances - summary["running"];
    
    return summary;
}

bool MultiInstanceManager::canDeploy(int count) const {
    QMutexLocker locker(&m_mutex);

    int currentRunning = 0;
    ReDroidController& ctrl = ReDroidController::instance();

    for (const InstanceInfo& info : ctrl.listInstances()) {
        if (info.state == InstanceState::Running) {
            currentRunning++;
        }
    }

    if (currentRunning + count > m_maxConcurrentInstances)
        return false;

    // RAM overcommit guard: refuse when the host clearly cannot fit the
    // requested instances. Skipped when free RAM is undetectable (-1).
    const qint64 freeMB = systemAvailableMemoryMB();
    if (freeMB >= 0 &&
        freeMB < static_cast<qint64>(count) * m_maxMemoryPerInstance)
        return false;

    return true;
}

int MultiInstanceManager::getRecommendedMaxInstances() const {
    int cpuCores = QThread::idealThreadCount();
    if (cpuCores <= 0) cpuCores = 4;

    // Cross-platform RAM detection (-1 = unknown -> conservative fallback).
    const qint64 totalRAM = systemTotalMemoryMB();
    qint64 availableRAM;
    if (totalRAM > 0) {
        // Reserve 3GB for host OS + Docker daemon + this GUI.
        availableRAM = qMax<qint64>(0, totalRAM - 3072);
    } else {
        availableRAM = 4096;
    }

    const int perInstanceMB = qMax(1, m_maxMemoryPerInstance);
    const int maxByCPU = qMax(1, cpuCores / 2);
    const int maxByRAM = static_cast<int>(availableRAM / perInstanceMB);

    // At least 1 (a single instance is always worth offering), at most the
    // configured hard limit.
    return qBound(1, qMin(maxByCPU, maxByRAM), m_maxConcurrentInstances);
}

void MultiInstanceManager::setResourceLimits(int maxInstances, int maxMemoryPerInstance) {
    QMutexLocker locker(&m_mutex);
    
    m_maxConcurrentInstances = maxInstances;
    m_maxMemoryPerInstance = maxMemoryPerInstance;
    
    m_threadPool.setMaxThreadCount(maxInstances);
    
    qDebug() << "Resource limits set - Max instances:" << maxInstances 
             << "Max memory/instance:" << maxMemoryPerInstance << "MB";
}

// ============================================================================
// Query Methods
// ============================================================================

QStringList MultiInstanceManager::getAllInstances() const {
    QStringList instances;
    
    ReDroidController& ctrl = ReDroidController::instance();
    for (const InstanceInfo& info : ctrl.listInstances()) {
        instances.append(info.instanceId);
    }
    
    return instances;
}

int MultiInstanceManager::getInstanceCount() const {
    ReDroidController& ctrl = ReDroidController::instance();
    return ctrl.listInstances().size();
}

int MultiInstanceManager::getRunningCount() const {
    int count = 0;
    
    ReDroidController& ctrl = ReDroidController::instance();
    for (const InstanceInfo& info : ctrl.listInstances()) {
        if (info.state == InstanceState::Running) {
            count++;
        }
    }
    
    return count;
}

QStringList MultiInstanceManager::getInstancesByState(InstanceState state) const {
    QStringList instances;
    
    ReDroidController& ctrl = ReDroidController::instance();
    for (const InstanceInfo& info : ctrl.listInstances()) {
        if (info.state == state) {
            instances.append(info.instanceId);
        }
    }
    
    return instances;
}

int MultiInstanceManager::findAvailablePort() {
    QMutexLocker locker(&m_mutex);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    while (true) {
        bool used = false;
        
        for (const InstanceInfo& info : ctrl.listInstances()) {
            if (info.adbPort == m_nextAvailablePort) {
                used = true;
                break;
            }
        }
        
        if (!used) {
            int port = m_nextAvailablePort;
            m_nextAvailablePort += 2;
            return port;
        }
        
        m_nextAvailablePort += 2;
        
        if (m_nextAvailablePort > 65535) {
            m_nextAvailablePort = 5555;
        }
    }
}

int MultiInstanceManager::cleanupStaleContainers() {
    // List vpp-* containers in any state. Short timeout — a wedged docker
    // daemon must not block the launch path.
    QProcess ps;
    ps.start(QStringLiteral("docker"),
             {QStringLiteral("ps"), QStringLiteral("-a"),
              QStringLiteral("--filter"), QStringLiteral("name=vpp-"),
              QStringLiteral("--format"), QStringLiteral("{{.Names}} {{.State}}")});
    if (!ps.waitForFinished(10000)) {
        ps.kill();
        ps.waitForFinished(2000);
        qWarning() << "cleanupStaleContainers: 'docker ps' timed out — skipping sweep";
        return 0;
    }

    // Instance IDs that must never be removed.
    QSet<QString> active;
    for (const InstanceInfo& info : ReDroidController::instance().listInstances()) {
        if (info.state == InstanceState::Starting ||
            info.state == InstanceState::Running  ||
            info.state == InstanceState::Paused) {
            active.insert(info.instanceId);
        }
    }

    int removed = 0;
    const QString output = QString::fromUtf8(ps.readAllStandardOutput());
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList parts = line.simplified().split(QLatin1Char(' '));
        if (parts.size() < 2)
            continue;
        const QString name  = parts.at(0);             // e.g. "vpp-<instanceId>"
        const QString state = parts.at(1).toLower();   // running/exited/dead/...
        const QString id    = name.mid(4);             // strip "vpp-"

        // Stale = not running, or running but unknown to us (orphaned).
        const bool orphanRunning = (state == QStringLiteral("running")) && !active.contains(id);
        const bool deadContainer = (state == QStringLiteral("exited") ||
                                    state == QStringLiteral("dead")   ||
                                    state == QStringLiteral("created"));
        if (active.contains(id) || (!orphanRunning && !deadContainer))
            continue;

        qDebug() << "cleanupStaleContainers: removing stale container" << name
                 << "(state:" << state << ")";
        QProcess rm;
        rm.start(QStringLiteral("docker"),
                 {QStringLiteral("rm"), QStringLiteral("-f"), name});
        if (!rm.waitForFinished(10000)) {
            rm.kill();
            rm.waitForFinished(2000);
            qWarning() << "cleanupStaleContainers: timed out removing" << name;
            continue;
        }
        if (rm.exitCode() == 0)
            ++removed;
        else
            qWarning() << "cleanupStaleContainers: failed to remove" << name
                       << ":" << QString::fromUtf8(rm.readAllStandardError()).trimmed();
    }
    return removed;
}

} // namespace VirtualPhonePro
