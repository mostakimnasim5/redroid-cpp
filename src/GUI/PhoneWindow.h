#pragma once

#ifndef VIRTUALPHONEPRO_PHONE_WINDOW_H
#define VIRTUALPHONEPRO_PHONE_WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QProcess>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QPixmap>
#include <QByteArray>
#include <QString>

#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/DeviceProfile.h"

namespace VirtualPhonePro {

/**
 * @brief PhoneWindow - Individual phone instance window
 * 
 * Each instance gets its own window with:
 * - Live screen mirror (screenshot updates)
 * - Touch input (tap, swipe)
 * - Keyboard input
 * - Hardware buttons (Back, Home, Apps)
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
    
    // Status updates
    void onInstanceStateChanged(const QString& instanceId, InstanceState state);
    void onAdbConnectionChanged(const QString& instanceId, bool connected);

protected:
    // Keyboard events
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    // Helper functions
    void setupUI();
    void setupScreenMirror();
    void setupConnections();
    
    // ADB commands
    void sendAdbTap(int x, int y);
    void sendAdbSwipe(int x1, int y1, int x2, int y2, int duration);
    void sendAdbKeyEvent(int keyCode);
    void sendAdbText(const QString& text);
    
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
    
    // UI Components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_statusLabel;
    
    // Hardware buttons
    QPushButton* m_backButton;
    QPushButton* m_homeButton;
    QPushButton* m_appsButton;
    QPushButton* m_powerButton;
    
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
