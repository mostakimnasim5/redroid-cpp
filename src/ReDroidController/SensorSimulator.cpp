/**
 * @file SensorSimulator.cpp
 * @brief Sensor Simulation Implementation
 * @version 2.0.0
 * 
 * Handles GPS, accelerometer, gyroscope and other sensor simulation.
 */

#include "VirtualPhonePro/SensorSimulator.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>
#include <QJsonObject>
#include <QtMath>      // M_PI cross-platform

namespace VirtualPhonePro {

SensorSimulator* SensorSimulator::s_instance = nullptr;

SensorSimulator& SensorSimulator::instance() {
    if (!s_instance) {
        s_instance = new SensorSimulator();
    }
    return *s_instance;
}

SensorSimulator::SensorSimulator(QObject* parent)
    : QObject(parent)
{
}

SensorSimulator::~SensorSimulator() {
    stopAllSensors();
}

bool SensorSimulator::setGPSLocation(const QString& instanceId, double latitude, 
                                      double longitude, double altitude, double accuracy) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Enable mock location
    enableMockLocation(instanceId);
    
    // Set mock location provider
    QStringList commands = {
        // Set GPS coordinates via mock location
        QString("setprop persist.gps.mock_location 1"),
        QString("setprop persist.gps.latitude %1").arg(latitude),
        QString("setprop persist.gps.longitude %2").arg(longitude),
        QString("setprop persist.gps.altitude %1").arg(altitude),
        QString("setprop persist.gps.accuracy %1").arg(accuracy),
        
        // Set location via app (requires mock location permission)
        QString("appops set %1 android:mock_location allow").arg("com.android.location")
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Also try setting via settings command
    QString gpsCmd = QString("settings put secure mock_location %1").arg(latitude) + 
                     QString(",") + QString::number(longitude);
    ctrl.executeShell(instanceId, gpsCmd);
    
    qDebug() << "[SensorSimulator] GPS set to:" << latitude << "," << longitude;
    return true;
}

bool SensorSimulator::startGPSRoute(const QString& instanceId, const QList<GPSLocation>& route, 
                                     int intervalMs) {
    if (instanceId.isEmpty() || route.isEmpty()) {
        return false;
    }
    
    // Store route data
    m_gpsRoutes[instanceId] = route;
    m_gpsRouteIndex[instanceId] = 0;
    m_gpsRouteInterval[instanceId] = intervalMs;
    m_gpsRouteActive[instanceId] = true;
    
    // Start route simulation timer
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, instanceId]() {
        if (m_gpsRouteActive.value(instanceId)) {
            simulateNextRoutePoint(instanceId);
        }
    });
    m_gpsTimers[instanceId] = timer;
    timer->start(intervalMs);
    
    return true;
}

void SensorSimulator::simulateNextRoutePoint(const QString& instanceId) {
    if (!m_gpsRoutes.contains(instanceId)) return;
    
    const QList<GPSLocation>& route = m_gpsRoutes[instanceId];
    int& index = m_gpsRouteIndex[instanceId];
    
    if (index >= route.size()) {
        index = 0; // Loop back to start
    }
    
    const GPSLocation& loc = route[index];
    setGPSLocation(instanceId, loc.latitude, loc.longitude, loc.altitude, loc.accuracy);
    
    index++;
}

bool SensorSimulator::stopGPSRoute(const QString& instanceId) {
    if (m_gpsTimers.contains(instanceId)) {
        m_gpsTimers[instanceId]->stop();
        delete m_gpsTimers[instanceId];
        m_gpsTimers.remove(instanceId);
    }
    
    m_gpsRouteActive[instanceId] = false;
    m_gpsRoutes.remove(instanceId);
    m_gpsRouteIndex.remove(instanceId);
    m_gpsRouteInterval.remove(instanceId);
    
    return true;
}

bool SensorSimulator::startGPSCircle(const QString& instanceId, double centerLat, 
                                     double centerLon, double radiusMeters, int intervalMs) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    // Generate circular route points
    QList<GPSLocation> circlePoints;
    int numPoints = 36; // One point every 10 degrees
    
    for (int i = 0; i < numPoints; i++) {
        double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
        
        // Convert meters to degrees (approximate)
        double latOffset = (radiusMeters / 111111.0) * qCos(angle);
        double lonOffset = (radiusMeters / (111111.0 * qCos(centerLat * M_PI / 180.0))) * qSin(angle);
        
        GPSLocation loc;
        loc.latitude = centerLat + latOffset;
        loc.longitude = centerLon + lonOffset;
        loc.altitude = 0;
        loc.accuracy = 5.0;
        circlePoints.append(loc);
    }
    
    return startGPSRoute(instanceId, circlePoints, intervalMs);
}

bool SensorSimulator::setAccelerometer(const QString& instanceId, float x, float y, float z) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Send sensor event via dumpsys (limited simulation)
    QString cmd = QString("sendevent /dev/input/event0 %1 %2 %3 %4")
                      .arg(x).arg(y).arg(z).arg(0);
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

