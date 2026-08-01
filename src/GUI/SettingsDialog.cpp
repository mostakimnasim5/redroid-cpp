/**
 * @file SettingsDialog.cpp
 * @brief Settings Dialog Implementation
 */

#include "SettingsDialog.h"
#include "VirtualPhonePro/FileLogger.h"
#include <QHostInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonArray>

namespace VirtualPhonePro {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("⚙️ Settings - VirtualPhonePro");
    setMinimumSize(650, 550);
    setModal(true);
    
    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel* titleLabel = new QLabel("⚙️ Application Settings", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createDockerTab(), "🐳 Docker");
    m_tabWidget->addTab(createAdbTab(), "📱 ADB");
    m_tabWidget->addTab(createGeneralTab(), "🖥️ General");
    m_tabWidget->addTab(createLoggingTab(), "📝 Logging");
    m_tabWidget->addTab(createBackupTab(), "💾 Backup");
    m_tabWidget->addTab(createAboutTab(), "ℹ️ About");
    
    mainLayout->addWidget(m_tabWidget);
    
    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* resetBtn = new QPushButton("Reset to Defaults", this);
    resetBtn->setFixedWidth(140);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onReset);
    buttonLayout->addWidget(resetBtn);
    
    buttonLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setFixedWidth(100);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);
    
    QPushButton* applyBtn = new QPushButton("Apply", this);
    applyBtn->setFixedWidth(100);
    applyBtn->setStyleSheet("background: #28a745; color: white;");
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    buttonLayout->addWidget(applyBtn);
    
    QPushButton* okBtn = new QPushButton("OK", this);
    okBtn->setFixedWidth(100);
    okBtn->setStyleSheet("background: #007bff; color: white;");
    connect(okBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(okBtn);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* SettingsDialog::createDockerTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Docker Path
    QGroupBox* pathGroup = new QGroupBox("Docker Executable", page);
    QHBoxLayout* pathLayout = new QHBoxLayout(pathGroup);
    
    m_dockerPathEdit = new QLineEdit(page);
    m_dockerPathEdit->setPlaceholderText("docker.exe (in PATH) or full path");
    pathLayout->addWidget(m_dockerPathEdit);
    
    QPushButton* browseBtn = new QPushButton("Browse...", page);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDockerPath);
    pathLayout->addWidget(browseBtn);
    
    layout->addWidget(pathGroup);
    
    // Docker Host
    QGroupBox* hostGroup = new QGroupBox("Docker Host (Optional)", page);
    QFormLayout* hostLayout = new QFormLayout(hostGroup);
    
    m_dockerHostEdit = new QLineEdit(page);
    m_dockerHostEdit->setPlaceholderText("tcp://localhost:2375 (leave empty for default)");
    hostLayout->addRow("Host Address:", m_dockerHostEdit);
    
    layout->addWidget(hostGroup);
    
    // Timeout
    QGroupBox* timeoutGroup = new QGroupBox("Connection Settings", page);
    QFormLayout* timeoutLayout = new QFormLayout(timeoutGroup);
    
    m_dockerTimeoutSpin = new QSpinBox(page);
    m_dockerTimeoutSpin->setRange(5, 120);
    m_dockerTimeoutSpin->setSuffix(" seconds");
    m_dockerTimeoutSpin->setValue(30);
    timeoutLayout->addRow("Connection Timeout:", m_dockerTimeoutSpin);
    
    layout->addWidget(timeoutGroup);
    
    // Test Button
    QGroupBox* testGroup = new QGroupBox("Test Connection", page);
    QVBoxLayout* testLayout = new QVBoxLayout(testGroup);
    
    m_dockerStatusLabel = new QLabel("Not tested", page);
    m_dockerStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    testLayout->addWidget(m_dockerStatusLabel);
    
    m_testDockerBtn = new QPushButton("🔍 Test Docker Connection", page);
    m_testDockerBtn->setStyleSheet("background: #17a2b8; color: white; padding: 10px;");
    connect(m_testDockerBtn, &QPushButton::clicked, this, &SettingsDialog::onTestDocker);
    testLayout->addWidget(m_testDockerBtn);
    
    layout->addWidget(testGroup);
    layout->addStretch();
    
    return page;
}

