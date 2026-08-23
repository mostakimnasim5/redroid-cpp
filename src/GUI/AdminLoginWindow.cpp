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
#include <QCryptographicHash>
#include <QDebug>

namespace {

// Constant-time comparison so password checks do not leak match length
// through timing side channels.
bool constantTimeEqual(const QByteArray& a, const QByteArray& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
    }
    return diff == 0;
}

// Salted SHA-256 of (salt || password), hex-encoded. The salt is stored
// per-admin in Firestore ("passwordSalt" field) alongside "passwordHash".
QByteArray hashPassword(const QString& salt, const QString& password) {
    QByteArray data = salt.toUtf8() + password.toUtf8();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

} // namespace

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

    // Admin ID
    QLabel* adminIdLabel = new QLabel("Admin ID:", loginPage);
    adminIdLabel->setStyleSheet("color: #cbd5e1; font-size: 14px; font-weight: 500;");
    mainLayout->addWidget(adminIdLabel);

    m_adminIdInput = new QLineEdit(loginPage);
    m_adminIdInput->setPlaceholderText("Enter your admin ID");
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

    QString adminId = m_adminIdInput->text().trimmed();
    QString password = m_passwordInput->text();

    // Validation
    if (adminId.isEmpty()) {
        showError("Please enter your admin ID");
        m_adminIdInput->setFocus();
        return;
    }

    if (password.isEmpty()) {
        showError("Please enter your password");
        m_passwordInput->setFocus();
        return;
    }

    m_isLoggingIn = true;
    m_loginButton->setEnabled(false);
    m_loginButton->setText("Authenticating...");
    m_statusLabel->setVisible(false);

    // Use Firebase REST API for admin authentication.
    // Credentials come from REDROID_FB_PROJECT_ID / REDROID_FB_API_KEY
    // environment variables or the user config file — never hardcoded.
    auto& config = VirtualPhonePro::ConfigManager::instance();
    if (!config.hasFirebaseConfig()) {
        m_isLoggingIn = false;
        m_loginButton->setEnabled(true);
        m_loginButton->setText("LOGIN");
        showError("Firebase is not configured. Set REDROID_FB_PROJECT_ID and "
                  "REDROID_FB_API_KEY environment variables.");
        return;
    }
    QString baseUrl = config.getFirebaseBaseUrl() + "/admins";
    QString apiKey = config.getFirebaseApiKey();

    // Build query to find admin by ID
    QUrl url(baseUrl + ":runQuery?key=" + apiKey);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    QJsonObject structuredQuery;
    QJsonObject fromObj;
    fromObj["collectionId"] = "admins";
    structuredQuery["from"] = QJsonArray{fromObj};

    // Where clause: adminId == input
    QJsonObject whereClause;
    QJsonObject fieldFilter;
    QJsonObject field;
    field["fieldPath"] = "adminId";
    fieldFilter["field"] = field;
    fieldFilter["op"] = "EQUAL";
    fieldFilter["value"] = QJsonObject{{"stringValue", adminId}};
    whereClause["fieldFilter"] = fieldFilter;
    structuredQuery["where"] = whereClause;

    QJsonObject queryObj;
    queryObj["structuredQuery"] = structuredQuery;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(queryObj).toJson());
    
    // Store credentials for later verification
    reply->setProperty("adminPassword", password);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onLoginReply(reply);
    });
}

void AdminLoginWindow::onLoginReply(QNetworkReply* reply) {
    m_isLoggingIn = false;
    m_loginButton->setEnabled(true);
    m_loginButton->setText("LOGIN");

    QString password = reply->property("adminPassword").toString();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        // Network or Firebase error - do NOT grant access
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        showError(QString("Connection failed (code %1). Check internet and try again.").arg(httpCode));
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);

    if (!doc.isArray()) {
        showError("Authentication error. Please try again.");
        return;
    }

    QJsonArray results = doc.array();
    
    if (results.isEmpty() || !results[0].toObject().contains("document")) {
        showError("Invalid admin credentials");
        return;
    }

    // Admin found - verify password against its salted hash.
    // Firestore must store "passwordSalt" (hex) and "passwordHash"
    // (hex SHA-256 of salt||password); plaintext is never stored or compared.
    QJsonObject document = results[0].toObject()["document"].toObject();
    QJsonObject fields = document["fields"].toObject();
    QString storedHash = fields["passwordHash"].toObject()["stringValue"].toString();
    QString salt = fields["passwordSalt"].toObject()["stringValue"].toString();

    if (storedHash.isEmpty() || salt.isEmpty()) {
        // Fail closed: legacy documents with a plaintext "password" field
        // are never honored — the account must be migrated to hashed storage.
        qWarning() << "[AdminLogin] Admin document lacks passwordHash/passwordSalt;"
                   << "legacy plaintext credentials are not accepted.";
        showError("This account uses legacy password storage. "
                  "Please reset the password to a hashed credential.");
        return;
    }

    if (!constantTimeEqual(hashPassword(salt, password), storedHash.toUtf8())) {
        showError("Invalid password");
        return;
    }

    // Login successful
    QString adminDocId = document["name"].toString().split("/").last();
    QString adminName = fields["name"].toString();
    QString token = fields["token"].toString();

    showSuccess("Login successful!");
    
    // Open dashboard
    AdminDashboardWindow* dashboard = new AdminDashboardWindow(this, adminDocId, token);
    dashboard->show();
    hide();
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
