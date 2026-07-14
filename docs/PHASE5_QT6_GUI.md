# VirtualPhonePro - Phase 5: Qt6 GUI

## Overview

Phase 5 implements the Qt6-based graphical user interface for VirtualPhonePro.

## GUI Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        MainWindow                                │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  MenuBar: File | Edit | View | Instances | Tools | Help   │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                      Toolbar                              │  │
│  │  [▶ Start] [⏹ Stop] [📋 Clone] [⚙ Settings] [🔄 Refresh] │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────┬─────────────────────────────────────┤
│  │                       │                                     │
│  │   Instance List       │         Instance Details            │
│  │   Panel               │         Panel                       │
│  │   ┌───────────────┐  │  ┌─────────────────────────────┐   │
│  │   │ 📱 Instance 1 │  │  │ Device Info                 │   │
│  │   │    Running    │  │  │ ─────────────────────────── │   │
│  │   ├───────────────┤  │  │ Brand: Samsung              │   │
│  │   │ 📱 Instance 2 │  │  │ Model: Galaxy S24 Ultra     │   │
│  │   │    Stopped    │  │  │ Android: 14                 │   │
│  │   ├───────────────┤  │  │ IMEI: 358751090123456       │   │
│  │   │ 📱 Instance 3 │  │  │ Serial: R5CWXXXXXXX         │   │
│  │   │    Running    │  │  └─────────────────────────────┘   │
│  │   └───────────────┘  │  ┌─────────────────────────────┐   │
│  │                       │  │ Quick Actions               │   │
│  │  [+ New Instance]    │  │ [Screen] [Reboot] [Shell]   │   │
│  │                       │  │ [Spoof] [Profile]           │   │
│  │  ┌───────────────┐   │  └─────────────────────────────┘   │
│  │  │ Profile: ▾   │   │                                     │
│  │  └───────────────┘   │                                     │
│  └───────────────────────┴─────────────────────────────────────┤
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                      Status Bar                           │  │
│  │  Instances: 3/10 | Memory: 1.5GB/8GB | Docker: Connected   │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Components

### 5.1 MainWindow

```cpp
// src/ui/MainWindow.h
#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>

#include "InstanceListWidget.h"
#include "InstanceDetailsWidget.h"
#include "ProfileEditorDialog.h"
#include "SettingsDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // File Menu
    void onNewInstance();
    void onImportProfile();
    void onExportProfile();
    void onExit();
    
    // Instance Menu
    void onStartInstance();
    void onStopInstance();
    void onCloneInstance();
    void onDeleteInstance();
    void onStartAllInstances();
    void onStopAllInstances();
    
    // Tools Menu
    void onSettings();
    void onProfileEditor();
    void onDeviceSpoofer();
    void onNetworkManager();
    
    // Instance List
    void onInstanceSelected(const QString& instanceId);
    void onInstanceStateChanged(const QString& instanceId, InstanceState state);
    void onInstanceStatsUpdated(const QString& instanceId, const InstanceStats& stats);
    
    // System Tray
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowFromTray();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    
    void updateWindowTitle();
    void updateStatusBar();
    void updateInstanceActions();
    
    // UI Components
    QToolBar* m_toolBar;
    InstanceListWidget* m_instanceList;
    InstanceDetailsWidget* m_instanceDetails;
    QLabel* m_statusLabel;
    QLabel* m_memoryLabel;
    QLabel* m_dockerStatusLabel;
    
    // System Tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // Dialogs
    ProfileEditorDialog* m_profileEditor;
    SettingsDialog* m_settings;
    
    // State
    QString m_selectedInstanceId;
    bool m_minimizeToTray;
};
```

### 5.2 InstanceListWidget

```cpp
// src/ui/InstanceListWidget.h
#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "core/InstanceManager.h"

class InstanceListWidget : public QWidget {
    Q_OBJECT

public:
    explicit InstanceListWidget(QWidget* parent = nullptr);
    
    void refreshList();
    void setProfiles(const std::vector<DeviceProfile>& profiles);

signals:
    void instanceSelected(const QString& instanceId);
    void createInstanceRequested(const QString& profileId);
    void deleteInstanceRequested(const QString& instanceId);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onNewInstanceClicked();
    void onProfileChanged(int index);

private:
    void addInstanceItem(const InstanceInfo& info);
    void updateItemState(QListWidgetItem* item, InstanceState state);
    
    QListWidget* m_listWidget;
    QPushButton* m_newInstanceBtn;
    QComboBox* m_profileSelector;
    QVBoxLayout* m_layout;
};
```

