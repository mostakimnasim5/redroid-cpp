/**
 * @file AdminDashboardWindow.cpp
 * @brief Admin Dashboard Window - Full Firebase Integration
 * @version 1.0.0
 */

#include "AdminDashboardWindow.hpp"
#include "AdminLoginWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QDebug>
#include <QInputDialog>
#include <QTimer>

// ============================================================================
// Create User Dialog
// ============================================================================

CreateUserDialog::CreateUserDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create New User");
    setModal(true);
    setFixedSize(450, 380);
    setStyleSheet(R"(
        QDialog {
            background-color: #1a1a2e;
            color: #ffffff;
        }
        QLabel {
            color: #e0e0e0;
            font-size: 14px;
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
        }
        QSpinBox {
            padding: 12px;
            border: 2px solid #4a4a6a;
            border-radius: 6px;
            background-color: #0f0f23;
            color: #ffffff;
            font-size: 14px;
        }
        QPushButton {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#createBtn {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00d4ff, stop:1 #0099cc);
            color: white;
        }
        QPushButton#cancelBtn {
            background-color: #4a4a6a;
            color: white;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(25, 25, 25, 25);

    // Title
    QLabel* titleLabel = new QLabel("Create New User Account", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #00d4ff; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // User Name
    QLabel* nameLabel = new QLabel("User Name:", this);
    mainLayout->addWidget(nameLabel);
    m_userNameInput = new QLineEdit(this);
    m_userNameInput->setPlaceholderText("Enter user's full name");
    mainLayout->addWidget(m_userNameInput);

    // Phone Number
    QLabel* phoneLabel = new QLabel("Phone Number:", this);
    mainLayout->addWidget(phoneLabel);
    m_phoneInput = new QLineEdit(this);
    m_phoneInput->setPlaceholderText("+8801XXXXXXXXX");
    mainLayout->addWidget(m_phoneInput);

    // Profile Count
    QLabel* profileLabel = new QLabel("Number of Profiles:", this);
    mainLayout->addWidget(profileLabel);
    m_profileCountSpin = new QSpinBox(this);
    m_profileCountSpin->setMinimum(1);
    m_profileCountSpin->setMaximum(100);
    m_profileCountSpin->setValue(3);
    mainLayout->addWidget(m_profileCountSpin);

    // Duration
    QLabel* durationLabel = new QLabel("Duration (Days):", this);
    mainLayout->addWidget(durationLabel);
    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setMinimum(1);
    m_durationSpin->setMaximum(365);
    m_durationSpin->setValue(30);
    mainLayout->addWidget(m_durationSpin);

    // Spacer
    mainLayout->addSpacing(15);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* createBtn = new QPushButton("Create User", this);
    createBtn->setObjectName("createBtn");
    connect(createBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(createBtn);

    mainLayout->addLayout(btnLayout);
}

// ============================================================================
// Admin Dashboard Window
// ============================================================================

AdminDashboardWindow::AdminDashboardWindow(QWidget* parent, const QString& adminId, const QString& authToken)
    : QMainWindow(parent)
    , m_adminId(adminId)
    , m_authToken(authToken)
    , m_firebaseClient(new FirebaseHelper::FirestoreClient(this))
{
    setWindowTitle("Admin Dashboard - ReDroidCPP");
    setMinimumSize(1000, 700);
    resize(1200, 800);
    
    if (!m_authToken.isEmpty()) {
        m_firebaseClient->setAuthToken(m_authToken);
    }

    setStyleSheet(R"(
        QMainWindow {
            background-color: #0f172a;
        }
        QTabWidget::pane {
            border: 1px solid #334155;
            background-color: #1e293b;
            border-radius: 8px;
        }
        QTabBar::tab {
            background-color: #1e293b;
            color: #94a3b8;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QTabBar::tab:selected {
            background-color: #3b82f6;
            color: white;
        }
        QTabBar::tab:hover:!selected {
            background-color: #334155;
            color: #e2e8f0;
        }
        QTableWidget {
            background-color: #1e293b;
            color: #e2e8f0;
            border: none;
            gridline-color: #334155;
            font-size: 13px;
        }
        QTableWidget::item {
            padding: 8px;
        }
        QTableWidget::item:selected {
            background-color: #3b82f6;
        }
        QHeaderView::section {
            background-color: #334155;
            color: #e2e8f0;
            padding: 10px;
            font-weight: bold;
            font-size: 13px;
            border: none;
        }
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            font-size: 13px;
            font-weight: bold;
            min-height: 35px;
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
        .btn-success {
            background-color: #22c55e;
        }
        .btn-success:hover {
            background-color: #16a34a;
        }
        .btn-danger {
            background-color: #ef4444;
        }
        .btn-danger:hover {
            background-color: #dc2626;
        }
        .btn-warning {
            background-color: #f59e0b;
            color: black;
        }
        .btn-warning:hover {
            background-color: #d97706;
        }
        QLabel {
            color: #e2e8f0;
        }
        QLabel#statusLabel {
            padding: 10px;
            font-size: 13px;
        }
        .success {
            color: #22c55e;
        }
        .error {
            color: #ef4444;
        }
        .info {
            color: #60a5fa;
        }
    )");

    setupUI();

    // Connect Firebase signals
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::accessRequestsFetched,
            this, &AdminDashboardWindow::onAccessRequestsResponse);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::accessRequestApproved,
            this, &AdminDashboardWindow::onApproveResponse);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::accessRequestRejected,
            this, &AdminDashboardWindow::onRejectResponse);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::accessRequestError,
            this, &AdminDashboardWindow::onRequestError);

    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::activeUsersFetched,
            this, &AdminDashboardWindow::onActiveUsersResponse);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::activeUserCreated,
            this, &AdminDashboardWindow::onUserCreated);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::activeUserUpdated,
            this, &AdminDashboardWindow::onUserUpdated);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::activeUserDeleted,
            this, &AdminDashboardWindow::onUserDeleted);
    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::activeUserError,
            this, &AdminDashboardWindow::onUserError);

    connect(m_firebaseClient, &FirebaseHelper::FirestoreClient::networkError,
            this, [this](const QString& error) {
                showMessage("Network Error", error, true);
            });

    // Load initial data
    QTimer::singleShot(100, this, [this]() {
        onRefreshRequests();
        onRefreshUsers();
    });
}

