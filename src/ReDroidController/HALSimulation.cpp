/**
 * @file HALSimulation.cpp
 * @brief Hardware Abstraction Layer Simulation Implementation
 */

#include "VirtualPhonePro/HALSimulation.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>

namespace VirtualPhonePro {

HALSimulation* HALSimulation::s_instance = nullptr;

HALSimulation& HALSimulation::instance() {
    if (!s_instance) {
        s_instance = new HALSimulation();
    }
    return *s_instance;
}

HALSimulation::HALSimulation() {
}

// ============================================================================
// Configuration
// ============================================================================

bool HALSimulation::configureForDevice(const QString& manufacturer, const QString& model) {
    qDebug() << "Configuring HAL for:" << manufacturer << model;
    
    // This will apply defaults when instances are created
    return true;
}

bool HALSimulation::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    HALState& state = m_halStates[instanceId];
    
    // Apply camera HAL
    QStringList cameraCommands = {
        // Back camera
        QString("setprop persist.camera.physical.main %1").arg(
            state.backCamera.isAvailable ? "1" : "0"),
        QString("setprop persist.camera.physical.front %1").arg(
            state.frontCamera.isAvailable ? "1" : "0"),
        
        // Camera capabilities
        "setprop persist.camera.hdr true",
        "setprop persist.camera.night.mode true",
        "setprop persist.camera.portrait true",
        "setprop persist.camera.ois true",
        "setprop persist.camera.eis true",
        
        // Biometric HAL
        "setprop persist.fingerprint.enabled true",
        "setprop persist.biometric.face.enabled true",
        "setprop ro.hardware.fingerprint goodix",
        "setprop ro.hardware.face_unlock qualcomm",
        
        // Audio HAL
        "setprop persist.audio.dirac.enabled true",
        "setprop persist.audio.dolby.enabled true",
        "setprop persist.audio.hifi true",
        
        // HAL versions
        "setprop ro.camera.version 3",
        "setprop ro.audio.version 7",
        "setprop ro.hardware.consumerir 1",
    };
    
