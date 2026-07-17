#include "DashboardWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace VirtualPhonePro {

// ==============================================================================
// NewPhoneDialog Implementation
// ==============================================================================

NewPhoneDialog::NewPhoneDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create New Phone");
    setModal(true);
    setMinimumWidth(450);
    
    setupUI();
    loadManufacturerModels();
}

NewPhoneDialog::~NewPhoneDialog() {}

void NewPhoneDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Basic Info
    QGroupBox* basicGroup = new QGroupBox("Basic Information", this);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);
    
    m_instanceNameEdit = new QLineEdit(this);
    m_instanceNameEdit->setPlaceholderText("e.g., Phone-1");
    basicLayout->addRow("Instance Name:", m_instanceNameEdit);
    
    m_manufacturerCombo = new QComboBox(this);
    connect(m_manufacturerCombo, &QComboBox::currentTextChanged,
            this, &NewPhoneDialog::onManufacturerChanged);
    basicLayout->addRow("Manufacturer:", m_manufacturerCombo);
    
    m_modelCombo = new QComboBox(this);
    basicLayout->addRow("Model:", m_modelCombo);
    
    m_androidVersionCombo = new QComboBox(this);
    m_androidVersionCombo->addItems({"14", "15", "16"});
    m_androidVersionCombo->setCurrentText("14");
    basicLayout->addRow("Android Version:", m_androidVersionCombo);
    
    m_memorySpin = new QSpinBox(this);
    m_memorySpin->setRange(256, 4096);
    m_memorySpin->setSuffix(" MB");
    m_memorySpin->setValue(512);
    basicLayout->addRow("Memory:", m_memorySpin);
    
    mainLayout->addWidget(basicGroup);
    
    // Identity Info
    QGroupBox* identityGroup = new QGroupBox("Device Identity (Auto-generated)", this);
    QFormLayout* identityLayout = new QFormLayout(identityGroup);
    
    m_imeiEdit = new QLineEdit(this);
    m_imeiEdit->setReadOnly(true);
    identityLayout->addRow("IMEI:", m_imeiEdit);
    
    m_serialEdit = new QLineEdit(this);
    m_serialEdit->setReadOnly(true);
    identityLayout->addRow("Serial:", m_serialEdit);
    
    m_androidIdEdit = new QLineEdit(this);
    m_androidIdEdit->setReadOnly(true);
    identityLayout->addRow("Android ID:", m_androidIdEdit);
    
    QPushButton* randomizeBtn = new QPushButton("🎲 Randomize", this);
    connect(randomizeBtn, &QPushButton::clicked, this, &NewPhoneDialog::onRandomizeProfile);
    identityLayout->addRow("", randomizeBtn);
    
    mainLayout->addWidget(identityGroup);
    
    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &NewPhoneDialog::onOk);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    // Generate initial values
    onRandomizeProfile();
}

void NewPhoneDialog::loadManufacturerModels() {
    m_manufacturerCombo->addItems({
        "Samsung", "Google", "Xiaomi", "OnePlus", "Huawei", "OPPO", "Vivo", "Realme"
    });
}

void NewPhoneDialog::onManufacturerChanged(const QString& manufacturer) {
    m_manufacturer = manufacturer;
    m_modelCombo->clear();
    
    if (manufacturer == "Samsung") {
        m_modelCombo->addItems({"Galaxy S24 Ultra", "Galaxy S23", "Galaxy A54", "Galaxy Z Fold 5"});
    } else if (manufacturer == "Google") {
        m_modelCombo->addItems({"Pixel 8 Pro", "Pixel 8", "Pixel 7 Pro", "Pixel 7a"});
    } else if (manufacturer == "Xiaomi") {
        m_modelCombo->addItems({"Xiaomi 14", "Redmi Note 13", "POCO F5", "Xiaomi 13 Ultra"});
    } else if (manufacturer == "OnePlus") {
        m_modelCombo->addItems({"OnePlus 12", "OnePlus 11", "OnePlus Nord 3", "OnePlus 10T"});
    } else if (manufacturer == "Huawei") {
        m_modelCombo->addItems({"P60 Pro", "Mate 60 Pro", "Nova 11", "P50 Pro"});
    } else if (manufacturer == "OPPO") {
        m_modelCombo->addItems({"Find X7 Ultra", "Reno 11", "Find N3", "A78"});
    } else if (manufacturer == "Vivo") {
        m_modelCombo->addItems({"X100 Pro", "X90", "V30 Pro", "Y100"});
    } else if (manufacturer == "Realme") {
        m_modelCombo->addItems({"GT 5 Pro", "Realme 11 Pro+", "C55", "GT Neo 5"});
    }
    
    if (m_modelCombo->count() > 0) {
        m_modelCombo->setCurrentIndex(0);
    }
}

