/**
 * @file admin_control_panel.cpp
 * @brief Admin Control Panel Implementation
 * @version 3.0.0
 * 
 * User-friendly admin panel for managing device profiles and users.
 * Features large, clear fields with proper validation.
 */

#include "admin_control_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QDialogButtonBox>
#include <QScrollArea>

AdminControlPanel::AdminControlPanel(QWidget* parent)
    : QDialog(parent)
    , m_isSubmitting(false) {
    
    setWindowTitle("Admin Control Panel - ReDroidCPP");
    setModal(true);
    resize(700, 650);
    setMinimumSize(600, 550);
    
    // Modern styling
    setStyleSheet(R"(
        QDialog {
            background-color: #1a1a2e;
            color: #ffffff;
        }
        QGroupBox {
            font-weight: bold;
            font-size: 14px;
            border: 2px solid #4a4a6a;
            border-radius: 8px;
            margin-top: 12px;
            padding: 15px;
            background-color: #16213e;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 10px;
            color: #00d4ff;
        }
        QLabel {
            color: #e0e0e0;
            font-size: 13px;
        }
        QLineEdit {
            padding: 12px;
            border: 2px solid #4a4a6a;
            border-radius: 6px;
            background-color: #0f0f23;
            color: #ffffff;
            font-size: 14px;
            min-height: 25px;
        }
        QLineEdit:focus {
            border-color: #00d4ff;
            background-color: #1a1a3e;
        }
        QLineEdit:disabled {
            background-color: #2a2a4a;
            color: #888888;
        }
        QSpinBox {
            padding: 12px;
            border: 2px solid #4a4a6a;
            border-radius: 6px;
            background-color: #0f0f23;
            color: #ffffff;
            font-size: 14px;
            min-height: 25px;
        }
        QSpinBox:focus {
            border-color: #00d4ff;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #4a4a6a;
            border-radius: 3px;
        }
        QComboBox {
            padding: 12px;
            border: 2px solid #4a4a6a;
            border-radius: 6px;
            background-color: #0f0f23;
            color: #ffffff;
            font-size: 14px;
            min-height: 25px;
        }
        QComboBox:focus {
            border-color: #00d4ff;
        }
        QComboBox::drop-down {
            border: none;
            width: 30px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 8px solid #00d4ff;
        }
        QPushButton {
            padding: 14px 28px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#submitButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00d4ff, stop:1 #0099cc);
            color: white;
        }
        QPushButton#submitButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00e5ff, stop:1 #00b8e6);
        }
        QPushButton#submitButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #007799, stop:1 #005577);
        }
        QPushButton#submitButton:disabled {
            background: #4a4a6a;
            color: #888888;
        }
        QPushButton#cancelButton {
            background-color: #4a4a6a;
            color: white;
        }
        QPushButton#cancelButton:hover {
            background-color: #5a5a7a;
        }
        QPushButton#clearButton {
            background-color: #ff6b6b;
            color: white;
        }
        QPushButton#clearButton:hover {
            background-color: #ff8888;
        }
        QScrollArea {
            background-color: transparent;
            border: none;
        }
        QStatusLabel {
            color: #888888;
            font-size: 12px;
        }
    )");
    
    setupUI();
    loadPreviousData();
}

AdminControlPanel::~AdminControlPanel() {
}

void AdminControlPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel* titleLabel = new QLabel("👤 User Configuration", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #00d4ff; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Scroll area for content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget* scrollContent = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(20);
    
    // ========================================================================
    // User Information Group
    // ========================================================================
    QGroupBox* userGroup = new QGroupBox("User Information", this);
    QFormLayout* userFormLayout = new QFormLayout(userGroup);
    userFormLayout->setSpacing(15);
    userFormLayout->setLabelAlignment(Qt::AlignLeft);
    
    // User Name - Large input field
    m_userNameEdit = new QLineEdit(this);
    m_userNameEdit->setPlaceholderText("Enter your full name");
    m_userNameEdit->setMinimumWidth(400);
    m_userNameEdit->setMaxLength(100);
    m_userNameEdit->setStyleSheet("font-size: 16px; padding: 15px;");
    connect(m_userNameEdit, &QLineEdit::textChanged, this, &AdminControlPanel::validateForm);
    
    QLabel* nameLabel = new QLabel("Full Name:", this);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    userFormLayout->addRow(nameLabel, m_userNameEdit);
    
    // Phone Number - Large input field
    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText("Enter phone number (e.g., +8801XXXXXXXXX)");
    m_phoneEdit->setMinimumWidth(400);
    m_phoneEdit->setMaxLength(20);
    m_phoneEdit->setStyleSheet("font-size: 16px; padding: 15px;");
    m_phoneEdit->setInputMask("+999999999999999;_");
    connect(m_phoneEdit, &QLineEdit::textChanged, this, &AdminControlPanel::validateForm);
    
    QLabel* phoneLabel = new QLabel("Phone Number:", this);
    phoneLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    userFormLayout->addRow(phoneLabel, m_phoneEdit);
    
    // Email (optional)
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("Enter email address (optional)");
    m_emailEdit->setMinimumWidth(400);
    m_emailEdit->setMaxLength(150);
    m_emailEdit->setStyleSheet("font-size: 16px; padding: 15px;");
    
    QLabel* emailLabel = new QLabel("Email:", this);
    emailLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    userFormLayout->addRow(emailLabel, m_emailEdit);
    
    scrollLayout->addWidget(userGroup);
    
    // ========================================================================
    // Profile Configuration Group
    // ========================================================================
    QGroupBox* profileGroup = new QGroupBox("Profile Configuration", this);
    QFormLayout* profileFormLayout = new QFormLayout(profileGroup);
    profileFormLayout->setSpacing(15);
    
    // Number of Profiles - Large spinbox
    m_profileCountSpin = new QSpinBox(this);
    m_profileCountSpin->setRange(1, 100);
    m_profileCountSpin->setValue(1);
    m_profileCountSpin->setMinimumWidth(400);
    m_profileCountSpin->setStyleSheet("font-size: 18px; padding: 15px;");
    m_profileCountSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    
    QLabel* profileCountLabel = new QLabel("Number of Profiles:", this);
    profileCountLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    profileFormLayout->addRow(profileCountLabel, m_profileCountSpin);
    
    // Profile Type
    m_profileTypeCombo = new QComboBox(this);
    m_profileTypeCombo->addItem("Samsung Galaxy S24 Ultra", "samsung_s24_ultra");
    m_profileTypeCombo->addItem("Google Pixel 8 Pro", "pixel_8_pro");
    m_profileTypeCombo->addItem("Xiaomi 14", "xiaomi_14");
    m_profileTypeCombo->addItem("OnePlus 12", "oneplus_12");
    m_profileTypeCombo->addItem("Huawei P60 Pro", "huawei_p60_pro");
    m_profileTypeCombo->addItem("Custom Profile", "custom");
    m_profileTypeCombo->setMinimumWidth(400);
    m_profileTypeCombo->setStyleSheet("font-size: 16px; padding: 15px;");
    
    QLabel* profileTypeLabel = new QLabel("Device Type:", this);
    profileTypeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    profileFormLayout->addRow(profileTypeLabel, m_profileTypeCombo);
    
    // Profile Duration
    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(1, 365);
    m_durationSpin->setValue(30);
    m_durationSpin->setMinimumWidth(400);
    m_durationSpin->setStyleSheet("font-size: 18px; padding: 15px;");
    m_durationSpin->setSuffix(" days");
    m_durationSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    
    QLabel* durationLabel = new QLabel("Duration (days):", this);
    durationLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    profileFormLayout->addRow(durationLabel, m_durationSpin);
    
    // Auto-renewal checkbox
    m_autoRenewCheck = new QComboBox(this);
    m_autoRenewCheck->addItem("No Auto-Renewal", 0);
    m_autoRenewCheck->addItem("Auto-Renew 7 days before expiry", 7);
    m_autoRenewCheck->addItem("Auto-Renew 1 day before expiry", 1);
    m_autoRenewCheck->addItem("Auto-Renew on expiry day", 0);
    m_autoRenewCheck->setMinimumWidth(400);
    m_autoRenewCheck->setStyleSheet("font-size: 16px; padding: 15px;");
    
    QLabel* autoRenewLabel = new QLabel("Auto-Renewal:", this);
    autoRenewLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #00d4ff;");
    profileFormLayout->addRow(autoRenewLabel, m_autoRenewCheck);
    
    scrollLayout->addWidget(profileGroup);
    
    // ========================================================================
    // Notes Group
    // ========================================================================
    QGroupBox* notesGroup = new QGroupBox("Additional Notes", this);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);
    
    m_notesEdit = new QLineEdit(this);
    m_notesEdit->setPlaceholderText("Add any additional notes or instructions (optional)");
    m_notesEdit->setMinimumWidth(400);
    m_notesEdit->setStyleSheet("font-size: 14px; padding: 15px;");
    
    notesLayout->addWidget(m_notesEdit);
    
    scrollLayout->addWidget(notesGroup);
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    
    // ========================================================================
    // Buttons
    // ========================================================================
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    // Clear Button
    QPushButton* clearBtn = new QPushButton("🗑️ Clear All", this);
    clearBtn->setObjectName("clearButton");
    clearBtn->setMinimumWidth(120);
    connect(clearBtn, &QPushButton::clicked, this, &AdminControlPanel::onClearForm);
    buttonLayout->addWidget(clearBtn);
    
    buttonLayout->addStretch();
    
    // Cancel Button
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelButton");
    cancelBtn->setMinimumWidth(120);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);
    
    // Submit Button
    m_submitButton = new QPushButton("✅ Submit Configuration", this);
    m_submitButton->setObjectName("submitButton");
    m_submitButton->setMinimumWidth(200);
    m_submitButton->setEnabled(false);
    connect(m_submitButton, &QPushButton::clicked, this, &AdminControlPanel::onSubmit);
    buttonLayout->addWidget(m_submitButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Status bar
    m_statusLabel = new QLabel("Please fill in all required fields", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 5px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
}

void AdminControlPanel::validateForm() {
    bool isValid = true;
    QString statusMsg;
    
    // Validate User Name
    QString userName = m_userNameEdit->text().trimmed();
    if (userName.isEmpty()) {
        isValid = false;
        statusMsg = "Please enter your name";
    } else if (userName.length() < 2) {
        isValid = false;
        statusMsg = "Name must be at least 2 characters";
    } else if (userName.length() > 100) {
        isValid = false;
        statusMsg = "Name is too long (max 100 characters)";
    }
    
    // Validate Phone Number
    QString phone = m_phoneEdit->text().trimmed();
    if (phone.isEmpty()) {
        isValid = false;
        statusMsg = "Please enter your phone number";
    } else {
        // Remove all non-digit characters for validation
        QString digitsOnly = phone.remove(QRegExp("[^0-9]"));
        if (digitsOnly.length() < 10) {
            isValid = false;
            statusMsg = "Phone number must have at least 10 digits";
        } else if (digitsOnly.length() > 15) {
            isValid = false;
            statusMsg = "Phone number is too long (max 15 digits)";
        }
    }
    
    // Validate Email (if provided)
    QString email = m_emailEdit->text().trimmed();
    if (!email.isEmpty()) {
        QRegExp emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
        if (!emailRegex.exactMatch(email)) {
            isValid = false;
            statusMsg = "Please enter a valid email address";
        }
    }
    
    // Validate Profile Count
    int profileCount = m_profileCountSpin->value();
    if (profileCount < 1 || profileCount > 100) {
        isValid = false;
        statusMsg = "Profile count must be between 1 and 100";
    }
    
    // Validate Duration
    int duration = m_durationSpin->value();
    if (duration < 1 || duration > 365) {
        isValid = false;
        statusMsg = "Duration must be between 1 and 365 days";
    }
    
    m_submitButton->setEnabled(isValid);
    
    if (isValid) {
        statusMsg = QString("Ready to submit: %1 profile(s) for %2 days")
                        .arg(profileCount)
                        .arg(duration);
        m_statusLabel->setStyleSheet("color: #00ff00; font-size: 12px; padding: 5px;");
    } else {
        m_statusLabel->setStyleSheet("color: #ff6b6b; font-size: 12px; padding: 5px;");
    }
    
    m_statusLabel->setText(statusMsg);
}

void AdminControlPanel::onClearForm() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear Form",
        "Are you sure you want to clear all fields?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_userNameEdit->clear();
        m_phoneEdit->clear();
        m_emailEdit->clear();
        m_profileCountSpin->setValue(1);
        m_profileTypeCombo->setCurrentIndex(0);
        m_durationSpin->setValue(30);
        m_autoRenewCheck->setCurrentIndex(0);
        m_notesEdit->clear();
        
        m_statusLabel->setText("Form cleared. Please fill in all required fields.");
        m_statusLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 5px;");
        m_submitButton->setEnabled(false);
    }
}

