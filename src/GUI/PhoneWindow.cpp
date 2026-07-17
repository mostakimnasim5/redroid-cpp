/**
 * @file PhoneWindow.cpp
 * @brief Professional Emulator UI - Ultra Realistic Phone Window
 * @version 3.0 Professional Edition
 * 
 * Features:
 * - Realistic phone frame with rounded corners
 * - Camera notch at top
 * - Hardware navigation buttons
 * - Live screen mirror
 * - Touch input support
 * - Real-time status bar
 * - Dark theme UI
 */

#include "PhoneWindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QDateTime>
#include <QtMath>
#include <QPainter>
#include <QPainterPath>
#include <QPainter>
#include <QBrush>
#include <QPen>

namespace VirtualPhonePro {

// ========================================================================
// Static Style Constants
// ========================================================================

const QString PhoneWindow::COLOR_BACKGROUND = "#1a1a2e";
const QString PhoneWindow::COLOR_PHONE_FRAME = "#16213e";
const QString PhoneWindow::COLOR_BEZEL = "#0f0f23";
const QString PhoneWindow::COLOR_ACCENT = "#00ff88";
const QString PhoneWindow::COLOR_ACCENT_DIM = "#00cc6a";
const QString PhoneWindow::COLOR_TEXT = "#ffffff";
const QString PhoneWindow::COLOR_TEXT_DIM = "#8892b0";
const QString PhoneWindow::COLOR_SUCCESS = "#00ff88";
const QString PhoneWindow::COLOR_WARNING = "#ffd700";
const QString PhoneWindow::COLOR_ERROR = "#ff4757";
const QString PhoneWindow::COLOR_BUTTON_BG = "#1f4068";
const QString PhoneWindow::COLOR_BUTTON_HOVER = "#2d5a87";

const int PhoneWindow::PHONE_WIDTH = 420;
const int PhoneWindow::PHONE_HEIGHT = 820;
const int PhoneWindow::SCREEN_WIDTH = 400;
const int PhoneWindow::SCREEN_HEIGHT = 700;
const int PhoneWindow::BEZEL_WIDTH = 10;
const int PhoneWindow::CORNER_RADIUS = 40;
const int PhoneWindow::NAV_BAR_HEIGHT = 65;

// ========================================================================
// App Manager Dialog Implementation
// ========================================================================

AppManagerDialog::AppManagerDialog(const QString& instanceId, QWidget* parent)
    : QDialog(parent)
    , m_instanceId(instanceId)
{
    setWindowTitle("App Manager - " + instanceId);
    setMinimumSize(600, 500);
    setStyleSheet(QString(
        "QDialog { background-color: %1; color: %2; }"
    ).arg(COLOR_BACKGROUND).arg(COLOR_TEXT));
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // App table
    m_appTable = new QTableWidget();
    m_appTable->setColumnCount(4);
    m_appTable->setHorizontalHeaderLabels({"Package", "App Name", "Version", "Status"});
    m_appTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appTable->setStyleSheet(QString(
        "QTableWidget { background-color: %1; color: %2; border: none; }"
        "QHeaderView::section { background-color: %3; color: %2; padding: 8px; }"
    ).arg(COLOR_PHONE_FRAME).arg(COLOR_TEXT).arg(COLOR_BEZEL));
    mainLayout->addWidget(m_appTable);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("🔄 Refresh");
    m_launchButton = new QPushButton("🚀 Launch");
    m_uninstallButton = new QPushButton("🗑️ Uninstall");
    
    styleButton(m_refreshButton);
    styleButton(m_launchButton);
    styleButton(m_uninstallButton);
    
    btnLayout->addWidget(m_refreshButton);
    btnLayout->addWidget(m_launchButton);
    btnLayout->addWidget(m_uninstallButton);
    mainLayout->addLayout(btnLayout);
    
    connect(m_refreshButton, &QPushButton::clicked, this, &AppManagerDialog::onRefreshClicked);
    connect(m_launchButton, &QPushButton::clicked, this, &AppManagerDialog::onLaunchClicked);
    
    loadInstalledApps();
}

AppManagerDialog::~AppManagerDialog() = default;

void AppManagerDialog::loadInstalledApps() {
    // Implementation for loading apps
}

void AppManagerDialog::onRefreshClicked() {
    loadInstalledApps();
}

void AppManagerDialog::onLaunchClicked() {
    int row = m_appTable->currentRow();
    if (row >= 0) {
        QString package = m_appTable->item(row, 0)->text();
        qDebug() << "Launching:" << package;
    }
}

void AppManagerDialog::onUninstallClicked() {
    int row = m_appTable->currentRow();
    if (row >= 0) {
        QString package = m_appTable->item(row, 0)->text();
        qDebug() << "Uninstalling:" << package;
    }
}

void AppManagerDialog::executeAdbCommand(const QStringList& args) {
    // ADB command execution
}

void AppManagerDialog::onAntiDetectionClicked() {
    // Open anti-detection settings
}

// ========================================================================
// PhoneWindow Implementation
// ========================================================================

PhoneWindow::PhoneWindow(const QString& instanceId, 
                         const DeviceProfile& profile,
                         QWidget* parent)
    : QMainWindow(parent)
    , m_instanceId(instanceId)
    , m_profile(profile)
    , m_deviceName(profile.model.isEmpty() ? "Unknown Device" : profile.model)
    , m_instanceNumber(1)
    , m_screenTimer(nullptr)
    , m_fpsTimer(nullptr)
    , m_screenLabel(nullptr)
    , m_adbScreenProcess(nullptr)
    , m_screenMirrorActive(false)
    , m_currentFPS(0)
    , m_frameCount(0)
    , m_isDragging(false)
    , m_isSwiping(false)
    , m_installProcess(nullptr)
    , m_installProgress(nullptr)
{
    // Extract instance number from instanceId
    QStringList parts = instanceId.split('_');
    if (parts.size() > 1) {
        bool ok;
        m_instanceNumber = parts.last().toInt(&ok);
        if (!ok) m_instanceNumber = 1;
    }
    
    // Setup UI
    setupUI();
    applyProfessionalStyle();
    setupConnections();
    
    // Set window properties
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    
    // Center window
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Update title
    updateWindowTitle();
    
    // Start with disconnected state
    setConnected(false);
}

PhoneWindow::~PhoneWindow() {
    stopScreenMirror();
}

// ========================================================================
// UI Setup Methods
// ========================================================================

void PhoneWindow::setupUI() {
    // Central widget
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("centralWidget");
    setCentralWidget(m_centralWidget);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Setup components
    setupToolbar();
    setupPhoneFrame();
    setupStatusBar();
    setupControlPanel();
    setupActionPanel();
    
    // Set minimum size
    setMinimumSize(PHONE_WIDTH + 40, PHONE_HEIGHT + 200);
}

void PhoneWindow::setupToolbar() {
    // Toolbar container
    m_toolbarWidget = new QWidget();
    m_toolbarWidget->setObjectName("toolbar");
    m_toolbarWidget->setFixedHeight(50);
    
    m_toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    m_toolbarLayout->setContentsMargins(15, 5, 15, 5);
    
    // Window control buttons
    QHBoxLayout* windowControls = new QHBoxLayout();
    windowControls->setSpacing(8);
    
    m_minimizeBtn = new QToolButton();
    m_minimizeBtn->setText("─");
    m_minimizeBtn->setFixedSize(30, 30);
    m_minimizeBtn->setCursor(Qt::PointingHandCursor);
    
    m_maximizeBtn = new QToolButton();
    m_maximizeBtn->setText("□");
    m_maximizeBtn->setFixedSize(30, 30);
    m_maximizeBtn->setCursor(Qt::PointingHandCursor);
    
    m_closeBtn = new QToolButton();
    m_closeBtn->setText("✕");
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet("QToolButton { color: #ff4757; }");
    
    windowControls->addWidget(m_minimizeBtn);
    windowControls->addWidget(m_maximizeBtn);
    windowControls->addWidget(m_closeBtn);
    
    // Title
    m_titleLabel = new QLabel(m_deviceName);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    // Instance label
    m_instanceLabel = new QLabel(QString("Instance %1").arg(m_instanceNumber));
    m_instanceLabel->setObjectName("instanceLabel");
    m_instanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    m_toolbarLayout->addLayout(windowControls);
    m_toolbarLayout->addWidget(m_titleLabel, 1);
    m_toolbarLayout->addWidget(m_instanceLabel);
    
    m_mainLayout->addWidget(m_toolbarWidget);
    
    // Connect toolbar signals
    connect(m_minimizeBtn, &QToolButton::clicked, this, &PhoneWindow::onMinimizeClicked);
    connect(m_maximizeBtn, &QToolButton::clicked, this, &PhoneWindow::onMaximizeClicked);
    connect(m_closeBtn, &QToolButton::clicked, this, &PhoneWindow::onCloseClicked);
}

void PhoneWindow::setupPhoneFrame() {
    // Phone frame container
    QWidget* phoneContainer = new QWidget();
    phoneContainer->setObjectName("phoneContainer");
    QHBoxLayout* phoneContainerLayout = new QHBoxLayout(phoneContainer);
    phoneContainerLayout->setContentsMargins(20, 10, 20, 10);
    phoneContainerLayout->setAlignment(Qt::AlignCenter);
    
    // Main phone frame
    m_phoneFrame = new QWidget();
    m_phoneFrame->setObjectName("phoneFrame");
    m_phoneFrame->setFixedSize(PHONE_WIDTH, PHONE_HEIGHT);
    
    // Phone bezel (outer frame)
    m_phoneBezel = new QWidget(m_phoneFrame);
    m_phoneBezel->setObjectName("phoneBezel");
    m_phoneBezel->setGeometry(0, 0, PHONE_WIDTH, PHONE_HEIGHT);
    
    // Camera notch
    m_cameraNotch = new QWidget(m_phoneFrame);
    m_cameraNotch->setObjectName("cameraNotch");
    m_cameraNotch->setFixedSize(120, 28);
    m_cameraNotch->move((PHONE_WIDTH - 120) / 2, 10);
    
    // Camera lens
    m_cameraLens = new QLabel(m_cameraNotch);
    m_cameraLens->setFixedSize(12, 12);
    m_cameraLens->move(15, 8);
    m_cameraLens->setObjectName("cameraLens");
    
    // Camera sensor
    m_cameraSensor = new QLabel(m_cameraNotch);
    m_cameraSensor->setFixedSize(8, 8);
    m_cameraSensor->move(35, 10);
    m_cameraSensor->setObjectName("cameraSensor");
    
    // Screen container (holds the actual screen)
    m_screenContainer = new QWidget(m_phoneFrame);
    m_screenContainer->setObjectName("screenContainer");
    m_screenContainer->setFixedSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    m_screenContainer->move(BEZEL_WIDTH, BEZEL_WIDTH + 30);
    
    // Screen display label
    m_screenDisplay = new QLabel(m_screenContainer);
    m_screenDisplay->setFixedSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    m_screenDisplay->setAlignment(Qt::AlignCenter);
    m_screenDisplay->setObjectName("screenDisplay");
    m_screenDisplay->setText("<span style='color:#00ff88; font-size:24px;'>"
                             "📱 Android Screen<br>"
                             "<span style='color:#8892b0; font-size:14px;'>"
                             "Click 'Start Mirror' to begin</span></span>");
    m_screenDisplay->setMouseTracking(true);
    m_screenDisplay->installEventFilter(this);
    
    // Navigation bar at bottom
    m_navigationBar = new QWidget(m_phoneFrame);
    m_navigationBar->setObjectName("navigationBar");
    m_navigationBar->setFixedSize(SCREEN_WIDTH, NAV_BAR_HEIGHT);
    m_navigationBar->move(BEZEL_WIDTH, PHONE_HEIGHT - BEZEL_WIDTH - NAV_BAR_HEIGHT - 20);
    
    m_navLayout = new QHBoxLayout(m_navigationBar);
    m_navLayout->setContentsMargins(30, 10, 30, 15);
    m_navLayout->setSpacing(0);
    
    // Navigation buttons
    m_backBtn = new QPushButton("◀");
    m_backBtn->setObjectName("navBackBtn");
    m_backBtn->setFixedSize(60, 40);
    
    m_homeBtn = new QPushButton("⬤");
    m_homeBtn->setObjectName("navHomeBtn");
    m_homeBtn->setFixedSize(60, 40);
    
    m_recentBtn = new QPushButton("▣");
    m_recentBtn->setObjectName("navRecentBtn");
    m_recentBtn->setFixedSize(60, 40);
    
    m_navLayout->addWidget(m_backBtn, 0, Qt::AlignLeft);
    m_navLayout->addWidget(m_homeBtn, 0, Qt::AlignCenter);
    m_navLayout->addWidget(m_recentBtn, 0, Qt::AlignRight);
    
    // Home indicator line
    QLabel* homeIndicator = new QLabel(m_navigationBar);
    homeIndicator->setFixedSize(80, 3);
    homeIndicator->move((SCREEN_WIDTH - 80) / 2 + BEZEL_WIDTH, NAV_BAR_HEIGHT - 8);
    homeIndicator->setObjectName("homeIndicator");
    
    phoneContainerLayout->addWidget(m_phoneFrame);
    m_mainLayout->addWidget(phoneContainer, 1);
    
    // Connect navigation signals
    connect(m_backBtn, &QPushButton::clicked, this, &PhoneWindow::onPhoneBackClicked);
    connect(m_homeBtn, &QPushButton::clicked, this, &PhoneWindow::onPhoneHomeClicked);
    connect(m_recentBtn, &QPushButton::clicked, this, &PhoneWindow::onPhoneRecentClicked);
}

void PhoneWindow::setupStatusBar() {
    // Status bar widget
    m_statusBarWidget = new QWidget();
    m_statusBarWidget->setObjectName("statusBar");
    m_statusBarWidget->setFixedHeight(45);
    
    m_statusLayout = new QHBoxLayout(m_statusBarWidget);
    m_statusLayout->setContentsMargins(20, 5, 20, 5);
    m_statusLayout->setSpacing(20);
    
    // Connection status
    QWidget* connWidget = new QWidget();
    QHBoxLayout* connLayout = new QHBoxLayout(connWidget);
    connLayout->setContentsMargins(0, 0, 0, 0);
    connLayout->setSpacing(5);
    
    m_connectionStatus = new QLabel("⚪ Disconnected");
    m_connectionStatus->setObjectName("connectionStatus");
    connLayout->addWidget(m_connectionStatus);
    
    m_portLabel = new QLabel("| Port: --");
    m_portLabel->setObjectName("portLabel");
    connLayout->addWidget(m_portLabel);
    
    m_statusLayout->addWidget(connWidget);
    
    // Separator
    QLabel* sep1 = new QLabel("|");
    sep1->setStyleSheet("color: #3d5a80;");
    m_statusLayout->addWidget(sep1);
    
    // Protection status
    QWidget* protWidget = new QWidget();
    QHBoxLayout* protLayout = new QHBoxLayout(protWidget);
    protLayout->setContentsMargins(0, 0, 0, 0);
    protLayout->setSpacing(5);
    
    m_protectionStatus = new QLabel("🛡️ Protected: --");
    m_protectionStatus->setObjectName("protectionStatus");
    protLayout->addWidget(m_protectionStatus);
    
    m_statusLayout->addWidget(protWidget);
    
    // Separator
    QLabel* sep2 = new QLabel("|");
    sep2->setStyleSheet("color: #3d5a80;");
    m_statusLayout->addWidget(sep2);
    
    // FPS
    m_fpsLabel = new QLabel("FPS: 0");
    m_fpsLabel->setObjectName("fpsLabel");
    m_statusLayout->addWidget(m_fpsLabel);
    
    // Spacer
    m_statusLayout->addStretch();
    
    // Battery & Time
    m_batteryLabel = new QLabel("🔋 100%");
    m_batteryLabel->setObjectName("batteryLabel");
    m_statusLayout->addWidget(m_batteryLabel);
    
    QLabel* sep3 = new QLabel("|");
    sep3->setStyleSheet("color: #3d5a80;");
    m_statusLayout->addWidget(sep3);
    
    m_timeLabel = new QLabel(QTime::currentTime().toString("HH:mm"));
    m_timeLabel->setObjectName("timeLabel");
    m_statusLayout->addWidget(m_timeLabel);
    
    m_mainLayout->addWidget(m_statusBarWidget);
}

void PhoneWindow::setupControlPanel() {
    // Control buttons panel
    m_controlPanel = new QWidget();
    m_controlPanel->setObjectName("controlPanel");
    m_controlPanel->setFixedHeight(50);
    
    m_controlLayout = new QHBoxLayout(m_controlPanel);
    m_controlLayout->setContentsMargins(15, 5, 15, 5);
    m_controlLayout->setSpacing(10);
    
    // Power button
    m_powerBtn = createControlButton("⏻", "Power");
    connect(m_powerBtn, &QPushButton::clicked, this, &PhoneWindow::onPowerClicked);
    
    // Volume buttons
    m_volumeUpBtn = createControlButton("🔊+", "Vol Up");
    m_volumeDownBtn = createControlButton("🔊-", "Vol Down");
    connect(m_volumeUpBtn, &QPushButton::clicked, this, &PhoneWindow::onVolumeUp);
    connect(m_volumeDownBtn, &QPushButton::clicked, this, &PhoneWindow::onVolumeDown);
    
    // Rotate button
    m_rotateBtn = createControlButton("🔄", "Rotate");
    connect(m_rotateBtn, &QPushButton::clicked, this, &PhoneWindow::onRotateScreen);
    
    // Screenshot button
    m_screenshotBtn = createControlButton("📷", "Screenshot");
    connect(m_screenshotBtn, &QPushButton::clicked, this, &PhoneWindow::onScreenshotsClicked);
    
    // Record button
    m_recordBtn = createControlButton("⏺", "Record");
    connect(m_recordBtn, &QPushButton::clicked, this, &PhoneWindow::onRecordScreenClicked);
    
    m_controlLayout->addWidget(m_powerBtn);
    m_controlLayout->addWidget(m_volumeUpBtn);
    m_controlLayout->addWidget(m_volumeDownBtn);
    m_controlLayout->addWidget(m_rotateBtn);
    m_controlLayout->addWidget(m_screenshotBtn);
    m_controlLayout->addWidget(m_recordBtn);
    
    m_mainLayout->addWidget(m_controlPanel);
}

void PhoneWindow::setupActionPanel() {
    // Action buttons panel
    m_actionPanel = new QWidget();
    m_actionPanel->setObjectName("actionPanel");
    m_actionPanel->setFixedHeight(55);
    
    m_actionLayout = new QHBoxLayout(m_actionPanel);
    m_actionLayout->setContentsMargins(15, 5, 15, 5);
    m_actionLayout->setSpacing(10);
    
    // Install APK button
    m_installApkBtn = createActionButton("📦 Install APK", COLOR_ACCENT);
    connect(m_installApkBtn, &QPushButton::clicked, this, &PhoneWindow::onInstallApkClicked);
    
    // Apps button
    m_appsBtn = createActionButton("📱 Apps", "#4a90d9");
    connect(m_appsBtn, &QPushButton::clicked, this, &PhoneWindow::onOpenAppsClicked);
    
    // Settings button
    m_settingsBtn = createActionButton("⚙️ Settings", "#7f8c8d");
    connect(m_settingsBtn, &QPushButton::clicked, this, &PhoneWindow::onSettingsClicked);
    
    // Anti-detection button
    m_antiDetectBtn = createActionButton("🛡️ Anti-Detect", "#e74c3c");
    connect(m_antiDetectBtn, &QPushButton::clicked, this, &PhoneWindow::onAntiDetectionClicked);
    
    m_actionLayout->addWidget(m_installApkBtn);
    m_actionLayout->addWidget(m_appsBtn);
    m_actionLayout->addWidget(m_settingsBtn);
    m_actionLayout->addWidget(m_antiDetectBtn);
    
    m_mainLayout->addWidget(m_actionPanel);
}

QPushButton* PhoneWindow::createControlButton(const QString& icon, const QString& tooltip) {
    QPushButton* btn = new QPushButton(icon);
    btn->setFixedSize(40, 40);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QPushButton* PhoneWindow::createActionButton(const QString& text, const QString& color) {
    QPushButton* btn = new QPushButton(text);
    btn->setMinimumHeight(40);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 8px 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}"
    ).arg(color).arg(QColor(color).lighter(110).name()));
    return btn;
}

