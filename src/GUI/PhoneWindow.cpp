#include "PhoneWindow.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QProcess>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFile>
#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QDateTime>

namespace VirtualPhonePro {

// ==============================================================================
// AppManagerDialog Implementation
// ==============================================================================

AppManagerDialog::AppManagerDialog(const QString& instanceId, QWidget* parent)
    : QDialog(parent)
    , m_instanceId(instanceId)
{
    setWindowTitle(QString("App Manager - %1").arg(instanceId));
    setMinimumSize(600, 500);
    resize(700, 550);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QLabel* header = new QLabel("Installed Applications", this);
    header->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(header);
    
    m_appTable = new QTableWidget(this);
    m_appTable->setColumnCount(3);
    m_appTable->setHorizontalHeaderLabels({"Package Name", "App Name", "Version"});
    m_appTable->horizontalHeader()->setStretchLastSection(true);
    m_appTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_appTable);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton("🔄 Refresh", this);
    connect(m_refreshButton, &QPushButton::clicked, this, &AppManagerDialog::onRefreshClicked);
    buttonLayout->addWidget(m_refreshButton);
    
    m_launchButton = new QPushButton("🚀 Launch", this);
    m_launchButton->setEnabled(false);
    connect(m_launchButton, &QPushButton::clicked, this, &AppManagerDialog::onLaunchClicked);
    buttonLayout->addWidget(m_launchButton);
    
    m_uninstallButton = new QPushButton("🗑 Uninstall", this);
    m_uninstallButton->setEnabled(false);
    connect(m_uninstallButton, &QPushButton::clicked, this, &AppManagerDialog::onUninstallClicked);
    buttonLayout->addWidget(m_uninstallButton);
    
    buttonLayout->addStretch();
    
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_appTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_appTable->selectedItems().isEmpty();
        m_launchButton->setEnabled(hasSelection);
        m_uninstallButton->setEnabled(hasSelection);
    });
    
    loadInstalledApps();
}

AppManagerDialog::~AppManagerDialog() {}

void AppManagerDialog::loadInstalledApps() {
    m_appTable->setRowCount(0);
    m_apps.clear();
    
    QString adbPath = "adb";
    QString serial = QString("127.0.0.1:%1").arg(5555);
    
    QProcess process;
    process.start(adbPath, {"-s", serial, "shell", "pm", "list", "packages", "-3"});
    process.waitForFinished(10000);
    
    QString output = process.readAllStandardOutput();
    QStringList packages = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : packages) {
        QString package = line.trimmed();
        if (package.startsWith("package:")) {
            package = package.mid(8);
            
            AppInfo app;
            app.packageName = package;
            
            QProcess nameProcess;
            nameProcess.start(adbPath, {"-s", serial, "shell", "dumpsys", "package", package});
            nameProcess.waitForFinished(5000);
            QString dumpOutput = nameProcess.readAllStandardOutput();
            
            QStringList lines = dumpOutput.split('\n');
            for (const QString& l : lines) {
                if (l.trimmed().startsWith("Application label:")) {
                    app.appName = l.split(':').value(1).trimmed();
                    break;
                }
            }
            if (app.appName.isEmpty()) {
                app.appName = package;
            }
            
            m_apps.append(app);
        }
    }
    
    m_appTable->setRowCount(m_apps.size());
    for (int i = 0; i < m_apps.size(); ++i) {
        const AppInfo& app = m_apps[i];
        m_appTable->setItem(i, 0, new QTableWidgetItem(app.packageName));
        m_appTable->setItem(i, 1, new QTableWidgetItem(app.appName));
        m_appTable->setItem(i, 2, new QTableWidgetItem(app.version));
    }
}

void AppManagerDialog::onRefreshClicked() {
    loadInstalledApps();
}

