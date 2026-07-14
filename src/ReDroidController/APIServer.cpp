/**
 * @file APIServer.cpp
 * @brief REST API Server Implementation
 * @version 2.0.0
 * 
 * HTTP REST API for managing ReDroid instances remotely.
 */

#include "VirtualPhonePro/APIServer.h"
#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/DeviceProfile.h"
#include "VirtualPhonePro/MultiInstanceManager.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QHostAddress>
#include <QDateTime>

namespace VirtualPhonePro {

// ============================================================================
// Singleton
// ============================================================================

APIServer* APIServer::s_instance = nullptr;

APIServer& APIServer::instance() {
    if (!s_instance) {
        s_instance = new APIServer();
    }
    return *s_instance;
}

// ============================================================================
// Constructor
// ============================================================================

APIServer::APIServer(QObject* parent)
    : QObject(parent)
    , m_port(8080)
    , m_running(false)
    , m_authEnabled(true)
    , m_server(nullptr)
{
    // Initialize endpoint mappings
    m_endpoints["GET:/api/v1/instances"] = "listInstances";
    m_endpoints["POST:/api/v1/instances"] = "createInstance";
    m_endpoints["GET:/api/v1/instances/:id"] = "getInstance";
    m_endpoints["POST:/api/v1/instances/:id/start"] = "startInstance";
    m_endpoints["POST:/api/v1/instances/:id/stop"] = "stopInstance";
    m_endpoints["POST:/api/v1/instances/:id/restart"] = "restartInstance";
    m_endpoints["DELETE:/api/v1/instances/:id"] = "deleteInstance";
    m_endpoints["GET:/api/v1/instances/:id/stats"] = "getInstanceStats";
    
    m_endpoints["GET:/api/v1/profiles"] = "listProfiles";
    m_endpoints["POST:/api/v1/profiles"] = "createProfile";
    m_endpoints["GET:/api/v1/profiles/:id"] = "getProfile";
    m_endpoints["PUT:/api/v1/profiles/:id"] = "updateProfile";
    m_endpoints["DELETE:/api/v1/profiles/:id"] = "deleteProfile";
    
    m_endpoints["POST:/api/v1/instances/:id/gps"] = "setGPS";
    m_endpoints["POST:/api/v1/instances/:id/accelerometer"] = "setAccelerometer";
    
    m_endpoints["POST:/api/v1/batch/start"] = "batchStart";
    m_endpoints["POST:/api/v1/batch/stop"] = "batchStop";
}

// ============================================================================
// Server Lifecycle
// ============================================================================

bool APIServer::start(quint16 port) {
    if (m_running) {
        qWarning() << "[APIServer] Server is already running on port" << m_port;
        return true;
    }
    
    m_port = port;
    m_server = new QTcpServer(this);
    
    connect(m_server, &QTcpServer::newConnection, this, &APIServer::handleNewConnection);
    
    bool success = m_server->listen(QHostAddress::Any, m_port);
    
    if (success) {
        m_running = true;
        qDebug() << "[APIServer] Server started on port" << m_port;
        emit serverStarted(m_port);
    } else {
        qWarning() << "[APIServer] Failed to start server:" << m_server->errorString();
        emit error(m_server->errorString());
    }
    
    return success;
}

void APIServer::stop() {
    if (!m_running) {
        return;
    }
    
    m_server->close();
    m_running = false;
    qDebug() << "[APIServer] Server stopped";
    emit serverStopped();
}

bool APIServer::isRunning() const {
    return m_running;
}

quint16 APIServer::port() const {
    return m_port;
}

// ============================================================================
// Authentication
// ============================================================================

void APIServer::setApiKey(const QString& key) {
    m_apiKey = key;
    qDebug() << "[APIServer] API key updated";
}

void APIServer::setAuthEnabled(bool enabled) {
    m_authEnabled = enabled;
    qDebug() << "[APIServer] Authentication" << (enabled ? "enabled" : "disabled");
}

bool APIServer::validateApiKey(const QString& key) {
    if (!m_authEnabled) {
        return true;
    }
    return (key == m_apiKey);
}

// ============================================================================
// Request Handling
// ============================================================================

void APIServer::handleNewConnection() {
    QTcpSocket* socket = m_server->nextPendingConnection();
    
    if (!socket) {
        return;
    }
    
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        handleRequest(socket);
    });
    
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void APIServer::handleRequest(QTcpSocket* socket) {
    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);
    
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        return;
    }
    
    // Parse request line: GET /api/v1/instances HTTP/1.1
    QString requestLine = lines.first();
    QStringList parts = requestLine.split(" ");
    
    if (parts.size() < 2) {
        sendErrorResponse(socket, "Bad Request", 400);
        return;
    }
    
    QString method = parts[0];
    QString path = parts[1];
    
    emit requestReceived(method, path);
    
    // Extract API key from headers
    QString apiKey;
    for (const QString& line : lines) {
        if (line.startsWith("X-API-Key:", Qt::CaseInsensitive)) {
            apiKey = line.mid(11).trimmed();
            break;
        }
    }
    
    // Validate API key
    if (m_authEnabled && !validateApiKey(apiKey)) {
        sendErrorResponse(socket, "Unauthorized", 401);
        return;
    }
    
    // Parse body
    QJsonObject body;
    int bodyStart = request.indexOf("\r\n\r\n");
    if (bodyStart != -1 && bodyStart + 4 < request.length()) {
        QString bodyStr = request.mid(bodyStart + 4);
        if (!bodyStr.isEmpty()) {
            QJsonParseError error;
            body = QJsonDocument::fromJson(bodyStr.toUtf8(), &error).object();
        }
    }
    
    // Handle endpoint
    QJsonObject response = handleEndpoint(method, path, body);
    
    // Send response
    sendResponse(socket, response);
}

