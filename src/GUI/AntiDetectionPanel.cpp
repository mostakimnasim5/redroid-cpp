/**
 * @file AntiDetectionPanel.cpp
 * @brief Anti-Detection Control Panel Implementation
 */

#include "AntiDetectionPanel.h"

#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/RealPhoneHardening.hpp"
#include "VirtualPhonePro/HardwareFingerprintSpoofer.h"
#include "VirtualPhonePro/BankingAppSpoofer.h"
#include "VirtualPhonePro/SafetyNetAdvancedBypass.hpp"
#include "VirtualPhonePro/EmulatorDetectionBypass.hpp"
#include "VirtualPhonePro/TLSFingerprint.hpp"
#include "VirtualPhonePro/SELinuxManager.hpp"
#include "VirtualPhonePro/PlayIntegrityManager.hpp"
#include "VirtualPhonePro/UniqueDeviceGenerator.h"

#include <QHeaderView>
#include <QCheckBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QDebug>
#include <QJsonObject>

namespace VirtualPhonePro {

// ============================================================================
// JSON Conversion
// ============================================================================

QJsonObject AntiDetectionModuleStatus::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["icon"] = icon;
    obj["isActive"] = isActive;
    obj["isEnabled"] = isEnabled;
    obj["status"] = status;
    obj["description"] = description;
    return obj;
}

// ============================================================================
// AntiDetectionManager Implementation
// ============================================================================

AntiDetectionManager* AntiDetectionManager::s_instance = nullptr;

AntiDetectionManager& AntiDetectionManager::instance() {
    if (!s_instance) {
        s_instance = new AntiDetectionManager();
    }
    return *s_instance;
}

AntiDetectionManager::AntiDetectionManager(QObject* parent)
    : QObject(parent)
{
}