    for (const QString& cmd : cameraCommands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Set biometric enrollment
    if (state.fingerprint.isEnrolled) {
        int fingerCount = state.fingerprint.enrolledFingers.size();
        ctrl.executeShell(instanceId,
            QString("settings put secure fingerprint_enrolled 1"));
        ctrl.executeShell(instanceId,
            QString("settings put secure fingerprint_count %1").arg(fingerCount));
    }
    
    if (state.faceUnlock.isEnrolled) {
        ctrl.executeShell(instanceId,
            "settings put secure face_enrolled 1");
    }
    
    // Apply audio configuration
    ctrl.executeShell(instanceId,
        QString("settings put system volume_alarm %1").arg(state.audio.masterVolume));
    ctrl.executeShell(instanceId,
        QString("settings put system volume_music %1").arg(state.audio.masterVolume));
    ctrl.executeShell(instanceId,
        QString("settings put system volume_ring %1").arg(state.audio.masterVolume));
    ctrl.executeShell(instanceId,
        QString("settings put system volume_notification %1").arg(state.audio.masterVolume));
    
    qDebug() << "HAL configuration applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Camera HAL
// ============================================================================

bool HALSimulation::configureBackCamera(const QString& instanceId, int resolution, const QString& model) {
    HALState& state = m_halStates[instanceId];
    state.backCamera = getCameraDefaults(CameraFacing::BACK, "", model);
    state.backCamera.cameraId = 0;
    state.backCamera.isAvailable = true;
    
    return applyToInstance(instanceId);
}

bool HALSimulation::configureFrontCamera(const QString& instanceId, int resolution, const QString& model) {
    HALState& state = m_halStates[instanceId];
    state.frontCamera = getCameraDefaults(CameraFacing::FRONT, "", model);
    state.frontCamera.cameraId = 1;
    state.frontCamera.isAvailable = true;
    
    return applyToInstance(instanceId);
}

bool HALSimulation::enableCamera(const QString& instanceId, CameraFacing facing) {
    HALState& state = m_halStates[instanceId];
    
    if (facing == CameraFacing::BACK) {
        state.backCamera.isEnabled = true;
    } else {
        state.frontCamera.isEnabled = true;
    }
    
    return applyToInstance(instanceId);
}

bool HALSimulation::disableCamera(const QString& instanceId, CameraFacing facing) {
    HALState& state = m_halStates[instanceId];
    
    if (facing == CameraFacing::BACK) {
        state.backCamera.isEnabled = false;
    } else {
        state.frontCamera.isEnabled = false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    if (facing == CameraFacing::BACK) {
        ctrl.executeShell(instanceId, "setprop persist.camera.physical.main 0");
    } else {
        ctrl.executeShell(instanceId, "setprop persist.camera.physical.front 0");
    }
    
    return true;
}

// ============================================================================
// Biometric HAL
// ============================================================================

bool HALSimulation::configureFingerprint(const QString& instanceId, bool enrolled) {
    HALState& state = m_halStates[instanceId];
    
    state.fingerprint.type = BiometricType::FINGERPRINT;
    state.fingerprint.strength = BiometricStrength::STRONG;
    state.fingerprint.isHardwarePresent = true;
    state.fingerprint.isEnabled = true;
    state.fingerprint.isEnrolled = enrolled;
    state.fingerprint.isLockout = false;
    state.fingerprint.failedAttempts = 0;
    state.fingerprint.maxEnrollments = 5;
    
    if (enrolled) {
        BiometricEnrolledFinger finger;
        finger.fingerId = 1;
        finger.name = "Right Thumb";
        finger.enrolledAt = QDateTime::currentMSecsSinceEpoch();
        state.fingerprint.enrolledFingers.append(finger);
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set fingerprint HAL properties
    QStringList cmds = {
        "setprop ro.hardware.fingerprint goodix",
        "setprop ro.hardware.fp.vendor 1",
        "setprop ro.fingerprint.optical false",
        "setprop ro.fingerprint.supports_udfps true",
        "setprop ro.fingerprint.delay_after_error 5000",
        "setprop ro.fingerprint.max_failed_attempts 5",
        "setprop ro.fingerprint.lockout_duration 30000",
        "settings put secure fingerprint_available_on_device true",
        "settings put secure fingerprint_always_required false",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool HALSimulation::enrollFingerprint(const QString& instanceId, int fingerId, const QString& name) {
    HALState& state = m_halStates[instanceId];
    
    if (state.fingerprint.enrolledFingers.size() >= state.fingerprint.maxEnrollments) {
        qWarning() << "Maximum fingerprint enrollments reached";
        return false;
    }
    
    BiometricEnrolledFinger finger;
    finger.fingerId = fingerId;
    finger.name = name;
    finger.enrolledAt = QDateTime::currentMSecsSinceEpoch();
    
    state.fingerprint.enrolledFingers.append(finger);
    state.fingerprint.isEnrolled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId,
        QString("settings put secure fingerprint_enrolled 1"));
    ctrl.executeShell(instanceId,
        QString("settings put secure fingerprint_count %1")
            .arg(state.fingerprint.enrolledFingers.size()));
    
    return true;
}

bool HALSimulation::configureFaceUnlock(const QString& instanceId, bool enrolled) {
    HALState& state = m_halStates[instanceId];
    
    state.faceUnlock.type = BiometricType::FACE;
    state.faceUnlock.strength = BiometricStrength::STRONG;
    state.faceUnlock.isHardwarePresent = true;
    state.faceUnlock.isEnabled = true;
    state.faceUnlock.isEnrolled = enrolled;
    state.faceUnlock.isLockout = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        "setprop ro.hardware.face_unlock qualcomm",
        "setprop ro.hardware.face version 2",
        "setprop ro.hardware.face.sfe true",
        "setprop ro.face.3d_recognition true",
        "setprop ro.face.supports_3d true",
        "setprop ro.face.supports_ir true",
        "setprop ro.face.supports_ambient true",
        "settings put secure face_available_on_device true",
        "settings put secure face_always_required false",
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

bool HALSimulation::enrollFace(const QString& instanceId) {
    HALState& state = m_halStates[instanceId];
    state.faceUnlock.isEnrolled = true;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure face_enrolled 1");
    
    return true;
}

bool HALSimulation::authenticateBiometric(const QString& instanceId, BiometricType type) {
    HALState& state = m_halStates[instanceId];
    
    BiometricConfig* config = nullptr;
    if (type == BiometricType::FINGERPRINT) {
        config = &state.fingerprint;
    } else if (type == BiometricType::FACE) {
        config = &state.faceUnlock;
    }
    
    if (!config) return false;
    
    if (config->isLockout) {
        qWarning() << "Biometric is locked out";
        return false;
    }
    
    if (!config->isEnrolled) {
        qWarning() << "Biometric not enrolled";
        return false;
    }
    
    // Simulate successful authentication
    config->failedAttempts = 0;
    
    return true;
}

bool HALSimulation::lockoutBiometric(const QString& instanceId, BiometricType type) {
    HALState& state = m_halStates[instanceId];
    
    if (type == BiometricType::FINGERPRINT) {
        state.fingerprint.isLockout = true;
        state.fingerprint.failedAttempts = 5;
    } else if (type == BiometricType::FACE) {
        state.faceUnlock.isLockout = true;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, "settings put secure biometric_lockout 1");
    
    return true;
}

// ============================================================================
// Audio HAL
// ============================================================================

bool HALSimulation::configureAudio(const QString& instanceId) {
    HALState& state = m_halStates[instanceId];
    
    state.audio.primaryOutput = "speaker";
    state.audio.primaryInput = "mic";
    state.audio.masterVolume = 70;
    state.audio.isMuted = false;
    state.audio.isSpeakerOn = true;
    state.audio.isHeadsetConnected = false;
    state.audio.isBluetoothConnected = false;
    state.audio.activeProfile = AudioProfile::DEFAULT;
    
    // Add default devices
    AudioDevice speaker;
    speaker.name = "Speaker";
    speaker.isConnected = true;
    speaker.isActive = true;
    speaker.volume = 70;
    state.audio.outputDevices.append(speaker);
    
    AudioDevice mic;
    mic.name = "Microphone";
    mic.isConnected = true;
    mic.isActive = true;
    state.audio.inputDevices.append(mic);
    
    return applyToInstance(instanceId);
}

bool HALSimulation::setSpeakerEnabled(const QString& instanceId, bool enabled) {
    HALState& state = m_halStates[instanceId];
    state.audio.isSpeakerOn = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId,
        QString("settings put system speaker_on %1").arg(enabled ? 1 : 0));
    
    return true;
}

bool HALSimulation::setHeadsetConnected(const QString& instanceId, bool connected) {
    HALState& state = m_halStates[instanceId];
    state.audio.isHeadsetConnected = connected;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (connected) {
        QStringList cmds = {
            "settings put secure headphone_plugged 1",
            "setprop persist.audio.headphone.inserted true",
            "setprop ro.headphone.type analog",
        };
        for (const QString& cmd : cmds) {
            ctrl.executeShell(instanceId, cmd);
        }
    } else {
        QStringList cmds = {
            "settings put secure headphone_plugged 0",
            "setprop persist.audio.headphone.inserted false",
        };
        for (const QString& cmd : cmds) {
            ctrl.executeShell(instanceId, cmd);
        }
    }
    
    return true;
}

bool HALSimulation::setBluetoothAudio(const QString& instanceId, bool connected) {
    HALState& state = m_halStates[instanceId];
    state.audio.isBluetoothConnected = connected;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (connected) {
        ctrl.executeShell(instanceId, "settings put global bluetooth_on 1");
        ctrl.executeShell(instanceId, "setprop ro.bluetooth.a2dp_enabled true");
    } else {
        ctrl.executeShell(instanceId, "setprop ro.bluetooth.a2dp_enabled false");
    }
    
    return true;
}

bool HALSimulation::setVolume(const QString& instanceId, int level) {
    level = qBound(0, level, 100);
    
    HALState& state = m_halStates[instanceId];
    state.audio.masterVolume = level;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList cmds = {
        QString("media volume --stream 3 --set %1").arg(level),  // Music
        QString("media volume --stream 2 --set %1").arg(level),  // Ring
        QString("media volume --stream 4 --set %1").arg(level),  // Alarm
        QString("media volume --stream 6 --set %1").arg(level),  // Notification
    };
    
    for (const QString& cmd : cmds) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

QMap<QString, QString> HALSimulation::getAllHALProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    HALState& state = m_halStates[instanceId];
    
    // Camera
    props["camera.back.available"] = state.backCamera.isAvailable ? "true" : "false";
    props["camera.front.available"] = state.frontCamera.isAvailable ? "true" : "false";
    props["camera.back.enabled"] = state.backCamera.isEnabled ? "true" : "false";
    props["camera.front.enabled"] = state.frontCamera.isEnabled ? "true" : "false";
    
    // Biometric
    props["biometric.fingerprint.hardware"] = state.fingerprint.isHardwarePresent ? "true" : "false";
    props["biometric.fingerprint.enrolled"] = state.fingerprint.isEnrolled ? "true" : "false";
    props["biometric.fingerprint.strength"] = 
        state.fingerprint.strength == BiometricStrength::STRONG ? "strong" : "weak";
    
    props["biometric.face.hardware"] = state.faceUnlock.isHardwarePresent ? "true" : "false";
    props["biometric.face.enrolled"] = state.faceUnlock.isEnrolled ? "true" : "false";
    
    // Audio
    props["audio.output.primary"] = state.audio.primaryOutput;
    props["audio.input.primary"] = state.audio.primaryInput;
    props["audio.headphone.connected"] = state.audio.isHeadsetConnected ? "true" : "false";
    props["audio.bluetooth.connected"] = state.audio.isBluetoothConnected ? "true" : "false";
    props["audio.volume.master"] = QString::number(state.audio.masterVolume);
    
    return props;
}

bool HALSimulation::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

bool HALSimulation::resetHAL(const QString& instanceId) {
    HALState defaultState;
    
    // Default cameras
    defaultState.backCamera = getCameraDefaults(CameraFacing::BACK, "Samsung", "SM-S928B");
    defaultState.frontCamera = getCameraDefaults(CameraFacing::FRONT, "Samsung", "SM-S928B");
    defaultState.backCamera.cameraId = 0;
    defaultState.frontCamera.cameraId = 1;
    
    // Default biometric (enrolled)
    configureFingerprint(instanceId, true);
    
    // Default audio
    configureAudio(instanceId);
    
    defaultState.allHALPresent = true;
    m_halStates[instanceId] = defaultState;
    
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

CameraConfig HALSimulation::getCameraDefaults(CameraFacing facing, const QString& manufacturer, const QString& model) {
    CameraConfig config;
    
    config.facing = facing;
    config.isAvailable = true;
    config.isEnabled = true;
    
    if (facing == CameraFacing::BACK) {
        config.cameraId = 0;
        config.cameraName = "main";
        config.cameraFullName = "Samsung Camera";
        
        config.capabilities.maxResolution = 200; // MP
        config.capabilities.minResolution = 1;
        config.capabilities.hasFlash = true;
        config.capabilities.hasAutofocus = true;
        config.capabilities.hasOpticalStabilization = true;
        config.capabilities.hasElectronicStabilization = true;
        config.capabilities.supportsHDR = true;
        config.capabilities.supportsNightMode = true;
        config.capabilities.supportsPortraitMode = true;
        config.capabilities.maxZoom = 100;
        config.capabilities.maxAperture = 1.8f;
    } else {
        config.cameraId = 1;
        config.cameraName = "front";
        config.cameraFullName = "Samsung Front Camera";
        
        config.capabilities.maxResolution = 12;
        config.capabilities.minResolution = 1;
        config.capabilities.hasFlash = false;
        config.capabilities.hasAutofocus = false;
        config.capabilities.hasOpticalStabilization = false;
        config.capabilities.hasElectronicStabilization = true;
        config.capabilities.supportsHDR = true;
        config.capabilities.supportsNightMode = true;
        config.capabilities.supportsPortraitMode = true;
        config.capabilities.maxZoom = 10;
        config.capabilities.maxAperture = 2.4f;
    }
    
    config.capabilities.supportedModes = "auto,manual,portrait,night,panorama,hdr,video,slowmo";
    config.capabilities.supportedSceneModes = "auto,landscape,portrait,night,beach,snow,sunset,fireworks,sports";
    config.capabilities.supportedWhiteBalance = "auto,daylight,cloudy,tungsten,fluorescent,shade";
    
    return config;
}

BiometricConfig HALSimulation::getBiometricDefaults(BiometricType type) {
    BiometricConfig config;
    
    config.type = type;
    config.strength = BiometricStrength::STRONG;
    config.isHardwarePresent = true;
    config.isEnabled = true;
    config.isEnrolled = false;
    config.isLockout = false;
    config.failedAttempts = 0;
    
    if (type == BiometricType::FINGERPRINT) {
        config.maxEnrollments = 5;
    }
    
    return config;
}

} // namespace VirtualPhonePro