void AppManagerDialog::onLaunchClicked() {
    int row = m_appTable->currentRow();
    if (row >= 0 && row < m_apps.size()) {
        QString package = m_apps[row].packageName;
        
        QString adbPath = "adb";
        QString serial = QString("127.0.0.1:%1").arg(5555);
        
        QProcess process;
        process.start(adbPath, {"-s", serial, "shell", "monkey", "-p", package, "1"});
        process.waitForFinished(10000);
        
        if (process.exitCode() == 0) {
            QMessageBox::information(this, "Launch", 
                QString("App '%1' launched successfully").arg(m_apps[row].appName));
        } else {
            QMessageBox::warning(this, "Error", 
                QString("Failed to launch app '%1'").arg(m_apps[row].appName));
        }
    }
}

void AppManagerDialog::onUninstallClicked() {
    int row = m_appTable->currentRow();
    if (row >= 0 && row < m_apps.size()) {
        QString package = m_apps[row].packageName;
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Uninstall",
            QString("Are you sure you want to uninstall '%1'?").arg(m_apps[row].appName),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            QString adbPath = "adb";
            QString serial = QString("127.0.0.1:%1").arg(5555);
            
            QProcess process;
            process.start(adbPath, {"-s", serial, "shell", "pm", "uninstall", package});
            process.waitForFinished(30000);
            
            QString output = process.readAllStandardOutput();
            if (output.contains("Success")) {
                QMessageBox::information(this, "Success", "App uninstalled successfully!");
                loadInstalledApps();
            } else {
                QMessageBox::warning(this, "Error", "Failed to uninstall app");
            }
        }
    }
}

// ==============================================================================
// PhoneWindow Implementation
// ==============================================================================

PhoneWindow::PhoneWindow(const QString& instanceId, 
                         const DeviceProfile& profile,
                         QWidget* parent)
    : QMainWindow(parent)
    , m_instanceId(instanceId)
    , m_profile(profile)
    , m_screenTimer(nullptr)
    , m_screenLabel(nullptr)
    , m_adbScreenProcess(nullptr)
    , m_screenMirrorActive(false)
    , m_isDragging(false)
    , m_installProcess(nullptr)
    , m_installProgress(nullptr)
{
    setWindowTitle(QString("%1 - %2").arg(m_instanceId).arg(profile.name));
    setMinimumSize(480, 950);
    resize(480, 980);
    setAcceptDrops(true);
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    
    setupUI();
    setupToolbar();
    setupScreenMirror();
    setupConnections();
    
    QTimer::singleShot(500, this, &PhoneWindow::startScreenMirror);
}

PhoneWindow::~PhoneWindow() {
    stopScreenMirror();
    
    if (m_adbScreenProcess) {
        m_adbScreenProcess->kill();
        m_adbScreenProcess->waitForFinished();
        delete m_adbScreenProcess;
    }
    
    if (m_installProcess) {
        m_installProcess->kill();
        m_installProcess->waitForFinished();
        delete m_installProcess;
    }
}

void PhoneWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);
    
    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel(m_profile.name, this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(m_titleLabel);
    
    m_statusLabel = new QLabel("Connecting...", this);
    m_statusLabel->setStyleSheet("color: #f39c12; font-size: 12px;");
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();
    
    m_mainLayout->addLayout(headerLayout);
    
    // Phone Screen
    QFrame* screenFrame = new QFrame(this);
    screenFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    screenFrame->setStyleSheet("QFrame { background-color: #1a1a2e; border-radius: 20px; border: 3px solid #3a3a4e; }");
    
    QVBoxLayout* screenLayout = new QVBoxLayout(screenFrame);
    screenLayout->setContentsMargins(5, 5, 5, 5);
    
    m_screenLabel = new QLabel(this);
    m_screenLabel->setMinimumSize(400, 700);
    m_screenLabel->setMaximumSize(400, 700);
    m_screenLabel->setAlignment(Qt::AlignCenter);
    m_screenLabel->setStyleSheet("QLabel { background-color: #0a0a15; border-radius: 16px; color: #666; font-size: 14px; }");
    m_screenLabel->setText("\n\n📱\nConnecting to device...\n\nDrag & drop APK here to install");
    m_screenLabel->setWordWrap(true);
    m_screenLabel->setAcceptDrops(true);
    m_screenLabel->setMouseTracking(true);
    m_screenLabel->installEventFilter(this);
    
    screenLayout->addWidget(m_screenLabel, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(screenFrame, 0, Qt::AlignCenter);
    
    // Hardware Buttons
    QHBoxLayout* hardwareLayout = new QHBoxLayout();
    hardwareLayout->addStretch();
    
    m_backButton = new QPushButton("◀ Back", this);
    m_backButton->setMinimumSize(70, 40);
    m_backButton->setStyleSheet("QPushButton { background-color: #34495e; color: white; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background-color: #4a6278; }");
    connect(m_backButton, &QPushButton::clicked, this, &PhoneWindow::onBackClicked);
    hardwareLayout->addWidget(m_backButton);
    
    m_homeButton = new QPushButton("⬤ Home", this);
    m_homeButton->setMinimumSize(70, 40);
    m_homeButton->setStyleSheet("QPushButton { background-color: #34495e; color: white; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background-color: #4a6278; }");
    connect(m_homeButton, &QPushButton::clicked, this, &PhoneWindow::onHomeClicked);
    hardwareLayout->addWidget(m_homeButton);
    
    m_appsButton = new QPushButton("▣ Apps", this);
    m_appsButton->setMinimumSize(70, 40);
    m_appsButton->setStyleSheet("QPushButton { background-color: #34495e; color: white; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background-color: #4a6278; }");
    connect(m_appsButton, &QPushButton::clicked, this, &PhoneWindow::onAppsClicked_Open);
    hardwareLayout->addWidget(m_appsButton);
    
    m_powerButton = new QPushButton("⏻", this);
    m_powerButton->setMinimumSize(50, 40);
    m_powerButton->setStyleSheet("QPushButton { background-color: #34495e; color: white; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background-color: #4a6278; }");
    connect(m_powerButton, &QPushButton::clicked, this, &PhoneWindow::onHomeClicked);
    hardwareLayout->addWidget(m_powerButton);
    
    hardwareLayout->addStretch();
    m_mainLayout->addLayout(hardwareLayout);
    
    // Mirror Controls
    QHBoxLayout* mirrorLayout = new QHBoxLayout();
    mirrorLayout->addStretch();
    
    m_startMirrorButton = new QPushButton("▶ Start Mirror", this);
    m_startMirrorButton->setEnabled(false);
    m_startMirrorButton->setStyleSheet("QPushButton { background-color: #27ae60; color: white; border: none; padding: 8px 16px; border-radius: 6px; font-weight: bold; } QPushButton:hover { background-color: #2ecc71; } QPushButton:disabled { background-color: #555; }");
    connect(m_startMirrorButton, &QPushButton::clicked, this, &PhoneWindow::startScreenMirror);
    mirrorLayout->addWidget(m_startMirrorButton);
    
    m_stopMirrorButton = new QPushButton("⏹ Stop Mirror", this);
    m_stopMirrorButton->setEnabled(false);
    m_stopMirrorButton->setStyleSheet("QPushButton { background-color: #e74c3c; color: white; border: none; padding: 8px 16px; border-radius: 6px; font-weight: bold; } QPushButton:hover { background-color: #c0392b; } QPushButton:disabled { background-color: #555; }");
    connect(m_stopMirrorButton, &QPushButton::clicked, this, &PhoneWindow::stopScreenMirror);
    mirrorLayout->addWidget(m_stopMirrorButton);
    
    mirrorLayout->addStretch();
    m_mainLayout->addLayout(mirrorLayout);
    
    // Device Info
    QGroupBox* infoGroup = new QGroupBox("Device Info", this);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_androidVersionLabel = new QLabel(QString("Android %1").arg(m_profile.build.androidVersion), this);
    infoLayout->addRow("Version:", m_androidVersionLabel);
    
    m_resolutionLabel = new QLabel("1080x1920", this);
    infoLayout->addRow("Resolution:", m_resolutionLabel);
    
    m_imeiLabel = new QLabel(m_profile.identity.imei, this);
    m_imeiLabel->setStyleSheet("font-size: 10px;");
    infoLayout->addRow("IMEI:", m_imeiLabel);
    
    m_mainLayout->addWidget(infoGroup);
    
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void PhoneWindow::setupToolbar() {
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setStyleSheet("QToolBar { background-color: #2a2a3e; border: none; spacing: 5px; }");
    addToolBar(m_toolbar);
    
    m_installApkAction = new QAction("📦 Install APK", this);
    m_installApkAction->setToolTip("Install APK file");
    connect(m_installApkAction, &QAction::triggered, this, &PhoneWindow::onInstallApkClicked);
    m_toolbar->addAction(m_installApkAction);
    
    m_appsAction = new QAction("📱 Apps", this);
    m_appsAction->setToolTip("Manage installed apps");
    connect(m_appsAction, &QAction::triggered, this, &PhoneWindow::onAppsClicked_Open);
    m_toolbar->addAction(m_appsAction);
    
    m_toolbar->addSeparator();
    
    m_screenshotAction = new QAction("📸 Screenshot", this);
    m_screenshotAction->setToolTip("Save screenshot to Desktop");
    connect(m_screenshotAction, &QAction::triggered, this, &PhoneWindow::onScreenshotClicked);
    m_toolbar->addAction(m_screenshotAction);
}

void PhoneWindow::setupScreenMirror() {
    m_adbScreenProcess = new QProcess(this);
    m_adbScreenProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(m_adbScreenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneWindow::onScreenProcessFinished);
    
    m_screenTimer = new QTimer(this);
    connect(m_screenTimer, &QTimer::timeout, this, &PhoneWindow::updateScreen);
}

void PhoneWindow::setupConnections() {
    ReDroidController& controller = ReDroidController::instance();
    
    connect(&controller, &ReDroidController::instanceStateChanged,
            this, &PhoneWindow::onInstanceStateChanged);
    connect(&controller, &ReDroidController::adbConnectionChanged,
            this, &PhoneWindow::onAdbConnectionChanged);
}

bool PhoneWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_screenLabel && m_screenMirrorActive) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            onScreenMousePress(mouseEvent);
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            onScreenMouseMove(mouseEvent);
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            onScreenMouseRelease(mouseEvent);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void PhoneWindow::startScreenMirror() {
    if (m_screenMirrorActive) return;
    
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state != InstanceState::Running || !info.adbConnected) {
        m_statusLabel->setText("Device not connected");
        return;
    }
    
    QString adbSerial = getAdbSerial();
    QString adbPath = getAdbPath();
    
    QStringList args = {"-s", adbSerial, "exec-out", "screencap", "-p"};
    
    m_adbScreenProcess->start(adbPath, args);
    
    if (m_adbScreenProcess->waitForStarted(1000)) {
        m_screenMirrorActive = true;
        m_screenTimer->start(100);
        m_startMirrorButton->setEnabled(false);
        m_stopMirrorButton->setEnabled(true);
        m_statusLabel->setText("Mirror Active");
        m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
    } else {
        m_statusLabel->setText("Failed to start mirror");
    }
}