bool AntiDetectionManager::applyCompleteProtection(const QString& instanceId, const DeviceProfile& profile) {
    qDebug() << "[AntiDetection] Applying complete protection to:" << instanceId;
    
    bool allSuccess = true;
    int successCount = 0;
    int totalCount = 7;
    
    // Step 1: RealPhoneHardening - Apply hardware profile
    qDebug() << "[AntiDetection] Step 1/7: Applying RealPhoneHardening...";
    try {
        RealPhoneHardening& hardening = RealPhoneHardening::instance();
        hardening.applyAllHardening(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ RealPhoneHardening applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ RealPhoneHardening failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 2: HardwareFingerprintSpoofer - Spoof IMEI/Serial
    qDebug() << "[AntiDetection] Step 2/7: Applying HardwareFingerprintSpoofer...";
    try {
        HardwareFingerprintSpoofer& spoofer = HardwareFingerprintSpoofer::instance();
        spoofer.initialize(instanceId);
        spoofer.spoofDeviceIdentity(instanceId, profile);
        successCount++;
        qDebug() << "[AntiDetection] ✓ HardwareFingerprintSpoofer applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ HardwareFingerprintSpoofer failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 3: BankingAppSpoofer - Setup banking bypass
    qDebug() << "[AntiDetection] Step 3/7: Applying BankingAppSpoofer...";
    try {
        BankingAppSpoofer& spoofer = BankingAppSpoofer::instance();
        spoofer.configure(instanceId);
        spoofer.applyBankingBypass(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ BankingAppSpoofer applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ BankingAppSpoofer failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 4: SafetyNet Advanced Bypass
    qDebug() << "[AntiDetection] Step 4/7: Applying SafetyNet bypass...";
    try {
        SafetyNetAdvancedBypass& bypass = SafetyNetAdvancedBypass::instance();
        bypass.performFullBypass(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ SafetyNet bypass applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ SafetyNet bypass failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 5: EmulatorDetectionBypass - Hide emulator signs
    qDebug() << "[AntiDetection] Step 5/7: Applying EmulatorDetectionBypass...";
    try {
        EmulatorDetectionBypass& bypass = EmulatorDetectionBypass::instance();
        bypass.performCompleteBypass(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ EmulatorDetectionBypass applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ EmulatorDetectionBypass failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 6: TLS Fingerprinting
    qDebug() << "[AntiDetection] Step 6/7: Applying TLS Fingerprinting...";
    try {
        TLSFingerprint& tls = TLSFingerprint::instance();
        tls.initialize(profile.name);
        tls.applyToInstance(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ TLS Fingerprinting applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ TLS Fingerprinting failed:" << e.what();
        allSuccess = false;
    }
    
    // Step 7: SELinux Masking
    qDebug() << "[AntiDetection] Step 7/7: Applying SELinux Masking...";
    try {
        SELinuxManager& selinux = SELinuxManager::instance();
        selinux.resetConfig(instanceId);
        selinux.applyEnforcementMasking(instanceId);
        selinux.loadDefaultRules(instanceId);
        successCount++;
        qDebug() << "[AntiDetection] ✓ SELinux Masking applied";
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] ✗ SELinux Masking failed:" << e.what();
        allSuccess = false;
    }
    
    // Mark instance as protected
    m_protectedInstances[instanceId] = allSuccess;
    
    // Store protection status
    QJsonObject status;
    status["allApplied"] = allSuccess;
    status["successCount"] = successCount;
    status["totalCount"] = totalCount;
    status["successRate"] = QString::number((successCount * 100) / totalCount) + "%";
    m_protectionStatuses[instanceId] = status;
    
    emit protectionApplied(instanceId, allSuccess);
    
    qDebug() << "[AntiDetection] Protection complete:" << successCount << "/" << totalCount 
             << "modules applied" << (allSuccess ? "successfully" : "with errors");
    
    return allSuccess;
}

QJsonObject AntiDetectionManager::getProtectionStatus(const QString& instanceId) const {
    QJsonObject status;
    
    if (m_protectedInstances.contains(instanceId)) {
        status = m_protectionStatuses.value(instanceId);
        status["isProtected"] = m_protectedInstances[instanceId];
    } else {
        status["isProtected"] = false;
        status["message"] = "Not protected";
    }
    
    return status;
}

bool AntiDetectionManager::isFullyProtected(const QString& instanceId) const {
    return m_protectedInstances.value(instanceId, false);
}

QStringList AntiDetectionManager::getProtectedInstances() const {
    QStringList protectedList;
    for (auto it = m_protectedInstances.begin(); it != m_protectedInstances.end(); ++it) {
        if (it.value()) {
            protectedList.append(it.key());
        }
    }
    return protectedList;
}

// ============================================================================
// AntiDetectionPanel Implementation
// ============================================================================

AntiDetectionPanel::AntiDetectionPanel(const QString& instanceId, QWidget* parent)
    : QDialog(parent)
    , m_instanceId(instanceId)
    , m_moduleTable(nullptr)
    , m_applyAllButton(nullptr)
    , m_applySelectedButton(nullptr)
    , m_refreshButton(nullptr)
    , m_statusLabel(nullptr)
    , m_overallStatusLabel(nullptr)
    , m_statusTimer(nullptr)
    , m_autoApplyTimer(nullptr)
    , m_autoApply(true)
    , m_allApplied(false)
{
    setWindowTitle(QString("🛡️ Anti-Detection - %1").arg(instanceId));
    setMinimumSize(700, 550);
    resize(750, 600);
    setModal(false);
    
    setupUI();
    loadModuleStatus();
    
    // Start auto-refresh timer
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &AntiDetectionPanel::updateStatusTimer);
    m_statusTimer->start(5000); // Update every 5 seconds
    
    // Auto-apply on show if not already applied
    if (m_autoApply && !m_allApplied) {
        QTimer::singleShot(500, this, &AntiDetectionPanel::onApplyAllClicked);
    }
}

AntiDetectionPanel::~AntiDetectionPanel() {
    if (m_statusTimer) m_statusTimer->stop();
    if (m_autoApplyTimer) m_autoApplyTimer->stop();
}

void AntiDetectionPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Header
    QLabel* header = new QLabel("🛡️ Anti-Detection Protection Panel", this);
    header->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(header);
    
    // Instance info
    QLabel* instanceInfo = new QLabel(QString("Instance: %1").arg(m_instanceId), this);
    instanceInfo->setStyleSheet("color: #7f8c8d; font-size: 12px;");
    mainLayout->addWidget(instanceInfo);
    
    // Overall status
    m_overallStatusLabel = new QLabel("⏳ Initializing...", this);
    m_overallStatusLabel->setStyleSheet(
        "background-color: #2c3e50; color: white; padding: 10px; "
        "border-radius: 8px; font-size: 14px; font-weight: bold;"
    );
    mainLayout->addWidget(m_overallStatusLabel);
    
    // Module table
    createModuleTable();
    mainLayout->addWidget(m_moduleTable);
    
    // Control buttons
    createControlButtons();
    mainLayout->addWidget(m_applyAllButton);
    
    QHBoxLayout* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_applySelectedButton);
    buttonRow->addWidget(m_refreshButton);
    buttonRow->addStretch();
    mainLayout->addLayout(buttonRow);
    
    // Status area
    createStatusArea();
    mainLayout->addWidget(m_statusLabel);
    
    // Auto-apply checkbox
    QCheckBox* autoApplyCheck = new QCheckBox("Auto-apply protection on open", this);
    autoApplyCheck->setChecked(m_autoApply);
    connect(autoApplyCheck, &QCheckBox::toggled, this, &AntiDetectionPanel::onAutoApplyToggled);
    mainLayout->addWidget(autoApplyCheck);
}

void AntiDetectionPanel::createModuleTable() {
    m_moduleTable = new QTableWidget(this);
    m_moduleTable->setColumnCount(5);
    m_moduleTable->setHorizontalHeaderLabels({"Module", "Status", "Toggle", "Info", "Last Applied"});
    
    // Column widths
    m_moduleTable->setColumnWidth(0, 200);  // Module name
    m_moduleTable->setColumnWidth(1, 100);  // Status icon
    m_moduleTable->setColumnWidth(2, 60);    // Toggle
    m_moduleTable->setColumnWidth(3, 250);    // Info
    m_moduleTable->setColumnWidth(4, 100);   // Last applied
    
    m_moduleTable->horizontalHeader()->setStretchLastSection(true);
    m_moduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_moduleTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_moduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_moduleTable->setAlternatingRowColors(true);
    
    // Initialize modules
    m_modules = {
        {"RealPhoneHardening", {"RealPhoneHardening", "🔒", false, true, "Not Applied", "Hardware profile & system hardening"}},
        {"HardwareFingerprintSpoofer", {"HardwareFingerprintSpoofer", "📱", false, true, "Not Applied", "IMEI, Serial, MAC spoofing"}},
        {"BankingAppSpoofer", {"BankingAppSpoofer", "🏦", false, true, "Not Applied", "Banking app bypass setup"}},
        {"SafetyNetBypass", {"SafetyNetBypass", "✅", false, true, "Not Applied", "SafetyNet attestation"}},
        {"EmulatorDetectionBypass", {"EmulatorDetectionBypass", "👻", false, true, "Not Applied", "Hide emulator signatures"}},
        {"TLSFingerprinting", {"TLSFingerprinting", "🔐", false, true, "Not Applied", "TLS/SSL fingerprint spoofing"}},
        {"SELinuxMasking", {"SELinuxMasking", "🛡️", false, true, "Not Applied", "SELinux enforcement masking"}}
    };
    
    int row = 0;
    for (auto it = m_modules.begin(); it != m_modules.end(); ++it, ++row) {
        m_moduleTable->insertRow(row);
        
        const AntiDetectionModuleStatus& mod = it.value();
        
        // Module name with icon
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString("%1 %2").arg(mod.icon, mod.name));
        nameItem->setData(Qt::UserRole, it.key());
        m_moduleTable->setItem(row, 0, nameItem);
        
        // Status
        QTableWidgetItem* statusItem = new QTableWidgetItem(mod.isActive ? "🟢 Active" : "⚪ Inactive");
        statusItem->setForeground(mod.isActive ? Qt::green : Qt::gray);
        m_moduleTable->setItem(row, 1, statusItem);
        
        // Toggle checkbox
        QWidget* toggleWidget = new QWidget();
        QHBoxLayout* toggleLayout = new QHBoxLayout(toggleWidget);
        toggleLayout->setAlignment(Qt::AlignCenter);
        toggleLayout->setMargin(0);
        QCheckBox* toggle = new QCheckBox();
        toggle->setChecked(mod.isEnabled);
        toggle->setProperty("moduleName", it.key());
        connect(toggle, &QCheckBox::toggled, this, [this, toggle](bool enabled) {
            QString moduleName = toggle->property("moduleName").toString();
            onModuleToggled(moduleName, enabled);
        });
        toggleLayout->addWidget(toggle);
        m_moduleTable->setCellWidget(row, 2, toggleWidget);
        
        // Info
        QTableWidgetItem* infoItem = new QTableWidgetItem(mod.description);
        infoItem->setForeground(Qt::darkGray);
        m_moduleTable->setItem(row, 3, infoItem);
        
        // Last applied
        QTableWidgetItem* lastItem = new QTableWidgetItem(mod.status);
        m_moduleTable->setItem(row, 4, lastItem);
    }
    
    connect(m_moduleTable, &QTableWidget::itemSelectionChanged, 
            this, &AntiDetectionPanel::onModuleSelectionChanged);
}

void AntiDetectionPanel::createControlButtons() {
    m_applyAllButton = new QPushButton("🛡️ Apply All Protection Modules", this);
    m_applyAllButton->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; padding: 12px; "
        "border-radius: 8px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #2ecc71; }"
        "QPushButton:disabled { background-color: #95a5a6; }"
    );
    connect(m_applyAllButton, &QPushButton::clicked, this, &AntiDetectionPanel::onApplyAllClicked);
    
    m_applySelectedButton = new QPushButton("✓ Apply Selected", this);
    m_applySelectedButton->setEnabled(false);
    connect(m_applySelectedButton, &QPushButton::clicked, this, &AntiDetectionPanel::onApplySelectedClicked);
    
    m_refreshButton = new QPushButton("🔄 Refresh Status", this);
    connect(m_refreshButton, &QPushButton::clicked, this, &AntiDetectionPanel::onRefreshClicked);
}

void AntiDetectionPanel::createStatusArea() {
    m_statusLabel = new QLabel("Ready to apply protection...", this);
    m_statusLabel->setStyleSheet(
        "background-color: #ecf0f1; padding: 8px; border-radius: 4px; color: #7f8c8d;"
    );
}

void AntiDetectionPanel::loadModuleStatus() {
    AntiDetectionManager& manager = AntiDetectionManager::instance();
    bool isProtected = manager.isFullyProtected(m_instanceId);
    
    for (int row = 0; row < m_moduleTable->rowCount(); ++row) {
        QTableWidgetItem* nameItem = m_moduleTable->item(row, 0);
        if (nameItem) {
            QString moduleName = nameItem->data(Qt::UserRole).toString();
            bool isActive = isProtected;
            
            // Update status column
            QTableWidgetItem* statusItem = m_moduleTable->item(row, 1);
            if (statusItem) {
                statusItem->setText(isActive ? "🟢 Active" : "⚪ Inactive");
                statusItem->setForeground(isActive ? Qt::green : Qt::gray);
            }
            
            // Update last applied
            QTableWidgetItem* lastItem = m_moduleTable->item(row, 4);
            if (lastItem) {
                lastItem->setText(isActive ? QTime::currentTime().toString("HH:mm") : "Not Applied");
            }
            
            // Update module data
            if (m_modules.contains(moduleName)) {
                m_modules[moduleName].isActive = isActive;
            }
        }
    }
    
    m_allApplied = isProtected;
    updateOverallStatus();
}

void AntiDetectionPanel::onApplyAllClicked() {
    m_statusLabel->setText("⏳ Applying all protection modules...");
    m_statusLabel->setStyleSheet("background-color: #f39c12; color: white; padding: 8px; border-radius: 4px;");
    
    m_applyAllButton->setEnabled(false);
    
    // Apply in background
    QtConcurrent::run([this]() {
        bool success = AntiDetectionManager::instance().applyCompleteProtection(m_instanceId, m_profile);
        
        // Update UI in main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            m_allApplied = success;
            loadModuleStatus();
            refreshStatus();
            
            if (success) {
                m_statusLabel->setText("✅ All protection modules applied successfully!");
                m_statusLabel->setStyleSheet("background-color: #27ae60; color: white; padding: 8px; border-radius: 4px;");
                m_applyAllButton->setText("🛡️ Re-apply All (Already Protected)");
            } else {
                m_statusLabel->setText("⚠️ Some modules failed to apply. Check individual status.");
                m_statusLabel->setStyleSheet("background-color: #e74c3c; color: white; padding: 8px; border-radius: 4px;");
            }
            
            m_applyAllButton->setEnabled(true);
            emit antiDetectionApplied(m_instanceId, success);
        });
    });
}

void AntiDetectionPanel::onApplySelectedClicked() {
    QList<QTableWidgetItem*> selected = m_moduleTable->selectedItems();
    if (selected.isEmpty()) return;
    
    int row = selected.first()->row();
    QTableWidgetItem* nameItem = m_moduleTable->item(row, 0);
    if (nameItem) {
        QString moduleName = nameItem->data(Qt::UserRole).toString();
        applyModule(moduleName, true);
    }
}

void AntiDetectionPanel::onModuleToggled(const QString& moduleName, bool enabled) {
    m_modules[moduleName].isEnabled = enabled;
    qDebug() << "[AntiDetection] Module toggled:" << moduleName << enabled;
}

void AntiDetectionPanel::onRefreshClicked() {
    refreshStatus();
    loadModuleStatus();
}

void AntiDetectionPanel::onModuleSelectionChanged() {
    QList<QTableWidgetItem*> selected = m_moduleTable->selectedItems();
    m_applySelectedButton->setEnabled(!selected.isEmpty());
}

void AntiDetectionPanel::onAutoApplyToggled(bool enabled) {
    m_autoApply = enabled;
}

void AntiDetectionPanel::updateStatusTimer() {
    if (m_allApplied) {
        // Verify modules are still active
        loadModuleStatus();
    }
}

void AntiDetectionPanel::refreshStatus() {
    AntiDetectionManager& manager = AntiDetectionManager::instance();
    bool isProtected = manager.isFullyProtected(m_instanceId);
    
    updateOverallStatus();
    
    for (int row = 0; row < m_moduleTable->rowCount(); ++row) {
        QTableWidgetItem* nameItem = m_moduleTable->item(row, 0);
        if (nameItem) {
            QString moduleName = nameItem->data(Qt::UserRole).toString();
            
            QTableWidgetItem* statusItem = m_moduleTable->item(row, 1);
            if (statusItem) {
                statusItem->setText(isProtected ? "🟢 Active" : "⚪ Inactive");
                statusItem->setForeground(isProtected ? Qt::green : Qt::gray);
            }
        }
    }
}

void AntiDetectionPanel::updateOverallStatus() {
    if (m_allApplied) {
        m_overallStatusLabel->setText("🛡️ FULLY PROTECTED - All modules active");
        m_overallStatusLabel->setStyleSheet(
            "background-color: #27ae60; color: white; padding: 10px; "
            "border-radius: 8px; font-size: 14px; font-weight: bold;"
        );
    } else {
        int activeCount = 0;
        for (const auto& mod : m_modules) {
            if (mod.isActive) activeCount++;
        }
        m_overallStatusLabel->setText(
            QString("⚠️ PARTIALLY PROTECTED - %1/%2 modules active")
                .arg(activeCount).arg(m_modules.size())
        );
        m_overallStatusLabel->setStyleSheet(
            "background-color: #f39c12; color: white; padding: 10px; "
            "border-radius: 8px; font-size: 14px; font-weight: bold;"
        );
    }
}

void AntiDetectionPanel::applyAllModules() {
    onApplyAllClicked();
}

bool AntiDetectionPanel::applyModule(const QString& moduleName, bool enabled) {
    if (!enabled) {
        m_statusLabel->setText(QString("Module %1 disabled").arg(moduleName));
        return true;
    }
    
    m_statusLabel->setText(QString("Applying %1...").arg(moduleName));
    qDebug() << "[AntiDetection] Applying module:" << moduleName;
    
    bool success = false;
    
    if (moduleName == "RealPhoneHardening") {
        success = applyRealPhoneHardening();
    } else if (moduleName == "HardwareFingerprintSpoofer") {
        success = applyHardwareFingerprintSpoofing();
    } else if (moduleName == "BankingAppSpoofer") {
        success = applyBankingAppSpoofing();
    } else if (moduleName == "SafetyNetBypass") {
        success = applySafetyNetBypass();
    } else if (moduleName == "EmulatorDetectionBypass") {
        success = applyEmulatorDetectionBypass();
    } else if (moduleName == "TLSFingerprinting") {
        success = applyTLSFingerprinting();
    } else if (moduleName == "SELinuxMasking") {
        success = applySELinuxMasking();
    }
    
    if (success) {
        m_modules[moduleName].isActive = true;
        m_modules[moduleName].status = QTime::currentTime().toString("HH:mm");
        loadModuleStatus();
    }
    
    return success;
}

bool AntiDetectionPanel::applyRealPhoneHardening() {
    try {
        RealPhoneHardening::instance().applyAllHardening(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] RealPhoneHardening failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applyHardwareFingerprintSpoofing() {
    try {
        HardwareFingerprintSpoofer& spoofer = HardwareFingerprintSpoofer::instance();
        spoofer.initialize(m_instanceId);
        spoofer.spoofDeviceIdentity(m_instanceId, m_profile);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] HardwareFingerprintSpoofer failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applyBankingAppSpoofing() {
    try {
        BankingAppSpoofer::instance().configure(m_instanceId);
        BankingAppSpoofer::instance().applyBankingBypass(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] BankingAppSpoofer failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applySafetyNetBypass() {
    try {
        SafetyNetAdvancedBypass::instance().performFullBypass(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] SafetyNet bypass failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applyEmulatorDetectionBypass() {
    try {
        EmulatorDetectionBypass::instance().performCompleteBypass(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] EmulatorDetectionBypass failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applyTLSFingerprinting() {
    try {
        TLSFingerprint::instance().initialize(m_profile.name);
        TLSFingerprint::instance().applyToInstance(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] TLS Fingerprinting failed:" << e.what();
        return false;
    }
}

bool AntiDetectionPanel::applySELinuxMasking() {
    try {
        SELinuxManager::instance().resetConfig(m_instanceId);
        SELinuxManager::instance().applyEnforcementMasking(m_instanceId);
        SELinuxManager::instance().loadDefaultRules(m_instanceId);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[AntiDetection] SELinux Masking failed:" << e.what();
        return false;
    }
}

} // namespace VirtualPhonePro