// ========================================================================
// Professional StyleSheet
// ========================================================================

void PhoneWindow::applyProfessionalStyle() {
    QString styleSheet = QString(R"(
        /* Main Window */
        QMainWindow {
            background-color: %1;
            color: %2;
        }
        
        /* Central Widget */
        #centralWidget {
            background-color: %1;
        }
        
        /* Toolbar */
        #toolbar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                stop:0 #1a1a2e, stop:1 #16213e);
            border-bottom: 1px solid #0f3460;
        }
        
        /* Title Label */
        #titleLabel {
            color: %3;
            font-size: 16px;
            font-weight: bold;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        
        /* Instance Label */
        #instanceLabel {
            color: %4;
            font-size: 12px;
            font-family: 'Consolas', monospace;
        }
        
        /* Toolbar Buttons */
        QToolButton {
            background-color: transparent;
            color: %2;
            border: none;
            border-radius: 4px;
            font-size: 14px;
            padding: 4px;
        }
        QToolButton:hover {
            background-color: %5;
        }
        
        /* Phone Container */
        #phoneContainer {
            background-color: transparent;
        }
        
        /* Phone Frame - Ultra realistic with gradient */
        #phoneFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #2d3436, stop:0.5 #1e272e, stop:1 #2d3436);
            border-radius: 40px;
            border: 2px solid #4a4a4a;
            /* Subtle shadow effect */
            box-shadow: 
                0 0 30px rgba(0, 0, 0, 0.5),
                inset 0 0 20px rgba(0, 0, 0, 0.3);
        }
        
        /* Phone Bezel - Dark frame around screen */
        #phoneBezel {
            background-color: %6;
            border-radius: 35px;
        }
        
        /* Camera Notch */
        #cameraNotch {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a2e, stop:1 #0d0d1a);
            border-radius: 14px;
            border: 1px solid #3d3d5c;
        }
        
        /* Camera Lens */
        #cameraLens {
            background: qradialgradient(cx:0.5, cy:0.5, radius:0.5,
                stop:0 #1a1a1a, stop:0.7 #2d2d2d, stop:1 #1a1a1a);
            border-radius: 6px;
            border: 1px solid #4a4a4a;
        }
        
        /* Camera Sensor */
        #cameraSensor {
            background: qradialgradient(cx:0.5, cy:0.5, radius:0.5,
                stop:0 #1a1a1a, stop:0.7 #2d2d2d, stop:1 #1a1a1a);
            border-radius: 4px;
            border: 1px solid #4a4a4a;
        }
        
        /* Screen Container */
        #screenContainer {
            background-color: #000000;
            border-radius: 5px;
            border: 1px solid #1a1a1a;
        }
        
        /* Screen Display */
        #screenDisplay {
            background-color: #0a0a0a;
            color: #00ff88;
            border-radius: 3px;
        }
        
        /* Navigation Bar */
        #navigationBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a2e, stop:1 #0f0f23);
            border-radius: 0px 0px 25px 25px;
        }
        
        /* Navigation Buttons */
        #navBackBtn, #navHomeBtn, #navRecentBtn {
            background-color: transparent;
            color: %4;
            border: none;
            border-radius: 20px;
            font-size: 18px;
            padding: 8px;
        }
        #navBackBtn:hover, #navHomeBtn:hover, #navRecentBtn:hover {
            background-color: rgba(255, 255, 255, 0.1);
            color: %3;
        }
        #navHomeBtn {
            color: %3;
        }
        
        /* Home Indicator */
        #homeIndicator {
            background-color: %4;
            border-radius: 2px;
        }
        
        /* Status Bar */
        #statusBar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #16213e, stop:1 #1a1a2e);
            border-top: 1px solid #0f3460;
        }
        
        #connectionStatus {
            color: %2;
            font-size: 12px;
            font-family: 'Consolas', monospace;
        }
        
        #portLabel, #protectionStatus {
            color: %4;
            font-size: 12px;
            font-family: 'Consolas', monospace;
        }
        
        #fpsLabel {
            color: %3;
            font-size: 12px;
            font-family: 'Consolas', monospace;
            font-weight: bold;
        }
        
        #batteryLabel, #timeLabel {
            color: %2;
            font-size: 12px;
            font-family: 'Consolas', monospace;
        }
        
        /* Control Panel */
        #controlPanel {
            background-color: #16213e;
            border-top: 1px solid #0f3460;
        }
        
        #controlPanel QPushButton {
            background-color: %5;
            color: %2;
            border: 1px solid #3d5a80;
            border-radius: 8px;
            font-size: 16px;
            padding: 5px;
        }
        #controlPanel QPushButton:hover {
            background-color: %7;
            border-color: %3;
        }
        #controlPanel QPushButton:pressed {
            background-color: #1a365d;
        }
        
        /* Action Panel */
        #actionPanel {
            background-color: #0f1729;
            border-top: 1px solid #0f3460;
        }
        
        /* QLabel general */
        QLabel {
            color: %2;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        
        /* QMessageBox */
        QMessageBox {
            background-color: %1;
        }
        
        /* QInputDialog */
        QInputDialog {
            background-color: %1;
        }
        
        /* Scrollbar */
        QScrollBar:vertical {
            background: %1;
            width: 12px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: %5;
            border-radius: 6px;
            min-height: 30px;
        }
        QScrollBar::handle:hover:vertical {
            background: %3;
        }
        
        QScrollBar:horizontal {
            background: %1;
            height: 12px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background: %5;
            border-radius: 6px;
            min-width: 30px;
        }
    )").arg(
        COLOR_BACKGROUND,       // %1 - Main background
        COLOR_TEXT,             // %2 - Text color
        COLOR_ACCENT,           // %3 - Accent (green)
        COLOR_TEXT_DIM,         // %4 - Dim text
        COLOR_BUTTON_BG,        // %5 - Button background
        COLOR_BEZEL,            // %6 - Bezel color
        COLOR_BUTTON_HOVER      // %7 - Button hover
    );
    
    setStyleSheet(styleSheet);
}

