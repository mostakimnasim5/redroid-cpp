#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Docker & Emulator Controls
    void onStartEmulator();
    void onStopEmulator();
    void onRestartEmulator();
    void onConnectADB();
    void onDisconnectADB();
    
    // Profile Management
    void onProfileSelected(int index);
    void onCreateProfile();
    void onDeleteProfile();
    void onApplyProfile();
    
    // Device Controls
    void onRebootDevice();
    void onScreenshot();
    void onInstallAPK();
    void onOpenShell();
    void onOpenFileManager();
    
    // Settings
    void onSaveSettings();
    void onLoadSettings();
    
    // Process handlers
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void updateStatus();
    void checkDockerStatus();
    void checkADBStatus();
    void updateDeviceInfo();

private:
    void setupUI();
    void setupConnections();
    void setupSystemTray();
    void runCommand(const QString &command);
    void appendLog(const QString &text);
    void updateProfileList();
    QString getDeviceProperty(const QString &property);
    bool isDockerRunning();
    bool isEmulatorRunning();
    bool isADBConnected();

private:
    // Main UI Elements
    QWidget *centralWidget;
    QTabWidget *tabWidget;
    
    // Status Bar
    QLabel *statusLabel;
    QLabel *dockerStatusLabel;
    QLabel *emulatorStatusLabel;
    QLabel *adbStatusLabel;
    QProgressBar *bootProgressBar;
    
    // Docker Tab
    QPushButton *btnStartEmulator;
    QPushButton *btnStopEmulator;
    QPushButton *btnRestartEmulator;
    QPushButton *btnConnectADB;
    QPushButton *btnDisconnectADB;
    QTextEdit *logTextEdit;
    QLabel *bootStatusLabel;
    
    // Profile Tab
    QComboBox *profileComboBox;
    QPushButton *btnCreateProfile;
    QPushButton *btnDeleteProfile;
    QPushButton *btnApplyProfile;
    QTableWidget *profileTableWidget;
    
    // Device Tab
    QPushButton *btnReboot;
    QPushButton *btnScreenshot;
    QPushButton *btnInstallAPK;
    QPushButton *btnOpenShell;
    QPushButton *btnOpenFileManager;
    QLabel *deviceInfoLabel;
    QLabel *androidVersionLabel;
    QLabel *buildLabel;
    QLabel *securityPatchLabel;
    
    // Settings Tab
    QSpinBox *memorySpinBox;
    QSpinBox *coresSpinBox;
    QSlider *screenWidthSlider;
    QSlider *screenHeightSlider;
    QCheckBox *enableKVMCheckBox;
    QCheckBox *enableGPUCheckBox;
    QCheckBox *autoStartCheckBox;
    QPushButton *btnSaveSettings;
    
    // System Tray
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    
    // Timers
    QTimer *statusTimer;
    QTimer *dockerCheckTimer;
    
    // Current state
    bool emulatorRunning;
    bool adbConnected;
};

#endif // MAINWINDOW_H
