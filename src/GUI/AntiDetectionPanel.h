/**
 * @file AntiDetectionPanel.h
 * @brief Anti-Detection Control Panel for PhoneWindow
 * 
 * Shows status of all anti-detection modules and allows toggling them.
 */

#pragma once

#ifndef VIRTUALPHONEPRO_ANTIDETECTIONPANEL_H
#define VIRTUALPHONEPRO_ANTIDETECTIONPANEL_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSwitch>
#include <QGroupBox>
#include <QTableWidget>
#include <QTimer>
#include <QJsonObject>

namespace VirtualPhonePro {

// ============================================================================
// Module Status
// ============================================================================

struct AntiDetectionModuleStatus {
    QString name;
    QString icon;
    bool isActive;
    bool isEnabled;
    QString status;
    QString description;
    
    QJsonObject toJson() const;
};

// ============================================================================
// AntiDetection Panel
// ============================================================================

class AntiDetectionPanel : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct Anti-Detection Panel
     * @param instanceId The phone instance ID
     * @param parent Parent widget
     */
    explicit AntiDetectionPanel(const QString& instanceId, QWidget* parent = nullptr);
    ~AntiDetectionPanel();

    /**
     * @brief Refresh all module status
     */
    void refreshStatus();

    /**
     * @brief Apply all anti-detection modules
     */
    void applyAllModules();

    /**
     * @brief Get instance ID
     */
    QString getInstanceId() const { return m_instanceId; }

signals:
    void antiDetectionApplied(const QString& instanceId, bool success);
    void moduleStatusChanged(const QString& moduleName, bool enabled);

private slots:
    void onApplyAllClicked();
    void onApplySelectedClicked();
    void onModuleToggled(const QString& moduleName, bool enabled);
    void onRefreshClicked();
    void onModuleSelectionChanged();
    void onAutoApplyToggled(bool enabled);
    void updateStatusTimer();

private:
    void setupUI();
    void createModuleTable();
    void createControlButtons();
    void createStatusArea();
    void loadModuleStatus();
    void applyModule(const QString& moduleName, bool enabled);
    
    // Module application methods
    bool applyRealPhoneHardening();
    bool applyHardwareFingerprintSpoofing();
    bool applyBankingAppSpoofing();
    bool applySafetyNetBypass();
    bool applyEmulatorDetectionBypass();
    bool applyTLSFingerprinting();
    bool applySELinuxMasking();
    bool applyPlayIntegrity();

    QString m_instanceId;
    QTableWidget* m_moduleTable;
    QPushButton* m_applyAllButton;
    QPushButton* m_applySelectedButton;
    QPushButton* m_refreshButton;
    QLabel* m_statusLabel;
    QLabel* m_overallStatusLabel;
    QTimer* m_statusTimer;
    QTimer* m_autoApplyTimer;
    
    bool m_autoApply;
    bool m_allApplied;
    
    QMap<QString, AntiDetectionModuleStatus> m_modules;
};

// ============================================================================
// AntiDetection Manager (Orchestrator)
// ============================================================================

class AntiDetectionManager : public QObject {
    Q_OBJECT

public:
    static AntiDetectionManager& instance();
    
    /**
     * @brief Apply all anti-detection modules to an instance
     * @param instanceId Device instance ID
     * @param profile Device profile
     * @return true if all modules applied successfully
     */
    bool applyCompleteProtection(const QString& instanceId, const DeviceProfile& profile);
    
    /**
     * @brief Get status of all modules for an instance
     */
    QJsonObject getProtectionStatus(const QString& instanceId) const;
    
    /**
     * @brief Check if instance is fully protected
     */
    bool isFullyProtected(const QString& instanceId) const;
    
    /**
     * @brief Get list of protected instances
     */
    QStringList getProtectedInstances() const;

signals:
    void protectionApplied(const QString& instanceId, bool success);
    void protectionStatusChanged(const QString& instanceId, const QJsonObject& status);

private:
    explicit AntiDetectionManager(QObject* parent = nullptr);
    ~AntiDetectionManager() = default;
    
    AntiDetectionManager(const AntiDetectionManager&) = delete;
    AntiDetectionManager& operator=(const AntiDetectionManager&) = delete;
    
    static AntiDetectionManager* s_instance;
    
    QMap<QString, bool> m_protectedInstances;
    QMap<QString, QJsonObject> m_protectionStatuses;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_ANTIDETECTIONPANEL_H
