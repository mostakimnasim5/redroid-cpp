/**
 * @file ScreenStateManager.cpp
 * @brief Realistic Screen State & Brightness Implementation
 */

#include "VirtualPhonePro/ScreenStateManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

namespace VirtualPhonePro {

ScreenStateManager* ScreenStateManager::s_instance = nullptr;

ScreenStateManager& ScreenStateManager::instance() {
    if (!s_instance) {
        s_instance = new ScreenStateManager();
    }
    return *s_instance;
}

ScreenStateManager::ScreenStateManager() {
    // Initialize default brightness profile
    m_brightnessProfile.name = "Android Auto";
    m_brightnessProfile.minBrightness = 2;
    m_brightnessProfile.maxBrightness = 255;
    m_brightnessProfile.defaultBrightness = 128;
    m_brightnessProfile.transitionDuration = 500;
    
    // Typical brightness curve for auto brightness
    m_brightnessProfile.luxToBrightness = {
        {0, 5},      // Darkness
        {10, 15},    // Dim room
        {50, 50},    // Normal indoor
        {100, 100},  // Bright indoor
        {500, 150},  // Cloudy outdoor
        {1000, 200}, // Bright outdoor
        {5000, 230}, // Very bright
        {10000, 255} // Direct sunlight
    };
}

// ============================================================================
// Configuration
// ============================================================================

bool ScreenStateManager::configureForDevice(const QString& manufacturer, const QString& model) {
    qDebug() << "Configuring screen for:" << manufacturer << model;
    
    ScreenInfo defaults = getDeviceDefaults(manufacturer, model);
    return true;
}

bool ScreenStateManager::setScreenInfo(const QString& instanceId, const ScreenInfo& info) {
    m_screenStates[instanceId] = info;
    return applyToInstance(instanceId);
}

ScreenInfo ScreenStateManager::getScreenInfo(const QString& instanceId) const {
    if (m_screenStates.contains(instanceId)) {
        return m_screenStates[instanceId];
    }
    
    // Return default screen info
    ScreenInfo defaultInfo;
    defaultInfo.state = ScreenState::ON;
    defaultInfo.displayState = DisplayState::ON;
    defaultInfo.isOn = true;
    defaultInfo.isDozing = false;
    defaultInfo.brightness = 128;
    defaultInfo.autoBrightness = 128;
    defaultInfo.actualBrightness = 128;
    defaultInfo.isAutoBrightness = true;
    defaultInfo.lightLevel = AmbientLightLevel::NORMAL;
    defaultInfo.luxReading = 500;
    defaultInfo.luxMin = 1;
    defaultInfo.luxMax = 30000;
    defaultInfo.luxCurrent = 500;
    defaultInfo.width = 1440;
    defaultInfo.height = 3120;
    defaultInfo.densityDpi = 480;
    defaultInfo.refreshRate = 120;
    defaultInfo.panelType = "AMOLED";
    defaultInfo.resolution = "1440x3120";
    defaultInfo.hdrSupported = 1;
    defaultInfo.dolbyVisionSupported = 1;
    defaultInfo.screenTimeout = 30000;
    defaultInfo.screenOffTimeout = 30000;
    defaultInfo.isPowerSaving = false;
    defaultInfo.isHBMEnabled = false;
    defaultInfo.lastUserActivity = QDateTime::currentMSecsSinceEpoch();
    defaultInfo.lastInputEvent = QDateTime::currentMSecsSinceEpoch();
    defaultInfo.screenOnTime = QDateTime::currentMSecsSinceEpoch();
    defaultInfo.totalScreenOnTime = 0;
    
    return defaultInfo;
}

bool ScreenStateManager::applyToInstance(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    ScreenInfo info = getScreenInfo(instanceId);
    
    QStringList commands = {
        // Brightness settings
        QString("settings put system screen_brightness %1").arg(info.brightness),
        QString("settings put system screen_brightness_for_apps %1").arg(info.brightness),
        QString("settings put system screen_brightness_mode %1").arg(info.isAutoBrightness ? 1 : 0),
        QString("settings put system screen_auto_brightness_adj %1").arg(info.autoBrightness),
        
        // Brightness for different states
        QString("settings put system screen_brightness_for_apps_sleep %1").arg(info.brightness),
        
        // Display power state
        QString("settings put system display_low_power %1").arg(info.isPowerSaving ? 1 : 0),
        
        // Screen timeout
        QString("settings put system screen_off_timeout %1").arg(info.screenTimeout),
        
        // Light sensor
        QString("settings put system screen_auto_brightness_default %1").arg(info.autoBrightness),
        QString("settings put system screen_auto_brightness_low %1").arg(10),
        QString("settings put system screen_auto_brightness_high %1").arg(200),
        
        // Power state
        "dumpsys power setDisplayState on",
    };
    
    // Apply power modes
    if (info.isPowerSaving) {
        commands.append("settings put global low_power_mode 1");
    }
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    // Set system properties
    QStringList props = {
        QString("setprop persist.sys.screen.brightness %1").arg(info.brightness),
        QString("setprop persist.sys.screen.auto %1").arg(info.isAutoBrightness ? "true" : "false"),
        QString("setprop hw.screen.on %1").arg(info.isOn ? "true" : "false"),
        QString("setprop hw.display.state %1").arg(
            info.isOn ? "on" : (info.isDozing ? "doze" : "off")),
    };
    
    for (const QString& prop : props) {
        ctrl.executeShell(instanceId, prop);
    }
    
    qDebug() << "Screen state applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Screen State Control
// ============================================================================

bool ScreenStateManager::screenOn(const QString& instanceId) {
    ScreenInfo& info = m_screenStates[instanceId];
    info.isOn = true;
    info.state = ScreenState::ON;
    info.displayState = DisplayState::ON;
    info.isDozing = false;
    info.screenOnTime = QDateTime::currentMSecsSinceEpoch();
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Wake up display
    ctrl.executeShell(instanceId, "input keyevent 82"); // Wake up
    ctrl.executeShell(instanceId, "wm dismiss-keyguard");
    ctrl.executeShell(instanceId, "dumpsys power setDisplayState on");
    
    // Record event
    ScreenEvent event;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.fromState = ScreenState::OFF;
    event.toState = ScreenState::ON;
    event.trigger = "user";
    
    if (!m_screenEvents.contains(instanceId)) {
        m_screenEvents[instanceId] = QList<ScreenEvent>();
    }
    m_screenEvents[instanceId].append(event);
    
    return applyToInstance(instanceId);
}

bool ScreenStateManager::screenOff(const QString& instanceId) {
    ScreenInfo& info = m_screenStates[instanceId];
    info.isOn = false;
    info.state = ScreenState::OFF;
    info.displayState = DisplayState::OFF;
    info.isDozing = false;
    info.screenOffTime = QDateTime::currentMSecsSinceEpoch();
    info.totalScreenOnTime += (info.screenOffTime - info.screenOnTime);
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Turn off display
    ctrl.executeShell(instanceId, "input keyevent 26"); // Power key
    ctrl.executeShell(instanceId, "dumpsys power setDisplayState off");
    
    // Record event
    ScreenEvent event;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.fromState = ScreenState::ON;
    event.toState = ScreenState::OFF;
    event.trigger = "user";
    
    if (!m_screenEvents.contains(instanceId)) {
        m_screenEvents[instanceId] = QList<ScreenEvent>();
    }
    m_screenEvents[instanceId].append(event);
    
    return applyToInstance(instanceId);
}

bool ScreenStateManager::toggleScreen(const QString& instanceId) {
    ScreenInfo info = getScreenInfo(instanceId);
    if (info.isOn) {
        return screenOff(instanceId);
    } else {
        return screenOn(instanceId);
    }
}

bool ScreenStateManager::enterDoze(const QString& instanceId) {
    ScreenInfo& info = m_screenStates[instanceId];
    info.isOn = true;
    info.isDozing = true;
    info.state = ScreenState::DOZE;
    info.displayState = DisplayState::DOZE;
    info.brightness = 10; // Very dim in doze
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "dumpsys deviceidle enable");
    ctrl.executeShell(instanceId, "dumpsys deviceidle step");
    
    return applyToInstance(instanceId);
}

bool ScreenStateManager::exitDoze(const QString& instanceId) {
    ScreenInfo& info = m_screenStates[instanceId];
    info.isDozing = false;
    info.state = ScreenState::ON;
    info.displayState = DisplayState::ON;
    info.brightness = info.autoBrightness;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "dumpsys deviceidle disable");
    ctrl.executeShell(instanceId, "input keyevent 82");
    
