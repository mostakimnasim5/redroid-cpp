#pragma once

#ifndef VIRTUALPHONEPRO_PHONE_WINDOW_H
#define VIRTUALPHONEPRO_PHONE_WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QPixmap>
#include <QByteArray>
#include <QString>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QProcess>
#include <QFrame>
#include <QSlider>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QToolButton>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSystemTrayIcon>
#include <QResizeEvent>
#include <QSplitter>
#include <QScrollArea>
#include <QStackedWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QTabWidget>

#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/DeviceProfile.h"

namespace VirtualPhonePro {

// ========================================================================
// Forward Declarations
// ========================================================================

struct DeviceProfile;
struct AppInfo;

// ========================================================================
// App Manager Dialog
// ========================================================================

class AppManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit AppManagerDialog(const QString& instanceId, QWidget* parent = nullptr);
    ~AppManagerDialog();

private slots:
    void onAntiDetectionClicked();
    void onRefreshClicked();
    void onLaunchClicked();
    void onUninstallClicked();

private:
    void loadInstalledApps();
    void executeAdbCommand(const QStringList& args);
    
    QString m_instanceId;
    QTableWidget* m_appTable;
    QPushButton* m_refreshButton;
    QPushButton* m_launchButton;
    QPushButton* m_uninstallButton;
    QList<AppInfo> m_apps;
};

// ========================================================================
// PROFESSIONAL PHONE WINDOW - Ultra Realistic Emulator UI
// ========================================================================

/**
 * @brief PhoneWindow - Professional Emulator with Realistic Phone UI
 * 
 * Features:
 * - Ultra-realistic phone frame with rounded corners and dark bezel
 * - Camera notch at top
 * - Hardware buttons (Back, Home, Recent)
 * - Live screen mirror via VNC/ADB
 * - Touch input support
 * - Real-time status bar
 * - Dark theme professional UI
 */
class PhoneWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Professional Phone Window
     * @param instanceId Unique instance identifier
     * @param profile Device profile for this instance
     * @param parent Parent widget
     */
    explicit PhoneWindow(const QString& instanceId, 
                         const DeviceProfile& profile,
                         QWidget* parent = nullptr);
    ~PhoneWindow();
    
    QString getInstanceId() const { return m_instanceId; }
    DeviceProfile getProfile() const { return m_profile; }

public slots:
    // Screen operations
    void startScreenMirror();
    void stopScreenMirror();
    void refreshInstance();
    void updateScreen();
    
    // Control operations
    void onPowerClicked();
    void onVolumeUp();
    void onVolumeDown();
    void onRotateScreen();
    void onScreenshotsClicked();
    void onRecordScreenClicked();
    
    // APK operations
    void onInstallApkClicked();
    void onOpenAppsClicked();
    
    // Settings
    void onSettingsClicked();
    void onAntiDetectionClicked();
    
    // Connection status
    void onInstanceStateChanged(const QString& instanceId, InstanceState state);
    void onAdbConnectionChanged(const QString& instanceId, bool connected);

private slots:
    // Screen capture
    void onScreenProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Touch input
    void onScreenMousePress(QMouseEvent* event);
    void onScreenMouseMove(QMouseEvent* event);
    void onScreenMouseRelease(QMouseEvent* event);
    void onScreenDoubleClick(QMouseEvent* event);
    
    // Hardware buttons
    void onBackClicked();
    void onHomeClicked();
    void onRecentClicked();
    
    // Navigation buttons from phone frame
    void onPhoneBackClicked();
    void onPhoneHomeClicked();
    void onPhoneRecentClicked();
    
    // Status bar updates
    void updateStatusBar();
    void updateFPS();
    
    // Window controls
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    void onAlwaysOnTopToggled(bool checked);

