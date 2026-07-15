/**
 * @file TimingAttackPrevention.hpp
 * @brief Advanced timing attack prevention for anti-detection
 * 
 * Features:
 * - Per-Device Unique Random Seed
 * - Gaussian Distribution Delays
 * - Touch Pressure Simulation
 * - Gesture Velocity Profiles
 * - Battery Drain Simulation
 * - Network Timing Jitter
 * 
 * @author ReDroidCPP
 * @version 1.0.0
 */

#ifndef VIRTUALPHONEPRO_TIMINGATTACKPREVENTION_HPP
#define VIRTUALPHONEPRO_TIMINGATTACKPREVENTION_HPP

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QRandomGenerator>
#include <QDateTime>
#include <QJsonObject>
#include <QTimer>

namespace VirtualPhonePro {

// ========================================================================
// GAUSSIAN DISTRIBUTION
// ========================================================================

class GaussianRandom {
public:
    explicit GaussianRandom(quint64 seed);
    float next(float mean, float stddev);
    int nextInt(int mean, int stddev);
    double nextDouble(double mean, double stddev);
    
private:
    quint64 m_seed;
    bool m_hasSpare = false;
    double m_spare;
    std::mt19937_64 m_generator;
};

// ========================================================================
// PER-DEVICE TIMING SEED
// ========================================================================

struct DeviceTimingSeed {
    quint64 baseSeed;           // Primary seed for this device
    quint64 typingSeed;        // Seed for typing patterns
    quint64 gestureSeed;       // Seed for gesture patterns
    quint64 networkSeed;       // Seed for network timing
    quint64 sensorSeed;        // Seed for sensor data
    qint64 createdTimestamp;   // When this seed was created
    int usageCount;            // How many times used
    
    QJsonObject toJson() const;
    static DeviceTimingSeed fromJson(const QJsonObject& obj);
};

// ========================================================================
// TOUCH PRESSURE SIMULATION
// ========================================================================

struct TouchPressureConfig {
    float minPressure = 0.1f;
    float maxPressure = 0.95f;
    float avgPressure = 0.5f;
    float pressureStdDev = 0.15f;
    float driftRate = 0.02f;     // How much pressure changes over time
    float holdVariation = 0.1f;   // Variation when holding
    
    QJsonObject toJson() const;
};

// ========================================================================
// GESTURE VELOCITY PROFILES
// ========================================================================

struct GestureVelocityProfile {
    QString name;
    float minVelocity = 0.3f;
    float maxVelocity = 1.5f;
    float avgVelocity = 0.8f;
    float stdDev = 0.2f;
    
    // Acceleration profiles
    float startAcceleration = 0.5f;
    float midCruise = 1.0f;
    float endDeceleration = 0.3f;
    
    // Variation per gesture type
    float tapVariation = 0.1f;
    float swipeVariation = 0.2f;
    float scrollVariation = 0.15f;
    float pinchVariation = 0.25f;
    
    QJsonObject toJson() const;
};

// ========================================================================
// BATTERY DRAIN SIMULATION
// ========================================================================

struct BatteryDrainConfig {
    int initialLevel = 85;           // Starting battery %
    int minLevel = 20;               // Minimum allowed level
    int maxLevel = 100;              // Maximum allowed level
    
    // Drain rates per activity (percent per hour)
    float idleDrainPerHour = 2.0f;
    float screenOnDrainPerHour = 8.0f;
    float gamingDrainPerHour = 15.0f;
    float chargingRatePerHour = 50.0f;
    
    // Temperature (Celsius)
    float minTemp = 20.0f;
    float maxTemp = 42.0f;
    float avgTemp = 32.0f;
    float tempStdDev = 3.0f;
    
    // Charging state
    bool isCharging = false;
    int chargeLevel = 0;
    
    QJsonObject toJson() const;
};

// ========================================================================
// NETWORK TIMING JITTER
// ========================================================================

struct NetworkJitterConfig {
    float baseLatency = 50.0f;      // Base latency in ms
    float minLatency = 10.0f;
    float maxLatency = 200.0f;
    float jitterStdDev = 15.0f;      // Standard deviation
    
    // Packet timing
    float packetInterval = 20.0f;     // ms between packets
    float packetJitter = 5.0f;
    
    // TCP timing
    float tcpRetransmitTime = 200.0f;
    float tcpRttVariation = 30.0f;
    
    // DNS timing
    float dnsQueryTime = 25.0f;
    float dnsJitter = 10.0f;
    
    QJsonObject toJson() const;
};

// ========================================================================
// SENSOR TIMING NOISE
// ========================================================================

struct SensorNoiseConfig {
    float accelerometerNoise = 0.05f;
    float gyroscopeNoise = 0.02f;
    float magnetometerNoise = 0.5f;
    float gpsNoise = 3.0f;           // meters
    float lightSensorNoise = 10.0f;  // lux
    
    // Update rate variation
    int minUpdateInterval = 16;       // ms (60fps)
    int maxUpdateInterval = 100;      // ms (10fps)
    int updateJitter = 5;            // ms variation
    
