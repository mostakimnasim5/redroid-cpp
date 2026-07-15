/**
 * @file mainwindow.cpp
 * @brief Main Window Implementation
 * @version 3.0 Ultimate Banking Edition
 */

#include "mainwindow.h"
#include "devicecard.h"
#include "dashboard.h"
#include "anti_detection_panel.h"
#include "device_profile_manager.h"
#include "vnc_viewer.h"
#include "settings_dialog.h"
#include "about_dialog.h"
#include "log_viewer.h"
#include "network_panel.h"
#include "simulation_panel.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDialog>
#include <QDebug>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dashboard(nullptr)
    , m_antiDetectionPanel(nullptr)
    , m_networkPanel(nullptr)
    , m_simulationPanel(nullptr)
    , m_profileManager(nullptr)
    , m_vncViewer(nullptr)
    , m_logViewer(nullptr)
    , m_settingsDialog(nullptr)
    , m_aboutDialog(nullptr)
    , m_deviceGridWidget(nullptr)
    , m_deviceGridLayout(nullptr)
    , m_trayIcon(nullptr)
    , m_updateTimer(nullptr)
    , m_nextVncPort(5900)
    , m_nextAdbPort(5555)
    , m_autoStartEnabled(false)
    , m_minimizeToTray(true)
    , m_maxDevices(10)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupDockWidgets();
    setupConnections();
    setupShortcuts();
    createSystemTray();
    
    loadSettings();
    loadDevices();
    createDeviceGrid();
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateTimer);
    m_updateTimer->start(5000);
    
    log("ReDroidCPP v3.0 started", "INFO");
}

MainWindow::~MainWindow()
{
    saveSettings();
    saveDevices();
}

void MainWindow::setupUI()
{
    setWindowTitle("ReDroidCPP - Ultra Advanced Anti-Detection System v3.0");
    setWindowIcon(QIcon(":/icons/app.png"));
    resize(1400, 900);
    minimumSize(1200, 700);
    
    // Central Widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Main Splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    // Device Grid (Center)
    m_deviceGridWidget = new QWidget();
    QVBoxLayout* deviceLayout = new QVBoxLayout(m_deviceGridWidget);
    deviceLayout->setContentsMargins(10, 10, 10, 10);
    
    QLabel* deviceTitle = new QLabel("Virtual Devices");
    deviceTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #00d4ff;");
    deviceLayout->addWidget(deviceTitle);
    
    QLabel* deviceSubtitle = new QLabel("Click '+' to create a new virtual Android device");
    deviceSubtitle->setStyleSheet("color: #888; font-size: 12px;");
    deviceLayout->addWidget(deviceSubtitle);
    
    m_deviceGridLayout = new QGridLayout();
    m_deviceGridLayout->setSpacing(20);
    m_deviceGridLayout->setContentsMargins(0, 20, 0, 0);
    
    QWidget* gridContainer = new QWidget();
    gridContainer->setLayout(m_deviceGridLayout);
    deviceLayout->addWidget(gridContainer);
    
    // Add Device Button
    QPushButton* addBtn = new QPushButton("+ Create New Device");
    addBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00d4ff, stop:1 #0099cc);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 15px 30px;
            font-size: 14px;
            font-weight: bold;
            min-width: 200px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00e5ff, stop:1 #00b8e6);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0099cc, stop:1 #007799);
        }
    )");
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onNewDevice);
    deviceLayout->addWidget(addBtn, 0, Qt::AlignCenter);
    
    m_mainSplitter->addWidget(m_deviceGridWidget);
    
    // Dashboard (Right Panel)
    m_dashboard = new Dashboard(this);
    m_mainSplitter->addWidget(m_dashboard);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
}

