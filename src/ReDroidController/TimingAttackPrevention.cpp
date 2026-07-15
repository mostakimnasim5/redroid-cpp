/**
 * @file TimingAttackPrevention.cpp
 * @brief Implementation of advanced timing attack prevention
 */

#include "VirtualPhonePro/TimingAttackPrevention.hpp"
#include <QtMath>
#include <QDateTime>
#include <QThread>

namespace VirtualPhonePro {

// ========================================================================
// GAUSSIAN RANDOM
// ========================================================================

GaussianRandom::GaussianRandom(quint64 seed)
    : m_seed(seed)
    , m_generator(seed)
{
}

float GaussianRandom::next(float mean, float stddev) {
    // Box-Muller transform for Gaussian distribution
    if (m_hasSpare) {
        m_hasSpare = false;
        return mean + stddev * m_spare;
    }
    
    float u1, u2;
    do {
        u1 = std::uniform_real_distribution<float>(0.0f, 1.0f)(m_generator);
        u2 = std::uniform_real_distribution<float>(0.0f, 1.0f)(m_generator);
    } while (u1 <= 1e-7f);
    
    m_spare = qSqrt(-2.0f * qLn(u1)) * qCos(2.0f * M_PI * u2);
    m_hasSpare = true;
    
    return mean + stddev * m_spare;
}

int GaussianRandom::nextInt(int mean, int stddev) {
    return qRound(next(static_cast<float>(mean), static_cast<float>(stddev)));
}

double GaussianRandom::nextDouble(double mean, double stddev) {
    return next(static_cast<float>(mean), static_cast<float>(stddev));
}

// ========================================================================
// JSON SERIALIZATION
// ========================================================================

QJsonObject DeviceTimingSeed::toJson() const {
    QJsonObject obj;
    obj["baseSeed"] = QString::number(baseSeed);
    obj["typingSeed"] = QString::number(typingSeed);
    obj["gestureSeed"] = QString::number(gestureSeed);
    obj["networkSeed"] = QString::number(networkSeed);
    obj["sensorSeed"] = QString::number(sensorSeed);
    obj["createdTimestamp"] = createdTimestamp;
    obj["usageCount"] = usageCount;
    return obj;
}

DeviceTimingSeed DeviceTimingSeed::fromJson(const QJsonObject& obj) {
    DeviceTimingSeed seed;
    seed.baseSeed = obj["baseSeed"].toString().toULongLong();
    seed.typingSeed = obj["typingSeed"].toString().toULongLong();
    seed.gestureSeed = obj["gestureSeed"].toString().toULongLong();
    seed.networkSeed = obj["networkSeed"].toString().toULongLong();
    seed.sensorSeed = obj["sensorSeed"].toString().toULongLong();
    seed.createdTimestamp = obj["createdTimestamp"].toLongLong();
    seed.usageCount = obj["usageCount"].toInt();
    return seed;
}

QJsonObject TouchPressureConfig::toJson() const {
    QJsonObject obj;
    obj["minPressure"] = minPressure;
    obj["maxPressure"] = maxPressure;
    obj["avgPressure"] = avgPressure;
    obj["pressureStdDev"] = pressureStdDev;
    obj["driftRate"] = driftRate;
    obj["holdVariation"] = holdVariation;
    return obj;
}

QJsonObject GestureVelocityProfile::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["minVelocity"] = minVelocity;
    obj["maxVelocity"] = maxVelocity;
    obj["avgVelocity"] = avgVelocity;
    obj["stdDev"] = stdDev;
    obj["startAcceleration"] = startAcceleration;
    obj["midCruise"] = midCruise;
    obj["endDeceleration"] = endDeceleration;
    obj["tapVariation"] = tapVariation;
    obj["swipeVariation"] = swipeVariation;
    obj["scrollVariation"] = scrollVariation;
    obj["pinchVariation"] = pinchVariation;
    return obj;
}

QJsonObject BatteryDrainConfig::toJson() const {
    QJsonObject obj;
    obj["initialLevel"] = initialLevel;
    obj["minLevel"] = minLevel;
    obj["maxLevel"] = maxLevel;
    obj["idleDrainPerHour"] = idleDrainPerHour;
    obj["screenOnDrainPerHour"] = screenOnDrainPerHour;
    obj["gamingDrainPerHour"] = gamingDrainPerHour;
    obj["chargingRatePerHour"] = chargingRatePerHour;
    obj["minTemp"] = minTemp;
    obj["maxTemp"] = maxTemp;
    obj["avgTemp"] = avgTemp;
    obj["tempStdDev"] = tempStdDev;
    obj["isCharging"] = isCharging;
    obj["chargeLevel"] = chargeLevel;
    return obj;
}

