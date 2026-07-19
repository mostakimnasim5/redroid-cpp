/**
 * @file SensorInjector.cpp
 * @brief Advanced Sensor Spoofing Implementation
 */

#include "Android/SensorInjector.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>
#include <QtMath>
#include <QJsonObject>

namespace VirtualPhonePro {

SensorInjector* SensorInjector::s_instance = nullptr;

SensorInjector& SensorInjector::instance() {
    if (!s_instance) {
        s_instance = new SensorInjector();
    }
    return *s_instance;
}

SensorInjector::SensorInjector(QObject* parent)
    : QObject(parent)
    , m_generator(QRandomGenerator::global()->generate())
{
    // Initialize with default distributions
    m_gaussianDist = std::normal_distribution<double>(0.0, STATIONARY_NOISE_STDDEV);
    m_uniformDist = std::uniform_real_distribution<double>(-0.1, 0.1);
}

SensorInjector::~SensorInjector() {
    stopAll();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool SensorInjector::start(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    if (m_instances.contains(instanceId)) {
        qDebug() << "SensorInjector already running for:" << instanceId;
        return true;
    }
    
    qDebug() << "Starting SensorInjector for:" << instanceId;
    
    InstanceState* state = new InstanceState();
    state->isRunning = true;
    state->pattern = MovementPattern::STATIONARY;
    state->accumulatedX = 0;
    state->accumulatedY = 0;
    state->accumulatedZ = 0;
    state->lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    state->stepCount = 0;
    state->walkPhase = 0;
    
    // Configure default sensors
    SensorConfig accelConfig;
    accelConfig.type = SensorType::ACCELEROMETER;
    accelConfig.enabled = true;
    accelConfig.useGaussianNoise = true;
    accelConfig.baseValue[0] = 0.0;
    accelConfig.baseValue[1] = 0.0;
    accelConfig.baseValue[2] = 9.81;  // Gravity
    accelConfig.noiseMean = 0.0;
    accelConfig.noiseStdDev = STATIONARY_NOISE_STDDEV;
    accelConfig.updateIntervalMs = 50;  // 20Hz
    accelConfig.range = 39.2266;  // ±2g typical
    accelConfig.resolution = 0.0012;
    state->sensors[SensorType::ACCELEROMETER] = accelConfig;
    
    SensorConfig gyroConfig;
    gyroConfig.type = SensorType::GYROSCOPE;
    gyroConfig.enabled = true;
    gyroConfig.useGaussianNoise = true;
    gyroConfig.baseValue[0] = 0.0;
    gyroConfig.baseValue[1] = 0.0;
    gyroConfig.baseValue[2] = 0.0;
    gyroConfig.noiseMean = 0.0;
    gyroConfig.noiseStdDev = 0.001;
    gyroConfig.updateIntervalMs = 50;
    gyroConfig.range = 17.4533;  // ±1000 deg/s typical
    gyroConfig.resolution = 0.0003;
    state->sensors[SensorType::GYROSCOPE] = gyroConfig;
    
    SensorConfig magConfig;
    magConfig.type = SensorType::MAGNETOMETER;
    magConfig.enabled = true;
    magConfig.useGaussianNoise = true;
    magConfig.baseValue[0] = -25.0;
    magConfig.baseValue[1] = 10.0;
    magConfig.baseValue[2] = 45.0;
    magConfig.noiseMean = 0.0;
    magConfig.noiseStdDev = 0.5;
    magConfig.updateIntervalMs = 100;
    magConfig.range = 4915.0;  // ±50 gauss typical
    magConfig.resolution = 0.15;
    state->sensors[SensorType::MAGNETOMETER] = magConfig;
    
    // Create timer
    state->updateTimer = new QTimer(this);
    connect(state->updateTimer, &QTimer::timeout, this, &SensorInjector::onTimerTimeout);
    state->updateTimer->start(50);  // 20Hz update rate
    
    m_instances[instanceId] = state;
    
    // Initialize sensor system in Android
    initializeSensorSystem(instanceId);
    
    qDebug() << "SensorInjector started for:" << instanceId;
    return true;
}

bool SensorInjector::stop(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) {
        return true;
    }
    
    qDebug() << "Stopping SensorInjector for:" << instanceId;
    
    InstanceState* state = m_instances[instanceId];
    
    if (state->updateTimer) {
        state->updateTimer->stop();
        delete state->updateTimer;
    }
    
    state->isRunning = false;
    delete state;
    m_instances.remove(instanceId);
    
    return true;
}

void SensorInjector::stopAll() {
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_instances.begin(); it != m_instances.end(); ++it) {
        stop(it.key());
    }
}

