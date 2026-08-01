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

// Firebase Configuration
const QString FIREBASE_PROJECT_ID = QStringLiteral("redroid-d8110");
const QString FIREBASE_API_KEY = QStringLiteral("AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA");
const QString FIREBASE_BASE_URL = QStringLiteral("https://firestore.googleapis.com/v1/projects/") + FIREBASE_PROJECT_ID + QStringLiteral("/databases/(default)/documents");

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    setWindowTitle("ReDroidCPP - Login");
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
            padding: 12px;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
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
            padding: 12px;
            border-radius: 8px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #3b82f6;
        }
        QComboBox {
            background-color: #1e293b;
            color: white;
            border: 1px solid #334155;
            padding: 12px;
            border-radius: 8px;
            font-size: 14px;
        }
        QSpinBox {
            background-color: #1e293b;
            color: white;
            border: 1px solid #334155;
            padding: 12px;
            border-radius: 8px;
            font-size: 14px;
        }
        QLabel#titleLabel {
            font-size: 24px;
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
        }
        .success {
            background-color: rgba(22, 163, 74, 0.2);
            color: #86efac;
            border: 1px solid rgba(22, 163, 74, 0.3);
        }
        QLabel#loadingLabel {
            color: #60a5fa;
            font-size: 12px;
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

    // Code Input
    QLabel *codeLabel = new QLabel("আপনার ৮-সংখ্যার কোড দিন:", loginPage);
    codeInput = new QLineEdit(loginPage);
    codeInput->setPlaceholderText("XXXXXXXX");
    codeInput->setMaxLength(8);
    codeInput->setAlignment(Qt::AlignCenter);
    codeInput->setFont(QFont("Segoe UI", 18, QFont::Bold));

    // Status
    statusLabel = new QLabel(loginPage);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setVisible(false);

    // Buttons
    btnLogin = new QPushButton("লগইন", loginPage);
    btnSwitchToRequest = new QPushButton("নতুন অ্যাকাউন্ট চান? রিকোয়েস্ট পাঠান", loginPage);
    btnSwitchToRequest->setStyleSheet("background-color: transparent; color: #60a5fa; font-size: 12px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(codeLabel);
    mainLayout->addWidget(codeInput);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(btnLogin);
    mainLayout->addWidget(btnSwitchToRequest);

    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(btnSwitchToRequest, &QPushButton::clicked, this, &LoginWindow::onSwitchToRequest);
    connect(codeInput, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::setupRequestUI() {
    requestPage = new QWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(requestPage);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(15);

    // Title
    QLabel *titleLabel = new QLabel("📝 Access Request", loginPage);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel("নতুন অ্যাকাউন্টের জন্য আবেদন করুন", loginPage);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    // Name
    QLabel *nameLabel = new QLabel("আপনার নাম:", loginPage);
    nameInput = new QLineEdit(requestPage);
    nameInput->setPlaceholderText("আপনার নাম লিখুন");

    // Phone
    QLabel *phoneLabel = new QLabel("ফোন নম্বর:", loginPage);
    phoneInput = new QLineEdit(requestPage);
    phoneInput->setPlaceholderText("01XXXXXXXXX");

    // Profiles
    QLabel *profilesLabel = new QLabel("কতটি প্রোফাইল চান?", loginPage);
    profilesSpinBox = new QSpinBox(requestPage);
    profilesSpinBox->setMinimum(1);
    profilesSpinBox->setMaximum(10);
    profilesSpinBox->setValue(3);

    // Duration
    QLabel *durationLabel = new QLabel("কতক্ষণ চান?", loginPage);
    durationCombo = new QComboBox(requestPage);
    durationCombo->addItem("৫ মিনিট", 5);
    durationCombo->addItem("১৫ মিনিট", 15);
    durationCombo->addItem("৩০ মিনিট", 30);
    durationCombo->addItem("৬০ মিনিট (১ ঘণ্টা)", 60);
    durationCombo->addItem("১ দিন", 1440);
    durationCombo->addItem("৩ দিন", 4320);
    durationCombo->addItem("৭ দিন (১ সপ্তাহ)", 10080);
    durationCombo->addItem("১৫ দিন", 21600);
    durationCombo->addItem("১ মাস", 43200);
    durationCombo->addItem("১ বছর", 525600);

    // Status
    requestStatusLabel = new QLabel(requestPage);
    requestStatusLabel->setObjectName("statusLabel");
    requestStatusLabel->setAlignment(Qt::AlignCenter);
    requestStatusLabel->setVisible(false);

    // Buttons
    btnSendRequest = new QPushButton("রিকোয়েস্ট পাঠান", requestPage);
    btnSwitchToLogin = new QPushButton("আগে থেকে কোড আছে? লগইন করুন", requestPage);
    btnSwitchToLogin->setStyleSheet("background-color: transparent; color: #60a5fa; font-size: 12px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(nameLabel);
    mainLayout->addWidget(nameInput);
    mainLayout->addWidget(phoneLabel);
    mainLayout->addWidget(phoneInput);
    mainLayout->addWidget(profilesLabel);
    mainLayout->addWidget(profilesSpinBox);
    mainLayout->addWidget(durationLabel);
    mainLayout->addWidget(durationCombo);
    mainLayout->addWidget(requestStatusLabel);
    mainLayout->addWidget(btnSendRequest);
    mainLayout->addWidget(btnSwitchToLogin);

    connect(btnSendRequest, &QPushButton::clicked, this, &LoginWindow::onSendRequestClicked);
    connect(btnSwitchToLogin, &QPushButton::clicked, this, &LoginWindow::onSwitchToLogin);
}

void LoginWindow::onLoginClicked() {
    QString code = codeInput->text().trimmed();

    if (code.isEmpty()) {
        showError("কোড দিন");
        return;
    }

    if (code.length() != 8) {
        showError("কোড ৮ সংখ্যার হতে হবে");
        return;
    }

    btnLogin->setEnabled(false);
    btnLogin->setText("যাচাই হচ্ছে...");
    verifyCode(code);
}

void LoginWindow::verifyCode(const QString &code) {
    // Firebase Firestore REST API - Query by uniqueKey using simple document fetch
    // For simplicity, we'll fetch all activeUsers and filter client-side
    // In production, use a proper query index
    
    QString queryUrl = QStringLiteral("https://firestore.googleapis.com/v1/projects/") + FIREBASE_PROJECT_ID + 
                    QStringLiteral("/databases/(default)/documents:runQuery?key=") + FIREBASE_API_KEY;
    
    QNetworkRequest queryRequest(QUrl(queryUrl));
    queryRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Build simple query to get all activeUsers
    QJsonObject structuredQuery;
    
    QJsonObject fromObj;
    fromObj["collectionId"] = QStringLiteral("activeUsers");
    QJsonArray fromArray;
    fromArray.append(fromObj);
    structuredQuery["from"] = fromArray;
    
    // Where clause: uniqueKey == code
    QJsonObject whereClause;
    QJsonObject fieldFilter;
    QJsonObject field;
    field["fieldPath"] = QStringLiteral("uniqueKey");
    fieldFilter["field"] = field;
    fieldFilter["op"] = QStringLiteral("EQUAL");
    
    QJsonObject value;
    value["stringValue"] = code;
    fieldFilter["value"] = value;
    
    QJsonObject compositeFilter;
    compositeFilter["op"] = QStringLiteral("AND");
    
    QJsonArray filtersArray;
    QJsonObject filterWrapper;
    filterWrapper["fieldFilter"] = fieldFilter;
    filtersArray.append(filterWrapper);
    compositeFilter["filters"] = filtersArray;
    
    whereClause["compositeFilter"] = compositeFilter;
    structuredQuery["where"] = whereClause;
    
    QJsonObject queryObj;
    queryObj["structuredQuery"] = structuredQuery;
    
    QJsonDocument doc(queryObj);
    QByteArray data = doc.toJson();
    
    QNetworkReply *reply = networkManager->post(queryRequest, data);
    pendingRequestId = code;
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->handleLoginResponse(reply);
    });
}

void LoginWindow::handleLoginResponse(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        showError("নেটওয়ার্ক সমস্যা: " + reply->errorString());
        btnLogin->setEnabled(true);
        btnLogin->setText("লগইন");
        reply->deleteLater();
        return;
    }
    
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    
    // Parse runQuery response - it returns an array of document results
    if (!doc.isArray()) {
        showError("সার্ভার সমস্যা");
        btnLogin->setEnabled(true);
        btnLogin->setText("লগইন");
        reply->deleteLater();
        return;
    }
    
    QJsonArray results = doc.array();
    
    // Check if we got a valid document
    if (results.isEmpty() || !results[0].toObject().contains("document")) {
        showError("এই কোডটি পাওয়া যায়নি");
        btnLogin->setEnabled(true);
        btnLogin->setText("লগইন");
        reply->deleteLater();
        return;
    }
    
    QJsonObject document = results[0].toObject()["document"].toObject();
    QJsonObject fields = document["fields"].toObject();
    
    // Check if blocked
    if (fields.contains("isBlocked") && fields["isBlocked"].toObject()["booleanValue"].toBool()) {
        showError("আপনার অ্যাকাউন্ট ব্লক করা আছে। Admin এর সাথে যোগাযোগ করুন।");
        btnLogin->setEnabled(true);
        btnLogin->setText("লগইন");
        reply->deleteLater();
        return;
    }
    
    // Get user data
    QString userId = document["name"].toString().split("/").last();
    QString uniqueKey = fields.contains("uniqueKey") ? fields["uniqueKey"].toObject()["stringValue"].toString() : "";
    int remainingProfiles = fields.contains("remainingProfiles") ? fields["remainingProfiles"].toObject()["integerValue"].toInt() : 0;
    int totalProfiles = fields.contains("totalProfiles") ? fields["totalProfiles"].toObject()["integerValue"].toInt() : 0;
    
    // Success! Hide window and emit login success
    hide();
    emit loginSuccess(userId, uniqueKey, remainingProfiles, totalProfiles);
    
    btnLogin->setEnabled(true);
    btnLogin->setText("লগইন");
    reply->deleteLater();
}