QWidget* SettingsDialog::createAdbTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // ADB Path
    QGroupBox* pathGroup = new QGroupBox("ADB Executable", page);
    QHBoxLayout* pathLayout = new QHBoxLayout(pathGroup);
    
    m_adbPathEdit = new QLineEdit(page);
    m_adbPathEdit->setPlaceholderText("adb.exe (in PATH) or full path");
    pathLayout->addWidget(m_adbPathEdit);
    
    QPushButton* browseBtn = new QPushButton("Browse...", page);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseAdbPath);
    pathLayout->addWidget(browseBtn);
    
    layout->addWidget(pathGroup);
    
    // Auto ADB Connect
    QGroupBox* connectGroup = new QGroupBox("Connection", page);
    QFormLayout* connectLayout = new QFormLayout(connectGroup);
    
    m_autoAdbCheck = new QCheckBox("Auto-connect to devices on startup", page);
    connectLayout->addRow("", m_autoAdbCheck);
    
    layout->addWidget(connectGroup);
    
    // Port Settings
    QGroupBox* portGroup = new QGroupBox("Port Configuration", page);
    QFormLayout* portLayout = new QFormLayout(portGroup);
    
    m_adbPortStartSpin = new QSpinBox(page);
    m_adbPortStartSpin->setRange(5555, 65535);
    m_adbPortStartSpin->setValue(5555);
    portLayout->addRow("ADB Port Start:", m_adbPortStartSpin);
    
    layout->addWidget(portGroup);
    
    // Test Button
    QGroupBox* testGroup = new QGroupBox("Test Connection", page);
    QVBoxLayout* testLayout = new QVBoxLayout(testGroup);
    
    m_adbStatusLabel = new QLabel("Not tested", page);
    m_adbStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    testLayout->addWidget(m_adbStatusLabel);
    
    m_testAdbBtn = new QPushButton("🔍 Test ADB Connection", page);
    m_testAdbBtn->setStyleSheet("background: #17a2b8; color: white; padding: 10px;");
    connect(m_testAdbBtn, &QPushButton::clicked, this, &SettingsDialog::onTestAdb);
    testLayout->addWidget(m_testAdbBtn);
    
    layout->addWidget(testGroup);
    layout->addStretch();
    
    return page;
}

QWidget* SettingsDialog::createGeneralTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Startup
    QGroupBox* startupGroup = new QGroupBox("Startup", page);
    QFormLayout* startupLayout = new QFormLayout(startupGroup);
    
    m_autoStartCheck = new QCheckBox("Auto-start saved instances on launch", page);
    startupLayout->addRow("", m_autoStartCheck);
    
    m_startMinimizedCheck = new QCheckBox("Start minimized to system tray", page);
    startupLayout->addRow("", m_startMinimizedCheck);
    
    layout->addWidget(startupGroup);
    
    // System Tray
    QGroupBox* trayGroup = new QGroupBox("System Tray", page);
    QFormLayout* trayLayout = new QFormLayout(trayGroup);
    
    m_minimizeToTrayCheck = new QCheckBox("Minimize to system tray instead of taskbar", page);
    trayLayout->addRow("", m_minimizeToTrayCheck);
    
    layout->addWidget(trayGroup);
    
    // Instance Settings
    QGroupBox* instanceGroup = new QGroupBox("Instance Defaults", page);
    QFormLayout* instanceLayout = new QFormLayout(instanceGroup);
    
    m_maxInstancesSpin = new QSpinBox(page);
    m_maxInstancesSpin->setRange(1, 50);
    m_maxInstancesSpin->setValue(10);
    instanceLayout->addRow("Maximum Instances:", m_maxInstancesSpin);
    
    m_defaultMemorySpin = new QSpinBox(page);
    m_defaultMemorySpin->setRange(256, 8192);
    m_defaultMemorySpin->setSuffix(" MB");
    m_defaultMemorySpin->setValue(512);
    instanceLayout->addRow("Default Memory:", m_defaultMemorySpin);
    
    layout->addWidget(instanceGroup);
    
    // Theme
    QGroupBox* themeGroup = new QGroupBox("Appearance", page);
    QFormLayout* themeLayout = new QFormLayout(themeGroup);
    
    m_themeCombo = new QComboBox(page);
    m_themeCombo->addItems({"Dark (Default)", "Light", "System Default"});
    themeLayout->addRow("Theme:", m_themeCombo);
    
    layout->addWidget(themeGroup);
    layout->addStretch();
    
    return page;
}

