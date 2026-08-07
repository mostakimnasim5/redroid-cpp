#include "LoginWindow.h"
#include <QApplication>
#include <QStyle>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

#include "VirtualPhonePro/ConfigManager.hpp"

// Firebase Configuration - Loaded from config file for security
#define getFirebaseProjectId() VirtualPhonePro::ConfigManager::instance().getFirebaseProjectId()
#define getFirebaseApiKey() VirtualPhonePro::ConfigManager::instance().getFirebaseApiKey()
#define getFirebaseBaseUrl() VirtualPhonePro::ConfigManager::instance().getFirebaseBaseUrl()

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    setWindowTitle("ReDroidCPP - Login");
    setFixedSize(520, 750);
    setStyleSheet(R"(
        QMainWindow {
            background-color: #0f172a;
        }
        QLabel {
            color: #e2e8f0;
        }
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
            min-height: 42px;
        }
        QPushButton:hover {
            background-color: #2563eb;
        }
        QPushButton:pressed {
            background-color: #1d4ed8;
        }
        QPushButton:disabled {
            background-color: #475569;
            color: #94a3b8;
        }
        QLineEdit {
            background-color: #1e293b;
            color: white;
            border: 1px solid #334155;
            padding: 12px 15px;
            border-radius: 8px;
            font-size: 15px;
            min-height: 38px;
        }
        QLineEdit:focus {
            border-color: #3b82f6;
        }
        QComboBox {
            background-color: #1e293b;
            color: white;
            border: 1px solid #334155;
            padding: 10px 15px;
            border-radius: 8px;
            font-size: 15px;
            min-height: 38px;
        }
        QSpinBox {
            background-color: #1e293b;
            color: white;
            border: 1px solid #334155;
            padding: 10px 15px;
            border-radius: 8px;
            font-size: 15px;
            min-height: 38px;
        }
        QLabel#titleLabel {
            font-size: 26px;
            font-weight: bold;
            color: #60a5fa;
        }
        QLabel#subtitleLabel {
            font-size: 14px;
            color: #94a3b8;
        }
        QLabel#statusLabel {
            padding: 10px;
            border-radius: 8px;
        }
        .error {
            background-color: rgba(220, 38, 38, 0.2);
            color: #fca5a5;
            border: 1px solid rgba(220, 38, 38, 0.3);
            padding: 12px;
            border-radius: 8px;
        }
        .success {
            background-color: rgba(22, 163, 74, 0.2);
            color: #86efac;
            border: 1px solid rgba(22, 163, 74, 0.3);
            padding: 12px;
            border-radius: 8px;
        }
        QLabel#loadingLabel {
            color: #60a5fa;
            font-size: 12px;
        }
        QWidget#requestScrollArea {
            background-color: transparent;
        }
    )");

    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    setupLoginUI();
    setupRequestUI();

    stackedWidget->addWidget(loginPage);
    stackedWidget->addWidget(requestPage);
}

LoginWindow::~LoginWindow() {}

