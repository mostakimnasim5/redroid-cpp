/**
 * @file HALSimulation.h
 * @brief Hardware Abstraction Layer Simulation
 * @version 2.0.0
 * 
 * Simulates Android HAL (Hardware Abstraction Layer) for:
 * - Camera HAL
 * - Biometric HAL
 * - Audio HAL
 * - Sensor HAL
 * 
 * This makes emulators appear as having real hardware capabilities.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_HAL_SIMULATION_H
#define VIRTUALPHONEPRO_HAL_SIMULATION_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// ============================================================================
// Camera HAL Types
// ============================================================================

enum class CameraFacing {
    FRONT,
    BACK,
    EXTERNAL
};

enum class FlashMode {
    OFF,
    ON,
    AUTO,
    TORCH
};

struct CameraCapabilities {
    int maxResolution;
    int minResolution;
    bool hasFlash;
    bool hasAutofocus;
    bool hasOpticalStabilization;
    bool hasElectronicStabilization;
    bool supportsHDR;
    bool supportsNightMode;
    bool supportsPortraitMode;
    int maxZoom;
    float maxAperture;
    QString supportedModes;
    QString supportedSceneModes;
    QString supportedWhiteBalance;
};

struct CameraConfig {
    int cameraId;
    CameraFacing facing;
    QString cameraName;
    QString cameraFullName;
    CameraCapabilities capabilities;
    bool isAvailable;
    bool isEnabled;
};

// ============================================================================
// Biometric HAL Types
// ============================================================================

enum class BiometricType {
    NONE,
    FINGERPRINT,
    FACE,
    IRIS,
    MULTI
};

enum class BiometricStrength {
    WEAK,
    STRONG,
    CONVENIENT
};

struct BiometricEnrolledFinger {
    int fingerId;
    QString name;
    qint64 enrolledAt;
};

struct BiometricConfig {
    BiometricType type;
    BiometricStrength strength;
    bool isEnrolled;
    bool isEnabled;
    bool isHardwarePresent;
    bool isLockout;
    int failedAttempts;
    QList<BiometricEnrolledFinger> enrolledFingers;
    int maxEnrollments;
};

// ============================================================================
// Audio HAL Types
// ============================================================================

enum class AudioProfile {
    DEFAULT,
    VOIP,
    MUSIC,
    VIDEO,
    RECORDING
};

struct AudioDevice {
    QString name;
    QString address;
    bool isConnected;
    bool isActive;
    int volume;
    bool isMuted;
};

struct AudioConfig {
    QString primaryOutput;
    QString primaryInput;
    QList<AudioDevice> outputDevices;
    QList<AudioDevice> inputDevices;
    AudioProfile activeProfile;
    int masterVolume;
    bool isMuted;
    bool isSpeakerOn;
    bool isHeadsetConnected;
    bool isBluetoothConnected;
};

// ============================================================================
// Complete HAL Configuration
// ============================================================================

struct HALState {
    CameraConfig backCamera;
    CameraConfig frontCamera;
    BiometricConfig fingerprint;
    BiometricConfig faceUnlock;
    AudioConfig audio;
    bool allHALPresent;
};

// ============================================================================
// HAL Simulation Class
// ============================================================================

class HALSimulation {
public:
    static HALSimulation& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure HAL for device
     */
    bool configureForDevice(const QString& manufacturer, const QString& model);
    
    /**
     * @brief Apply to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Camera HAL
    // =========================================================================
    
    /**
     * @brief Configure back camera
     */
    bool configureBackCamera(const QString& instanceId, int resolution, const QString& model);
    
    /**
     * @brief Configure front camera
     */
    bool configureFrontCamera(const QString& instanceId, int resolution, const QString& model);
    
    /**
     * @brief Enable camera with specific capabilities
     */
    bool enableCamera(const QString& instanceId, CameraFacing facing);
    
    /**
     * @brief Disable camera
     */
    bool disableCamera(const QString& instanceId, CameraFacing facing);
    
    // =========================================================================
    // Biometric HAL
    // =========================================================================
    
    /**
     * @brief Configure fingerprint
     */
    bool configureFingerprint(const QString& instanceId, bool enrolled = false);
    
    /**
     * @brief Enroll fingerprint
     */
    bool enrollFingerprint(const QString& instanceId, int fingerId, const QString& name);
    
    /**
     * @brief Configure face unlock
     */
    bool configureFaceUnlock(const QString& instanceId, bool enrolled = false);
    
    /**
     * @brief Enroll face
     */
    bool enrollFace(const QString& instanceId);
    
    /**
     * @brief Check biometric authentication
     */
    bool authenticateBiometric(const QString& instanceId, BiometricType type);
    
    /**
     * @brief Lockout biometric
     */
    bool lockoutBiometric(const QString& instanceId, BiometricType type);
    
    // =========================================================================
    // Audio HAL
    // =========================================================================
    
    /**
     * @brief Configure audio output
     */
    bool configureAudio(const QString& instanceId);
    
    /**
     * @brief Set speaker state
     */
    bool setSpeakerEnabled(const QString& instanceId, bool enabled);
    
    /**
     * @brief Set headphone state
     */
    bool setHeadsetConnected(const QString& instanceId, bool connected);
    
    /**
     * @brief Set Bluetooth audio
     */
    bool setBluetoothAudio(const QString& instanceId, bool connected);
    
    /**
     * @brief Set volume
     */
    bool setVolume(const QString& instanceId, int level);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get all HAL properties
     */
    QMap<QString, QString> getAllHALProperties(const QString& instanceId);
    
    /**
     * @brief Apply all HAL spoofing
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Reset HAL
     */
    bool resetHAL(const QString& instanceId);
    
private:
    static HALSimulation* s_instance;
    HALSimulation();
    
    CameraConfig getCameraDefaults(CameraFacing facing, const QString& manufacturer, const QString& model);
    BiometricConfig getBiometricDefaults(BiometricType type);
    
    QMap<QString, HALState> m_halStates;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_HAL_SIMULATION_H
