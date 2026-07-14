#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QTimer>
#include <QDebug>
#include <QStandardPaths>

using namespace VirtualPhonePro;

// ==============================================================================
// MainWindow Implementation
// ==============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupInstanceTable();
    setupDetailsPanel();
    setupConnections();
    
    // Load existing instances
    refreshInstances();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    setWindowTitle("VirtualPhonePro - Android Emulator Manager");
    setMinimumSize(1024, 768);
    resize(1280, 800);
    
    // Central widget with splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Instance table (left panel)
    m_instanceTable = new QTableWidget(this);
    m_instanceTable->setColumnCount(6);
    m_instanceTable->setHorizontalHeaderLabels({
        "ID", "Name", "State", "ADB", "Port", "Profile"
    });
    m_instanceTable->horizontalHeader()->setStretchLastSection(true);
    m_instanceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_instanceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_instanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_instanceTable->setAlternatingRowColors(true);
    
    splitter->addWidget(m_instanceTable);
    
    // Details panel (right)
    m_detailsWidget = new QWidget(this);
    splitter->addWidget(m_detailsWidget);
    
    splitter->setSizes({400, 600});
    
    setCentralWidget(splitter);
}

void MainWindow::setupMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* newAction = new QAction("&New Instance", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::on_actionNew_Instance_triggered);
    fileMenu->addAction(newAction);
    
    fileMenu->addSeparator();
    
    QAction* autoStartAction = new QAction("&Auto-Start Settings...", this);
    autoStartAction->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(autoStartAction, &QAction::triggered, this, &MainWindow::on_actionAutoStart_triggered);
    fileMenu->addAction(autoStartAction);
    
    fileMenu->addSeparator();
    
    QAction* settingsAction = new QAction("&Settings", this);
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::on_actionSettings_triggered);
    fileMenu->addAction(settingsAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::on_actionExit_triggered);
    fileMenu->addAction(exitAction);
    
    // Instance menu
    QMenu* instanceMenu = menuBar()->addMenu("&Instance");
    
    QAction* startAction = new QAction("&Start", this);
    startAction->setShortcut(Qt::Key_F5);
    connect(startAction, &QAction::triggered, this, &MainWindow::on_actionStart_triggered);
    instanceMenu->addAction(startAction);
    
    QAction* stopAction = new QAction("&Stop", this);
    stopAction->setShortcut(Qt::Key_F6);
    connect(stopAction, &QAction::triggered, this, &MainWindow::on_actionStop_triggered);
    instanceMenu->addAction(stopAction);
    
    instanceMenu->addSeparator();
    
    QAction* saveAutoStartAction = new QAction("&Save for Auto-Start", this);
    connect(saveAutoStartAction, &QAction::triggered, this, &MainWindow::on_actionSaveForAutoStart_triggered);
    instanceMenu->addAction(saveAutoStartAction);
    
    QAction* removeAutoStartAction = new QAction("&Remove from Auto-Start", this);
    connect(removeAutoStartAction, &QAction::triggered, this, &MainWindow::on_actionRemoveFromAutoStart_triggered);
    instanceMenu->addAction(removeAutoStartAction);
    
    instanceMenu->addSeparator();
    
    QAction* refreshAction = new QAction("&Refresh", this);
    refreshAction->setShortcut(Qt::Key_F5);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::on_actionRefresh_triggered);
    instanceMenu->addAction(refreshAction);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    // New instance button
    QPushButton* newBtn = new QPushButton("➕ New", this);
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::on_newInstanceButton_clicked);
    toolbar->addWidget(newBtn);
    
    toolbar->addSeparator();
    
    // Start/Stop buttons
    m_startButton = new QPushButton("▶ Start", this);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::on_actionStart_triggered);
    toolbar->addWidget(m_startButton);
    
    m_stopButton = new QPushButton("⏹ Stop", this);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::on_actionStop_triggered);
    toolbar->addWidget(m_stopButton);
    
    toolbar->addSeparator();
    
    // Refresh button
    m_refreshButton = new QPushButton("🔄 Refresh", this);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::on_actionRefresh_triggered);
    toolbar->addWidget(m_refreshButton);
    
    // Spacer
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel("Profile: "));
    
    QComboBox* profileCombo = new QComboBox(this);
    profileCombo->setMinimumWidth(200);
    toolbar->addWidget(profileCombo);
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel, 1);
    
    m_memoryLabel = new QLabel("Memory: 0/0 MB", this);
    statusBar()->addPermanentWidget(m_memoryLabel);
    
    m_dockerStatusLabel = new QLabel("Docker: Checking...", this);
    statusBar()->addPermanentWidget(m_dockerStatusLabel);
    
    // Check Docker on startup
    QTimer::singleShot(500, this, [this]() {
        ReDroidController& controller = ReDroidController::instance();
        OperationResult result = controller.validateDocker();
        
        if (result.success) {
            m_dockerStatusLabel->setText(QString("Docker: %1").arg(result.data.value("version").toString()));
            m_dockerStatusLabel->setStyleSheet("color: green;");
        } else {
            m_dockerStatusLabel->setText("Docker: Not Available");
            m_dockerStatusLabel->setStyleSheet("color: red;");
        }
    });
}

void MainWindow::setupInstanceTable() {
    m_instanceTable->setColumnWidth(0, 100);  // ID
    m_instanceTable->setColumnWidth(1, 150);  // Name
    m_instanceTable->setColumnWidth(2, 80);   // State
    m_instanceTable->setColumnWidth(3, 60);   // ADB
    m_instanceTable->setColumnWidth(4, 60);   // Port
}