void NewPhoneDialog::onRandomizeProfile() {
    m_manufacturer = m_manufacturerCombo->currentText();
    m_androidVersion = m_androidVersionCombo->currentText();
    
    // Generate unique IMEI
    QStringList samsungTacs = {"35875107", "35746608", "35746609"};
    QString tac = samsungTacs[QRandomGenerator::global()->bounded(samsungTacs.size())];
    for (int i = 0; i < 6; i++) tac += QString::number(QRandomGenerator::global()->bounded(10));
    
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
    
    // Generate serial
    m_serialEdit->setText("R" + QString::number(100000 + QRandomGenerator::global()->bounded(900000)) + "X78");
    
    // Generate Android ID
    QString hexChars = "0123456789ABCDEF";
    QString androidId;
    for (int i = 0; i < 16; i++) androidId += hexChars[QRandomGenerator::global()->bounded(16)];
    m_androidIdEdit->setText(androidId);
}

void NewPhoneDialog::onOk() {
    QString name = m_instanceNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please enter an instance name");
        return;
    }
    
    m_instanceId = name;
    m_manufacturer = m_manufacturerCombo->currentText();
    m_androidVersion = m_androidVersionCombo->currentText();
    
    // Create profile
    m_profile = DeviceProfile();
    m_profile.name = m_manufacturer + " " + m_modelCombo->currentText();
    m_profile.manufacturer = m_manufacturer;
    m_profile.build.brand = m_manufacturer.toLower();
    m_profile.build.manufacturer = m_manufacturer;
    m_profile.build.model = m_modelCombo->currentText().replace(" ", "_");
    m_profile.build.androidVersion = m_androidVersion.toInt();
    m_profile.identity.imei = m_imeiEdit->text();
    m_profile.identity.serialNumber = m_serialEdit->text();
    m_profile.identity.androidId = m_androidIdEdit->text();
    
    accept();
}

// ==============================================================================
// PhoneCard Implementation
// ==============================================================================

PhoneCard::PhoneCard(const QString& instanceId, QWidget* parent)
    : QFrame(parent)
    , m_instanceId(instanceId)
{
    setMinimumSize(280, 380);
    setMaximumSize(320, 420);
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setStyleSheet(
        "PhoneCard {"
        "    background-color: #2a2a3e;"
        "    border-radius: 12px;"
        "    border: 1px solid #3a3a4e;"
        "}"
    );
    
    setupUI();
}

PhoneCard::~PhoneCard() {}