void MainWindow::setupMenuBar()
{
    // File Menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* newDeviceAction = new QAction(QIcon(":/icons/add.png"), "New Device", this);
    newDeviceAction->setShortcut(QKeySequence::New);
    connect(newDeviceAction, &QAction::triggered, this, &MainWindow::onNewDevice);
    fileMenu->addAction(newDeviceAction);
    
    fileMenu->addSeparator();
    
    QAction* importAction = new QAction(QIcon(":/icons/import.png"), "Import Profile", this);
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportProfile);
    fileMenu->addAction(importAction);
    
    QAction* exportAction = new QAction(QIcon(":/icons/export.png"), "Export Profile", this);
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportProfile);
    fileMenu->addAction(exportAction);
    
    fileMenu->addSeparator();
    
    QAction* settingsAction = new QAction(QIcon(":/icons/settings.png"), "Settings", this);
    settingsAction->setShortcut(QKeySequence::PreferencesStart);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    fileMenu->addAction(settingsAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction(QIcon(":/icons/exit.png"), "Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);
    fileMenu->addAction(exitAction);
    
    // Device Menu
    QMenu* deviceMenu = menuBar()->addMenu("&Device");
    
    QAction* startAction = new QAction(QIcon(":/icons/play.png"), "Start Device", this);
    startAction->setShortcut(Qt::CTRL + Qt::Key_S);
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartDevice);
    deviceMenu->addAction(startAction);
    
    QAction* stopAction = new QAction(QIcon(":/icons/stop.png"), "Stop Device", this);
    stopAction->setShortcut(Qt::CTRL + Qt::Key_T);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopDevice);
    deviceMenu->addAction(stopAction);
    
    QAction* restartAction = new QAction(QIcon(":/icons/restart.png"), "Restart Device", this);
    restartAction->setShortcut(Qt::CTRL + Qt::Key_R);
    connect(restartAction, &QAction::triggered, this, &MainWindow::onRestartDevice);
    deviceMenu->addAction(restartAction);
    
    deviceMenu->addSeparator();
    
    QAction* viewScreenAction = new QAction(QIcon(":/icons/screen.png"), "View Screen", this);
    viewScreenAction->setShortcut(Qt::CTRL + Qt::Key_V);
    connect(viewScreenAction, &QAction::triggered, this, &MainWindow::onViewScreen);
    deviceMenu->addAction(viewScreenAction);
    
    deviceMenu->addSeparator();
    
    QAction* cloneAction = new QAction(QIcon(":/icons/clone.png"), "Clone Device", this);
    connect(cloneAction, &QAction::triggered, this, &MainWindow::onCloneDevice);
    deviceMenu->addAction(cloneAction);
    
    QAction* deleteAction = new QAction(QIcon(":/icons/delete.png"), "Delete Device", this);
    deleteAction->setShortcut(Qt::Key_Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteDevice);
    deviceMenu->addAction(deleteAction);
    
    // Anti-Detection Menu
    QMenu* antiMenu = menuBar()->addMenu("&Anti-Detection");
    
    QAction* applyAllAction = new QAction(QIcon(":/icons/shield.png"), "Apply Complete Anti-Detection", this);
    applyAllAction->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(applyAllAction, &QAction::triggered, this, &MainWindow::onApplyAntiDetection);
    antiMenu->addAction(applyAllAction);
    
    antiMenu->addSeparator();
    
    QAction* bankingAction = new QAction(QIcon(":/icons/bank.png"), "Apply Banking Setup", this);
    bankingAction->setShortcut(Qt::CTRL + Qt::Key_B);
    connect(bankingAction, &QAction::triggered, this, &MainWindow::onApplyBankingSetup);
    antiMenu->addAction(bankingAction);
    
    QAction* googleAction = new QAction(QIcon(":/icons/google.png"), "Apply Google Setup", this);
    googleAction->setShortcut(Qt::CTRL + Qt::Key_G);
    connect(googleAction, &QAction::triggered, this, &MainWindow::onApplyGoogleSetup);
    antiMenu->addAction(googleAction);
    
    antiMenu->addSeparator();
    
    QAction* customAction = new QAction(QIcon(":/icons/custom.png"), "Custom Profile", this);
    connect(customAction, &QAction::triggered, this, &MainWindow::onCustomProfile);
    antiMenu->addAction(customAction);
    
    // View Menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    QAction* dashboardAction = new QAction("Dashboard", this);
    connect(dashboardAction, &QAction::triggered, this, &MainWindow::onShowDashboard);
    viewMenu->addAction(dashboardAction);
    
    QAction* logAction = new QAction("Log Viewer", this);
    logAction->setShortcut(Qt::CTRL + Qt::Key_L);
    connect(logAction, &QAction::triggered, this, &MainWindow::onShowLogViewer);
    viewMenu->addAction(logAction);
    
    viewMenu->addSeparator();
    
    QAction* fullscreenAction = new QAction("Fullscreen", this);
    fullscreenAction->setShortcut(Qt::Key_F11);
    connect(fullscreenAction, &QAction::triggered, this, &MainWindow::onToggleFullscreen);
    viewMenu->addAction(fullscreenAction);
    
    // Help Menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = new QAction(QIcon(":/icons/info.png"), "About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction);
    
    QAction* docsAction = new QAction("Documentation", this);
    docsAction->setShortcut(Qt::Key_F1);
    connect(docsAction, &QAction::triggered, this, &MainWindow::onDocumentation);
    helpMenu->addAction(docsAction);
}