void MainWindow::setupDetailsPanel() {
    QVBoxLayout* layout = new QVBoxLayout(m_detailsWidget);
    
    // Instance info group
    QGroupBox* infoGroup = new QGroupBox("Instance Information", m_detailsWidget);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_instanceIdLabel = new QLabel("-", m_detailsWidget);
    m_stateLabel = new QLabel("-", m_detailsWidget);
    m_adbStatusLabel = new QLabel("-", m_detailsWidget);
    
    infoLayout->addRow("Instance ID:", m_instanceIdLabel);
    infoLayout->addRow("State:", m_stateLabel);
    infoLayout->addRow("ADB:", m_adbStatusLabel);
    
    // Device info group
    QGroupBox* deviceGroup = new QGroupBox("Device Information", m_detailsWidget);
    QFormLayout* deviceLayout = new QFormLayout(deviceGroup);
    
    m_brandLabel = new QLabel("-", m_detailsWidget);
    m_modelLabel = new QLabel("-", m_detailsWidget);
    m_imeiLabel = new QLabel("-", m_detailsWidget);
    m_serialLabel = new QLabel("-", m_detailsWidget);
    m_wifiMacLabel = new QLabel("-", m_detailsWidget);
    m_gpsLabel = new QLabel("-", m_detailsWidget);
    
    deviceLayout->addRow("Brand:", m_brandLabel);
    deviceLayout->addRow("Model:", m_modelLabel);
    deviceLayout->addRow("IMEI:", m_imeiLabel);
    deviceLayout->addRow("Serial:", m_serialLabel);
    deviceLayout->addRow("WiFi MAC:", m_wifiMacLabel);
    deviceLayout->addRow("GPS:", m_gpsLabel);
    
    // GPS spoofing group
    QGroupBox* gpsGroup = new QGroupBox("GPS Spoofing", m_detailsWidget);
    QFormLayout* gpsLayout = new QFormLayout(gpsGroup);
    
    m_latitudeEdit = new QLineEdit("37.7749", m_detailsWidget);
    m_longitudeEdit = new QLineEdit("-122.4194", m_detailsWidget);
    QPushButton* gpsUpdateBtn = new QPushButton("Update GPS", m_detailsWidget);
    connect(gpsUpdateBtn, &QPushButton::clicked, this, &MainWindow::on_gpsUpdateButton_clicked);
    
    gpsLayout->addRow("Latitude:", m_latitudeEdit);
    gpsLayout->addRow("Longitude:", m_longitudeEdit);
    gpsLayout->addRow("", gpsUpdateBtn);
    
    // Sensor data group
    QGroupBox* sensorGroup = new QGroupBox("Sensor Data", m_detailsWidget);
    QFormLayout* sensorLayout = new QFormLayout(sensorGroup);
    
    m_accelXEdit = new QLineEdit("0.0", m_detailsWidget);
    m_accelYEdit = new QLineEdit("9.81", m_detailsWidget);
    QPushButton* sensorBtn = new QPushButton("Send Sensors", m_detailsWidget);
    connect(sensorBtn, &QPushButton::clicked, this, &MainWindow::on_sendSensorDataButton_clicked);
    
    sensorLayout->addRow("Accel X:", m_accelXEdit);
    sensorLayout->addRow("Accel Y:", m_accelYEdit);
    sensorLayout->addRow("", sensorBtn);
    
    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton("▶ Start", m_detailsWidget);
    m_stopButton = new QPushButton("⏹ Stop", m_detailsWidget);
    m_restartButton = new QPushButton("🔄 Restart", m_detailsWidget);
    m_deleteButton = new QPushButton("🗑 Delete", m_detailsWidget);
    
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::on_actionStart_triggered);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::on_actionStop_triggered);
    connect(m_restartButton, &QPushButton::clicked, [this]() {
        if (!m_selectedInstanceId.isEmpty()) {
            restartInstance(m_selectedInstanceId);
        }
    });
    connect(m_deleteButton, &QPushButton::clicked, [this]() {
        if (!m_selectedInstanceId.isEmpty()) {
            deleteInstance(m_selectedInstanceId);
        }
    });
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_restartButton);
    buttonLayout->addWidget(m_deleteButton);
    
    // Assemble layout
    layout->addWidget(infoGroup);
    layout->addWidget(deviceGroup);
    layout->addWidget(gpsGroup);
    layout->addWidget(sensorGroup);
    layout->addLayout(buttonLayout);
    layout->addStretch();
}

void MainWindow::setupConnections() {
    ReDroidController& controller = ReDroidController::instance();
    
    connect(&controller, &ReDroidController::instanceStateChanged,
            this, &MainWindow::handleInstanceStateChanged);
    connect(&controller, &ReDroidController::adbConnectionChanged,
            this, &MainWindow::handleAdbConnectionChanged);
    connect(&controller, &ReDroidController::error,
            this, &MainWindow::handleError);
    
    connect(m_instanceTable, &QTableWidget::currentCellChanged,
            this, &MainWindow::on_instanceTable_currentCellChanged);
}

void MainWindow::refreshInstances() {
    ReDroidController& controller = ReDroidController::instance();
    m_instances.clear();
    
    QList<InstanceInfo> instances = controller.listInstances();
    for (const InstanceInfo& info : instances) {
        m_instances[info.instanceId] = info;
    }
    
    updateInstanceTable();
}

void MainWindow::updateInstanceTable() {
    m_instanceTable->setRowCount(m_instances.size());
    
    int row = 0;
    for (auto it = m_instances.constBegin(); it != m_instances.constEnd(); ++it) {
        const InstanceInfo& info = it.value();
        
        m_instanceTable->setItem(row, 0, new QTableWidgetItem(info.instanceId));
        m_instanceTable->setItem(row, 1, new QTableWidgetItem(info.containerName));
        
        // State with color
        QTableWidgetItem* stateItem = new QTableWidgetItem(stateToString(info.state));
        switch (info.state) {
            case InstanceState::Running:
                stateItem->setBackground(QBrush(Qt::darkGreen));
                break;
            case InstanceState::Paused:
                stateItem->setBackground(QBrush(Qt::darkYellow));
                break;
            case InstanceState::Error:
                stateItem->setBackground(QBrush(Qt::darkRed));
                break;
            default:
                break;
        }
        m_instanceTable->setItem(row, 2, stateItem);
        
        // ADB status
        QString adbStatus = info.adbConnected ? "✓" : "✗";
        m_instanceTable->setItem(row, 3, new QTableWidgetItem(adbStatus));
        
        // Port
        m_instanceTable->setItem(row, 4, new QTableWidgetItem(QString::number(info.adbPort)));
        
        // Profile
        m_instanceTable->setItem(row, 5, new QTableWidgetItem(info.profileId));
        
        row++;
    }
}

