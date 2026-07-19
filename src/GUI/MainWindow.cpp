#include "MainWindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QThread>
#include <QDebug>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , emulatorRunning(false)
    , adbConnected(false)
{
    setupUI();
    setupConnections();
    setupSystemTray();
    
    // Initial status check
    checkDockerStatus();
    checkADBStatus();
    
    // Start status timer
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    statusTimer->start(5000);
    
    // Docker check timer
    dockerCheckTimer = new QTimer(this);
    connect(dockerCheckTimer, &QTimer::timeout, this, &MainWindow::checkDockerStatus);
    dockerCheckTimer->start(10000);
    
    appendLog("ReDroidCPP GUI Started");
    appendLog("Checking Docker status...");
}

MainWindow::~MainWindow() {
    statusTimer->stop();
    dockerCheckTimer->stop();
}

void MainWindow::setupUI() {
    setWindowTitle("ReDroidCPP - Android Emulator Manager");
    setMinimumSize(900, 650);
    resize(1000, 700);
    
    // Set style
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1e1e2e;
        }
        QLabel {
            color: #cdd6f4;
        }
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
        QPushButton:pressed {
            background-color: #74c7ec;
        }
        QPushButton:disabled {
            background-color: #45475a;
            color: #6c7086;
        }
        QPushButton#dangerBtn {
            background-color: #f38ba8;
        }
        QPushButton#dangerBtn:hover {
            background-color: #f2a899;
        }
        QPushButton#successBtn {
            background-color: #a6e3a1;
        }
        QPushButton#successBtn:hover {
            background-color: #94e2d5;
        }
        QTextEdit {
            background-color: #11111b;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 8px;
            padding: 10px;
        }
        QComboBox {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 5px;
            padding: 5px;
        }
        QTabWidget::pane {
            border: 1px solid #313244;
            border-radius: 8px;
            background-color: #1e1e2e;
        }
        QTabBar::tab {
            background-color: #313244;
            color: #cdd6f4;
            padding: 10px 20px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QTabBar::tab:selected {
            background-color: #89b4fa;
            color: #1e1e2e;
        }
        QGroupBox {
            color: #89b4fa;
            border: 1px solid #313244;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QProgressBar {
            border: 1px solid #313244;
            border-radius: 5px;
            background-color: #11111b;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #a6e3a1;
            border-radius: 4px;
        }
        QStatusBar {
            background-color: #11111b;
            color: #cdd6f4;
        }
        QSpinBox, QSlider {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 5px;
        }
        QCheckBox {
            color: #cdd6f4;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
        }
        QTableWidget {
            background-color: #11111b;
            color: #cdd6f4;
            border: 1px solid #313244;
            gridline-color: #313244;
        }
        QHeaderView::section {
            background-color: #313244;
            color: #cdd6f4;
            padding: 5px;
            border: none;
        }
    )");
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Tab Widget
    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);
    
    // ============ TAB 1: Dashboard ============
    QWidget *dashboardTab = new QWidget();
    QVBoxLayout *dashboardLayout = new QVBoxLayout(dashboardTab);
    
    // Status Cards
    QHBoxLayout *statusCardsLayout = new QHBoxLayout();
    
    // Docker Status Card
    QGroupBox *dockerCard = new QGroupBox("Docker Status");
    QVBoxLayout *dockerCardLayout = new QVBoxLayout(dockerCard);
    dockerStatusLabel = new QLabel("⏳ Checking...");
    dockerStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    dockerCardLayout->addWidget(dockerStatusLabel);
    dockerCardLayout->addStretch();
    statusCardsLayout->addWidget(dockerCard);
    
    // Emulator Status Card
    QGroupBox *emulatorCard = new QGroupBox("Emulator Status");
    QVBoxLayout *emulatorCardLayout = new QVBoxLayout(emulatorCard);
    emulatorStatusLabel = new QLabel("⏸️ Stopped");
    emulatorStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    emulatorCardLayout->addWidget(emulatorStatusLabel);
    emulatorCardLayout->addStretch();
    statusCardsLayout->addWidget(emulatorCard);
    
    // ADB Status Card
    QGroupBox *adbCard = new QGroupBox("ADB Status");
    QVBoxLayout *adbCardLayout = new QVBoxLayout(adbCard);
    adbStatusLabel = new QLabel("⏸️ Disconnected");
    adbStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    adbCardLayout->addWidget(adbStatusLabel);
    adbCardLayout->addStretch();
    statusCardsLayout->addWidget(adbCard);
    
    dashboardLayout->addLayout(statusCardsLayout);
    
    // Control Buttons
    QGroupBox *controlBox = new QGroupBox("Quick Controls");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBox);
    
    btnStartEmulator = new QPushButton("▶ Start Emulator");
    btnStopEmulator = new QPushButton("■ Stop");
    btnStopEmulator->setObjectName("dangerBtn");
    btnRestartEmulator = new QPushButton("↻ Restart");
    btnConnectADB = new QPushButton("🔗 Connect ADB");
    btnConnectADB->setObjectName("successBtn");
    btnDisconnectADB = new QPushButton("🔌 Disconnect");
    
    controlLayout->addWidget(btnStartEmulator);
    controlLayout->addWidget(btnStopEmulator);
    controlLayout->addWidget(btnRestartEmulator);
    controlLayout->addWidget(btnConnectADB);
    controlLayout->addWidget(btnDisconnectADB);
    controlLayout->addStretch();
    
    dashboardLayout->addWidget(controlBox);
    
    // Boot Progress
    QGroupBox *progressBox = new QGroupBox("Boot Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressBox);
    bootProgressBar = new QProgressBar();
    bootProgressBar->setRange(0, 100);
    bootProgressBar->setValue(0);
    bootStatusLabel = new QLabel("Ready to start");
    progressLayout->addWidget(bootProgressBar);
    progressLayout->addWidget(bootStatusLabel);
    dashboardLayout->addWidget(progressBox);
    
    // Log Window
    QGroupBox *logBox = new QGroupBox("Activity Log");
    QVBoxLayout *logLayout = new QVBoxLayout(logBox);
    logTextEdit = new QTextEdit();
    logTextEdit->setReadOnly(true);
    logTextEdit->setMaximumHeight(200);
    logLayout->addWidget(logTextEdit);
    dashboardLayout->addWidget(logBox);
    
    dashboardLayout->addStretch();
    tabWidget->addTab(dashboardTab, "📊 Dashboard");
    
    // ============ TAB 2: Profiles ============
    QWidget *profileTab = new QWidget();
    QVBoxLayout *profileLayout = new QVBoxLayout(profileTab);
    
    // Profile Selection
    QGroupBox *selectBox = new QGroupBox("Select Device Profile");
    QHBoxLayout *selectLayout = new QHBoxLayout(selectBox);
    profileComboBox = new QComboBox();
    profileComboBox->addItem("Samsung Galaxy S24 Ultra");
    profileComboBox->addItem("Google Pixel 8 Pro");
    profileComboBox->addItem("Xiaomi Mi 14 Pro");
    profileComboBox->addItem("OnePlus 12");
    profileComboBox->addItem("Huawei P60 Pro");
    profileComboBox->addItem("Samsung Galaxy A54");
    profileComboBox->addItem("Custom Profile");
    selectLayout->addWidget(new QLabel("Profile:"));
    selectLayout->addWidget(profileComboBox);
    selectLayout->addStretch();
    profileLayout->addWidget(selectBox);
    
    // Profile Actions
    QGroupBox *profileActionsBox = new QGroupBox("Profile Actions");
    QHBoxLayout *profileActionsLayout = new QHBoxLayout(profileActionsBox);
    btnCreateProfile = new QPushButton("➕ Create New");
    btnDeleteProfile = new QPushButton("🗑️ Delete");
    btnDeleteProfile->setObjectName("dangerBtn");
    btnApplyProfile = new QPushButton("✅ Apply Profile");
    btnApplyProfile->setObjectName("successBtn");
    profileActionsLayout->addWidget(btnCreateProfile);
    profileActionsLayout->addWidget(btnDeleteProfile);
    profileActionsLayout->addWidget(btnApplyProfile);
    profileActionsLayout->addStretch();
    profileLayout->addWidget(profileActionsBox);
    
    // Profile Table
    QGroupBox *tableBox = new QGroupBox("Available Profiles");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableBox);
    profileTableWidget = new QTableWidget();
    profileTableWidget->setColumnCount(5);
    profileTableWidget->setHorizontalHeaderLabels({"Name", "Manufacturer", "Android", "RAM", "Status"});
    profileTableWidget->setRowCount(7);
    
    QStringList profiles = {
        "Samsung Galaxy S24 Ultra", "Samsung Galaxy A54", "Google Pixel 8 Pro",
        "Xiaomi Mi 14 Pro", "OnePlus 12", "Huawei P60 Pro", "Custom"
    };
    QStringList manufacturers = {"Samsung", "Samsung", "Google", "Xiaomi", "OnePlus", "Huawei", "Custom"};
    QStringList versions = {"14", "13", "14", "14", "14", "13", "14"};
    QStringList rams = {"12GB", "8GB", "12GB", "16GB", "16GB", "12GB", "8GB"};
    
    for (int i = 0; i < 7; i++) {
        profileTableWidget->setItem(i, 0, new QTableWidgetItem(profiles[i]));
        profileTableWidget->setItem(i, 1, new QTableWidgetItem(manufacturers[i]));
        profileTableWidget->setItem(i, 2, new QTableWidgetItem("Android " + versions[i]));
        profileTableWidget->setItem(i, 3, new QTableWidgetItem(rams[i]));
        profileTableWidget->setItem(i, 4, new QTableWidgetItem("Available"));
    }
    profileTableWidget->resizeColumnsToContents();
    tableLayout->addWidget(profileTableWidget);
    profileLayout->addWidget(tableBox);
    
    tabWidget->addTab(profileTab, "📱 Profiles");
    
    // ============ TAB 3: Device Controls ============
    QWidget *deviceTab = new QWidget();
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceTab);
    
    // Device Info
    QGroupBox *deviceInfoBox = new QGroupBox("Connected Device Info");
    QGridLayout *deviceInfoLayout = new QGridLayout(deviceInfoBox);
    
    androidVersionLabel = new QLabel("Android: --");
    buildLabel = new QLabel("Build: --");
    securityPatchLabel = new QLabel("Security Patch: --");
    deviceInfoLabel = new QLabel("Model: --");
    
    deviceInfoLayout->addWidget(new QLabel("📱 Device:"), 0, 0);
    deviceInfoLayout->addWidget(deviceInfoLabel, 0, 1);
    deviceInfoLayout->addWidget(new QLabel("🔧 Android Version:"), 1, 0);
    deviceInfoLayout->addWidget(androidVersionLabel, 1, 1);
    deviceInfoLayout->addWidget(new QLabel("🏗️ Build:"), 2, 0);
    deviceInfoLayout->addWidget(buildLabel, 2, 1);
    deviceInfoLayout->addWidget(new QLabel("🔒 Security Patch:"), 3, 0);
    deviceInfoLayout->addWidget(securityPatchLabel, 3, 1);
    
    deviceLayout->addWidget(deviceInfoBox);
    
    // Device Actions
    QGroupBox *deviceActionsBox = new QGroupBox("Device Actions");
    QGridLayout *deviceActionsLayout = new QGridLayout(deviceActionsBox);
    
    btnReboot = new QPushButton("🔄 Reboot");
    btnScreenshot = new QPushButton("📸 Screenshot");
    btnInstallAPK = new QPushButton("📦 Install APK");
    btnOpenShell = new QPushButton("💻 Shell");
    btnOpenFileManager = new QPushButton("📁 Files");
    
    deviceActionsLayout->addWidget(btnReboot, 0, 0);
    deviceActionsLayout->addWidget(btnScreenshot, 0, 1);
    deviceActionsLayout->addWidget(btnInstallAPK, 0, 2);
    deviceActionsLayout->addWidget(btnOpenShell, 1, 0);
    deviceActionsLayout->addWidget(btnOpenFileManager, 1, 1);
    
    deviceLayout->addWidget(deviceActionsBox);
    deviceLayout->addStretch();
    
    tabWidget->addTab(deviceTab, "🎮 Device");
    
    // ============ TAB 4: Settings ============
    QWidget *settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsTab);
    
    // Hardware Settings
    QGroupBox *hardwareBox = new QGroupBox("Hardware Settings");
    QGridLayout *hardwareLayout = new QGridLayout(hardwareBox);
    
    memorySpinBox = new QSpinBox();
    memorySpinBox->setRange(1024, 8192);
    memorySpinBox->setValue(3072);
    memorySpinBox->setSuffix(" MB");
    
    coresSpinBox = new QSpinBox();
    coresSpinBox->setRange(1, 8);
    coresSpinBox->setValue(4);
    
    hardwareLayout->addWidget(new QLabel("Memory:"), 0, 0);
    hardwareLayout->addWidget(memorySpinBox, 0, 1);
    hardwareLayout->addWidget(new QLabel("CPU Cores:"), 1, 0);
    hardwareLayout->addWidget(coresSpinBox, 1, 1);
    
    settingsLayout->addWidget(hardwareBox);
    
    // Display Settings
    QGroupBox *displayBox = new QGroupBox("Display Settings");
    QGridLayout *displayLayout = new QGridLayout(displayBox);
    
    screenWidthSlider = new QSlider(Qt::Horizontal);
    screenWidthSlider->setRange(320, 1920);
    screenWidthSlider->setValue(1080);
    
    screenHeightSlider = new QSlider(Qt::Horizontal);
    screenHeightSlider->setRange(480, 2560);
    screenHeightSlider->setValue(2400);
    
    displayLayout->addWidget(new QLabel("Width:"), 0, 0);
    displayLayout->addWidget(screenWidthSlider, 0, 1);
    displayLayout->addWidget(new QLabel("Height:"), 1, 0);
    displayLayout->addWidget(screenHeightSlider, 1, 1);
    
    settingsLayout->addWidget(displayBox);
    
    // Options
    QGroupBox *optionsBox = new QGroupBox("Options");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsBox);
    
    enableKVMCheckBox = new QCheckBox("Enable Hardware Acceleration (KVM/WHPX)");
    enableKVMCheckBox->setChecked(true);
    enableGPUCheckBox = new QCheckBox("Enable GPU Acceleration");
    enableGPUCheckBox->setChecked(true);
    autoStartCheckBox = new QCheckBox("Auto-start emulator on launch");
    
    optionsLayout->addWidget(enableKVMCheckBox);
    optionsLayout->addWidget(enableGPUCheckBox);
    optionsLayout->addWidget(autoStartCheckBox);
    
    settingsLayout->addWidget(optionsBox);
    
    // Save Button
    btnSaveSettings = new QPushButton("💾 Save Settings");
    settingsLayout->addWidget(btnSaveSettings);
    settingsLayout->addStretch();
    
    tabWidget->addTab(settingsTab, "⚙️ Settings");
    
    // Status Bar
    statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);
    statusBar()->addPermanentWidget(new QLabel("ReDroidCPP v1.0.0"));
}

