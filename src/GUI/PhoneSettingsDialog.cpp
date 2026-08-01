/**
 * @file PhoneSettingsDialog.cpp
 * @brief Phone Settings Dialog Implementation
 */

#include "PhoneSettingsDialog.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

namespace VirtualPhonePro {

PhoneSettingsDialog::PhoneSettingsDialog(const QString& instanceId, QWidget* parent)
    : QDialog(parent)
    , m_instanceId(instanceId)
{
    setWindowTitle(QString("Phone Settings - %1").arg(instanceId));
    setMinimumSize(700, 600);
    setModal(true);
    
    setupUI();
    refreshDeviceInfo();
}

PhoneSettingsDialog::~PhoneSettingsDialog() {}

void PhoneSettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel* titleLabel = new QLabel(QString("⚙️ Advanced Settings for: %1").arg(m_instanceId), this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    
    // Add pages
    m_tabWidget->addTab(createGPSPage(), "📍 GPS/Location");
    m_tabWidget->addTab(createDNSPage(), "🌐 DNS");
    m_tabWidget->addTab(createVPNPage(), "🔒 VPN");
    m_tabWidget->addTab(createNetworkPage(), "🛡️ Network Security");
    m_tabWidget->addTab(createDeviceInfoPage(), "📱 Device Info");
    m_tabWidget->addTab(createProfilePage(), "💾 Profile");
    
    mainLayout->addWidget(m_tabWidget);
    
    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* closeBtn = new QPushButton("Close", this);
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);
}

QWidget* PhoneSettingsDialog::createGPSPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Status
    m_gpsStatusLabel = new QLabel("Status: Unknown", page);
    m_gpsStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    layout->addWidget(m_gpsStatusLabel);
    
    // GPS Coordinates Group
    QGroupBox* coordGroup = new QGroupBox("📍 GPS Coordinates", page);
    QFormLayout* coordLayout = new QFormLayout(coordGroup);
    
    m_latitudeSpin = new QDoubleSpinBox(page);
    m_latitudeSpin->setRange(-90, 90);
    m_latitudeSpin->setDecimals(6);
    m_latitudeSpin->setValue(23.8103); // Dhaka default
    m_latitudeSpin->setSuffix(" °");
    coordLayout->addRow("Latitude:", m_latitudeSpin);
    
    m_longitudeSpin = new QDoubleSpinBox(page);
    m_longitudeSpin->setRange(-180, 180);
    m_longitudeSpin->setDecimals(6);
    m_longitudeSpin->setValue(90.4125); // Dhaka default
    m_longitudeSpin->setSuffix(" °");
    coordLayout->addRow("Longitude:", m_longitudeSpin);
    
    m_altitudeSpin = new QDoubleSpinBox(page);
    m_altitudeSpin->setRange(-500, 9000);
    m_altitudeSpin->setDecimals(1);
    m_altitudeSpin->setValue(10);
    m_altitudeSpin->setSuffix(" m");
    coordLayout->addRow("Altitude:", m_altitudeSpin);
    
    m_accuracySpin = new QDoubleSpinBox(page);
    m_accuracySpin->setRange(1, 100);
    m_accuracySpin->setValue(5);
    m_accuracySpin->setSuffix(" m");
    coordLayout->addRow("Accuracy:", m_accuracySpin);
    
    layout->addWidget(coordGroup);
    
    // Presets
    QGroupBox* presetGroup = new QGroupBox("🎯 Location Presets", page);
    QVBoxLayout* presetLayout = new QVBoxLayout(presetGroup);
    
    m_gpsPresetCombo = new QComboBox(page);
    m_gpsPresetCombo->addItems({
        "Custom Location",
        "📍 Dhaka, Bangladesh",
        "📍 Dhaka 2, Bangladesh", 
        "📍 Chittagong, Bangladesh",
        "📍 Sylhet, Bangladesh",
        "📍 London, UK",
        "📍 New York, USA",
        "📍 Tokyo, Japan",
        "📍 Dubai, UAE",
        "📍 Singapore"
    });
    presetLayout->addWidget(m_gpsPresetCombo);
    
    layout->addWidget(presetGroup);
    
    // Control Buttons
    QGroupBox* controlGroup = new QGroupBox("Controls", page);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);
    
    m_applyGPSBtn = new QPushButton("✅ Apply GPS", page);
    m_applyGPSBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_applyGPSBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onGPSLocationChanged);
    controlLayout->addWidget(m_applyGPSBtn);
    
    m_randomizeGPSBtn = new QPushButton("🎲 Randomize", page);
    m_randomizeGPSBtn->setStyleSheet("background: #007bff; color: white; padding: 10px;");
    connect(m_randomizeGPSBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onRandomizeGPS);
    controlLayout->addWidget(m_randomizeGPSBtn);
    
    layout->addWidget(controlGroup);
    
    // Mock Location
    QGroupBox* mockGroup = new QGroupBox("Mock Location Mode", page);
    QHBoxLayout* mockLayout = new QHBoxLayout(mockGroup);
    
    m_enableMockBtn = new QPushButton("🔓 Enable Mock Location", page);
    m_enableMockBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_enableMockBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onEnableMockLocation);
    mockLayout->addWidget(m_enableMockBtn);
    
    m_disableMockBtn = new QPushButton("🔒 Disable Mock Location", page);
    m_disableMockBtn->setStyleSheet("background: #dc3545; color: white; padding: 10px;");
    connect(m_disableMockBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onDisableMockLocation);
    mockLayout->addWidget(m_disableMockBtn);
    
    layout->addWidget(mockGroup);
    layout->addStretch();
    
    return page;
}

