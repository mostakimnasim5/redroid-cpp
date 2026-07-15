/**
 * @file dashboard.h
 * @brief Dashboard Panel
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QChartView>
#include <QChart>

class Dashboard : public QWidget
{
    Q_OBJECT

public:
    explicit Dashboard(QWidget* parent = nullptr);
    ~Dashboard();
    
    void updateStatistics(int totalDevices, int runningDevices);
    void showDeviceDetails(const DeviceInfo& device);
    void addLogEntry(const QString& entry);

signals:
    void deviceSelected(const QString& deviceId);

private slots:
    void onRefreshClicked();

private:
    void setupUI();
    
    QLabel* m_totalDevicesLabel;
    QLabel* m_runningDevicesLabel;
    QLabel* m_antiDetectionLabel;
    QLabel* m_bankingModeLabel;
    
    QProgressBar* m_cpuBar;
    QProgressBar* m_memoryBar;
    QProgressBar* m_networkBar;
    
    QLabel* m_cpuValue;
    QLabel* m_memoryValue;
    QLabel* m_networkValue;
};

#endif // DASHBOARD_H