void LoginWindow::setupLoginUI() {
    loginPage = new QWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(loginPage);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel("🔐 ReDroidCPP", loginPage);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel("Android Emulator Manager", loginPage);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    // Phone Input
    QLabel *phoneLoginLabel = new QLabel("Phone Number:", loginPage);
    phoneLoginLabel->setStyleSheet("color: #94a3b8; font-size: 13px;");
    phoneLoginInput = new QLineEdit(loginPage);
    phoneLoginInput->setPlaceholderText("01XXXXXXXXX");
    phoneLoginInput->setAlignment(Qt::AlignCenter);
    phoneLoginInput->setFont(QFont("Segoe UI", 14));

    // Code Input
    QLabel *codeLabel = new QLabel("Enter your 8-digit access code:", loginPage);
    codeInput = new QLineEdit(loginPage);
    codeInput->setPlaceholderText("XXXXXXXX");
    codeInput->setMaxLength(8);
    codeInput->setAlignment(Qt::AlignCenter);
    codeInput->setFont(QFont("Segoe UI", 20, QFont::Bold));

    // Status
    statusLabel = new QLabel(loginPage);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setVisible(false);

    // Buttons
    btnLogin = new QPushButton("LOGIN", loginPage);
    btnSwitchToRequest = new QPushButton("Don't have a code? Request Access", loginPage);
    btnSwitchToRequest->setStyleSheet("background-color: transparent; color: #60a5fa; font-size: 12px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(phoneLoginLabel);
    mainLayout->addWidget(phoneLoginInput);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(codeLabel);
    mainLayout->addWidget(codeInput);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(btnLogin);
    mainLayout->addWidget(btnSwitchToRequest);

    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(btnSwitchToRequest, &QPushButton::clicked, this, &LoginWindow::onSwitchToRequest);
    connect(codeInput, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(phoneLoginInput, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::setupRequestUI() {
    requestPage = new QWidget(this);
    requestPage->setObjectName("requestScrollArea");

    QVBoxLayout *mainLayout = new QVBoxLayout(requestPage);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->setContentsMargins(40, 25, 40, 25);
    mainLayout->setSpacing(8);

    // Title
    QLabel *titleLabel = new QLabel("📝 Access Request", requestPage);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setContentsMargins(0, 5, 0, 5);

    QLabel *subtitleLabel = new QLabel("Request a new account to use ReDroidCPP", requestPage);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    // Spacer
    mainLayout->addSpacing(5);

    // Name Field with Label
    QLabel *nameLabel = new QLabel("Full Name:", requestPage);
    nameLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500; padding-bottom: 4px;");
    mainLayout->addWidget(nameLabel);
    
    nameInput = new QLineEdit(requestPage);
    nameInput->setPlaceholderText("Full Name লিখুন");
    nameInput->setMinimumHeight(42);
    nameInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(nameInput);

    // Add spacing between fields
    mainLayout->addSpacing(10);

    // Phone Field with Label
    QLabel *phoneLabel = new QLabel("Phone Number:", requestPage);
    phoneLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500; padding-bottom: 4px;");
    mainLayout->addWidget(phoneLabel);
    
    phoneInput = new QLineEdit(requestPage);
    phoneInput->setPlaceholderText("+8801XXXXXXXXX");
    phoneInput->setMinimumHeight(42);
    phoneInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(phoneInput);

    // Add spacing between fields
    mainLayout->addSpacing(10);

    // Profiles Field with Label
    QLabel *profilesLabel = new QLabel("Number of Profiles (1-100)?", requestPage);
    profilesLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500; padding-bottom: 4px;");
    mainLayout->addWidget(profilesLabel);
    
    profilesSpinBox = new QSpinBox(requestPage);
    profilesSpinBox->setMinimum(1);
    profilesSpinBox->setMaximum(10);
    profilesSpinBox->setValue(3);
    profilesSpinBox->setMinimumHeight(42);
    profilesSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(profilesSpinBox);

    // Add spacing between fields
    mainLayout->addSpacing(10);

    // Duration Field with Label
    QLabel *durationLabel = new QLabel("Duration?", requestPage);
    durationLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500; padding-bottom: 4px;");
    mainLayout->addWidget(durationLabel);
    
    durationCombo = new QComboBox(requestPage);
    durationCombo->addItem("5 minutes", 5);
    durationCombo->addItem("15 minutes", 15);
    durationCombo->addItem("30 minutes", 30);
    durationCombo->addItem("1 hour", 60);
    durationCombo->addItem("1 day", 1440);
    durationCombo->addItem("3 days", 4320);
    durationCombo->addItem("7 days (1 week)", 10080);
    durationCombo->addItem("15 days", 21600);
    durationCombo->addItem("30 days (1 month)", 43200);
    durationCombo->addItem("1 year", 525600);
    durationCombo->setMinimumHeight(42);
    durationCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(durationCombo);

    // Spacer before status and button
    mainLayout->addSpacing(20);

    // Status Label with proper word wrap
    requestStatusLabel = new QLabel(requestPage);
    requestStatusLabel->setObjectName("statusLabel");
    requestStatusLabel->setAlignment(Qt::AlignCenter);
    requestStatusLabel->setWordWrap(true);
    requestStatusLabel->setVisible(false);
    requestStatusLabel->setMinimumHeight(50);
    requestStatusLabel->setContentsMargins(10, 0, 10, 0);
    requestStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    mainLayout->addWidget(requestStatusLabel);

    // Add small spacing before button
    mainLayout->addSpacing(10);

    // Submit Button - Fixed height, centered, bold
    btnSendRequest = new QPushButton("SUBMIT REQUEST", requestPage);
    btnSendRequest->setMinimumHeight(48);
    btnSendRequest->setMaximumHeight(48);
    btnSendRequest->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnSendRequest->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 15px;
            font-weight: bold;
            min-height: 48px;
            padding: 0 20px;
        }
        QPushButton:hover {
            background-color: #2563eb;
        }
        QPushButton:pressed {
            background-color: #1d4ed8;
        }
        QPushButton:disabled {
            background-color: #475569;
            color: #94a3b8;
        }
    )");
    mainLayout->addWidget(btnSendRequest);

    // Add spacing between buttons
    mainLayout->addSpacing(8);

    // Back to Login button
    btnSwitchToLogin = new QPushButton("আগে থেকে কোড আছে? LOGIN করুন", requestPage);
    btnSwitchToLogin->setMinimumHeight(36);
    btnSwitchToLogin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnSwitchToLogin->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #60a5fa;
            border: none;
            font-size: 13px;
            min-height: 36px;
        }
        QPushButton:hover {
            color: #93c5fd;
            text-decoration: underline;
        }
    )");
    mainLayout->addWidget(btnSwitchToLogin);

    // Add stretch at the end for extra space
    mainLayout->addStretch(1);

    connect(btnSendRequest, &QPushButton::clicked, this, &LoginWindow::onSendRequestClicked);
    connect(btnSwitchToLogin, &QPushButton::clicked, this, &LoginWindow::onSwitchToLogin);
}