void PhoneWindow::stopScreenMirror() {
    if (!m_screenMirrorActive) return;
    
    m_screenTimer->stop();
    
    if (m_adbScreenProcess && m_adbScreenProcess->state() != QProcess::NotRunning) {
        m_adbScreenProcess->kill();
        m_adbScreenProcess->waitForFinished(500);
    }
    
    m_screenMirrorActive = false;
    m_screenBuffer.clear();
    
    m_startMirrorButton->setEnabled(true);
    m_stopMirrorButton->setEnabled(false);
    m_statusLabel->setText("Mirror Stopped");
    m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    
    if (m_screenLabel) {
        m_screenLabel->setText("\n\n📱\nMirror stopped\nClick 'Start Mirror' to continue\n\nDrag & drop APK here to install");
    }
}

void PhoneWindow::refreshInstance() {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state == InstanceState::Running && info.adbConnected && !m_screenMirrorActive) {
        startScreenMirror();
    }
    
    updateInstanceCard(info);
}

void PhoneWindow::updateScreen() {
    if (!m_screenMirrorActive || !m_adbScreenProcess) return;
    
    QByteArray data = m_adbScreenProcess->readAllStandardOutput();
    if (data.isEmpty()) return;
    
    m_screenBuffer.append(data);
    
    static const QByteArray pngSig("\x89PNG\r\n\x1A\n", 8);
    static const QByteArray pngEnd("\x49\x45\x4E\x44\xAE\x42\x60\x82", 8);
    
    int startPos = m_screenBuffer.indexOf(pngSig);
    if (startPos == -1) {
        if (m_screenBuffer.size() > 10 * 1024 * 1024) m_screenBuffer.clear();
        return;
    }
    
    int endPos = m_screenBuffer.indexOf(pngEnd, startPos);
    if (endPos == -1) return;
    
    endPos += 8;
    QByteArray pngData = m_screenBuffer.mid(startPos, endPos - startPos);
    m_screenBuffer.remove(0, endPos);
    
    QPixmap pixmap;
    if (pixmap.loadFromData(pngData, "PNG")) {
        QPixmap scaled = pixmap.scaled(QSize(400, 700), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_screenLabel->setPixmap(scaled);
    }
}

void PhoneWindow::onScreenProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    if (exitStatus == QProcess::CrashExit && m_screenMirrorActive) {
        QTimer::singleShot(100, this, &PhoneWindow::startScreenMirror);
    }
}

