#include "VirtualPhonePro/MultiInstanceManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QThread>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QCoreApplication>

namespace VirtualPhonePro {

MultiInstanceManager* MultiInstanceManager::s_instance = nullptr;

MultiInstanceManager& MultiInstanceManager::instance() {
    if (!s_instance) {
        s_instance = new MultiInstanceManager();
    }
    return *s_instance;
}

MultiInstanceManager::MultiInstanceManager(QObject* parent)
    : QObject(parent)
    , m_maxConcurrentInstances(10)
    , m_maxMemoryPerInstance(768)
    , m_nextAvailablePort(5555)
{
    m_threadPool.setMaxThreadCount(m_maxConcurrentInstances);
}

DeviceProfile MultiInstanceManager::cloneProfile(const DeviceProfile& base, 
                                               const QString& instanceId, int index) {
    DeviceProfile profile = base;
    
    // Generate unique ID
    profile.id = QUuid::createUuid().toString();
    profile.name = QString("%1 #%2").arg(base.name).arg(index + 1);
    profile.instanceIndex = index;
    
    if (m_maxConcurrentInstances.assignUniqueIdentity) {
        // Generate unique IMEI
        QString tac = base.identity.imei.left(8);
        profile.identity.imei = DeviceProfile::generateIMEI(tac);
        profile.identity.imei2 = DeviceProfile::generateIMEI(tac);
        profile.identity.serialNumber = DeviceProfile::generateSerial(base.manufacturer);
        profile.identity.androidId = DeviceProfile::generateAndroidId();
        profile.identity.gsfId = QString::number(QRandomGenerator::global()->bounded(1000000000, 9999999999));
        
        // Generate unique MAC addresses
        profile.mac.wifiMac = DeviceProfile::generateMAC("8C:71:F8");
        profile.mac.bluetoothMac = DeviceProfile::generateMAC("94:EB:2C");
        profile.mac.ethernetMac = DeviceProfile::generateMAC("00:1A:11");
    }
    
    if (m_maxConcurrentInstances.assignUniqueIP) {
        // Unique IP would be assigned via Docker network
    }
    
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
    
    qDebug() << "Starting batch deployment of" << config.count << "instances";
    
    QString batchId = QUuid::createUuid().toString();
    
    for (int i = 0; i < config.count; ++i) {
        if (!canDeploy(1)) {
            qWarning() << "Cannot deploy more instances - resource limit reached";
            emit resourceWarning("Resource limit reached. Cannot deploy more instances.");
            break;
        }
        
        QString instanceId = generateUniqueId(config.profilePrefix, i);
        
        // Clone profile with unique identity
        DeviceProfile profile = cloneProfile(config.baseProfile, instanceId, i);
        
        // Set unique ports
        if (config.assignUniquePort) {
            profile.adbPort = config.portStart + (i * 2);
            profile.vncPort = config.portStart + (i * 2) + 1;
        }
        
        qDebug() << "Deploying instance" << i + 1 << "/" << config.count << ":" << instanceId;
        
        // Start instance
        ReDroidController& ctrl = ReDroidController::instance();
        bool success = ctrl.startInstance(instanceId, profile);
        
        status.completed++;
        status.instanceResults[instanceId] = success;
        
        if (success) {
            updateResourceTracking(instanceId, true);
        } else {
            status.failed++;
            status.errors.append(QString("Failed to deploy %1: Unknown error").arg(instanceId));
        }
        
        emit batchProgress(batchId, i + 1, config.count);
        
        // Delay between deployments
        if (config.delayBetween > 0 && i < config.count - 1) {
            QThread::msleep(config.delayBetween);
        }
    }
    
    status.running = status.total - status.failed;
    
    qDebug() << "Batch deployment complete:" << status.completed << "succeeded," 
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
    
    return (currentRunning + count <= m_maxConcurrentInstances);
}

int MultiInstanceManager::getRecommendedMaxInstances() const {
    // Simple heuristic based on typical system
    // In production, would check actual CPU cores and memory
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    int cpuCores = sysInfo.dwNumberOfProcessors;
    
    // Assume 2 cores per instance, 1GB RAM per instance
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    
    quint64 totalRAM = memInfo.ullTotalPhys / (1024 * 1024);  // MB
    
    int maxByCPU = cpuCores / 2;
    int maxByRAM = totalRAM / 1024;
    
    return qMin(maxByCPU, maxByRAM);
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

int MultiInstanceManager::findAvailablePort() const {
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

} // namespace VirtualPhonePro