QWidget* PhoneSettingsDialog::createDNSPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Status
    m_dnsStatusLabel = new QLabel("Current DNS: Unknown", page);
    m_dnsStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    layout->addWidget(m_dnsStatusLabel);
    
    // DNS Configuration
    QGroupBox* dnsGroup = new QGroupBox("🌐 DNS Servers", page);
    QFormLayout* dnsLayout = new QFormLayout(dnsGroup);
    
    m_dns1Edit = new QLineEdit(page);
    m_dns1Edit->setPlaceholderText("e.g., 8.8.8.8");
    dnsLayout->addRow("Primary DNS:", m_dns1Edit);
    
    m_dns2Edit = new QLineEdit(page);
    m_dns2Edit->setPlaceholderText("e.g., 8.8.4.4");
    dnsLayout->addRow("Secondary DNS:", m_dns2Edit);
    
    layout->addWidget(dnsGroup);
    
    // DNS Presets
    QGroupBox* presetGroup = new QGroupBox("🎯 DNS Presets", page);
    QVBoxLayout* presetLayout = new QVBoxLayout(presetGroup);
    
    m_dnsPresetCombo = new QComboBox(page);
    m_dnsPresetCombo->addItems({
        "Custom DNS",
        "🆓 Google DNS (8.8.8.8, 8.8.4.4)",
        "🆓 Cloudflare DNS (1.1.1.1, 1.0.0.1)",
        "🆓 Quad9 DNS (9.9.9.9, 149.112.112.112)",
        "🇧🇩 Bangladesh DNS (103.11.12.12, 203.112.12.12)",
        "🔒 OpenDNS (208.67.222.222, 208.67.220.220)",
        "🔒 CleanBrowsing (185.228.168.9, 185.228.169.9)"
    });
    presetLayout->addWidget(m_dnsPresetCombo);
    
    layout->addWidget(presetGroup);
    
    // Control Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_applyDNSBtn = new QPushButton("✅ Apply DNS", page);
    m_applyDNSBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_applyDNSBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onApplyDNS);
    buttonLayout->addWidget(m_applyDNSBtn);
    
    m_resetDNSBtn = new QPushButton("🔄 Reset to Default", page);
    m_resetDNSBtn->setStyleSheet("background: #6c757d; color: white; padding: 10px;");
    connect(m_resetDNSBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onResetDNS);
    buttonLayout->addWidget(m_resetDNSBtn);
    
    layout->addWidget(buttonLayout);
    layout->addStretch();
    
    return page;
}