void PhoneCard::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    
    // Screenshot preview
    m_screenshotLabel = new QLabel(this);
    m_screenshotLabel->setMinimumSize(256, 448);
    m_screenshotLabel->setMaximumSize(256, 448);
    m_screenshotLabel->setAlignment(Qt::AlignCenter);
    m_screenshotLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #1a1a2e;"
        "    border-radius: 8px;"
        "    border: 2px solid #3a3a4e;"
        "}"
    );
    m_screenshotLabel->setText("\n\n📱\nNo Screenshot");
    
    layout->addWidget(m_screenshotLabel, 0, Qt::AlignCenter);
    
    // Name and status
    m_nameLabel = new QLabel(m_instanceId, this);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");
    layout->addWidget(m_nameLabel);
    
    // Model label
    m_modelLabel = new QLabel("Samsung Galaxy S24", this);
    m_modelLabel->setStyleSheet("font-size: 11px; color: #888888;");
    layout->addWidget(m_modelLabel);
    
    // Status row
    QHBoxLayout* statusLayout = new QHBoxLayout();
    
    m_statusLabel = new QLabel("Stopped", this);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    padding: 4px 8px;"
        "    background-color: #444;"
        "    border-radius: 4px;"
        "    color: #ff6b6b;"
        "    font-size: 11px;"
        "}"
    );
    statusLayout->addWidget(m_statusLabel);
    
    m_portLabel = new QLabel("Port: 5555", this);
    m_portLabel->setStyleSheet("color: #888888; font-size: 11px;");
    statusLayout->addWidget(m_portLabel);
    statusLayout->addStretch();
    
    layout->addLayout(statusLayout);
    
    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton("▶ Start", this);
    m_startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2ecc71; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_startButton, &QPushButton::clicked, this, &PhoneCard::onStartClicked);
    buttonLayout->addWidget(m_startButton);
    
    m_stopButton = new QPushButton("⏹", this);
    m_stopButton->setEnabled(false);
    m_stopButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #c0392b; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_stopButton, &QPushButton::clicked, this, &PhoneCard::onStopClicked);
    buttonLayout->addWidget(m_stopButton);
    
    layout->addLayout(buttonLayout);
    
    // Second row
    QHBoxLayout* buttonLayout2 = new QHBoxLayout();
    
    m_openButton = new QPushButton("📱 Open", this);
    m_openButton->setEnabled(false);
    m_openButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_openButton, &QPushButton::clicked, this, &PhoneCard::onOpenClicked);
    buttonLayout2->addWidget(m_openButton);
    
    m_deleteButton = new QPushButton("🗑", this);
    m_deleteButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #666;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #888; }"
    );
    connect(m_deleteButton, &QPushButton::clicked, this, &PhoneCard::onDeleteClicked);
    buttonLayout2->addWidget(m_deleteButton);
    
    layout->addLayout(buttonLayout2);
}

void PhoneCard::setInstanceInfo(const InstanceInfo& info) {
    m_info = info;
    updateUI();
}

void PhoneCard::setProfile(const DeviceProfile& profile) {
    m_profile = profile;
    m_modelLabel->setText(profile.name);
}

void PhoneCard::setScreenshot(const QPixmap& pixmap) {
    QPixmap scaled = pixmap.scaled(256, 448, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_screenshotLabel->setPixmap(scaled);
}

void PhoneCard::updateStatus() {
    updateUI();
}

void PhoneCard::updateUI() {
    QString stateText;
    QString stateColor;
    
    switch (m_info.state) {
        case InstanceState::Running:
            stateText = "Running";
            stateColor = "#27ae60";
            m_startButton->setEnabled(false);
            m_stopButton->setEnabled(true);
            m_openButton->setEnabled(m_info.adbConnected);
            break;
        case InstanceState::Starting:
            stateText = "Starting...";
            stateColor = "#f39c12";
            m_startButton->setEnabled(false);
            m_stopButton->setEnabled(false);
            m_openButton->setEnabled(false);
            break;
        case InstanceState::Paused:
            stateText = "Paused";
            stateColor = "#9b59b6";
            m_startButton->setEnabled(true);
            m_stopButton->setEnabled(true);
            m_openButton->setEnabled(false);
            break;
        case InstanceState::Error:
            stateText = "Error";
            stateColor = "#e74c3c";
            m_startButton->setEnabled(true);
            m_stopButton->setEnabled(false);
            m_openButton->setEnabled(false);
            break;
        default:
            stateText = "Stopped";
            stateColor = "#ff6b6b";
            m_startButton->setEnabled(true);
            m_stopButton->setEnabled(false);
            m_openButton->setEnabled(false);
            break;
    }
    
    m_statusLabel->setText(stateText);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    padding: 4px 8px;"
        "    background-color: " + stateColor + ";"
        "    border-radius: 4px;"
        "    color: white;"
        "    font-size: 11px;"
        "}"
    );
    
    m_portLabel->setText("Port: " + QString::number(m_info.adbPort));
}

