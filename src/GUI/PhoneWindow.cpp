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

namespace VirtualPhonePro {

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
{
    setWindowTitle(QString("%1 - %2").arg(m_instanceId).arg(profile.name));
    setMinimumSize(480, 900);
    resize(480, 920);
    
    // Set window flags for proper multi-window behavior
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    
    setupUI();
    setupScreenMirror();
    setupConnections();
    
    // Auto-start screen mirror
    QTimer::singleShot(500, this, &PhoneWindow::startScreenMirror);
}

PhoneWindow::~PhoneWindow() {
    stopScreenMirror();
    
    if (m_adbScreenProcess) {
        m_adbScreenProcess->kill();
        m_adbScreenProcess->waitForFinished();
        delete m_adbScreenProcess;
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
    
    m_titleLabel = new QLabel(profile.name, this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(m_titleLabel);
    
    m_statusLabel = new QLabel("Connecting...", this);
    m_statusLabel->setStyleSheet("color: #f39c12; font-size: 12px;");
    headerLayout->addWidget(m_statusLabel);
    
    headerLayout->addStretch();
    
    mainLayout->addLayout(headerLayout);
    
    // Phone Screen (scaled for display - 400x700)
    QFrame* screenFrame = new QFrame(this);
    screenFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    screenFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #1a1a2e;"
        "    border-radius: 20px;"
        "    border: 3px solid #3a3a4e;"
        "}"
    );
    
    QVBoxLayout* screenLayout = new QVBoxLayout(screenFrame);
    screenLayout->setContentsMargins(5, 5, 5, 5);
    
    m_screenLabel = new QLabel(this);
    m_screenLabel->setMinimumSize(400, 700);
    m_screenLabel->setMaximumSize(400, 700);
    m_screenLabel->setAlignment(Qt::AlignCenter);
    m_screenLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #0a0a15;"
        "    border-radius: 16px;"
        "    color: #666;"
        "    font-size: 14px;"
        "}"
    );
    m_screenLabel->setText("\n\n📱\nConnecting to device...");
    m_screenLabel->setWordWrap(true);
    
    // Enable mouse tracking for touch input
    m_screenLabel->setMouseTracking(true);
    m_screenLabel->installEventFilter(this);
    
    screenLayout->addWidget(m_screenLabel, 0, Qt::AlignCenter);
    
    mainLayout->addWidget(screenFrame, 0, Qt::AlignCenter);
    
    // Hardware Buttons
    QHBoxLayout* hardwareLayout = new QHBoxLayout();
    
    hardwareLayout->addStretch();
    
    m_backButton = new QPushButton("◀ Back", this);
    m_backButton->setMinimumSize(80, 40);
    m_backButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #34495e;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #4a6278; }"
    );
    connect(m_backButton, &QPushButton::clicked, this, &PhoneWindow::onBackClicked);
    hardwareLayout->addWidget(m_backButton);
    
