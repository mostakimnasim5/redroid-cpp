/**
 * @file qtmain.cpp
 * @brief Qt6 GUI Application Main Entry Point
 * @version 2.0.0
 *
 * VirtualPhonePro Qt6 GUI with Docker Auto-Start support
 * Includes Firebase-based admin authentication
 */

#include <QApplication>
#include <QMessageBox>
#include <QSocketNotifier>
#include <QStyleFactory>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QPixmap>
#include <QIcon>

#include "GUI/DashboardWindow.hpp"
#include "GUI/LoginWindow.hpp"
#include "VirtualPhonePro/ReDroidController.hpp"
#include "VirtualPhonePro/WebhookManager.hpp"
#include "VirtualPhonePro/APIServer.hpp"
#include "VirtualPhonePro/MultiInstanceManager.hpp"
#include "VirtualPhonePro/FileLogger.hpp"

#include <csignal>

#ifdef Q_OS_WIN
#  include <io.h>
#  include <fcntl.h>
   // Windows has no pipe() — use a self-connected UDP socket pair instead.
#  include <winsock2.h>
#else
#  include <unistd.h>   // pipe(), write(), read()
#endif

using namespace VirtualPhonePro;

// ============================================================================
// Crash Handler — async-signal-safe pipe bridge
//
// POSIX (and Windows SEH) prohibit calling anything that acquires a lock,
// allocates heap memory, or touches Qt internals from inside a signal
// handler.  That rules out QString, QMessageBox, QList, FileLogger, and
// every other Qt class.
//
// Solution: the raw signal handler does ONE thing — write(2) the signal
// number into a self-pipe.  A QSocketNotifier on the main Qt thread reads
// that byte and dispatches the real cleanup + UI from safe context.
// ============================================================================

// Self-pipe file descriptors (write-end written by signal handler,
// read-end watched by QSocketNotifier on the main thread).
static int g_crashPipe[2] = {-1, -1};

// The only code that runs inside the signal handler.
// write(2) is async-signal-safe; everything else is forbidden here.
static void rawSignalHandler(int signum) noexcept {
#ifdef Q_OS_WIN
    // On Windows we use a loopback socket pair; send() is safe enough
    // for our purposes (called once, no retry needed).
    char byte = static_cast<char>(signum);
    ::send(g_crashPipe[1], &byte, 1, 0);
#else
    char byte = static_cast<char>(signum);
    // Ignore return value — we cannot do anything useful on failure
    // inside a signal handler.
    (void)::write(g_crashPipe[1], &byte, 1);
#endif
}

// Called on the main Qt thread by QSocketNotifier — all Qt APIs are safe.
static void onCrashSignalReceived(int signum) {
    const char* sigName = "Unknown Signal";
    const char* detail  = "An unexpected error occurred.";

    switch (signum) {
        case SIGSEGV:
            sigName = "SIGSEGV";
            detail  = "Segmentation fault — illegal memory access.";
            break;
        case SIGABRT:
            sigName = "SIGABRT";
            detail  = "Abnormal termination — assertion or abort() called.";
            break;
        case SIGFPE:
            sigName = "SIGFPE";
            detail  = "Floating-point exception — division by zero or overflow.";
            break;
    }

    // 1. Write to log (safe — we are on the main thread now).
    const QString logMsg = QString("[%1] %2").arg(sigName, detail);
    FileLogger::instance().critical("CrashHandler", logMsg);

    // 2. Stop all running containers gracefully.
    ReDroidController& ctrl = ReDroidController::instance();
    const QList<InstanceInfo> instances = ctrl.listInstances();
    for (const InstanceInfo& info : instances) {
        if (info.state == InstanceState::Running) {
            ctrl.stopInstance(info.instanceId, /*force=*/true);
        }
    }

    // 3. Show user-facing dialog (safe — main thread, event loop still alive).
    QMessageBox::critical(
        nullptr,
        QStringLiteral("Application Crash — %1").arg(sigName),
        QString("Signal: %1\n\n%2\n\nLog: %3")
            .arg(sigName, detail, FileLogger::instance().getLogFilePath())
    );

    // 4. Hard exit — do not re-enter Qt event loop after a fatal signal.
    ::exit(1);
}