void PhoneCard::onOpenClicked() {
    emit openRequested(m_instanceId);
}

void PhoneCard::onStartClicked() {
    emit startRequested(m_instanceId);
}

void PhoneCard::onStopClicked() {
    emit stopRequested(m_instanceId);
}

void PhoneCard::onDeleteClicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Instance",
        QString("Are you sure you want to delete '%1'?").arg(m_instanceId),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        emit deleteRequested(m_instanceId);
    }
}

// ==============================================================================
// DashboardWindow Implementation
// ==============================================================================

DashboardWindow::DashboardWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("VirtualPhonePro - Multi-Instance Dashboard");
    setMinimumSize(1024, 768);
    resize(1280, 800);
    
    setupUI();
    setupMenuBar();
    setupConnections();
    
    refreshInstances();
}

DashboardWindow::~DashboardWindow() {
    // Close all phone windows
    for (PhoneWindow* window : m_phoneWindows.values()) {
        if (window) {
            window->close();
            delete window;
        }
    }
}

void DashboardWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("📱 Multi-Instance Dashboard", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    // Status
    m_statusLabel = new QLabel("Loading...", this);
    m_statusLabel->setStyleSheet("color: #888888;");
    headerLayout->addWidget(m_statusLabel);
    
    mainLayout->addLayout(headerLayout);
    
    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    
    m_newPhoneButton = new QPushButton("➕ New Phone", this);
    m_newPhoneButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    connect(m_newPhoneButton, &QPushButton::clicked, this, &DashboardWindow::onNewPhoneClicked);
    toolbarLayout->addWidget(m_newPhoneButton);
    
    m_refreshButton = new QPushButton("🔄 Refresh", this);
    m_refreshButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(m_refreshButton, &QPushButton::clicked, this, &DashboardWindow::onRefreshClicked);
    toolbarLayout->addWidget(m_refreshButton);
    
    toolbarLayout->addStretch();
    
    mainLayout->addLayout(toolbarLayout);
    
    // Scroll area with phone cards
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
    );
    
    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet("background-color: transparent;");
    
    m_cardGrid = new QGridLayout(m_scrollContent);
    m_cardGrid->setSpacing(20);
    m_cardGrid->setContentsMargins(10, 10, 10, 10);
    
    QVBoxLayout* scrollLayout = new QVBoxLayout(m_scrollContent);
    scrollLayout->addLayout(m_cardGrid);
    scrollLayout->addStretch();
    
    m_scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(m_scrollArea);
    
    // Screenshot refresh timer
    m_screenshotTimer = new QTimer(this);
    connect(m_screenshotTimer, &QTimer::timeout, this, &DashboardWindow::onRefreshScreenshots);
    m_screenshotTimer->start(5000); // Refresh every 5 seconds
}

void DashboardWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    menuBar->setStyleSheet("background-color: #1a1a2e; color: white;");
    
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* newAction = new QAction("&New Phone Instance", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &DashboardWindow::onNewPhoneClicked);
    fileMenu->addAction(newAction);
    
    fileMenu->addSeparator();
    
    QAction* refreshAction = new QAction("&Refresh", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &DashboardWindow::onRefreshClicked);
    fileMenu->addAction(refreshAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    fileMenu->addAction(exitAction);
    
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, "About VirtualPhonePro",
            "VirtualPhonePro v3.0.0\n"
            "Multi-Instance Android Emulator Manager\n"
            "\n"
            "Features:\n"
            "• 98%+ Detection Avoidance\n"
            "• 40+ Anti-Detection Modules\n"
            "• Multi-Instance Support\n"
            "• Qt6 GUI Interface"
        );
    });
    helpMenu->addAction(aboutAction);
}

