/**
 * @file FridaXposedDetector.h
 * @brief Frida & Xposed Detection Bypass
 * @version 4.0.0
 * 
 * Provides comprehensive detection bypass for:
 * - Frida detection
 * - Xposed detection
 * - SSL pinning bypass
 * - Debug detection
 * - Root detection
 * - Emulator detection
 */

#pragma once

#ifndef VIRTUALPHONEPRO_FRIDA_XPOSED_DETECTOR_H
#define VIRTUALPHONEPRO_FRIDA_XPOSED_DETECTOR_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QStringList>
#include <QList>

namespace VirtualPhonePro {

// Detection Type
enum class FridaDetectionType {
    FRIDA,
    XPOSED,
    SUBSTRATE,
    MAGISK,
    ROOT,
    DEBUG,
    EMULATOR,
    SSL_PINNING,
    HOOK
};

// Detection Bypass Configuration
struct BypassConfig {
    FridaDetectionType type;
    bool isEnabled;
    bool isAutoApply;
    QString bypassMethod;
    QStringList targetPackages;
};

// SSL Pinning Bypass
struct SSLPinningBypass {
    QString domain;
    QStringList domains;
    bool bypassEnabled;
    QString certificateData;
};

// Hook Detection Bypass
struct HookBypass {
    QString hookType;  // "frida", "xposed", "substrate", "native"
    bool detectNativeHooks;
    bool detectJavaHooks;
    bool bypassDetections;
};

// Complete Detection State
struct DetectionBypassState {
    QString instanceId;
    QList<BypassConfig> bypassConfigs;
    QList<SSLPinningBypass> sslPinningBypasses;
    QList<HookBypass> hookBypasses;
    bool isInitialized;
    bool isActive;
    int totalBypasses;
    int activeBypasses;
};

// Detection Callback
typedef std::function<void(FridaDetectionType, bool)> DetectionBypassCallback;

class FridaXposedDetector {
public:
    static FridaXposedDetector& instance();
    
    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize bypass system
     */
    bool initialize(const QString& instanceId);
    
    /**
     * @brief Apply all bypasses
     */
    bool applyAllBypasses(const QString& instanceId);
    
    // =========================================================================
    // Frida Detection Bypass
    // =========================================================================
    
    /**
     * @brief Enable Frida bypass
     */
    bool enableFridaBypass(const QString& instanceId);
    
    /**
     * @brief Disable Frida bypass
     */
    bool disableFridaBypass(const QString& instanceId);
    
    /**
     * @brief Apply Frida-specific bypasses
     */
    bool applyFridaBypass(const QString& instanceId);
    
    // =========================================================================
    // Xposed Detection Bypass
    // =========================================================================
    
    /**
     * @brief Enable Xposed bypass
     */
    bool enableXposedBypass(const QString& instanceId);
    
    /**
     * @brief Disable Xposed bypass
     */
    bool disableXposedBypass(const QString& instanceId);
    
    /**
     * @brief Apply Xposed-specific bypasses
     */
    bool applyXposedBypass(const QString& instanceId);
    
    // =========================================================================
    // Root Detection Bypass
    // =========================================================================
    
    /**
     * @brief Enable root detection bypass
     */
    bool enableRootBypass(const QString& instanceId);
    
    /**
     * @brief Disable root detection bypass
     */
    bool disableRootBypass(const QString& instanceId);
    
    /**
     * @brief Apply root bypass
     */
    bool applyRootBypass(const QString& instanceId);
    
    // =========================================================================
    // Debug Detection Bypass
    // =========================================================================
    
    /**
     * @brief Enable debug bypass
     */
    bool enableDebugBypass(const QString& instanceId);
    
    /**
     * @brief Disable debug bypass
     */
    bool disableDebugBypass(const QString& instanceId);
    
    /**
     * @brief Apply debug bypass
     */
    bool applyDebugBypass(const QString& instanceId);
    
    // =========================================================================
    // Emulator Detection Bypass
    // =========================================================================
    
    /**
     * @brief Enable emulator bypass
     */
    bool enableEmulatorBypass(const QString& instanceId);
    
    /**
     * @brief Disable emulator bypass
     */
    bool disableEmulatorBypass(const QString& instanceId);
    
    /**
     * @brief Apply emulator bypass
     */
    bool applyEmulatorBypass(const QString& instanceId);
    
    // =========================================================================
    // SSL Pinning Bypass
    // =========================================================================
    
    /**
     * @brief Add SSL pinning bypass
     */
    bool addSSLPinningBypass(const QString& instanceId, const SSLPinningBypass& bypass);
    
    /**
     * @brief Remove SSL pinning bypass
     */
    bool removeSSLPinningBypass(const QString& instanceId, const QString& domain);
    
    /**
     * @brief Apply SSL pinning bypasses
     */
    bool applySSLPinningBypass(const QString& instanceId);
    
    /**
     * @brief Get all SSL bypasses
     */
    QList<SSLPinningBypass> getSSLPinningBypasses(const QString& instanceId) const;
    
    // =========================================================================
    // Hook Detection Bypass
    // =========================================================================
    
    /**
     * @brief Configure hook bypass
     */
    bool configureHookBypass(const QString& instanceId, const HookBypass& bypass);
    
    /**
     * @brief Apply hook bypasses
     */
    bool applyHookBypass(const QString& instanceId);
    
    // =========================================================================
    // Application-Specific Bypass
    // =========================================================================
    
    /**
     * @brief Apply bypasses for specific app
     */
    bool applyBypassForApp(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Get bypass status for app
     */
    bool isBypassActiveForApp(const QString& instanceId, const QString& packageName) const;
    
    // =========================================================================
    // Hook Callbacks
    // =========================================================================
    
    /**
     * @brief Set detection callback
     */
    void setDetectionCallback(DetectionBypassCallback callback);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    /**
     * @brief Get bypass state
     */
    DetectionBypassState getBypassState(const QString& instanceId) const;
    
    /**
     * @brief Get bypass state as JSON
     */
    QJsonObject getBypassStateJSON(const QString& instanceId) const;
    
    /**
     * @brief Reset to defaults
     */
    bool reset(const QString& instanceId);
    
signals:
    void bypassApplied(const QString& instanceId, FridaDetectionType type, bool success);
    void detectionTriggered(const QString& instanceId, FridaDetectionType type);
    
private:
    FridaXposedDetector();
    ~FridaXposedDetector();
    
    static FridaXposedDetector* s_instance;
    
    void initializeBypassConfigs(const QString& instanceId);
    
    bool applyFridaArtifacts(const QString& instanceId);
    bool applyXposedArtifacts(const QString& instanceId);
    bool applyRootArtifacts(const QString& instanceId);
    bool applyDebugArtifacts(const QString& instanceId);
    bool applyEmulatorArtifacts(const QString& instanceId);
    
    QMap<QString, DetectionBypassState> m_states;
    DetectionBypassCallback m_callback;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_FRIDA_XPOSED_DETECTOR_H