QJsonObject NetworkJitterConfig::toJson() const {
    QJsonObject obj;
    obj["baseLatency"] = baseLatency;
    obj["minLatency"] = minLatency;
    obj["maxLatency"] = maxLatency;
    obj["jitterStdDev"] = jitterStdDev;
    obj["packetInterval"] = packetInterval;
    obj["packetJitter"] = packetJitter;
    obj["tcpRetransmitTime"] = tcpRetransmitTime;
    obj["tcpRttVariation"] = tcpRttVariation;
    obj["dnsQueryTime"] = dnsQueryTime;
    obj["dnsJitter"] = dnsJitter;
    return obj;
}

QJsonObject SensorNoiseConfig::toJson() const {
    QJsonObject obj;
    obj["accelerometerNoise"] = accelerometerNoise;
    obj["gyroscopeNoise"] = gyroscopeNoise;
    obj["magnetometerNoise"] = magnetometerNoise;
    obj["gpsNoise"] = gpsNoise;
    obj["lightSensorNoise"] = lightSensorNoise;
    obj["minUpdateInterval"] = minUpdateInterval;
    obj["maxUpdateInterval"] = maxUpdateInterval;
    obj["updateJitter"] = updateJitter;
    return obj;
}

// ========================================================================
// TIMING ATTACK PREVENTION
// ========================================================================

TimingAttackPrevention* TimingAttackPrevention::s_instance = nullptr;

TimingAttackPrevention& TimingAttackPrevention::instance() {
    if (!s_instance) {
        s_instance = new TimingAttackPrevention();
    }
    return *s_instance;
}

TimingAttackPrevention::TimingAttackPrevention(QObject* parent)
    : QObject(parent)
{
    qDebug() << "TimingAttackPrevention initialized";
}

TimingAttackPrevention::~TimingAttackPrevention() {
    // Clean up gaussian generators
    qDeleteAll(m_gaussianGenerators);
    m_gaussianGenerators.clear();
}

// ========================================================================
// SEED MANAGEMENT
// ========================================================================

DeviceTimingSeed TimingAttackPrevention::createDeviceSeed(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    DeviceTimingSeed seed;
    
    // Use instance ID and current time to create unique seed
    quint64 baseValue = qHash(instanceId) ^ static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    
    // Derive different seeds for different purposes
    seed.baseSeed = baseValue;
    seed.typingSeed = baseValue ^ 0x123456789ABCDEF0ULL;
    seed.gestureSeed = baseValue ^ 0xFEDCBA9876543210ULL;
    seed.networkSeed = baseValue ^ 0xABCDEF1234567890ULL;
    seed.sensorSeed = baseValue ^ 0x9876543210FEDCBAULL;
    seed.createdTimestamp = QDateTime::currentMSecsSinceEpoch();
    seed.usageCount = 0;
    
    m_deviceSeeds[instanceId] = seed;
    
    // Create Gaussian generator for this device
    m_gaussianGenerators[instanceId] = new GaussianRandom(seed.baseSeed);
    
    // Initialize default configurations
    TouchPressureConfig pressureConfig;
    m_touchPressureConfigs[instanceId] = pressureConfig;
    
    GestureVelocityProfile profile;
    profile.name = "Default";
    m_gestureProfiles[instanceId] = profile;
    
    NetworkJitterConfig networkConfig;
    m_networkConfigs[instanceId] = networkConfig;
    
    SensorNoiseConfig sensorConfig;
    m_sensorConfigs[instanceId] = sensorConfig;
    
    BatteryDrainConfig batteryConfig;
    m_batteryConfigs[instanceId] = batteryConfig;
    m_batteryLevels[instanceId] = batteryConfig.initialLevel;
    m_batteryTemperatures[instanceId] = batteryConfig.avgTemp;
    
    m_sessionStartTime[instanceId] = QDateTime::currentMSecsSinceEpoch();
    m_lastTouchTime[instanceId] = 0;
    m_lastPressure[instanceId] = pressureConfig.avgPressure;
    
    qDebug() << "Created timing seed for device:" << instanceId 
             << "baseSeed:" << QString::number(seed.baseSeed, 16);
    
    return seed;
}

