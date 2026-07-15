/**
 * @file anti_detection_panel.cpp
 * @brief Anti-Detection Panel Implementation
 */

#include "anti_detection_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

AntiDetectionPanel::AntiDetectionPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

AntiDetectionPanel::~AntiDetectionPanel()
{
}

void AntiDetectionPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Title
    QLabel* title = new QLabel("🛡️ Anti-Detection");
    title->setStyleSheet("color: #00d4ff; font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(title);
    
    // Quick Actions
    QHBoxLayout* quickLayout = new QHBoxLayout();
    
    m_bankingBtn = new QPushButton("🏦 Banking");
    m_bankingBtn->setStyleSheet(R"(
        QPushButton {
            background: #aa55ff;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px;
        }
        QPushButton:hover {
            background: #bb66ff;
        }
    )");
    quickLayout->addWidget(m_bankingBtn);
    
    m_googleBtn = new QPushButton("🔍 Google");
    m_googleBtn->setStyleSheet(R"(
        QPushButton {
            background: #00d4ff;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px;
        }
        QPushButton:hover {
            background: #00e5ff;
        }
    )");
    quickLayout->addWidget(m_googleBtn);
    
    mainLayout->addLayout(quickLayout);
    
    // Core Modules
    QGroupBox* coreGroup = new QGroupBox("Core Modules");
    coreGroup->setStyleSheet(R"(
        QGroupBox {
            color: #00d4ff;
            border: 1px solid #0f3460;
            border-radius: 8px;
            margin-top: 10px;
        }
    )");
    QVBoxLayout* coreLayout = new QVBoxLayout(coreGroup);
    coreLayout->setSpacing(5);
    
    m_hypervisorBypass = new QCheckBox("🖥️ Hypervisor Bypass");
    m_safetyNetBypass = new QCheckBox("🔐 SafetyNet Bypass");
    m_realPhoneHardening = new QCheckBox("📱 Real Phone Hardening");
    m_timingPrevention = new QCheckBox("⏱️ Timing Prevention");
    m_playIntegrity = new QCheckBox("✅ Play Integrity");
    m_trustZoneEmu = new QCheckBox("🔒 TrustZone Emulation");
    
    // Set defaults
    m_hypervisorBypass->setChecked(true);
    m_safetyNetBypass->setChecked(true);
    m_realPhoneHardening->setChecked(true);
    m_timingPrevention->setChecked(true);
    m_playIntegrity->setChecked(true);
    m_trustZoneEmu->setChecked(true);
    
    coreLayout->addWidget(m_hypervisorBypass);
    coreLayout->addWidget(m_safetyNetBypass);
    coreLayout->addWidget(m_realPhoneHardening);
    coreLayout->addWidget(m_timingPrevention);
    coreLayout->addWidget(m_playIntegrity);
    coreLayout->addWidget(m_trustZoneEmu);
    
    mainLayout->addWidget(coreGroup);
    
    // Banking Modules
    QGroupBox* bankingGroup = new QGroupBox("Banking Modules");
    bankingGroup->setStyleSheet(R"(
        QGroupBox {
            color: #aa55ff;
            border: 1px solid #6020f0;
            border-radius: 8px;
            margin-top: 10px;
        }
    )");
    QVBoxLayout* bankingLayout = new QVBoxLayout(bankingGroup);
    bankingLayout->setSpacing(5);
    
    m_bankingSpoofer = new QCheckBox("🏦 Banking App Spoofer");
    m_canvasSpoof = new QCheckBox("🎨 Canvas Spoofing");
    m_webglSpoof = new QCheckBox("🌐 WebGL Hardening");
    
    m_bankingSpoofer->setChecked(true);
    m_canvasSpoof->setChecked(true);
    m_webglSpoof->setChecked(true);
    
    bankingLayout->addWidget(m_bankingSpoofer);
    bankingLayout->addWidget(m_canvasSpoof);
    bankingLayout->addWidget(m_webglSpoof);
    
    mainLayout->addWidget(bankingGroup);
    
    // Hardware/Network
    QGroupBox* hwGroup = new QGroupBox("Hardware/Network");
    hwGroup->setStyleSheet(R"(
        QGroupBox {
            color: #00ff00;
            border: 1px solid #00aa00;
            border-radius: 8px;
            margin-top: 10px;
        }
    )");
    QVBoxLayout* hwLayout = new QVBoxLayout(hwGroup);
    hwLayout->setSpacing(5);
    
    m_hardwareSpoofer = new QCheckBox("💻 Hardware Spoofer");
    m_networkSpoofer = new QCheckBox("🌐 Network Spoofer");
    m_tlsFingerprint = new QCheckBox("🔑 TLS Fingerprint");
    m_googleSpoofer = new QCheckBox("🔍 Google/Facebook Spoofer");
    
    m_hardwareSpoofer->setChecked(true);
    m_networkSpoofer->setChecked(true);
    m_tlsFingerprint->setChecked(true);
    
    hwLayout->addWidget(m_hardwareSpoofer);
    hwLayout->addWidget(m_networkSpoofer);
    hwLayout->addWidget(m_tlsFingerprint);
    hwLayout->addWidget(m_googleSpoofer);
    
    mainLayout->addWidget(hwGroup);
    
    // Integrity Level
    QGroupBox* integrityGroup = new QGroupBox("Integrity Settings");
    integrityGroup->setStyleSheet(R"(
        QGroupBox {
            color: #ff6600;
            border: 1px solid #aa4400;
            border-radius: 8px;
            margin-top: 10px;
        }
    )");
    QVBoxLayout* intLayout = new QVBoxLayout(integrityGroup);
    
    QHBoxLayout* levelLayout = new QHBoxLayout();
    QLabel* levelLabel = new QLabel("Level:");
    levelLabel->setStyleSheet("color: #ffffff;");
    levelLayout->addWidget(levelLabel);
    m_integrityLevel = new QComboBox();
    m_integrityLevel->addItems({"Basic", "Device", "Strong", "Certified"});
    m_integrityLevel->setCurrentIndex(1);
    m_integrityLevel->setStyleSheet(R"(
        QComboBox {
            background: #333;
            color: #ffffff;
            border: 1px solid #555;
            border-radius: 5px;
            padding: 5px;
        }
    )");
    levelLayout->addWidget(m_integrityLevel);
    intLayout->addLayout(levelLayout);
    
    QHBoxLayout* avoidLayout = new QHBoxLayout();
    QLabel* avoidLabel = new QLabel("Target:");
    avoidLabel->setStyleSheet("color: #ffffff;");
    avoidLayout->addWidget(avoidLabel);
    m_detectionAvoidance = new QSpinBox();
    m_detectionAvoidance->setRange(50, 100);
    m_detectionAvoidance->setValue(98);
    m_detectionAvoidance->setSuffix("%");
    m_detectionAvoidance->setStyleSheet(R"(
        QSpinBox {
            background: #333;
            color: #00d4ff;
            border: 1px solid #555;
            border-radius: 5px;
            padding: 5px;
        }
    )");
    avoidLayout->addWidget(m_detectionAvoidance);
    intLayout->addLayout(avoidLayout);
    
    mainLayout->addWidget(integrityGroup);
    
    // Apply Button
    m_applyBtn = new QPushButton("🛡️ Apply Anti-Detection");
    m_applyBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00d4ff, stop:1 #0099cc);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00e5ff, stop:1 #00b8e6);
        }
    )");
    connect(m_applyBtn, &QPushButton::clicked, this, &AntiDetectionPanel::onApplyClicked);
    mainLayout->addWidget(m_applyBtn);
    
    mainLayout->addStretch();
}