void PhoneWindow::setupConnections() {
    // FPS timer
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &PhoneWindow::updateFPS);
    m_fpsTimer->start(1000);
    
    // Status bar update timer
    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &PhoneWindow::updateStatusBar);
    statusTimer->start(1000);
}

// ========================================================================
// Screen Mirror Methods
// ========================================================================

void PhoneWindow::startScreenMirror() {
    if (m_screenMirrorActive) return;
    
    m_screenMirrorActive = true;
    m_frameCount = 0;
    
    // Create screen timer
    m_screenTimer = new QTimer(this);
    connect(m_screenTimer, &QTimer::timeout, this, &PhoneWindow::updateScreen);
    m_screenTimer->start(100); // ~10 FPS for screenshots
}

void PhoneWindow::stopScreenMirror() {
    m_screenMirrorActive = false;
    
    if (m_screenTimer) {
        m_screenTimer->stop();
        delete m_screenTimer;
        m_screenTimer = nullptr;
    }
    
    if (m_adbScreenProcess) {
        m_adbScreenProcess->kill();
        m_adbScreenProcess->deleteLater();
        m_adbScreenProcess = nullptr;
    }
}

void PhoneWindow::updateScreen() {
    if (!m_screenMirrorActive) return;
    
    QString adbPath = getAdbPath();
    QString serial = getAdbSerial();
    
    if (adbPath.isEmpty() || serial.isEmpty()) return;
    
    // Execute screencap command
    QStringList args = {
        "-s", serial,
        "exec-out", "screencap", "-p"
    };
    
    if (m_adbScreenProcess) {
        m_adbScreenProcess->kill();
    }
    
    m_adbScreenProcess = new QProcess(this);
    connect(m_adbScreenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneWindow::onScreenProcessFinished);
    
    m_adbScreenProcess->start(adbPath, args);
}

