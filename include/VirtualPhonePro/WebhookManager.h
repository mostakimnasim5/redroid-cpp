#pragma once

#ifndef VIRTUALPHONEPRO_WEBHOOK_MANAGER_H
#define VIRTUALPHONEPRO_WEBHOOK_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

namespace VirtualPhonePro {

/**
 * @brief Webhook event types
 */
enum class WebhookEvent {
    InstanceStarted,
    InstanceStopped,
    InstanceError,
    InstanceCreated,
    InstanceDeleted,
    AdbConnected,
    AdbDisconnected,
    ProfileApplied,
    SafetyNetCheck,
    MemoryWarning,
    ResourceWarning
};

/**
 * @brief Webhook configuration
 */
struct WebhookConfig {
    QString id;
    QString name;
    QString url;
    QStringList events;  // List of event types to trigger
    QMap<QString, QString> headers;  // Custom headers
    int retryCount;
    int timeoutMs;
    bool enabled;
};

/**
 * @brief Webhook payload
 */
struct WebhookPayload {
    QString event;
    QString timestamp;
    QJsonObject data;
};

/**
 * @brief WebhookManager - Event-based automation triggers
 * 
 * Manages webhooks for instance lifecycle events,
 * enabling integration with external systems.
 */
class WebhookManager : public QObject {
    Q_OBJECT

public:
    static WebhookManager& instance();
    
    // =========================================================================
    // Webhook Management
    // =========================================================================
    
    /**
     * @brief Add webhook
     */
    bool addWebhook(const WebhookConfig& config);
    
    /**
     * @brief Remove webhook
     */
    bool removeWebhook(const QString& id);
    
    /**
     * @brief Update webhook
     */
    bool updateWebhook(const QString& id, const WebhookConfig& config);
    
    /**
     * @brief Get webhook by ID
     */
    WebhookConfig getWebhook(const QString& id) const;
    
    /**
     * @brief List all webhooks
     */
    QList<WebhookConfig> listWebhooks() const;
    
    /**
     * @brief Enable/disable webhook
     */
    bool setWebhookEnabled(const QString& id, bool enabled);
    
    // =========================================================================
    // Event Triggering
    // =========================================================================
    
    /**
     * @brief Trigger webhook for event
     */
    void triggerEvent(WebhookEvent event, const QJsonObject& data);
    
    /**
     * @brief Trigger all matching webhooks
     */
    void triggerWebhooks(WebhookEvent event, const QJsonObject& data);
    
    // =========================================================================
    // Testing
    // =========================================================================
    
    /**
     * @brief Test webhook delivery
     */
    void testWebhook(const QString& id);
    
signals:
    void webhookTriggered(const QString& id, WebhookEvent event);
    void webhookDelivered(const QString& id, bool success);
    void webhookError(const QString& id, const QString& error);

private:
    static WebhookManager* s_instance;
    explicit WebhookManager(QObject* parent = nullptr);
    
    WebhookPayload createPayload(WebhookEvent event, const QJsonObject& data);
    QString eventToString(WebhookEvent event);
    WebhookEvent stringToEvent(const QString& str);
    
    void sendWebhook(const WebhookConfig& config, const WebhookPayload& payload);
    void handleReply(const WebhookConfig& config, QNetworkReply* reply);
    
    QList<WebhookConfig> m_webhooks;
    QNetworkAccessManager* m_networkManager;
    QTimer* m_retryTimer;
    QMap<QString, int> m_retryCount;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_WEBHOOK_MANAGER_H