void LoginWindow::onLoginClicked() {
    QString phone = phoneLoginInput->text().trimmed();
    QString code = codeInput->text().trimmed().toUpper();

    if (phone.isEmpty()) {
        showError("Phone Number দিন");
        phoneLoginInput->setFocus();
        return;
    }

    // Phone minimum 11 digits
    QString digitsOnly = phone;
    digitsOnly.replace(QRegularExpression("[^0-9]"), "");
    if (digitsOnly.length() < 11) {
        showError("Phone Number কমপক্ষে ১১ সংখ্যার হতে হবে");
        phoneLoginInput->setFocus();
        return;
    }

    if (code.isEmpty()) {
        showError("Access Code দিন");
        codeInput->setFocus();
        return;
    }

    if (code.length() != 8) {
        showError("কোড ৮ সংখ্যার হতে হবে");
        codeInput->setFocus();
        return;
    }

    btnLogin->setEnabled(false);
    btnLogin->setText("যাচাই হচ্ছে...");
    verifyCode(code, phone);
}

void LoginWindow::verifyCode(const QString &code, const QString &phone) {
    // Firebase Firestore runQuery: match uniqueKey AND contactNumber (phone)
    QString queryUrl = getFirebaseBaseUrl() + QStringLiteral(":runQuery?key=") + getFirebaseApiKey();

    QUrl url(queryUrl);
    QNetworkRequest queryRequest;
    queryRequest.setUrl(url);
    queryRequest.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    // Filter 1: uniqueKey == code
    QJsonObject keyFilter;
    keyFilter["fieldFilter"] = QJsonObject{
        {"field", QJsonObject{{"fieldPath", "uniqueKey"}}},
        {"op", "EQUAL"},
        {"value", QJsonObject{{"stringValue", code}}}
    };

    // Filter 2: contactNumber == phone (matches what admin panel stores)
    QJsonObject phoneFilter;
    phoneFilter["fieldFilter"] = QJsonObject{
        {"field", QJsonObject{{"fieldPath", "contactNumber"}}},
        {"op", "EQUAL"},
        {"value", QJsonObject{{"stringValue", phone}}}
    };

    // Composite AND filter
    QJsonObject compositeFilter;
    compositeFilter["op"] = QStringLiteral("AND");
    compositeFilter["filters"] = QJsonArray{keyFilter, phoneFilter};

    QJsonObject structuredQuery;
    structuredQuery["from"] = QJsonArray{QJsonObject{{"collectionId", "activeUsers"}}};
    structuredQuery["where"] = QJsonObject{{"compositeFilter", compositeFilter}};
    structuredQuery["limit"] = 1;

    QJsonObject queryObj;
    queryObj["structuredQuery"] = structuredQuery;

    QNetworkReply *reply = networkManager->post(queryRequest, QJsonDocument(queryObj).toJson());
    pendingRequestId = code;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->handleLoginResponse(reply);
    });
}

