/**
 * @file AdminLoginWindow.cpp
 * @brief Admin Login Window - Firebase Admin Authentication
 * @version 1.0.0
 */

#include "AdminLoginWindow.hpp"
#include "AdminDashboardWindow.hpp"
#include "VirtualPhonePro/ConfigManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>

AdminLoginWindow::AdminLoginWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_isLoggingIn(false)
{
    setWindowTitle("Admin Login - ReDroidCPP");
    setFixedSize(450, 550);
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
            padding: 14px 24px;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
            min-height: 45px;
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
            border: 2px solid #334155;
            padding: 14px 16px;
            border-radius: 8px;
            font-size: 15px;
            min-height: 42px;
        }
        QLineEdit:focus {
            border-color: #3b82f6;
        }
        QLabel#titleLabel {
            font-size: 28px;
            font-weight: bold;
            color: #60a5fa;
        }
        QLabel#subtitleLabel {
            font-size: 14px;
            color: #94a3b8;
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
    )");

    setupUI();
}

AdminLoginWindow::~AdminLoginWindow() {
}

void AdminLoginWindow::setupUI() {
    loginPage = new QWidget(this);
    setCentralWidget(loginPage);

    QVBoxLayout* mainLayout = new QVBoxLayout(loginPage);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(15);

    // Title - must be added to layout
    m_titleLabel = new QLabel("🔐 Admin Portal", loginPage);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    QLabel* subtitleLabel = new QLabel("ReDroidCPP Administration", loginPage);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subtitleLabel);

    // Spacer
    mainLayout->addSpacing(20);

    // Admin Email
    QLabel* adminIdLabel = new QLabel("Admin Email:", loginPage);
    adminIdLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500;");
    mainLayout->addWidget(adminIdLabel);

    m_adminIdInput = new QLineEdit(loginPage);
    m_adminIdInput->setPlaceholderText("Enter your admin email");
    m_adminIdInput->setMinimumHeight(45);
    mainLayout->addWidget(m_adminIdInput);

    // Password
    QLabel* passwordLabel = new QLabel("Password:", loginPage);
    passwordLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500;");
    mainLayout->addWidget(passwordLabel);

    m_passwordInput = new QLineEdit(loginPage);
    m_passwordInput->setPlaceholderText("Enter your password");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setMinimumHeight(45);
    mainLayout->addWidget(m_passwordInput);

    // Status
    m_statusLabel = new QLabel(loginPage);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);

    // Spacer
    mainLayout->addSpacing(10);

    // Login Button
    m_loginButton = new QPushButton("LOGIN", loginPage);
    m_loginButton->setMinimumHeight(50);
    mainLayout->addWidget(m_loginButton);

    // Info text
    QLabel* infoLabel = new QLabel("Contact super admin for credentials", loginPage);
    infoLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel);

    // Connect signals
    connect(m_loginButton, &QPushButton::clicked, this, &AdminLoginWindow::onLoginClicked);
    connect(m_passwordInput, &QLineEdit::returnPressed, this, &AdminLoginWindow::onLoginClicked);
}

void AdminLoginWindow::onLoginClicked() {
    if (m_isLoggingIn) return;

    QString email = m_adminIdInput->text().trimmed();
    QString password = m_passwordInput->text();

    // Validation
    if (email.isEmpty()) {
        showError("Please enter your admin email");
        m_adminIdInput->setFocus();
        return;
    }

    if (password.isEmpty()) {
        showError("Please enter your password");
        m_passwordInput->setFocus();
        return;
    }

    // Firebase project ID and web API key are compiled into the app
    // (public-by-design values, same as the Mainadmin web panel); env vars
    // or the config file can override them for development.
    auto& config = VirtualPhonePro::ConfigManager::instance();
    if (!config.hasFirebaseConfig()) {
        showError("App Firebase configuration is invalid. Please contact support.");
        return;
    }

    m_isLoggingIn = true;
    m_loginButton->setEnabled(false);
    m_loginButton->setText("Authenticating...");
    m_statusLabel->setVisible(false);

    // Sign in with the same Firebase Auth email/password credentials that
    // the Mainadmin web panel uses (Identity Toolkit REST API).
    QUrl url("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key="
             + config.getFirebaseApiKey());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;
    payload["returnSecureToken"] = true;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    reply->setProperty("phase", QStringLiteral("auth"));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onLoginReply(reply);
    });
}