DeviceTimingSeed TimingAttackPrevention::getDeviceSeed(const QString& instanceId) const {
    QMutexLocker locker(&m_mutex);
    return m_deviceSeeds.value(instanceId);
}

void TimingAttackPrevention::removeDeviceSeed(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    
    m_deviceSeeds.remove(instanceId);
    
    if (m_gaussianGenerators.contains(instanceId)) {
        delete m_gaussianGenerators.take(instanceId);
    }
    
    m_touchPressureConfigs.remove(instanceId);
    m_gestureProfiles.remove(instanceId);
    m_networkConfigs.remove(instanceId);
    m_sensorConfigs.remove(instanceId);
    m_batteryConfigs.remove(instanceId);
    m_batteryLevels.remove(instanceId);
    m_batteryTemperatures.remove(instanceId);
    m_sessionStartTime.remove(instanceId);
    m_lastTouchTime.remove(instanceId);
    m_lastPressure.remove(instanceId);
    
    qDebug() << "Removed timing seed for device:" << instanceId;
}

// ========================================================================
// GAUSSIAN DELAYS
// ========================================================================

GaussianRandom* TimingAttackPrevention::getGaussianForDevice(const QString& instanceId) {
    if (!m_gaussianGenerators.contains(instanceId)) {
        createDeviceSeed(instanceId);
    }
    return m_gaussianGenerators.value(instanceId);
}

float TimingAttackPrevention::clamp(float value, float min, float max) {
    return qBound(min, value, max);
}

int TimingAttackPrevention::generateGaussianDelay(const QString& instanceId, int mean, int stddev) {
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    int delay = gauss->nextInt(mean, stddev);
    
    // Clamp to reasonable bounds
    delay = qMax(1, delay);
    delay = qMin(delay, mean + (stddev * 5)); // Cap at 5 standard deviations
    
    return delay;
}

int TimingAttackPrevention::generateTypingDelay(const QString& instanceId, int baseDelay) {
    // Human typing typically has 60-120ms between keys
    // With Gaussian variation
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    int delay = static_cast<int>(gauss->next(static_cast<float>(baseDelay), 30.0f));
    
    // Clamp to human-achievable range
    delay = qMax(20, delay);  // Minimum human reaction
    delay = qMin(delay, 300);  // Maximum for typing
    
    return delay;
}

int TimingAttackPrevention::generateTapDelay(const QString& instanceId) {
    // Tap delay: 50-200ms with Gaussian distribution
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    int delay = static_cast<int>(gauss->next(100.0f, 40.0f));
    
    delay = qMax(30, delay);
    delay = qMin(delay, 300);
    
    return delay;
}

int TimingAttackPrevention::generateSwipeDuration(const QString& instanceId, int distance) {
    // Swipe duration depends on distance and velocity
    GestureVelocityProfile profile = m_gestureProfiles.value(instanceId);
    
    float avgVelocity = (profile.minVelocity + profile.maxVelocity) / 2.0f;
    float velocity = QRandomGenerator::global()->bounded(profile.minVelocity, profile.maxVelocity);
    
    int duration = static_cast<int>(distance / velocity);
    
    // Add Gaussian variation
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    duration = static_cast<int>(gauss->next(static_cast<float>(duration), 50.0f));
    
    duration = qMax(100, duration);
    duration = qMin(duration, 2000);
    
    return duration;
}

int TimingAttackPrevention::generateScrollDuration(const QString& instanceId, int scrollLength) {
    // Scroll duration: 100-800ms depending on length
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    int baseDuration = static_cast<int>(scrollLength * 0.8f);
    
    int duration = static_cast<int>(gauss->next(static_cast<float>(baseDuration), 80.0f));
    
    duration = qMax(50, duration);
    duration = qMin(duration, 1500);
    
    return duration;
}

// ========================================================================
// TOUCH PRESSURE
// ========================================================================