void MainWindow::setupToolBar()
{
    // Main Toolbar
    m_mainToolBar = addToolBar("Main");
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->setIconSize(QSize(24, 24));
    
    QLabel* logoLabel = new QLabel("  ReDroidCPP  ");
    logoLabel->setStyleSheet(R"(
        QLabel {
            color: #00d4ff;
            font-size: 16px;
            font-weight: bold;
            padding: 5px 15px;
            background: rgba(0, 212, 255, 0.1);
            border-radius: 5px;
        }
    )");
    m_mainToolBar->addWidget(logoLabel);
    m_mainToolBar->addSeparator();
    
    QPushButton* newBtn = new QPushButton("New Device");
    newBtn->setIcon(QIcon(":/icons/add.png"));
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::onNewDevice);
    m_mainToolBar->addWidget(newBtn);
    
    m_mainToolBar->addSeparator();
    
    // Anti-Detection Buttons
    QPushButton* antiBtn = new QPushButton("Anti-Detection");
    antiBtn->setIcon(QIcon(":/icons/shield.png"));
    connect(antiBtn, &QPushButton::clicked, this, &MainWindow::onApplyAntiDetection);
    m_mainToolBar->addWidget(antiBtn);
    
    QPushButton* bankingBtn = new QPushButton("Banking Mode");
    bankingBtn->setIcon(QIcon(":/icons/bank.png"));
    connect(bankingBtn, &QPushButton::clicked, this, &MainWindow::onApplyBankingSetup);
    m_mainToolBar->addWidget(bankingBtn);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #00ff00;");
    statusBar()->addWidget(m_statusLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    m_deviceCountLabel = new QLabel("Devices: 0");
    statusBar()->addPermanentWidget(m_deviceCountLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    m_memoryLabel = new QLabel("Memory: 0%");
    statusBar()->addPermanentWidget(m_memoryLabel);
    
    m_cpuProgressBar = new QProgressBar();
    m_cpuProgressBar->setMaximumWidth(100);
    m_cpuProgressBar->setMaximum(100);
    m_cpuProgressBar->setValue(0);
    statusBar()->addPermanentWidget(new QLabel("CPU:"));
    statusBar()->addPermanentWidget(m_cpuProgressBar);
}

void MainWindow::setupDockWidgets()
{
    // Anti-Detection Panel
    QDockWidget* antiDock = new QDockWidget("Anti-Detection", this);
    antiDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_antiDetectionPanel = new AntiDetectionPanel(this);
    antiDock->setWidget(m_antiDetectionPanel);
    addDockWidget(Qt::RightDockWidgetArea, antiDock);
    
    // Network Panel
    QDockWidget* networkDock = new QDockWidget("Network", this);
    networkDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_networkPanel = new NetworkPanel(this);
    networkDock->setWidget(m_networkPanel);
    addDockWidget(Qt::BottomDockWidgetArea, networkDock);
    
    // Simulation Panel
    QDockWidget* simDock = new QDockWidget("Simulation", this);
    simDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_simulationPanel = new SimulationPanel(this);
    simDock->setWidget(m_simulationPanel);
    addDockWidget(Qt::BottomDockWidgetArea, simDock);
    
    // Profile Manager
    QDockWidget* profileDock = new QDockWidget("Device Profiles", this);
    profileDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_profileManager = new DeviceProfileManager(this);
    profileDock->setWidget(m_profileManager);
    addDockWidget(Qt::RightDockWidgetArea, profileDock);
}

void MainWindow::setupConnections()
{
    // Connect anti-detection panel signals
    connect(m_antiDetectionPanel, &AntiDetectionPanel::applyRequested, 
            this, &MainWindow::onApplyAntiDetection);
    
    // Connect network panel signals
    connect(m_networkPanel, &NetworkPanel::applyRequested, this, [this](const QString& deviceId) {
        log("Network settings applied to: " + deviceId);
    });
    
    // Connect simulation panel signals
    connect(m_simulationPanel, &SimulationPanel::applyRequested, this, [this](const QString& deviceId) {
        log("Simulation settings applied to: " + deviceId);
    });
    
    // Connect profile manager signals
    connect(m_profileManager, &DeviceProfileManager::profileSelected, this, [this](const QString& profile) {
        log("Profile selected: " + profile);
    });
    
    // Connect dashboard signals
    connect(m_dashboard, &Dashboard::deviceSelected, this, [this](const QString& deviceId) {
        log("Device selected: " + deviceId);
    });
}

void MainWindow::setupShortcuts()
{
    // Already set in menu bar
}

void MainWindow::createSystemTray()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/app.png"));
    
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("Show Window", this, [this]() { show(); });
    m_trayMenu->addAction("Hide Window", this, [this]() { hide(); });
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Exit", this, &MainWindow::onExit);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    
    m_trayIcon->show();
}