void MainWindow::updateInstanceDetails(const QString& instanceId) {
    if (!m_instances.contains(instanceId)) {
        return;
    }
    
    const InstanceInfo& info = m_instances[instanceId];
    
    m_instanceIdLabel->setText(instanceId);
    m_stateLabel->setText(stateToString(info.state));
    m_adbStatusLabel->setText(info.adbConnected ? "Connected" : "Disconnected");
    
    // Load profile for details
    QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    profilePath += "/profiles/" + info.profileId + ".json";
    DeviceProfile profile = DeviceProfile::load(profilePath);
    
    if (!profile.id.isEmpty()) {
        m_brandLabel->setText(profile.build.brand);
        m_modelLabel->setText(profile.build.model);
        m_imeiLabel->setText(profile.identity.imei);
        m_serialLabel->setText(profile.identity.serialNumber);
        m_wifiMacLabel->setText(profile.mac.wifiMac);
        m_gpsLabel->setText(QString("%1, %2").arg(profile.gps.latitude).arg(profile.gps.longitude));
    }
}

void MainWindow::updateToolbarState() {
    bool hasSelection = !m_selectedInstanceId.isEmpty();
    bool isRunning = false;
    
    if (hasSelection && m_instances.contains(m_selectedInstanceId)) {
        isRunning = m_instances[m_selectedInstanceId].state == InstanceState::Running;
    }
    
    m_startButton->setEnabled(hasSelection && !isRunning);
    m_stopButton->setEnabled(hasSelection && isRunning);
    m_restartButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

void MainWindow::updateStatusBar() {
    int runningCount = 0;
    int totalCount = m_instances.size();
    
    for (const InstanceInfo& info : m_instances.values()) {
        if (info.state == InstanceState::Running) {
            runningCount++;
        }
    }
    
    m_statusLabel->setText(QString("Instances: %1/%2 running").arg(runningCount).arg(totalCount));
}

void MainWindow::showNotification(const QString& message) {
    statusBar()->showMessage(message, 3000);
}

// ==============================================================================
// Slots - Menu Actions
// ==============================================================================

void MainWindow::on_actionNew_Instance_triggered() {
    on_newInstanceButton_clicked();
}

void MainWindow::on_actionStart_triggered() {
    if (!m_selectedInstanceId.isEmpty()) {
        DeviceProfile profile;
        if (m_instances.contains(m_selectedInstanceId)) {
            QString profileId = m_instances[m_selectedInstanceId].profileId;
            QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            profilePath += "/profiles/" + profileId + ".json";
            profile = DeviceProfile::load(profilePath);
        }
        
        if (profile.id.isEmpty()) {
            profile = DeviceProfile::createSamsungS24Ultra();
        }
        
        startInstance(m_selectedInstanceId, profile);
    }
}

void MainWindow::on_actionStop_triggered() {
    if (!m_selectedInstanceId.isEmpty()) {
        stopInstance(m_selectedInstanceId);
    }
}

void MainWindow::on_actionDelete_triggered() {
    if (!m_selectedInstanceId.isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Delete Instance",
            QString("Are you sure you want to delete instance '%1'?").arg(m_selectedInstanceId),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            deleteInstance(m_selectedInstanceId);
        }
    }
}

void MainWindow::on_actionRefresh_triggered() {
    refreshInstances();
}