float TimingAttackPrevention::generateTouchPressure(const QString& instanceId, int x, int y, int holdTime) {
    TouchPressureConfig config = m_touchPressureConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    // Base pressure with Gaussian variation
    float pressure = gauss->next(config.avgPressure, config.pressureStdDev);
    
    // Position affects pressure (thumb vs index finger typical areas)
    // Center of screen tends to have more pressure
    int screenCenter = 540; // Assuming 1080p width
    float centerInfluence = 1.0f + (0.1f * (1.0f - qAbs(static_cast<float>(x - screenCenter)) / screenCenter));
    pressure *= centerInfluence;
    
    // Drift over time (pressure decreases slightly with hold)
    if (holdTime > 0) {
        float drift = exp(-static_cast<float>(holdTime) / 10000.0f) * config.driftRate * 100;
        pressure -= drift;
    }
    
    // Add hold variation
    float holdVar = QRandomGenerator::global()->bounded(-config.holdVariation, config.holdVariation);
    pressure += holdVar;
    
    // Clamp to valid range
    pressure = clamp(pressure, config.minPressure, config.maxPressure);
    
    // Store for next calculation
    m_lastPressure[instanceId] = pressure;
    
    return pressure;
}

float TimingAttackPrevention::generateTouchArea(const QString& instanceId) {
    TouchPressureConfig config = m_touchPressureConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    // Touch area: typically 8-15mm
    float avgArea = (config.minPressure + config.maxPressure) * 7.5f; // Rough estimate
    float area = gauss->next(avgArea, 2.0f);
    
    area = clamp(area, 5.0f, 20.0f);
    
    return area;
}

// ========================================================================
// GESTURE VELOCITY
// ========================================================================

float TimingAttackPrevention::generateGestureVelocity(const QString& instanceId, const QString& gestureType, const QString& direction) {
    GestureVelocityProfile profile = m_gestureProfiles.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float variation = 0.0f;
    if (gestureType == "tap") {
        variation = profile.tapVariation;
    } else if (gestureType == "swipe") {
        variation = profile.swipeVariation;
    } else if (gestureType == "scroll") {
        variation = profile.scrollVariation;
    } else if (gestureType == "pinch") {
        variation = profile.pinchVariation;
    }
    
    float velocity = gauss->next(profile.avgVelocity, profile.stdDev * variation * 100);
    
    // Direction affects velocity (up/down typically faster than left/right)
    if (direction == "up" || direction == "down") {
        velocity *= 1.1f;
    }
    
    velocity = clamp(velocity, profile.minVelocity, profile.maxVelocity);
    
    return velocity;
}

QVector<float> TimingAttackPrevention::generateAccelerationProfile(const QString& instanceId, const QString& gestureType, int duration) {
    GestureVelocityProfile profile = m_gestureProfiles.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    QVector<float> velocities;
    int numPoints = duration / 16; // 60fps
    
    for (int i = 0; i < numPoints; i++) {
        float t = static_cast<float>(i) / numPoints;
        
        float velocity;
        if (t < 0.2f) {
            // Start acceleration
            velocity = profile.startAcceleration * (t / 0.2f);
        } else if (t < 0.8f) {
            // Middle cruise
            velocity = profile.midCruise + gauss->next(0.0f, 0.1f);
        } else {
            // End deceleration
            float decelT = (t - 0.8f) / 0.2f;
            velocity = profile.midCruise * (1.0f - decelT * profile.endDeceleration);
        }
        
        velocities.append(velocity);
    }
    
    return velocities;
}

// ========================================================================
// BATTERY SIMULATION
// ========================================================================

void TimingAttackPrevention::initializeBattery(const QString& instanceId, const BatteryDrainConfig& config) {
    QMutexLocker locker(&m_mutex);
    
    m_batteryConfigs[instanceId] = config;
    m_batteryLevels[instanceId] = config.initialLevel;
    m_batteryTemperatures[instanceId] = config.avgTemp;
    m_sessionStartTime[instanceId] = QDateTime::currentMSecsSinceEpoch();
    
    qDebug() << "Initialized battery for device:" << instanceId 
             << "level:" << config.initialLevel << "%"
             << "temp:" << config.avgTemp << "C";
}

int TimingAttackPrevention::getBatteryLevel(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    return m_batteryLevels.value(instanceId, 100);
}

float TimingAttackPrevention::getBatteryTemperature(const QString& instanceId) {
    QMutexLocker locker(&m_mutex);
    return m_batteryTemperatures.value(instanceId, 30.0f);
}

