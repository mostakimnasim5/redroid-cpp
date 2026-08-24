#include "DashboardWindow.hpp"

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
#include <QProgressDialog>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtConcurrent>

#include "VirtualPhonePro/MultiInstanceManager.hpp"
#include "VirtualPhonePro/AppCloner.hpp"
#include "VirtualPhonePro/NetworkConfigManager.hpp"
#include "VirtualPhonePro/ProfileGeneratorFactory.hpp"
#include "SettingsDialog.hpp"

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
    
    // Proxy Configuration
    QGroupBox* proxyGroup = new QGroupBox("🌐 Network Configuration", this);
    QFormLayout* proxyLayout = new QFormLayout(proxyGroup);
    
    m_proxyModeCombo = new QComboBox(this);
    m_proxyModeCombo->addItem("Without Proxy (Random IP)",  0);
    m_proxyModeCombo->addItem("With ISP/Residential Proxy (WiFi)", 1);
    m_proxyModeCombo->addItem("With Mobile Proxy (Same Device IP)", 2);
    proxyLayout->addRow("Network Mode:", m_proxyModeCombo);
    
    // Proxy details container (shown/hidden based on selection)
    m_proxyDetailsWidget = new QWidget(this);
    QFormLayout* proxyDetailsLayout = new QFormLayout(m_proxyDetailsWidget);
    proxyDetailsLayout->setContentsMargins(0, 10, 0, 0);
    
    m_proxyHostEdit = new QLineEdit(this);
    m_proxyHostEdit->setPlaceholderText("e.g., 192.168.1.100 or proxy.example.com");
    proxyDetailsLayout->addRow("Proxy Host:", m_proxyHostEdit);
    
    m_proxyPortSpin = new QSpinBox(this);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(8080);
    proxyDetailsLayout->addRow("Proxy Port:", m_proxyPortSpin);
    
    m_proxyUsernameEdit = new QLineEdit(this);
    m_proxyUsernameEdit->setPlaceholderText("Username (optional)");
    proxyDetailsLayout->addRow("Username:", m_proxyUsernameEdit);
    
    m_proxyPasswordEdit = new QLineEdit(this);
    m_proxyPasswordEdit->setPlaceholderText("Password (optional)");
    m_proxyPasswordEdit->setEchoMode(QLineEdit::Password);
    proxyDetailsLayout->addRow("Password:", m_proxyPasswordEdit);
    
    proxyLayout->addRow("", m_proxyDetailsWidget);
    
    // Initially hide proxy details
    m_proxyDetailsWidget->setVisible(false);
    
    // Connect proxy mode change
    connect(m_proxyModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewPhoneDialog::onProxyModeChanged);
    
    mainLayout->addWidget(proxyGroup);
    
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

    if (!generateDeterministicIdentity()) {
        QMessageBox::warning(this, "Identity Generation",
            "Could not allocate a unique profile index. "
            "Please check the application's config directory.");
        return;
    }

    m_imeiEdit->setText(QString::fromStdString(m_identity.imei1));
    m_serialEdit->setText(QString::fromStdString(m_identity.serial_number));
    m_androidIdEdit->setText(QString::fromStdString(m_identity.android_id));
}

bool NewPhoneDialog::generateDeterministicIdentity() {
    // Shared allocation path with the batch/multi deploy path: identity is
    // derived from the hardware-anchored deterministic engine
    // (Master_Seed = HMAC-SHA256(HWID + License_Key, "PROFILE_" + Index)),
    // never from process-random sources.
    const HardwareAnchoredIdentity result = generateUniqueHardwareAnchoredIdentity();
    if (!result.ok) {
        return false;
    }
    m_identity = result.identity;
    m_profileIndex = result.profileIndex;
    return true;
}

