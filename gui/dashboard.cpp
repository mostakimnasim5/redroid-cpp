/**
 * @file dashboard.cpp
 * @brief Dashboard Implementation
 */

#include "dashboard.h"
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>

Dashboard::Dashboard(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

Dashboard::~Dashboard()
{
}

void Dashboard::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel* title = new QLabel("📊 Dashboard");
    title->setStyleSheet("color: #00d4ff; font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(title);
    
    // Statistics Cards
    QGridLayout* statsGrid = new QGridLayout();
    statsGrid->setSpacing(15);
    
    // Total Devices Card
    QFrame* totalCard = new QFrame();
    totalCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a1a2e, stop:1 #16213e);
            border-radius: 10px;
            border: 1px solid #0f3460;
            padding: 15px;
        }
    )");
    QVBoxLayout* totalLayout = new QVBoxLayout(totalCard);
    QLabel* totalIcon = new QLabel("📱");
    totalIcon->setStyleSheet("font-size: 24px;");
    totalLayout->addWidget(totalIcon, 0, Qt::AlignCenter);
    m_totalDevicesLabel = new QLabel("0");
    m_totalDevicesLabel->setStyleSheet("color: #ffffff; font-size: 32px; font-weight: bold;");
    totalLayout->addWidget(m_totalDevicesLabel, 0, Qt::AlignCenter);
    QLabel* totalText = new QLabel("Total Devices");
    totalText->setStyleSheet("color: #888888; font-size: 12px;");
    totalLayout->addWidget(totalText, 0, Qt::AlignCenter);
    statsGrid->addWidget(totalCard, 0, 0);
    
    // Running Devices Card
    QFrame* runningCard = new QFrame();
    runningCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a2e1a, stop:1 #162116);
            border-radius: 10px;
            border: 1px solid #0f6020;
            padding: 15px;
        }
    )");
    QVBoxLayout* runningLayout = new QVBoxLayout(runningCard);
    QLabel* runningIcon = new QLabel("▶️");
    runningIcon->setStyleSheet("font-size: 24px;");
    runningLayout->addWidget(runningIcon, 0, Qt::AlignCenter);
    m_runningDevicesLabel = new QLabel("0");
    m_runningDevicesLabel->setStyleSheet("color: #00ff00; font-size: 32px; font-weight: bold;");
    runningLayout->addWidget(m_runningDevicesLabel, 0, Qt::AlignCenter);
    QLabel* runningText = new QLabel("Running");
    runningText->setStyleSheet("color: #888888; font-size: 12px;");
    runningLayout->addWidget(runningText, 0, Qt::AlignCenter);
    statsGrid->addWidget(runningCard, 0, 1);
    
    // Anti-Detection Card
    QFrame* antiCard = new QFrame();
    antiCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a1a2e, stop:1 #16213e);
            border-radius: 10px;
            border: 1px solid #0f3460;
            padding: 15px;
        }
    )");
    QVBoxLayout* antiLayout = new QVBoxLayout(antiCard);
    QLabel* antiIcon = new QLabel("🛡️");
    antiIcon->setStyleSheet("font-size: 24px;");
    antiLayout->addWidget(antiIcon, 0, Qt::AlignCenter);
    m_antiDetectionLabel = new QLabel("0");
    m_antiDetectionLabel->setStyleSheet("color: #00d4ff; font-size: 32px; font-weight: bold;");
    antiLayout->addWidget(m_antiDetectionLabel, 0, Qt::AlignCenter);
    QLabel* antiText = new QLabel("Anti-Detection");
    antiText->setStyleSheet("color: #888888; font-size: 12px;");
    antiLayout->addWidget(antiText, 0, Qt::AlignCenter);
    statsGrid->addWidget(antiCard, 1, 0);
    
    // Banking Mode Card
    QFrame* bankingCard = new QFrame();
    bankingCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2e1a2e, stop:1 #211621);
            border-radius: 10px;
            border: 1px solid #6020f0;
            padding: 15px;
        }
    )");
    QVBoxLayout* bankingLayout = new QVBoxLayout(bankingCard);
    QLabel* bankingIcon = new QLabel("🏦");
    bankingIcon->setStyleSheet("font-size: 24px;");
    bankingLayout->addWidget(bankingIcon, 0, Qt::AlignCenter);
    m_bankingModeLabel = new QLabel("0");
    m_bankingModeLabel->setStyleSheet("color: #aa55ff; font-size: 32px; font-weight: bold;");
    bankingLayout->addWidget(m_bankingModeLabel, 0, Qt::AlignCenter);
    QLabel* bankingText = new QLabel("Banking Mode");
    bankingText->setStyleSheet("color: #888888; font-size: 12px;");
    bankingLayout->addWidget(bankingText, 0, Qt::AlignCenter);
    statsGrid->addWidget(bankingCard, 1, 1);
    
    mainLayout->addLayout(statsGrid);
    
    // System Resources Section
    QGroupBox* resourcesGroup = new QGroupBox("System Resources");
    resourcesGroup->setStyleSheet(R"(
        QGroupBox {
            color: #00d4ff;
            border: 1px solid #0f3460;
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    ");
    QVBoxLayout* resourcesLayout = new QVBoxLayout(resourcesGroup);
    
    // CPU
    QHBoxLayout* cpuLayout = new QHBoxLayout();
    QLabel* cpuLabel = new QLabel("CPU:");
    cpuLabel->setStyleSheet("color: #ffffff;");
    cpuLayout->addWidget(cpuLabel);
    m_cpuBar = new QProgressBar();
    m_cpuBar->setValue(25);
    m_cpuBar->setStyleSheet(R"(
        QProgressBar {
            background: #333;
            border: none;
            border-radius: 5px;
            height: 20px;
        }
        QProgressBar::chunk {
            background: #00d4ff;
            border-radius: 5px;
        }
    )");
    cpuLayout->addWidget(m_cpuBar);
    m_cpuValue = new QLabel("25%");
    m_cpuValue->setStyleSheet("color: #00d4ff; min-width: 40px;");
    cpuLayout->addWidget(m_cpuValue);
    resourcesLayout->addLayout(cpuLayout);
    
    // Memory
    QHBoxLayout* memLayout = new QHBoxLayout();
    QLabel* memLabel = new QLabel("Memory:");
    memLabel->setStyleSheet("color: #ffffff;");
    memLayout->addWidget(memLabel);
    m_memoryBar = new QProgressBar();
    m_memoryBar->setValue(45);
    m_memoryBar->setStyleSheet(R"(
        QProgressBar {
            background: #333;
            border: none;
            border-radius: 5px;
            height: 20px;
        }
        QProgressBar::chunk {
            background: #00ff00;
            border-radius: 5px;
        }
    )");
    memLayout->addWidget(m_memoryBar);
    m_memoryValue = new QLabel("45%");
    m_memoryValue->setStyleSheet("color: #00ff00; min-width: 40px;");
    memLayout->addWidget(m_memoryValue);
    resourcesLayout->addLayout(memLayout);
    
    // Network
    QHBoxLayout* netLayout = new QHBoxLayout();
    QLabel* netLabel = new QLabel("Network:");
    netLabel->setStyleSheet("color: #ffffff;");
    netLayout->addWidget(netLabel);
    m_networkBar = new QProgressBar();
    m_networkBar->setValue(10);
    m_networkBar->setStyleSheet(R"(
        QProgressBar {
            background: #333;
            border: none;
            border-radius: 5px;
            height: 20px;
        }
        QProgressBar::chunk {
            background: #ff6600;
            border-radius: 5px;
        }
    )");
    netLayout->addWidget(m_networkBar);
    m_networkValue = new QLabel("10 Mbps");
    m_networkValue->setStyleSheet("color: #ff6600; min-width: 60px;");
    netLayout->addWidget(m_networkValue);
    resourcesLayout->addLayout(netLayout);
    
    mainLayout->addWidget(resourcesGroup);
    
    // Refresh Button
    QPushButton* refreshBtn = new QPushButton("🔄 Refresh");
    refreshBtn->setStyleSheet(R"(
        QPushButton {
            background: #0f3460;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 10px;
        }
        QPushButton:hover {
            background: #1a4a80;
        }
    )");
    connect(refreshBtn, &QPushButton::clicked, this, &Dashboard::onRefreshClicked);
    mainLayout->addWidget(refreshBtn);
    
    mainLayout->addStretch();
}

void Dashboard::updateStatistics(int totalDevices, int runningDevices)
{
    m_totalDevicesLabel->setText(QString::number(totalDevices));
    m_runningDevicesLabel->setText(QString::number(runningDevices));
    m_antiDetectionLabel->setText(QString::number(totalDevices > 0 ? totalDevices : 0));
    m_bankingModeLabel->setText("0");
}

void Dashboard::showDeviceDetails(const DeviceInfo& device)
{
    // Update dashboard with device details
    m_antiDetectionLabel->setText(device.antiDetectionEnabled ? "1" : "0");
}

void Dashboard::addLogEntry(const QString& entry)
{
    // Add log entry to dashboard
}

void Dashboard::onRefreshClicked()
{
    // Refresh statistics
}