void LoginWindow::handleLoginResponse(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        // User-friendly error messages - no raw HTTP URLs
        QString errorMsg;
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "Connection refused. Please check your internet connection.";
                break;
            case QNetworkReply::TimeoutError:
                errorMsg = "Request timed out. Please try again.";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "Server not found. Please try again later.";
                break;
            case QNetworkReply::NetworkSessionFailedError:
                errorMsg = "Network error occurred. Please check your connection.";
                break;
            default:
                // Show actual error code to help diagnose
                errorMsg = QString("Login error (%1). Check Firestore rules allow unauthenticated reads.")
                    .arg((int)reply->error());
                break;
        }
        showError(errorMsg);
        btnLogin->setEnabled(true);
        btnLogin->setText("LOGIN");
        reply->deleteLater();
        return;
    }
    
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    
    // Parse runQuery response - it returns an array of document results
    if (!doc.isArray()) {
        // Check if it's an error response
        QJsonObject obj = doc.object();
        if (obj.contains("error")) {
            showError("Service temporarily unavailable. Please try again later.");
        } else {
            showError("Server error. Please try again later.");
        }
        btnLogin->setEnabled(true);
        btnLogin->setText("LOGIN");
        reply->deleteLater();
        return;
    }
    
    QJsonArray results = doc.array();
    
    // Check if we got a valid document
    if (results.isEmpty() || !results[0].toObject().contains("document")) {
        showError("Invalid Access Code. Please check and try again.");
        btnLogin->setEnabled(true);
        btnLogin->setText("LOGIN");
        reply->deleteLater();
        return;
    }
    
    QJsonObject document = results[0].toObject()["document"].toObject();
    QJsonObject fields = document["fields"].toObject();
    
    // Check if blocked
    if (fields.contains("isBlocked") && fields["isBlocked"].toObject()["booleanValue"].toBool()) {
        showError("Your account has been blocked. Please contact Admin.");
        btnLogin->setEnabled(true);
        btnLogin->setText("LOGIN");
        reply->deleteLater();
        return;
    }
    
    // Get user data
    QString userId = document["name"].toString().split("/").last();
    QString uniqueKey = fields.contains("uniqueKey") ? fields["uniqueKey"].toObject()["stringValue"].toString() : "";
    // Firestore REST returns integerValue as STRING - must use toString().toInt()
    int remainingProfiles = fields.contains("remainingProfiles") ? fields["remainingProfiles"].toObject()["integerValue"].toString().toInt() : 0;
    int totalProfiles = fields.contains("totalProfiles") ? fields["totalProfiles"].toObject()["integerValue"].toString().toInt() : 0;

    // Success! Hide window and emit login success
    hide();
    emit loginSuccess(userId, uniqueKey, remainingProfiles, totalProfiles);
    
    btnLogin->setEnabled(true);
    btnLogin->setText("LOGIN");
    reply->deleteLater();
}

void LoginWindow::onSendRequestClicked() {
    QString name = nameInput->text().trimmed();
    QString phone = phoneInput->text().trimmed();
    int profiles = profilesSpinBox->value();
    int duration = durationCombo->currentData().toInt();

    // Validate Firebase config first
    if (!VirtualPhonePro::ConfigManager::instance().hasFirebaseConfig()) {
        showRequestError("⚠️ Firebase কনফিগ সঠিক নয়! অ্যাপ সেটিংসে Firebase credentials যাচাই করুন।");
        return;
    }

    // Validate name - must not be empty and at least 2 characters
    if (name.isEmpty()) {
        showRequestError("Please enter your full name");
        nameInput->setFocus();
        return;
    }
    if (name.length() < 2) {
        showRequestError("Name must be at least 2 characters long");
        nameInput->setFocus();
        return;
    }

    // Validate phone - must not be empty and should be valid format
    if (phone.isEmpty()) {
        showRequestError("Please enter your phone number");
        phoneInput->setFocus();
        return;
    }
    
    // Check phone format - should be at least 11 digits
    QString digitsOnly = phone;
    digitsOnly.replace(QRegularExpression("[^0-9]"), "");
    if (digitsOnly.length() < 11) {
        showRequestError("Phone Number কমপক্ষে ১১ সংখ্যার হতে হবে");
        phoneInput->setFocus();
        return;
    }

    // Validate profiles
    if (profiles < 1) {
        showRequestError("At least 1 profile is required");
        return;
    }

    // Validate duration
    if (duration < 5) {
        showRequestError("কমপক্ষে 5 minutesের জন্য আবেদন করুন");
        return;
    }

    btnSendRequest->setEnabled(false);
    btnSendRequest->setText("Submitting...");
    sendAccessRequest(name, phone, profiles, duration);
}