void DashboardWindow::setupConnections() {
    ReDroidController& controller = ReDroidController::instance();
    
    connect(&controller, &ReDroidController::instanceStateChanged,
            this, &DashboardWindow::updateInstanceCard);
    connect(&controller, &ReDroidController::adbConnectionChanged,
            this, &DashboardWindow::onAdbConnectionChanged);
}

void DashboardWindow::refreshInstances() {
    ReDroidController& controller = ReDroidController::instance();
    QList<InstanceInfo> instances = controller.listInstances();
    
    // Update status
    int running = 0;
    for (const InstanceInfo& info : instances) {
        if (info.state == InstanceState::Running) running++;
    }
    m_statusLabel->setText(
        QString("Total: %1 | Running: %2 | Available Slots: %3")
            .arg(instances.size())
            .arg(running)
            .arg(10 - running)
    );
    
    // Create cards for new instances
    for (const InstanceInfo& info : instances) {
        if (!m_phoneCards.contains(info.instanceId)) {
            createPhoneCard(info.instanceId);
        }
        m_phoneCards[info.instanceId]->setInstanceInfo(info);
    }
    
    // Remove cards for deleted instances
    QStringList currentIds;
    for (const InstanceInfo& info : instances) {
        currentIds.append(info.instanceId);
    }
    
    for (const QString& cardId : m_phoneCards.keys()) {
        if (!currentIds.contains(cardId)) {
            removePhoneCard(cardId);
        }
    }
}

void DashboardWindow::onNewPhoneClicked() {
    NewPhoneDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString instanceId = dialog.getInstanceId();
        DeviceProfile profile = dialog.getProfile();
        
        // Get available port
        MultiInstanceManager& multiManager = MultiInstanceManager::instance();
        int port = multiManager.findAvailablePort();
        
        // Set ports
        profile.adbPort = port;
        profile.vncPort = port + 1;
        
        // Start instance
        ReDroidController& controller = ReDroidController::instance();
        
        if (controller.startInstance(instanceId, profile)) {
            // Apply anti-detection
            BankingAppSpoofer::instance().applyCompleteBankingSetup(instanceId);
            SafetyNetSpoofer::instance().spoofSafetyNet(instanceId);
            
            // Refresh and create card
            refreshInstances();
            
            QMessageBox::information(this, "Success",
                QString("Phone '%1' created successfully!\n"
                        "ADB Port: %2\n"
                        "Profile: %3")
                    .arg(instanceId)
                    .arg(port)
                    .arg(profile.name)
            );
        } else {
            QMessageBox::warning(this, "Error",
                QString("Failed to create phone '%1'").arg(instanceId)
            );
        }
    }
}

void DashboardWindow::createPhoneCard(const QString& instanceId) {
    PhoneCard* card = new PhoneCard(instanceId, m_scrollContent);
    
    // Load profile if exists
    QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    profilePath += "/profiles/" + instanceId + ".json";
    DeviceProfile profile = DeviceProfile::load(profilePath);
    
    if (!profile.id.isEmpty()) {
        card->setProfile(profile);
    }
    
    // Connect signals
    connect(card, &PhoneCard::openRequested, this, &DashboardWindow::onPhoneCardOpen);
    connect(card, &PhoneCard::startRequested, this, &DashboardWindow::onPhoneCardStart);
    connect(card, &PhoneCard::stopRequested, this, &DashboardWindow::onPhoneCardStop);
    connect(card, &PhoneCard::deleteRequested, this, &DashboardWindow::onPhoneCardDelete);
    
    // Add to grid
    int row = m_cardGrid->rowCount();
    int col = m_cardGrid->columnCount();
    if (col >= 4) {
        col = 0;
        row++;
    }
    m_cardGrid->addWidget(card, row, col);
    
    m_phoneCards[instanceId] = card;
}

