/**
 * @file ScreenStateManager.h
 * @brief Realistic Screen State & Brightness Simulation
 * @version 2.0.0
 * 
 * Provides realistic screen states, brightness levels, refresh rates,
 * and ambient light sensor simulation for authentic device behavior.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_SCREEN_STATE_MANAGER_H
#define VIRTUALPHONEPRO_SCREEN_STATE_MANAGER_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

namespace VirtualPhonePro {

// Screen state
enum class ScreenState {
    OFF,
    ON,
    DOZE,
    SUSPEND
};

// Display state
enum class DisplayState {
    OFF,
    ON,
    VR,
    HBM,
    AoD,      // Always on Display
    LOW_POWER
};

// Screen mode
enum class ScreenMode {
    STANDARD,
    VIVID,
    NATURAL,
    READING,
    NIGHT_MODE
};

// Ambient light level
enum class AmbientLightLevel {
    DARKNESS,     // < 10 lux
    DIM,           // 10-100 lux
    NORMAL,        // 100-1000 lux
    BRIGHT,        // 1000-10000 lux
    OUTDOOR        // > 10000 lux
};

// Complete screen state
struct ScreenInfo {
    // Basic state
    ScreenState state;
    DisplayState displayState;
    bool isOn;
    bool isDozing;
    
    // Brightness
    int brightness;           // 0-255 (manual)
    int autoBrightness;       // 0-255 (auto-adjusted)
    int actualBrightness;     // Current effective brightness
    bool isAutoBrightness;
    
    // Light sensor
    AmbientLightLevel lightLevel;
    int luxReading;          // Actual lux value
    int luxMin;              // Sensor min
    int luxMax;              // Sensor max
    int luxCurrent;          // Current reading
    
    // Display properties
    int width;
    int height;
    int densityDpi;
    int refreshRate;         // Hz (60, 90, 120, 144)
    QString panelType;      // AMOLED, LCD, OLED
    QString resolution;      // "1440x3120"
    int hdrSupported;
    int dolbyVisionSupported;
    
    // Screen timeout
    int screenTimeout;        // ms
    int screenOffTimeout;    // ms
    
    // Power saving
    bool isPowerSaving;
    bool isHBMEnabled;       // High Brightness Mode
    
    // User activity
    qint64 lastUserActivity;
    qint64 lastInputEvent;
    
    // Timestamps
    qint64 screenOnTime;
    qint64 screenOffTime;
    qint64 totalScreenOnTime;
};

// Screen-on event for realistic behavior
struct ScreenEvent {
    qint64 timestamp;
    ScreenState fromState;
    ScreenState toState;
    QString trigger;         // "user", "proximity", "scheduled", "notification"
};

// Brightness profile
struct BrightnessProfile {
    QString name;
    
    // Brightness curve
    QMap<int, int> luxToBrightness;  // lux -> brightness
    
    // Timing
    int transitionDuration;  // ms
    
    // Defaults
    int minBrightness;
    int maxBrightness;
    int defaultBrightness;
};

class ScreenStateManager {
public:
    static ScreenStateManager& instance();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Configure screen for device model
     */
    bool configureForDevice(const QString& manufacturer, const QString& model);
    
    /**
     * @brief Set screen info
     */
    bool setScreenInfo(const QString& instanceId, const ScreenInfo& info);
    
    /**
     * @brief Get current screen info
     */
    ScreenInfo getScreenInfo(const QString& instanceId) const;
    
    /**
     * @brief Apply screen configuration to instance
     */
    bool applyToInstance(const QString& instanceId);
    
    // =========================================================================
    // Screen State Control
    // =========================================================================
    
    /**
     * @brief Turn screen on
     */
    bool screenOn(const QString& instanceId);
    
    /**
     * @brief Turn screen off
     */
    bool screenOff(const QString& instanceId);
    
    /**
     * @brief Toggle screen state
     */
    bool toggleScreen(const QString& instanceId);
    
    /**
     * @brief Enable doze mode
     */
    bool enterDoze(const QString& instanceId);
    
    /**
     * @brief Exit doze mode
     */
    bool exitDoze(const QString& instanceId);
    
    /**
     * @brief Simulate user activity (keeps screen on)
     */
    bool simulateUserActivity(const QString& instanceId);
    
    // =========================================================================
    // Brightness Control
    // =========================================================================
    
    /**
     * @brief Set manual brightness
     */
    bool setBrightness(const QString& instanceId, int brightness);
    
    /**
     * @brief Enable auto brightness
     */
    bool enableAutoBrightness(const QString& instanceId);
    
    /**
     * @brief Disable auto brightness
     */
    bool disableAutoBrightness(const QString& instanceId);
    
    /**
     * @brief Set ambient light level
     */
    bool setAmbientLight(const QString& instanceId, int lux);
    
    /**
     * @brief Set ambient light level by category
     */
    bool setAmbientLightLevel(const QString& instanceId, AmbientLightLevel level);
    
    /**
     * @brief Simulate brightness transition
     */
    bool simulateBrightnessChange(const QString& instanceId, int targetBrightness, int durationMs);
    
    // =========================================================================
    // Display Properties
    // =========================================================================
    
    /**
     * @brief Set refresh rate
     */
    bool setRefreshRate(const QString& instanceId, int hz);
    
    /**
     * @brief Enable high brightness mode (outdoor)
     */
    bool enableHBM(const QString& instanceId);
    
    /**
     * @brief Disable high brightness mode
     */
    bool disableHBM(const QString& instanceId);
    
    /**
     * @brief Set screen mode (vivid, natural, etc.)
     */
    bool setScreenMode(const QString& instanceId, ScreenMode mode);
    
    /**
     * @brief Enable night mode (blue light filter)
     */
    bool enableNightMode(const QString& instanceId, int intensity);
    
    /**
     * @brief Disable night mode
     */
    bool disableNightMode(const QString& instanceId);
    
    /**
     * @brief Set screen timeout
     */
    bool setScreenTimeout(const QString& instanceId, int timeoutMs);
    
    // =========================================================================
    // Power Saving
    // =========================================================================
    
    /**
     * @brief Enable power saving mode
     */
    bool enablePowerSaving(const QString& instanceId);
    
    /**
     * @brief Disable power saving mode
     */
    bool disablePowerSaving(const QString& instanceId);
    
    /**
     * @brief Enable/disable AOD (Always on Display)
     */
    bool setAODEnabled(const QString& instanceId, bool enabled);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get all screen properties for spoofing
     */
    QMap<QString, QString> getAllScreenProperties(const QString& instanceId);
    
    /**
     * @brief Apply all screen spoofing
     */
    bool applyAllSpoofing(const QString& instanceId);
    
    /**
     * @brief Get screen statistics
     */
    QJsonObject getScreenStats(const QString& instanceId);
    
    /**
     * @brief Reset screen to default
     */
    bool resetScreen(const QString& instanceId);
    
private:
    ScreenStateManager();
    
    int calculateAutoBrightness(int lux, const BrightnessProfile& profile);
    QString ambientLevelToString(AmbientLightLevel level) const;
    ScreenInfo getDeviceDefaults(const QString& manufacturer, const QString& model);
    
    QMap<QString, ScreenInfo> m_screenStates;
    QMap<QString, QList<ScreenEvent>> m_screenEvents;
    BrightnessProfile m_brightnessProfile;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SCREEN_STATE_MANAGER_H