// Install the self-pipe and register the raw signal handler.
// Returns false if pipe creation fails (non-fatal: app runs without handler).
static bool installCrashHandler(QObject* parent) {
#ifdef Q_OS_WIN
    // Windows: loopback socket pair (Winsock must already be initialised
    // by Qt at this point).
    SOCKET listener = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = 0;
    ::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(listener, 1);
    int addrLen = sizeof(addr);
    ::getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    g_crashPipe[1] = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
    ::connect(static_cast<SOCKET>(g_crashPipe[1]),
              reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    g_crashPipe[0] = static_cast<int>(::accept(listener, nullptr, nullptr));
    ::closesocket(listener);
#else
    if (::pipe(g_crashPipe) != 0)
        return false;
#endif

    // QSocketNotifier watches the read-end on the main thread.
    auto* notifier = new QSocketNotifier(g_crashPipe[0],
                                         QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated,
                     [notifier](QSocketDescriptor) {
        // Disable notifier before reading to avoid re-entrancy.
        notifier->setEnabled(false);

        char byte = 0;
#ifdef Q_OS_WIN
        ::recv(g_crashPipe[0], &byte, 1, 0);
#else
        (void)::read(g_crashPipe[0], &byte, 1);
#endif
        onCrashSignalReceived(static_cast<int>(byte));
    });

    // Register the async-signal-safe raw handler for each fatal signal.
    ::signal(SIGSEGV, rawSignalHandler);
    ::signal(SIGABRT, rawSignalHandler);
    ::signal(SIGFPE,  rawSignalHandler);

    return true;
}

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
    // Initialize FileLogger FIRST (before everything else)
    FileLogger::instance().info("Startup", "========================================");
    FileLogger::instance().info("Startup", "VirtualPhonePro v3.0.0 Starting...");
    FileLogger::instance().info("Startup", "========================================");
    
    // Set application info
    QCoreApplication::setApplicationName("VirtualPhonePro");
    QCoreApplication::setApplicationVersion("2.0.0");
    QCoreApplication::setOrganizationName("VirtualPhonePro");
    
    // Create application
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    // =========================================================================
    // Install crash handler AFTER QApplication exists.
    // QSocketNotifier requires a running event loop; the pipe write-end is
    // filled by a minimal async-signal-safe raw handler, and the real
    // cleanup (FileLogger, QMessageBox, container shutdown) runs safely on
    // the main thread via the notifier callback.
    // =========================================================================
    if (!installCrashHandler(&app)) {
        FileLogger::instance().warning("Startup",
            "Could not install crash handler (pipe creation failed). "
            "Crashes will not be caught gracefully.");
    }
    
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

    // =========================================================================
    // Full system prerequisite check — run before any container operation.
    // Shows user exactly what is missing and how to fix it.
    // =========================================================================
    FileLogger::instance().info("Startup", "Checking system requirements...");
    SystemCheckReport sysReport = controller.checkSystemRequirements();

    if (!sysReport.canRun) {
        // Build a readable report for the dialog
        QString details;
        for (const SystemRequirement& req : sysReport.checks) {
            QString icon = req.met ? "✓" : (req.required ? "✗" : "⚠");
            details += QString("%1  %2: %3\n").arg(icon, req.name, req.status);
            if (!req.met && !req.fixInstruction.isEmpty()) {
                for (const QString& line : req.fixInstruction.split('\n'))
                    details += QString("      %1\n").arg(line);
            }
        }

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("System Requirements Not Met");
        msgBox.setText(
            "The following required components are missing.\n"
            "Container features will not work until they are resolved.");
        msgBox.setDetailedText(details);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        FileLogger::instance().critical("Startup",
            "Missing required components: " +
            sysReport.missingRequired().join(", "));
        // App continues — user can still view settings / profiles
    } else {
        // Log optional warnings
        for (const SystemRequirement& req : sysReport.checks) {
            if (!req.met)
                FileLogger::instance().warning("Startup",
                    QString("[OPTIONAL] %1: %2").arg(req.name, req.status));
        }
        qDebug() << "[Startup] All required system checks passed.";
    }

    // ── WebhookManager — connect to ReDroidController event signals ───────────
    // WebhookManager sends HTTP notifications on instance start/stop/error
    // events to any configured webhook URL (set via Settings UI).
    // No webhook URL configured by default — user opts in via Settings.
    {
        using namespace VirtualPhonePro;
        WebhookManager& wm = WebhookManager::instance();
        ReDroidController& ctrl = ReDroidController::instance();

        QObject::connect(&ctrl, &ReDroidController::instanceStateChanged,
            [&wm](const QString& instanceId, InstanceState state) {
                QJsonObject data;
                data["instanceId"] = instanceId;
                if (state == InstanceState::Running)
                    wm.triggerEvent("instance_started", data);
                else if (state == InstanceState::Stopped)
                    wm.triggerEvent("instance_stopped", data);
            });

        QObject::connect(&ctrl, &ReDroidController::error,
            [&wm](const QString& msg) {
                // Error messages don't carry instanceId at the signal level.
                QJsonObject data;
                data["instanceId"] = QString();
                data["error"] = msg;
                wm.triggerEvent("instance_error", data);
            });

        qDebug() << "[Startup] WebhookManager connected to instance events";
    }

    // ── APIServer — REST API for remote instance control ──────────────────────
    // Exposes /instances (list), /instances/:id/start|stop over HTTP.
    // APIServer internally calls ReDroidController::instance() to serve
    // requests — no explicit signal wiring needed (pull model).
    // Default port 8080; warns and skips if port is already bound.
    {
        using namespace VirtualPhonePro;
        APIServer& api = APIServer::instance();

        const quint16 apiPort = 8080;
        if (api.start(apiPort)) {
            qDebug() << "[Startup] APIServer listening on port" << apiPort;
        } else {
            qWarning() << "[Startup] APIServer could not bind port" << apiPort
                       << "— remote control unavailable";
        }

        QObject::connect(&app, &QApplication::aboutToQuit, [&api]() {
            api.stop();
        });
    }
    
    // ============================================================================
    // Authentication Flow - Show Login Window First
    // ============================================================================
    qDebug() << "[Startup] Showing login window...";
    
    LoginWindow loginWindow;
    loginWindow.setWindowTitle("ReDroidCPP - Login");
    loginWindow.setFixedSize(450, 550);
    
    // Try to load icon
    QIcon appIcon(":/icons/app.png");
    if (!appIcon.isNull()) {
        loginWindow.setWindowIcon(appIcon);
    } else {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::blue);
        loginWindow.setWindowIcon(QIcon(pixmap));
    }
    
    loginWindow.show();
    
    // Variable to track if user is authenticated
    bool authenticated = false;
    
    // Check if login window is closed without login - exit app
    QObject::connect(&loginWindow, &QWidget::destroyed, [&]() {
        if (!authenticated) {
            qDebug() << "[Auth] Login window closed without authentication - exiting";
            app.quit();
        }
    });
    
    // Also handle close event
    QObject::connect(&loginWindow, &QObject::destroyed, [&]() {
        if (!authenticated) {
            qDebug() << "[Auth] Login window closed - exiting application";
        }
    });
    
    // Connect login success signal - when user successfully logs in, show Dashboard
    QObject::connect(&loginWindow, &LoginWindow::loginSuccess,
                     [&](const QString& userId, const QString& uniqueKey, int remainingProfiles, int totalProfiles) {
        qDebug() << "[Auth] Login successful! User:" << userId;
        qDebug() << "[Auth] Remaining profiles:" << remainingProfiles << "/" << totalProfiles;
        authenticated = true;
        Q_UNUSED(userId);
        Q_UNUSED(uniqueKey);
        Q_UNUSED(remainingProfiles);
        Q_UNUSED(totalProfiles);
        
        // Hide login window
        loginWindow.hide();
        
        // Create and show Dashboard window
        DashboardWindow* dashboard = new DashboardWindow();
        dashboard->setAttribute(Qt::WA_DeleteOnClose);
        
        if (!appIcon.isNull()) {
            dashboard->setWindowIcon(appIcon);
        } else {
            QPixmap pixmap(32, 32);
            pixmap.fill(Qt::blue);
            dashboard->setWindowIcon(QIcon(pixmap));
        }
        
        dashboard->show();
        
        // Auto-start saved instances after dashboard is shown
        QTimer::singleShot(1000, [&, dashboard]() {
            if (sysReport.canRun) {
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
    });
    
    // Run application event loop
    // The login window will be shown first
    // After successful login, loginSuccess signal is emitted and DashboardWindow is shown
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