void MainWindow::createDeviceGrid()
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_deviceGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // Add "Create New" card at position 0,0
    int row = 0;
    int col = 0;
    int maxCols = 3;
    
    // Create button to add new device
    QPushButton* addCard = new QPushButton();
    addCard->setFixedSize(280, 200);
    addCard->setText("+ Create New Device");
    addCard->setStyleSheet(R"(
        QPushButton {
            background: rgba(0, 212, 255, 0.1);
            border: 2px dashed #00d4ff;
            border-radius: 15px;
            color: #00d4ff;
            font-size: 16px;
        }
        QPushButton:hover {
            background: rgba(0, 212, 255, 0.2);
        }
    )");
    connect(addCard, &QPushButton::clicked, this, &MainWindow::onNewDevice);
    
    m_deviceGridLayout->addWidget(addCard, row, col);
    col++;
    if (col >= maxCols) { col = 0; row++; }
    
    // Add device cards
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        DeviceCard* card = new DeviceCard(it.value(), this);
        m_deviceCards[it.key()] = card;
        
        connect(card, &DeviceCard::clicked, this, &MainWindow::onDeviceCardClicked);
        connect(card, &DeviceCard::startClicked, this, &MainWindow::onDeviceStartClicked);
        connect(card, &DeviceCard::stopClicked, this, &MainWindow::onDeviceStopClicked);
        connect(card, &DeviceCard::screenClicked, this, &MainWindow::onDeviceScreenClicked);
        connect(card, &DeviceCard::settingsClicked, this, &MainWindow::onDeviceSettingsClicked);
        connect(card, &DeviceCard::deleteClicked, this, &MainWindow::onDeviceDeleteClicked);
        
        m_deviceGridLayout->addWidget(card, row, col);
        col++;
        if (col >= maxCols) { col = 0; row++; }
    }
}