void DashboardWindow::removePhoneCard(const QString& instanceId) {
    if (m_phoneCards.contains(instanceId)) {
        PhoneCard* card = m_phoneCards[instanceId];
        m_cardGrid->removeWidget(card);
        delete card;
        m_phoneCards.remove(instanceId);
    }
    
    // Close phone window if open
    if (m_phoneWindows.contains(instanceId)) {
        PhoneWindow* window = m_phoneWindows[instanceId];
        window->close();
        delete window;
        m_phoneWindows.remove(instanceId);
    }
}

void DashboardWindow::openPhoneWindow(const QString& instanceId) {
    if (m_phoneWindows.contains(instanceId)) {
        // Bring to front
        PhoneWindow* window = m_phoneWindows[instanceId];
        window->show();
        window->raise();
        window->activateWindow();
        return;
    }
    
    // Load profile
    QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    profilePath += "/profiles/" + instanceId + ".json";
    DeviceProfile profile = DeviceProfile::load(profilePath);
    
    if (profile.id.isEmpty()) {
        profile = DeviceProfile::createSamsungS24Ultra();
    }
    
    // Create new phone window
    PhoneWindow* window = new PhoneWindow(instanceId, profile);
    window->show();
    
    m_phoneWindows[instanceId] = window;
    
    // Connect close signal
    connect(window, &QWidget::destroyed, this, [this, instanceId]() {
        m_phoneWindows.remove(instanceId);
    });
}

void DashboardWindow::onPhoneCardOpen(const QString& instanceId) {
    openPhoneWindow(instanceId);
}

void DashboardWindow::onPhoneCardStart(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    
    // Load profile
    QString profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    profilePath += "/profiles/" + instanceId + ".json";
    DeviceProfile profile = DeviceProfile::load(profilePath);
    
    if (profile.id.isEmpty()) {
        profile = DeviceProfile::createSamsungS24Ultra();
    }
    
    if (controller.startInstance(instanceId, profile)) {
        // Apply anti-detection
        BankingAppSpoofer::instance().applyCompleteBankingSetup(instanceId);
        SafetyNetSpoofer::instance().spoofSafetyNet(instanceId);
    }
    
    refreshInstances();
}

void DashboardWindow::onPhoneCardStop(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    controller.stopInstance(instanceId);
    
    // Stop screen mirror if window is open
    if (m_phoneWindows.contains(instanceId)) {
        m_phoneWindows[instanceId]->stopScreenMirror();
    }
    
    refreshInstances();
}

void DashboardWindow::onPhoneCardDelete(const QString& instanceId) {
    ReDroidController& controller = ReDroidController::instance();
    controller.deleteInstance(instanceId);
    
    removePhoneCard(instanceId);
    refreshInstances();
}

void DashboardWindow::onRefreshClicked() {
    refreshInstances();
}

void DashboardWindow::onRefreshScreenshots() {
    // Refresh screenshots for running instances with open windows
    for (auto it = m_phoneWindows.begin(); it != m_phoneWindows.end(); ++it) {
        if (it.value()) {
            it.value()->refreshInstance();
        }
    }
}

void DashboardWindow::updateInstanceCard(const QString& instanceId, InstanceState state) {
    if (m_phoneCards.contains(instanceId)) {
        ReDroidController& controller = ReDroidController::instance();
        InstanceInfo info = controller.getInstanceInfo(instanceId);
        m_phoneCards[instanceId]->setInstanceInfo(info);
    }
    
    // Also update phone window if open
    if (m_phoneWindows.contains(instanceId)) {
        m_phoneWindows[instanceId]->onInstanceStateChanged(instanceId, state);
    }
}

void DashboardWindow::onAdbConnectionChanged(const QString& instanceId, bool connected) {
    if (m_phoneCards.contains(instanceId)) {
        ReDroidController& controller = ReDroidController::instance();
        InstanceInfo info = controller.getInstanceInfo(instanceId);
        m_phoneCards[instanceId]->setInstanceInfo(info);
    }
    
    if (m_phoneWindows.contains(instanceId)) {
        m_phoneWindows[instanceId]->onAdbConnectionChanged(instanceId, connected);
    }
}

} // namespace VirtualPhonePro