AdminDashboardWindow::~AdminDashboardWindow() {
}

void AdminDashboardWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("📊 Admin Dashboard", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #60a5fa;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QPushButton* logoutBtn = new QPushButton("Logout", this);
    logoutBtn->setStyleSheet("background-color: #ef4444;");
    connect(logoutBtn, &QPushButton::clicked, this, &AdminDashboardWindow::onLogout);
    headerLayout->addWidget(logoutBtn);

    mainLayout->addLayout(headerLayout);

    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &AdminDashboardWindow::onTabChanged);

    // Access Requests Tab
    setupAccessRequestsTab();
    m_tabWidget->addTab(m_requestsTab, "📋 Access Requests");

    // Active Users Tab
    setupActiveUsersTab();
    m_tabWidget->addTab(m_usersTab, "👥 Active Users");

    mainLayout->addWidget(m_tabWidget);
}

void AdminDashboardWindow::setupAccessRequestsTab() {
    m_requestsTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_requestsTab);
    layout->setSpacing(10);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("Pending Access Requests", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_refreshRequestsBtn = new QPushButton("🔄 Refresh", this);
    connect(m_refreshRequestsBtn, &QPushButton::clicked, this, &AdminDashboardWindow::onRefreshRequests);
    headerLayout->addWidget(m_refreshRequestsBtn);

    layout->addLayout(headerLayout);

    // Table
    m_requestsTable = new QTableWidget(this);
    m_requestsTable->setColumnCount(7);
    m_requestsTable->setHorizontalHeaderLabels({
        "ID", "Name", "Phone", "Profiles", "Duration", "Status", "Actions"
    });
    m_requestsTable->horizontalHeader()->setStretchLastSection(true);
    m_requestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_requestsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_requestsTable->setAlternatingRowColors(true);
    layout->addWidget(m_requestsTable);

    // Status
    m_requestsStatusLabel = new QLabel("Loading...", this);
    m_requestsStatusLabel->setObjectName("statusLabel");
    m_requestsStatusLabel->setStyleSheet("color: #64748b;");
    layout->addWidget(m_requestsStatusLabel);
}