void MainWindow::addDevice(const DeviceInfo& device)
{
    m_devices[device.id] = device;
    createDeviceGrid();
    updateStatistics();
    log("Device created: " + device.name);
}

void MainWindow::removeDevice(const QString& deviceId)
{
    if (m_devices.contains(deviceId)) {
        QString name = m_devices[deviceId].name;
        m_devices.remove(deviceId);
        if (m_deviceCards.contains(deviceId)) {
            delete m_deviceCards[deviceId];
            m_deviceCards.remove(deviceId);
        }
        createDeviceGrid();
        updateStatistics();
        log("Device deleted: " + name);
    }
}

void MainWindow::updateDeviceStatus(const QString& deviceId, const QString& status)
{
    if (m_devices.contains(deviceId)) {
        m_devices[deviceId].status = status;
        if (m_deviceCards.contains(deviceId)) {
            m_deviceCards[deviceId]->updateStatus(status);
        }
    }
}

DeviceInfo MainWindow::getDeviceInfo(const QString& deviceId)
{
    return m_devices.value(deviceId);
}

QList<DeviceInfo> MainWindow::getAllDevices()
{
    return m_devices.values();
}

void MainWindow::applyAntiDetection(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;
    
    log("Applying complete anti-detection to: " + deviceId);
    updateDeviceStatus(deviceId, "Applying anti-detection...");
    
    // Simulate anti-detection process
    QTimer::singleShot(2000, this, [this, deviceId]() {
        updateDeviceStatus(deviceId, "Anti-detection active");
        m_devices[deviceId].antiDetectionEnabled = true;
        emit antiDetectionApplied(deviceId);
        log("Anti-detection applied successfully to: " + deviceId);
    });
}

void MainWindow::applyBankingSetup(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;
    
    log("Applying banking setup to: " + deviceId);
    updateDeviceStatus(deviceId, "Applying banking mode...");
    
    QTimer::singleShot(1500, this, [this, deviceId]() {
        updateDeviceStatus(deviceId, "Banking mode active");
        log("Banking setup applied to: " + deviceId);
    });
}

void MainWindow::applyGoogleSetup(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;
    
    log("Applying Google setup to: " + deviceId);
    updateDeviceStatus(deviceId, "Applying Google mode...");
    
    QTimer::singleShot(1500, this, [this, deviceId]() {
        updateDeviceStatus(deviceId, "Google mode active");
        log("Google setup applied to: " + deviceId);
    });
}

void MainWindow::showVNCViewer(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;
    
    if (!m_vncViewer) {
        m_vncViewer = new VNCViewer(this);
    }
    
    DeviceInfo info = m_devices[deviceId];
    m_vncViewer->connectToDevice(info.vncPort, info.name);
    m_vncViewer->show();
    
    log("Opening VNC viewer for: " + deviceId);
}

// Menu Actions
void MainWindow::onNewDevice()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Device", 
                                          "Enter device name:", 
                                          QLineEdit::Normal, 
                                          "Device-" + QString::number(m_devices.size() + 1),
                                          &ok);
    if (!ok || name.isEmpty()) return;
    
    // Select manufacturer and model
    QStringList manufacturers = {"Samsung", "Google", "Xiaomi", "OnePlus", "Huawei", "Oppo", "Vivo"};
    QString manufacturer = QInputDialog::getItem(this, "Manufacturer", 
                                               "Select manufacturer:", 
                                               manufacturers, 0, false, &ok);
    if (!ok) return;
    
    QStringList models;
    if (manufacturer == "Samsung") {
        models = {"SM-S928B (S24 Ultra)", "SM-S921B (S24)", "SM-S918B (S23 Ultra)", "SM-A546E (A54)"};
    } else if (manufacturer == "Google") {
        models = {"Pixel 8 Pro", "Pixel 8", "Pixel 7 Pro", "Pixel 7"};
    } else if (manufacturer == "Xiaomi") {
        models = {"Mi 14 Ultra", "Mi 13 Pro", "Xiaomi 13"};
    } else {
        models = {"Default Model"};
    }
    
    QString model = QInputDialog::getItem(this, "Model", 
                                         "Select model:", 
                                         models, 0, false, &ok);
    if (!ok) return;
    
    // Create device
    DeviceInfo device;
    device.id = name.toLower().replace(" ", "-");
    device.name = name;
    device.manufacturer = manufacturer;
    device.model = model;
    device.status = "Created";
    device.vncPort = m_nextVncPort++;
    device.adbPort = m_nextAdbPort++;
    device.isRunning = false;
    device.antiDetectionEnabled = false;
    device.statusColor = QColor("#888888");
    
    addDevice(device);
}