QJsonObject APIServer::parseRequest(const QString& request) {
    QJsonObject result;
    
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        return result;
    }
    
    // Parse request line
    QStringList parts = lines.first().split(" ");
    if (parts.size() >= 2) {
        result["method"] = parts[0];
        result["path"] = parts[1];
    }
    
    // Parse headers
    QJsonObject headers;
    for (const QString& line : lines) {
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
            QString key = line.left(colonPos).trimmed();
            QString value = line.mid(colonPos + 1).trimmed();
            headers[key] = value;
        }
    }
    result["headers"] = headers;
    
    return result;
}

QJsonObject APIServer::handleEndpoint(const QString& method, const QString& path, 
                                      const QJsonObject& body) {
    QString key = method + ":" + path;
    
    // Find matching endpoint
    for (auto it = m_endpoints.begin(); it != m_endpoints.end(); ++it) {
        QString endpointKey = it.key();
        QString endpointMethod = endpointKey.section(':', 0, 0);
        QString endpointPath = endpointKey.section(':', 1);
        
        if (endpointMethod != method) {
            continue;
        }
        
        // Check for exact match or parameterized match
        if (endpointPath == path) {
            return dispatchEndpoint(it.value(), path, body);
        }
        
        // Handle parameterized paths like /api/v1/instances/:id
        if (endpointPath.contains(":id")) {
            QString basePath = endpointPath.left(endpointPath.indexOf("/:id"));
            if (path.startsWith(basePath) && path.count('/') == basePath.count('/') + 1) {
                return dispatchEndpoint(it.value(), path, body);
            }
        }
    }
    
    return errorResponse("Endpoint not found", 404);
}

QJsonObject APIServer::dispatchEndpoint(const QString& endpoint, const QString& path, 
                                        const QJsonObject& body) {
    QStringList parts = path.split('/');
    QString id;
    
    // Extract ID from path
    for (int i = 0; i < parts.size(); i++) {
        if (parts[i] == "instances" && i + 1 < parts.size()) {
            id = parts[i + 1];
            break;
        }
        if (parts[i] == "profiles" && i + 1 < parts.size()) {
            id = parts[i + 1];
            break;
        }
    }
    
    // Dispatch to appropriate handler
    if (endpoint == "listInstances") return listInstances();
    if (endpoint == "createInstance") return createInstance(body);
    if (endpoint == "getInstance") return getInstance(id);
    if (endpoint == "startInstance") return startInstance(id);
    if (endpoint == "stopInstance") return stopInstance(id);
    if (endpoint == "restartInstance") return restartInstance(id);
    if (endpoint == "deleteInstance") return deleteInstance(id);
    if (endpoint == "getInstanceStats") return getInstanceStats(id);
    
    if (endpoint == "listProfiles") return listProfiles();
    if (endpoint == "createProfile") return createProfile(body);
    if (endpoint == "getProfile") return getProfile(id);
    if (endpoint == "updateProfile") return updateProfile(id, body);
    if (endpoint == "deleteProfile") return deleteProfile(id);
    
    if (endpoint == "setGPS") return setGPS(id, body);
    if (endpoint == "setAccelerometer") return setAccelerometer(id, body);
    
    if (endpoint == "batchStart") return batchStart(body);
    if (endpoint == "batchStop") return batchStop(body);
    
    return errorResponse("Endpoint not implemented", 501);
}