bool SensorInjector::isRunning(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    return m_instances.contains(instanceId) && m_instances[instanceId]->isRunning;
}

// ============================================================================
// Configuration
// ============================================================================

void SensorInjector::configureSensor(const QString& instanceId, const SensorConfig& config) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) {
        qWarning() << "Instance not found:" << instanceId;
        return;
    }
    
    m_instances[instanceId]->sensors[config.type] = config;
}

void SensorInjector::setSensorEnabled(const QString& instanceId, SensorType type, bool enabled) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) return;
    
    auto& sensor = m_instances[instanceId]->sensors[type];
    sensor.enabled = enabled;
}

void SensorInjector::setMovementPattern(const QString& instanceId, MovementPattern pattern) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) return;
    
    qDebug() << "Setting movement pattern for" << instanceId << "to" << static_cast<int>(pattern);
    m_instances[instanceId]->pattern = pattern;
    
    // Adjust noise parameters based on pattern
    auto& sensors = m_instances[instanceId]->sensors;
    
    switch (pattern) {
        case MovementPattern::WALKING:
            sensors[SensorType::ACCELEROMETER].noiseStdDev = WALK_ACCEL_AMPLITUDE;
            sensors[SensorType::GYROSCOPE].noiseStdDev = WALK_GYRO_AMPLITUDE;
            sensors[SensorType::ACCELEROMETER].updateIntervalMs = 30;  // Higher rate
            break;
            
        case MovementPattern::STATIONARY:
            sensors[SensorType::ACCELEROMETER].noiseStdDev = STATIONARY_NOISE_STDDEV;
            sensors[SensorType::GYROSCOPE].noiseStdDev = 0.001;
            break;
            
        case MovementPattern::ON_TABLE:
            sensors[SensorType::ACCELEROMETER].noiseStdDev = TABLE_VIBRATION_STDDEV;
            sensors[SensorType::GYROSCOPE].noiseStdDev = 0.0005;
            break;
            
        case MovementPattern::IN_CAR:
            sensors[SensorType::ACCELEROMETER].noiseStdDev = 0.5;
            sensors[SensorType::GYROSCOPE].noiseStdDev = 0.3;
            break;
            
        default:
            sensors[SensorType::ACCELEROMETER].noiseStdDev = 0.1;
            sensors[SensorType::GYROSCOPE].noiseStdDev = 0.05;
            break;
    }
}

void SensorInjector::setUpdateRate(const QString& instanceId, SensorType type, int intervalMs) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) return;
    
    m_instances[instanceId]->sensors[type].updateIntervalMs = intervalMs;
}

// ============================================================================
// Direct Injection
// ============================================================================

bool SensorInjector::injectReading(const QString& instanceId, const SensorReading& reading) {
    return injectViaADB(instanceId, reading);
}

bool SensorInjector::injectAccelerometer(const QString& instanceId, double x, double y, double z) {
    SensorReading reading;
    reading.type = SensorType::ACCELEROMETER;
    reading.timestamp = QDateTime::currentMSecsSinceEpoch();
    reading.nanoseconds = QDateTime::currentMSecsSinceEpoch() * 1000000;
    reading.x = x;
    reading.y = y;
    reading.z = z;
    reading.valid = true;
    
    return injectReading(instanceId, reading);
}

bool SensorInjector::injectGyroscope(const QString& instanceId, double x, double y, double z) {
    SensorReading reading;
    reading.type = SensorType::GYROSCOPE;
    reading.timestamp = QDateTime::currentMSecsSinceEpoch();
    reading.nanoseconds = QDateTime::currentMSecsSinceEpoch() * 1000000;
    reading.x = x;
    reading.y = y;
    reading.z = z;
    reading.valid = true;
    
    return injectReading(instanceId, reading);
}