void MainWindow::onImportProfile()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Profile", "", 
                                                "JSON Files (*.json);;All Files (*)");
    if (file.isEmpty()) return;
    
    log("Importing profile from: " + file);
}

void MainWindow::onExportProfile()
{
    QString file = QFileDialog::getSaveFileName(this, "Export Profile", "", 
                                                "JSON Files (*.json);;All Files (*)");
    if (file.isEmpty()) return;
    
    log("Exporting profile to: " + file);
}

void MainWindow::onSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    m_settingsDialog->show();
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onStartDevice()
{
    QStringList deviceIds = m_devices.keys();
    if (deviceIds.isEmpty()) return;
    
    QString id = deviceIds.first();
    if (m_devices.contains(id)) {
        m_devices[id].isRunning = true;
        m_devices[id].status = "Running";
        m_devices[id].statusColor = QColor("#00ff00");
        createDeviceGrid();
        log("Device started: " + id);
    }
}

void MainWindow::onStopDevice()
{
    QStringList deviceIds = m_devices.keys();
    if (deviceIds.isEmpty()) return;
    
    QString id = deviceIds.first();
    if (m_devices.contains(id)) {
        m_devices[id].isRunning = false;
        m_devices[id].status = "Stopped";
        m_devices[id].statusColor = QColor("#ff6600");
        createDeviceGrid();
        log("Device stopped: " + id);
    }
}

void MainWindow::onRestartDevice()
{
    onStopDevice();
    QTimer::singleShot(500, this, &MainWindow::onStartDevice);
}

void MainWindow::onDeleteDevice()
{
    QStringList deviceIds = m_devices.keys();
    if (deviceIds.isEmpty()) return;
    
    QString id = deviceIds.first();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Device", 
        "Are you sure you want to delete this device?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        removeDevice(id);
    }
}

void MainWindow::onCloneDevice()
{
    QStringList deviceIds = m_devices.keys();
    if (deviceIds.isEmpty()) return;
    
    QString sourceId = deviceIds.first();
    DeviceInfo source = m_devices[sourceId];
    
    bool ok;
    QString name = QInputDialog::getText(this, "Clone Device", 
                                          "Enter new device name:", 
                                          QLineEdit::Normal, 
                                          source.name + "-clone",
                                          &ok);
    if (!ok || name.isEmpty()) return;
    
    DeviceInfo clone = source;
    clone.id = name.toLower().replace(" ", "-");
    clone.name = name;
    clone.vncPort = m_nextVncPort++;
    clone.adbPort = m_nextAdbPort++;
    
    addDevice(clone);
    log("Device cloned: " + source.name + " -> " + name);
}

void MainWindow::onViewScreen()
{
    QStringList deviceIds = m_devices.keys();
    if (deviceIds.isEmpty()) return;
    
    showVNCViewer(deviceIds.first());
}

void MainWindow::onApplyAntiDetection()
{
    QStringList deviceIds = m_devices.keys();
    for (const QString& id : deviceIds) {
        applyAntiDetection(id);
    }
}

void MainWindow::onApplyBankingSetup()
{
    QStringList deviceIds = m_devices.keys();
    for (const QString& id : deviceIds) {
        applyBankingSetup(id);
    }
}

