#pragma once

#ifndef VIRTUALPHONEPRO_MAINWINDOW_H
#define VIRTUALPHONEPRO_MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidgetItem>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "VirtualPhonePro/DeviceProfile.h"
#include "VirtualPhonePro/ReDroidController.h"

namespace Ui {
    class MainWindow;
}

namespace VirtualPhonePro {

// Forward declaration
class AutoStartDialog;

/**
 * @brief Main Window for VirtualPhonePro Application
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    // Instance Management
    void startInstance(const QString& instanceId, const DeviceProfile& profile);
    void stopInstance(const QString& instanceId);
    void restartInstance(const QString& instanceId);
    void deleteInstance(const QString& instanceId);
    
    // Profile
    void createNewProfile();
    void editProfile(const QString& profileId);
    void deleteProfile(const QString& profileId);
    
    // Actions
    void refreshInstances();
    void connectAdb(const QString& instanceId);
    void disconnectAdb(const QString& instanceId);
    
    // Auto-Start
    void showAutoStartSettings();
    void autoStartInstances();
    void saveInstanceForAutoStart(const QString& instanceId, const DeviceProfile& profile);
    void removeInstanceFromAutoStart(const QString& instanceId);
    
    // Settings
    void showSettings();

private slots:
    // Menu actions
    void on_actionNew_Instance_triggered();
    void on_actionStart_triggered();
    void on_actionStop_triggered();
    void on_actionDelete_triggered();
    void on_actionRefresh_triggered();
    void on_actionAutoStart_triggered();
    void on_actionSettings_triggered();
    void on_actionExit_triggered();
    void on_actionSaveForAutoStart_triggered();
    void on_actionRemoveFromAutoStart_triggered();
    
    // Toolbar actions
    void on_newInstanceButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_refreshButton_clicked();
    
    // Table selection
    void on_instanceTable_currentCellChanged(int row, int column);
    void on_instanceTable_itemDoubleClicked(QTableWidgetItem* item);
    
    // Instance state changes
    void handleInstanceStateChanged(const QString& instanceId, InstanceState state);
    void handleAdbConnectionChanged(const QString& instanceId, bool connected);
    void handleError(const QString& message);
    
    // Sensor data
    void on_sendSensorDataButton_clicked();
    void on_gpsUpdateButton_clicked();

private:
    // UI Setup
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupInstanceTable();
    void setupDetailsPanel();
    void setupConnections();
    
    // Profile management
    void loadProfiles();
    void saveProfile(const DeviceProfile& profile);
    
    // UI Updates
    void updateInstanceTable();
    void updateInstanceDetails(const QString& instanceId);
    void updateToolbarState();
    void updateStatusBar();
    void showNotification(const QString& message);
    
    // Dialogs
    QString showNewInstanceDialog();
    DeviceProfile showProfileSelectorDialog();
    void showAboutDialog();

private:
    // UI Components
    QTableWidget* m_instanceTable;
    QWidget* m_detailsWidget;
    QLabel* m_statusLabel;
    QLabel* m_memoryLabel;
    QLabel* m_dockerStatusLabel;
    
    // Details panel widgets
    QLabel* m_instanceIdLabel;
    QLabel* m_stateLabel;
    QLabel* m_adbStatusLabel;
    QLabel* m_brandLabel;
    QLabel* m_modelLabel;
    QLabel* m_imeiLabel;
    QLabel* m_serialLabel;
    QLabel* m_wifiMacLabel;
    QLabel* m_gpsLabel;
    
    // Controls
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_restartButton;
    QPushButton* m_deleteButton;
    QPushButton* m_connectButton;
    QPushButton* m_screenshotButton;
    QPushButton* m_spoofButton;
    
    // Sensor inputs
    QLineEdit* m_latitudeEdit;
    QLineEdit* m_longitudeEdit;
    QLineEdit* m_accelXEdit;
    QLineEdit* m_accelYEdit;
    
    // State
    QString m_selectedInstanceId;
    QMap<QString, InstanceInfo> m_instances;
    QMap<QString, DeviceProfile> m_profiles;
};

/**
 * @brief Settings Dialog
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

    DockerConfig getConfig() const;
    void setConfig(const DockerConfig& config);

private slots:
    void onBrowseDocker();
    void onBrowseAdb();
    void onTestConnection();
    void onOk();
    void onCancel();

private:
    void setupUI();
    
    QLineEdit* m_dockerPathEdit;
    QLineEdit* m_adbPathEdit;
    QLineEdit* m_imageNameEdit;
    QSpinBox* m_memoryLimitSpin;
    QSpinBox* m_cpuQuotaSpin;
    QSpinBox* m_baseAdbPortSpin;
    QSpinBox* m_baseVncPortSpin;
    QCheckBox* m_autoConnectCheck;
    QCheckBox* m_autoSpoofCheck;
    QLabel* m_connectionStatusLabel;
    
    DockerConfig m_config;
};

/**
 * @brief Profile Editor Dialog
 */
class ProfileEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileEditorDialog(QWidget* parent = nullptr);
    ~ProfileEditorDialog();

    DeviceProfile getProfile() const;
    void setProfile(const DeviceProfile& profile);
    void setReadOnly(bool readOnly);

private slots:
    void onRandomize();
    void onManufacturerChanged();
    void onOk();
    void onCancel();

private:
    void setupUI();
    void loadManufacturerModels();
    void populateFromProfile();

    // Form fields
    QLineEdit* m_nameEdit;
    QComboBox* m_manufacturerCombo;
    QComboBox* m_modelCombo;
    
    // Identity
    QLineEdit* m_brandEdit;
    QLineEdit* m_imeiEdit;
    QLineEdit* m_imei2Edit;
    QLineEdit* m_serialEdit;
    QLineEdit* m_androidIdEdit;
    QLineEdit* m_gsfIdEdit;
    
    // Network
    QLineEdit* m_wifiMacEdit;
    QLineEdit* m_btMacEdit;
    QLineEdit* m_hostnameEdit;
    
    // SIM
    QLineEdit* m_iccidEdit;
    QLineEdit* m_imsiEdit;
    QComboBox* m_carrierCombo;
    
    // GPS
    QLineEdit* m_latitudeEdit;
    QLineEdit* m_longitudeEdit;
    
    // Buttons
    QPushButton* m_randomizeButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    DeviceProfile m_profile;
    bool m_readOnly;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_MAINWINDOW_H