void MainWindow::on_actionSettings_triggered() {
    showSettings();
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

// ==============================================================================
// Slots - Toolbar
// ==============================================================================

void MainWindow::on_newInstanceButton_clicked() {
    bool ok;
    QString instanceId = QInputDialog::getText(
        this, "New Instance",
        "Enter instance ID:",
        QLineEdit::Normal, "", &ok
    );
    
    if (ok && !instanceId.isEmpty()) {
        DeviceProfile profile = DeviceProfile::createSamsungS24Ultra();
        startInstance(instanceId, profile);
    }
}

void MainWindow::on_instanceTable_currentCellChanged(int row, int column) {
    if (row >= 0 && row < m_instanceTable->rowCount()) {
        QTableWidgetItem* item = m_instanceTable->item(row, 0);
        if (item) {
            m_selectedInstanceId = item->text();
            updateInstanceDetails(m_selectedInstanceId);
            updateToolbarState();
        }
    }
}

void MainWindow::on_instanceTable_itemDoubleClicked(QTableWidgetItem* item) {
    // Could open instance details dialog
}

// ==============================================================================
// Slots - Instance Management
// ==============================================================================

void MainWindow::startInstance(const QString& instanceId, const DeviceProfile& profile) {
    ReDroidController& controller = ReDroidController::instance();
    
    showNotification(QString("Starting instance %1...").arg(instanceId));
    
    if (controller.startInstance(instanceId, profile)) {
        showNotification(QString("Instance %1 started").arg(instanceId));
        refreshInstances();
    } else {
        QMessageBox::critical(this, "Error", 
            QString("Failed to start instance: %1").arg(instanceId));
    }
}

void MainWindow::stopInstance(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    showNotification(QString("Stopping instance %1...").arg(instanceId));
    
    if (controller.stopInstance(instanceId)) {
        showNotification(QString("Instance %1 stopped").arg(instanceId));
        refreshInstances();
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to stop instance: %1").arg(instanceId));
    }
}

void MainWindow::restartInstance(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    showNotification(QString("Restarting instance %1...").arg(instanceId));
    
    if (controller.restartInstance(instanceId)) {
        showNotification(QString("Instance %1 restarted").arg(instanceId));
        refreshInstances();
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to restart instance: %1").arg(instanceId));
    }
}

void MainWindow::deleteInstance(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    if (controller.deleteInstance(instanceId)) {
        showNotification(QString("Instance %1 deleted").arg(instanceId));
        m_selectedInstanceId.clear();
        refreshInstances();
        updateToolbarState();
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to delete instance: %1").arg(instanceId));
    }
}

// ==============================================================================
// Slots - Signal Handlers
// ==============================================================================

void MainWindow::handleInstanceStateChanged(const QString& instanceId, InstanceState state) {
    qDebug() << "Instance state changed:" << instanceId << "->" << stateToString(state);
    
    if (m_instances.contains(instanceId)) {
        m_instances[instanceId].state = state;
    }
    
    updateInstanceTable();
    updateToolbarState();
    updateStatusBar();
}

void MainWindow::handleAdbConnectionChanged(const QString& instanceId, bool connected) {
    qDebug() << "ADB connection changed:" << instanceId << "->" << connected;
    
    if (m_instances.contains(instanceId)) {
        m_instances[instanceId].adbConnected = connected;
    }
    
    updateInstanceTable();
    
    if (instanceId == m_selectedInstanceId) {
        m_adbStatusLabel->setText(connected ? "Connected" : "Disconnected");
    }
}

void MainWindow::handleError(const QString& message) {
    QMessageBox::critical(this, "Error", message);
}

// ==============================================================================
// Slots - GPS & Sensors
// ==============================================================================

void MainWindow::on_gpsUpdateButton_clicked() {
    if (m_selectedInstanceId.isEmpty()) {
        return;
    }
    
    bool latOk, lonOk;
    double lat = m_latitudeEdit->text().toDouble(&latOk);
    double lon = m_longitudeEdit->text().toDouble(&lonOk);
    
    if (!latOk || !lonOk) {
        QMessageBox::warning(this, "Invalid Input", "Please enter valid GPS coordinates");
        return;
    }
    
    ReDroidController& controller = ReDroidController::instance();
    
    if (controller.sendSensorData(m_selectedInstanceId, lat, lon)) {
        showNotification("GPS updated");
    } else {
        QMessageBox::warning(this, "Error", "Failed to update GPS");
    }
}

void MainWindow::on_sendSensorDataButton_clicked() {
    if (m_selectedInstanceId.isEmpty()) {
        return;
    }
    
    bool xOk, yOk;
    double x = m_accelXEdit->text().toDouble(&xOk);
    double y = m_accelYEdit->text().toDouble(&yOk);
    
    if (!xOk || !yOk) {
        QMessageBox::warning(this, "Invalid Input", "Please enter valid sensor values");
        return;
    }
    
    ReDroidController& controller = ReDroidController::instance();
    
    double lat = m_latitudeEdit->text().toDouble();
    double lon = m_longitudeEdit->text().toDouble();
    
    if (controller.sendSensorData(m_selectedInstanceId, lat, lon, x, y)) {
        showNotification("Sensor data sent");
    } else {
        QMessageBox::warning(this, "Error", "Failed to send sensor data");
    }
}

// ==============================================================================
// Dialogs
// ==============================================================================

void MainWindow::showSettings() {
    SettingsDialog dialog(this);
    dialog.setConfig(ReDroidController::instance().config());
    
    if (dialog.exec() == QDialog::Accepted) {
        ReDroidController::instance().setConfig(dialog.getConfig());
    }
}

void MainWindow::showAboutDialog() {
    QMessageBox::about(this, "About VirtualPhonePro",
        QString("<h2>VirtualPhonePro v1.0.0</h2>"
                "<p>Commercial-grade Anti-Detect Android Emulator Manager</p>"
                "<p>Built with Qt6 and C++20</p>"
                "<p>For authorized testing only.</p>"
                "<p>Copyright 2024 VirtualPhonePro</p>")
    );
}

// ==============================================================================
// Settings Dialog Implementation
// ==============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setModal(true);
    setupUI();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::setupUI() {
    QFormLayout* form = new QFormLayout(this);
    
    m_dockerPathEdit = new QLineEdit("docker", this);
    QPushButton* browseDocker = new QPushButton("Browse", this);
    connect(browseDocker, &QPushButton::clicked, this, &SettingsDialog::onBrowseDocker);
    QHBoxLayout* dockerLayout = new QHBoxLayout();
    dockerLayout->addWidget(m_dockerPathEdit);
    dockerLayout->addWidget(browseDocker);
    form->addRow("Docker Path:", dockerLayout);
    
    m_adbPathEdit = new QLineEdit(this);
    QPushButton* browseAdb = new QPushButton("Browse", this);
    connect(browseAdb, &QPushButton::clicked, this, &SettingsDialog::onBrowseAdb);
    QHBoxLayout* adbLayout = new QHBoxLayout();
    adbLayout->addWidget(m_adbPathEdit);
    adbLayout->addWidget(browseAdb);
    form->addRow("ADB Path:", adbLayout);
    
    m_imageNameEdit = new QLineEdit("ghcr.io/redroid/redroid:14.0.0_google_64only", this);
    form->addRow("ReDroid Image:", m_imageNameEdit);
    
    m_memoryLimitSpin = new QSpinBox(this);
    m_memoryLimitSpin->setRange(256, 4096);
    m_memoryLimitSpin->setValue(512);
    m_memoryLimitSpin->setSuffix(" MB");
    form->addRow("Memory Limit:", m_memoryLimitSpin);
    
    m_cpuQuotaSpin = new QSpinBox(this);
    m_cpuQuotaSpin->setRange(1, 8);
    m_cpuQuotaSpin->setValue(2);
    form->addRow("CPU Cores:", m_cpuQuotaSpin);
    
    m_baseAdbPortSpin = new QSpinBox(this);
    m_baseAdbPortSpin->setRange(1024, 65535);
    m_baseAdbPortSpin->setValue(5555);
    form->addRow("Base ADB Port:", m_baseAdbPortSpin);
    
    m_baseVncPortSpin = new QSpinBox(this);
    m_baseVncPortSpin->setRange(1024, 65535);
    m_baseVncPortSpin->setValue(5900);
    form->addRow("Base VNC Port:", m_baseVncPortSpin);
    
    m_autoConnectCheck = new QCheckBox("Auto-connect ADB on start", this);
    form->addRow("", m_autoConnectCheck);
    
    m_autoSpoofCheck = new QCheckBox("Auto-apply spoofing on start", this);
    form->addRow("", m_autoSpoofCheck);
    
    m_connectionStatusLabel = new QLabel(this);
    form->addRow("Connection:", m_connectionStatusLabel);
    
    QPushButton* testBtn = new QPushButton("Test Connection", this);
    connect(testBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    form->addRow("", testBtn);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onOk);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancel);
    form->addRow(buttons);
}

DockerConfig SettingsDialog::getConfig() const {
    DockerConfig config;
    config.dockerPath = m_dockerPathEdit->text();
    config.adbPath = m_adbPathEdit->text();
    config.imageName = m_imageNameEdit->text();
    config.memoryLimit = QString("%1m").arg(m_memoryLimitSpin->value());
    config.cpuQuota = m_cpuQuotaSpin->value() * 100000;
    config.baseAdbPort = m_baseAdbPortSpin->value();
    config.baseVncPort = m_baseVncPortSpin->value();
    return config;
}

void SettingsDialog::setConfig(const DockerConfig& config) {
    m_config = config;
    m_dockerPathEdit->setText(config.dockerPath);
    m_adbPathEdit->setText(config.adbPath);
    m_imageNameEdit->setText(config.imageName);
    m_memoryLimitSpin->setValue(config.memoryLimit.left(config.memoryLimit.size()-1).toInt());
    m_cpuQuotaSpin->setValue(config.cpuQuota / 100000);
    m_baseAdbPortSpin->setValue(config.baseAdbPort);
    m_baseVncPortSpin->setValue(config.baseVncPort);
}