void AdminControlPanel::onSubmit() {
    if (m_isSubmitting) {
        return;
    }
    
    m_isSubmitting = true;
    m_submitButton->setEnabled(false);
    m_submitButton->setText("⏳ Submitting...");
    
    // Gather all data
    QString userName = m_userNameEdit->text().trimmed();
    QString phone = m_phoneEdit->text().trimmed();
    QString email = m_emailEdit->text().trimmed();
    int profileCount = m_profileCountSpin->value();
    QString profileType = m_profileTypeCombo->currentData().toString();
    int duration = m_durationSpin->value();
    int autoRenewDays = m_autoRenewCheck->currentData().toInt();
    QString notes = m_notesEdit->text().trimmed();
    
    // Create JSON configuration
    QJsonObject config;
    config["userName"] = userName;
    config["phoneNumber"] = phone;
    config["email"] = email;
    config["profileCount"] = profileCount;
    config["profileType"] = profileType;
    config["durationDays"] = duration;
    config["autoRenewDays"] = autoRenewDays;
    config["notes"] = notes;
    config["createdAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    config["version"] = "3.0.0";
    
    // Save configuration
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString configFile = configDir + "/admin_config.json";
    
    QFile file(configFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
        file.close();
        
        // Success message
        QMessageBox::information(
            this,
            "✅ Configuration Saved",
            QString("<h3>Configuration saved successfully!</h3>"
                    "<p><b>User:</b> %1</p>"
                    "<p><b>Phone:</b> %2</p>"
                    "<p><b>Profiles:</b> %3</p>"
                    "<p><b>Device:</b> %4</p>"
                    "<p><b>Duration:</b> %5 days</p>"
                    "<p><b>Auto-Renew:</b> %6</p>"
                    "<p style='color: #888888; margin-top: 15px;'>"
                    "Configuration has been saved to:<br/>%7"
                    "</p>")
                .arg(userName)
                .arg(phone)
                .arg(profileCount)
                .arg(m_profileTypeCombo->currentText())
                .arg(duration)
                .arg(autoRenewDays == 0 ? "No" : QString("%1 days before").arg(autoRenewDays))
                .arg(configFile)
        );
        
        // Emit signal with configuration
        emit configurationSubmitted(config);
        
        accept();
    } else {
        QMessageBox::critical(
            this,
            "❌ Error",
            QString("Failed to save configuration!\n\n"
                    "Error: %1\n\n"
                    "Please check if you have write permissions.")
                .arg(file.errorString())
        );
        
        m_submitButton->setEnabled(true);
        m_submitButton->setText("✅ Submit Configuration");
    }
    
    m_isSubmitting = false;
}

void AdminControlPanel::loadPreviousData() {
    QString configFile = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) 
                         + "/admin_config.json";
    
    QFile file(configFile);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error == QJsonParseError::NoError) {
            QJsonObject config = doc.object();
            
            // Load previous values
            if (config.contains("userName")) {
                m_userNameEdit->setText(config["userName"].toString());
            }
            if (config.contains("phoneNumber")) {
                m_phoneEdit->setText(config["phoneNumber"].toString());
            }
            if (config.contains("email")) {
                m_emailEdit->setText(config["email"].toString());
            }
            if (config.contains("profileCount")) {
                m_profileCountSpin->setValue(config["profileCount"].toInt());
            }
            if (config.contains("profileType")) {
                QString profileType = config["profileType"].toString();
                for (int i = 0; i < m_profileTypeCombo->count(); ++i) {
                    if (m_profileTypeCombo->itemData(i).toString() == profileType) {
                        m_profileTypeCombo->setCurrentIndex(i);
                        break;
                    }
                }
            }
            if (config.contains("durationDays")) {
                m_durationSpin->setValue(config["durationDays"].toInt());
            }
            if (config.contains("autoRenewDays")) {
                int autoRenewDays = config["autoRenewDays"].toInt();
                for (int i = 0; i < m_autoRenewCheck->count(); ++i) {
                    if (m_autoRenewCheck->itemData(i).toInt() == autoRenewDays) {
                        m_autoRenewCheck->setCurrentIndex(i);
                        break;
                    }
                }
            }
            if (config.contains("notes")) {
                m_notesEdit->setText(config["notes"].toString());
            }
            
            m_statusLabel->setText("Previous configuration loaded. Update values and submit.");
            m_statusLabel->setStyleSheet("color: #00d4ff; font-size: 12px; padding: 5px;");
        }
    }
    
    validateForm();
}

QJsonObject AdminControlPanel::getConfiguration() const {
    QJsonObject config;
    config["userName"] = m_userNameEdit->text().trimmed();
    config["phoneNumber"] = m_phoneEdit->text().trimmed();
    config["email"] = m_emailEdit->text().trimmed();
    config["profileCount"] = m_profileCountSpin->value();
    config["profileType"] = m_profileTypeCombo->currentData().toString();
    config["durationDays"] = m_durationSpin->value();
    config["autoRenewDays"] = m_autoRenewCheck->currentData().toInt();
    config["notes"] = m_notesEdit->text().trimmed();
    return config;
}