void NewPhoneDialog::onProxyModeChanged(int index) {
    int mode = m_proxyModeCombo->itemData(index).toInt();
    
    if (mode == 0) {
        // Without Proxy - hide proxy details
        m_proxyDetailsWidget->setVisible(false);
        m_useProxy = false;
    } else if (mode == 1) {
        // With Proxy (Custom IP) - show proxy details
        m_proxyDetailsWidget->setVisible(true);
        m_useProxy = true;
        m_proxyHostEdit->setPlaceholderText("Enter proxy IP or hostname");
    } else if (mode == 2) {
        // With Mobile Proxy (Same Device IP) - proxy details are REQUIRED so
        // the carrier/timezone/locale auto-sync can resolve the exit IP.
        m_proxyDetailsWidget->setVisible(true);
        m_useProxy = true;
        m_proxyHostEdit->setPlaceholderText("Mobile proxy gateway host (required)");
    }
}

void NewPhoneDialog::onOk() {
    QString name = m_instanceNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please enter an instance name");
        return;
    }
    
    // Validate proxy if using proxy mode
    int mode = m_proxyModeCombo->itemData(m_proxyModeCombo->currentIndex()).toInt();
    if (mode == 1 || mode == 2) {
        // Both proxy modes require a proxy host — mode 2 (mobile proxy) must
        // not be assignable with an empty proxy, or auto-sync would resolve
        // against the host IP and produce an inconsistent device identity.
        QString proxyHost = m_proxyHostEdit->text().trimmed();
        if (proxyHost.isEmpty()) {
            QMessageBox::warning(this, "Validation",
                mode == 2
                    ? "Please enter the mobile proxy gateway address (required for this network mode)"
                    : "Please enter a proxy host address");
            return;
        }
    }
    
    m_instanceId = name;
    m_manufacturer = m_manufacturerCombo->currentText();
    m_androidVersion = m_androidVersionCombo->currentText();
    
    // Get proxy settings
    int proxyMode = m_proxyModeCombo->itemData(m_proxyModeCombo->currentIndex()).toInt();
    m_useProxy = (proxyMode != 0);
    m_proxyHost = m_proxyHostEdit->text().trimmed();
    m_proxyPort = m_proxyPortSpin->value();
    m_proxyUsername = m_proxyUsernameEdit->text().trimmed();
    m_proxyPassword = m_proxyPasswordEdit->text();
    
    // Create profile — requires a successfully generated deterministic identity
    if (m_identity.imei1.empty()) {
        QMessageBox::warning(this, "Validation",
            "Device identity was not generated. Please click Randomize and try again.");
        return;
    }

    m_profile = DeviceProfile();
    m_profile.name = m_manufacturer + " " + m_modelCombo->currentText();
    m_profile.manufacturer = m_manufacturer;
    m_profile.build.brand = m_manufacturer.toLower();
    m_profile.build.manufacturer = m_manufacturer;
    m_profile.build.model = m_modelCombo->currentText().replace(" ", "_");
    m_profile.build.androidVersion = m_androidVersion.toInt();

    // Full deterministic identity from the hardware-anchored engine — shared
    // mapping with the batch/multi deploy path (all 20 units, nothing dropped).
    applyIdentityToDeviceProfile(m_profile, m_identity);

    // Record the issued identity so future allocations can detect collisions
    registerIssuedIdentity(m_instanceId, m_profile);

    accept();
}

// ==============================================================================
// PhoneCard Implementation
// ==============================================================================

PhoneCard::PhoneCard(const QString& instanceId, QWidget* parent)
    : QFrame(parent)
    , m_instanceId(instanceId)
    , m_isProtected(false)
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
    
    // Shield icon for protection status
    m_shieldLabel = new QLabel("⚠️", this);
    m_shieldLabel->setStyleSheet("font-size: 16px;");
    m_shieldLabel->setToolTip("Protection: Not Protected");
    layout->addWidget(m_shieldLabel, 0, Qt::AlignRight);
    
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

    // Clone button
    m_cloneButton = new QPushButton("⧉ Clone", this);
    m_cloneButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #8e44ad;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #9b59b6; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_cloneButton, &QPushButton::clicked, this, &PhoneCard::onCloneClicked);
    buttonLayout2->addWidget(m_cloneButton);
    
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

void PhoneCard::setProtectionStatus(bool isProtected) {
    m_isProtected = isProtected;
    updateProtectionIcon();
}