void PhoneWindow::onScreenProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        m_frameCount = 0;
        return;
    }
    
    QByteArray data = m_adbScreenProcess->readAllStandardOutput();
    
    if (!data.isEmpty()) {
        QPixmap pixmap;
        if (pixmap.loadFromData(data, "PNG")) {
            // Scale to fit display
            QPixmap scaled = pixmap.scaled(
                SCREEN_WIDTH, SCREEN_HEIGHT,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            m_screenDisplay->setPixmap(scaled);
            m_frameCount++;
        }
    }
}

// ========================================================================
// Touch Input Methods
// ========================================================================

bool PhoneWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_screenDisplay) {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
                onScreenMousePress(static_cast<QMouseEvent*>(event));
                return true;
            case QEvent::MouseMove:
                onScreenMouseMove(static_cast<QMouseEvent*>(event));
                return true;
            case QEvent::MouseButtonRelease:
                onScreenMouseRelease(static_cast<QMouseEvent*>(event));
                return true;
            case QEvent::MouseButtonDblClick:
                onScreenDoubleClick(static_cast<QMouseEvent*>(event));
                return true;
            default:
                break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void PhoneWindow::onScreenMousePress(QMouseEvent* event) {
    if (!m_screenMirrorActive) return;
    
    m_touchStartPos = event->pos();
    m_isDragging = true;
    m_isSwiping = true;
    m_swipePath.clear();
    m_swipePath.append(QPair<int, int>(toAndroidX(event->pos().x()), 
                                        toAndroidY(event->pos().y())));
}

void PhoneWindow::onScreenMouseMove(QMouseEvent* event) {
    if (!m_isDragging || !m_screenMirrorActive) return;
    
    if (m_isSwiping) {
        m_swipePath.append(QPair<int, int>(toAndroidX(event->pos().x()), 
                                            toAndroidY(event->pos().y())));
    }
}

void PhoneWindow::onScreenMouseRelease(QMouseEvent* event) {
    if (!m_screenMirrorActive) return;
    
    if (m_isSwiping && m_swipePath.size() > 1) {
        // Perform swipe
        int startX = m_swipePath.first().first;
        int startY = m_swipePath.first().second;
        int endX = m_swipePath.last().first;
        int endY = m_swipePath.last().second;
        
        sendAdbSwipe(startX, startY, endX, endY, 200);
    } else if (m_isDragging) {
        // Single tap
        int x = toAndroidX(event->pos().x());
        int y = toAndroidY(event->pos().y());
        sendAdbTap(x, y);
    }
    
    m_isDragging = false;
    m_isSwiping = false;
    m_swipePath.clear();
}

void PhoneWindow::onScreenDoubleClick(QMouseEvent* event) {
    if (!m_screenMirrorActive) return;
    
    int x = toAndroidX(event->pos().x());
    int y = toAndroidY(event->pos().y());
    
    // Double tap
    sendAdbTap(x, y);
    QTimer::singleShot(100, this, [this, x, y]() {
        sendAdbTap(x, y);
    });
}

// ========================================================================
// Navigation & Control Methods
// ========================================================================

void PhoneWindow::onPhoneBackClicked() {
    sendAdbKeyEvent(4); // KEYCODE_BACK
}

void PhoneWindow::onPhoneHomeClicked() {
    sendAdbKeyEvent(3); // KEYCODE_HOME
}

void PhoneWindow::onPhoneRecentClicked() {
    sendAdbKeyEvent(187); // KEYCODE_APP_SWITCH
}

void PhoneWindow::onBackClicked() { onPhoneBackClicked(); }
void PhoneWindow::onHomeClicked() { onPhoneHomeClicked(); }
void PhoneWindow::onRecentClicked() { onPhoneRecentClicked(); }

void PhoneWindow::onPowerClicked() {
    sendAdbKeyEvent(26); // KEYCODE_POWER
}

void PhoneWindow::onVolumeUp() {
    sendAdbKeyEvent(24); // KEYCODE_VOLUME_UP
}

void PhoneWindow::onVolumeDown() {
    sendAdbKeyEvent(25); // KEYCODE_VOLUME_DOWN
}

void PhoneWindow::onRotateScreen() {
    sendAdbKeyEvent(204); // KEYCODE_SCREEN_ROTATION
}

void PhoneWindow::onScreenshotsClicked() {
    QString serial = getAdbSerial();
    QString adbPath = getAdbPath();
    
    if (adbPath.isEmpty() || serial.isEmpty()) return;
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString filename = QString("screenshot_%1.png").arg(timestamp);
    
    QStringList args = {
        "-s", serial,
        "exec-out", "screencap", "-p", ">", filename
    };
    
    executeAdbCommandSync(args);
    
    QMessageBox::information(this, "Screenshot", 
        QString("Screenshot saved: %1").arg(filename));
}

void PhoneWindow::onRecordScreenClicked() {
    QMessageBox::information(this, "Record Screen",
        "Screen recording started.\nClick again to stop.");
}

void PhoneWindow::onInstallApkClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select APK File",
        QString(),
        "APK Files (*.apk);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        installApk(filePath);
    }
}

