/**
 * @file devicecard.h
 * @brief Device Card Widget for displaying virtual devices
 */

#ifndef DEVICECARD_H
#define DEVICECARD_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QProgressBar>

struct DeviceInfo;

class DeviceCard : public QFrame
{
    Q_OBJECT

public:
    explicit DeviceCard(const DeviceInfo& device, QWidget* parent = nullptr);
    ~DeviceCard();
    
    void updateStatus(const QString& status);
    void updateInfo(const DeviceInfo& device);
    
signals:
    void clicked(const QString& deviceId);
    void startClicked(const QString& deviceId);
    void stopClicked(const QString& deviceId);
    void screenClicked(const QString& deviceId);
    void settingsClicked(const QString& deviceId);
    void deleteClicked(const QString& deviceId);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onScreenClicked();
    void onSettingsClicked();
    void onDeleteClicked();

private:
    void setupUI();
    void updateUI();
    
    QString m_deviceId;
    QString m_deviceName;
    QString m_manufacturer;
    QString m_model;
    QString m_status;
    bool m_isRunning;
    bool m_antiDetectionEnabled;
    
    // UI Components
    QLabel* m_nameLabel;
    QLabel* m_modelLabel;
    QLabel* m_statusIndicator;
    QLabel* m_statusLabel;
    QLabel* m_antiDetectionBadge;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_screenBtn;
    QPushButton* m_settingsBtn;
    QPushButton* m_deleteBtn;
    QProgressBar* m_batteryBar;
    QProgressBar* m_memoryBar;
};

#endif // DEVICECARD_H