    return applyToInstance(instanceId);
}

bool ScreenStateManager::simulateUserActivity(const QString& instanceId) {
    ScreenInfo& info = m_screenStates[instanceId];
    info.lastUserActivity = QDateTime::currentMSecsSinceEpoch();
    info.lastInputEvent = QDateTime::currentMSecsSinceEpoch();
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Send some activity to prevent screen from turning off
    ctrl.executeShell(instanceId, "input tap 540 960"); // Tap center
    ctrl.executeShell(instanceId, "dumpsys activity activityResumed");
    
    return true;
}

// ============================================================================
// Brightness Control
// ============================================================================

bool ScreenStateManager::setBrightness(const QString& instanceId, int brightness) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.brightness = qBound(1, brightness, 255);
    info.actualBrightness = info.brightness;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, 
        QString("settings put system screen_brightness %1").arg(info.brightness));
    
    return true;
}

bool ScreenStateManager::enableAutoBrightness(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isAutoBrightness = true;
    info.actualBrightness = info.autoBrightness;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put system screen_brightness_mode 1");
    
    return true;
}

bool ScreenStateManager::disableAutoBrightness(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isAutoBrightness = false;
    info.actualBrightness = info.brightness;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put system screen_brightness_mode 0");
    
    return true;
}

bool ScreenStateManager::setAmbientLight(const QString& instanceId, int lux) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.luxCurrent = lux;
    info.luxReading = lux;
    
    // Update light level
    if (lux < 10) {
        info.lightLevel = AmbientLightLevel::DARKNESS;
    } else if (lux < 100) {
        info.lightLevel = AmbientLightLevel::DIM;
    } else if (lux < 1000) {
        info.lightLevel = AmbientLightLevel::NORMAL;
    } else if (lux < 10000) {
        info.lightLevel = AmbientLightLevel::BRIGHT;
    } else {
        info.lightLevel = AmbientLightLevel::OUTDOOR;
    }
    
    // Calculate auto brightness if enabled
    if (info.isAutoBrightness) {
        info.autoBrightness = calculateAutoBrightness(lux, m_brightnessProfile);
        info.actualBrightness = info.autoBrightness;
    }
    
    // Apply to instance via light sensor
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set sensor values
    ctrl.executeShell(instanceId, 
        QString("settings put system screen_auto_brightness_lux %1").arg(lux));
    
    // Set brightness
    if (info.isAutoBrightness) {
        ctrl.executeShell(instanceId,
            QString("settings put system screen_brightness %1").arg(info.autoBrightness));
    }
    
    return true;
}