bool SensorInjector::injectMagnetometer(const QString& instanceId, double x, double y, double z) {
    SensorReading reading;
    reading.type = SensorType::MAGNETOMETER;
    reading.timestamp = QDateTime::currentMSecsSinceEpoch();
    reading.nanoseconds = QDateTime::currentMSecsSinceEpoch() * 1000000;
    reading.x = x;
    reading.y = y;
    reading.z = z;
    reading.valid = true;
    
    return injectReading(instanceId, reading);
}

// ============================================================================
// Status
// ============================================================================

QJsonObject SensorInjector::getSensorState(const QString& instanceId) const {
    QJsonObject state;
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) {
        return state;
    }
    
    InstanceState* instState = m_instances[instanceId];
    state["running"] = instState->isRunning;
    state["pattern"] = static_cast<int>(instState->pattern);
    state["stepCount"] = instState->stepCount;
    
    QJsonObject sensors;
    for (auto it = instState->sensors.begin(); it != instState->sensors.end(); ++it) {
        QJsonObject sensor;
        sensor["enabled"] = it.value().enabled;
        sensor["noiseStdDev"] = it.value().noiseStdDev;
        sensor["updateIntervalMs"] = it.value().updateIntervalMs;
        sensors[QString::number(static_cast<int>(it.key()))] = sensor;
    }
    state["sensors"] = sensors;
    
    return state;
}

QVector<SensorReading> SensorInjector::getLastReadings(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) {
        return QVector<SensorReading>();
    }
    
    return m_instances[instanceId]->recentReadings;
}

// ============================================================================
// Private Methods
// ============================================================================

void SensorInjector::onTimerTimeout() {
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_instances.begin(); it != m_instances.end(); ++it) {
        if (!it.value()->isRunning) continue;
        
        const QString& instanceId = it.key();
        InstanceState* state = it.value();
        
        // Update movement state
        updateMovementState(instanceId);
        
        // Generate and inject readings for each sensor
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        
        for (auto sensorIt = state->sensors.begin(); sensorIt != state->sensors.end(); ++sensorIt) {
            if (!sensorIt.value().enabled) continue;
            
            // Check if update is needed based on interval
            SensorType type = sensorIt.key();
            int interval = sensorIt.value().updateIntervalMs;
            
            if (currentTime % interval < 60) {  // Approximate check
                SensorReading reading = generateReading(instanceId, type);
                
                // Apply movement pattern
                switch (state->pattern) {
                    case MovementPattern::WALKING:
                        simulateWalkingMotion(instanceId, reading);
                        break;
                    case MovementPattern::STATIONARY:
                        simulateStationaryMotion(instanceId, reading);
                        break;
                    case MovementPattern::IN_POCKET:
                        simulateInPocketMotion(instanceId, reading);
                        break;
                    case MovementPattern::ON_TABLE:
                        simulateOnTableMotion(instanceId, reading);
                        break;
                    case MovementPattern::IN_CAR:
                        simulateInCarMotion(instanceId, reading);
                        break;
                    default:
                        break;
                }
                
                // Store recent reading
                state->recentReadings.append(reading);
                if (state->recentReadings.size() > 100) {
                    state->recentReadings.removeFirst();
                }
                
                // Inject via ADB
                injectViaADB(instanceId, reading);
                
                emit sensorDataUpdated(instanceId, reading);
            }
        }
    }
}

void SensorInjector::updateSensorData() {
    onTimerTimeout();
}