void AntiDetectionPanel::onApplyClicked()
{
    emit applyRequested("");
}

void AntiDetectionPanel::onModuleToggled(bool checked)
{
    // Handle module toggle
}

QList<QString> AntiDetectionPanel::getSelectedModules()
{
    QList<QString> modules;
    if (m_hypervisorBypass->isChecked()) modules << "HypervisorBypass";
    if (m_safetyNetBypass->isChecked()) modules << "SafetyNetBypass";
    if (m_realPhoneHardening->isChecked()) modules << "RealPhoneHardening";
    if (m_timingPrevention->isChecked()) modules << "TimingPrevention";
    if (m_playIntegrity->isChecked()) modules << "PlayIntegrity";
    if (m_trustZoneEmu->isChecked()) modules << "TrustZoneEmu";
    if (m_bankingSpoofer->isChecked()) modules << "BankingSpoofer";
    if (m_canvasSpoof->isChecked()) modules << "CanvasSpoof";
    if (m_webglSpoof->isChecked()) modules << "WebGLSpoof";
    if (m_hardwareSpoofer->isChecked()) modules << "HardwareSpoofer";
    if (m_networkSpoofer->isChecked()) modules << "NetworkSpoofer";
    if (m_tlsFingerprint->isChecked()) modules << "TLSFingerprint";
    if (m_googleSpoofer->isChecked()) modules << "GoogleSpoofer";
    return modules;
}

int AntiDetectionPanel::getIntegrityLevel()
{
    return m_integrityLevel->currentIndex() + 1;
}

bool AntiDetectionPanel::isFullBypassEnabled()
{
    return m_hypervisorBypass->isChecked() && 
           m_safetyNetBypass->isChecked() && 
           m_realPhoneHardening->isChecked();
}