void PhoneCard::updateProtectionIcon() {
    if (m_isProtected) {
        m_shieldLabel->setText("🛡️");
        m_shieldLabel->setStyleSheet("font-size: 16px; color: #27ae60;");
        m_shieldLabel->setToolTip("Protection: Fully Protected");
    } else {
        m_shieldLabel->setText("⚠️");
        m_shieldLabel->setStyleSheet("font-size: 16px; color: #e74c3c;");
        m_shieldLabel->setToolTip("Protection: Not Protected");
    }
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

void PhoneCard::onCloneClicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clone Instance",
        QString("Clone '%1' into a new instance with a fresh identity?\n\n"
                "The new instance will have different IMEI, serial, MAC and AndroidId.")
            .arg(m_instanceId),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        emit cloneRequested(m_instanceId);
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

    // ── Batch Launch button ──────────────────────────────────────────────────
    m_batchLaunchButton = new QPushButton("⚡ Batch Launch", this);
    m_batchLaunchButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e67e22;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #f39c12; }"
        "QPushButton:disabled { background-color: #555; color: #aaa; }"
    );
    connect(m_batchLaunchButton, &QPushButton::clicked,
            this, &DashboardWindow::onBatchLaunchClicked);
    toolbarLayout->addWidget(m_batchLaunchButton);
    
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

    // Install Requirements button — sits immediately to the right of Refresh.
    m_installRequirementsButton = new QPushButton("⬇ Install Requirements", this);
    m_installRequirementsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #8e44ad;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #9b59b6; }"
        "QPushButton:disabled { background-color: #555; color: #aaa; }"
    );
    connect(m_installRequirementsButton, &QPushButton::clicked,
            this, &DashboardWindow::onInstallRequirementsClicked);
    toolbarLayout->addWidget(m_installRequirementsButton);

    // Uninstall button — only shown after install completes.
    m_uninstallRequirementsButton = new QPushButton("🗑 Uninstall", this);
    m_uninstallRequirementsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #c0392b;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #e74c3c; }"
        "QPushButton:disabled { background-color: #555; color: #aaa; }"
    );
    m_uninstallRequirementsButton->hide();
    connect(m_uninstallRequirementsButton, &QPushButton::clicked,
            this, &DashboardWindow::onUninstallRequirementsClicked);
    toolbarLayout->addWidget(m_uninstallRequirementsButton);

    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // Blue progress bar — sits above the card grid, animates left→right
    // while install/uninstall runs. Hidden when idle.
    m_requirementsProgressBar = new QProgressBar(this);
    m_requirementsProgressBar->setRange(0, 100);
    m_requirementsProgressBar->setValue(0);
    m_requirementsProgressBar->setTextVisible(false);
    m_requirementsProgressBar->setFixedHeight(6);
    m_requirementsProgressBar->setStyleSheet(
        "QProgressBar {"
        "    background-color: #1a1a2e;"
        "    border: none;"
        "    border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3498db;"
        "    border-radius: 3px;"
        "}"
    );
    m_requirementsProgressBar->hide();
    mainLayout->addWidget(m_requirementsProgressBar);

    // Requirements manager (lives for the lifetime of the window).
    m_requirementsManager = new RequirementsManager(this);
    connect(m_requirementsManager, &RequirementsManager::logMessage,
            this, &DashboardWindow::onRequirementsLog);
    connect(m_requirementsManager, &RequirementsManager::progress,
            this, &DashboardWindow::onRequirementsProgress);
    connect(m_requirementsManager, &RequirementsManager::finished,
            this, &DashboardWindow::onRequirementsFinished);

    refreshRequirementsState();

    
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

    // QGridLayout is installed directly as m_scrollContent's sole layout.
    // A second QVBoxLayout must NOT be created on the same widget: Qt
    // silently orphans the first layout, making the grid invisible and
    // addStretch() ineffective.  Cards are pushed to the top-left via
    // Qt::AlignTop|AlignLeft so empty rows do not expand to fill height.
    m_cardGrid = new QGridLayout(m_scrollContent);
    m_cardGrid->setSpacing(20);
    m_cardGrid->setContentsMargins(10, 10, 10, 10);
    m_cardGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

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
    
    QAction* settingsAction = new QAction("⚙️ &Settings...", this);
    settingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(settingsAction, &QAction::triggered, [this]() {
        SettingsDialog dialog(this);
        dialog.exec();
    });
    fileMenu->addAction(settingsAction);
    
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
        
        // Start instance
        ReDroidController& controller = ReDroidController::instance();
        
        if (controller.startInstance(instanceId, profile)) {
            // Apply proxy if enabled
            if (dialog.useProxy()) {
                ProxyConfig proxyConfig;
                proxyConfig.host = dialog.getProxyHost();
                proxyConfig.port = dialog.getProxyPort();
                proxyConfig.username = dialog.getProxyUsername();
                proxyConfig.password = dialog.getProxyPassword();
                proxyConfig.type = "http"; // Default to HTTP proxy
                
                if (proxyConfig.isValid()) {
                    // Map the GUI network mode to the access-network kind:
                    // mode 1 (ISP/residential) = WiFi (no SIM/carrier story),
                    // mode 2 (mobile) = Cellular (existing behavior).
                    int mode = dialog.getProxyMode();
                    VirtualPhonePro::SyncNetworkKind kind =
                        (mode == 1) ? VirtualPhonePro::SyncNetworkKind::WiFi
                                    : VirtualPhonePro::SyncNetworkKind::Cellular;
                    qDebug() << "[Dashboard] Applying proxy:" << proxyConfig.host << ":" << proxyConfig.port
                             << "(kind:" << (mode == 1 ? "WiFi" : "Cellular") << ")";
                    controller.assignProxy(instanceId, proxyConfig, kind);
                    
                    // Show proxy info in success message
                    QMessageBox::information(this, "Success",
                        QString("Phone '%1' created successfully!\n\n"
                                "🌐 Network Mode: With Proxy\n"
                                "📍 Proxy: %2:%3\n"
                                "ADB Port: %4\n"
                                "Profile: %5")
                            .arg(instanceId)
                            .arg(proxyConfig.host)
                            .arg(proxyConfig.port)
                            .arg(port)
                            .arg(profile.name)
                    );
                } else {
                    QMessageBox::warning(this, "Proxy Warning",
                        "Phone created but proxy configuration is invalid."
                    );
                }
            } else {
                // No proxy - show normal success message
                QMessageBox::information(this, "Success",
                    QString("Phone '%1' created successfully!\n\n"
                            "🌐 Network Mode: Without Proxy (Random IP)\n"
                            "ADB Port: %2\n"
                            "Profile: %3")
                        .arg(instanceId)
                        .arg(port)
                        .arg(profile.name)
                );
            }
            
            // Apply anti-detection
            BankingAppSpoofer::instance().applyCompleteBankingSetup(instanceId);
            SafetyNetSpoofer::instance().spoofSafetyNetResponse(instanceId);
            
            // Refresh and create card
            refreshInstances();
            
        } else {
            QMessageBox::warning(this, "Error",
                QString("Failed to create phone '%1'").arg(instanceId)
            );
        }
    }
}