bool ScreenStateManager::setAmbientLightLevel(const QString& instanceId, AmbientLightLevel level) {
    int lux;
    
    switch (level) {
        case AmbientLightLevel::DARKNESS:
            lux = QRandomGenerator::global()->bounded(0, 10);
            break;
        case AmbientLightLevel::DIM:
            lux = QRandomGenerator::global()->bounded(10, 100);
            break;
        case AmbientLightLevel::NORMAL:
            lux = QRandomGenerator::global()->bounded(100, 1000);
            break;
        case AmbientLightLevel::BRIGHT:
            lux = QRandomGenerator::global()->bounded(1000, 10000);
            break;
        case AmbientLightLevel::OUTDOOR:
            lux = QRandomGenerator::global()->bounded(10000, 30000);
            break;
        default:
            lux = 500;
    }
    
    return setAmbientLight(instanceId, lux);
}

bool ScreenStateManager::simulateBrightnessChange(const QString& instanceId, int targetBrightness, int durationMs) {
    ScreenInfo info = getScreenInfo(instanceId);
    int currentBrightness = info.brightness;
    int steps = qMax(1, durationMs / 50);
    int stepSize = (targetBrightness - currentBrightness) / steps;
    
    for (int i = 0; i < steps; i++) {
        currentBrightness += stepSize;
        setBrightness(instanceId, currentBrightness);
        QThread::msleep(50);
    }
    
    return setBrightness(instanceId, targetBrightness);
}