void PhoneWindow::onScreenMousePress(QMouseEvent* event) {
    QPoint pos = event->pos();
    if (m_screenLabel->rect().contains(pos)) {
        m_touchStartPos = pos;
        m_isDragging = false;
    }
}

void PhoneWindow::onScreenMouseMove(QMouseEvent* event) {
    QPoint pos = event->pos();
    if (m_touchStartPos != QPoint(0, 0) && (pos - m_touchStartPos).manhattanLength() > 10) {
        m_isDragging = true;
    }
}

void PhoneWindow::onScreenMouseRelease(QMouseEvent* event) {
    QPoint pos = event->pos();
    
    if (m_isDragging) {
        sendAdbSwipe(toAndroidX(m_touchStartPos.x()), toAndroidY(m_touchStartPos.y()),
                   toAndroidX(pos.x()), toAndroidY(pos.y()), 300);
    } else {
        sendAdbTap(toAndroidX(pos.x()), toAndroidY(pos.y()));
    }
    
    m_touchStartPos = QPoint(0, 0);
    m_isDragging = false;
}

void PhoneWindow::onBackClicked() { sendAdbKeyEvent(4); }
void PhoneWindow::onHomeClicked() { sendAdbKeyEvent(3); }
void PhoneWindow::onAppsClicked() { sendAdbKeyEvent(187); }

void PhoneWindow::onInstallApkClicked() {
    QString apkPath = QFileDialog::getOpenFileName(
        this, "Select APK File",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
        "APK Files (*.apk);;All Files (*)"
    );
    
    if (!apkPath.isEmpty()) {
        installApk(apkPath);
    }
}

void PhoneWindow::onAppsClicked_Open() {
    AppManagerDialog dialog(m_instanceId, this);
    dialog.exec();
}