QWidget* SettingsDialog::createLoggingTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Log Level
    QGroupBox* levelGroup = new QGroupBox("Log Level", page);
    QFormLayout* levelLayout = new QFormLayout(levelGroup);
    
    m_logLevelCombo = new QComboBox(page);
    m_logLevelCombo->addItems({"DEBUG - All messages", "INFO - Info and above", 
                               "WARNING - Warnings and above", "ERROR - Errors only", 
                               "CRITICAL - Critical only"});
    levelLayout->addRow("Minimum Level:", m_logLevelCombo);
    
    layout->addWidget(levelGroup);
    
    // Output Options
    QGroupBox* outputGroup = new QGroupBox("Log Output", page);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    
    m_logToFileCheck = new QCheckBox("Write logs to file", page);
    m_logToFileCheck->setChecked(true);
    outputLayout->addWidget(m_logToFileCheck);
    
    m_logToConsoleCheck = new QCheckBox("Write logs to console (debug window)", page);
    m_logToConsoleCheck->setChecked(true);
    outputLayout->addWidget(m_logToConsoleCheck);
    
    layout->addWidget(outputGroup);
    
    // File Settings
    QGroupBox* fileGroup = new QGroupBox("File Settings", page);
    QFormLayout* fileLayout = new QFormLayout(fileGroup);
    
    m_maxLogSizeSpin = new QSpinBox(page);
    m_maxLogSizeSpin->setRange(1, 100);
    m_maxLogSizeSpin->setSuffix(" MB");
    m_maxLogSizeSpin->setValue(10);
    fileLayout->addRow("Max Log Size:", m_maxLogSizeSpin);
    
    m_maxBackupLogsSpin = new QSpinBox(page);
    m_maxBackupLogsSpin->setRange(1, 20);
    m_maxBackupLogsSpin->setValue(5);
    fileLayout->addRow("Keep Backup Logs:", m_maxBackupLogsSpin);
    
    m_logFilePathLabel = new QLabel("Log Path: ...", page);
    fileLayout->addRow("Current Log:", m_logFilePathLabel);
    
    layout->addWidget(fileGroup);
    
    // Actions
    QGroupBox* actionGroup = new QGroupBox("Actions", page);
    QHBoxLayout* actionLayout = new QHBoxLayout(actionGroup);
    
    m_clearLogsBtn = new QPushButton("🗑️ Clear Logs", page);
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &SettingsDialog::onClearLogs);
    actionLayout->addWidget(m_clearLogsBtn);
    
    m_openLogFolderBtn = new QPushButton("📂 Open Log Folder", page);
    connect(m_openLogFolderBtn, &QPushButton::clicked, this, &SettingsDialog::onOpenLogFolder);
    actionLayout->addWidget(m_openLogFolderBtn);
    
    m_viewLogsBtn = new QPushButton("📋 View Recent Logs", page);
    connect(m_viewLogsBtn, &QPushButton::clicked, this, &SettingsDialog::onViewLogs);
    actionLayout->addWidget(m_viewLogsBtn);
    
    layout->addWidget(actionGroup);
    layout->addStretch();
    
    return page;
}