### 5.3 InstanceDetailsWidget

```cpp
// src/ui/InstanceDetailsWidget.h
#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>

#include "core/DeviceProfile.h"

struct InstanceStats {
    uint64_t memoryUsage;
    uint64_t memoryLimit;
    double cpuPercent;
    int uptime;
};

class InstanceDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit InstanceDetailsWidget(QWidget* parent = nullptr);
    
    void setInstance(const QString& instanceId, const DeviceProfile& profile);
    void clear();
    void updateStats(const InstanceStats& stats);

signals:
    void startInstanceRequested(const QString& instanceId);
    void stopInstanceRequested(const QString& instanceId);
    void rebootInstanceRequested(const QString& instanceId);
    void openShellRequested(const QString& instanceId);
    void openScreenRequested(const QString& instanceId);
    void editProfileRequested(const QString& profileId);
    void applySpoofingRequested(const QString& instanceId);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onRebootClicked();
    void onShellClicked();
    void onScreenClicked();
    void onEditProfileClicked();
    void onApplySpoofingClicked();

private:
    void setupDeviceInfoTab();
    void setupIdentityTab();
    void setupNetworkTab();
    void setupSensorsTab();
    void setupActionsTab();
    
    // Info Group
    QLabel* m_instanceIdLabel;
    QLabel* m_stateLabel;
    QLabel* m_uptimeLabel;
    
    // Device Info
    QLabel* m_brandLabel;
    QLabel* m_modelLabel;
    QLabel* m_androidLabel;
    
    // Identity
    QLabel* m_imeiLabel;
    QLabel* m_serialLabel;
    QLabel* m_androidIdLabel;
    
    // Network
    QLabel* m_wifiMacLabel;
    QLabel* m_ipLabel;
    
    // Stats
    QLabel* m_memoryUsageLabel;
    QLabel* m_cpuUsageLabel;
    
    // Actions
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_rebootBtn;
    QPushButton* m_shellBtn;
    QPushButton* m_screenBtn;
    QPushButton* m_spoofBtn;
    QPushButton* m_profileBtn;
    
    // Tabs
    QTabWidget* m_tabWidget;
    
    QString m_currentInstanceId;
    DeviceProfile m_currentProfile;
};
```

### 5.4 Settings Dialog

```cpp
// src/ui/SettingsDialog.h
#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    
    struct Settings {
        // General
        int maxInstances = 10;
        bool minimizeToTray = true;
        bool startMinimized = false;
        
        // Docker
        QString dockerEndpoint = "http://localhost:2375";
        QString adbPath;
        QString redroidImage = "ghcr.io/redroid/redroid:14.0.0_google_64only";
        
        // Default Instance
        int defaultMemoryMB = 768;
        int defaultCPUCores = 2;
        QString defaultProfile;
        
        // Spoofing
        bool autoSpoofOnStart = true;
        bool spoofSafetyNet = true;
        bool spoofPlayIntegrity = true;
        
        // Network
        QString networkMode = "bridge";
        QString customSubnet;
        
        // Logging
        int logLevel = 2;  // 0=Debug, 1=Info, 2=Warn, 3=Error
        QString logPath;
    };
    
    Settings getSettings() const { return m_settings; }
    void setSettings(const Settings& settings) { m_settings = settings; }

private slots:
    void onApply();
    void onOK();
    void onBrowseADB();
    void onBrowseLogPath();
    void onTestDockerConnection();

private:
    void setupGeneralTab();
    void setupDockerTab();
    void setupDefaultsTab();
    void setupSpoofingTab();
    void setupNetworkTab();
    void setupLoggingTab();
    
    Settings m_settings;
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    
    // General Tab
    QSpinBox* m_maxInstancesSpin;
    QCheckBox* m_minimizeToTrayCheck;
    QCheckBox* m_startMinimizedCheck;
    
    // Docker Tab
    QLineEdit* m_dockerEndpointEdit;
    QLineEdit* m_adbPathEdit;
    QLineEdit* m_redroidImageEdit;
    QPushButton* m_testConnectionBtn;
    QLabel* m_connectionStatusLabel;
    
    // Defaults Tab
    QSpinBox* m_defaultMemorySpin;
    QSpinBox* m_defaultCPUCoresSpin;
    QComboBox* m_defaultProfileCombo;
    
    // Spoofing Tab
    QCheckBox* m_autoSpoofCheck;
    QCheckBox* m_spoofSafetyNetCheck;
    QCheckBox* m_spoofPlayIntegrityCheck;
    
    // Network Tab
    QComboBox* m_networkModeCombo;
    QLineEdit* m_customSubnetEdit;
    
    // Logging Tab
    QSpinBox* m_logLevelSpin;
    QLineEdit* m_logPathEdit;
    QPushButton* m_browseLogBtn;
};
```