void PhoneWindow::onScreenshotClicked() {
    if (!m_screenMirrorActive || !m_screenLabel || m_screenLabel->pixmap().isNull()) {
        QMessageBox::warning(this, "Screenshot", "No screen to capture. Start mirror first.");
        return;
    }
    
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("%1/Screenshot_%2_%3.png").arg(desktopPath).arg(m_instanceId).arg(timestamp);
    
    if (m_screenLabel->pixmap().save(filename)) {
        QMessageBox::information(this, "Screenshot", QString("Screenshot saved to:\n%1").arg(filename));
    } else {
        QMessageBox::warning(this, "Error", "Failed to save screenshot");
    }
}

void PhoneWindow::installApk(const QString& apkPath) {
    QFileInfo apkInfo(apkPath);
    QString apkName = apkInfo.fileName();
    
    m_installProgress = new QProgressDialog(
        QString("Installing %1...").arg(apkName), "Cancel", 0, 100, this);
    m_installProgress->setWindowTitle("Installing APK");
    m_installProgress->setWindowModality(Qt::WindowModal);
    m_installProgress->setValue(10);
    m_installProgress->show();
    
    QString pushPath = "/sdcard/Download/" + apkName;
    QString adbPath = getAdbPath();
    QString serial = getAdbSerial();
    
    // Push APK to device
    QProcess pushProcess;
    pushProcess.start(adbPath, {"-s", serial, "push", apkPath, pushPath});
    pushProcess.waitForFinished(60000);
    
    if (pushProcess.exitCode() != 0) {
        QMessageBox::warning(this, "Error", 
            QString("Failed to push APK to device:\n%1").arg(pushProcess.readAllStandardError()));
        delete m_installProgress;
        m_installProgress = nullptr;
        return;
    }
    
    m_installProgress->setValue(50);
    
    // Install APK
    m_installProcess = new QProcess(this);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneWindow::onInstallFinished);
    
    m_installProcess->start(adbPath, {"-s", serial, "shell", "pm", "install", "-r", pushPath});
}

void PhoneWindow::onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    QString output = m_installProcess->readAllStandardOutput();
    QString error = m_installProcess->readAllStandardError();
    
    if (m_installProgress) {
        m_installProgress->setValue(100);
        m_installProgress->close();
        delete m_installProgress;
        m_installProgress = nullptr;
    }
    
    if (exitCode == 0 && output.contains("Success")) {
        QMessageBox::information(this, "Success", "APK installed successfully!");
    } else {
        QString message = output.isEmpty() ? error : output;
        QMessageBox::warning(this, "Error", QString("Failed to install APK:\n%1").arg(message));
    }
    
    delete m_installProcess;
    m_installProcess = nullptr;
}

void PhoneWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().toLocalFile().endsWith(".apk", Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            m_screenLabel->setStyleSheet("QLabel { background-color: #1a4a1a; border-radius: 16px; border: 3px dashed #27ae60; color: #27ae60; font-size: 14px; }");
            m_screenLabel->setText("\n\n📦\nDrop to install APK");
        }
    }
}

void PhoneWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (filePath.endsWith(".apk", Qt::CaseInsensitive)) {
                m_screenLabel->setStyleSheet("QLabel { background-color: #0a0a15; border-radius: 16px; color: #666; font-size: 14px; }");
                m_screenLabel->setText("\n\n📱\nInstalling APK...\n" + filePath);
                installApk(filePath);
                event->acceptProposedAction();
                return;
            }
        }
    }
    
    m_screenLabel->setStyleSheet("QLabel { background-color: #0a0a15; border-radius: 16px; color: #666; font-size: 14px; }");
    event->ignore();
}

void PhoneWindow::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    
    switch (key) {
        case Qt::Key_Back: case Qt::Key_Escape:
            sendAdbKeyEvent(4); event->accept(); return;
        case Qt::Key_Home:
            sendAdbKeyEvent(3); event->accept(); return;
        case Qt::Key_Menu:
            sendAdbKeyEvent(82); event->accept(); return;
        case Qt::Key_Enter: case Qt::Key_Return:
            sendAdbKeyEvent(66); event->accept(); return;
        case Qt::Key_Backspace: case Qt::Key_Delete:
            sendAdbKeyEvent(67); event->accept(); return;
    }
    
    QString text = event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        sendAdbText(text);
        event->accept();
        return;
    }
    
    QMainWindow::keyPressEvent(event);
}