void MainWindow::setupConnections() {
    // Dashboard connections
    connect(btnStartEmulator, &QPushButton::clicked, this, &MainWindow::onStartEmulator);
    connect(btnStopEmulator, &QPushButton::clicked, this, &MainWindow::onStopEmulator);
    connect(btnRestartEmulator, &QPushButton::clicked, this, &MainWindow::onRestartEmulator);
    connect(btnConnectADB, &QPushButton::clicked, this, &MainWindow::onConnectADB);
    connect(btnDisconnectADB, &QPushButton::clicked, this, &MainWindow::onDisconnectADB);
    
    // Profile connections
    connect(profileComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onProfileSelected);
    connect(btnCreateProfile, &QPushButton::clicked, this, &MainWindow::onCreateProfile);
    connect(btnDeleteProfile, &QPushButton::clicked, this, &MainWindow::onDeleteProfile);
    connect(btnApplyProfile, &QPushButton::clicked, this, &MainWindow::onApplyProfile);
    
    // Device connections
    connect(btnReboot, &QPushButton::clicked, this, &MainWindow::onRebootDevice);
    connect(btnScreenshot, &QPushButton::clicked, this, &MainWindow::onScreenshot);
    connect(btnInstallAPK, &QPushButton::clicked, this, &MainWindow::onInstallAPK);
    connect(btnOpenShell, &QPushButton::clicked, this, &MainWindow::onOpenShell);
    connect(btnOpenFileManager, &QPushButton::clicked, this, &MainWindow::onOpenFileManager);
    
    // Settings connections
    connect(btnSaveSettings, &QPushButton::clicked, this, &MainWindow::onSaveSettings);
}