void LoginWindow::sendAccessRequest(const QString &name, const QString &phone, int profiles, int duration) {
    // Firebase Firestore REST API - Create document
    QString urlStr = getFirebaseBaseUrl() + QStringLiteral("/accessRequests?key=") + getFirebaseApiKey();
    
    QUrl reqUrl(urlStr);
    QNetworkRequest netRequest;
    netRequest.setUrl(reqUrl);
    netRequest.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    
    // Create document data
    QJsonObject fields;
    
    QJsonObject userNameObj;
    userNameObj["stringValue"] = name;
    fields["userName"] = userNameObj;
    
    QJsonObject contactObj;
    contactObj["stringValue"] = phone;
    fields["contactNumber"] = contactObj;
    
    QJsonObject profileCountObj;
    profileCountObj["integerValue"] = QString::number(profiles);
    fields["profileCount"] = profileCountObj;
    
    QJsonObject durationObj;
    durationObj["integerValue"] = QString::number(duration);
    fields["durationMinutes"] = durationObj;
    
    QJsonObject statusObj;
    statusObj["stringValue"] = QStringLiteral("pending");
    fields["status"] = statusObj;
    
    QJsonObject timestampObj;
    timestampObj["timestampValue"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    fields["requestedAt"] = timestampObj;
    
    QJsonObject document;
    document["fields"] = fields;
    
    QJsonDocument doc(document);
    QByteArray data = doc.toJson();
    
    QNetworkReply *reply = networkManager->post(netRequest, data);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->handleRequestResponse(reply);
    });
}

void LoginWindow::handleRequestResponse(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        // User-friendly error messages - no raw HTTP URLs
        QString errorMsg;
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "Connection refused. Please check your internet connection.";
                break;
            case QNetworkReply::TimeoutError:
                errorMsg = "Request timed out. Please try again.";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "Server not found. Please try again later.";
                break;
            case QNetworkReply::NetworkSessionFailedError:
                errorMsg = "Network error occurred. Please check your connection.";
                break;
            case QNetworkReply::ContentNotFoundError:
                errorMsg = "Service temporarily unavailable. Please try again.";
                break;
            default:
                errorMsg = QString("Network error (%1). Please try again.").arg((int)reply->error());
                break;
        }
        showRequestError(errorMsg);
        btnSendRequest->setEnabled(true);
        btnSendRequest->setText("SUBMIT REQUEST");
        reply->deleteLater();
        return;
    }
    
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();
    
    if (obj.contains("name")) {
        QString docName = obj["name"].toString();
        showRequestSuccess("Request submitted successfully!\n\nYour request has been sent to Admin. You will receive an 8-digit access code after Admin approval.");
        
        // Clear form
        nameInput->clear();
        phoneInput->clear();
        profilesSpinBox->setValue(3);
        durationCombo->setCurrentIndex(4); // 1 day
        
        // Switch to login after 3 seconds
        QTimer::singleShot(3000, this, &LoginWindow::onSwitchToLogin);
    } else if (obj.contains("error")) {
        // Firebase returned an error - show user-friendly message
        QJsonObject error = obj["error"].toObject();
        QString errorCode = error["code"].toString();
        QString errorMessage = error["message"].toString();
        
        // Map Firebase error codes to user-friendly messages
        QString userMsg;
        if (errorCode == "RESOURCE_EXHAUSTED") {
            userMsg = "Service is busy. Please try again in a few moments.";
        } else if (errorCode == "UNAUTHENTICATED") {
            userMsg = "Authentication required. Please try again.";
        } else if (errorCode == "PERMISSION_DENIED") {
            userMsg = "Permission denied. Please contact support.";
        } else if (errorMessage.contains("QUOTA_EXCEEDED", Qt::CaseInsensitive)) {
            userMsg = "Daily limit exceeded. Please try again tomorrow.";
        } else {
            // Show actual Firebase error for debugging
            userMsg = QString("Error: %1 - %2").arg(errorCode).arg(errorMessage);
        }
        
        showRequestError(userMsg);
        qDebug() << "[AccessRequest] Firebase Error:" << errorCode << "-" << errorMessage;
    } else {
        showRequestError("Failed to submit request. Please try again.");
        qDebug() << "[AccessRequest] Unknown response:" << QString::fromUtf8(response);
    }
    
    btnSendRequest->setEnabled(true);
    btnSendRequest->setText("SUBMIT REQUEST");
    reply->deleteLater();
}