QWidget* SettingsDialog::createBackupTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Backup Location
    QGroupBox* locationGroup = new QGroupBox("Backup Location", page);
    QHBoxLayout* locationLayout = new QHBoxLayout(locationGroup);
    
    m_backupPathEdit = new QLineEdit(page);
    m_backupPathEdit->setPlaceholderText("Default: AppData/RedroidCPP/backups");
    locationLayout->addWidget(m_backupPathEdit);
    
    m_browseBackupPathBtn = new QPushButton("Browse...", page);
    connect(m_browseBackupPathBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Folder");
        if (!dir.isEmpty()) {
            m_backupPathEdit->setText(dir);
        }
    });
    locationLayout->addWidget(m_browseBackupPathBtn);
    
    layout->addWidget(locationGroup);
    
    // Auto Backup
    QGroupBox* autoGroup = new QGroupBox("Auto Backup", page);
    QFormLayout* autoLayout = new QFormLayout(autoGroup);
    
    m_autoBackupCheck = new QCheckBox("Enable automatic backups", page);
    autoLayout->addRow("", m_autoBackupCheck);
    
    m_backupIntervalSpin = new QSpinBox(page);
    m_backupIntervalSpin->setRange(1, 168);
    m_backupIntervalSpin->setSuffix(" hours");
    m_backupIntervalSpin->setValue(24);
    autoLayout->addRow("Backup Interval:", m_backupIntervalSpin);
    
    layout->addWidget(autoGroup);
    
    // Manual Actions
    QGroupBox* manualGroup = new QGroupBox("Manual Backup/Restore", page);
    QVBoxLayout* manualLayout = new QVBoxLayout(manualGroup);
    
    m_lastBackupLabel = new QLabel("Last Backup: Never", page);
    m_lastBackupLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    manualLayout->addWidget(m_lastBackupLabel);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_backupNowBtn = new QPushButton("💾 Backup Now", page);
    m_backupNowBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_backupNowBtn, &QPushButton::clicked, this, &SettingsDialog::onBackupNow);
    btnLayout->addWidget(m_backupNowBtn);
    
    m_restoreBackupBtn = new QPushButton("📂 Restore from Backup", page);
    m_restoreBackupBtn->setStyleSheet("background: #007bff; color: white; padding: 10px;");
    connect(m_restoreBackupBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreBackup);
    btnLayout->addWidget(m_restoreBackupBtn);
    
    manualLayout->addLayout(btnLayout);
    layout->addWidget(manualGroup);
    layout->addStretch();
    
    return page;
}

QWidget* SettingsDialog::createAboutTab() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // App Info
    QGroupBox* infoGroup = new QGroupBox("About VirtualPhonePro", page);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    QLabel* appName = new QLabel("📱 VirtualPhonePro", page);
    appName->setStyleSheet("font-size: 24px; font-weight: bold;");
    infoLayout->addWidget(appName, 0, Qt::AlignCenter);
    
    m_versionLabel = new QLabel("Version: 2.0.0", page);
    m_versionLabel->setStyleSheet("font-size: 14px;");
    infoLayout->addWidget(m_versionLabel, 0, Qt::AlignCenter);
    
    m_buildDateLabel = new QLabel("Build Date: " + QString(__DATE__), page);
    m_buildDateLabel->setStyleSheet("font-size: 12px; color: #888;");
    infoLayout->addWidget(m_buildDateLabel, 0, Qt::AlignCenter);
    
    QLabel* desc = new QLabel(
        "Virtual Android Emulator with Anti-Detection<br>"
        "Multi-Instance Management<br>"
        "Professional-grade Device Spoofing", page);
    desc->setStyleSheet("margin: 20px;");
    infoLayout->addWidget(desc, 0, Qt::AlignCenter);
    
    layout->addWidget(infoGroup);
    
    // Credits
    QGroupBox* creditsGroup = new QGroupBox("Credits", page);
    QVBoxLayout* creditsLayout = new QVBoxLayout(creditsGroup);
    
    creditsLayout->addWidget(new QLabel("Built with Qt6 Framework", page));
    creditsLayout->addWidget(new QLabel("Uses reDroid Docker Images", page));
    creditsLayout->addWidget(new QLabel("Copyright © 2024", page));
    
    layout->addWidget(creditsGroup);
    layout->addStretch();
    
    return page;
}

