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
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

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
    void verifyCode(const QString &code);
    void handleLoginResponse(QNetworkReply *reply);
    void sendAccessRequest(const QString &name, const QString &phone, int profiles, int durationMinutes);
    void handleRequestResponse(QNetworkReply *reply);
    void showError(const QString &message);
    void showSuccess(const QString &message);

private:
    QStackedWidget *stackedWidget;
    QWidget *loginPage;
    QWidget *requestPage;

    // Login Page
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

    QNetworkAccessManager *networkManager;
    QString pendingRequestId;
};

#endif // LOGINWINDOW_H