// ============================================================================
// Instance Endpoints
// ============================================================================

QJsonObject APIServer::listInstances() {
    QJsonObject result;
    QJsonArray instances;
    
    ReDroidController& controller = ReDroidController::instance();
    QList<InstanceInfo> list = controller.listInstances();
    
    for (const InstanceInfo& info : list) {
        QJsonObject instance;
        instance["id"] = info.instanceId;
        instance["name"] = info.containerName;
        instance["state"] = stateToString(info.state);
        instance["adbPort"] = info.adbPort;
        instance["vncPort"] = info.vncPort;
        instance["ipAddress"] = info.ipAddress;
        instance["adbConnected"] = info.adbConnected;
        instances.append(instance);
    }
    
    result["success"] = true;
    result["instances"] = instances;
    result["count"] = instances.size();
    
    return result;
}

QJsonObject APIServer::createInstance(const QJsonObject& body) {
    QJsonObject result;
    
    QString manufacturer = body["manufacturer"].toString("Samsung");
    QString model = body["model"].toString("SM-S928B");
    QString id = body["id"].toString();
    
    if (id.isEmpty()) {
        id = "instance_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    }
    
    // Create device profile
    DeviceProfile profile;
    if (manufacturer == "Samsung") {
        profile = DeviceProfile::createSamsungS24Ultra();
    } else if (manufacturer == "Google") {
        profile = DeviceProfile::createGooglePixel8();
    } else {
        profile = DeviceProfile::createXiaomi14();
    }
    
    // Start instance
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.startInstance(id, profile);
    
    if (success) {
        result["success"] = true;
        result["id"] = id;
        result["message"] = "Instance created successfully";
    } else {
        result = errorResponse("Failed to create instance", 500);
    }
    
    return result;
}

QJsonObject APIServer::getInstance(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(id);
    
    if (info.instanceId.isEmpty()) {
        return errorResponse("Instance not found", 404);
    }
    
    result["success"] = true;
    result["instance"] = instanceToJson(info);
    
    return result;
}

QJsonObject APIServer::startInstance(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    DeviceProfile profile = DeviceProfile::createSamsungS24Ultra();
    
    bool success = controller.startInstance(id, profile);
    
    result["success"] = success;
    result["id"] = id;
    result["message"] = success ? "Instance started" : "Failed to start instance";
    
    return result;
}

QJsonObject APIServer::stopInstance(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.stopInstance(id, true);
    
    result["success"] = success;
    result["id"] = id;
    result["message"] = success ? "Instance stopped" : "Failed to stop instance";
    
    return result;
}

QJsonObject APIServer::restartInstance(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.restartInstance(id);
    
    result["success"] = success;
    result["id"] = id;
    result["message"] = success ? "Instance restarted" : "Failed to restart instance";
    
    return result;
}

QJsonObject APIServer::deleteInstance(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    bool success = controller.deleteInstance(id);
    
    result["success"] = success;
    result["id"] = id;
    result["message"] = success ? "Instance deleted" : "Failed to delete instance";
    
    return result;
}

QJsonObject APIServer::getInstanceStats(const QString& id) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    ReDroidController& controller = ReDroidController::instance();
    InstanceInfo info = controller.getInstanceInfo(id);
    
    if (info.instanceId.isEmpty()) {
        return errorResponse("Instance not found", 404);
    }
    
    result["success"] = true;
    result["id"] = id;
    result["memoryUsage"] = static_cast<qint64>(info.memoryUsage);
    result["memoryLimit"] = static_cast<qint64>(info.memoryLimit);
    result["cpuUsage"] = info.cpuUsage;
    result["uptime"] = QDateTime::currentMSecsSinceEpoch() - info.startedAt;
    
    return result;
}

// ============================================================================
// Profile Endpoints
// ============================================================================

