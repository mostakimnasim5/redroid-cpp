/**
 * @file AutoStartDialog.h
 * @brief Auto-Start Settings Dialog
 * @version 2.0.0
 * 
 * Manages auto-start settings for Docker containers
 */

#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

namespace VirtualPhonePro {

/**
 * @brief Auto-Start Settings Dialog
 */
class AutoStartDialog : public QDialog {
    Q_OBJECT

public:
    explicit AutoStartDialog(QWidget* parent = nullptr);
    ~AutoStartDialog();

    // Settings
    bool isAutoStartEnabled() const { return m_autoStartCheck->isChecked(); }
    void setAutoStartEnabled(bool enabled) { m_autoStartCheck->setChecked(enabled); }
    
    int getMaxInstances() const { return m_maxInstancesSpin->value(); }
    void setMaxInstances(int max) { m_maxInstancesSpin->setValue(max); }
    
    bool isRestoreOnStartup() const { return m_restoreOnStartupCheck->isChecked(); }
    void setRestoreOnStartup(bool enabled) { m_restoreOnStartupCheck->setChecked(enabled); }
    
    bool isAutoConnectAdb() const { return m_autoConnectAdbCheck->isChecked(); }
    void setAutoConnectAdb(bool enabled) { m_autoConnectAdbCheck->setChecked(enabled); }
    
    bool isAutoSpoofSafetyNet() const { return m_autoSpoofCheck->isChecked(); }
    void setAutoSpoofSafetyNet(bool enabled) { m_autoSpoofCheck->setChecked(enabled); }

private slots:
    void onSaveSettings();
    void onLoadInstances();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // Auto-start settings
    QCheckBox* m_autoStartCheck;
    QCheckBox* m_restoreOnStartupCheck;
    QSpinBox* m_maxInstancesSpin;
    
    // ADB settings
    QCheckBox* m_autoConnectAdbCheck;
    
    // Spoofing settings
    QCheckBox* m_autoSpoofCheck;
    
    // Instance list
    QListWidget* m_instanceList;
};

} // namespace VirtualPhonePro