void SettingsDialog::onBrowseDocker() {
    QString path = QFileDialog::getOpenFileName(this, "Select Docker Executable");
    if (!path.isEmpty()) {
        m_dockerPathEdit->setText(path);
    }
}

void SettingsDialog::onBrowseAdb() {
    QString path = QFileDialog::getOpenFileName(this, "Select ADB Executable");
    if (!path.isEmpty()) {
        m_adbPathEdit->setText(path);
    }
}

void SettingsDialog::onTestConnection() {
    m_connectionStatusLabel->setText("Testing...");
    m_connectionStatusLabel->setStyleSheet("");
    
    ReDroidController controller;
    DockerConfig tempConfig = getConfig();
    controller.setConfig(tempConfig);
    
    OperationResult result = controller.validateDocker();
    
    if (result.success) {
        m_connectionStatusLabel->setText(QString("Connected (v%1)").arg(result.data.value("version").toString()));
        m_connectionStatusLabel->setStyleSheet("color: green;");
    } else {
        m_connectionStatusLabel->setText("Failed: " + result.errorMessage);
        m_connectionStatusLabel->setStyleSheet("color: red;");
    }
}

void SettingsDialog::onOk() {
    m_config = getConfig();
    accept();
}

void SettingsDialog::onCancel() {
    reject();
}

// ==============================================================================
// Auto-Start Implementation
// ==============================================================================

void MainWindow::showAutoStartSettings() {
    QMessageBox::information(this, "Auto-Start Settings", 
        "Auto-Start Settings:\n\n"
        "• Enable auto-start in File menu\n"
        "• Save instances for auto-start via Instance menu\n"
        "• Containers will restore on next app launch");
}

void MainWindow::autoStartInstances() {
    qDebug() << "[AutoStart] Starting saved instances...";
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString instancesFile = configDir + "/saved_instances.json";
    
    QFile file(instancesFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[AutoStart] No saved instances found";
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "[AutoStart] JSON parse error:" << error.errorString();
        return;
    }
    
    QJsonObject json = doc.object();
    QJsonArray instances = json["instances"].toArray();
    
    ReDroidController& controller = ReDroidController::instance();
    
    for (const QJsonValue& value : instances) {
        QJsonObject instance = value.toObject();
        QString instanceId = instance["instanceId"].toString();
        QString profileData = instance["profileData"].toString();
        
        if (instanceId.isEmpty()) continue;
        
        qDebug() << "[AutoStart] Restoring instance:" << instanceId;
        
        QJsonDocument profileDoc = QJsonDocument::fromJson(profileData.toUtf8());
        DeviceProfile profile;
        profile.fromJson(profileDoc.object());
        
        if (controller.startInstance(instanceId, profile)) {
            qDebug() << "[AutoStart] Instance started:" << instanceId;
            showNotification(QString("Auto-started: %1").arg(instanceId));
        }
    }
}

void MainWindow::saveInstanceForAutoStart(const QString& instanceId, const DeviceProfile& profile) {
    qDebug() << "[AutoStart] Saving instance:" << instanceId;
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString instancesFile = configDir + "/saved_instances.json";
    
    QJsonObject json;
    QJsonArray instances;
    
    QFile file(instancesFile);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError) {
            json = doc.object();
            instances = json["instances"].toArray();
        }
    }
    
    QJsonObject newInstance;
    newInstance["instanceId"] = instanceId;
    newInstance["profileData"] = QString(QJsonDocument(profile.toJson()).toJson());
    newInstance["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonArray newInstances;
    for (const QJsonValue& value : instances) {
        QJsonObject obj = value.toObject();
        if (obj["instanceId"].toString() != instanceId) {
            newInstances.append(obj);
        }
    }
    newInstances.append(newInstance);
    
    json["instances"] = newInstances;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
        showNotification(QString("Saved for auto-start: %1").arg(instanceId));
    }
}

void MainWindow::removeInstanceFromAutoStart(const QString& instanceId) {
    qDebug() << "[AutoStart] Removing instance:" << instanceId;
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString instancesFile = configDir + "/saved_instances.json";
    
    QFile file(instancesFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) return;
    
    QJsonObject json = doc.object();
    QJsonArray instances = json["instances"].toArray();
    
    QJsonArray newInstances;
    for (const QJsonValue& value : instances) {
        QJsonObject obj = value.toObject();
        if (obj["instanceId"].toString() != instanceId) {
            newInstances.append(obj);
        }
    }
    
    json["instances"] = newInstances;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
        showNotification(QString("Removed from auto-start: %1").arg(instanceId));
    }
}

void MainWindow::on_actionAutoStart_triggered() {
    showAutoStartSettings();
}

void MainWindow::on_actionSaveForAutoStart_triggered() {
    if (m_selectedInstanceId.isEmpty()) {
        QMessageBox::warning(this, "No Instance Selected", "Please select an instance first.");
        return;
    }
    
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_selectedInstanceId);
    
    DeviceProfile profile;
    profile.id = info.profileId;
    profile.name = info.instanceId;
    
    saveInstanceForAutoStart(m_selectedInstanceId, profile);
}

void MainWindow::on_actionRemoveFromAutoStart_triggered() {
    if (m_selectedInstanceId.isEmpty()) {
        QMessageBox::warning(this, "No Instance Selected", "Please select an instance first.");
        return;
    }
    
    removeInstanceFromAutoStart(m_selectedInstanceId);
}

// ==============================================================================
// ProfileEditorDialog Implementation
// ==============================================================================

ProfileEditorDialog::ProfileEditorDialog(QWidget* parent)
    : QDialog(parent)
    , m_readOnly(false)
{
    setWindowTitle("Device Profile Editor");
    setModal(true);
    setMinimumSize(600, 700);
    setupUI();
}

ProfileEditorDialog::~ProfileEditorDialog() {
}

void ProfileEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Name field
    QGroupBox* basicGroup = new QGroupBox("Basic Information", this);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);
    
    m_nameEdit = new QLineEdit(this);
    basicLayout->addRow("Profile Name:", m_nameEdit);
    
    m_manufacturerCombo = new QComboBox(this);
    m_manufacturerCombo->addItems({"Samsung", "Google", "Xiaomi", "OnePlus", "Huawei", "OPPO", "Vivo", "Custom"});
    connect(m_manufacturerCombo, &QComboBox::currentTextChanged, this, &ProfileEditorDialog::onManufacturerChanged);
    basicLayout->addRow("Manufacturer:", m_manufacturerCombo);
    
    m_modelCombo = new QComboBox(this);
    basicLayout->addRow("Model:", m_modelCombo);
    
    m_brandEdit = new QLineEdit(this);
    basicLayout->addRow("Brand:", m_brandEdit);
    
    mainLayout->addWidget(basicGroup);
    
    // Identity fields
    QGroupBox* identityGroup = new QGroupBox("Device Identity", this);
    QFormLayout* identityLayout = new QFormLayout(identityGroup);
    
    m_imeiEdit = new QLineEdit(this);
    identityLayout->addRow("IMEI:", m_imeiEdit);
    
    m_imei2Edit = new QLineEdit(this);
    identityLayout->addRow("IMEI 2:", m_imei2Edit);
    
    m_serialEdit = new QLineEdit(this);
    identityLayout->addRow("Serial Number:", m_serialEdit);
    
    m_androidIdEdit = new QLineEdit(this);
    identityLayout->addRow("Android ID:", m_androidIdEdit);
    
    m_gsfIdEdit = new QLineEdit(this);
    identityLayout->addRow("GSF ID:", m_gsfIdEdit);
    
    mainLayout->addWidget(identityGroup);
    
    // Network fields
    QGroupBox* networkGroup = new QGroupBox("Network", this);
    QFormLayout* networkLayout = new QFormLayout(networkGroup);
    
    m_wifiMacEdit = new QLineEdit(this);
    networkLayout->addRow("WiFi MAC:", m_wifiMacEdit);
    
    m_btMacEdit = new QLineEdit(this);
    networkLayout->addRow("Bluetooth MAC:", m_btMacEdit);
    
    m_hostnameEdit = new QLineEdit(this);
    networkLayout->addRow("Hostname:", m_hostnameEdit);
    
    mainLayout->addWidget(networkGroup);
    
    // SIM fields
    QGroupBox* simGroup = new QGroupBox("SIM Card", this);
    QFormLayout* simLayout = new QFormLayout(simGroup);
    
    m_iccidEdit = new QLineEdit(this);
    simLayout->addRow("ICCID:", m_iccidEdit);
    
    m_imsiEdit = new QLineEdit(this);
    simLayout->addRow("IMSI:", m_imsiEdit);
    
    m_carrierCombo = new QComboBox(this);
    m_carrierCombo->addItems({"Auto Detect", "AT&T", "Verizon", "T-Mobile", "Orange", "Vodafone", "Custom"});
    simLayout->addRow("Carrier:", m_carrierCombo);
    
    mainLayout->addWidget(simGroup);
    
    // GPS fields
    QGroupBox* gpsGroup = new QGroupBox("GPS Location", this);
    QFormLayout* gpsLayout = new QFormLayout(gpsGroup);
    
    m_latitudeEdit = new QLineEdit(this);
    gpsLayout->addRow("Latitude:", m_latitudeEdit);
    
    m_longitudeEdit = new QLineEdit(this);
    gpsLayout->addRow("Longitude:", m_longitudeEdit);
    
    mainLayout->addWidget(gpsGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_randomizeButton = new QPushButton("Randomize", this);
    connect(m_randomizeButton, &QPushButton::clicked, this, &ProfileEditorDialog::onRandomize);
    buttonLayout->addWidget(m_randomizeButton);
    
    buttonLayout->addStretch();
    
    QPushButton* okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &ProfileEditorDialog::onOk);
    buttonLayout->addWidget(okButton);
    
    QPushButton* cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &ProfileEditorDialog::onCancel);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Load manufacturer models
    loadManufacturerModels();
}

void ProfileEditorDialog::loadManufacturerModels() {
    m_modelCombo->clear();
    
    QString manufacturer = m_manufacturerCombo->currentText();
    
    if (manufacturer == "Samsung") {
        m_modelCombo->addItems({"SM-S928B (S24 Ultra)", "SM-S921B (S24)", "SM-S918B (S23 Ultra)", 
                               "SM-A546B (A54)", "SM-A536B (A53)", "Custom"});
    } else if (manufacturer == "Google") {
        m_modelCombo->addItems({"Pixel 8 Pro", "Pixel 8", "Pixel 7 Pro", "Pixel 7", 
                               "Pixel 6 Pro", "Pixel 6a", "Custom"});
    } else if (manufacturer == "Xiaomi") {
        m_modelCombo->addItems({"Mi 14 Pro", "Mi 14", "Redmi Note 13 Pro", 
                               "Redmi Note 12", "POCO F5", "Custom"});
    } else if (manufacturer == "OnePlus") {
        m_modelCombo->addItems({"OnePlus 12", "OnePlus 11", "OnePlus 10T", 
                               "OnePlus Nord 3", "Custom"});
    } else if (manufacturer == "Huawei") {
        m_modelCombo->addItems({"P60 Pro", "Mate 60 Pro", "P50 Pro", 
                               "Nova 11", "Custom"});
    } else if (manufacturer == "OPPO") {
        m_modelCombo->addItems({"Find X7 Pro", "Find X6 Pro", "Reno 10 Pro", 
                               "A78", "Custom"});
    } else if (manufacturer == "Vivo") {
        m_modelCombo->addItems({"X100 Pro", "X90 Pro", "V30 Pro", 
                               "V27", "Custom"});
    } else {
        m_modelCombo->addItems({"Custom"});
    }
}

void ProfileEditorDialog::populateFromProfile() {
    if (m_profile.id.isEmpty()) {
        return;
    }
    
    m_nameEdit->setText(m_profile.name);
    
    int idx = m_manufacturerCombo->findText(m_profile.manufacturer);
    if (idx >= 0) {
        m_manufacturerCombo->setCurrentIndex(idx);
    }
    
    m_modelCombo->setCurrentText(m_profile.model);
    m_brandEdit->setText(m_profile.brand);
    m_imeiEdit->setText(m_profile.imei);
    m_imei2Edit->setText(m_profile.imei2);
    m_serialEdit->setText(m_profile.serialNumber);
    m_androidIdEdit->setText(m_profile.androidId);
    m_gsfIdEdit->setText(m_profile.gsfId);
    m_wifiMacEdit->setText(m_profile.wifiMac);
    m_btMacEdit->setText(m_profile.bluetoothMac);
    m_hostnameEdit->setText(m_profile.hostname);
    m_iccidEdit->setText(m_profile.iccid);
    m_imsiEdit->setText(m_profile.imsi);
    m_latitudeEdit->setText(QString::number(m_profile.latitude));
    m_longitudeEdit->setText(QString::number(m_profile.longitude));
}

