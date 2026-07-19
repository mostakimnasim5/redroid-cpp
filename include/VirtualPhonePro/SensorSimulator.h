#pragma once

#ifndef VIRTUALPHONEPRO_SENSOR_SIMULATOR_H
#define VIRTUALPHONEPRO_SENSOR_SIMULATOR_H

#include <QString>
#include <QTimer>
#include <QMap>

namespace VirtualPhonePro {

/**
 * @brief Sensor type enumeration
 */
enum class SensorType {
    Accelerometer,
    Gyroscope,
    MagneticField,
    GPS,
    Light,
    Proximity,
    Pressure,
    AmbientTemperature,
    RelativeHumidity
};

/**
 * @brief 3D Vector data
 */
struct Vector3D {
    float x;
    float y;
    float z;
    float accuracy;
    qint64 timestamp;
};

/**
 * @brief GPS Location data
 */
struct GPSLocation {
    double latitude;
    double longitude;
    double altitude;
    float accuracy;
    float speed;
    float bearing;
    qint64 timestamp;
};

/**
 * @brief SimSensorCalibration data
 */
struct SimSensorCalibration {
    float offsetX;
    float offsetY;
    float offsetZ;
    float scaleX;
    float scaleY;
    float scaleZ;
};

/**
 * @brief SensorSimulator - Advanced sensor data simulation
 * 
 * Provides comprehensive sensor simulation including GPS, accelerometer,
 * gyroscope, and other device sensors.
 */
class SensorSimulator : public QObject {
    Q_OBJECT

public:
    static SensorSimulator& instance();
    
    // =========================================================================
    // GPS Simulation
    // =========================================================================
    
    /**
     * @brief Set static GPS location
     */
    bool setGPSLocation(const QString& instanceId, 
                        double latitude, double longitude,
                        double altitude = 0, float accuracy = 5);
    
    /**
     * @brief Start GPS movement simulation (route)
     */
    bool startGPSRoute(const QString& instanceId,
                       const QList<GPSLocation>& route,
                       int intervalMs = 1000);
    
    /**
     * @brief Start circular GPS movement
     */
    bool startGPSCircle(const QString& instanceId,
                        double centerLat, double centerLon,
                        double radiusMeters, int intervalMs = 1000);
    
    /**
     * @brief Stop GPS simulation
     */
    bool stopGPSSimulation(const QString& instanceId);
    
    // =========================================================================
    // Accelerometer Simulation
    // =========================================================================
    
    /**
     * @brief Set static acceleration
     */
    bool setAccelerometer(const QString& instanceId,
                        float x, float y, float z);
    
    /**
     * @brief Start shake simulation
     */
    bool startShake(const QString& instanceId, float intensity = 1.0f);
    
    /**
     * @brief Start tilt simulation
     */
    bool startTilt(const QString& instanceId, float pitch, float roll);
    
    /**
     * @brief Stop accelerometer simulation
     */
    bool stopAccelerometer(const QString& instanceId);
    
    // =========================================================================
    // Gyroscope Simulation
    // =========================================================================
    
    /**
     * @brief Set static rotation
     */
    bool setGyroscope(const QString& instanceId,
                     float x, float y, float z);
    
    /**
     * @brief Start rotation simulation
     */
    bool startRotation(const QString& instanceId, float speedX, float speedY, float speedZ);
    
    /**
     * @brief Stop gyroscope simulation
     */
    bool stopGyroscope(const QString& instanceId);
    
    // =========================================================================
    // Magnetic Field Simulation
    // =========================================================================
    
    /**
     * @brief Set magnetic field
     */
    bool setMagneticField(const QString& instanceId,
                        float x, float y, float z);
    
    /**
     * @brief Simulate compass movement
     */
    bool startCompass(const QString& instanceId, float heading);
    
    // =========================================================================
    // Multi-Sensor Management
    // =========================================================================
    
    /**
     * @brief Start all sensors with preset
     */
    bool startAllSensors(const QString& instanceId, const QString& preset = "walking");
    
    /**
     * @brief Stop all sensor simulations
     */
    bool stopAllSensors(const QString& instanceId);
    
    /**
     * @brief Get current sensor values
     */
    QMap<QString, QVariant> getCurrentSensors(const QString& instanceId);
    
signals:
    void sensorUpdated(const QString& instanceId, SensorType type, const QVariant& value);
    void gpsLocationChanged(const QString& instanceId, const GPSLocation& location);

private:
    explicit SensorSimulator(QObject* parent = nullptr);
    
    struct SensorState {
        bool active;
        QTimer* timer;
        QVariantMap values;
    };
    
    QMap<QString, QMap<SensorType, SensorState>> m_sensorStates;
    QMap<QString, QList<GPSLocation>> m_gpsRoutes;
    QMap<QString, int> m_routeIndices;
    
    bool setSensorActive(const QString& instanceId, SensorType type, bool active);
    bool sendSensorEvent(const QString& instanceId, SensorType type, const QVariant& value);
    bool executeCommand(const QString& instanceId, const QString& command);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SENSOR_SIMULATOR_H
