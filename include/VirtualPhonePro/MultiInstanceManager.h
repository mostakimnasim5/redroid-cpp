#pragma once

#ifndef VIRTUALPHONEPRO_MULTI_INSTANCE_MANAGER_H
#define VIRTUALPHONEPRO_MULTI_INSTANCE_MANAGER_H

#include "VirtualPhonePro/DeviceProfile.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QList>
#include <QSet>
#include <QQueue>
#include <QMutex>
#include <QSemaphore>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent>

namespace VirtualPhonePro {

/**
 * @brief Configuration for multi-instance deployment
 */
struct InstanceDeployConfig {
    int count;                      // Number of instances to deploy
    int maxConcurrent;             // Max concurrent deployments
    int delayBetween;              // Delay between deployments (ms)
    QString profilePrefix;          // Profile name prefix
    DeviceProfile baseProfile;      // Base profile to clone
    bool assignUniqueIdentity;      // Generate unique IMEI/MAC per instance
    bool assignUniqueIP;           // Assign unique IP per instance
    bool assignUniquePort;         // Assign unique ADB port
    int portStart;                 // Starting port
    QString networkSubnet;          // Network subnet for instances
};

/**
 * @brief Status of a batch operation
 */
struct BatchStatus {
    int total;
    int completed;
    int failed;
    int running;
    QStringList errors;
    QMap<QString, bool> instanceResults;  // instanceId -> success
};

/**
 * @brief MultiInstanceManager - Manages multiple ReDroid instances
 * 
 * Handles deployment, orchestration, and lifecycle of 10+ emulator instances
 * with resource management and load balancing.
 */
class MultiInstanceManager : public QObject {
    Q_OBJECT

public:
    static MultiInstanceManager& instance();
    
    // =========================================================================
    // Batch Operations
    // =========================================================================
    
    /**
     * @brief Deploy multiple instances in batch
     * @param config Deployment configuration
     * @return BatchStatus with results
     */
    BatchStatus deployBatch(const InstanceDeployConfig& config);
    
    /**
     * @brief Start multiple instances
     * @param instanceIds List of instance IDs
     * @return BatchStatus with results
     */
    BatchStatus startBatch(const QStringList& instanceIds);
    
    /**
     * @brief Stop multiple instances
     * @param instanceIds List of instance IDs
     * @return BatchStatus with results
     */
    BatchStatus stopBatch(const QStringList& instanceIds);
    
    /**
     * @brief Delete multiple instances
     * @param instanceIds List of instance IDs
     * @return BatchStatus with results
     */
    BatchStatus deleteBatch(const QStringList& instanceIds);
    
    /**
     * @brief Restart multiple instances
     * @param instanceIds List of instance IDs
     * @return BatchStatus with results
     */
    BatchStatus restartBatch(const QStringList& instanceIds);
    
    // =========================================================================
    // Instance Group Management
    // =========================================================================
    
    /**
     * @brief Create instance group
     * @param groupName Name of the group
     * @param instanceIds Instances to include
     * @return true if created
     */
    bool createGroup(const QString& groupName, const QStringList& instanceIds);
    
    /**
     * @brief Delete instance group
     * @param groupName Name of the group
     * @return true if deleted
     */
    bool deleteGroup(const QString& groupName);
    
    /**
     * @brief Get instances in group
     * @param groupName Name of the group
     * @return List of instance IDs
     */
    QStringList getGroupInstances(const QString& groupName) const;
    
    /**
     * @brief Start all instances in group
     * @param groupName Name of the group
     * @return BatchStatus
     */
    BatchStatus startGroup(const QString& groupName);
    
    /**
     * @brief Stop all instances in group
     * @param groupName Name of the group
     * @return BatchStatus
     */
    BatchStatus stopGroup(const QString& groupName);
    
    /**
     * @brief Get all groups
     * @return List of group names
     */
    QStringList getAllGroups() const;
    
    // =========================================================================
    // Resource Management
    // =========================================================================
    
    /**
     * @brief Get resource usage summary
     * @return Resource usage map
     */
    QMap<QString, int> getResourceSummary() const;
    
    /**
     * @brief Check if resources available for more instances
     * @param count Number of instances to check
     * @return true if can deploy
     */
    bool canDeploy(int count) const;
    
    /**
     * @brief Get recommended max instances based on system resources
     * @return Recommended max instance count
     */
    int getRecommendedMaxInstances() const;
    
    /**
     * @brief Set resource limits
     * @param maxInstances Maximum concurrent instances
     * @param maxMemoryPerInstance Memory limit per instance (MB)
     */
    void setResourceLimits(int maxInstances, int maxMemoryPerInstance);
    
    // =========================================================================
    // Query Methods
    // =========================================================================
    
    /**
     * @brief Get all instances across all groups
     * @return Complete list of instance IDs
     */
    QStringList getAllInstances() const;
    
    /**
     * @brief Get instance count
     * @return Total number of instances
     */
    int getInstanceCount() const;
    
    /**
     * @brief Get running instance count
     * @return Number of running instances
     */
    int getRunningCount() const;
    
    /**
     * @brief Get instances by state
     * @param state State to filter by
     * @return List of matching instance IDs
     */
    QStringList getInstancesByState(InstanceState state) const;
    
    /**
     * @brief Find available port for new instance
     * @return Available port number
     */
    int findAvailablePort() const;
    
signals:
    /**
     * @brief Emitted when batch progress updates
     */
    void batchProgress(const QString& batchId, int current, int total);
    
    /**
     * @brief Emitted when instance state changes in batch
     */
    void batchInstanceStateChanged(const QString& instanceId, InstanceState state);
    
    /**
     * @brief Emitted when batch completes
     */
    void batchCompleted(const QString& batchId, const BatchStatus& status);
    
    /**
     * @brief Emitted when resource warning occurs
     */
    void resourceWarning(const QString& message);

private:
    explicit MultiInstanceManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(MultiInstanceManager)
    
    // Internal helpers
    DeviceProfile cloneProfile(const DeviceProfile& base, const QString& instanceId, int index);
    QString generateUniqueId(const QString& prefix, int index);
    void updateResourceTracking(const QString& instanceId, bool added);
    
    // Resource limits
    int m_maxConcurrentInstances;
    int m_maxMemoryPerInstance;
    
    // Thread pool for parallel operations
    QThreadPool m_threadPool;
    
    // Groups
    QMap<QString, QStringList> m_groups;
    
    // Resource tracking
    QSet<QString> m_deployedInstances;
    QMutex m_mutex;
    
    // Port allocation
    int m_nextAvailablePort;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_MULTI_INSTANCE_MANAGER_H