    m_homeButton = new QPushButton("⬤ Home", this);
    m_homeButton->setMinimumSize(80, 40);
    m_homeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #34495e;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #4a6278; }"
    );
    connect(m_homeButton, &QPushButton::clicked, this, &PhoneWindow::onHomeClicked);
    hardwareLayout->addWidget(m_homeButton);
    
    m_appsButton = new QPushButton("▣ Apps", this);
    m_appsButton->setMinimumSize(80, 40);
    m_appsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #34495e;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #4a6278; }"
    );
    connect(m_appsButton, &QPushButton::clicked, this, &PhoneWindow::onAppsClicked);
    hardwareLayout->addWidget(m_appsButton);
    
    m_powerButton = new QPushButton("⏻", this);
    m_powerButton->setMinimumSize(50, 40);
    m_powerButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #34495e;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #4a6278; }"
    );
    connect(m_powerButton, &QPushButton::clicked, this, &PhoneWindow::onHomeClicked);
    hardwareLayout->addWidget(m_powerButton);
    
    hardwareLayout->addStretch();
    
    mainLayout->addLayout(hardwareLayout);
    
    // Mirror Controls
    QHBoxLayout* mirrorLayout = new QHBoxLayout();
    
    mirrorLayout->addStretch();
    
    m_startMirrorButton = new QPushButton("▶ Start Mirror", this);
    m_startMirrorButton->setEnabled(false);
    m_startMirrorButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2ecc71; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_startMirrorButton, &QPushButton::clicked, this, &PhoneWindow::startScreenMirror);
    mirrorLayout->addWidget(m_startMirrorButton);
    
    m_stopMirrorButton = new QPushButton("⏹ Stop Mirror", this);
    m_stopMirrorButton->setEnabled(false);
    m_stopMirrorButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #c0392b; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_stopMirrorButton, &QPushButton::clicked, this, &PhoneWindow::stopScreenMirror);
    mirrorLayout->addWidget(m_stopMirrorButton);
    
    mirrorLayout->addStretch();
    
    mainLayout->addLayout(mirrorLayout);
    
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
    
    mainLayout->addWidget(infoGroup);
    
    // Set focus policy
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void PhoneWindow::setupScreenMirror() {
    // Create ADB screen process
    m_adbScreenProcess = new QProcess(this);
    m_adbScreenProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    // Connect process signals
    connect(m_adbScreenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneWindow::onScreenProcessFinished);
    
    // Create screen timer (100ms interval for ~10fps)
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
    
    qDebug() << "[PhoneWindow" << m_instanceId << "] Starting mirror with serial:" << adbSerial;
    
    QStringList args = {
        "-s", adbSerial,
        "exec-out",
        "screencap",
        "-p"
    };
    
    m_adbScreenProcess->start(adbPath, args);
    
    if (m_adbScreenProcess->waitForStarted(1000)) {
        m_screenMirrorActive = true;
        m_screenTimer->start(100);
        
        m_startMirrorButton->setEnabled(false);
        m_stopMirrorButton->setEnabled(true);
        m_statusLabel->setText("Mirror Active");
        m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
        
        qDebug() << "[PhoneWindow" << m_instanceId << "] Mirror started successfully";
    } else {
        m_statusLabel->setText("Failed to start mirror");
        qWarning() << "[PhoneWindow" << m_instanceId << "] Failed to start ADB process";
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
        m_screenLabel->setText("\n\n📱\nMirror stopped\nClick 'Start Mirror' to continue");
    }
    
    qDebug() << "[PhoneWindow" << m_instanceId << "] Mirror stopped";
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
    
    // Read available data from process
    QByteArray data = m_adbScreenProcess->readAllStandardOutput();
    
    if (data.isEmpty()) return;
    
    // Append to buffer
    m_screenBuffer.append(data);
    
    // Try to find complete PNG image
    static const QByteArray pngSignature("\x89PNG\r\n\x1A\n", 8);
    static const QByteArray pngEnd("\x49\x45\x4E\x44\xAE\x42\x60\x82", 8);
    
    int startPos = m_screenBuffer.indexOf(pngSignature);
    if (startPos == -1) {
        if (m_screenBuffer.size() > 10 * 1024 * 1024) {
            m_screenBuffer.clear();
        }
        return;
    }
    
    int endPos = m_screenBuffer.indexOf(pngEnd, startPos);
    if (endPos == -1) return;
    
    endPos += 8;
    QByteArray pngData = m_screenBuffer.mid(startPos, endPos - startPos);
    m_screenBuffer.remove(0, endPos);
    
    // Convert to QPixmap and display
    QPixmap pixmap;
    if (pixmap.loadFromData(pngData, "PNG")) {
        QPixmap scaled = pixmap.scaled(
            QSize(400, 700),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        m_screenLabel->setPixmap(scaled);
    }
}

void PhoneWindow::onScreenProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    
    if (exitStatus == QProcess::CrashExit) {
        qWarning() << "[PhoneWindow" << m_instanceId << "] ADB process crashed";
        
        if (m_screenMirrorActive) {
            QTimer::singleShot(100, this, &PhoneWindow::startScreenMirror);
        }
    }
}

void PhoneWindow::onScreenMousePress(QMouseEvent* event) {
    QPoint pos = event->pos();
    
    if (!m_screenLabel->rect().contains(pos)) return;
    
    m_touchStartPos = pos;
    m_isDragging = false;
    
    qDebug() << "[PhoneWindow" << m_instanceId << "] Touch press at" << pos;
}

void PhoneWindow::onScreenMouseMove(QMouseEvent* event) {
    QPoint pos = event->pos();
    
    if (m_touchStartPos == QPoint(0, 0)) return;
    
    // Detect drag (moved more than 10 pixels)
    if ((pos - m_touchStartPos).manhattanLength() > 10) {
        m_isDragging = true;
    }
}

void PhoneWindow::onScreenMouseRelease(QMouseEvent* event) {
    QPoint pos = event->pos();
    
    if (m_isDragging) {
        // Perform swipe
        int startX = toAndroidX(m_touchStartPos.x());
        int startY = toAndroidY(m_touchStartPos.y());
        int endX = toAndroidX(pos.x());
        int endY = toAndroidY(pos.y());
        
        sendAdbSwipe(startX, startY, endX, endY, 300);
        qDebug() << "[PhoneWindow" << m_instanceId << "] Swipe:" << startX << "," << startY << "->" << endX << "," << endY;
    } else {
        // Perform tap
        int androidX = toAndroidX(pos.x());
        int androidY = toAndroidY(pos.y());
        
        sendAdbTap(androidX, androidY);
        qDebug() << "[PhoneWindow" << m_instanceId << "] Tap:" << androidX << "," << androidY;
    }
    
    m_touchStartPos = QPoint(0, 0);
    m_isDragging = false;
}