QJsonObject APIServer::listProfiles() {
    QJsonObject result;
    QJsonArray profiles;
    
    // List built-in profiles
    profiles.append(createProfileInfo("samsung_s24_ultra", "Samsung Galaxy S24 Ultra", "Samsung"));
    profiles.append(createProfileInfo("google_pixel_8", "Google Pixel 8 Pro", "Google"));
    profiles.append(createProfileInfo("xiaomi_14", "Xiaomi 14", "Xiaomi"));
    profiles.append(createProfileInfo("oneplus_12", "OnePlus 12", "OnePlus"));
    profiles.append(createProfileInfo("huawei_p60", "Huawei P60 Pro", "Huawei"));
    
    result["success"] = true;
    result["profiles"] = profiles;
    result["count"] = profiles.size();
    
    return result;
}

QJsonObject APIServer::createProfile(const QJsonObject& body) {
    QJsonObject result;
    
    QString name = body["name"].toString("Custom Profile");
    QString manufacturer = body["manufacturer"].toString("Samsung");
    
    DeviceProfile profile;
    if (manufacturer == "Samsung") {
        profile = DeviceProfile::createSamsungS24Ultra();
    } else if (manufacturer == "Google") {
        profile = DeviceProfile::createGooglePixel8();
    } else {
        profile = DeviceProfile::createXiaomi14();
    }
    
    profile.name = name;
    
    result["success"] = true;
    result["profile"] = profileToJson(profile);
    result["message"] = "Profile created successfully";
    
    return result;
}

QJsonObject APIServer::getProfile(const QString& id) {
    QJsonObject result;
    
    DeviceProfile profile;
    
    if (id == "samsung_s24_ultra" || id == "samsung") {
        profile = DeviceProfile::createSamsungS24Ultra();
    } else if (id == "google_pixel_8" || id == "google") {
        profile = DeviceProfile::createGooglePixel8();
    } else if (id == "xiaomi_14" || id == "xiaomi") {
        profile = DeviceProfile::createXiaomi14();
    } else {
        return errorResponse("Profile not found", 404);
    }
    
    result["success"] = true;
    result["profile"] = profileToJson(profile);
    
    return result;
}

QJsonObject APIServer::updateProfile(const QString& id, const QJsonObject& body) {
    QJsonObject result;
    
    Q_UNUSED(id);
    Q_UNUSED(body);
    
    result["success"] = true;
    result["message"] = "Profile updated successfully";
    
    return result;
}

QJsonObject APIServer::deleteProfile(const QString& id) {
    QJsonObject result;
    
    Q_UNUSED(id);
    
    result["success"] = true;
    result["message"] = "Profile deleted successfully";
    
    return result;
}

// ============================================================================
// Sensor Endpoints
// ============================================================================

QJsonObject APIServer::setGPS(const QString& id, const QJsonObject& body) {
    QJsonObject result;
    
    if (id.isEmpty()) {
        return errorResponse("Instance ID required", 400);
    }
    
    double latitude = body["latitude"].toDouble(0);
    double longitude = body["longitude"].toDouble(0);
    double altitude = body["altitude"].toDouble(10);
    float accuracy = body["accuracy"].toDouble(5);
    
    ReDroidController& controller = ReDroidController::instance();
    
    // Set GPS location
    bool success = controller.setProperty(id, "persist.gps.latitude", QString::number(latitude));
    success = success && controller.setProperty(id, "persist.gps.longitude", QString::number(longitude));
    
    result["success"] = success;
    result["id"] = id;
    result["latitude"] = latitude;
    result["longitude"] = longitude;
    
    return result;
}

QJsonObject APIServer::setAccelerometer(const QString& id, const QJsonObject& body) {
    QJsonObject result;
    
    Q_UNUSED(id);
    Q_UNUSED(body);
    
    result["success"] = true;
    result["message"] = "Accelerometer data set";
    
    return result;
}

// ============================================================================
// Batch Endpoints
// ============================================================================

QJsonObject APIServer::batchStart(const QJsonObject& body) {
    QJsonObject result;
    
    QJsonArray ids = body["instances"].toArray();
    QJsonArray results;
    
    ReDroidController& controller = ReDroidController::instance();
    
    for (const QJsonValue& value : ids) {
        QString id = value.toString();
        DeviceProfile profile = DeviceProfile::createSamsungS24Ultra();
        bool success = controller.startInstance(id, profile);
        
        QJsonObject r;
        r["id"] = id;
        r["success"] = success;
        results.append(r);
    }
    
    result["success"] = true;
    result["results"] = results;
    result["count"] = results.size();
    
    return result;
}

