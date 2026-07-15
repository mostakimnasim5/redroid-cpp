/**
 * @file mainwindow.h
 * @brief Main Window for ReDroidCPP GUI
 * @version 3.0 Ultimate Banking Edition
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QDockWidget>
#include <QSplitter>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHash>

// Forward declarations
class DeviceCard;
class Dashboard;
class AntiDetectionPanel;
class DeviceProfileManager;
class VNCViewer;
class SettingsDialog;
class AboutDialog;
class LogViewer;
class NetworkPanel;
class SimulationPanel;

namespace Ui {
class MainWindow;
}

struct DeviceInfo {
    QString id;
    QString name;
    QString manufacturer;
    QString model;
    QString status;
    QString ipAddress;
    int vncPort;
    int adbPort;
    bool isRunning;
    bool antiDetectionEnabled;
    QString fingerprint;
    QColor statusColor;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Device Management
    void addDevice(const DeviceInfo& device);
    void removeDevice(const QString& deviceId);
    void updateDeviceStatus(const QString& deviceId, const QString& status);
    DeviceInfo getDeviceInfo(const QString& deviceId);
    QList<DeviceInfo> getAllDevices();
    
    // Anti-Detection Control
    void applyAntiDetection(const QString& deviceId);
    void applyBankingSetup(const QString& deviceId);
    void applyGoogleSetup(const QString& deviceId);
    
    // View Management
    void showVNCViewer(const QString& deviceId);
    void showDashboard();
    void showLogViewer();

signals:
    void deviceCreated(const QString& deviceId);
    void deviceDeleted(const QString& deviceId);
    void antiDetectionApplied(const QString& deviceId);
    void screenViewRequested(const QString& deviceId);

private slots:
    // File Menu
    void onNewDevice();
    void onImportProfile();
    void onExportProfile();
    void onSettings();
    void onExit();
    
    // Device Menu
    void onStartDevice();
    void onStopDevice();
    void onRestartDevice();
    void onDeleteDevice();
    void onCloneDevice();
    void onViewScreen();
    
    // Anti-Detection Menu
    void onApplyAntiDetection();
    void onApplyBankingSetup();
    void onApplyGoogleSetup();
    void onCustomProfile();
    
    // View Menu
    void onShowDashboard();
    void onShowGridView();
    void onShowListView();
    void onShowLogViewer();
    void onToggleFullscreen();
    
    // Help Menu
    void onAbout();
    void onDocumentation();
    void onCheckUpdates();
    
    // Device Card Actions
    void onDeviceCardClicked(const QString& deviceId);
    void onDeviceStartClicked(const QString& deviceId);
    void onDeviceStopClicked(const QString& deviceId);
    void onDeviceScreenClicked(const QString& deviceId);
    void onDeviceSettingsClicked(const QString& deviceId);
    void onDeviceDeleteClicked(const QString& deviceId);
    
    // System Tray
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowTrayMessage(const QString& title, const QString& message);
    
    // Timer Updates
    void onUpdateTimer();
    void onStatusUpdate();
    
    // Settings
    void onSettingsChanged();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupConnections();
    
    void createDeviceGrid();
    void createDeviceList();
    void updateDeviceGrid();
    
    void loadSettings();
    void saveSettings();
    void loadDevices();
    void saveDevices();
    
    void createSystemTray();
    void setupShortcuts();
    
    void log(const QString& message, const QString& level = "INFO");
    void updateStatistics();
    
    // UI Components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    
    // Panels
    Dashboard* m_dashboard;
    AntiDetectionPanel* m_antiDetectionPanel;
    NetworkPanel* m_networkPanel;
    SimulationPanel* m_simulationPanel;
    DeviceProfileManager* m_profileManager;
    VNCViewer* m_vncViewer;
    LogViewer* m_logViewer;
    SettingsDialog* m_settingsDialog;
    AboutDialog* m_aboutDialog;
    
    // Device Cards
    QWidget* m_deviceGridWidget;
    QGridLayout* m_deviceGridLayout;
    QHash<QString, DeviceCard*> m_deviceCards;
    
    // Status Bar Components
    QLabel* m_statusLabel;
    QLabel* m_deviceCountLabel;
    QLabel* m_memoryLabel;
    QProgressBar* m_cpuProgressBar;
    QProgressBar* m_memoryProgressBar;
    
    // Toolbars
    QToolBar* m_mainToolBar;
    QToolBar* m_deviceToolBar;
    QToolBar* m_viewToolBar;
    
    // System Tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // Timer
    QTimer* m_updateTimer;
    
    // Data
    QHash<QString, DeviceInfo> m_devices;
    int m_nextVncPort;
    int m_nextAdbPort;
    
    // Settings
    bool m_autoStartEnabled;
    bool m_minimizeToTray;
    QString m_defaultManufacturer;
    QString m_defaultModel;
    int m_maxDevices;
};

#endif // MAINWINDOW_H