void DashboardWindow::onBatchLaunchClicked() {
    // ── Dialog — ask how many instances and which profile ────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle("Batch Launch");
    dlg.setFixedWidth(420);

    QVBoxLayout* dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->setSpacing(12);
    dlgLayout->setContentsMargins(20, 20, 20, 20);

    dlgLayout->addWidget(new QLabel("<b>Number of instances:</b>", &dlg));
    QSpinBox* countSpin = new QSpinBox(&dlg);
    countSpin->setRange(1, 20);
    countSpin->setValue(3);
    countSpin->setFixedHeight(32);
    dlgLayout->addWidget(countSpin);

    dlgLayout->addWidget(new QLabel("<b>Stagger delay between launches (ms):</b>", &dlg));
    QSpinBox* delaySpin = new QSpinBox(&dlg);
    delaySpin->setRange(0, 10000);
    delaySpin->setValue(500);
    delaySpin->setSingleStep(250);
    delaySpin->setFixedHeight(32);
    dlgLayout->addWidget(delaySpin);

    dlgLayout->addWidget(new QLabel("<b>Profile prefix (e.g. 'farm'):</b>", &dlg));
    QLineEdit* prefixEdit = new QLineEdit("phone", &dlg);
    prefixEdit->setFixedHeight(32);
    dlgLayout->addWidget(prefixEdit);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const int     count  = countSpin->value();
    const int     delay  = delaySpin->value();
    const QString prefix = prefixEdit->text().trimmed().isEmpty()
                           ? "phone" : prefixEdit->text().trimmed();

    // ── Build config ─────────────────────────────────────────────────────────
    InstanceDeployConfig config;
    config.count         = count;
    config.profilePrefix = prefix;
    config.baseProfile   = DeviceProfile::createSamsungS24Ultra();
    config.assignUniquePort = true;
    config.portStart     = 5560;
    config.delayBetween  = delay;

    // ── Progress dialog ───────────────────────────────────────────────────────
    QProgressDialog* prog = new QProgressDialog(
        QString("Launching %1 instances in parallel...").arg(count),
        "Cancel", 0, count, this);
    prog->setWindowTitle("Batch Launch");
    prog->setWindowModality(Qt::WindowModal);
    prog->setMinimumDuration(0);
    prog->setValue(0);

    MultiInstanceManager& mgr = MultiInstanceManager::instance();

    // Connect progress signal BEFORE starting so we don't miss early updates
    connect(&mgr, &MultiInstanceManager::batchProgress,
            prog, [prog](const QString&, int done, int total) {
        prog->setMaximum(total);
        prog->setValue(done);
        prog->setLabelText(QString("Launched %1 / %2 instances...").arg(done).arg(total));
        QCoreApplication::processEvents();
    });

    connect(&mgr, &MultiInstanceManager::batchCompleted,
            this, [this, prog](const QString&, const BatchStatus& status) {
        prog->close();
        refreshInstances();
        QMessageBox::information(this, "Batch Launch Complete",
            QString("✅  %1 launched successfully\n"
                    "❌  %2 failed")
                .arg(status.completed - status.failed)
                .arg(status.failed));
    });

    // Disable button during launch to prevent double-click
    m_batchLaunchButton->setEnabled(false);
    connect(&mgr, &MultiInstanceManager::batchCompleted,
            this, [this]() { m_batchLaunchButton->setEnabled(true); });

    // Launch asynchronously so the UI stays responsive
    QtConcurrent::run([&mgr, config]() {
        mgr.deployBatch(config);
    });

    prog->exec();
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
    connect(card, &PhoneCard::cloneRequested,  this, &DashboardWindow::onPhoneCardClone);
    
    // Add to grid - calculate correct row/col from current card count
    // rowCount()/columnCount() returns TOTAL spans, not next position
    int count = m_phoneCards.size(); // count before this card is added
    int col = count % 4;
    int row = count / 4;
    m_cardGrid->addWidget(card, row, col);
    
    m_phoneCards[instanceId] = card;
}