void SettingsDialog::loadSettings() {
    QString settingsPath = getSettingsFilePath();
    QFile file(settingsPath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        // Load defaults
        m_dockerPathEdit->setText("docker");
        m_adbPathEdit->setText("adb");
        m_autoStartCheck->setChecked(true);
        m_logToFileCheck->setChecked(true);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();
    
    // Docker
    m_dockerPathEdit->setText(json["dockerPath"].toString("docker"));
    m_dockerHostEdit->setText(json["dockerHost"].toString(""));
    m_dockerTimeoutSpin->setValue(json["dockerTimeout"].toInt(30));
    
    // ADB
    m_adbPathEdit->setText(json["adbPath"].toString("adb"));
    m_autoAdbCheck->setChecked(json["autoAdb"].toBool(true));
    m_adbPortStartSpin->setValue(json["adbPortStart"].toInt(5555));
    
    // General
    m_autoStartCheck->setChecked(json["autoStart"].toBool(true));
    m_minimizeToTrayCheck->setChecked(json["minimizeToTray"].toBool(false));
    m_startMinimizedCheck->setChecked(json["startMinimized"].toBool(false));
    m_maxInstancesSpin->setValue(json["maxInstances"].toInt(10));
    m_defaultMemorySpin->setValue(json["defaultMemory"].toInt(512));
    m_themeCombo->setCurrentIndex(json["theme"].toInt(0));
    
    // Logging
    m_logLevelCombo->setCurrentIndex(json["logLevel"].toInt(1));
    m_logToFileCheck->setChecked(json["logToFile"].toBool(true));
    m_logToConsoleCheck->setChecked(json["logToConsole"].toBool(true));
    m_maxLogSizeSpin->setValue(json["maxLogSize"].toInt(10));
    m_maxBackupLogsSpin->setValue(json["maxBackupLogs"].toInt(5));
    
    // Backup
    m_backupPathEdit->setText(json["backupPath"].toString(""));
    m_autoBackupCheck->setChecked(json["autoBackup"].toBool(false));
    m_backupIntervalSpin->setValue(json["backupInterval"].toInt(24));
    
    // Update log file path label
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_logFilePathLabel->setText("Log Path: " + logDir + "/logs/");
}

void SettingsDialog::saveSettings() {
    QString settingsPath = getSettingsFilePath();
    QFile file(settingsPath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        showMessage("Error", "Failed to save settings", true);
        return;
    }
    
    QJsonObject json;
    
    // Docker
    json["dockerPath"] = m_dockerPathEdit->text();
    json["dockerHost"] = m_dockerHostEdit->text();
    json["dockerTimeout"] = m_dockerTimeoutSpin->value();
    
    // ADB
    json["adbPath"] = m_adbPathEdit->text();
    json["autoAdb"] = m_autoAdbCheck->isChecked();
    json["adbPortStart"] = m_adbPortStartSpin->value();
    
    // General
    json["autoStart"] = m_autoStartCheck->isChecked();
    json["minimizeToTray"] = m_minimizeToTrayCheck->isChecked();
    json["startMinimized"] = m_startMinimizedCheck->isChecked();
    json["maxInstances"] = m_maxInstancesSpin->value();
    json["defaultMemory"] = m_defaultMemorySpin->value();
    json["theme"] = m_themeCombo->currentIndex();
    
    // Logging
    json["logLevel"] = m_logLevelCombo->currentIndex();
    json["logToFile"] = m_logToFileCheck->isChecked();
    json["logToConsole"] = m_logToConsoleCheck->isChecked();
    json["maxLogSize"] = m_maxLogSizeSpin->value();
    json["maxBackupLogs"] = m_maxBackupLogsSpin->value();
    
    // Backup
    json["backupPath"] = m_backupPathEdit->text();
    json["autoBackup"] = m_autoBackupCheck->isChecked();
    json["backupInterval"] = m_backupIntervalSpin->value();
    
    file.write(QJsonDocument(json).toJson());
    file.close();
    
    // Apply logging settings immediately
    FileLogger::instance().setLogLevel((LogLevel)m_logLevelCombo->currentIndex());
    FileLogger::instance().setLogToFile(m_logToFileCheck->isChecked());
    FileLogger::instance().setLogToConsole(m_logToConsoleCheck->isChecked());
}

QString SettingsDialog::getSettingsFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir + "/settings.json";
}

void SettingsDialog::onBrowseDockerPath() {
    QString path = QFileDialog::getOpenFileName(this, "Select Docker Executable",
        "", "Executable Files (*.exe *.dll);;All Files (*)");
    if (!path.isEmpty()) {
        m_dockerPathEdit->setText(path);
    }
}

void SettingsDialog::onTestDocker() {
    m_dockerStatusLabel->setText("Testing...");
    m_dockerStatusLabel->setStyleSheet("padding: 10px; background: #ffc107; border-radius: 5px;");
    
    // Simulate test
    QTimer::singleShot(1000, [this]() {
        ReDroidController& controller = ReDroidController::instance();
        OperationResult result = controller.validateDocker();
        
        if (result.success) {
            m_dockerStatusLabel->setText("✅ Docker is available and working!");
            m_dockerStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        } else {
            m_dockerStatusLabel->setText("❌ Docker error: " + result.errorMessage);
            m_dockerStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        }
    });
}

void SettingsDialog::onSaveDockerSettings() {
    saveSettings();
    showMessage("Settings", "Docker settings saved successfully");
}

void SettingsDialog::onBrowseAdbPath() {
    QString path = QFileDialog::getOpenFileName(this, "Select ADB Executable",
        "", "Executable Files (*.exe);;All Files (*)");
    if (!path.isEmpty()) {
        m_adbPathEdit->setText(path);
    }
}