void LoginWindow::onNetworkReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        // User-friendly error messages - no raw HTTP URLs
        QString errorMsg;
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "Connection refused. Please check your internet connection.";
                break;
            case QNetworkReply::TimeoutError:
                errorMsg = "Request timed out. Please try again.";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "Server not found. Please try again later.";
                break;
            case QNetworkReply::NetworkSessionFailedError:
                errorMsg = "Network error occurred. Please check your connection.";
                break;
            default:
                // Show actual error code to help diagnose
                errorMsg = QString("Login error (%1). Check Firestore rules allow unauthenticated reads.")
                    .arg((int)reply->error());
                break;
        }
        showError(errorMsg);
        btnLogin->setEnabled(true);
        btnLogin->setText("LOGIN");
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();

    if (obj.contains("documents")) {
        QJsonArray docs = obj["documents"].toArray();
        if (docs.isEmpty()) {
            showError("Invalid access code. Please check and try again.");
        } else {
            QJsonObject userDoc = docs.first().toObject();
            QJsonObject fields = userDoc["fields"].toObject();

            bool isBlocked = fields.contains("isBlocked") && fields["isBlocked"].toObject()["booleanValue"].toBool();

            if (isBlocked) {
                showError("Your account has been blocked. Please contact Admin.");
            } else {
                QString userId = userDoc["name"].toString().split("/").last();
                QString uniqueKey = fields.contains("uniqueKey") ? fields["uniqueKey"].toObject()["stringValue"].toString() : "";
                int remaining = fields.contains("remainingProfiles") ? fields["remainingProfiles"].toObject()["integerValue"].toInt() : 0;
                int total = fields.contains("totalProfiles") ? fields["totalProfiles"].toObject()["integerValue"].toInt() : 0;

                hide();
                emit loginSuccess(userId, uniqueKey, remaining, total);
            }
        }
    } else {
        showError("Code verification failed");
    }

    btnLogin->setEnabled(true);
    btnLogin->setText("LOGIN");
    reply->deleteLater();
}

void LoginWindow::onSwitchToRequest() {
    stackedWidget->setCurrentWidget(requestPage);
    setWindowTitle("ReDroidCPP - Access Request");
}

void LoginWindow::onSwitchToLogin() {
    stackedWidget->setCurrentWidget(loginPage);
    setWindowTitle("ReDroidCPP - Login");
}

void LoginWindow::showError(const QString &message) {
    statusLabel->setText(message);
    statusLabel->setProperty("class", "error");
    statusLabel->setStyleSheet(statusLabel->styleSheet());
    statusLabel->setVisible(true);
}

void LoginWindow::showSuccess(const QString &message) {
    statusLabel->setText(message);
    statusLabel->setProperty("class", "success");
    statusLabel->setStyleSheet(statusLabel->styleSheet());
    statusLabel->setVisible(true);

    QTimer::singleShot(3000, this, [this]() {
        statusLabel->setVisible(false);
    });
}

void LoginWindow::showRequestError(const QString &message) {
    requestStatusLabel->setText(message);
    requestStatusLabel->setProperty("class", "error");
    requestStatusLabel->setStyleSheet(requestStatusLabel->styleSheet());
    requestStatusLabel->setVisible(true);
}

void LoginWindow::showRequestSuccess(const QString &message) {
    requestStatusLabel->setText(message);
    requestStatusLabel->setProperty("class", "success");
    requestStatusLabel->setStyleSheet(requestStatusLabel->styleSheet());
    requestStatusLabel->setVisible(true);

    QTimer::singleShot(3000, this, [this]() {
        requestStatusLabel->setVisible(false);
    });
}
