/**
 * @file FirebaseHelper.cpp
 * @brief Firebase Firestore REST API Helper Implementation
 * @version 1.0.0
 */

#include "FirebaseHelper.hpp"
#include <QDebug>
#include <QJsonDocument>

namespace FirebaseHelper {

FirestoreClient::FirestoreClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

FirestoreClient::~FirestoreClient() {
}

void FirestoreClient::setAuthToken(const QString& token) {
    m_authToken = token;
}

QString FirestoreClient::getBaseUrl() const {
    return QString("https://firestore.googleapis.com/v1/projects/%1/databases/(default)/documents")
        .arg(VirtualPhonePro::ConfigManager::instance().getFirebaseProjectId());
}

QString FirestoreClient::getApiKey() const {
    return VirtualPhonePro::ConfigManager::instance().getFirebaseApiKey();
}

QString FirestoreClient::generateAccessCode() {
    // Generate 8-digit random code
    quint32 code = QRandomGenerator::global()->bounded(10000000, 99999999);
    return QString::number(code);
}

QString FirestoreClient::getUserFriendlyError(QNetworkReply::NetworkError error) {
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
            return "Connection refused. Please check your internet connection.";
        case QNetworkReply::TimeoutError:
            return "Request timed out. Please try again.";
        case QNetworkReply::HostNotFoundError:
            return "Server not found. Please try again later.";
        case QNetworkReply::NetworkSessionFailedError:
            return "Network error occurred. Please check your connection.";
        case QNetworkReply::ContentNotFoundError:
            return "Resource not found.";
        case QNetworkReply::AuthenticationRequiredError:
            return "Authentication failed. Please login again.";
        default:
            return "An error occurred. Please try again.";
    }
}

void FirestoreClient::makeRequest(const QString& endpoint, const QString& method,
                                  const QJsonObject& data) {
    QString urlStr = getBaseUrl() + endpoint;
    if (!getApiKey().isEmpty()) {
        urlStr += (endpoint.contains('?') ? "&" : "?") + QString("key=%1").arg(getApiKey());
    }

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }

    QNetworkReply* reply = nullptr;
    
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        QJsonDocument doc(data);
        reply = m_networkManager->post(request, doc.toJson());
    } else if (method == "PATCH") {
        QJsonDocument doc(data);
        QBuffer* buffer = new QBuffer();
        buffer->setData(doc.toJson());
        buffer->open(QIODevice::ReadOnly);
        reply = m_networkManager->sendCustomRequest(request, "PATCH", buffer);
        buffer->setParent(reply);
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    }

    if (reply) {
        connect(reply, &QNetworkReply::finished, this, &FirestoreClient::onReplyFinished);
    }
}

