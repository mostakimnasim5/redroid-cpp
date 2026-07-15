/**
 * @file anti_detection_panel.h
 * @brief Anti-Detection Control Panel
 */

#ifndef ANTI_DETECTION_PANEL_H
#define ANTI_DETECTION_PANEL_H

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>

class AntiDetectionPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AntiDetectionPanel(QWidget* parent = nullptr);
    ~AntiDetectionPanel();
    
    QList<QString> getSelectedModules();
    int getIntegrityLevel();
    bool isFullBypassEnabled();

signals:
    void applyRequested(const QString& deviceId);
    void moduleToggled(const QString& module, bool enabled);

private slots:
    void onApplyClicked();
    void onModuleToggled(bool checked);

private:
    void setupUI();
    
    QCheckBox* m_hypervisorBypass;
    QCheckBox* m_safetyNetBypass;
    QCheckBox* m_realPhoneHardening;
    QCheckBox* m_timingPrevention;
    QCheckBox* m_bankingSpoofer;
    QCheckBox* m_googleSpoofer;
    QCheckBox* m_hardwareSpoofer;
    QCheckBox* m_networkSpoofer;
    QCheckBox* m_tlsFingerprint;
    QCheckBox* m_trustZoneEmu;
    QCheckBox* m_playIntegrity;
    QCheckBox* m_canvasSpoof;
    QCheckBox* m_webglSpoof;
    
    QComboBox* m_integrityLevel;
    QSpinBox* m_detectionAvoidance;
    
    QPushButton* m_applyBtn;
    QPushButton* m_bankingBtn;
    QPushButton* m_googleBtn;
};

#endif // ANTI_DETECTION_PANEL_H