void DashboardWindow::removePhoneCard(const QString& instanceId) {
    if (m_phoneCards.contains(instanceId)) {
        PhoneCard* card = m_phoneCards[instanceId];
        m_cardGrid->removeWidget(card);
        delete card;
        m_phoneCards.remove(instanceId);

        // Re-index all remaining cards so no holes appear in the grid.
        // QGridLayout leaves ghost cells when a widget is removed mid-grid;
        // we must re-add every surviving card at its new sequential position.
        int i = 0;
        for (PhoneCard* remaining : m_phoneCards.values()) {
            m_cardGrid->removeWidget(remaining);
            m_cardGrid->addWidget(remaining, i / 4, i % 4);
            ++i;
        }
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
        SafetyNetSpoofer::instance().spoofSafetyNetResponse(instanceId);
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

void DashboardWindow::onPhoneCardClone(const QString& instanceId) {
    // AppCloner creates a new instance with a completely fresh device identity
    // (IMEI, serial number, MAC address, AndroidId, GAID) while inheriting
    // the same profile type as the source instance.
    VirtualPhonePro::AppCloner& cloner = VirtualPhonePro::AppCloner::instance();

    QString newId = cloner.cloneInstance(instanceId);
    if (newId.isEmpty()) {
        QMessageBox::warning(this, "Clone Failed",
            QString("Could not clone '%1'.\n\nCheck the application log for details.")
                .arg(instanceId));
        return;
    }

    refreshInstances();
    QMessageBox::information(this, "Clone Complete",
        QString("'%1' cloned successfully.\nNew instance: %2")
            .arg(instanceId, newId));
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

// ============================================================================
// Requirements (WSL2 + Docker Desktop) install / uninstall
// ============================================================================

void DashboardWindow::refreshRequirementsState()
{
    const bool installed = RequirementsManager::areRequirementsInstalled();
    const bool busy = m_requirementsManager && m_requirementsManager->isBusy();

    if (busy) {
        m_installRequirementsButton->setEnabled(false);
        m_uninstallRequirementsButton->setEnabled(false);
        return;
    }

    if (installed) {
        m_installRequirementsButton->setText("✓ Installed Requirements");
        m_installRequirementsButton->setEnabled(false);
        m_uninstallRequirementsButton->show();
        m_uninstallRequirementsButton->setEnabled(true);
    } else {
        m_installRequirementsButton->setText("⬇ Install Requirements");
        m_installRequirementsButton->setEnabled(true);
        m_uninstallRequirementsButton->hide();
    }
}

void DashboardWindow::onInstallRequirementsClicked()
{
    if (m_requirementsManager->isBusy()) return;

    // Confirm: WSL2 install needs elevation + a reboot.
    auto confirm = QMessageBox::question(
        this, "Install Requirements",
        "This will install WSL2 and Docker Desktop.\n\n"
        "Notes:\n"
        "• Windows may show a UAC (admin) prompt — please accept.\n"
        "• Enabling WSL2 requires a system reboot afterwards.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (confirm != QMessageBox::Yes) return;

    // UI → "Installing Requirements" state.
    m_installRequirementsButton->setText("⏳ Installing Requirements...");
    m_installRequirementsButton->setEnabled(false);
    m_uninstallRequirementsButton->hide();

    m_requirementsProgressBar->setValue(0);
    m_requirementsProgressBar->show();
    m_statusLabel->setText("Installing requirements...");

    m_requirementsManager->install();
}

void DashboardWindow::onUninstallRequirementsClicked()
{
    if (m_requirementsManager->isBusy()) return;

    auto confirm = QMessageBox::question(
        this, "Uninstall Requirements",
        "This will uninstall Docker Desktop and remove WSL2 components.\n\n"
        "Any running containers will be stopped.\n\nContinue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (confirm != QMessageBox::Yes) return;

    m_installRequirementsButton->hide();
    m_uninstallRequirementsButton->setText("⏳ Uninstalling...");
    m_uninstallRequirementsButton->setEnabled(false);

    m_requirementsProgressBar->setValue(0);
    m_requirementsProgressBar->show();
    m_statusLabel->setText("Uninstalling requirements...");

    m_requirementsManager->uninstall();
}

void DashboardWindow::onRequirementsLog(const QString& line)
{
    qDebug() << "[Requirements]" << line;
    m_statusLabel->setText(line);
}

void DashboardWindow::onRequirementsProgress(int percent)
{
    m_requirementsProgressBar->setValue(percent);
}

void DashboardWindow::onRequirementsFinished(bool success, const QString& summary)
{
    m_requirementsProgressBar->setValue(success ? 100 : m_requirementsProgressBar->value());
    m_statusLabel->setText(summary);

    // Keep the bar visible briefly so the user sees it reach the end,
    // then hide it and restore button state.
    QTimer::singleShot(1200, this, [this]() {
        m_requirementsProgressBar->hide();
        m_uninstallRequirementsButton->setText("🗑 Uninstall");
        refreshRequirementsState();
    });

    QMessageBox::information(this,
        success ? "Requirements Installed" : "Requirements — Finished",
        summary + "\n\n" +
        (success ? "If WSL2 was just enabled, please reboot Windows once "
                   "before launching phone instances."
                 : "Check the status line for details."));
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