// ============================================================================
// Display Properties
// ============================================================================

bool ScreenStateManager::setRefreshRate(const QString& instanceId, int hz) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.refreshRate = hz;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Set refresh rate via wm
    ctrl.executeShell(instanceId, QString("wm refreshrate %1").arg(hz));
    ctrl.executeShell(instanceId, 
        QString("settings put system peak_refresh_rate %1").arg(hz));
    ctrl.executeShell(instanceId,
        QString("settings put system minimal_refresh_rate %1").arg(hz));
    
    return true;
}

bool ScreenStateManager::enableHBM(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isHBMEnabled = true;
    info.brightness = 255; // Maximum brightness
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put system screen_brightness 255");
    ctrl.executeShell(instanceId, "settings put global peak_refresh_rate 120");
    ctrl.executeShell(instanceId, "settings put system high_brightness_mode 1");
    
    return true;
}

bool ScreenStateManager::disableHBM(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isHBMEnabled = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put system high_brightness_mode 0");
    
    return true;
}

bool ScreenStateManager::setScreenMode(const QString& instanceId, ScreenMode mode) {
    QString modeName;
    
    switch (mode) {
        case ScreenMode::STANDARD:
            modeName = "standard";
            break;
        case ScreenMode::VIVID:
            modeName = "vivid";
            break;
        case ScreenMode::NATURAL:
            modeName = "natural";
            break;
        case ScreenMode::READING:
            modeName = "reading";
            break;
        case ScreenMode::NIGHT_MODE:
            modeName = "night";
            break;
        default:
            modeName = "standard";
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId,
        QString("settings put system screen_mode %1").arg(modeName));
    ctrl.executeShell(instanceId,
        QString("settings put system display_mode %1").arg(modeName));
    
    return true;
}

bool ScreenStateManager::enableNightMode(const QString& instanceId, int intensity) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Intensity: 0-100
    ctrl.executeShell(instanceId,
        QString("settings put secure night_display_activated 1"));
    ctrl.executeShell(instanceId,
        QString("settings put secure night_display_auto_mode 0"));
    ctrl.executeShell(instanceId,
        QString("settings put secure night_display_custom_start_time 0"));
    ctrl.executeShell(instanceId,
        QString("settings put secure night_display_custom_end_time 1440"));
    ctrl.executeShell(instanceId,
        QString("settings put secure night_display_color_temperature %1").arg(2500 + intensity * 15));
    
    return true;
}

bool ScreenStateManager::disableNightMode(const QString& instanceId) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put secure night_display_activated 0");
    
    return true;
}

bool ScreenStateManager::setScreenTimeout(const QString& instanceId, int timeoutMs) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.screenTimeout = timeoutMs;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId,
        QString("settings put system screen_off_timeout %1").arg(timeoutMs));
    
    return true;
}

// ============================================================================
// Power Saving
// ============================================================================