void AdminDashboardWindow::setupActiveUsersTab() {
    m_usersTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_usersTab);
    layout->setSpacing(10);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("Active Users", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_createUserBtn = new QPushButton("➕ Create User", this);
    m_createUserBtn->setStyleSheet("background-color: #22c55e;");
    connect(m_createUserBtn, &QPushButton::clicked, this, &AdminDashboardWindow::onCreateUser);
    headerLayout->addWidget(m_createUserBtn);

    m_refreshUsersBtn = new QPushButton("🔄 Refresh", this);
    connect(m_refreshUsersBtn, &QPushButton::clicked, this, &AdminDashboardWindow::onRefreshUsers);
    headerLayout->addWidget(m_refreshUsersBtn);

    layout->addLayout(headerLayout);

    // Table
    m_usersTable = new QTableWidget(this);
    m_usersTable->setColumnCount(8);
    m_usersTable->setHorizontalHeaderLabels({
        "ID", "Access Code", "Name", "Phone", "Profiles (Total/Rem)", "Status", "Expires", "Actions"
    });
    m_usersTable->horizontalHeader()->setStretchLastSection(true);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setAlternatingRowColors(true);
    layout->addWidget(m_usersTable);

    // Status
    m_usersStatusLabel = new QLabel("Loading...", this);
    m_usersStatusLabel->setObjectName("statusLabel");
    m_usersStatusLabel->setStyleSheet("color: #64748b;");
    layout->addWidget(m_usersStatusLabel);
}

// ============================================================================
// Access Requests Management
// ============================================================================

void AdminDashboardWindow::onRefreshRequests() {
    m_refreshRequestsBtn->setEnabled(false);
    m_refreshRequestsBtn->setText("Loading...");
    m_requestsStatusLabel->setText("Fetching access requests...");
    m_requestsStatusLabel->setStyleSheet("color: #60a5fa;");
    m_firebaseClient->fetchAccessRequests();
}