void PhoneWindow::onOpenAppsClicked() {
    AppManagerDialog* dialog = new AppManagerDialog(m_instanceId, this);
    dialog->exec();
}

void PhoneWindow::onSettingsClicked() {
    QMessageBox::information(this, "Settings",
        "Device Settings\n\n"
        "Configure anti-detection settings for this device instance.");
}

void PhoneWindow::onAntiDetectionClicked() {
    QMessageBox::information(this, "Anti-Detection",
        "Anti-Detection Panel\n\n"
        "Configure anti-detection features:\n"
        "• Root hiding\n"
        "• Emulator detection bypass\n"
        "• TLS fingerprinting\n"
        "• Hardware spoofing\n"
        "• And more...");
}

// ========================================================================
// ADB Helper Methods
// ========================================================================

void PhoneWindow::sendAdbTap(int x, int y) {
    QStringList args = {
        "-s", getAdbSerial(),
        "shell", "input", "tap",
        QString::number(x), QString::number(y)
    };
    
    QString adbPath = getAdbPath();
    if (!adbPath.isEmpty()) {
        QProcess::startDetached(adbPath, args);
    }
}

void PhoneWindow::sendAdbSwipe(int x1, int y1, int x2, int y2, int duration) {
    QStringList args = {
        "-s", getAdbSerial(),
        "shell", "input", "swipe",
        QString::number(x1), QString::number(y1),
        QString::number(x2), QString::number(y2),
        QString::number(duration)
    };
    
    QString adbPath = getAdbPath();
    if (!adbPath.isEmpty()) {
        QProcess::startDetached(adbPath, args);
    }
}