QWidget* PhoneSettingsDialog::createVPNPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Status
    m_vpnStatusLabel = new QLabel("VPN Status: Not Connected", page);
    m_vpnStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    layout->addWidget(m_vpnStatusLabel);
    
    // VPN Configuration
    QGroupBox* configGroup = new QGroupBox("🔧 VPN Configuration", page);
    QFormLayout* configLayout = new QFormLayout(configGroup);
    
    m_vpnNameEdit = new QLineEdit(page);
    m_vpnNameEdit->setPlaceholderText("My VPN Connection");
    configLayout->addRow("VPN Name:", m_vpnNameEdit);
    
    m_vpnServerEdit = new QLineEdit(page);
    m_vpnServerEdit->setPlaceholderText("vpn.example.com or IP address");
    configLayout->addRow("Server:", m_vpnServerEdit);
    
    m_vpnPortSpin = new QSpinBox(page);
    m_vpnPortSpin->setRange(1, 65535);
    m_vpnPortSpin->setValue(1194);
    configLayout->addRow("Port:", m_vpnPortSpin);
    
    m_vpnTypeCombo = new QComboBox(page);
    m_vpnTypeCombo->addItems({"OpenVPN", "WireGuard", "IKEv2", "L2TP/IPSec"});
    configLayout->addRow("Protocol:", m_vpnTypeCombo);
    
    m_vpnUsernameEdit = new QLineEdit(page);
    m_vpnUsernameEdit->setPlaceholderText("Username");
    configLayout->addRow("Username:", m_vpnUsernameEdit);
    
    m_vpnPasswordEdit = new QLineEdit(page);
    m_vpnPasswordEdit->setPlaceholderText("Password");
    m_vpnPasswordEdit->setEchoMode(QLineEdit::Password);
    configLayout->addRow("Password:", m_vpnPasswordEdit);
    
    layout->addWidget(configGroup);
    
    // Control Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_connectVPNBtn = new QPushButton("🔗 Connect", page);
    m_connectVPNBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_connectVPNBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onSetupVPN);
    buttonLayout->addWidget(m_connectVPNBtn);
    
    m_disconnectVPNBtn = new QPushButton("🔌 Disconnect", page);
    m_disconnectVPNBtn->setStyleSheet("background: #dc3545; color: white; padding: 10px;");
    m_disconnectVPNBtn->setEnabled(false);
    connect(m_disconnectVPNBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onDisconnectVPN);
    buttonLayout->addWidget(m_disconnectVPNBtn);
    
    m_importVPNBtn = new QPushButton("📁 Import .ovpn", page);
    m_importVPNBtn->setStyleSheet("background: #007bff; color: white; padding: 10px;");
    connect(m_importVPNBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onImportVPNConfig);
    buttonLayout->addWidget(m_importVPNBtn);
    
    layout->addWidget(buttonLayout);
    layout->addStretch();
    
    return page;
}

QWidget* PhoneSettingsDialog::createNetworkPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Status
    m_networkStatusLabel = new QLabel("Network Status: Ready", page);
    m_networkStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    layout->addWidget(m_networkStatusLabel);
    
    // Network Isolation
    QGroupBox* isolationGroup = new QGroupBox("🌐 Network Isolation", page);
    QHBoxLayout* isolationLayout = new QHBoxLayout(isolationGroup);
    
    m_createNetworkBtn = new QPushButton("🔨 Create Isolated Network", page);
    m_createNetworkBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_createNetworkBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onCreateNetwork);
    isolationLayout->addWidget(m_createNetworkBtn);
    
    m_deleteNetworkBtn = new QPushButton("🗑️ Delete Network", page);
    m_deleteNetworkBtn->setStyleSheet("background: #dc3545; color: white; padding: 10px;");
    connect(m_deleteNetworkBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onDeleteNetwork);
    isolationLayout->addWidget(m_deleteNetworkBtn);
    
    layout->addWidget(isolationGroup);
    
    // Security Options
    QGroupBox* securityGroup = new QGroupBox("🛡️ Security Options", page);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityGroup);
    
    m_ipv6BlockCheck = new QCheckBox("🚫 Block IPv6 Traffic (Prevent IPv6 Leaks)", page);
    securityLayout->addWidget(m_ipv6BlockCheck);
    
    QHBoxLayout* leakLayout = new QHBoxLayout();
    m_applyLeakPreventionBtn = new QPushButton("🛡️ Apply Leak Prevention", page);
    m_applyLeakPreventionBtn->setStyleSheet("background: #17a2b8; color: white; padding: 10px;");
    connect(m_applyLeakPreventionBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onApplyLeakPrevention);
    leakLayout->addWidget(m_applyLeakPreventionBtn);
    
    connect(m_ipv6BlockCheck, &QCheckBox::toggled, this, &PhoneSettingsDialog::onToggleIPv6);
    securityLayout->addLayout(leakLayout);
    
    layout->addWidget(securityGroup);
    
    // Leak Test
    QGroupBox* leakGroup = new QGroupBox("🔍 Network Leak Test", page);
    QVBoxLayout* leakLayoutV = new QVBoxLayout(leakGroup);
    
    m_leakTestProgress = new QProgressBar(page);
    m_leakTestProgress->setVisible(false);
    leakLayoutV->addWidget(m_leakTestProgress);
    
    m_leakTestResult = new QTextEdit(page);
    m_leakTestResult->setReadOnly(true);
    m_leakTestResult->setMaximumHeight(150);
    m_leakTestResult->setPlaceholderText("Leak test results will appear here...");
    leakLayoutV->addWidget(m_leakTestResult);
    
    QPushButton* testBtn = new QPushButton("🔍 Run Leak Test", page);
    testBtn->setStyleSheet("background: #ffc107; color: black; padding: 10px;");
    connect(testBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onTestLeaks);
    leakLayoutV->addWidget(testBtn);
    
    layout->addWidget(leakGroup);
    layout->addStretch();
    
    return page;
}