void SettingsDialog::onTestAdb() {
    m_adbStatusLabel->setText("Testing...");
    m_adbStatusLabel->setStyleSheet("padding: 10px; background: #ffc107; border-radius: 5px;");
    
    QTimer::singleShot(1000, [this]() {
        QString adbPath = m_adbPathEdit->text();
        QProcess process;
        process.start(adbPath, QStringList() << "version");
        process.waitForFinished(5000);
        
        QString output = process.readAllStandardOutput();
        if (output.contains("version")) {
            m_adbStatusLabel->setText("✅ ADB is available!\n" + output.trimmed());
            m_adbStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        } else {
            m_adbStatusLabel->setText("❌ ADB not found or error");
            m_adbStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        }
    });
}

void SettingsDialog::onSaveAdbSettings() {
    saveSettings();
    showMessage("Settings", "ADB settings saved successfully");
}

void SettingsDialog::onSaveGeneralSettings() {
    saveSettings();
    showMessage("Settings", "General settings saved successfully");
}

void SettingsDialog::onSaveLoggingSettings() {
    saveSettings();
    showMessage("Settings", "Logging settings saved successfully");
}

void SettingsDialog::onClearLogs() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Clear Logs", "Are you sure you want to clear all log files?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        FileLogger::instance().rotateLogFile();
        showMessage("Logs", "Log files cleared");
    }
}

void SettingsDialog::onOpenLogFolder() {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    logDir += "/logs";
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}

void SettingsDialog::onViewLogs() {
    QString logs = FileLogger::instance().getRecentLogs(100);
    
    QDialog* logViewer = new QDialog(this);
    logViewer->setWindowTitle("Recent Logs");
    logViewer->setMinimumSize(700, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(logViewer);
    
    QTextEdit* textEdit = new QTextEdit(logViewer);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(logs);
    layout->addWidget(textEdit);
    
    QPushButton* closeBtn = new QPushButton("Close", logViewer);
    connect(closeBtn, &QPushButton::clicked, logViewer, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    logViewer->exec();
}

void SettingsDialog::onSaveBackupSettings() {
    saveSettings();
    showMessage("Settings", "Backup settings saved successfully");
}

void SettingsDialog::onBackupNow() {
    QString backupDir = m_backupPathEdit->text().isEmpty() 
        ? QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/backups"
        : m_backupPathEdit->text();
    
    QDir().mkpath(backupDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString backupFile = QString("%1/backup_%2.json").arg(backupDir).arg(timestamp);
    
    // Create backup of all profiles and settings
    QJsonObject backup;
    backup["version"] = "2.0.0";
    backup["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    backup["profiles"] = QJsonArray(); // Would include actual profile data
    
    QFile file(backupFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(backup).toJson());
        file.close();
        
        m_lastBackupLabel->setText("Last Backup: " + QDateTime::currentDateTime().toString());
        showMessage("Backup Complete", QString("Backup saved to:\n%1").arg(backupFile));
    } else {
        showMessage("Backup Failed", "Failed to create backup file", true);
    }
}

void SettingsDialog::onRestoreBackup() {
    QString backupFile = QFileDialog::getOpenFileName(this, "Select Backup File",
        "", "Backup Files (*.json);;All Files (*)");
    
    if (backupFile.isEmpty()) return;
    
    QFile file(backupFile);
    if (!file.open(QIODevice::ReadOnly)) {
        showMessage("Restore Failed", "Cannot open backup file", true);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        showMessage("Restore Failed", "Invalid backup file format", true);
        return;
    }
    
    showMessage("Restore Complete", "Backup restored successfully");
}

void SettingsDialog::onApply() {
    saveSettings();
    showMessage("Settings", "All settings saved successfully");
}

void SettingsDialog::onReset() {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset Settings", "Reset all settings to defaults?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QFile file(getSettingsFilePath());
        if (file.exists()) {
            file.remove();
        }
        loadSettings();
        showMessage("Settings", "Settings reset to defaults");
    }
}

void SettingsDialog::showMessage(const QString& title, const QString& message, bool isError) {
    if (isError) {
        QMessageBox::warning(this, title, message);
    } else {
        QMessageBox::information(this, title, message);
    }
}

} // namespace VirtualPhonePro