void FirestoreClient::onReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QNetworkReply::NetworkError error = reply->error();
    QByteArray response = reply->readAll();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        QString errorMsg = getUserFriendlyError(error);
        
        if (m_pendingRequestType == "fetch_requests") {
            emit accessRequestError(errorMsg);
        } else if (m_pendingRequestType == "fetch_users") {
            emit activeUserError(errorMsg);
        } else {
            emit networkError(errorMsg);
        }
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(response);
    
    // Handle different request types
    if (m_pendingRequestType == "fetch_requests") {
        QList<AccessRequest> requests;
        
        // Firestore documents.list returns {"documents":[...]} NOT an array
        QJsonObject rootObj = doc.object();
        if (rootObj.contains("documents")) {
            QJsonArray docs = rootObj["documents"].toArray();
            for (const QJsonValue& val : docs) {
                QJsonObject document = val.toObject();
                QJsonObject fields = document["fields"].toObject();
                
                AccessRequest req;
                req.documentName = document["name"].toString();
                req.id = document["name"].toString().split("/").last();
                req.userName = fields["userName"].toObject()["stringValue"].toString();
                req.contactNumber = fields["contactNumber"].toObject()["stringValue"].toString();
                // integerValue comes as string from Firestore REST API
                req.profileCount = fields["profileCount"].toObject()["integerValue"].toString().toInt();
                req.durationMinutes = fields["durationMinutes"].toObject()["integerValue"].toString().toInt();
                req.status = fields["status"].toObject()["stringValue"].toString();
                req.requestedAt = fields["requestedAt"].toObject()["timestampValue"].toString();
                
                requests.append(req);
            }
        }
        // Empty collection returns {} with no "documents" key - that's fine
        
        emit accessRequestsFetched(requests);
        
    } else if (m_pendingRequestType == "fetch_users") {
        QList<ActiveUser> users;
        
        // Firestore documents.list returns {"documents":[...]} NOT an array
        QJsonObject rootUsersObj = doc.object();
        if (rootUsersObj.contains("documents")) {
            QJsonArray docs = rootUsersObj["documents"].toArray();
            for (const QJsonValue& val : docs) {
                QJsonObject document = val.toObject();
                QJsonObject fields = document["fields"].toObject();
                
                ActiveUser user;
                user.documentName = document["name"].toString();
                user.id = document["name"].toString().split("/").last();
                user.uniqueKey = fields["uniqueKey"].toObject()["stringValue"].toString();
                user.userName = fields["userName"].toObject()["stringValue"].toString();
                user.phoneNumber = fields["contactNumber"].toObject()["stringValue"].toString();
                // integerValue comes as string from Firestore REST API
                user.totalProfiles = fields["totalProfiles"].toObject()["integerValue"].toString().toInt();
                user.remainingProfiles = fields["remainingProfiles"].toObject()["integerValue"].toString().toInt();
                user.isBlocked = fields["isBlocked"].toObject()["booleanValue"].toBool();
                user.createdAt = fields["createdAt"].toObject()["timestampValue"].toString();
                user.expiresAt = fields["expiresAt"].toObject()["timestampValue"].toString();
                
                users.append(user);
            }
        }
        
        emit activeUsersFetched(users);
        
    } else if (m_pendingRequestType == "approve_request") {
        emit accessRequestApproved(m_pendingDocumentId, m_pendingAccessCode);
        
    } else if (m_pendingRequestType == "reject_request") {
        emit accessRequestRejected(m_pendingDocumentId);
        
    } else if (m_pendingRequestType == "create_user") {
        QJsonObject obj = doc.object();
        if (obj.contains("name")) {
            QString docId = obj["name"].toString().split("/").last();
            emit activeUserCreated(docId, m_pendingAccessCode);
        }
        
    } else if (m_pendingRequestType == "update_user") {
        emit activeUserUpdated(m_pendingDocumentId);
        
    } else if (m_pendingRequestType == "delete_user") {
        emit activeUserDeleted(m_pendingDocumentId);
    }

    // Reset state
    m_pendingRequestType.clear();
    m_pendingDocumentId.clear();
    m_pendingAccessCode.clear();
}

// ========================================================================
// Access Requests Operations
// ========================================================================

void FirestoreClient::fetchAccessRequests() {
    m_pendingRequestType = "fetch_requests";
    makeRequest("/accessRequests", "GET");
}

