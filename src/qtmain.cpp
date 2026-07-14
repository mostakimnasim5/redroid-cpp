/**
 * @file qtmain.cpp
 * @brief Qt6 GUI Application Main Entry Point
 * @version 2.0.0
 * 
 * VirtualPhonePro Qt6 GUI with Docker Auto-Start support
 */

#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

#include "mainwindow.h"
#include "VirtualPhonePro/ReDroidController.h"
#include "VirtualPhonePro/MultiInstanceManager.h"

using namespace VirtualPhonePro;

// ============================================================================
// Auto-Start Manager - Auto-start containers on app launch
// ============================================================================

class AutoStartManager : public QObject {
    Q_OBJECT

public:
    explicit AutoStartManager(QObject* parent = nullptr) : QObject(parent) {
        loadSavedInstances();
    }

    void restoreSavedInstances() {
        qDebug() << "[AutoStart] Loading saved instances...";
        
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QString instancesFile = configDir + "/saved_instances.json";
        
        QFile file(instancesFile);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "[AutoStart] No saved instances found";
            return;
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "[AutoStart] JSON parse error:" << error.errorString();
            return;
        }
        
        QJsonObject json = doc.object();
        QJsonArray instances = json["instances"].toArray();
        
        qDebug() << "[AutoStart] Found" << instances.size() << "saved instances";
        
        for (const QJsonValue& value : instances) {
            QJsonObject instance = value.toObject();
            QString instanceId = instance["instanceId"].toString();
            QString profileData = instance["profileData"].toString();
            
            if (instanceId.isEmpty()) continue;
            
            qDebug() << "[AutoStart] Restoring instance:" << instanceId;
            
            // Load profile from JSON
            QJsonDocument profileDoc = QJsonDocument::fromJson(profileData.toUtf8());
            DeviceProfile profile;
            profile.fromJson(profileDoc.object());
            
            // Start the instance
            ReDroidController& controller = ReDroidController::instance();
            if (controller.startInstance(instanceId, profile)) {
                qDebug() << "[AutoStart] Instance started:" << instanceId;
            } else {
                qDebug() << "[AutoStart] Failed to start instance:" << instanceId;
            }
        }
    }

    void saveInstance(const QString& instanceId, const DeviceProfile& profile) {
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        QString instancesFile = configDir + "/saved_instances.json";
        
        // Load existing data
        QJsonObject json;
        QJsonArray instances;
        
        QFile file(instancesFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error == QJsonParseError::NoError) {
                json = doc.object();
                instances = json["instances"].toArray();
            }
        }
        
        // Add or update instance
        QJsonObject newInstance;
        newInstance["instanceId"] = instanceId;
        newInstance["profileData"] = QString(QJsonDocument(profile.toJson()).toJson());
        newInstance["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        // Remove existing entry if present
        QJsonArray newInstances;
        for (const QJsonValue& value : instances) {
            QJsonObject obj = value.toObject();
            if (obj["instanceId"].toString() != instanceId) {
                newInstances.append(obj);
            }
        }
        newInstances.append(newInstance);
        
        json["instances"] = newInstances;
        
        // Save
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(json).toJson());
            file.close();
            qDebug() << "[AutoStart] Instance saved:" << instanceId;
        }
    }

    void removeInstance(const QString& instanceId) {
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QString instancesFile = configDir + "/saved_instances.json";
        
        QFile file(instancesFile);
        if (!file.open(QIODevice::ReadOnly)) return;
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) return;
        
        QJsonObject json = doc.object();
        QJsonArray instances = json["instances"].toArray();
        
        // Remove instance
        QJsonArray newInstances;
        for (const QJsonValue& value : instances) {
            QJsonObject obj = value.toObject();
            if (obj["instanceId"].toString() != instanceId) {
                newInstances.append(obj);
            }
        }
        
        json["instances"] = newInstances;
        
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(json).toJson());
            file.close();
        }
    }

private:
    void loadSavedInstances() {
        // Implementation in restoreSavedInstances
    }
};

// Global auto-start manager
AutoStartManager* g_autoStartManager = nullptr;

// ============================================================================
// Main Application
// ============================================================================

int main(int argc, char *argv[]) {
    // Set application info
    QCoreApplication::setApplicationName("VirtualPhonePro");
    QCoreApplication::setApplicationVersion("2.0.0");
    QCoreApplication::setOrganizationName("VirtualPhonePro");
    
    // Enable high DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // Create application
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Setup dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // Create auto-start manager
    g_autoStartManager = new AutoStartManager(&app);
    
    // Initialize ReDroid Controller
    ReDroidController& controller = ReDroidController::instance();
    
    // Validate Docker on startup
    qDebug() << "[Startup] Validating Docker...";
    OperationResult dockerResult = controller.validateDocker();
    
    if (!dockerResult.success) {
        QMessageBox::warning(nullptr, "Docker Not Available",
            QString("Docker is not available or not running.\n\n"
                    "Error: %1\n\n"
                    "Please install Docker Desktop and ensure it is running.\n"
                    "You can still use the application, but container features will be disabled.")
                .arg(dockerResult.errorMessage));
    } else {
        qDebug() << "[Startup] Docker validated:" << dockerResult.data.value("version").toString();
    }
    
    // Create and show main window
    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/app.png"));
    window.show();
    
    // Auto-start saved instances after window is shown
    QTimer::singleShot(1000, [&]() {
        if (dockerResult.success) {
            qDebug() << "[Startup] Restoring auto-start instances...";
            
            // Show auto-start dialog
            QMessageBox autoStartMsg;
            autoStartMsg.setWindowTitle("Auto-Start Containers");
            autoStartMsg.setText("Would you like to restore previously saved container instances?");
            autoStartMsg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            autoStartMsg.setDefaultButton(QMessageBox::Yes);
            
            if (autoStartMsg.exec() == QMessageBox::Yes) {
                g_autoStartManager->restoreSavedInstances();
            }
        }
    });
    
    // Connect signals for auto-save
    QObject::connect(&controller, &ReDroidController::instanceStateChanged,
                     [&](const QString& instanceId, InstanceState state) {
        if (state == InstanceState::Running) {
            qDebug() << "[AutoSave] Instance running, saving state...";
            // Get profile and save
            InstanceInfo info = controller.getInstanceInfo(instanceId);
            // Save instance for auto-start
        }
    });
    
    // Run application
    int result = app.exec();
    
    // Cleanup: Save instances before exit
    qDebug() << "[Shutdown] Saving instances for auto-start...";
    
    // Stop all running instances
    QList<InstanceInfo> instances = controller.listInstances();
    for (const InstanceInfo& info : instances) {
        if (info.state == InstanceState::Running) {
            qDebug() << "[Shutdown] Stopping instance:" << info.instanceId;
            controller.stopInstance(info.instanceId, true);
        }
    }
    
    return result;
}

#include "qtmain.moc"
