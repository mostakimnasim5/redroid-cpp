/**
 * @file SettingsDialog.h
 * @brief Application Settings/Preferences Dialog
 * @version 2.0.0
 */

#ifndef VIRTUALPHONEPRO_SETTINGS_DIALOG_H
#define VIRTUALPHONEPRO_SETTINGS_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

#include "VirtualPhonePro/ReDroidController.hpp"

namespace VirtualPhonePro {

/**
 * @brief Application Settings Dialog
 * 
 * Allows users to configure:
 * - Docker settings
 * - ADB settings
 * - Auto-start preferences
 * - Logging preferences
 * - Backup settings
 * - About information
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    // Docker Settings
    void onBrowseDockerPath();
    void onTestDocker();
    void onSaveDockerSettings();

    // ADB Settings
    void onBrowseAdbPath();
    void onTestAdb();
    void onSaveAdbSettings();

    // General Settings
    void onSaveGeneralSettings();

    // Logging Settings
    void onSaveLoggingSettings();
    void onClearLogs();
    void onOpenLogFolder();
    void onViewLogs();

    // Backup Settings
    void onBackupNow();
    void onRestoreBackup();
    void onSaveBackupSettings();

    // Apply all settings
    void onApply();
    void onReset();

	private:
    void setupUI();
    QWidget* createDockerTab();
    QWidget* createAdbTab();
    QWidget* createGeneralTab();
    QWidget* createLoggingTab();
    QWidget* createBackupTab();
    QWidget* createAboutTab();
    void loadSettings();
    void saveSettings();
    QString getSettingsFilePath() const;
    void showMessage(const QString& title, const QString& message, bool isError = false);

    // UI Components
    QTabWidget* m_tabWidget;
    
    // Docker Tab
    QComboBox* m_dockerRuntimeCombo;
    QLineEdit* m_wslDistroEdit;
    QLineEdit* m_dockerPathEdit;
    QLineEdit* m_dockerHostEdit;
    QSpinBox* m_dockerTimeoutSpin;
    QPushButton* m_testDockerBtn;
    QLabel* m_dockerStatusLabel;
    
    // ADB Tab
    QLineEdit* m_adbPathEdit;
    QCheckBox* m_autoAdbCheck;
    QSpinBox* m_adbPortStartSpin;
    QPushButton* m_testAdbBtn;
    QLabel* m_adbStatusLabel;
    
    // General Tab
    QCheckBox* m_autoStartCheck;
    QCheckBox* m_minimizeToTrayCheck;
    QCheckBox* m_startMinimizedCheck;
    QSpinBox* m_maxInstancesSpin;
    QSpinBox* m_defaultMemorySpin;
    QComboBox* m_themeCombo;
    
    // Logging Tab
    QComboBox* m_logLevelCombo;
    QCheckBox* m_logToFileCheck;
    QCheckBox* m_logToConsoleCheck;
    QSpinBox* m_maxLogSizeSpin;
    QSpinBox* m_maxBackupLogsSpin;
    QPushButton* m_clearLogsBtn;
    QPushButton* m_openLogFolderBtn;
    QPushButton* m_viewLogsBtn;
    QLabel* m_logFilePathLabel;
    
    // Backup Tab
    QLineEdit* m_backupPathEdit;
    QCheckBox* m_autoBackupCheck;
    QSpinBox* m_backupIntervalSpin;
    QPushButton* m_browseBackupPathBtn;
    QPushButton* m_backupNowBtn;
    QPushButton* m_restoreBackupBtn;
    QLabel* m_lastBackupLabel;
    
    // About Tab
    QLabel* m_versionLabel;
    QLabel* m_buildDateLabel;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SETTINGS_DIALOG_H