void AdminDashboardWindow::populateAccessRequestsTable(const QList<FirebaseHelper::AccessRequest>& requests) {
    m_requestsTable->setRowCount(0);

    for (const FirebaseHelper::AccessRequest& req : requests) {
        int row = m_requestsTable->rowCount();
        m_requestsTable->insertRow(row);

        m_requestsTable->setItem(row, 0, new QTableWidgetItem(req.id));
        m_requestsTable->setItem(row, 1, new QTableWidgetItem(req.userName));
        m_requestsTable->setItem(row, 2, new QTableWidgetItem(req.contactNumber));
        m_requestsTable->setItem(row, 3, new QTableWidgetItem(QString::number(req.profileCount)));
        
        // Duration in days
        int days = req.durationMinutes / 1440;
        m_requestsTable->setItem(row, 4, new QTableWidgetItem(days > 0 ? QString("%1 days").arg(days) : "Same day"));

        // Status with color
        QTableWidgetItem* statusItem = new QTableWidgetItem(req.status.toUpper());
        if (req.status == "pending") {
            statusItem->setBackground(QBrush(QColor("#f59e0b")));
            statusItem->setForeground(QBrush(Qt::black));
        } else if (req.status == "approved") {
            statusItem->setBackground(QBrush(QColor("#22c55e")));
            statusItem->setForeground(QBrush(Qt::white));
        } else if (req.status == "rejected") {
            statusItem->setBackground(QBrush(QColor("#ef4444")));
            statusItem->setForeground(QBrush(Qt::white));
        }
        m_requestsTable->setItem(row, 5, statusItem);

        // Actions
        QWidget* actionWidget = new QWidget();
        QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(5, 5, 5, 5);

        if (req.status == "pending") {
            QPushButton* approveBtn = new QPushButton("✓ Approve");
            approveBtn->setStyleSheet("background-color: #22c55e; padding: 5px 10px; font-size: 12px;");
            connect(approveBtn, &QPushButton::clicked, this, [this, row]() { onApproveRequest(row); });
            actionLayout->addWidget(approveBtn);

            QPushButton* rejectBtn = new QPushButton("✗ Reject");
            rejectBtn->setStyleSheet("background-color: #ef4444; padding: 5px 10px; font-size: 12px;");
            connect(rejectBtn, &QPushButton::clicked, this, [this, row]() { onRejectRequest(row); });
            actionLayout->addWidget(rejectBtn);
        }

        m_requestsTable->setCellWidget(row, 6, actionWidget);
    }

    m_requestsStatusLabel->setText(QString("Found %1 request(s)").arg(requests.count()));
    m_requestsStatusLabel->setStyleSheet("color: #22c55e;");
    m_refreshRequestsBtn->setEnabled(true);
    m_refreshRequestsBtn->setText("🔄 Refresh");
}

void AdminDashboardWindow::onApproveRequest(int row) {
    QString docId = m_requestsTable->item(row, 0)->text();
    QString userName = m_requestsTable->item(row, 1)->text();
    
    // Get details from stored data
    QString phone = m_requestsTable->item(row, 2)->text();
    int profiles = m_requestsTable->item(row, 3)->text().toInt();
    int days = m_requestsTable->item(row, 4)->text().replace(" days", "").toInt();
    if (days == 0) days = 1;

    // Generate access code
    QString accessCode = FirebaseHelper::FirestoreClient::generateAccessCode();

    // Create active user
    m_firebaseClient->createActiveUser(userName, phone, profiles, days);

    // Show info with access code
    QMessageBox::information(this, "Request Approved", 
        QString("<h3>✅ User Approved!</h3>"
                "<p><b>User:</b> %1</p>"
                "<p><b>Access Code:</b> <span style='color: #22c55e; font-size: 20px; font-weight: bold;'>%2</span></p>"
                "<p><b>Share this code securely with the user!</b></p>").arg(userName, accessCode));

    onRefreshRequests();
}

void AdminDashboardWindow::onRejectRequest(int row) {
    QString docId = m_requestsTable->item(row, 0)->text();
    QString userName = m_requestsTable->item(row, 1)->text();

    bool ok;
    QString reason = QInputDialog::getText(this, "Reject Request",
        QString("Enter rejection reason for %1:").arg(userName),
        QLineEdit::Normal, "", &ok);

    if (ok) {
        m_firebaseClient->rejectAccessRequest(docId, reason);
        showMessage("Request Rejected", QString("%1 has been rejected").arg(userName));
    }
}

void AdminDashboardWindow::onAccessRequestsResponse(const QList<FirebaseHelper::AccessRequest>& requests) {
    populateAccessRequestsTable(requests);
}

void AdminDashboardWindow::onApproveResponse(const QString& docId, const QString& accessCode) {
    Q_UNUSED(docId);
    m_requestsStatusLabel->setText("Request approved! Access code: " + accessCode);
    m_requestsStatusLabel->setStyleSheet("color: #22c55e;");
}

void AdminDashboardWindow::onRejectResponse(const QString& docId) {
    Q_UNUSED(docId);
    onRefreshRequests();
}

void AdminDashboardWindow::onRequestError(const QString& error) {
    m_requestsStatusLabel->setText("Error: " + error);
    m_requestsStatusLabel->setStyleSheet("color: #ef4444;");
    m_refreshRequestsBtn->setEnabled(true);
    m_refreshRequestsBtn->setText("🔄 Refresh");
}