void TimingAttackPrevention::updateBatteryState(const QString& instanceId, const QString& activityType, qint64 elapsedMs) {
    QMutexLocker locker(&m_mutex);
    
    BatteryDrainConfig config = m_batteryConfigs.value(instanceId);
    int& level = m_batteryLevels[instanceId];
    float& temp = m_batteryTemperatures[instanceId];
    
    // Calculate drain based on activity
    float drainPerHour = 0.0f;
    if (activityType == "idle") {
        drainPerHour = config.idleDrainPerHour;
    } else if (activityType == "screen_on") {
        drainPerHour = config.screenOnDrainPerHour;
    } else if (activityType == "gaming") {
        drainPerHour = config.gamingDrainPerHour;
    } else if (activityType == "charging") {
        drainPerHour = -config.chargingRatePerHour;
    }
    
    // Calculate actual drain
    float elapsedHours = static_cast<float>(elapsedMs) / 3600000.0f;
    int change = qRound(drainPerHour * elapsedHours);
    
    level += change;
    
    // Clamp level
    level = qMax(config.minLevel, level);
    level = qMin(config.maxLevel, level);
    
    // Update temperature (rises with activity, cools when idle)
    float tempChange = 0.0f;
    if (activityType == "gaming") {
        tempChange = 2.0f * elapsedHours;
    } else if (activityType == "idle" || activityType == "charging") {
        tempChange = -1.0f * elapsedHours;
    }
    
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    tempChange += gauss->next(0.0f, 0.5f);
    
    temp += tempChange;
    temp = clamp(temp, config.minTemp, config.maxTemp);
    
    emit batteryLevelChanged(instanceId, level);
    emit temperatureChanged(instanceId, temp);
}

// ========================================================================
// NETWORK JITTER
// ========================================================================

float TimingAttackPrevention::generateNetworkLatency(const QString& instanceId) {
    NetworkJitterConfig config = m_networkConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float latency = gauss->next(config.baseLatency, config.jitterStdDev);
    
    // Add occasional spikes (simulating congestion)
    if (QRandomGenerator::global()->bounded(100) < 5) { // 5% chance of spike
        latency += QRandomGenerator::global()->bounded(50, 150);
    }
    
    latency = clamp(latency, config.minLatency, config.maxLatency);
    
    return latency;
}

float TimingAttackPrevention::generateDnsQueryTime(const QString& instanceId) {
    NetworkJitterConfig config = m_networkConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float time = gauss->next(config.dnsQueryTime, config.dnsJitter);
    time = clamp(time, 5.0f, 100.0f);
    
    return time;
}

float TimingAttackPrevention::generatePacketInterval(const QString& instanceId) {
    NetworkJitterConfig config = m_networkConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float interval = gauss->next(config.packetInterval, config.packetJitter);
    interval = clamp(interval, 5.0f, 100.0f);
    
    return interval;
}

float TimingAttackPrevention::generateTcpRtt(const QString& instanceId) {
    NetworkJitterConfig config = m_networkConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float rtt = gauss->next(config.baseLatency * 2, config.tcpRttVariation);
    rtt = clamp(rtt, 20.0f, 500.0f);
    
    return rtt;
}

// ========================================================================
// SENSOR NOISE
// ========================================================================

QVector<float> TimingAttackPrevention::generateAccelerometerReading(const QString& instanceId, float baseX, float baseY, float baseZ) {
    SensorNoiseConfig config = m_sensorConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float noiseX = gauss->next(0.0f, config.accelerometerNoise);
    float noiseY = gauss->next(0.0f, config.accelerometerNoise);
    float noiseZ = gauss->next(0.0f, config.accelerometerNoise);
    
    QVector<float> result;
    result.append(baseX + noiseX);
    result.append(baseY + noiseY);
    result.append(baseZ + noiseZ);
    
    return result;
}

QVector<float> TimingAttackPrevention::generateGyroscopeReading(const QString& instanceId, float baseX, float baseY, float baseZ) {
    SensorNoiseConfig config = m_sensorConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    float noiseX = gauss->next(0.0f, config.gyroscopeNoise);
    float noiseY = gauss->next(0.0f, config.gyroscopeNoise);
    float noiseZ = gauss->next(0.0f, config.gyroscopeNoise);
    
    QVector<float> result;
    result.append(baseX + noiseX);
    result.append(baseY + noiseY);
    result.append(baseZ + noiseZ);
    
    return result;
}