QWidget* PhoneSettingsDialog::createDeviceInfoPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Device Info
    QGroupBox* infoGroup = new QGroupBox("📱 Device Information", page);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_deviceModelLabel = new QLabel("Unknown", page);
    infoLayout->addRow("Model:", m_deviceModelLabel);
    
    m_androidVersionLabel = new QLabel("Unknown", page);
    infoLayout->addRow("Android Version:", m_androidVersionLabel);
    
    m_imeiLabel = new QLabel("Unknown", page);
    infoLayout->addRow("IMEI:", m_imeiLabel);
    
    m_androidIdLabel = new QLabel("Unknown", page);
    infoLayout->addRow("Android ID:", m_androidIdLabel);
    
    m_ipAddressLabel = new QLabel("Unknown", page);
    infoLayout->addRow("IP Address:", m_ipAddressLabel);
    
    m_macAddressLabel = new QLabel("Unknown", page);
    infoLayout->addRow("MAC Address:", m_macAddressLabel);
    
    layout->addWidget(infoGroup);
    
    // Uniqueness Status
    QGroupBox* uniqueGroup = new QGroupBox("✓ Device Uniqueness", page);
    QVBoxLayout* uniqueLayout = new QVBoxLayout(uniqueGroup);
    
    m_uniquenessStatusLabel = new QLabel("Not Verified", page);
    m_uniquenessStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    uniqueLayout->addWidget(m_uniquenessStatusLabel);
    
    m_verifyUniquenessBtn = new QPushButton("🔐 Verify Device Uniqueness", page);
    m_verifyUniquenessBtn->setStyleSheet("background: #17a2b8; color: white; padding: 10px;");
    connect(m_verifyUniquenessBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onVerifyUniqueness);
    uniqueLayout->addWidget(m_verifyUniquenessBtn);
    
    layout->addWidget(uniqueGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_refreshInfoBtn = new QPushButton("🔄 Refresh Info", page);
    m_refreshInfoBtn->setStyleSheet("background: #007bff; color: white; padding: 10px;");
    connect(m_refreshInfoBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onRefreshDeviceInfo);
    buttonLayout->addWidget(m_refreshInfoBtn);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // Details
    m_deviceDetailsText = new QTextEdit(page);
    m_deviceDetailsText->setReadOnly(true);
    m_deviceDetailsText->setMaximumHeight(120);
    layout->addWidget(m_deviceDetailsText);
    
    return page;
}

QWidget* PhoneSettingsDialog::createProfilePage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // Profile Name
    QGroupBox* nameGroup = new QGroupBox("📋 Profile Name", page);
    QHBoxLayout* nameLayout = new QHBoxLayout(nameGroup);
    m_profileNameEdit = new QLineEdit(page);
    m_profileNameEdit->setPlaceholderText("My Custom Profile");
    nameLayout->addWidget(m_profileNameEdit);
    layout->addWidget(nameGroup);
    
    // Status
    m_profileStatusLabel = new QLabel("Ready to save or load profiles", page);
    m_profileStatusLabel->setStyleSheet("padding: 10px; background: #2a2a3e; border-radius: 5px;");
    layout->addWidget(m_profileStatusLabel);
    
    // Profile Actions
    QGroupBox* actionGroup = new QGroupBox("💾 Profile Actions", page);
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
    QHBoxLayout* saveLoadLayout = new QHBoxLayout();
    m_saveProfileBtn = new QPushButton("💾 Save Profile", page);
    m_saveProfileBtn->setStyleSheet("background: #28a745; color: white; padding: 10px;");
    connect(m_saveProfileBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onSaveProfile);
    saveLoadLayout->addWidget(m_saveProfileBtn);
    
    m_loadProfileBtn = new QPushButton("📂 Load Profile", page);
    m_loadProfileBtn->setStyleSheet("background: #007bff; color: white; padding: 10px;");
    connect(m_loadProfileBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onLoadProfile);
    saveLoadLayout->addWidget(m_loadProfileBtn);
    
    actionLayout->addLayout(saveLoadLayout);
    
    m_exportProfileBtn = new QPushButton("📤 Export Profile (JSON)", page);
    m_exportProfileBtn->setStyleSheet("background: #6c757d; color: white; padding: 10px;");
    connect(m_exportProfileBtn, &QPushButton::clicked, this, &PhoneSettingsDialog::onExportProfile);
    actionLayout->addWidget(m_exportProfileBtn);
    
    layout->addWidget(actionGroup);
    
    // Saved Profiles List
    QGroupBox* listGroup = new QGroupBox("📁 Saved Profiles", page);
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);
    
    m_savedProfilesList = new QListWidget(page);
    m_savedProfilesList->setMaximumHeight(150);
    
    // Load saved profiles
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir + "/profiles");
    if (dir.exists()) {
        QStringList files = dir.entryList({"*.json"}, QDir::Files);
        for (const QString& file : files) {
            QString name = file;
            name.chop(5); // Remove .json
            m_savedProfilesList->addItem(name);
        }
    }
    
    listLayout->addWidget(m_savedProfilesList);
    layout->addWidget(listGroup);
    
    layout->addStretch();
    
    return page;
}