bool SensorSimulator::startShake(const QString& instanceId, float intensity) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    // Generate random shake values
    m_shakeActive[instanceId] = true;
    
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, instanceId, intensity]() {
        if (m_shakeActive.value(instanceId)) {
            float shakeX = (QRandomGenerator::global()->bounded(200) - 100) / 100.0f * intensity;
            float shakeY = (QRandomGenerator::global()->bounded(200) - 100) / 100.0f * intensity;
            float shakeZ = 9.8f + (QRandomGenerator::global()->bounded(200) - 100) / 100.0f * intensity;
            
            setAccelerometer(instanceId, shakeX, shakeY, shakeZ);
        }
    });
    
    m_sensorTimers[instanceId] = timer;
    timer->start(50); // Update every 50ms
    
    return true;
}

bool SensorSimulator::startTilt(const QString& instanceId, float angleX, float angleY) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    // Convert angles to accelerometer values
    float accelX = qSin(angleX * M_PI / 180.0f) * 9.8f;
    float accelY = qSin(angleY * M_PI / 180.0f) * 9.8f;
    
    return setAccelerometer(instanceId, accelX, accelY, 9.8f);
}

bool SensorSimulator::stopAccelerometer(const QString& instanceId) {
    if (m_sensorTimers.contains(instanceId)) {
        m_sensorTimers[instanceId]->stop();
        delete m_sensorTimers[instanceId];
        m_sensorTimers.remove(instanceId);
    }
    
    m_shakeActive[instanceId] = false;
    return true;
}

bool SensorSimulator::setGyroscope(const QString& instanceId, float x, float y, float z) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    // Gyroscope simulation (limited in emulator)
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set as system property
    QStringList commands = {
        QString("setprop persist.sensor.gyro.x %1").arg(x),
        QString("setprop persist.sensor.gyro.y %1").arg(y),
        QString("setprop persist.sensor.gyro.z %1").arg(z)
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool SensorSimulator::startRotation(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    m_rotationActive[instanceId] = true;
    
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, instanceId]() {
        if (m_rotationActive.value(instanceId)) {
            float time = QDateTime::currentMSecsSinceEpoch() / 1000.0f;
            float rotX = qSin(time) * 0.5f;
            float rotY = qCos(time) * 0.5f;
            float rotZ = qSin(time * 0.5f) * 0.3f;
            
            setGyroscope(instanceId, rotX, rotY, rotZ);
        }
    });
    
    m_sensorTimers[instanceId] = timer;
    timer->start(100);
    
    return true;
}

bool SensorSimulator::stopGyroscope(const QString& instanceId) {
    m_rotationActive[instanceId] = false;
    
    if (m_sensorTimers.contains(instanceId)) {
        m_sensorTimers[instanceId]->stop();
        delete m_sensorTimers[instanceId];
        m_sensorTimers.remove(instanceId);
    }
    
    return true;
}

bool SensorSimulator::setMagneticField(const QString& instanceId, float x, float y, float z) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("setprop persist.sensor.magnetic.x %1").arg(x),
        QString("setprop persist.sensor.magnetic.y %1").arg(y),
        QString("setprop persist.sensor.magnetic.z %1").arg(z)
    };
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool SensorSimulator::startCompass(const QString& instanceId) {
    if (instanceId.isEmpty()) {
        return false;
    }
    
    m_compassActive[instanceId] = true;
    
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, instanceId]() {
        if (m_compassActive.value(instanceId)) {
            float time = QDateTime::currentMSecsSinceEpoch() / 1000.0f;
            
            // Simulate compass heading changes
            float heading = fmod(time * 10.0f, 360.0f);
            
            // Convert heading to magnetic field (simplified)
            float magX = 25.0f * qCos(heading * M_PI / 180.0f);
            float magY = 25.0f * qSin(heading * M_PI / 180.0f);
            float magZ = 45.0f;
            
            setMagneticField(instanceId, magX, magY, magZ);
        }
    });
    
    m_sensorTimers[instanceId] = timer;
    timer->start(200);
    
    return true;
}

bool SensorSimulator::stopCompass(const QString& instanceId) {
    m_compassActive[instanceId] = false;
    
    if (m_sensorTimers.contains(instanceId)) {
        m_sensorTimers[instanceId]->stop();
        delete m_sensorTimers[instanceId];
        m_sensorTimers.remove(instanceId);
    }
    
    return true;
}

void SensorSimulator::stopAllSensors(const QString& instanceId) {
    stopGPSRoute(instanceId);
    stopAccelerometer(instanceId);
    stopGyroscope(instanceId);
    stopCompass(instanceId);
    
    m_shakeActive[instanceId] = false;
    m_rotationActive[instanceId] = false;
    m_compassActive[instanceId] = false;
}

QJsonObject SensorSimulator::getSensorStatus(const QString& instanceId) {
    QJsonObject status;
    
    status["gpsRouteActive"] = m_gpsRouteActive.value(instanceId, false);
    status["accelerometerActive"] = m_shakeActive.value(instanceId, false);
    status["gyroscopeActive"] = m_rotationActive.value(instanceId, false);
    status["compassActive"] = m_compassActive.value(instanceId, false);
    
    return status;
}

} // namespace VirtualPhonePro
