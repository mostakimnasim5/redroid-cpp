#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "VirtualPhonePro/ConfigManager.hpp"

class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

signals:
    void loginSuccess(const QString &userId, const QString &uniqueKey, int remainingProfiles, int totalProfiles);
    void accessRequestSent();

private slots:
    void onLoginClicked();
    void onSendRequestClicked();
    void onNetworkReply(QNetworkReply *reply);
    void onSwitchToRequest();
    void onSwitchToLogin();

private:
    void setupLoginUI();
    void setupRequestUI();
    void verifyCode(const QString &code, const QString &phone);
    void handleLoginResponse(QNetworkReply *reply);
    void sendAccessRequest(const QString &name, const QString &phone, int profiles, int durationMinutes);
    void handleRequestResponse(QNetworkReply *reply);
    void showError(const QString &message);
    void showSuccess(const QString &message);
    void showRequestError(const QString &message);
    void showRequestSuccess(const QString &message);

private:
    QStackedWidget *stackedWidget;
    QWidget *loginPage;
    QWidget *requestPage;

    // Login Page
    QLineEdit *phoneLoginInput;   // phone number for login verification
    QLineEdit *codeInput;
    QPushButton *btnLogin;
    QPushButton *btnSwitchToRequest;
    QLabel *statusLabel;

    // Request Page
    QLineEdit *nameInput;
    QLineEdit *phoneInput;
    QSpinBox *profilesSpinBox;
    QComboBox *durationCombo;
    QPushButton *btnSendRequest;
    QPushButton *btnSwitchToLogin;
    QLabel *requestStatusLabel;
    QLabel *profilesLabel;

    QNetworkAccessManager *networkManager;
    QString pendingRequestId;
};

#endif // LOGINWINDOW_H