// ============================================================================
// Active Users Management
// ============================================================================

void AdminDashboardWindow::onRefreshUsers() {
    m_refreshUsersBtn->setEnabled(false);
    m_refreshUsersBtn->setText("Loading...");
    m_usersStatusLabel->setText("Fetching active users...");
    m_usersStatusLabel->setStyleSheet("color: #60a5fa;");
    m_firebaseClient->fetchActiveUsers();
}

void AdminDashboardWindow::populateActiveUsersTable(const QList<FirebaseHelper::ActiveUser>& users) {
    m_usersTable->setRowCount(0);

    for (const FirebaseHelper::ActiveUser& user : users) {
        int row = m_usersTable->rowCount();
        m_usersTable->insertRow(row);

        m_usersTable->setItem(row, 0, new QTableWidgetItem(user.id));
        m_usersTable->setItem(row, 1, new QTableWidgetItem(user.uniqueKey));
        m_usersTable->setItem(row, 2, new QTableWidgetItem(user.userName));
        m_usersTable->setItem(row, 3, new QTableWidgetItem(user.phoneNumber));
        m_usersTable->setItem(row, 4, new QTableWidgetItem(QString("%1/%2").arg(user.totalProfiles).arg(user.remainingProfiles)));

        // Status with color
        QTableWidgetItem* statusItem = new QTableWidgetItem(user.isBlocked ? "BLOCKED" : "ACTIVE");
        if (user.isBlocked) {
            statusItem->setBackground(QBrush(QColor("#ef4444")));
            statusItem->setForeground(QBrush(Qt::white));
        } else {
            statusItem->setBackground(QBrush(QColor("#22c55e")));
            statusItem->setForeground(QBrush(Qt::white));
        }
        m_usersTable->setItem(row, 5, statusItem);

        // Expiration
        QString expiry = user.expiresAt;
        if (expiry.contains("T")) {
            expiry = expiry.split("T").first();
        }
        m_usersTable->setItem(row, 6, new QTableWidgetItem(expiry));

        // Actions
        QWidget* actionWidget = new QWidget();
        QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(5, 5, 5, 5);

        if (user.isBlocked) {
            QPushButton* unblockBtn = new QPushButton("🔓 Unblock");
            unblockBtn->setStyleSheet("background-color: #22c55e; padding: 5px 8px; font-size: 11px;");
            connect(unblockBtn, &QPushButton::clicked, this, [this, row]() { onUnblockUser(row); });
            actionLayout->addWidget(unblockBtn);
        } else {
            QPushButton* blockBtn = new QPushButton("🔒 Block");
            blockBtn->setStyleSheet("background-color: #f59e0b; padding: 5px 8px; font-size: 11px;");
            connect(blockBtn, &QPushButton::clicked, this, [this, row]() { onBlockUser(row); });
            actionLayout->addWidget(blockBtn);
        }

        QPushButton* extendBtn = new QPushButton("⏰ Extend");
        extendBtn->setStyleSheet("background-color: #3b82f6; padding: 5px 8px; font-size: 11px;");
        connect(extendBtn, &QPushButton::clicked, this, [this, row]() { onExtendUser(row); });
        actionLayout->addWidget(extendBtn);

        QPushButton* deleteBtn = new QPushButton("🗑️");
        deleteBtn->setStyleSheet("background-color: #ef4444; padding: 5px 10px; font-size: 11px;");
        connect(deleteBtn, &QPushButton::clicked, this, [this, row]() { onDeleteUser(row); });
        actionLayout->addWidget(deleteBtn);

        m_usersTable->setCellWidget(row, 7, actionWidget);
    }

    m_usersStatusLabel->setText(QString("Found %1 user(s)").arg(users.count()));
    m_usersStatusLabel->setStyleSheet("color: #22c55e;");
    m_refreshUsersBtn->setEnabled(true);
    m_refreshUsersBtn->setText("🔄 Refresh");
}