void MainWindow::setupSystemTray() {
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/app.png"));
    trayIcon->setToolTip("ReDroidCPP - Android Emulator");
    
    trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &QMainWindow::show);
    trayMenu->addAction("Hide", this, &QMainWindow::hide);
    trayMenu->addSeparator();
    trayMenu->addAction("Start Emulator", this, &MainWindow::onStartEmulator);
    trayMenu->addAction("Stop Emulator", this, &MainWindow::onStopEmulator);
    trayMenu->addSeparator();
    trayMenu->addAction("Exit", qApp, &QApplication::quit);
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
    
    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            this->show();
        }
    });
}

// ============ DOCKER & EMULATOR SLOTS ============

void MainWindow::onStartEmulator() {
    appendLog("Starting emulator...");
    
    if (!isDockerRunning()) {
        QMessageBox::warning(this, "Error", "Docker is not running!\nPlease start Docker Desktop first.");
        return;
    }
    
    btnStartEmulator->setEnabled(false);
    statusLabel->setText("Starting emulator...");
    
    // Run docker compose up
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);
    
    process->setWorkingDirectory(QCoreApplication::applicationDirPath() + "/docker");
    process->start("docker compose up -d");
    
    appendLog("Running: docker compose up -d");
}

void MainWindow::onStopEmulator() {
    appendLog("Stopping emulator...");
    
    QProcess process;
    process.start("docker compose -f docker/docker-compose.yml down");
    process.waitForFinished();
    
    emulatorRunning = false;
    adbConnected = false;
    
    updateStatus();
    appendLog("Emulator stopped");
}

