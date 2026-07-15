/**
 * @file SensorInjector.h
 * @brief Advanced Sensor Spoofing with Gaussian Noise Injection
 * @version 2.0.0
 * 
 * Continuously pushes micro-movements to Android sensors using ADB.
 * Prevents static '0.0' values that indicate emulator detection.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_SENSOR_INJECTOR_H
#define VIRTUALPHONEPRO_SENSOR_INJECTOR_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QJsonObject>
#include <random>

namespace VirtualPhonePro {

// Sensor types
enum class SensorType {
    ACCELEROMETER,
    GYROSCOPE,
    MAGNETOMETER,
    PROXIMITY,
    LIGHT,
    PRESSURE,
    GRAVITY,
    LINEAR_ACCELERATION,
    ROTATION_VECTOR
};

// Sensor configuration
struct SensorConfig {
    SensorType type;
    bool enabled;
    bool useGaussianNoise;
    double baseValue[3];              // Base sensor values
    double noiseMean;                 // Gaussian noise mean
    double noiseStdDev;               // Gaussian noise standard deviation
    double updateIntervalMs;           // Update interval in milliseconds
    double range;                     // Sensor range
    double resolution;                // Sensor resolution
    QString resolutionFile;            // /sys/class/sensor/... path
};

// Sensor reading
struct SensorReading {
    SensorType type;
    qint64 timestamp;
    qint64 nanoseconds;
    double x, y, z;                   // Primary values
    double w;                         // For quaternions (rotation vector)
    bool valid;
};

// Movement pattern types
enum class MovementPattern {
    STATIONARY,       // Nearly still with micro-movements
    WALKING,          // Simulates walking motion
    IN_POCKET,        // Irregular movements
    ON_TABLE,         // Very subtle vibrations
    IN_CAR,           // Vehicle-like motion
    RANDOM            // Completely random
};

class SensorInjector : public QObject {
    Q_OBJECT

public:
    static SensorInjector& instance();
    
    // =========================================================================
    // Lifecycle
    // =========================================================================
    
    /**
     * @brief Start sensor injection for an instance
     */
    bool start(const QString& instanceId);
    
    /**
     * @brief Stop sensor injection
     */
    bool stop(const QString& instanceId);
    
    /**
     * @brief Stop all instances
     */
    void stopAll();
    
    /**
     * @brief Check if running
     */
    bool isRunning(const QString& instanceId) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure sensor for instance
     */
    void configureSensor(const QString& instanceId, const SensorConfig& config);
    
    /**
     * @brief Enable/disable sensor
     */
    void setSensorEnabled(const QString& instanceId, SensorType type, bool enabled);
    
    /**
     * @brief Set movement pattern
     */
    void setMovementPattern(const QString& instanceId, MovementPattern pattern);
    
    /**
     * @brief Set update rate
     */
    void setUpdateRate(const QString& instanceId, SensorType type, int intervalMs);
    
    // =========================================================================
    // Direct Injection
    // =========================================================================
    
    /**
     * @brief Inject single sensor reading
     */
    bool injectReading(const QString& instanceId, const SensorReading& reading);
    
    /**
     * @brief Inject accelerometer values
     */
    bool injectAccelerometer(const QString& instanceId, double x, double y, double z);
    
    /**
     * @brief Inject gyroscope values
     */
    bool injectGyroscope(const QString& instanceId, double x, double y, double z);
    
    /**
     * @brief Inject magnetometer values
     */
    bool injectMagnetometer(const QString& instanceId, double x, double y, double z);
    
    // =========================================================================
    // Status
    // =========================================================================
    
    /**
     * @brief Get current sensor state
     */
    QJsonObject getSensorState(const QString& instanceId) const;
    
    /**
     * @brief Get last readings
     */
    QVector<SensorReading> getLastReadings(const QString& instanceId) const;

signals:
    void sensorDataUpdated(const QString& instanceId, const SensorReading& reading);
    void error(const QString& instanceId, const QString& message);

private slots:
    void updateSensorData();
    void onTimerTimeout();

private:
    SensorInjector(QObject* parent = nullptr);
    ~SensorInjector();
    Q_DISABLE_COPY(SensorInjector)
    
    // Internal methods
    void initializeSensorSystem(const QString& instanceId);
    SensorReading generateReading(const QString& instanceId, SensorType type);
    double generateGaussianNoise(double mean, double stddev);
    double generateUniformNoise(double min, double max);
    
    // Movement simulation
    void updateMovementState(const QString& instanceId);
    void simulateWalkingMotion(const QString& instanceId, SensorReading& reading);
    void simulateStationaryMotion(const QString& instanceId, SensorReading& reading);
    void simulateInPocketMotion(const QString& instanceId, SensorReading& reading);
    void simulateOnTableMotion(const QString& instanceId, SensorReading& reading);
    void simulateInCarMotion(const QString& instanceId, SensorReading& reading);
    
    // ADB injection
    bool injectViaADB(const QString& instanceId, const SensorReading& reading);
    
    // State management
    struct InstanceState {
        bool isRunning;
        MovementPattern pattern;
        QMap<SensorType, SensorConfig> sensors;
        QVector<SensorReading> recentReadings;
        QTimer* updateTimer;
        QThread* workerThread;
        
        // Movement state
        double accumulatedX, accumulatedY, accumulatedZ;
        qint64 lastUpdateTime;
        int stepCount;
        double walkPhase;
    };
    
    QMap<QString, InstanceState*> m_instances;
    mutable QMutex m_mutex;
    
    // Random number generation
    std::mt19937 m_generator;
    std::normal_distribution<double> m_gaussianDist;
    std::uniform_real_distribution<double> m_uniformDist;
    
    // Configuration for different patterns
    static constexpr double WALK_ACCEL_AMPLITUDE = 2.5;
    static constexpr double WALK_GYRO_AMPLITUDE = 1.5;
    static constexpr double STATIONARY_NOISE_STDDEV = 0.01;
    static constexpr double TABLE_VIBRATION_STDDEV = 0.002;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SENSOR_INJECTOR_H
