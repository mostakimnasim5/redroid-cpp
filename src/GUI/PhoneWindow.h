#pragma once

#ifndef VIRTUALPHONEPRO_PHONE_WINDOW_H
#define VIRTUALPHONEPRO_PHONE_WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "AntiDetectionPanel.h"
#include <QProcess>
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

#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/DeviceProfile.h"

namespace VirtualPhonePro {

// Forward declaration
struct AppInfo {
    QString packageName;
    QString appName;
    QString version;
};

/**
 * @brief App Manager Dialog - Lists and manages installed apps
 */
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

/**
 * @brief PhoneWindow - Individual phone instance window
 * 
 * Each instance gets its own window with:
 * - Live screen mirror (screenshot updates)
 * - Touch input (tap, swipe)
 * - Keyboard input
 * - Hardware buttons (Back, Home, Apps)
 * - APK installation
 * - App management
 * - Device info display
 */
class PhoneWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Phone Window
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
    void startScreenMirror();
    void stopScreenMirror();
    void refreshInstance();

private slots:
    void onAntiDetectionClicked();
    // Screen mirror
    void updateScreen();
    void onScreenProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Touch input
    void onScreenMousePress(QMouseEvent* event);
    void onScreenMouseMove(QMouseEvent* event);
    void onScreenMouseRelease(QMouseEvent* event);
    
    // Hardware buttons
    void onBackClicked();
    void onHomeClicked();
    void onAppsClicked();
    
    // APK Management
    void onInstallApkClicked();
    void onAppsClicked_Open();
    void onScreenshotClicked();
    
    // Status updates
    void onInstanceStateChanged(const QString& instanceId, InstanceState state);
    void onAdbConnectionChanged(const QString& instanceId, bool connected);

protected:
    // Keyboard events
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    
    // Drag and drop
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    // Helper functions
    void setupUI();
    void setupScreenMirror();
    void setupConnections();
    void setupToolbar();
    
    // ADB commands
    void sendAdbTap(int x, int y);
    void sendAdbSwipe(int x1, int y1, int x2, int y2, int duration);
    void sendAdbKeyEvent(int keyCode);
    void sendAdbText(const QString& text);
    QString executeAdbCommandSync(const QStringList& args, int timeoutMs = 10000);
    
    // APK Installation
    void installApk(const QString& apkPath);
    void onInstallProgress(int progress);
    void onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    QString getAdbSerial() const;
    QString getAdbPath() const;
    
    // Calculate Android coordinates from label coordinates
    int toAndroidX(int labelX) const;
    int toAndroidY(int labelY) const;
    
    // Instance data
    QString m_instanceId;
    DeviceProfile m_profile;
    
    // Screen mirror members
    QTimer* m_screenTimer;
    QLabel* m_screenLabel;
    QProcess* m_adbScreenProcess;
    QByteArray m_screenBuffer;
    bool m_screenMirrorActive;
    
    // Touch input members
    QPoint m_touchStartPos;
    bool m_isDragging;
    
    // APK Installation
    QProcess* m_installProcess;
    QProgressDialog* m_installProgress;
    
    // UI Components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_statusLabel;
    
    // Toolbar
    QToolBar* m_toolbar;
    QAction* m_installApkAction;
    QAction* m_appsAction;
    QAction* m_screenshotAction;
    
    // Hardware buttons
    QPushButton* m_backButton;
    QPushButton* m_homeButton;
    QPushButton* m_appsButton;
    QPushButton* m_powerButton;
    QPushButton* m_antiDetectionButton;
    
    // Control buttons
    QPushButton* m_startMirrorButton;
    QPushButton* m_stopMirrorButton;
    
    // Device info
    QLabel* m_androidVersionLabel;
    QLabel* m_resolutionLabel;
    QLabel* m_imeiLabel;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PHONE_WINDOW_H