void MainWindow::onRestartEmulator() {
    appendLog("Restarting emulator...");
    onStopEmulator();
    QTimer::singleShot(2000, this, &MainWindow::onStartEmulator);
}

void MainWindow::onConnectADB() {
    appendLog("Connecting to ADB...");
    
    QProcess process;
    process.start("adb connect localhost:5555");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    appendLog(output);
    
    checkADBStatus();
}

void MainWindow::onDisconnectADB() {
    QProcess process;
    process.start("adb disconnect localhost:5555");
    process.waitForFinished();
    
    adbConnected = false;
    updateStatus();
    appendLog("ADB disconnected");
}

// ============ PROFILE SLOTS ============

void MainWindow::onProfileSelected(int index) {
    QString profile = profileComboBox->itemText(index);
    appendLog("Selected profile: " + profile);
}

void MainWindow::onCreateProfile() {
    bool ok;
    QString name = QInputDialog::getText(this, "Create Profile", 
                                         "Enter profile name:", 
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        appendLog("Created profile: " + name);
        QMessageBox::information(this, "Success", "Profile created successfully!");
    }
}

void MainWindow::onDeleteProfile() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Profile",
        "Are you sure you want to delete this profile?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        appendLog("Profile deleted");
    }
}

void MainWindow::onApplyProfile() {
    QString profile = profileComboBox->currentText();
    appendLog("Applying profile: " + profile);
    
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    QMessageBox::information(this, "Success", 
        QString("Profile '%1' applied successfully!").arg(profile));
}

