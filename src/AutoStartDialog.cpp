/**
 * @file AutoStartDialog.cpp
 * @brief Auto-Start Settings Dialog Implementation
 * @version 2.0.0
 */

#include "AutoStartDialog.h"
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

using namespace VirtualPhonePro;

AutoStartDialog::AutoStartDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Auto-Start Settings");
    setModal(true);
    setMinimumWidth(500);
    setupUI();
    loadSettings();
}

AutoStartDialog::~AutoStartDialog() {
}

void AutoStartDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Auto-start settings group
    QGroupBox* autoStartGroup = new QGroupBox("Auto-Start Settings", this);
    QVBoxLayout* autoStartLayout = new QVBoxLayout(autoStartGroup);
    
    m_autoStartCheck = new QCheckBox("Enable auto-start for containers", this);
    m_autoStartCheck->setToolTip("Automatically start containers when the application launches");
    autoStartLayout->addWidget(m_autoStartCheck);
    
    m_restoreOnStartupCheck = new QCheckBox("Restore previously running instances", this);
    m_restoreOnStartupCheck->setToolTip("Restore and start containers that were running when the app was closed");
    autoStartLayout->addWidget(m_restoreOnStartupCheck);
    
    QFormLayout* formLayout = new QFormLayout();
    m_maxInstancesSpin = new QSpinBox(this);
    m_maxInstancesSpin->setRange(1, 20);
    m_maxInstancesSpin->setValue(5);
    m_maxInstancesSpin->setToolTip("Maximum number of instances to auto-start");
    formLayout->addRow("Max instances:", m_maxInstancesSpin);
    autoStartLayout->addLayout(formLayout);
    
    mainLayout->addWidget(autoStartGroup);
    
    // ADB settings group
    QGroupBox* adbGroup = new QGroupBox("ADB Settings", this);
    QVBoxLayout* adbLayout = new QVBoxLayout(adbGroup);
    
    m_autoConnectAdbCheck = new QCheckBox("Auto-connect ADB on container start", this);
    m_autoConnectAdbCheck->setToolTip("Automatically connect to containers via ADB when they start");
    adbLayout->addWidget(m_autoConnectAdbCheck);
    
    mainLayout->addWidget(adbGroup);
    
    // Spoofing settings group
    QGroupBox* spoofGroup = new QGroupBox("Spoofing Settings", this);
    QVBoxLayout* spoofLayout = new QVBoxLayout(spoofGroup);
    
    m_autoSpoofCheck = new QCheckBox("Auto-apply SafetyNet spoofing", this);
    m_autoSpoofCheck->setToolTip("Automatically apply SafetyNet/Play Integrity spoofing when containers start");
    spoofLayout->addWidget(m_autoSpoofCheck);
    
    mainLayout->addWidget(spoofGroup);
    
    // Saved instances group
    QGroupBox* instancesGroup = new QGroupBox("Saved Instances", this);
    QVBoxLayout* instancesLayout = new QVBoxLayout(instancesGroup);
    
    m_instanceList = new QListWidget(this);
    m_instanceList->setMaximumHeight(150);
    instancesLayout->addWidget(m_instanceList);
    
    QHBoxLayout* instancesBtnLayout = new QHBoxLayout();
    QPushButton* refreshBtn = new QPushButton("Refresh List", this);
    connect(refreshBtn, &QPushButton::clicked, this, &AutoStartDialog::onLoadInstances);
    QPushButton* clearBtn = new QPushButton("Clear All", this);
    connect(clearBtn, &QPushButton::clicked, [this]() {
        m_instanceList->clear();
        QMessageBox::information(this, "Cleared", "All saved instances have been cleared.");
    });
    instancesBtnLayout->addWidget(refreshBtn);
    instancesBtnLayout->addWidget(clearBtn);
    instancesBtnLayout->addStretch();
    instancesLayout->addLayout(instancesBtnLayout);
    
    mainLayout->addWidget(instancesGroup);
    
    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &AutoStartDialog::onSaveSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    // Load instances
    onLoadInstances();
}

void AutoStartDialog::loadSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                     "VirtualPhonePro", "Settings");
    
    m_autoStartCheck->setChecked(settings.value("autoStart/enabled", true).toBool());
    m_restoreOnStartupCheck->setChecked(settings.value("autoStart/restoreOnStartup", true).toBool());
    m_maxInstancesSpin->setValue(settings.value("autoStart/maxInstances", 5).toInt());
    m_autoConnectAdbCheck->setChecked(settings.value("autoStart/autoConnectAdb", true).toBool());
    m_autoSpoofCheck->setChecked(settings.value("autoStart/autoSpoof", true).toBool());
}

void AutoStartDialog::saveSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                     "VirtualPhonePro", "Settings");
    
    settings.beginGroup("autoStart");
    settings.setValue("enabled", m_autoStartCheck->isChecked());
    settings.setValue("restoreOnStartup", m_restoreOnStartupCheck->isChecked());
    settings.setValue("maxInstances", m_maxInstancesSpin->value());
    settings.setValue("autoConnectAdb", m_autoConnectAdbCheck->isChecked());
    settings.setValue("autoSpoof", m_autoSpoofCheck->isChecked());
    settings.endGroup();
    
    settings.sync();
}

void AutoStartDialog::onSaveSettings() {
    saveSettings();
    accept();
    
    QMessageBox::information(this, "Settings Saved", 
        "Auto-start settings have been saved.\n"
        "Changes will take effect on next application restart.");
}

void AutoStartDialog::onLoadInstances() {
    m_instanceList->clear();
    
    QString configDir = QStandardPaths::writableLocation(QSettings::IniFormat == QSettings::NativeFormat 
                                                         ? QStandardPaths::AppConfigLocation 
                                                         : QStandardPaths::AppConfigLocation);
    
    QString instancesFile = configDir + "/saved_instances.json";
    
    QFile file(instancesFile);
    if (!file.open(QIODevice::ReadOnly)) {
        m_instanceList->addItem("No saved instances found");
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        m_instanceList->addItem("Error reading saved instances");
        return;
    }
    
    QJsonObject json = doc.object();
    QJsonArray instances = json["instances"].toArray();
    
    if (instances.isEmpty()) {
        m_instanceList->addItem("No saved instances found");
        return;
    }
    
    for (const QJsonValue& value : instances) {
        QJsonObject instance = value.toObject();
        QString instanceId = instance["instanceId"].toString();
        QString savedAt = instance["savedAt"].toString();
        
        if (!instanceId.isEmpty()) {
            m_instanceList->addItem(QString("%1 (saved: %2)").arg(instanceId).arg(savedAt));
        }
    }
}