void PhoneWindow::closeEvent(QCloseEvent* event) {
    stopScreenMirror();
    event->accept();
}

void PhoneWindow::onInstanceStateChanged(const QString& instanceId, InstanceState state) {
    if (instanceId != m_instanceId) return;
    
    if (state == InstanceState::Running) {
        m_statusLabel->setText("Device Ready");
        m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
        QTimer::singleShot(1000, this, &PhoneWindow::startScreenMirror);
    } else if (state == InstanceState::Stopped) {
        stopScreenMirror();
        m_statusLabel->setText("Device Stopped");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
        m_screenLabel->setText("\n\n📱\nDevice stopped\nDrag & drop APK here to install");
    } else if (state == InstanceState::Starting) {
        m_statusLabel->setText("Starting...");
        m_statusLabel->setStyleSheet("color: #f39c12; font-size: 12px;");
    }
}

void PhoneWindow::onAdbConnectionChanged(const QString& instanceId, bool connected) {
    if (instanceId != m_instanceId) return;
    
    if (connected) {
        m_statusLabel->setText("ADB Connected");
        m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
        if (!m_screenMirrorActive) startScreenMirror();
    } else {
        m_statusLabel->setText("ADB Disconnected");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    }
}

void PhoneWindow::updateInstanceCard(const InstanceInfo& info) {
    m_statusLabel->setText(info.adbConnected ? "ADB Connected" : "ADB Disconnected");
}

QString PhoneWindow::getAdbSerial() const {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    return QString("127.0.0.1:%1").arg(info.adbPort - 1);
}

QString PhoneWindow::getAdbPath() const {
    ReDroidController& controller = ReDroidController::instance();
    DockerConfig config = controller.config();
    if (!config.adbPath.isEmpty() && QFile::exists(config.adbPath)) {
        return config.adbPath;
    }
    return "adb";
}

int PhoneWindow::toAndroidX(int labelX) const { return labelX * 1080 / m_screenLabel->width(); }
int PhoneWindow::toAndroidY(int labelY) const { return labelY * 1920 / m_screenLabel->height(); }

void PhoneWindow::sendAdbTap(int x, int y) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QProcess* p = new QProcess(this);
    p->start(getAdbPath(), {"-s", getAdbSerial(), "shell", "input", "tap", QString::number(x), QString::number(y)});
    p->waitForFinished(1000);
    p->deleteLater();
}

void PhoneWindow::sendAdbSwipe(int x1, int y1, int x2, int y2, int duration) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QProcess* p = new QProcess(this);
    p->start(getAdbPath(), {"-s", getAdbSerial(), "shell", "input", "swipe",
        QString::number(x1), QString::number(y1), QString::number(x2), QString::number(y2), QString::number(duration)});
    p->waitForFinished(1000);
    p->deleteLater();
}

void PhoneWindow::sendAdbKeyEvent(int keyCode) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QProcess* p = new QProcess(this);
    p->start(getAdbPath(), {"-s", getAdbSerial(), "shell", "input", "keyevent", QString::number(keyCode)});
    p->waitForFinished(1000);
    p->deleteLater();
}

void PhoneWindow::sendAdbText(const QString& text) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    if (info.state != InstanceState::Running || !info.adbConnected || text.isEmpty()) return;
    
    QString escText = text;
    escText.replace(" ", "%s").replace("'", "'\\''");
    
    QProcess* p = new QProcess(this);
    p->start(getAdbPath(), {"-s", getAdbSerial(), "shell", "input", "text", escText});
    p->waitForFinished(1000);
    p->deleteLater();
}

} // namespace VirtualPhonePro