void FirestoreClient::approveAccessRequest(const QString& documentId, const QString& adminNotes) {
    Q_UNUSED(adminNotes);
    
    m_pendingRequestType = "approve_request";
    m_pendingDocumentId = documentId;
    m_pendingAccessCode = generateAccessCode();
    
    // Update accessRequests document status to "approved"
    QJsonObject fields;
    fields["status"] = QJsonObject{{"stringValue", "approved"}};
    fields["processedAt"] = QJsonObject{{"timestampValue", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    
    QJsonObject data;
    data["fields"] = fields;
    
    makeRequest(
        QString("/accessRequests/%1?updateMask.fieldPaths=status&updateMask.fieldPaths=processedAt")
            .arg(documentId),
        "PATCH", data
    );
}

void FirestoreClient::rejectAccessRequest(const QString& documentId, const QString& reason) {
    m_pendingRequestType = "reject_request";
    m_pendingDocumentId = documentId;
    
    QJsonObject data;
    data["fields"] = QJsonObject{
        {"status", QJsonObject{{"stringValue", "rejected"}}},
        {"rejectionReason", QJsonObject{{"stringValue", reason}}},
        {"processedAt", QJsonObject{{"timestampValue", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}}}
    };
    
    makeRequest(QString("/accessRequests/%1?updateMask.fieldPaths=status&updateMask.fieldPaths=rejectionReason&updateMask.fieldPaths=processedAt").arg(documentId), "PATCH", data);
}

// ========================================================================
// Active Users Operations
// ========================================================================

void FirestoreClient::fetchActiveUsers() {
    m_pendingRequestType = "fetch_users";
    makeRequest("/activeUsers", "GET");
}

void FirestoreClient::createActiveUser(const QString& userName, const QString& phoneNumber,
                                       int totalProfiles, int durationDays) {
    m_pendingRequestType = "create_user";
    m_pendingAccessCode = generateAccessCode();
    
    // Calculate expiration date
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime expires = now.addDays(durationDays);
    
    QJsonObject fields;
    fields["uniqueKey"] = QJsonObject{{"stringValue", m_pendingAccessCode}};
    fields["userName"] = QJsonObject{{"stringValue", userName}};
    fields["phoneNumber"] = QJsonObject{{"stringValue", phoneNumber}};
    fields["totalProfiles"] = QJsonObject{{"integerValue", QString::number(totalProfiles)}};
    fields["remainingProfiles"] = QJsonObject{{"integerValue", QString::number(totalProfiles)}};
    fields["isBlocked"] = QJsonObject{{"booleanValue", false}};
    fields["createdAt"] = QJsonObject{{"timestampValue", now.toString(Qt::ISODate)}};
    fields["expiresAt"] = QJsonObject{{"timestampValue", expires.toString(Qt::ISODate)}};
    fields["status"] = QJsonObject{{"stringValue", "active"}};
    
    QJsonObject data;
    data["fields"] = fields;
    
    makeRequest("/activeUsers", "POST", data);
}

void FirestoreClient::updateUserStatus(const QString& documentId, bool isBlocked) {
    m_pendingRequestType = "update_user";
    m_pendingDocumentId = documentId;
    
    QJsonObject fields;
    fields["isBlocked"] = QJsonObject{{"booleanValue", isBlocked}};
    fields["updatedAt"] = QJsonObject{{"timestampValue", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    fields["status"] = QJsonObject{{"stringValue", isBlocked ? "blocked" : "active"}};
    
    QJsonObject data;
    data["fields"] = fields;
    
    makeRequest(QString("/activeUsers/%1?updateMask.fieldPaths=isBlocked&updateMask.fieldPaths=updatedAt&updateMask.fieldPaths=status").arg(documentId), "PATCH", data);
}

void FirestoreClient::deleteUser(const QString& documentId) {
    m_pendingRequestType = "delete_user";
    m_pendingDocumentId = documentId;
    makeRequest(QString("/activeUsers/%1").arg(documentId), "DELETE");
}

void FirestoreClient::extendUserDuration(const QString& documentId, int additionalDays) {
    m_pendingRequestType = "update_user";
    m_pendingDocumentId = documentId;
    
    // Calculate new expiration
    QDateTime newExpiry = QDateTime::currentDateTimeUtc().addDays(additionalDays);
    
    QJsonObject fields;
    fields["expiresAt"] = QJsonObject{{"timestampValue", newExpiry.toString(Qt::ISODate)}};
    fields["updatedAt"] = QJsonObject{{"timestampValue", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    fields["lastExtended"] = QJsonObject{{"timestampValue", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    
    QJsonObject data;
    data["fields"] = fields;
    
    makeRequest(QString("/activeUsers/%1?updateMask.fieldPaths=expiresAt&updateMask.fieldPaths=updatedAt&updateMask.fieldPaths=lastExtended").arg(documentId), "PATCH", data);
}

} // namespace FirebaseHelper