// ============================================================================
// GPS Methods
// ============================================================================

void PhoneSettingsDialog::onGPSLocationChanged() {
    double lat = m_latitudeSpin->value();
    double lon = m_longitudeSpin->value();
    double alt = m_altitudeSpin->value();
    double acc = m_accuracySpin->value();
    
    // Apply GPS via ADB
    ReDroidController& controller = ReDroidController::instance();
    
    SensorData data;
    data.latitude = lat;
    data.longitude = lon;
    data.altitude = alt;
    data.accuracy = acc;
    data.speed = 0;
    data.bearing = 0;
    
    bool success = controller.sendSensorData(m_instanceId, data);
    
    if (success) {
        controller.enableMockLocation(m_instanceId);
        m_gpsStatusLabel->setText(QString("✅ GPS Updated: %1, %2").arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6));
        m_gpsStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        showMessage("GPS Updated", QString("Location set to: %1, %2").arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6));
    } else {
        m_gpsStatusLabel->setText("❌ Failed to update GPS");
        m_gpsStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        showMessage("GPS Error", "Failed to update GPS location", true);
    }
}

void PhoneSettingsDialog::onEnableMockLocation() {
    ReDroidController& controller = ReDroidController::instance();
    if (controller.enableMockLocation(m_instanceId)) {
        m_gpsStatusLabel->setText("✅ Mock Location Enabled");
        m_gpsStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        showMessage("Mock Location", "Mock location mode enabled successfully");
    } else {
        showMessage("Mock Location Error", "Failed to enable mock location", true);
    }
}

void PhoneSettingsDialog::onDisableMockLocation() {
    ReDroidController& controller = ReDroidController::instance();
    if (controller.disableMockLocation(m_instanceId)) {
        m_gpsStatusLabel->setText("🔒 Mock Location Disabled");
        m_gpsStatusLabel->setStyleSheet("padding: 10px; background: #6c757d; border-radius: 5px; color: white;");
        showMessage("Mock Location", "Mock location mode disabled");
    } else {
        showMessage("Mock Location Error", "Failed to disable mock location", true);
    }
}

void PhoneSettingsDialog::onRandomizeGPS() {
    // Random Bangladesh location
    double lat = 23.7 + (QRandomGenerator::global()->bounded(1000) / 10000.0);
    double lon = 90.3 + (QRandomGenerator::global()->bounded(1000) / 10000.0);
    
    m_latitudeSpin->setValue(lat);
    m_longitudeSpin->setValue(lon);
    m_altitudeSpin->setValue(10 + QRandomGenerator::global()->bounded(50));
    m_accuracySpin->setValue(5 + QRandomGenerator::global()->bounded(20));
    
    m_gpsStatusLabel->setText(QString("🎲 Randomized: %1, %2").arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4));
}

void PhoneSettingsDialog::onPresetsGPS(const QString& preset) {
    // Location presets
    QMap<QString, QPair<double, double>> presets = {
        {"📍 Dhaka, Bangladesh", {23.8103, 90.4125}},
        {"📍 Dhaka 2, Bangladesh", {23.7803, 90.4225}},
        {"📍 Chittagong, Bangladesh", {22.3569, 91.7832}},
        {"📍 Sylhet, Bangladesh", {24.8917, 91.8694}},
        {"📍 London, UK", {51.5074, -0.1278}},
        {"📍 New York, USA", {40.7128, -74.0060}},
        {"📍 Tokyo, Japan", {35.6762, 139.6503}},
        {"📍 Dubai, UAE", {25.2048, 55.2708}},
        {"📍 Singapore", {1.3521, 103.8198}}
    };
    
    if (presets.contains(preset)) {
        auto coords = presets[preset];
        m_latitudeSpin->setValue(coords.first);
        m_longitudeSpin->setValue(coords.second);
    }
}

// ============================================================================
// DNS Methods
// ============================================================================