SensorReading SensorInjector::generateReading(const QString& instanceId, SensorType type) {
    QMutexLocker locker(&m_mutex);
    
    SensorReading reading;
    reading.type = type;
    reading.timestamp = QDateTime::currentMSecsSinceEpoch();
    reading.nanoseconds = reading.timestamp * 1000000;
    reading.valid = true;
    
    if (!m_instances.contains(instanceId)) {
        reading.x = reading.y = reading.z = 0;
        reading.w = 0;
        return reading;
    }
    
    InstanceState* state = m_instances[instanceId];
    const SensorConfig& config = state->sensors[type];
    
    // Base values
    reading.x = config.baseValue[0];
    reading.y = config.baseValue[1];
    reading.z = config.baseValue[2];
    
    // Add Gaussian noise
    if (config.useGaussianNoise) {
        reading.x += generateGaussianNoise(config.noiseMean, config.noiseStdDev);
        reading.y += generateGaussianNoise(config.noiseMean, config.noiseStdDev);
        reading.z += generateGaussianNoise(config.noiseMean, config.noiseStdDev);
    }
    
    // For rotation vector
    if (type == SensorType::ROTATION_VECTOR) {
        reading.w = 1.0;
    }
    
    return reading;
}

double SensorInjector::generateGaussianNoise(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(m_generator);
}

double SensorInjector::generateUniformNoise(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(m_generator);
}

void SensorInjector::simulateWalkingMotion(const QString& instanceId, SensorReading& reading) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_instances.contains(instanceId)) return;
    InstanceState* state = m_instances[instanceId];
    
    // Update walk phase
    state->walkPhase += 0.15;
    if (state->walkPhase > 2 * M_PI) {
        state->walkPhase -= 2 * M_PI;
        state->stepCount++;
    }
    
    // Simulate walking motion pattern
    double stepPhase = state->walkPhase;
    
    if (reading.type == SensorType::ACCELEROMETER) {
        // Walking creates oscillation in vertical axis
        double verticalOscillation = sin(stepPhase) * WALK_ACCEL_AMPLITUDE * 0.5;
        double lateralOscillation = cos(stepPhase * 0.5) * WALK_ACCEL_AMPLITUDE * 0.3;
        
        reading.x += lateralOscillation;
        reading.y += 0;  // Forward/backward
        reading.z += verticalOscillation;
        
        // Update accumulated position
        state->accumulatedX += reading.x * 0.001;
        state->accumulatedY += reading.y * 0.001;
    }
    else if (reading.type == SensorType::GYROSCOPE) {
        // Rotation during walking
        reading.x = sin(stepPhase) * WALK_GYRO_AMPLITUDE * 0.3;
        reading.y = cos(stepPhase) * WALK_GYRO_AMPLITUDE * 0.2;
        reading.z = sin(stepPhase * 2) * WALK_GYRO_AMPLITUDE * 0.5;
    }
}

void SensorInjector::simulateStationaryMotion(const QString& instanceId, SensorReading& reading) {
    // Stationary = almost no movement with tiny vibrations
    double microVibration = generateGaussianNoise(0, TABLE_VIBRATION_STDDEV * 2);
    
    if (reading.type == SensorType::ACCELEROMETER) {
        reading.x += microVibration * 0.1;
        reading.y += microVibration * 0.1;
        reading.z += microVibration * 0.05;
    }
    else if (reading.type == SensorType::GYROSCOPE) {
        // Very tiny rotation from breathing/hand tremor
        reading.x = generateGaussianNoise(0, 0.0005);
        reading.y = generateGaussianNoise(0, 0.0005);
        reading.z = generateGaussianNoise(0, 0.001);
    }
}

void SensorInjector::simulateInPocketMotion(const QString& instanceId, SensorReading& reading) {
    // In pocket = irregular, bouncing movements
    double randomBounce = generateUniformNoise(-0.5, 0.5);
    
    if (reading.type == SensorType::ACCELEROMETER) {
        // Random direction bouncing
        reading.x += generateGaussianNoise(0, 0.8);
        reading.y += generateGaussianNoise(0, 1.2) + 0.5;  // Slight upward bias
        reading.z += generateGaussianNoise(0, 0.8);
    }
    else if (reading.type == SensorType::GYROSCOPE) {
        reading.x = generateGaussianNoise(0, 0.5);
        reading.y = generateGaussianNoise(0, 0.8);
        reading.z = generateGaussianNoise(0, 0.3);
    }
}