int TimingAttackPrevention::getSensorUpdateInterval(const QString& instanceId, const QString& sensorType) {
    SensorNoiseConfig config = m_sensorConfigs.value(instanceId);
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    int baseInterval;
    if (sensorType == "accelerometer") {
        baseInterval = (config.minUpdateInterval + config.maxUpdateInterval) / 2;
    } else if (sensorType == "gyroscope") {
        baseInterval = config.minUpdateInterval; // Higher frequency
    } else if (sensorType == "gps") {
        baseInterval = 1000; // 1 second
    } else {
        baseInterval = 50;
    }
    
    int interval = baseInterval + gauss->nextInt(0, config.updateJitter);
    interval = qMax(config.minUpdateInterval, interval);
    
    return interval;
}

// ========================================================================
// HUMAN BEHAVIOR SIMULATION
// ========================================================================

int TimingAttackPrevention::generateHumanThinkDelay(const QString& instanceId, const QString& actionType) {
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    int baseDelay;
    if (actionType == "tap") {
        baseDelay = 200; // Quick decision
    } else if (actionType == "scroll") {
        baseDelay = 150;
    } else if (actionType == "open_app") {
        baseDelay = 800; // Looking at screen, deciding
    } else if (actionType == "type") {
        baseDelay = 500; // Thinking about what to type
    } else {
        baseDelay = 300;
    }
    
    int delay = gauss->nextInt(baseDelay, baseDelay / 3);
    delay = qMax(50, delay);
    delay = qMin(delay, baseDelay * 3);
    
    return delay;
}

int TimingAttackPrevention::generateErrorCorrectionDelay(const QString& instanceId, const QString& errorType) {
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    int baseDelay;
    if (errorType == "typo") {
        baseDelay = 400; // Notice and correct
    } else if (errorType == "wrong_tap") {
        baseDelay = 600; // Tap wrong thing, notice, react
    } else if (errorType == "cancel") {
        baseDelay = 300; // Quick cancel
    } else {
        baseDelay = 500;
    }
    
    int delay = gauss->nextInt(baseDelay, baseDelay / 2);
    return delay;
}

int TimingAttackPrevention::generateAppLaunchDelay(const QString& instanceId) {
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    // App launch delay: 500-3000ms
    // Includes boot time, splash screen, initialization
    int delay = gauss->nextInt(1000, 700);
    
    delay = qMax(300, delay);
    delay = qMin(delay, 4000);
    
    return delay;
}

int TimingAttackPrevention::generateAppSwitchDelay(const QString& instanceId) {
    GaussianRandom* gauss = getGaussianForDevice(instanceId);
    
    // App switch delay: 1000-5000ms
    int delay = gauss->nextInt(2000, 1000);
    
    delay = qMax(500, delay);
    delay = qMin(delay, 6000);
    
    return delay;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void TimingAttackPrevention::setTouchPressureConfig(const QString& instanceId, const TouchPressureConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_touchPressureConfigs[instanceId] = config;
}

void TimingAttackPrevention::setGestureVelocityProfile(const QString& instanceId, const GestureVelocityProfile& profile) {
    QMutexLocker locker(&m_mutex);
    m_gestureProfiles[instanceId] = profile;
}

void TimingAttackPrevention::setNetworkJitterConfig(const QString& instanceId, const NetworkJitterConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_networkConfigs[instanceId] = config;
}

void TimingAttackPrevention::setSensorNoiseConfig(const QString& instanceId, const SensorNoiseConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_sensorConfigs[instanceId] = config;
}

TouchPressureConfig TimingAttackPrevention::getTouchPressureConfig(const QString& instanceId) const {
    return m_touchPressureConfigs.value(instanceId);
}

GestureVelocityProfile TimingAttackPrevention::getGestureVelocityProfile(const QString& instanceId) const {
    return m_gestureProfiles.value(instanceId);
}

NetworkJitterConfig TimingAttackPrevention::getNetworkJitterConfig(const QString& instanceId) const {
    return m_networkConfigs.value(instanceId);
}

SensorNoiseConfig TimingAttackPrevention::getSensorNoiseConfig(const QString& instanceId) const {
    return m_sensorConfigs.value(instanceId);
}

} // namespace VirtualPhonePro