### 5.5 Profile Editor Dialog

```cpp
// src/ui/ProfileEditorDialog.h
#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "core/DeviceProfile.h"

class ProfileEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileEditorDialog(QWidget* parent = nullptr);
    
    DeviceProfile getProfile() const { return m_profile; }
    void setProfile(const DeviceProfile& profile);
    void setReadOnly(bool readOnly);

signals:
    void profileSaved(const DeviceProfile& profile);

private slots:
    void onManufacturerChanged(const QString& manufacturer);
    void onModelChanged(const QString& model);
    void onRandomizeClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    void loadManufacturerModels();
    void populateFromProfile();
    
    // Profile Data
    DeviceProfile m_profile;
    bool m_readOnly;
    
    // UI Elements
    QLineEdit* m_nameEdit;
    QComboBox* m_manufacturerCombo;
    QComboBox* m_modelCombo;
    QLineEdit* m_brandEdit;
    QLineEdit* m_imeiEdit;
    QLineEdit* m_imei2Edit;
    QLineEdit* m_serialEdit;
    QLineEdit* m_androidIdEdit;
    QLineEdit* m_gsfIdEdit;
    QLineEdit* m_wifiMacEdit;
    QLineEdit* m_btMacEdit;
    QLineEdit* m_iccidEdit;
    QLineEdit* m_imsiEdit;
    QComboBox* m_carrierCombo;
    QLineEdit* m_latitudeEdit;
    QLineEdit* m_longitudeEdit;
    
    QPushButton* m_randomizeBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};
```

## UI Design Guidelines

### Color Palette

| Element | Color | Hex |
|---------|-------|-----|
| Primary | Deep Blue | #1976D2 |
| Secondary | Teal | #00897B |
| Background | Dark Gray | #1E1E1E |
| Surface | Medium Gray | #2D2D2D |
| Text Primary | White | #FFFFFF |
| Text Secondary | Light Gray | #B0B0B0 |
| Success | Green | #4CAF50 |
| Warning | Orange | #FF9800 |
| Error | Red | #F44336 |
| Running | Bright Green | #00E676 |
| Stopped | Gray | #757575 |

### Typography

| Element | Font | Size | Weight |
|---------|------|------|--------|
| Window Title | Segoe UI | 14px | SemiBold |
| Section Header | Segoe UI | 13px | SemiBold |
| Body Text | Segoe UI | 12px | Regular |
| Label | Segoe UI | 11px | Regular |
| Monospace (IMEI, Serial) | Consolas | 11px | Regular |

### Spacing

- Window padding: 16px
- Section spacing: 12px
- Element spacing: 8px
- Button padding: 8px 16px

## Implementation Tasks

### Task 5.1: Main Window
- [ ] Implement main window layout
- [ ] Implement menu bar
- [ ] Implement toolbar
- [ ] Implement status bar
- [ ] Implement system tray

### Task 5.2: Instance List
- [ ] Implement instance list widget
- [ ] Implement profile selector
- [ ] Implement new instance button
- [ ] Implement context menu

### Task 5.3: Instance Details
- [ ] Implement device info tab
- [ ] Implement identity tab
- [ ] Implement network tab
- [ ] Implement sensors tab
- [ ] Implement quick actions

### Task 5.4: Dialogs
- [ ] Implement settings dialog
- [ ] Implement profile editor
- [ ] Implement about dialog

### Task 5.5: Integration
- [ ] Connect UI to core engine
- [ ] Implement signal/slot connections
- [ ] Add threading for async operations
- [ ] Add error handling UI

---

## Next Phase

[Phase 6: Device Profiles & ProfileEngine](./PHASE6_PROFILE_ENGINE.md)

---

*VirtualPhonePro - Phase 5*