void SensorInjector::simulateOnTableMotion(const QString& instanceId, SensorReading& reading) {
    // On table = very subtle vibrations, mostly gravity
    
    if (reading.type == SensorType::ACCELEROMETER) {
        // Tiny table vibrations
        double vibration = generateGaussianNoise(0, TABLE_VIBRATION_STDDEV);
        reading.x += vibration * 0.1;
        reading.y += vibration * 0.1;
        reading.z = 9.81 + vibration * 0.05;  // Keep gravity close to 9.81
    }
    else if (reading.type == SensorType::GYROSCOPE) {
        // Almost zero rotation
        reading.x = generateGaussianNoise(0, 0.0001);
        reading.y = generateGaussianNoise(0, 0.0001);
        reading.z = generateGaussianNoise(0, 0.0001);
    }
}

void SensorInjector::simulateInCarMotion(const QString& instanceId, SensorReading& reading) {
    // In car = smooth acceleration/deceleration with road vibrations
    
    if (reading.type == SensorType::ACCELEROMETER) {
        // Road vibrations
        double roadNoise = generateGaussianNoise(0, 0.3);
        
        // Gradual acceleration/braking pattern
        double carMotion = sin(QDateTime::currentMSecsSinceEpoch() / 1000.0) * 0.5;
        
        reading.x += carMotion * 0.5;  // Forward/backward
        reading.y += roadNoise;        // Lateral
        reading.z = 9.81 + roadNoise * 0.2;  // Vertical
    }
    else if (reading.type == SensorType::GYROSCOPE) {
        // Gentle turns and road bumps
        reading.x = generateGaussianNoise(0, 0.1);
        reading.y = generateGaussianNoise(0, 0.15);
        reading.z = generateGaussianNoise(0, 0.05);
    }
}

bool SensorInjector::injectViaADB(const QString& instanceId, const SensorReading& reading) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QString sensorType;
    double x, y, z;
    
    switch (reading.type) {
        case SensorType::ACCELEROMETER:
            sensorType = "accelerometer";
            x = reading.x;
            y = reading.y;
            z = reading.z;
            break;
        case SensorType::GYROSCOPE:
            sensorType = "gyroscope";
            x = reading.x;
            y = reading.y;
            z = reading.z;
            break;
        case SensorType::MAGNETOMETER:
            sensorType = "magnetic_field";
            x = reading.x;
            y = reading.y;
            z = reading.z;
            break;
        case SensorType::GRAVITY:
            sensorType = "gravity";
            x = reading.x;
            y = reading.y;
            z = reading.z;
            break;
        case SensorType::LINEAR_ACCELERATION:
            sensorType = "linear_acceleration";
            x = reading.x;
            y = reading.y;
            z = reading.z;
            break;
        default:
            return false;
    }
    
    // Use sensor send command via ADB
    QString cmd = QString("sensor send %1 %2 %3 %4 %5")
        .arg(sensorType)
        .arg(x, 0, 'f', 6)
        .arg(y, 0, 'f', 6)
        .arg(z, 0, 'f', 6)
        .arg(reading.nanoseconds);
    
    QString result = ctrl.executeShell(instanceId, cmd);
    
    return result.isEmpty() || !result.contains("error");
}

void SensorInjector::initializeSensorSystem(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Enable sensors in Android
    QStringList initCommands = {
        // Enable hardware sensors
        "setprop hw.sensors.accel 1",
        "setprop hw.sensors.gyro 1",
        "setprop hw.sensors.magnetic 1",
        "setprop hw.sensors.proximity 1",
        "setprop hw.sensors.light 1",
        
        // Set sensor HAL version
        "setprop hw.sensor.hal.version 3.0",
        "setprop hw.sensor.hub.enable 1",
        
        // Calibration data
        "setprop hw.sensor.accel.bias 0,0,0",
        "setprop hw.sensor.gyro.bias 0,0,0",
        "setprop hw.sensor.magnetic.calibration_matrix 1,0,0,0,1,0,0,0,1",
        
        // Sensor report rates
        "setprop hw.sensor.accel.rate 20000",  // 20ms = 50Hz
        "setprop hw.sensor.gyro.rate 20000",
        "setprop hw.sensor.magnetic.rate 50000",  // 50ms = 20Hz
    };
    
    for (const QString& cmd : initCommands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "Sensor system initialized for:" << instanceId;
}

} // namespace VirtualPhonePro