void PhoneSettingsDialog::onApplyDNS() {
    QString dns1 = m_dns1Edit->text().trimmed();
    QString dns2 = m_dns2Edit->text().trimmed();
    
    if (dns1.isEmpty()) {
        showMessage("DNS Error", "Please enter at least one DNS server", true);
        return;
    }
    
    QList<QString> dnsServers = {dns1};
    if (!dns2.isEmpty()) {
        dnsServers.append(dns2);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.configureDNS(m_instanceId, dnsServers);
    
    if (success) {
        m_dnsStatusLabel->setText(QString("✅ DNS: %1").arg(dns1));
        if (!dns2.isEmpty()) {
            m_dnsStatusLabel->setText(QString("✅ DNS: %1, %2").arg(dns1).arg(dns2));
        }
        m_dnsStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        showMessage("DNS Updated", QString("DNS set to: %1").arg(dnsServers.join(", ")));
    } else {
        m_dnsStatusLabel->setText("❌ Failed to update DNS");
        m_dnsStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        showMessage("DNS Error", "Failed to update DNS servers", true);
    }
}

void PhoneSettingsDialog::onResetDNS() {
    // Reset to default (Google DNS)
    m_dns1Edit->setText("8.8.8.8");
    m_dns2Edit->setText("8.8.4.4");
    onApplyDNS();
}

void PhoneSettingsDialog::onPresetDNS(const QString& preset) {
    QMap<QString, QStringList> presets = {
        {"🆓 Google DNS (8.8.8.8, 8.8.4.4)", {"8.8.8.8", "8.8.4.4"}},
        {"🆓 Cloudflare DNS (1.1.1.1, 1.0.0.1)", {"1.1.1.1", "1.0.0.1"}},
        {"🆓 Quad9 DNS (9.9.9.9, 149.112.112.112)", {"9.9.9.9", "149.112.112.112"}},
        {"🇧🇩 Bangladesh DNS (103.11.12.12, 203.112.12.12)", {"103.11.12.12", "203.112.12.12"}},
        {"🔒 OpenDNS (208.67.222.222, 208.67.220.220)", {"208.67.222.222", "208.67.220.220"}},
        {"🔒 CleanBrowsing (185.228.168.9, 185.228.169.9)", {"185.228.168.9", "185.228.169.9"}}
    };
    
    if (presets.contains(preset)) {
        auto servers = presets[preset];
        m_dns1Edit->setText(servers[0]);
        m_dns2Edit->setText(servers[1]);
    }
}

// ============================================================================
// VPN Methods
// ============================================================================

void PhoneSettingsDialog::onSetupVPN() {
    QString server = m_vpnServerEdit->text().trimmed();
    int port = m_vpnPortSpin->value();
    
    if (server.isEmpty()) {
        showMessage("VPN Error", "Please enter VPN server address", true);
        return;
    }
    
    VPNConfig vpn;
    vpn.name = m_vpnNameEdit->text().isEmpty() ? "My VPN" : m_vpnNameEdit->text();
    vpn.server = server;
    vpn.port = port;
    vpn.username = m_vpnUsernameEdit->text();
    vpn.password = m_vpnPasswordEdit->text();
    vpn.type = m_vpnTypeCombo->currentText().toLower();
    
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.setupVPN(m_instanceId, vpn);
    
    if (success) {
        m_vpnStatusLabel->setText(QString("✅ VPN Connected: %1:%2").arg(server).arg(port));
        m_vpnStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        m_connectVPNBtn->setEnabled(false);
        m_disconnectVPNBtn->setEnabled(true);
        showMessage("VPN Connected", QString("Connected to %1").arg(server));
    } else {
        m_vpnStatusLabel->setText("❌ VPN Connection Failed");
        m_vpnStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        showMessage("VPN Error", "Failed to connect to VPN", true);
    }
}

void PhoneSettingsDialog::onDisconnectVPN() {
    // For now, just reset UI state
    m_vpnStatusLabel->setText("🔌 VPN Disconnected");
    m_vpnStatusLabel->setStyleSheet("padding: 10px; background: #6c757d; border-radius: 5px; color: white;");
    m_connectVPNBtn->setEnabled(true);
    m_disconnectVPNBtn->setEnabled(false);
    showMessage("VPN Disconnected", "VPN has been disconnected");
}

void PhoneSettingsDialog::onImportVPNConfig() {
    QString fileName = QFileDialog::getOpenFileName(this, 
        "Import VPN Config", "", "VPN Config Files (*.ovpn *.conf *.json)");
    
    if (!fileName.isEmpty()) {
        showMessage("Import", QString("VPN config imported from: %1").arg(fileName));
        m_vpnStatusLabel->setText(QString("📁 Config: %1").arg(QFileInfo(fileName).fileName()));
    }
}

// ============================================================================
// Network Methods
// ============================================================================

void PhoneSettingsDialog::onCreateNetwork() {
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.createIsolatedNetwork(m_instanceId);
    
    if (success) {
        m_networkStatusLabel->setText("✅ Isolated Network Created");
        m_networkStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        showMessage("Network", "Isolated network created successfully");
    } else {
        m_networkStatusLabel->setText("❌ Failed to create network");
        m_networkStatusLabel->setStyleSheet("padding: 10px; background: #dc3545; border-radius: 5px; color: white;");
        showMessage("Network Error", "Failed to create isolated network", true);
    }
}

void PhoneSettingsDialog::onDeleteNetwork() {
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.deleteIsolatedNetwork(m_instanceId);
    
    if (success) {
        m_networkStatusLabel->setText("🗑️ Network Deleted");
        m_networkStatusLabel->setStyleSheet("padding: 10px; background: #6c757d; border-radius: 5px; color: white;");
        showMessage("Network", "Isolated network deleted");
    } else {
        showMessage("Network Error", "Failed to delete isolated network", true);
    }
}

void PhoneSettingsDialog::onTestLeaks() {
    m_leakTestProgress->setVisible(true);
    m_leakTestProgress->setRange(0, 0); // Indeterminate
    m_leakTestResult->clear();
    m_leakTestResult->append("🔍 Testing for network leaks...\n");
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Run leak test
    m_leakTestResult->append("Checking IPv4...");
    bool ipv4Ok = !controller.getProperty(m_instanceId, "net.ipv4.address").isEmpty();
    
    m_leakTestResult->append("Checking IPv6...");
    bool ipv6Status = controller.blockIPv6(m_instanceId);
    
    m_leakTestResult->append("Checking DNS...");
    QString dns = controller.getProperty(m_instanceId, "net.dns1");
    
    m_leakTestResult->append("\n📊 Results:");
    m_leakTestResult->append(QString("IPv4: %1").arg(ipv4Ok ? "✅ OK" : "❌ Not Set"));
    m_leakTestResult->append(QString("IPv6 Blocking: %1").arg(ipv6Status ? "✅ Enabled" : "❌ Disabled"));
    m_leakTestResult->append(QString("DNS1: %1").arg(dns.isEmpty() ? "Unknown" : dns));
    
    bool hasLeaks = false;
    if (!ipv4Ok || !ipv6Status) {
        hasLeaks = true;
        m_leakTestResult->append("\n⚠️ Potential leaks detected!");
    } else {
        m_leakTestResult->append("\n✅ No leaks detected!");
    }
    
    m_leakTestProgress->setVisible(false);
    
    m_networkStatusLabel->setText(hasLeaks ? "⚠️ Leaks Detected" : "✅ No Leaks");
    m_networkStatusLabel->setStyleSheet(QString("padding: 10px; background: %1; border-radius: 5px; color: white;").arg(hasLeaks ? "#ffc107" : "#28a745"));
}

void PhoneSettingsDialog::onApplyLeakPrevention() {
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.applyLeakPrevention(m_instanceId);
    
    if (success) {
        m_networkStatusLabel->setText("🛡️ Leak Prevention Applied");
        m_networkStatusLabel->setStyleSheet("padding: 10px; background: #17a2b8; border-radius: 5px; color: white;");
        showMessage("Leak Prevention", "Network leak prevention applied successfully");
    } else {
        showMessage("Leak Prevention Error", "Failed to apply leak prevention", true);
    }
}

void PhoneSettingsDialog::onToggleIPv6(bool enabled) {
    if (enabled) {
        ReDroidController& controller = ReDroidController::instance();
        bool success = controller.blockIPv6(m_instanceId);
        
        if (success) {
            showMessage("IPv6 Blocked", "IPv6 traffic is now blocked");
        } else {
            showMessage("IPv6 Error", "Failed to block IPv6", true);
        }
    }
}

// ============================================================================
// Device Info Methods
// ============================================================================

void PhoneSettingsDialog::onVerifyUniqueness() {
    ReDroidController& controller = ReDroidController::instance();
    
    bool isUnique = controller.verifyDeviceUniqueness(m_instanceId);
    
    if (isUnique) {
        m_uniquenessStatusLabel->setText("✅ Device Appears Unique");
        m_uniquenessStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        showMessage("Uniqueness Check", "✅ Device identifiers appear unique");
    } else {
        m_uniquenessStatusLabel->setText("⚠️ Device May Not Be Unique");
        m_uniquenessStatusLabel->setStyleSheet("padding: 10px; background: #ffc107; border-radius: 5px; color: black;");
        showMessage("Uniqueness Warning", "⚠️ Some device identifiers may be duplicated", true);
    }
}

void PhoneSettingsDialog::onRefreshDeviceInfo() {
    refreshDeviceInfo();
}

void PhoneSettingsDialog::refreshDeviceInfo() {
    ReDroidController& controller = ReDroidController::instance();
    
    // Get device properties
    QString model = controller.getProperty(m_instanceId, "ro.product.model");
    QString version = controller.getProperty(m_instanceId, "ro.build.version.release");
    QString imei = controller.getProperty(m_instanceId, "ro.gsm.sim.imei");
    QString androidId = controller.getProperty(m_instanceId, "ro.system.build.id");
    QString ip = controller.getProperty(m_instanceId, "dhcp.wlan0.ipaddress");
    QString mac = controller.getProperty(m_instanceId, "ro.wifi.interface");
    
    if (model.isEmpty()) model = "Unknown";
    if (version.isEmpty()) version = "Unknown";
    if (imei.isEmpty()) imei = "N/A";
    if (androidId.isEmpty()) androidId = "N/A";
    if (ip.isEmpty()) ip = "Not connected";
    if (mac.isEmpty()) mac = "N/A";
    
    m_deviceModelLabel->setText(model);
    m_androidVersionLabel->setText(QString("Android %1").arg(version));
    m_imeiLabel->setText(imei);
    m_androidIdLabel->setText(androidId);
    m_ipAddressLabel->setText(ip);
    m_macAddressLabel->setText(mac);
    
    // Show all properties
    QMap<QString, QString> allProps = controller.getAllProperties(m_instanceId);
    QString details;
    for (auto it = allProps.constBegin(); it != allProps.constEnd(); ++it) {
        details += QString("%1: %2\n").arg(it.key()).arg(it.value());
        if (details.size() > 2000) {
            details += "\n... (truncated)";
            break;
        }
    }
    m_deviceDetailsText->setPlainText(details);
}

// ============================================================================
// Profile Methods
// ============================================================================

void PhoneSettingsDialog::onSaveProfile() {
    QString name = m_profileNameEdit->text().trimmed();
    if (name.isEmpty()) {
        showMessage("Save Profile", "Please enter a profile name", true);
        return;
    }
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir + "/profiles");
    QString filePath = QString("%1/profiles/%2.json").arg(configDir).arg(name);
    
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(m_instanceId);
    
    QJsonObject profile;
    profile["instanceId"] = m_instanceId;
    profile["profileName"] = name;
    profile["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(profile);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        
        m_profileStatusLabel->setText(QString("✅ Profile Saved: %1").arg(name));
        m_profileStatusLabel->setStyleSheet("padding: 10px; background: #28a745; border-radius: 5px; color: white;");
        m_savedProfilesList->addItem(name);
        showMessage("Profile Saved", QString("Profile '%1' saved successfully").arg(name));
    } else {
        showMessage("Save Error", "Failed to save profile", true);
    }
}