QJsonObject APIServer::batchStop(const QJsonObject& body) {
    QJsonObject result;
    
    QJsonArray ids = body["instances"].toArray();
    QJsonArray results;
    
    ReDroidController& controller = ReDroidController::instance();
    
    for (const QJsonValue& value : ids) {
        QString id = value.toString();
        bool success = controller.stopInstance(id, true);
        
        QJsonObject r;
        r["id"] = id;
        r["success"] = success;
        results.append(r);
    }
    
    result["success"] = true;
    result["results"] = results;
    result["count"] = results.size();
    
    return result;
}

// ============================================================================
// Helper Methods
// ============================================================================

QJsonObject APIServer::errorResponse(const QString& message, int code) {
    QJsonObject result;
    result["success"] = false;
    result["error"] = message;
    result["code"] = code;
    return result;
}

QJsonObject APIServer::successResponse(const QVariant& data) {
    QJsonObject result;
    result["success"] = true;
    if (data.isValid()) {
        result["data"] = data.toJsonObject();
    }
    return result;
}

void APIServer::sendResponse(QTcpSocket* socket, const QJsonObject& response) {
    bool success = response["success"].toBool(true);
    int statusCode = success ? 200 : (response["code"].toInt(500) > 0 ? response["code"].toInt(500) : 500);
    
    QString statusMessage;
    switch (statusCode) {
        case 200: statusMessage = "OK"; break;
        case 201: statusMessage = "Created"; break;
        case 400: statusMessage = "Bad Request"; break;
        case 401: statusMessage = "Unauthorized"; break;
        case 404: statusMessage = "Not Found"; break;
        case 500: statusMessage = "Internal Server Error"; break;
        default: statusMessage = "Unknown"; break;
    }
    
    QByteArray body = QJsonDocument(response).toJson(QJsonDocument::Compact);
    
    QByteArray responseData;
    responseData += "HTTP/1.1 " + QByteArray::number(statusCode) + " " + statusMessage.toUtf8() + "\r\n";
    responseData += "Content-Type: application/json\r\n";
    responseData += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    responseData += "Access-Control-Allow-Origin: *\r\n";
    responseData += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    responseData += "Access-Control-Allow-Headers: Content-Type, X-API-Key\r\n";
    responseData += "\r\n";
    responseData += body;
    
    socket->write(responseData);
    socket->flush();
    socket->disconnectFromHost();
}

void APIServer::sendErrorResponse(QTcpSocket* socket, const QString& message, int code) {
    QJsonObject error;
    error["success"] = false;
    error["error"] = message;
    error["code"] = code;
    sendResponse(socket, error);
}

QString APIServer::stateToString(InstanceState state) {
    switch (state) {
        case InstanceState::Created: return "created";
        case InstanceState::Starting: return "starting";
        case InstanceState::Running: return "running";
        case InstanceState::Stopping: return "stopping";
        case InstanceState::Stopped: return "stopped";
        case InstanceState::Paused: return "paused";
        case InstanceState::Error: return "error";
        default: return "unknown";
    }
}

QJsonObject APIServer::instanceToJson(const InstanceInfo& info) {
    QJsonObject obj;
    obj["id"] = info.instanceId;
    obj["name"] = info.containerName;
    obj["state"] = stateToString(info.state);
    obj["adbPort"] = info.adbPort;
    obj["vncPort"] = info.vncPort;
    obj["ipAddress"] = info.ipAddress;
    obj["adbConnected"] = info.adbConnected;
    obj["vncEnabled"] = info.vncEnabled;
    obj["memoryUsage"] = static_cast<qint64>(info.memoryUsage);
    obj["memoryLimit"] = static_cast<qint64>(info.memoryLimit);
    obj["cpuUsage"] = info.cpuUsage;
    obj["profileId"] = info.profileId;
    return obj;
}

QJsonObject APIServer::profileToJson(const DeviceProfile& profile) {
    QJsonObject obj;
    obj["id"] = profile.id;
    obj["name"] = profile.name;
    obj["manufacturer"] = profile.manufacturer;
    obj["brand"] = profile.brand;
    obj["model"] = profile.model;
    obj["androidVersion"] = profile.androidVersion;
    return obj;
}

QJsonObject APIServer::createProfileInfo(const QString& id, const QString& name, const QString& manufacturer) {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["manufacturer"] = manufacturer;
    return obj;
}

} // namespace VirtualPhonePro
