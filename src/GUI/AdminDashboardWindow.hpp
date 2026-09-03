/**
 * @file AdminDashboardWindow.hpp
 * @brief Admin Dashboard Window - Full Firebase Integration
 * @version 1.0.0
 */

#ifndef ADMINDASHBOARDWINDOW_HPP
#define ADMINDASHBOARDWINDOW_HPP

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QLayout>
#include <QWidget>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "FirebaseHelper.hpp"

class AdminDashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminDashboardWindow(QWidget* parent = nullptr, 
                                 const QString& adminId = "", 
                                 const QString& authToken = "");
    ~AdminDashboardWindow();

private slots:
    // Tab switching
    void onTabChanged(int index);

    // Access Requests
    void onRefreshRequests();
    void onApproveRequest(int row);
    void onRejectRequest(int row);
    void onAccessRequestsResponse(const QList<FirebaseHelper::AccessRequest>& requests);
    void onApproveResponse(const QString& docId, const QString& accessCode);
    void onRejectResponse(const QString& docId);
    void onRequestError(const QString& error);

    // Active Users
    void onRefreshUsers();
    void onBlockUser(int row);
    void onUnblockUser(int row);
    void onDeleteUser(int row);
    void onExtendUser(int row);
    void onActiveUsersResponse(const QList<FirebaseHelper::ActiveUser>& users);
    void onUserCreated(const QString& docId, const QString& accessCode);
    void onUserUpdated(const QString& docId);
    void onUserDeleted(const QString& docId);
    void onUserError(const QString& error);

    // Manual user creation
    void onCreateUser();

    // Logout
    void onLogout();

private:
    void setupUI();
    void setupAccessRequestsTab();
    void setupActiveUsersTab();
    void populateAccessRequestsTable(const QList<FirebaseHelper::AccessRequest>& requests);
    void populateActiveUsersTable(const QList<FirebaseHelper::ActiveUser>& users);
    void showMessage(const QString& title, const QString& message, bool isError = false);

    QString m_adminId;
    QString m_authToken;
    FirebaseHelper::FirestoreClient* m_firebaseClient;

    // Pending approve operation (request is approved first, then the user is
    // created with the same access code once approval succeeds)
    QString m_pendingApproveName;
    QString m_pendingApprovePhone;
    int m_pendingApproveProfiles = 0;
    int m_pendingApproveDays = 0;
    QString m_pendingApproveCode;

    // UI Elements
    QTabWidget* m_tabWidget;
    
    // Access Requests Tab
    QWidget* m_requestsTab;
    QTableWidget* m_requestsTable;
    QPushButton* m_refreshRequestsBtn;
    QLabel* m_requestsStatusLabel;
    
    // Active Users Tab
    QWidget* m_usersTab;
    QTableWidget* m_usersTable;
    QPushButton* m_refreshUsersBtn;
    QPushButton* m_createUserBtn;
    QLabel* m_usersStatusLabel;
};

// Create User Dialog
class CreateUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateUserDialog(QWidget* parent = nullptr);
    QString getUserName() const { return m_userNameInput->text().trimmed(); }
    QString getPhoneNumber() const { return m_phoneInput->text().trimmed(); }
    int getProfileCount() const { return m_profileCountSpin->value(); }
    int getDurationDays() const { return m_durationSpin->value(); }

private:
    QLineEdit* m_userNameInput;
    QLineEdit* m_phoneInput;
    QSpinBox* m_profileCountSpin;
    QSpinBox* m_durationSpin;
};

#endif // ADMINDASHBOARDWINDOW_HPP