void LoginWindow::onSendRequestClicked() {
    QString name = nameInput->text().trimmed();
    QString phone = phoneInput->text().trimmed();
    int profiles = profilesSpinBox->value();
    int duration = durationCombo->currentData().toInt();

    if (phone.isEmpty()) {
        showRequestError("ফোন নম্বর দিন");
        return;
    }

    btnSendRequest->setEnabled(false);
    btnSendRequest->setText("পাঠানো হচ্ছে...");
    sendAccessRequest(name, phone, profiles, duration);
}

void LoginWindow::sendAccessRequest(const QString &name, const QString &phone, int profiles, int duration) {
    // Firebase Firestore REST API - Create document
    QString url = QStringLiteral("https://firestore.googleapis.com/v1/projects/") + FIREBASE_PROJECT_ID + 
                 QStringLiteral("/databases/(default)/documents/accessRequests?key=") + FIREBASE_API_KEY;
    
    QNetworkRequest request(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
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
    
    QNetworkReply *reply = networkManager->post(request, data);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->handleRequestResponse(reply);
    });
}

void LoginWindow::handleRequestResponse(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        showRequestError("সমস্যা হয়েছে: " + reply->errorString());
        btnSendRequest->setEnabled(true);
        btnSendRequest->setText("রিকোয়েস্ট পাঠান");
        reply->deleteLater();
        return;
    }
    
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();
    
    if (obj.contains("name")) {
        QString docName = obj["name"].toString();
        showRequestSuccess("রিকোয়েস্ট পাঠানো হয়েছে!\n\nআপনার অনুরোধ Admin কে জানানো হয়েছে। Admin approve করলে আপনি কোড পাবেন।");
        
        // Clear form
        nameInput->clear();
        phoneInput->clear();
        profilesSpinBox->setValue(3);
        durationCombo->setCurrentIndex(4); // 1 day
        
        // Switch to login after 3 seconds
        QTimer::singleShot(3000, this, &LoginWindow::onSwitchToLogin);
    } else {
        showRequestError("রিকোয়েস্ট পাঠাতে সমস্যা হয়েছে");
    }
    
    btnSendRequest->setEnabled(true);
    btnSendRequest->setText("রিকোয়েস্ট পাঠান");
    reply->deleteLater();
}

void LoginWindow::onNetworkReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        showError("নেটওয়ার্ক সমস্যা: " + reply->errorString());
        btnLogin->setEnabled(true);
        btnLogin->setText("লগইন");
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();

    if (obj.contains("documents")) {
        QJsonArray docs = obj["documents"].toArray();
        if (docs.isEmpty()) {
            showError("এই কোডটি পাওয়া যায়নি");
        } else {
            QJsonObject userDoc = docs.first().toObject();
            QJsonObject fields = userDoc["fields"].toObject();

            bool isBlocked = fields.contains("isBlocked") && fields["isBlocked"].toObject()["booleanValue"].toBool();

            if (isBlocked) {
                showError("আপনার অ্যাকাউন্ট ব্লক করা আছে। Admin এর সাথে যোগাযোগ করুন।");
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
        showError("কোড যাচাই ব্যর্থ");
    }

    btnLogin->setEnabled(true);
    btnLogin->setText("লগইন");
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