    QJsonObject toJson() const;
};

// ========================================================================
// TIMING ATTACK PREVENTION MAIN CLASS
// ========================================================================

class TimingAttackPrevention : public QObject {
    Q_OBJECT

public:
    static TimingAttackPrevention& instance();
    
    // ========================================================================
    // SEED MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Create a unique timing seed for a device instance
     * @param instanceId The device instance ID
     * @return The created seed configuration
     */
    DeviceTimingSeed createDeviceSeed(const QString& instanceId);
    
    /**
     * @brief Get existing seed for a device
     * @param instanceId The device instance ID
     * @return The seed or null if not found
     */
    DeviceTimingSeed getDeviceSeed(const QString& instanceId) const;
    
    /**
     * @brief Remove seed when device is destroyed
     */
    void removeDeviceSeed(const QString& instanceId);
    
    // ========================================================================
    // GAUSSIAN DELAYS
    // ========================================================================
    
    /**
     * @brief Generate a human-like delay using Gaussian distribution
     * @param instanceId Device instance
     * @param mean Mean delay in milliseconds
     * @param stddev Standard deviation
     * @return Random delay in milliseconds
     */
    int generateGaussianDelay(const QString& instanceId, int mean, int stddev);
    
    /**
     * @brief Generate typing delay with realistic variation
     * @param instanceId Device instance
     * @param baseDelay Base delay between keys
     * @return Realistic typing delay
     */
    int generateTypingDelay(const QString& instanceId, int baseDelay = 80);
    
    /**
     * @brief Generate tap delay with variation
     * @param instanceId Device instance
     * @return Realistic tap delay
     */
    int generateTapDelay(const QString& instanceId);
    
    /**
     * @brief Generate swipe gesture duration
     * @param instanceId Device instance
     * @param distance Distance in pixels
     * @return Realistic swipe duration
     */
    int generateSwipeDuration(const QString& instanceId, int distance);
    
    /**
     * @brief Generate scroll gesture duration
     * @param instanceId Device instance
     * @param scrollLength Number of scroll events
     * @return Realistic scroll duration
     */
    int generateScrollDuration(const QString& instanceId, int scrollLength);
    
    // ========================================================================
    // TOUCH PRESSURE
    // ========================================================================
    
    /**
     * @brief Generate realistic touch pressure
     * @param instanceId Device instance
     * @param x X coordinate (affects pressure)
     * @param y Y coordinate (affects pressure)
     * @param holdTime How long the touch has been held
     * @return Touch pressure value (0.0 - 1.0)
     */
    float generateTouchPressure(const QString& instanceId, int x, int y, int holdTime = 0);
    
    /**
     * @brief Generate touch area size
     * @param instanceId Device instance
     * @return Touch area in mm
     */
    float generateTouchArea(const QString& instanceId);
    
    // ========================================================================
    // GESTURE VELOCITY
    // ========================================================================
    
    /**
     * @brief Generate gesture velocity
     * @param instanceId Device instance
     * @param gestureType Type: "tap", "swipe", "scroll", "pinch"
     * @param direction Direction: "up", "down", "left", "right"
     * @return Velocity in pixels per second
     */
    float generateGestureVelocity(const QString& instanceId, const QString& gestureType, const QString& direction);
    
    /**
     * @brief Generate acceleration profile for a gesture
     * @param instanceId Device instance
     * @param gestureType Type of gesture
     * @param duration Total duration
     * @return Vector of velocity values over time
     */
    QVector<float> generateAccelerationProfile(const QString& instanceId, const QString& gestureType, int duration);
    
    // ========================================================================
    // BATTERY SIMULATION
    // ========================================================================
    
    /**
     * @brief Initialize battery for a device
     * @param instanceId Device instance
     * @param config Battery configuration
     */
    void initializeBattery(const QString& instanceId, const BatteryDrainConfig& config);
    
    /**
     * @brief Get current battery level
     * @param instanceId Device instance
     * @return Battery percentage
     */
    int getBatteryLevel(const QString& instanceId);
    
    /**
     * @brief Get current battery temperature
     * @param instanceId Device instance
     * @return Temperature in Celsius
     */
    float getBatteryTemperature(const QString& instanceId);
    
    /**
     * @brief Update battery state based on activity
     * @param instanceId Device instance
     * @param activityType Type: "idle", "screen_on", "gaming", "charging"
     * @param elapsedMs Time elapsed in milliseconds
     */
    void updateBatteryState(const QString& instanceId, const QString& activityType, qint64 elapsedMs);
    
    // ========================================================================
    // NETWORK JITTER
    // ========================================================================
    
    /**
     * @brief Generate network latency with jitter
     * @param instanceId Device instance
     * @return Latency in milliseconds
     */
    float generateNetworkLatency(const QString& instanceId);
    
    /**
     * @brief Generate DNS query time
     * @param instanceId Device instance
     * @return DNS query time in milliseconds
     */
    float generateDnsQueryTime(const QString& instanceId);
    