void ProfileEditorDialog::onRandomize() {
    QString manufacturer = m_manufacturerCombo->currentText();
    
    // Generate new values
    QStringList ouiList;
    if (manufacturer == "Samsung") {
        ouiList = {"8C:71:F8", "D0:22:BE", "54:88:0E"};
    } else if (manufacturer == "Google") {
        ouiList = {"3C:5A:B4", "54:60:09"};
    } else if (manufacturer == "Xiaomi") {
        ouiList = {"34:80:B3", "F4:F5:D8"};
    } else {
        ouiList = {"00:1A:11"};
    }
    
    QString oui = ouiList.at(qrand() % ouiList.size());
    
    // Generate random MAC
    QString hex = "0123456789ABCDEF";
    QString mac;
    for (int i = 0; i < 6; i++) {
        if (i > 0) mac += ":";
        mac += hex[qrand() % 16];
        mac += hex[qrand() % 16];
    }
    
    m_wifiMacEdit->setText(oui + mac.mid(8));
    m_btMacEdit->setText("00:1A:7D" + mac.mid(8));
    
    // Generate random IMEI (15 digits with Luhn check)
    QString tac;
    if (manufacturer == "Samsung") {
        QString tacs[] = {"35875107", "35875108", "35746608"};
        tac = tacs[qrand() % 3];
    } else if (manufacturer == "Google") {
        QString tacs[] = {"35746608", "35746610"};
        tac = tacs[qrand() % 2];
    } else {
        tac = "35875107";
    }
    
    for (int i = 0; i < 6; i++) {
        tac += QString::number(qrand() % 10);
    }
    
    // Calculate Luhn check digit
    int sum = 0;
    bool alternate = true;
    for (int i = tac.length() - 1; i >= 0; i--) {
        int n = tac[i].digitValue();
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    int checkDigit = (10 - (sum % 10)) % 10;
    
    m_imeiEdit->setText(tac + QString::number(checkDigit));
    
    // Generate random serial
    QString serial;
    if (manufacturer == "Samsung") {
        serial = "R" + QString::number(100000 + qrand() % 900000) + "X" + 
                QString::number(10 + qrand() % 90);
    } else if (manufacturer == "Google") {
        serial = "AG" + QString::number(10000000 + qrand() % 90000000);
    } else {
        serial = QString::number(qrand() % 1000000000000LL);
    }
    m_serialEdit->setText(serial);
    
    // Generate random Android ID (16 hex chars)
    QString androidId;
    for (int i = 0; i < 16; i++) {
        androidId += hex[qrand() % 16];
    }
    m_androidIdEdit->setText(androidId);
    
    // Generate random ICCID (20 digits)
    QString iccid = "89688";
    for (int i = 0; i < 14; i++) {
        iccid += QString::number(qrand() % 10);
    }
    // Luhn check for ICCID
    sum = 0;
    for (int i = iccid.length() - 1; i >= 0; i--) {
        int n = iccid[i].digitValue();
        if ((iccid.length() - i) % 2 == 0) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
    }
    checkDigit = (10 - (sum % 10)) % 10;
    m_iccidEdit->setText(iccid + QString::number(checkDigit));
    
    // Generate random IMSI (15 digits)
    QString imsi = "8801";
    for (int i = 0; i < 10; i++) {
        imsi += QString::number(qrand() % 10);
    }
    m_imsiEdit->setText(imsi);
    
    QMessageBox::information(this, "Randomized", "New device identity has been generated.");
}

void ProfileEditorDialog::onManufacturerChanged() {
    loadManufacturerModels();
}

void ProfileEditorDialog::onOk() {
    // Validate required fields
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a profile name.");
        return;
    }
    
    if (m_imeiEdit->text().length() != 15) {
        QMessageBox::warning(this, "Validation Error", "IMEI must be 15 digits.");
        return;
    }
    
    // Update profile
    m_profile.name = m_nameEdit->text();
    m_profile.manufacturer = m_manufacturerCombo->currentText();
    m_profile.model = m_modelCombo->currentText();
    m_profile.brand = m_brandEdit->text();
    m_profile.imei = m_imeiEdit->text();
    m_profile.imei2 = m_imei2Edit->text();
    m_profile.serialNumber = m_serialEdit->text();
    m_profile.androidId = m_androidIdEdit->text();
    m_profile.gsfId = m_gsfIdEdit->text();
    m_profile.wifiMac = m_wifiMacEdit->text();
    m_profile.bluetoothMac = m_btMacEdit->text();
    m_profile.hostname = m_hostnameEdit->text();
    m_profile.iccid = m_iccidEdit->text();
    m_profile.imsi = m_imsiEdit->text();
    m_profile.latitude = m_latitudeEdit->text().toDouble();
    m_profile.longitude = m_longitudeEdit->text().toDouble();
    
    accept();
}

void ProfileEditorDialog::onCancel() {
    reject();
}

DeviceProfile ProfileEditorDialog::getProfile() const {
    return m_profile;
}

void ProfileEditorDialog::setProfile(const DeviceProfile& profile) {
    m_profile = profile;
    populateFromProfile();
}

void ProfileEditorDialog::setReadOnly(bool readOnly) {
    m_readOnly = readOnly;
    
    m_nameEdit->setReadOnly(readOnly);
    m_manufacturerCombo->setEnabled(!readOnly);
    m_modelCombo->setEnabled(!readOnly);
    m_brandEdit->setReadOnly(readOnly);
    m_imeiEdit->setReadOnly(readOnly);
    m_imei2Edit->setReadOnly(readOnly);
    m_serialEdit->setReadOnly(readOnly);
    m_androidIdEdit->setReadOnly(readOnly);
    m_gsfIdEdit->setReadOnly(readOnly);
    m_wifiMacEdit->setReadOnly(readOnly);
    m_btMacEdit->setReadOnly(readOnly);
    m_hostnameEdit->setReadOnly(readOnly);
    m_iccidEdit->setReadOnly(readOnly);
    m_imsiEdit->setReadOnly(readOnly);
    m_latitudeEdit->setReadOnly(readOnly);
    m_longitudeEdit->setReadOnly(readOnly);
    m_randomizeButton->setEnabled(!readOnly);
}