bool ScreenStateManager::enablePowerSaving(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isPowerSaving = true;
    
    // Reduce refresh rate
    if (info.refreshRate > 60) {
        setRefreshRate(instanceId, 60);
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put global low_power_mode 1");
    ctrl.executeShell(instanceId, "dumpsys battery set power_save_mode 1");
    
    return true;
}

bool ScreenStateManager::disablePowerSaving(const QString& instanceId) {
    if (!m_screenStates.contains(instanceId)) {
        return false;
    }
    
    ScreenInfo& info = m_screenStates[instanceId];
    info.isPowerSaving = false;
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    ctrl.executeShell(instanceId, "settings put global low_power_mode 0");
    ctrl.executeShell(instanceId, "dumpsys battery set power_save_mode 0");
    
    return true;
}

bool ScreenStateManager::setAODEnabled(const QString& instanceId, bool enabled) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    if (enabled) {
        ctrl.executeShell(instanceId, "settings put secure doze_always_on 1");
        ctrl.executeShell(instanceId, "settings put secure ambient_display_always_on 1");
    } else {
        ctrl.executeShell(instanceId, "settings put secure doze_always_on 0");
        ctrl.executeShell(instanceId, "settings put secure ambient_display_always_on 0");
    }
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

QMap<QString, QString> ScreenStateManager::getAllScreenProperties(const QString& instanceId) {
    QMap<QString, QString> props;
    ScreenInfo info = getScreenInfo(instanceId);
    
    props["screen.state"] = info.isOn ? "on" : "off";
    props["screen.brightness"] = QString::number(info.brightness);
    props["screen.brightness.auto"] = info.isAutoBrightness ? "true" : "false";
    props["screen.brightness.auto_value"] = QString::number(info.autoBrightness);
    props["screen.lux"] = QString::number(info.luxCurrent);
    props["screen.refresh_rate"] = QString::number(info.refreshRate);
    props["screen.timeout"] = QString::number(info.screenTimeout);
    props["screen.resolution"] = info.resolution;
    props["screen.panel"] = info.panelType;
    props["screen.hbm"] = info.isHBMEnabled ? "true" : "false";
    props["screen.power_saving"] = info.isPowerSaving ? "true" : "false";
    props["screen.always_on"] = "false"; // AOD
    
    props["persist.sys.screen.brightness"] = QString::number(info.brightness);
    props["persist.sys.screen.auto"] = info.isAutoBrightness ? "true" : "false";
    props["hw.screen.on"] = info.isOn ? "true" : "false";
    
    props["window_manager.refresh_rate"] = QString::number(info.refreshRate);
    props["display_metrics.width"] = QString::number(info.width);
    props["display_metrics.height"] = QString::number(info.height);
    props["display_metrics.density"] = QString::number(info.densityDpi);
    
    return props;
}

bool ScreenStateManager::applyAllSpoofing(const QString& instanceId) {
    return applyToInstance(instanceId);
}

QJsonObject ScreenStateManager::getScreenStats(const QString& instanceId) {
    QJsonObject stats;
    ScreenInfo info = getScreenInfo(instanceId);
    
    stats["screen_on"] = info.isOn;
    stats["current_brightness"] = info.brightness;
    stats["auto_brightness"] = info.autoBrightness;
    stats["refresh_rate"] = info.refreshRate;
    stats["lux"] = info.luxCurrent;
    stats["light_level"] = ambientLevelToString(info.lightLevel);
    stats["hbm_enabled"] = info.isHBMEnabled;
    stats["power_saving"] = info.isPowerSaving;
    
    // Calculate total on time
    qint64 currentOnTime = 0;
    if (info.isOn) {
        currentOnTime = QDateTime::currentMSecsSinceEpoch() - info.screenOnTime;
    }
    qint64 totalOnTime = info.totalScreenOnTime + currentOnTime;
    
    stats["total_screen_on_time_ms"] = totalOnTime;
    stats["total_screen_on_time_min"] = totalOnTime / 60000;
    
    // Event count
    if (m_screenEvents.contains(instanceId)) {
        stats["screen_events_count"] = m_screenEvents[instanceId].size();
    }
    
    return stats;
}

bool ScreenStateManager::resetScreen(const QString& instanceId) {
    ScreenInfo defaultInfo;
    defaultInfo.state = ScreenState::ON;
    defaultInfo.displayState = DisplayState::ON;
    defaultInfo.isOn = true;
    defaultInfo.brightness = 128;
    defaultInfo.autoBrightness = 128;
    defaultInfo.actualBrightness = 128;
    defaultInfo.isAutoBrightness = true;
    defaultInfo.refreshRate = 120;
    defaultInfo.screenTimeout = 30000;
    defaultInfo.isPowerSaving = false;
    defaultInfo.isHBMEnabled = false;
    defaultInfo.luxCurrent = 500;
    
    m_screenStates[instanceId] = defaultInfo;
    
    return applyToInstance(instanceId);
}

// ============================================================================
// Private Helpers
// ============================================================================

int ScreenStateManager::calculateAutoBrightness(int lux, const BrightnessProfile& profile) {
    // Find appropriate brightness for lux value
    int lowerLux = 0;
    int lowerBrightness = profile.minBrightness;
    int upperLux = 10000;
    int upperBrightness = profile.maxBrightness;
    
    for (auto it = profile.luxToBrightness.begin(); it != profile.luxToBrightness.end(); ++it) {
        if (it.key() <= lux) {
            lowerLux = it.key();
            lowerBrightness = it.value();
        } else {
            upperLux = it.key();
            upperBrightness = it.value();
            break;
        }
    }
    
    // Interpolate
    if (upperLux == lowerLux) {
        return lowerBrightness;
    }
    
    double ratio = double(lux - lowerLux) / double(upperLux - lowerLux);
    return qBound(profile.minBrightness,
                  int(lowerBrightness + ratio * (upperBrightness - lowerBrightness)),
                  profile.maxBrightness);
}

QString ScreenStateManager::ambientLevelToString(AmbientLightLevel level) const {
    switch (level) {
        case AmbientLightLevel::DARKNESS: return "darkness";
        case AmbientLightLevel::DIM: return "dim";
        case AmbientLightLevel::NORMAL: return "normal";
        case AmbientLightLevel::BRIGHT: return "bright";
        case AmbientLightLevel::OUTDOOR: return "outdoor";
        default: return "unknown";
    }
}

ScreenInfo ScreenStateManager::getDeviceDefaults(const QString& manufacturer, const QString& model) {
    ScreenInfo info;
    
    if (manufacturer.contains("samsung", Qt::CaseInsensitive)) {
        info.width = 1440;
        info.height = 3120;
        info.densityDpi = 480;
        info.refreshRate = 120;
        info.panelType = "AMOLED";
        info.resolution = "1440x3120";
        info.hdrSupported = 1;
        info.dolbyVisionSupported = 1;
    } else if (manufacturer.contains("google", Qt::CaseInsensitive) ||
               manufacturer.contains("pixel", Qt::CaseInsensitive)) {
        info.width = 1344;
        info.height = 2992;
        info.densityDpi = 480;
        info.refreshRate = 120;
        info.panelType = "LTPO OLED";
        info.resolution = "1344x2992";
        info.hdrSupported = 1;
        info.dolbyVisionSupported = 0;
    } else if (manufacturer.contains("xiaomi", Qt::CaseInsensitive)) {
        info.width = 1440;
        info.height = 3200;
        info.densityDpi = 520;
        info.refreshRate = 120;
        info.panelType = "AMOLED";
        info.resolution = "1440x3200";
        info.hdrSupported = 1;
        info.dolbyVisionSupported = 1;
    } else {
        // Default
        info.width = 1080;
        info.height = 2400;
        info.densityDpi = 400;
        info.refreshRate = 60;
        info.panelType = "LCD";
        info.resolution = "1080x2400";
        info.hdrSupported = 0;
        info.dolbyVisionSupported = 0;
    }
    
    return info;
}

} // namespace VirtualPhonePro