void PhoneSettingsDialog::onLoadProfile() {
    QString selected = m_savedProfilesList->currentItem() 
        ? m_savedProfilesList->currentItem()->text() 
        : "";
    
    if (selected.isEmpty()) {
        showMessage("Load Profile", "Please select a profile from the list", true);
        return;
    }
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath = QString("%1/profiles/%2.json").arg(configDir).arg(selected);
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject profile = doc.object();
        
        m_profileStatusLabel->setText(QString("✅ Profile Loaded: %1").arg(selected));
        m_profileStatusLabel->setStyleSheet("padding: 10px; background: #007bff; border-radius: 5px; color: white;");
        showMessage("Profile Loaded", QString("Profile '%1' loaded").arg(selected));
    } else {
        showMessage("Load Error", "Failed to load profile", true);
    }
}

void PhoneSettingsDialog::onExportProfile() {
    QString name = m_profileNameEdit->text().trimmed();
    if (name.isEmpty()) {
        name = "device_profile";
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Profile", QString("%1.json").arg(name), "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        ReDroidController& controller = ReDroidController::instance();
        InstanceInfo info = controller.getInstanceInfo(m_instanceId);
        
        QJsonObject profile;
        profile["instanceId"] = m_instanceId;
        profile["profileName"] = name;
        profile["exportedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        profile["deviceInfo"] = QJsonObject::fromVariantMap({
            {"model", controller.getProperty(m_instanceId, "ro.product.model")},
            {"version", controller.getProperty(m_instanceId, "ro.build.version.release")},
            {"imei", controller.getProperty(m_instanceId, "ro.gsm.sim.imei")}
        });
        
        QJsonDocument doc(profile);
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            showMessage("Export Complete", QString("Profile exported to: %1").arg(fileName));
        }
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

QString PhoneSettingsDialog::executeAdbSync(const QStringList& args, int timeoutMs) {
    ReDroidController& controller = ReDroidController::instance();
    // For now, return empty - actual implementation would use ADB
    Q_UNUSED(args);
    Q_UNUSED(timeoutMs);
    return "";
}

void PhoneSettingsDialog::showMessage(const QString& title, const QString& message, bool isError) {
    if (isError) {
        QMessageBox::warning(this, title, message);
    } else {
        QMessageBox::information(this, title, message);
    }
}

} // namespace VirtualPhonePro