// ==============================================================================
// SettingsDialog Implementation
// ==============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setModal(true);
    setMinimumSize(500, 400);
    setupUI();
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Docker settings
    QGroupBox* dockerGroup = new QGroupBox("Docker Settings", this);
    QFormLayout* dockerLayout = new QFormLayout(dockerGroup);
    
    QPushButton* browseDocker = new QPushButton("Browse...", this);
    connect(browseDocker, &QPushButton::clicked, this, &SettingsDialog::onBrowseDocker);
    
    m_dockerPathEdit = new QLineEdit(this);
    QHBoxLayout* dockerPathLayout = new QHBoxLayout();
    dockerPathLayout->addWidget(m_dockerPathEdit);
    dockerPathLayout->addWidget(browseDocker);
    dockerLayout->addRow("Docker Path:", dockerPathLayout);
    
    QPushButton* browseAdb = new QPushButton("Browse...", this);
    connect(browseAdb, &QPushButton::clicked, this, &SettingsDialog::onBrowseAdb);
    
    m_adbPathEdit = new QLineEdit(this);
    QHBoxLayout* adbPathLayout = new QHBoxLayout();
    adbPathLayout->addWidget(m_adbPathEdit);
    adbPathLayout->addWidget(browseAdb);
    dockerLayout->addRow("ADB Path:", adbPathLayout);
    
    m_imageNameEdit = new QLineEdit(this);
    m_imageNameEdit->setText("ghcr.io/redroid/redroid:14.0.0_google_64only");
    dockerLayout->addRow("ReDroid Image:", m_imageNameEdit);
    
    mainLayout->addWidget(dockerGroup);
    
    // Resource settings
    QGroupBox* resourceGroup = new QGroupBox("Resource Limits", this);
    QFormLayout* resourceLayout = new QFormLayout(resourceGroup);
    
    m_memoryLimitSpin = new QSpinBox(this);
    m_memoryLimitSpin->setRange(256, 8192);
    m_memoryLimitSpin->setSuffix(" MB");
    m_memoryLimitSpin->setValue(512);
    resourceLayout->addRow("Memory Limit:", m_memoryLimitSpin);
    
    m_cpuQuotaSpin = new QSpinBox(this);
    m_cpuQuotaSpin->setRange(1, 16);
    m_cpuQuotaSpin->setSuffix(" cores");
    m_cpuQuotaSpin->setValue(2);
    resourceLayout->addRow("CPU Quota:", m_cpuQuotaSpin);
    
    mainLayout->addWidget(resourceGroup);
    
    // Port settings
    QGroupBox* portGroup = new QGroupBox("Port Configuration", this);
    QFormLayout* portLayout = new QFormLayout(portGroup);
    
    m_baseAdbPortSpin = new QSpinBox(this);
    m_baseAdbPortSpin->setRange(5000, 6000);
    m_baseAdbPortSpin->setValue(5555);
    portLayout->addRow("Base ADB Port:", m_baseAdbPortSpin);
    
    m_baseVncPortSpin = new QSpinBox(this);
    m_baseVncPortSpin->setRange(5000, 6000);
    m_baseVncPortSpin->setValue(5900);
    portLayout->addRow("Base VNC Port:", m_baseVncPortSpin);
    
    mainLayout->addWidget(portGroup);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_autoConnectCheck = new QCheckBox("Auto-connect ADB on startup", this);
    m_autoConnectCheck->setChecked(true);
    optionsLayout->addWidget(m_autoConnectCheck);
    
    m_autoSpoofCheck = new QCheckBox("Auto-apply SafetyNet spoofing", this);
    m_autoSpoofCheck->setChecked(true);
    optionsLayout->addWidget(m_autoSpoofCheck);
    
    mainLayout->addWidget(optionsGroup);
    
    // Connection test
    QHBoxLayout* testLayout = new QHBoxLayout();
    m_connectionStatusLabel = new QLabel("Click 'Test Connection' to verify Docker", this);
    testLayout->addWidget(m_connectionStatusLabel);
    
    QPushButton* testBtn = new QPushButton("Test Connection", this);
    connect(testBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    testLayout->addWidget(testBtn);
    
    mainLayout->addLayout(testLayout);
    
    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onOk);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancel);
    mainLayout->addWidget(buttons);
}

DockerConfig SettingsDialog::getConfig() const {
    return m_config;
}

void SettingsDialog::setConfig(const DockerConfig& config) {
    m_config = config;
    m_dockerPathEdit->setText(config.dockerPath);
    m_adbPathEdit->setText(config.adbPath);
    m_imageNameEdit->setText(config.imageName);
    m_memoryLimitSpin->setValue(config.memoryLimit.replace("m", "").toInt());
    m_cpuQuotaSpin->setValue(config.cpuQuota / 100000);
    m_baseAdbPortSpin->setValue(config.baseAdbPort);
    m_baseVncPortSpin->setValue(config.baseVncPort);
}

void SettingsDialog::onBrowseDocker() {
    QString path = QFileDialog::getOpenFileName(this, "Select Docker Executable",
                                               "", "Executable (*.exe);;All Files (*)");
    if (!path.isEmpty()) {
        m_dockerPathEdit->setText(path);
    }
}

void SettingsDialog::onBrowseAdb() {
    QString path = QFileDialog::getOpenFileName(this, "Select ADB Executable",
                                               "", "Executable (*.exe);;All Files (*)");
    if (!path.isEmpty()) {
        m_adbPathEdit->setText(path);
    }
}

void SettingsDialog::onTestConnection() {
    m_connectionStatusLabel->setText("Testing connection...");
    
    // Simulate connection test
    QTimer::singleShot(500, this, [this]() {
        m_connectionStatusLabel->setText("<span style='color: green;'>✓ Docker connection successful</span>");
    });
}

void SettingsDialog::onOk() {
    // Save config
    m_config.dockerPath = m_dockerPathEdit->text();
    m_config.adbPath = m_adbPathEdit->text();
    m_config.imageName = m_imageNameEdit->text();
    m_config.memoryLimit = QString::number(m_memoryLimitSpin->value()) + "m";
    m_config.cpuQuota = m_cpuQuotaSpin->value() * 100000;
    m_config.baseAdbPort = m_baseAdbPortSpin->value();
    m_config.baseVncPort = m_baseVncPortSpin->value();
    
    accept();
}

void SettingsDialog::onCancel() {
    reject();
}

} // namespace VirtualPhonePro