protected:
    // Event handlers
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // ====================================================================
    // UI SETUP METHODS
    // ====================================================================
    void setupUI();
    void setupPhoneFrame();
    void setupToolbar();
    void setupStatusBar();
    void setupScreenArea();
    void setupNavigationBar();
    void setupConnections();
    void applyProfessionalStyle();
    
    // ====================================================================
    // HELPER METHODS
    // ====================================================================
    
    // ADB commands
    void sendAdbTap(int x, int y);
    void sendAdbSwipe(int x1, int y1, int x2, int y2, int duration = 100);
    void sendAdbKeyEvent(int keyCode);
    void sendAdbText(const QString& text);
    QString executeAdbCommandSync(const QStringList& args, int timeoutMs = 10000);
    
    // APK Installation
    void installApk(const QString& apkPath);
    
    // Coordinate transformation
    int toAndroidX(int labelX) const;
    int toAndroidY(int labelY) const;
    
    // Get ADB serial/path
    QString getAdbSerial() const;
    QString getAdbPath() const;
    
    // Update UI state
    void updateWindowTitle();
    void setConnected(bool connected);
    
    // ====================================================================
    // MEMBER VARIABLES
    // ====================================================================
    
    // Instance data
    QString m_instanceId;
    DeviceProfile m_profile;
    QString m_deviceName;
    int m_instanceNumber;
    
    // Screen mirror
    QTimer* m_screenTimer;
    QTimer* m_fpsTimer;
    QLabel* m_screenLabel;
    QProcess* m_adbScreenProcess;
    QByteArray m_screenBuffer;
    bool m_screenMirrorActive;
    int m_currentFPS;
    int m_frameCount;
    
    // Touch input
    QPoint m_touchStartPos;
    bool m_isDragging;
    bool m_isSwiping;
    QList<QPair<int, int>> m_swipePath;
    
    // APK Installation
    QProcess* m_installProcess;
    QProgressDialog* m_installProgress;
    
    // ====================================================================
    // UI COMPONENTS - Phone Frame
    // ====================================================================
    
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    
    // === TOOLBAR ===
    QWidget* m_toolbarWidget;
    QHBoxLayout* m_toolbarLayout;
    QToolButton* m_minimizeBtn;
    QToolButton* m_maximizeBtn;
    QToolButton* m_closeBtn;
    QLabel* m_titleLabel;
    QLabel* m_instanceLabel;
    
    // === PHONE FRAME ===
    QWidget* m_phoneFrame;
    QWidget* m_phoneBezel;
    QWidget* m_cameraNotch;
    QLabel* m_cameraLens;
    QLabel* m_cameraSensor;
    
    // === SCREEN AREA ===
    QWidget* m_screenContainer;
    QLabel* m_screenDisplay;
    QFrame* m_screenFrame;
    
    // === NAVIGATION BAR ===
    QWidget* m_navigationBar;
    QHBoxLayout* m_navLayout;
    QPushButton* m_backBtn;
    QPushButton* m_homeBtn;
    QPushButton* m_recentBtn;
    
    // === STATUS BAR ===
    QWidget* m_statusBarWidget;
    QHBoxLayout* m_statusLayout;
    QLabel* m_connectionStatus;
    QLabel* m_portLabel;
    QLabel* m_protectionStatus;
    QLabel* m_fpsLabel;
    QLabel* m_batteryLabel;
    QLabel* m_timeLabel;
    
    // === CONTROL BUTTONS ===
    QWidget* m_controlPanel;
    QHBoxLayout* m_controlLayout;
    QPushButton* m_powerBtn;
    QPushButton* m_volumeUpBtn;
    QPushButton* m_volumeDownBtn;
    QPushButton* m_rotateBtn;
    QPushButton* m_screenshotBtn;
    QPushButton* m_recordBtn;
    
    // === ACTION BUTTONS ===
    QWidget* m_actionPanel;
    QHBoxLayout* m_actionLayout;
    QPushButton* m_installApkBtn;
    QPushButton* m_appsBtn;
    QPushButton* m_settingsBtn;
    QPushButton* m_antiDetectBtn;
    
    // ====================================================================
    // STYLE DEFINITIONS
    // ====================================================================
    
    // Colors
    static const QString COLOR_BACKGROUND;
    static const QString COLOR_PHONE_FRAME;
    static const QString COLOR_BEZEL;
    static const QString COLOR_ACCENT;
    static const QString COLOR_ACCENT_DIM;
    static const QString COLOR_TEXT;
    static const QString COLOR_TEXT_DIM;
    static const QString COLOR_SUCCESS;
    static const QString COLOR_WARNING;
    static const QString COLOR_ERROR;
    static const QString COLOR_BUTTON_BG;
    static const QString COLOR_BUTTON_HOVER;
    
    // Phone dimensions
    static const int PHONE_WIDTH;
    static const int PHONE_HEIGHT;
    static const int SCREEN_WIDTH;
    static const int SCREEN_HEIGHT;
    static const int BEZEL_WIDTH;
    static const int CORNER_RADIUS;
    static const int NAV_BAR_HEIGHT;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PHONE_WINDOW_H