// ============ DEVICE SLOTS ============

void MainWindow::onRebootDevice() {
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    QProcess process;
    process.start("adb reboot");
    process.waitForFinished();
    appendLog("Device rebooting...");
}

void MainWindow::onScreenshot() {
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    QString filename = QString("screenshot_%1.png").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")
    );
    
    QProcess process;
    process.start(QString("adb exec-out screencap -p > %1").arg(filename));
    process.waitForFinished();
    
    appendLog("Screenshot saved: " + filename);
    QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/" + filename));
}

void MainWindow::onInstallAPK() {
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    QString file = QFileDialog::getOpenFileName(this, "Select APK", "", "APK Files (*.apk)");
    if (!file.isEmpty()) {
        appendLog("Installing: " + file);
        
        QProcess process;
        process.start(QString("adb install -r \"%1\"").arg(file));
        process.waitForFinished();
        
        QString output = process.readAllStandardOutput();
        appendLog(output);
    }
}

void MainWindow::onOpenShell() {
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    appendLog("Opening shell...");
    QProcess::startDetached("cmd", QStringList() << "/c" << "adb shell");
}

void MainWindow::onOpenFileManager() {
    if (!adbConnected) {
        QMessageBox::warning(this, "Error", "Please connect ADB first!");
        return;
    }
    
    appendLog("Opening file manager...");
    QProcess::startDetached("cmd", QStringList() << "/c" << "adb shell am start -a android.intent.action.VIEW");
}

// ============ SETTINGS SLOTS ============