    /**
     * @brief Generate packet interval
     * @param instanceId Device instance
     * @return Packet interval in milliseconds
     */
    float generatePacketInterval(const QString& instanceId);
    
    /**
     * @brief Generate TCP RTT (Round Trip Time)
     * @param instanceId Device instance
     * @return TCP RTT in milliseconds
     */
    float generateTcpRtt(const QString& instanceId);
    
    // ========================================================================
    // SENSOR NOISE
    // ========================================================================
    
    /**
     * @brief Generate accelerometer reading with noise
     * @param instanceId Device instance
     * @param baseX Base X value
     * @param baseY Base Y value  
     * @param baseZ Base Z value
     * @return Vector of (x, y, z) with noise
     */
    QVector<float> generateAccelerometerReading(const QString& instanceId, float baseX, float baseY, float baseZ);
    
    /**
     * @brief Generate gyroscope reading with noise
     * @param instanceId Device instance
     * @param baseX Base X value
     * @param baseY Base Y value
     * @param baseZ Base Z value
     * @return Vector of (x, y, z) with noise
     */
    QVector<float> generateGyroscopeReading(const QString& instanceId, float baseX, float baseY, float baseZ);
    
    /**
     * @brief Get sensor update interval
     * @param instanceId Device instance
     * @param sensorType Type: "accelerometer", "gyroscope", "gps"
     * @return Update interval in milliseconds
     */
    int getSensorUpdateInterval(const QString& instanceId, const QString& sensorType);
    
    // ========================================================================
    // HUMAN BEHAVIOR SIMULATION
    // ========================================================================
    
    /**
     * @brief Generate random "thinking" delay before action
     * @param instanceId Device instance
     * @param actionType Type: "tap", "scroll", "open_app", "type"
     * @return Delay in milliseconds
     */
    int generateHumanThinkDelay(const QString& instanceId, const QString& actionType);
    
    /**
     * @brief Generate error correction delay (when human makes mistake)
     * @param instanceId Device instance
     * @param errorType Type: "typo", "wrong_tap", "cancel"
     * @return Delay in milliseconds
     */
    int generateErrorCorrectionDelay(const QString& instanceId, const QString& errorType);
    
    /**
     * @brief Generate variable delay for app launching
     * @param instanceId Device instance
     * @return Delay in milliseconds (typically 500-3000ms)
     */
    int generateAppLaunchDelay(const QString& instanceId);
    
    /**
     * @brief Generate delay between app switches
     * @param instanceId Device instance
     * @return Delay in milliseconds (typically 1000-5000ms)
     */
    int generateAppSwitchDelay(const QString& instanceId);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void setTouchPressureConfig(const QString& instanceId, const TouchPressureConfig& config);
    void setGestureVelocityProfile(const QString& instanceId, const GestureVelocityProfile& profile);
    void setNetworkJitterConfig(const QString& instanceId, const NetworkJitterConfig& config);
    void setSensorNoiseConfig(const QString& instanceId, const SensorNoiseConfig& config);
    
    TouchPressureConfig getTouchPressureConfig(const QString& instanceId) const;
    GestureVelocityProfile getGestureVelocityProfile(const QString& instanceId) const;
    NetworkJitterConfig getNetworkJitterConfig(const QString& instanceId) const;
    SensorNoiseConfig getSensorNoiseConfig(const QString& instanceId) const;
    
signals:
    void batteryLevelChanged(const QString& instanceId, int level);
    void temperatureChanged(const QString& instanceId, float temp);

private:
    explicit TimingAttackPrevention(QObject* parent = nullptr);
    ~TimingAttackPrevention() = default;
    
    TimingAttackPrevention(const TimingAttackPrevention&) = delete;
    TimingAttackPrevention& operator=(const TimingAttackPrevention&) = delete;
    
    static TimingAttackPrevention* s_instance;
    
    mutable QMutex m_mutex;
    
    // Per-device data
    QMap<QString, DeviceTimingSeed> m_deviceSeeds;
    QMap<QString, GaussianRandom*> m_gaussianGenerators;
    QMap<QString, TouchPressureConfig> m_touchPressureConfigs;
    QMap<QString, GestureVelocityProfile> m_gestureProfiles;
    QMap<QString, NetworkJitterConfig> m_networkConfigs;
    QMap<QString, SensorNoiseConfig> m_sensorConfigs;
    QMap<QString, BatteryDrainConfig> m_batteryConfigs;
    
    // Timing state
    QMap<QString, qint64> m_lastTouchTime;
    QMap<QString, float> m_lastPressure;
    QMap<QString, int> m_batteryLevels;
    QMap<QString, float> m_batteryTemperatures;
    QMap<QString, qint64> m_sessionStartTime;
    
    // Helper methods
    GaussianRandom* getGaussianForDevice(const QString& instanceId);
    quint64 deriveSeed(const QString& instanceId, const QString& purpose);
    float clamp(float value, float min, float max);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_TIMINGATTACKPREVENTION_HPP
