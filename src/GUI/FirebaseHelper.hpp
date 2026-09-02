/**
 * @file FirebaseHelper.hpp
 * @brief Firebase Firestore REST API Helper
 * @version 1.0.0
 */

#ifndef FIREBASEHELPER_HPP
#define FIREBASEHELPER_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>

#include "VirtualPhonePro/ConfigManager.hpp"

namespace FirebaseHelper {

struct AccessRequest {
    QString id;
    QString userName;
    QString contactNumber;
    int profileCount;
    int durationMinutes;
    QString status;
    QString requestedAt;
    QString documentName;
};

struct ActiveUser {
    QString id;
    QString uniqueKey;
    QString userName;
    QString phoneNumber;
    int totalProfiles;
    int remainingProfiles;
    bool isBlocked;
    QString createdAt;
    QString expiresAt;
    QString documentName;
};

class FirestoreClient : public QObject {
    Q_OBJECT

public:
    explicit FirestoreClient(QObject* parent = nullptr);
    ~FirestoreClient();

    // Configuration
    void setAuthToken(const QString& token);
    QString getAuthToken() const { return m_authToken; }
    bool isAuthenticated() const { return !m_authToken.isEmpty(); }

    // Access Requests Operations
    void fetchAccessRequests();
    void approveAccessRequest(const QString& documentId, const QString& adminNotes,
                             const QString& accessCode);
    void rejectAccessRequest(const QString& documentId, const QString& reason);

    // Active Users Operations
    void fetchActiveUsers();
    void createActiveUser(const QString& userName, const QString& phoneNumber,
                         int totalProfiles, int durationDays, const QString& accessCode);
    void updateUserStatus(const QString& documentId, bool isBlocked);
    void deleteUser(const QString& documentId);
    void extendUserDuration(const QString& documentId, int additionalDays);

    // Generate 8-digit access code
    static QString generateAccessCode();

signals:
    // Access Requests signals
    void accessRequestsFetched(const QList<AccessRequest>& requests);
    void accessRequestApproved(const QString& documentId, const QString& accessCode);
    void accessRequestRejected(const QString& documentId);
    void accessRequestError(const QString& error);

    // Active Users signals
    void activeUsersFetched(const QList<ActiveUser>& users);
    void activeUserCreated(const QString& documentId, const QString& accessCode);
    void activeUserUpdated(const QString& documentId);
    void activeUserDeleted(const QString& documentId);
    void activeUserError(const QString& error);

    // Generic error
    void networkError(const QString& errorMessage);

private slots:
    void onReplyFinished();

private:
    QString getBaseUrl() const;
    QString getApiKey() const;
    void makeRequest(const QString& endpoint, const QString& method,
                    const QJsonObject& data = QJsonObject());
    QString getUserFriendlyError(QNetworkReply::NetworkError error);

    QNetworkAccessManager* m_networkManager;
    QString m_authToken;
    QString m_pendingRequestType;
    QString m_pendingDocumentId;
    QString m_pendingAccessCode;
};

} // namespace FirebaseHelper

#endif // FIREBASEHELPER_HPP