void AdminLoginWindow::onLoginReply(QNetworkReply* reply) {
    const QString phase = reply->property("phase").toString();

    if (phase == QLatin1String("verify")) {
        m_isLoggingIn = false;
        m_loginButton->setEnabled(true);
        m_loginButton->setText("LOGIN");

        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            showError("Could not verify admin account. Please try again.");
            return;
        }

        QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();

        if (!doc.contains("fields")) {
            showError("This account is not registered as an admin.");
            return;
        }

        QJsonObject fields = doc["fields"].toObject();
        if (fields["isBlocked"].toObject()["booleanValue"].toBool()) {
            showError("This admin account has been blocked. Contact super admin.");
            return;
        }

        showSuccess("Login successful!");

        // The Firebase Auth idToken doubles as the Bearer token for the
        // Firestore REST calls made from the dashboard.
        AdminDashboardWindow* dashboard = new AdminDashboardWindow(this, m_uid, m_idToken);
        dashboard->show();
        hide();
        return;
    }

    // phase == "auth"
    QByteArray response = reply->readAll();
    QJsonObject obj = QJsonDocument::fromJson(response).object();

    if (reply->error() != QNetworkReply::NoError) {
        m_isLoggingIn = false;
        m_loginButton->setEnabled(true);
        m_loginButton->setText("LOGIN");

        QString code = obj["error"].toObject()["message"].toString();
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        if (code.contains("INVALID_LOGIN_CREDENTIALS") || code.contains("EMAIL_NOT_FOUND") ||
            code.contains("INVALID_EMAIL")) {
            showError("Invalid admin email or password");
        } else if (code.contains("INVALID_PASSWORD")) {
            showError("Invalid password");
        } else if (code.contains("USER_DISABLED")) {
            showError("This account has been disabled. Contact super admin.");
        } else if (code.contains("TOO_MANY_ATTEMPTS")) {
            showError("Too many failed attempts. Please try again later.");
        } else {
            showError(QString("Connection failed (code %1). Check internet and try again.").arg(httpCode));
        }
        return;
    }
    reply->deleteLater();

    m_idToken = obj["idToken"].toString();
    m_uid = obj["localId"].toString();
    if (m_idToken.isEmpty() || m_uid.isEmpty()) {
        m_isLoggingIn = false;
        m_loginButton->setEnabled(true);
        m_loginButton->setText("LOGIN");
        showError("Authentication error. Please try again.");
        return;
    }

    // Auth succeeded — verify the user is a registered, unblocked admin.
    // The Mainadmin panel keys admins/{uid} by the Firebase Auth UID.
    auto& config = VirtualPhonePro::ConfigManager::instance();
    QUrl url(config.getFirebaseBaseUrl() + "/admins/" + m_uid + "?key=" + config.getFirebaseApiKey());
    QNetworkRequest request(url);

    QNetworkReply* verifyReply = m_networkManager->get(request);
    verifyReply->setProperty("phase", QStringLiteral("verify"));

    connect(verifyReply, &QNetworkReply::finished, this, [this, verifyReply]() {
        this->onLoginReply(verifyReply);
    });
}

void AdminLoginWindow::showError(const QString& message) {
    m_statusLabel->setText(message);
    m_statusLabel->setProperty("class", "error");
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet());
    m_statusLabel->setVisible(true);
}

void AdminLoginWindow::showSuccess(const QString& message) {
    m_statusLabel->setText(message);
    m_statusLabel->setProperty("class", "success");
    m_statusLabel->setStyleSheet(m_statusLabel->styleSheet());
    m_statusLabel->setVisible(true);
}

void AdminLoginWindow::onSwitchToRequest() {
    // Optional: switch to user request view
}