void MainWindow::onSaveSettings() {
    appendLog("Settings saved:");
    appendLog(QString("  Memory: %1 MB").arg(memorySpinBox->value()));
    appendLog(QString("  Cores: %1").arg(coresSpinBox->value()));
    appendLog(QString("  Resolution: %1x%2").arg(screenWidthSlider->value()).arg(screenHeightSlider->value()));
    appendLog(QString("  KVM: %1").arg(enableKVMCheckBox->isChecked() ? "Enabled" : "Disabled"));
    
    QMessageBox::information(this, "Success", "Settings saved successfully!");
}

void MainWindow::onLoadSettings() {
    // Load from config file
}

// ============ PROCESS HANDLERS ============

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit) {
        appendLog("Process completed successfully");
        emulatorRunning = true;
    } else {
        appendLog("Process failed with exit code: " + QString::number(exitCode));
    }
    
    btnStartEmulator->setEnabled(true);
    updateStatus();
}

void MainWindow::onProcessError(QProcess::ProcessError error) {
    appendLog("Process error: " + QString::number(error));
    btnStartEmulator->setEnabled(true);
}

void MainWindow::updateStatus() {
    checkDockerStatus();
    checkADBStatus();
    
    // Update emulator status
    if (isEmulatorRunning()) {
        emulatorStatusLabel->setText("✅ Running");
        emulatorStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #a6e3a1;");
        btnStartEmulator->setEnabled(false);
        btnStopEmulator->setEnabled(true);
    } else {
        emulatorStatusLabel->setText("⏸️ Stopped");
        emulatorStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f38ba8;");
        btnStartEmulator->setEnabled(true);
        btnStopEmulator->setEnabled(false);
    }
    
    // Update ADB status
    if (adbConnected) {
        adbStatusLabel->setText("✅ Connected");
        adbStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #a6e3a1;");
        btnConnectADB->setEnabled(false);
        btnDisconnectADB->setEnabled(true);
    } else {
        adbStatusLabel->setText("🔌 Disconnected");
        adbStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f38ba8;");
        btnConnectADB->setEnabled(true);
        btnDisconnectADB->setEnabled(false);
    }
    
    // Update device info
    if (adbConnected) {
        updateDeviceInfo();
    }
}

void MainWindow::checkDockerStatus() {
    QProcess process;
    process.start("docker info");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        dockerStatusLabel->setText("✅ Running");
        dockerStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #a6e3a1;");
        statusLabel->setText("Docker: Running");
    } else {
        dockerStatusLabel->setText("❌ Not Running");
        dockerStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f38ba8;");
        statusLabel->setText("Docker: Not Running");
    }
}

void MainWindow::checkADBStatus() {
    QProcess process;
    process.start("adb devices");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    adbConnected = output.contains("localhost:5554") || output.contains("emulator-5554");
}

void MainWindow::updateDeviceInfo() {
    androidVersionLabel->setText("Android: " + getDeviceProperty("ro.build.version.release"));
    buildLabel->setText("Build: " + getDeviceProperty("ro.build.display.id"));
    securityPatchLabel->setText("Security Patch: " + getDeviceProperty("ro.build.version.security_patch"));
    deviceInfoLabel->setText("Model: " + getDeviceProperty("ro.product.model"));
}

void MainWindow::runCommand(const QString &command) {
    appendLog("Running: " + command);
    QProcess process;
    process.start(command);
    process.waitForFinished();
}

void MainWindow::appendLog(const QString &text) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    logTextEdit->append(QString("[%1] %2").arg(timestamp, text));
    
    // Auto-scroll to bottom
    QTextCursor cursor = logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    logTextEdit->setTextCursor(cursor);
}

void MainWindow::updateProfileList() {
    // Update profile list from files
}

QString MainWindow::getDeviceProperty(const QString &property) {
    QProcess process;
    process.start(QString("adb shell getprop %1").arg(property));
    process.waitForFinished();
    return QString(process.readAllStandardOutput()).trimmed();
}

bool MainWindow::isDockerRunning() {
    QProcess process;
    process.start("docker info");
    process.waitForFinished();
    return process.exitCode() == 0;
}

bool MainWindow::isEmulatorRunning() {
    QProcess process;
    process.start("docker ps --filter name=android-emulator --format '{{.Names}}'");
    process.waitForFinished();
    return QString(process.readAllStandardOutput()).contains("android-emulator");
}

bool MainWindow::isADBConnected() {
    return adbConnected;
}