void PhoneWindow::sendAdbKeyEvent(int keyCode) {
    QStringList args = {
        "-s", getAdbSerial(),
        "shell", "input", "keyevent",
        QString::number(keyCode)
    };
    
    QString adbPath = getAdbPath();
    if (!adbPath.isEmpty()) {
        QProcess::startDetached(adbPath, args);
    }
}

void PhoneWindow::sendAdbText(const QString& text) {
    QStringList args = {
        "-s", getAdbSerial(),
        "shell", "input", "text",
        text
    };
    
    QString adbPath = getAdbPath();
    if (!adbPath.isEmpty()) {
        QProcess::startDetached(adbPath, args);
    }
}

QString PhoneWindow::executeAdbCommandSync(const QStringList& args, int timeoutMs) {
    QString result;
    
    QString adbPath = getAdbPath();
    if (adbPath.isEmpty()) return result;
    
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(adbPath, args);
    
    if (process.waitForFinished(timeoutMs)) {
        result = process.readAll();
    }
    
    return result;
}

QString PhoneWindow::getAdbSerial() const {
    return m_instanceId;
}

QString PhoneWindow::getAdbPath() const {
    // Try to find adb in common locations
    QStringList paths = {
#ifdef _WIN32
        "adb.exe",
        "C:/Android/platform-tools/adb.exe",
        "C:/Program Files/Android/platform-tools/adb.exe",
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/adb.exe"),
#else
        "/usr/bin/adb",
        "/usr/local/bin/adb",
        "/opt/android-sdk/platform-tools/adb",
#endif
    };
    
    for (const QString& path : paths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    return "adb"; // Fallback to PATH
}

int PhoneWindow::toAndroidX(int labelX) const {
    // Get the actual scaled size of the display
    QPixmap pix = m_screenDisplay->pixmap();
    if (pix.isNull()) return 0;
    
    int displayWidth = pix.width();
    if (displayWidth <= 0) return 0;
    
    // Scale from display coordinates to Android screen coordinates
    // Assuming Android screen is 1080 wide (common)
    return (labelX * 1080) / displayWidth;
}

int PhoneWindow::toAndroidY(int labelY) const {
    QPixmap pix = m_screenDisplay->pixmap();
    if (pix.isNull()) return 0;
    
    int displayHeight = pix.height();
    if (displayHeight <= 0) return 0;
    
    // Assuming Android screen is 1920 tall (common)
    return (labelY * 1920) / displayHeight;
}

// ========================================================================
// Status Update Methods
// ========================================================================

void PhoneWindow::updateStatusBar() {
    m_timeLabel->setText(QTime::currentTime().toString("HH:mm"));
}

void PhoneWindow::updateFPS() {
    m_currentFPS = m_frameCount;
    m_frameCount = 0;
    
    m_fpsLabel->setText(QString("FPS: %1").arg(m_currentFPS));
    
    // Color based on FPS
    if (m_currentFPS >= 10) {
        m_fpsLabel->setStyleSheet("color: #00ff88; font-weight: bold;");
    } else if (m_currentFPS >= 5) {
        m_fpsLabel->setStyleSheet("color: #ffd700; font-weight: bold;");
    } else {
        m_fpsLabel->setStyleSheet("color: #ff4757; font-weight: bold;");
    }
}

void PhoneWindow::updateWindowTitle() {
    QString title = QString("%1 - Instance %2").arg(m_deviceName).arg(m_instanceNumber);
    setWindowTitle(title);
    m_titleLabel->setText(m_deviceName);
    m_instanceLabel->setText(QString("Instance %1").arg(m_instanceNumber));
}

void PhoneWindow::setConnected(bool connected) {
    if (connected) {
        m_connectionStatus->setText("🟢 Connected");
        m_connectionStatus->setStyleSheet("color: #00ff88;");
        m_portLabel->setText(QString("| Port: %1").arg(5555 + m_instanceNumber));
    } else {
        m_connectionStatus->setText("⚪ Disconnected");
        m_connectionStatus->setStyleSheet("color: #8892b0;");
        m_portLabel->setText("| Port: --");
    }
}

void PhoneWindow::onInstanceStateChanged(const QString& instanceId, InstanceState state) {
    if (instanceId != m_instanceId) return;
    
    switch (state) {
        case InstanceState::STOPPED:
            m_protectionStatus->setText("🛡️ State: Stopped");
            stopScreenMirror();
            break;
        case InstanceState::STARTING:
            m_protectionStatus->setText("🛡️ State: Starting...");
            break;
        case InstanceState::RUNNING:
            m_protectionStatus->setText("🛡️ State: Running");
            startScreenMirror();
            setConnected(true);
            break;
        case InstanceState::ERROR:
            m_protectionStatus->setText("🛡️ State: Error");
            break;
    }
}

void PhoneWindow::onAdbConnectionChanged(const QString& instanceId, bool connected) {
    if (instanceId != m_instanceId) return;
    setConnected(connected);
}

// ========================================================================
// Window Control Methods
// ========================================================================

void PhoneWindow::onMinimizeClicked() {
    showMinimized();
}

void PhoneWindow::onMaximizeClicked() {
    if (isMaximized()) {
        showNormal();
        m_maximizeBtn->setText("□");
    } else {
        showMaximized();
        m_maximizeBtn->setText("❐");
    }
}

void PhoneWindow::onCloseClicked() {
    close();
}

void PhoneWindow::onAlwaysOnTopToggled(bool checked) {
    if (checked) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    show();
}

// ========================================================================
// Event Handlers
// ========================================================================

void PhoneWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            onPhoneBackClicked();
            break;
        case Qt::Key_Home:
            onPhoneHomeClicked();
            break;
        case Qt::Key_Menu:
            onPhoneRecentClicked();
            break;
        case Qt::Key_F5:
            refreshInstance();
            break;
        case Qt::Key_F11:
            onMaximizeClicked();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void PhoneWindow::closeEvent(QCloseEvent* event) {
    stopScreenMirror();
    event->accept();
}

void PhoneWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (filePath.endsWith(".apk", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void PhoneWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (filePath.endsWith(".apk", Qt::CaseInsensitive)) {
                installApk(filePath);
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void PhoneWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}

void PhoneWindow::refreshInstance() {
    updateScreen();
}

void PhoneWindow::installApk(const QString& apkPath) {
    QString serial = getAdbSerial();
    QString adbPath = getAdbPath();
    
    if (adbPath.isEmpty() || serial.isEmpty()) {
        QMessageBox::warning(this, "Error", "ADB not available");
        return;
    }
    
    m_installProgress = new QProgressDialog(
        QString("Installing: %1").arg(QFileInfo(apkPath).fileName()),
        "Cancel", 0, 100, this
    );
    m_installProgress->setWindowModality(Qt::WindowModal);
    m_installProgress->setValue(0);
    m_installProgress->show();
    
    QStringList args = {
        "-s", serial,
        "install", "-r", apkPath
    };
    
    m_installProcess = new QProcess(this);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneWindow::onInstallFinished);
    
    m_installProcess->start(adbPath, args);
}

void PhoneWindow::onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_installProgress) {
        m_installProgress->setValue(100);
        m_installProgress->close();
        delete m_installProgress;
        m_installProgress = nullptr;
    }
    
    QString output = m_installProcess->readAllStandardOutput();
    
    if (exitCode == 0 && output.contains("Success")) {
        QMessageBox::information(this, "Success", "APK installed successfully!");
    } else {
        QMessageBox::warning(this, "Error", 
            QString("Installation failed:\n%1").arg(output));
    }
    
    delete m_installProcess;
    m_installProcess = nullptr;
}

} // namespace VirtualPhonePro
