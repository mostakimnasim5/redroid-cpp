#pragma once

#ifndef VIRTUALPHONEPRO_API_SERVER_H
#define VIRTUALPHONEPRO_API_SERVER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkProxy>

namespace VirtualPhonePro {

/**
 * @brief APIServer - REST API for remote management
 * 
 * Provides HTTP REST API for managing instances remotely,
 * supporting CRUD operations, batch commands, and monitoring.
 */
class APIServer : public QObject {
    Q_OBJECT

public:
    static APIServer& instance();
    
    // =========================================================================
    // Server Lifecycle
    // =========================================================================
    
    /**
     * @brief Start the API server
     * @param port Port to listen on (default: 8080)
     * @return true if started successfully
     */
    bool start(quint16 port = 8080);
    
    /**
     * @brief Stop the API server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const;
    
    /**
     * @brief Get server port
     */
    quint16 port() const;
    
    // =========================================================================
    // Authentication
    // =========================================================================
    
    /**
     * @brief Set API key for authentication
     */
    void setApiKey(const QString& key);
    
    /**
     * @brief Enable/disable authentication
     */
    void setAuthEnabled(bool enabled);
    
signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void requestReceived(const QString& method, const QString& path);
    void error(const QString& message);

private:
    explicit APIServer(QObject* parent = nullptr);
    
    void handleRequest(QTcpSocket* socket);
    QJsonObject parseRequest(const QString& request);
    QJsonObject handleEndpoint(const QString& method, const QString& path, 
                               const QJsonObject& body);
    
    // Instance endpoints
    QJsonObject listInstances();
    QJsonObject createInstance(const QJsonObject& body);
    QJsonObject getInstance(const QString& id);
    QJsonObject startInstance(const QString& id);
    QJsonObject stopInstance(const QString& id);
    QJsonObject deleteInstance(const QString& id);
    QJsonObject restartInstance(const QString& id);
    QJsonObject getInstanceStats(const QString& id);
    
    // Profile endpoints
    QJsonObject listProfiles();
    QJsonObject createProfile(const QJsonObject& body);
    QJsonObject getProfile(const QString& id);
    QJsonObject updateProfile(const QString& id, const QJsonObject& body);
    QJsonObject deleteProfile(const QString& id);
    
    // Sensor endpoints
    QJsonObject setGPS(const QString& id, const QJsonObject& body);
    QJsonObject setAccelerometer(const QString& id, const QJsonObject& body);
    
    // Batch endpoints
    QJsonObject batchStart(const QJsonObject& body);
    QJsonObject batchStop(const QJsonObject& body);
    
    // Helper
    QJsonObject errorResponse(const QString& message, int code = 400);
    QJsonObject successResponse(const QVariant& data = QVariant());
    bool validateApiKey(const QString& key);
    
    quint16 m_port;
    bool m_running;
    bool m_authEnabled;
    QString m_apiKey;
    QTcpServer* m_server;
    QMap<QString, QString> m_endpoints;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_API_SERVER_H