void AdminDashboardWindow::onBlockUser(int row) {
    QString docId = m_usersTable->item(row, 0)->text();
    QString userName = m_usersTable->item(row, 2)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Block User",
        QString("Are you sure you want to block %1?").arg(userName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_firebaseClient->updateUserStatus(docId, true);
    }
}

void AdminDashboardWindow::onUnblockUser(int row) {
    QString docId = m_usersTable->item(row, 0)->text();
    QString userName = m_usersTable->item(row, 2)->text();

    m_firebaseClient->updateUserStatus(docId, false);
    showMessage("User Unblocked", QString("%1 has been unblocked").arg(userName));
}

void AdminDashboardWindow::onDeleteUser(int row) {
    QString docId = m_usersTable->item(row, 0)->text();
    QString userName = m_usersTable->item(row, 2)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete User",
        QString("Are you sure you want to delete %1?\nThis action cannot be undone!").arg(userName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_firebaseClient->deleteUser(docId);
    }
}

void AdminDashboardWindow::onExtendUser(int row) {
    QString docId = m_usersTable->item(row, 0)->text();
    QString userName = m_usersTable->item(row, 2)->text();

    bool ok;
    int days = QInputDialog::getInt(this, "Extend Duration",
        QString("Extend duration for %1\nEnter additional days:").arg(userName),
        30, 1, 365, 1, &ok);

    if (ok) {
        m_firebaseClient->extendUserDuration(docId, days);
        showMessage("Duration Extended", QString("%1 extended by %2 days").arg(userName).arg(days));
    }
}

void AdminDashboardWindow::onCreateUser() {
    CreateUserDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString userName = dialog.getUserName();
        QString phone = dialog.getPhoneNumber();
        int profiles = dialog.getProfileCount();
        int days = dialog.getDurationDays();

        if (userName.isEmpty() || phone.isEmpty()) {
            showMessage("Validation Error", "Please fill in all required fields", true);
            return;
        }

        m_firebaseClient->createActiveUser(userName, phone, profiles, days);
    }
}

void AdminDashboardWindow::onActiveUsersResponse(const QList<FirebaseHelper::ActiveUser>& users) {
    populateActiveUsersTable(users);
}

void AdminDashboardWindow::onUserCreated(const QString& docId, const QString& accessCode) {
    Q_UNUSED(docId);
    showMessage("User Created", 
        QString("<h3>✅ New User Created!</h3>"
                "<p><b>Access Code:</b> <span style='color: #22c55e; font-size: 24px; font-weight: bold;'>%1</span></p>"
                "<p><b>Share this code securely with the user!</b></p>").arg(accessCode));
    onRefreshUsers();
}

void AdminDashboardWindow::onUserUpdated(const QString& docId) {
    Q_UNUSED(docId);
    onRefreshUsers();
}

void AdminDashboardWindow::onUserDeleted(const QString& docId) {
    Q_UNUSED(docId);
    showMessage("User Deleted", "User has been deleted successfully");
    onRefreshUsers();
}

void AdminDashboardWindow::onUserError(const QString& error) {
    m_usersStatusLabel->setText("Error: " + error);
    m_usersStatusLabel->setStyleSheet("color: #ef4444;");
    m_refreshUsersBtn->setEnabled(true);
    m_refreshUsersBtn->setText("🔄 Refresh");
}

// ============================================================================
// Utility
// ============================================================================

void AdminDashboardWindow::onTabChanged(int index) {
    Q_UNUSED(index);
}

void AdminDashboardWindow::onLogout() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        AdminLoginWindow* loginWindow = new AdminLoginWindow();
        loginWindow->show();
        close();
    }
}

void AdminDashboardWindow::showMessage(const QString& title, const QString& message, bool isError) {
    if (isError) {
        QMessageBox::critical(this, title, message);
    } else {
        QMessageBox::information(this, title, message);
    }
}