void PhoneWindow::onBackClicked() {
    sendAdbKeyEvent(4); // KEYCODE_BACK
    qDebug() << "[PhoneWindow" << m_instanceId << "] Back button clicked";
}

void PhoneWindow::onHomeClicked() {
    sendAdbKeyEvent(3); // KEYCODE_HOME
    qDebug() << "[PhoneWindow" << m_instanceId << "] Home button clicked";
}

void PhoneWindow::onAppsClicked() {
    sendAdbKeyEvent(187); // KEYCODE_APP_SWITCH
    qDebug() << "[PhoneWindow" << m_instanceId << "] Apps button clicked";
}

void PhoneWindow::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    
    // Special keys mapping
    switch (key) {
        case Qt::Key_Back:
        case Qt::Key_Escape:
            sendAdbKeyEvent(4);
            event->accept();
            return;
        case Qt::Key_Home:
            sendAdbKeyEvent(3);
            event->accept();
            return;
        case Qt::Key_Menu:
            sendAdbKeyEvent(82);
            event->accept();
            return;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            sendAdbKeyEvent(66);
            event->accept();
            return;
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
            sendAdbKeyEvent(67);
            event->accept();
            return;
        default:
            break;
    }
    
    // Regular text input
    QString text = event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        sendAdbText(text);
        qDebug() << "[PhoneWindow" << m_instanceId << "] Text input:" << text;
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
        
        // Auto-start mirror after a short delay
        QTimer::singleShot(1000, this, &PhoneWindow::startScreenMirror);
    } else if (state == InstanceState::Stopped) {
        stopScreenMirror();
        m_statusLabel->setText("Device Stopped");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
        m_screenLabel->setText("\n\n📱\nDevice stopped");
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
        
        if (!m_screenMirrorActive) {
            startScreenMirror();
        }
    } else {
        m_statusLabel->setText("ADB Disconnected");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    }
}

void PhoneWindow::updateInstanceCard(const InstanceInfo& info) {
    // Update UI based on instance info
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

int PhoneWindow::toAndroidX(int labelX) const {
    return labelX * 1080 / m_screenLabel->width();
}

int PhoneWindow::toAndroidY(int labelY) const {
    return labelY * 1920 / m_screenLabel->height();
}

void PhoneWindow::sendAdbTap(int x, int y) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QString adbPath = getAdbPath();
    QString adbSerial = getAdbSerial();
    
    QProcess* process = new QProcess(this);
    QStringList args = {
        "-s", adbSerial,
        "shell", "input", "tap",
        QString::number(x), QString::number(y)
    };
    
    process->start(adbPath, args);
    process->waitForFinished(1000);
    process->deleteLater();
}

void PhoneWindow::sendAdbSwipe(int x1, int y1, int x2, int y2, int duration) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QString adbPath = getAdbPath();
    QString adbSerial = getAdbSerial();
    
    QProcess* process = new QProcess(this);
    QStringList args = {
        "-s", adbSerial,
        "shell", "input", "swipe",
        QString::number(x1), QString::number(y1),
        QString::number(x2), QString::number(y2),
        QString::number(duration)
    };
    
    process->start(adbPath, args);
    process->waitForFinished(1000);
    process->deleteLater();
}

void PhoneWindow::sendAdbKeyEvent(int keyCode) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state != InstanceState::Running || !info.adbConnected) return;
    
    QString adbPath = getAdbPath();
    QString adbSerial = getAdbSerial();
    
    QProcess* process = new QProcess(this);
    QStringList args = {
        "-s", adbSerial,
        "shell", "input", "keyevent",
        QString::number(keyCode)
    };
    
    process->start(adbPath, args);
    process->waitForFinished(1000);
    process->deleteLater();
}

void PhoneWindow::sendAdbText(const QString& text) {
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    if (info.state != InstanceState::Running || !info.adbConnected || text.isEmpty()) return;
    
    QString adbPath = getAdbPath();
    QString adbSerial = getAdbSerial();
    
    // Escape special characters
    QString escapedText = text;
    escapedText.replace(" ", "%s");
    escapedText.replace("'", "'\\''");
    
    QProcess* process = new QProcess(this);
    QStringList args = {
        "-s", adbSerial,
        "shell", "input", "text", escapedText
    };
    
    process->start(adbPath, args);
    process->waitForFinished(1000);
    process->deleteLater();
}

} // namespace VirtualPhonePro
