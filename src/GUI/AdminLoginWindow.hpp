/**
 * @file AdminLoginWindow.hpp
 * @brief Admin Login Window - Firebase Admin Authentication
 * @version 1.0.0
 */

#ifndef ADMINLOGINWINDOW_HPP
#define ADMINLOGINWINDOW_HPP

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class AdminLoginWindow;
}

class AdminLoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminLoginWindow(QWidget* parent = nullptr);
    ~AdminLoginWindow();

signals:
    void adminLoginSuccess(const QString& adminId, const QString& token);
    void adminLogout();

private slots:
    void onLoginClicked();
    void onLoginReply(QNetworkReply* reply);
    void onSwitchToRequest();

private:
    void setupUI();
    void showError(const QString& message);
    void showSuccess(const QString& message);

    QWidget* loginPage;
    QLineEdit* m_adminIdInput;
    QLineEdit* m_passwordInput;
    QPushButton* m_loginButton;
    QLabel* m_statusLabel;
    QLabel* m_titleLabel;
    
    QNetworkAccessManager* m_networkManager;
    bool m_isLoggingIn;
};

#endif // ADMINLOGINWINDOW_HPP
