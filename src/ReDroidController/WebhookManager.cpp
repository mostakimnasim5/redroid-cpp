/**
 * @file WebhookManager.cpp
 * @brief Webhook Manager Implementation
 * @version 2.0.0
 * 
 * Handles webhook notifications for system events.
 */

#include "VirtualPhonePro/WebhookManager.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QJsonArray>
#include <QStandardPaths>

namespace VirtualPhonePro {

WebhookManager* WebhookManager::s_instance = nullptr;

WebhookManager& WebhookManager::instance() {
    if (!s_instance) {
        s_instance = new WebhookManager();
    }
    return *s_instance;
}

WebhookManager::WebhookManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

WebhookManager::~WebhookManager() {
    // Clean up pending requests
    for (QNetworkReply* reply : m_pendingReplies) {
        reply->abort();
        reply->deleteLater();
    }
}

bool WebhookManager::addWebhook(const WebhookConfig& webhook) {
    if (webhook.url.isEmpty()) {
        qWarning() << "[WebhookManager] Cannot add webhook with empty URL";
        return false;
    }
    
    if (webhook.id.isEmpty()) {
        qWarning() << "[WebhookManager] Cannot add webhook with empty ID";
        return false;
    }
    
    m_webhooks[webhook.id] = webhook;
    qDebug() << "[WebhookManager] Webhook added:" << webhook.id;
    
    saveWebhooks();
    return true;
}

bool WebhookManager::removeWebhook(const QString& webhookId) {
    if (m_webhooks.contains(webhookId)) {
        m_webhooks.remove(webhookId);
        qDebug() << "[WebhookManager] Webhook removed:" << webhookId;
        saveWebhooks();
        return true;
    }
    return false;
}

bool WebhookManager::updateWebhook(const QString& webhookId, const WebhookConfig& webhook) {
    if (!m_webhooks.contains(webhookId)) {
        return false;
    }
    
    m_webhooks[webhookId] = webhook;
    saveWebhooks();
    return true;
}

QList<WebhookConfig> WebhookManager::getWebhooks() const {
    return m_webhooks.values();
}

QList<WebhookConfig> WebhookManager::getWebhooksForEvent(const QString& event) const {
    QList<WebhookConfig> result;
    
    for (const WebhookConfig& webhook : m_webhooks) {
        if (webhook.events.contains(event)) {
            result.append(webhook);
        }
    }
    
    return result;
}

void WebhookManager::triggerEvent(const QString& event, const QJsonObject& data) {
    QList<WebhookConfig> webhooks = getWebhooksForEvent(event);
    
    for (const WebhookConfig& webhook : webhooks) {
        sendWebhook(webhook, event, data);
    }
}

void WebhookManager::sendWebhook(const WebhookConfig& webhook, const QString& event, 
                                   const QJsonObject& data) {
    if (webhook.url.isEmpty()) {
        return;
    }
    
    // Build payload
    QJsonObject payload;
    payload["event"] = event;
    payload["webhookId"] = webhook.id;
    payload["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["data"] = data;
    
    // Create request
    QNetworkRequest request(QUrl(webhook.url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "ReDroidCPP-Webhook/1.0");
    
    // Add headers
    for (auto it = webhook.headers.begin(); it != webhook.headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // Send request
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    
    // Track pending reply
    m_pendingReplies.append(reply);
    
    // Handle timeout
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(webhook.timeout);
    
    connect(timeoutTimer, &QTimer::timeout, this, [this, reply, webhook, event]() {
        qWarning() << "[WebhookManager] Webhook timeout:" << webhook.id << "for event" << event;
        reply->abort();
    });
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, webhook, event, timeoutTimer]() {
        timeoutTimer->deleteLater();
        m_pendingReplies.removeAll(reply);
        
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "[WebhookManager] Webhook sent successfully:" << webhook.id << "for event" << event;
            emit webhookSent(webhook.id, event, true, QString());
        } else {
            QString error = reply->errorString();
            qWarning() << "[WebhookManager] Webhook failed:" << webhook.id << "-" << error;
            emit webhookSent(webhook.id, event, false, error);
            
            // Retry if configured
            if (webhook.retryCount > 0) {
                scheduleRetry(webhook, event, {}, webhook.retryCount);
            }
        }
        
        reply->deleteLater();
    });
}

void WebhookManager::scheduleRetry(const WebhookConfig& webhook, const QString& event, 
                                     const QJsonObject& data, int remainingRetries) {
    if (remainingRetries <= 0) {
        qWarning() << "[WebhookManager] Max retries reached for webhook:" << webhook.id;
        return;
    }
    
    // Schedule retry after delay
    int retryDelay = webhook.retryDelayMs * (webhook.retryCount - remainingRetries + 1);
    
    QTimer* retryTimer = new QTimer(this);
    retryTimer->setSingleShot(true);
    retryTimer->setInterval(retryDelay);
    
    connect(retryTimer, &QTimer::timeout, this, [this, webhook, event, data, remainingRetries]() {
        qDebug() << "[WebhookManager] Retrying webhook:" << webhook.id 
                 << "(retries remaining:" << remainingRetries - 1 << ")";
        sendWebhook(webhook, event, data);
    });
    
    retryTimer->start();
}

void WebhookManager::loadWebhooks() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                         + "/webhooks.json";
    QFile file(configPath);
    