void MainWindow::onApplyGoogleSetup()
{
    QStringList deviceIds = m_devices.keys();
    for (const QString& id : deviceIds) {
        applyGoogleSetup(id);
    }
}

void MainWindow::onCustomProfile()
{
    log("Opening custom profile dialog");
}

void MainWindow::onShowDashboard()
{
    m_dashboard->show();
}

void MainWindow::onShowGridView()
{
    createDeviceGrid();
}

void MainWindow::onShowListView()
{
    createDeviceList();
}

void MainWindow::onShowLogViewer()
{
    if (!m_logViewer) {
        m_logViewer = new LogViewer(this);
    }
    m_logViewer->show();
}

void MainWindow::onToggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::onAbout()
{
    if (!m_aboutDialog) {
        m_aboutDialog = new AboutDialog(this);
    }
    m_aboutDialog->show();
}

void MainWindow::onDocumentation()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mostakimnasim5/redroid-cpp"));
}

void MainWindow::onCheckUpdates()
{
    QMessageBox::information(this, "Updates", "You are using the latest version!");
}

// Device Card Slots
void MainWindow::onDeviceCardClicked(const QString& deviceId)
{
    log("Device selected: " + deviceId);
    m_dashboard->showDeviceDetails(m_devices[deviceId]);
}

void MainWindow::onDeviceStartClicked(const QString& deviceId)
{
    if (m_devices.contains(deviceId)) {
        m_devices[deviceId].isRunning = true;
        m_devices[deviceId].status = "Running";
        m_devices[deviceId].statusColor = QColor("#00ff00");
        createDeviceGrid();
        log("Device started: " + deviceId);
    }
}

void MainWindow::onDeviceStopClicked(const QString& deviceId)
{
    if (m_devices.contains(deviceId)) {
        m_devices[deviceId].isRunning = false;
        m_devices[deviceId].status = "Stopped";
        m_devices[deviceId].statusColor = QColor("#ff6600");
        createDeviceGrid();
        log("Device stopped: " + deviceId);
    }
}

void MainWindow::onDeviceScreenClicked(const QString& deviceId)
{
    showVNCViewer(deviceId);
}

void MainWindow::onDeviceSettingsClicked(const QString& deviceId)
{
    log("Opening settings for: " + deviceId);
}

void MainWindow::onDeviceDeleteClicked(const QString& deviceId)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Device", 
        "Are you sure you want to delete this device?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        removeDevice(deviceId);
    }
}

// System Tray
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
    }
}

void MainWindow::onShowTrayMessage(const QString& title, const QString& message)
{
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 3000);
}

// Timer
void MainWindow::onUpdateTimer()
{
    onStatusUpdate();
    updateStatistics();
}

void MainWindow::onStatusUpdate()
{
    m_statusLabel->setText("Ready - " + QDateTime::currentDateTime().toString("hh:mm:ss"));
}

void MainWindow::onSettingsChanged()
{
    loadSettings();
}

// Settings
void MainWindow::loadSettings()
{
    // Load from QSettings
}

void MainWindow::saveSettings()
{
    // Save to QSettings
}

void MainWindow::loadDevices()
{
    // Load devices from config
}

void MainWindow::saveDevices()
{
    // Save devices to config
}

void MainWindow::log(const QString& message, const QString& level)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp, level, message);
    
    qDebug() << logMessage;
    
    if (m_logViewer) {
        m_logViewer->addLog(logMessage);
    }
}

void MainWindow::updateStatistics()
{
    int deviceCount = m_devices.size();
    int runningCount = 0;
    
    for (const DeviceInfo& info : m_devices.values()) {
        if (info.isRunning) runningCount++;
    }
    
    m_deviceCountLabel->setText(QString("Devices: %1 (%2 running)").arg(deviceCount).arg(runningCount));
    m_dashboard->updateStatistics(deviceCount, runningCount);
}
