/**
 * @file devicecard.cpp
 * @brief Device Card Implementation
 */

#include "devicecard.h"
#include "mainwindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

DeviceCard::DeviceCard(const DeviceInfo& device, QWidget* parent)
    : QFrame(parent)
    , m_deviceId(device.id)
    , m_deviceName(device.name)
    , m_manufacturer(device.manufacturer)
    , m_model(device.model)
    , m_status(device.status)
    , m_isRunning(device.isRunning)
    , m_antiDetectionEnabled(device.antiDetectionEnabled)
{
    setupUI();
    
    // Set card styling
    setFixedSize(280, 200);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1a2e, stop:1 #16213e);
            border-radius: 15px;
            border: 1px solid #0f3460;
        }
        QFrame:hover {
            border: 2px solid #00d4ff;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1a3e, stop:1 #16213e);
        }
    )");
}

DeviceCard::~DeviceCard()
{
}

void DeviceCard::setupUI()
{
    // Main Layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 10);
    mainLayout->setSpacing(8);
    
    // Header: Name and Status
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    // Device Icon
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(QPixmap(":/icons/android.png").scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(iconLabel);
    
    // Name and Model
    QVBoxLayout* nameLayout = new QVBoxLayout();
    m_nameLabel = new QLabel(m_deviceName);
    m_nameLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");
    nameLayout->addWidget(m_nameLabel);
    
    m_modelLabel = new QLabel(m_model);
    m_modelLabel->setStyleSheet("color: #888888; font-size: 11px;");
    nameLayout->addWidget(m_modelLabel);
    headerLayout->addLayout(nameLayout, 1);
    
    // Status Indicator
    m_statusIndicator = new QLabel();
    m_statusIndicator->setFixedSize(12, 12);
    m_statusIndicator->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(
        m_isRunning ? "#00ff00" : "#888888"));
    headerLayout->addWidget(m_statusIndicator);
    
    mainLayout->addLayout(headerLayout);
    
    // Anti-Detection Badge
    if (m_antiDetectionEnabled) {
        m_antiDetectionBadge = new QLabel("🛡️ Anti-Detection Active");
        m_antiDetectionBadge->setStyleSheet(R"(
            background: rgba(0, 255, 0, 0.2);
            color: #00ff00;
            border-radius: 5px;
            padding: 4px 8px;
            font-size: 10px;
        )");
        mainLayout->addWidget(m_antiDetectionBadge);
    }
    
    // Status Label
    m_statusLabel = new QLabel(m_status);
    m_statusLabel->setStyleSheet("color: #00d4ff; font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);
    
    // Progress Bars
    QHBoxLayout* progressLayout = new QHBoxLayout();
    
    QLabel* batteryIcon = new QLabel("🔋");
    progressLayout->addWidget(batteryIcon);
    m_batteryBar = new QProgressBar();
    m_batteryBar->setValue(75);
    m_batteryBar->setFixedHeight(6);
    m_batteryBar->setTextVisible(false);
    m_batteryBar->setStyleSheet(R"(
        QProgressBar {
            background: #333;
            border: none;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background: #00ff00;
            border-radius: 3px;
        }
    )");
    progressLayout->addWidget(m_batteryBar);
    progressLayout->setStretch(1, 1);
    
    mainLayout->addLayout(progressLayout);
    
    // Action Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);
    
    // Start/Stop Button
    if (m_isRunning) {
        m_startBtn = nullptr;
        m_stopBtn = new QPushButton("Stop");
        m_stopBtn->setIcon(QIcon(":/icons/stop.png"));
        m_stopBtn->setStyleSheet(R"(
            QPushButton {
                background: #ff4444;
                color: white;
                border: none;
                border-radius: 5px;
                padding: 5px 10px;
                font-size: 11px;
            }
            QPushButton:hover {
                background: #ff6666;
            }
        )");
        connect(m_stopBtn, &QPushButton::clicked, this, &DeviceCard::onStopClicked);
    } else {
        m_stopBtn = nullptr;
        m_startBtn = new QPushButton("Start");
        m_startBtn->setIcon(QIcon(":/icons/play.png"));
        m_startBtn->setStyleSheet(R"(
            QPushButton {
                background: #00cc66;
                color: white;
                border: none;
                border-radius: 5px;
                padding: 5px 10px;
                font-size: 11px;
            }
            QPushButton:hover {
                background: #00ff88;
            }
        )");
        connect(m_startBtn, &QPushButton::clicked, this, &DeviceCard::onStartClicked);
    }
    if (m_startBtn) buttonLayout->addWidget(m_startBtn);
    if (m_stopBtn) buttonLayout->addWidget(m_stopBtn);
    
    // Screen Button
    m_screenBtn = new QPushButton("Screen");
    m_screenBtn->setIcon(QIcon(":/icons/screen.png"));
    m_screenBtn->setStyleSheet(R"(
        QPushButton {
            background: #00d4ff;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 5px 10px;
            font-size: 11px;
        }
        QPushButton:hover {
            background: #00e5ff;
        }
    )");
    connect(m_screenBtn, &QPushButton::clicked, this, &DeviceCard::onScreenClicked);
    buttonLayout->addWidget(m_screenBtn);
    
    // Settings Button
    m_settingsBtn = new QPushButton("⚙️");
    m_settingsBtn->setFixedSize(30, 30);
    m_settingsBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #888;
            border: 1px solid #444;
            border-radius: 5px;
        }
        QPushButton:hover {
            background: #333;
            color: #fff;
        }
    )");
    connect(m_settingsBtn, &QPushButton::clicked, this, &DeviceCard::onSettingsClicked);
    buttonLayout->addWidget(m_settingsBtn);
    
    // Delete Button
    m_deleteBtn = new QPushButton("🗑️");
    m_deleteBtn->setFixedSize(30, 30);
    m_deleteBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #888;
            border: 1px solid #444;
            border-radius: 5px;
        }
        QPushButton:hover {
            background: #ff4444;
            color: #fff;
        }
    )");
    connect(m_deleteBtn, &QPushButton::clicked, this, &DeviceCard::onDeleteClicked);
    buttonLayout->addWidget(m_deleteBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void DeviceCard::updateStatus(const QString& status)
{
    m_status = status;
    m_statusLabel->setText(status);
    
    if (status.contains("Running")) {
        m_statusIndicator->setStyleSheet("background: #00ff00; border-radius: 6px;");
        m_isRunning = true;
        if (m_stopBtn) m_stopBtn->show();
        if (m_startBtn) m_startBtn->hide();
    } else if (status.contains("Anti-detection")) {
        m_statusIndicator->setStyleSheet("background: #00d4ff; border-radius: 6px;");
    }
}

void DeviceCard::updateInfo(const DeviceInfo& device)
{
    m_deviceId = device.id;
    m_deviceName = device.name;
    m_manufacturer = device.manufacturer;
    m_model = device.model;
    m_status = device.status;
    m_isRunning = device.isRunning;
    m_antiDetectionEnabled = device.antiDetectionEnabled;
    
    m_nameLabel->setText(m_deviceName);
    m_modelLabel->setText(m_model);
    updateStatus(m_status);
    
    updateUI();
}

void DeviceCard::onStartClicked()
{
    emit startClicked(m_deviceId);
}

void DeviceCard::onStopClicked()
{
    emit stopClicked(m_deviceId);
}

void DeviceCard::onScreenClicked()
{
    emit screenClicked(m_deviceId);
}

void DeviceCard::onSettingsClicked()
{
    emit settingsClicked(m_deviceId);
}

void DeviceCard::onDeleteClicked()
{
    emit deleteClicked(m_deviceId);
}

void DeviceCard::updateUI()
{
    if (m_isRunning) {
        m_statusIndicator->setStyleSheet("background: #00ff00; border-radius: 6px;");
    } else {
        m_statusIndicator->setStyleSheet("background: #888888; border-radius: 6px;");
    }
}
