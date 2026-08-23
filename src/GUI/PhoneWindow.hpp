
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
#include <QThread>
#include <QFrame>
#include <QSlider>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
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

#include "VirtualPhonePro/ReDroidController.hpp"
#include "VirtualPhonePro/DeviceProfile.hpp"
#include "VirtualPhonePro/AdbSocketClient.hpp"
#include "VirtualPhonePro/MediaStreamDecoder.hpp"
#include "VirtualPhonePro/FrameRenderer.hpp"
#include <QTcpServer>
#include <QTcpSocket>

namespace VirtualPhonePro {

// ========================================================================
// Forward Declarations
// ========================================================================

struct AppInfo {
    QString packageName;
    QString name;
    QString version;
    QString icon;
    bool isSystem = false;
    bool isEnabled = true;
    qint64 size = 0;
};

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

private slots:
    void onInstallFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setupControlPanel();
    void setupActionPanel();
    void styleButton(QPushButton* btn, const QString& style);
    QPushButton* createControlButton(const QString& text, const QString& icon);
    QPushButton* createActionButton(const QString& text, const QString& icon);
    bool eventFilter(QObject* obj, QEvent* event) override;

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
    void onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Touch input
    void onScreenMousePress(QMouseEvent* event);
    void onScreenMouseMove(QMouseEvent* event);
    void onScreenMouseRelease(QMouseEvent* event);
    void onScreenDoubleClick(QMouseEvent* event);
    
    // UI setup
    void setupControlPanel();
    void setupActionPanel();
    QPushButton* createControlButton(const QString& icon, const QString& tooltip);
    QPushButton* createActionButton(const QString& text, const QString& color);
    
    // Event handling
    bool eventFilter(QObject* obj, QEvent* event) override;
    
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
    // SCRCPY SCREEN MIRROR METHODS
    // ====================================================================
    bool tryStartScrcpy();
    void embedScrcpyWindow();
    void stopScrcpy();
    
    // ====================================================================
    // NATIVE STREAMING PIPELINE (no external exe)
    // ====================================================================
    AdbSocketClient*    m_adbClient     = nullptr;
    MediaStreamDecoder* m_decoder       = nullptr;
    FrameRenderer*      m_renderer      = nullptr;

    static constexpr quint16 SCRCPY_VIDEO_PORT   = 27183; // local forwarded port
    static constexpr quint16 SCRCPY_CONTROL_PORT = 27184; // local forwarded port

    bool startNativePipeline();
    void stopNativePipeline();
    bool pushScrcpyServer();
    bool startScrcpyServerProcess();

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
    QProcess* m_adbScreenProcess = nullptr;
    QProcess* m_scrcpyProcess = nullptr;
    bool m_scrcpyEmbedded = false;
    QString m_scrcpyWindowTitle;
    qintptr m_scrcpyHwnd = 0;
    QByteArray m_screenBuffer;
    bool m_screenMirrorActive = false;
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
    
    // Colors (using macros for easy access in all classes)
    #define COLOR_BACKGROUND "#1a1a2e"
    #define COLOR_PHONE_FRAME "#16213e"
    #define COLOR_BEZEL "#0f0f23"
    #define COLOR_ACCENT "#00ff88"
    #define COLOR_ACCENT_DIM "#00cc6a"
    #define COLOR_TEXT "#ffffff"
    #define COLOR_TEXT_DIM "#8892b0"
    #define COLOR_SUCCESS "#00ff88"
    #define COLOR_WARNING "#ffd700"
    #define COLOR_ERROR "#ff4757"
    #define COLOR_BUTTON_BG "#1f4068"
    #define COLOR_BUTTON_HOVER "#2d5a87"
    
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