    if (!file.exists()) {
        qDebug() << "[WebhookManager] No webhook config found, starting fresh";
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[WebhookManager] Cannot open webhook config:" << file.errorString();
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qWarning() << "[WebhookManager] Invalid webhook config format";
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray webhooksArray = root["webhooks"].toArray();
    
    for (const QJsonValue& val : webhooksArray) {
        QJsonObject obj = val.toObject();
        WebhookConfig webhook;
        webhook.id     = obj["id"].toString();
        webhook.url    = obj["url"].toString();
        webhook.secret = obj["secret"].toString();
        webhook.enabled = obj["enabled"].toBool(true);
        
        QJsonArray eventsArray = obj["events"].toArray();
        for (const QJsonValue& ev : eventsArray) {
            webhook.events.append(ev.toString());
        }
        
        if (!webhook.id.isEmpty() && !webhook.url.isEmpty()) {
            m_webhooks[webhook.id] = webhook;
        }
    }
    
    qDebug() << "[WebhookManager] Loaded" << m_webhooks.size() << "webhooks";
}

void WebhookManager::saveWebhooks() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    QString configPath = configDir + "/webhooks.json";
    
    QJsonArray webhooksArray;
    for (const WebhookConfig& webhook : m_webhooks) {
        QJsonObject obj;
        obj["id"]      = webhook.id;
        obj["url"]     = webhook.url;
        obj["secret"]  = webhook.secret;
        obj["enabled"] = webhook.enabled;
        
        QJsonArray eventsArray;
        for (const QString& event : webhook.events) {
            eventsArray.append(event);
        }
        obj["events"] = eventsArray;
        webhooksArray.append(obj);
    }
    
    QJsonObject root;
    root["webhooks"] = webhooksArray;
    root["savedAt"]  = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[WebhookManager] Cannot save webhooks:" << file.errorString();
        return;
    }
    
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    qDebug() << "[WebhookManager] Saved" << m_webhooks.size() << "webhooks to" << configPath;
}

void WebhookManager::testWebhook(const QString& webhookId) {
    if (!m_webhooks.contains(webhookId)) {
        emit webhookTestResult(webhookId, false, "Webhook not found");
        return;
    }
    
    const WebhookConfig& webhook = m_webhooks[webhookId];
    
    // Create test payload
    QJsonObject testData;
    testData["type"] = "test";
    testData["message"] = "This is a test webhook from ReDroidCPP";
    testData["webhookId"] = webhookId;
    
    // Send test webhook
    sendWebhook(webhook, "test", testData);
}

QJsonArray WebhookManager::getWebhookHistory(const QString& webhookId) {
    return m_webhookHistory.value(webhookId);
}

void WebhookManager::clearHistory() {
    m_webhookHistory.clear();
}

// Event-specific trigger methods

void WebhookManager::onInstanceStarted(const QString& instanceId) {
    QJsonObject data;
    data["instanceId"] = instanceId;
    data["event"] = "instance_started";
    triggerEvent("InstanceStarted", data);
}

void WebhookManager::onInstanceStopped(const QString& instanceId) {
    QJsonObject data;
    data["instanceId"] = instanceId;
    data["event"] = "instance_stopped";
    triggerEvent("InstanceStopped", data);
}

void WebhookManager::onInstanceError(const QString& instanceId, const QString& error) {
    QJsonObject data;
    data["instanceId"] = instanceId;
    data["error"] = error;
    data["event"] = "instance_error";
    triggerEvent("InstanceError", data);
}

void WebhookManager::onMemoryWarning(const QString& instanceId, quint64 memoryUsage, 
                                      quint64 memoryLimit) {
    QJsonObject data;
    data["instanceId"] = instanceId;
    data["memoryUsage"] = static_cast<qint64>(memoryUsage);
    data["memoryLimit"] = static_cast<qint64>(memoryLimit);
    data["event"] = "memory_warning";
    triggerEvent("MemoryWarning", data);
}

void WebhookManager::onSafetyNetCheck(const QString& instanceId, bool passed) {
    QJsonObject data;
    data["instanceId"] = instanceId;
    data["passed"] = passed;
    data["event"] = "safety_net_check";
    triggerEvent("SafetyNetCheck", data);
}

} // namespace VirtualPhonePro
