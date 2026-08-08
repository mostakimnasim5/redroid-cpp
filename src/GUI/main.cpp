#include "MainWindow.h"
#include "PhoneWindow.h"
#include "LoginWindow.h"
#include "VirtualPhonePro/DeviceProfile.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

#ifdef _WIN32
  #include <windows.h>
#endif

int main(int argc, char *argv[]) {

    // -----------------------------------------------------------------------
    // MUST run BEFORE QApplication — set Qt plugin path to exe directory
    // Fixes: "no Qt platform plugin could be initialized" on any Windows PC
    // Uses Windows API to get the REAL exe path regardless of working dir
    // -----------------------------------------------------------------------
#ifdef _WIN32
    WCHAR exePathW[MAX_PATH];
    GetModuleFileNameW(NULL, exePathW, MAX_PATH);
    QString exeDir = QFileInfo(QString::fromWCharArray(exePathW)).absolutePath();
    // Primary: exe directory itself (contains platforms/, styles/, etc.)
    QCoreApplication::addLibraryPath(exeDir);
    // Secondary: explicit platforms subfolder
    QCoreApplication::addLibraryPath(exeDir + "\\platforms");
#else
    QCoreApplication::addLibraryPath(".");
#endif

    QApplication app(argc, argv);

    // Set application info
    QApplication::setApplicationName("ReDroidCPP");
    QApplication::setApplicationVersion("3.0.0");
    QApplication::setOrganizationName("ReDroidCPP");

    // Command line parser for demo mode
    QCommandLineParser parser;
    parser.setApplicationDescription("ReDroidCPP - Professional Android Emulator");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption demoModeOption(
        QStringList() << "d" << "demo",
        "Run in demo mode with a sample phone window"
    );
    parser.addOption(demoModeOption);

    QCommandLineOption skipLoginOption(
        QStringList() << "s" << "skip-login",
        "Skip login and show main window directly (for testing)"
    );
    parser.addOption(skipLoginOption);

    parser.process(app);

    // Demo mode - show a sample phone window
    if (parser.isSet(demoModeOption)) {
        VirtualPhonePro::DeviceProfile demoProfile;
        demoProfile.manufacturer = "Samsung";
        demoProfile.name = "Samsung Galaxy S24 Ultra";
        demoProfile.build.model = "Galaxy S24 Ultra";
        demoProfile.build.androidVersion = 14;
        demoProfile.build.sdkVersion = 34;

        VirtualPhonePro::PhoneWindow phoneWindow("demo_1", demoProfile);
        phoneWindow.show();
        return app.exec();
    }

    // Skip login mode - for testing
    if (parser.isSet(skipLoginOption)) {
        MainWindow window;
        window.show();
        return app.exec();
    }

    // Normal mode - show login window first
    LoginWindow loginWindow;
    loginWindow.show();

    // Connect login success signal
    QObject::connect(&loginWindow, &LoginWindow::loginSuccess,
        [&](const QString &userId, const QString &uniqueKey,
            int remainingProfiles, int totalProfiles) {
        MainWindow *mainWindow = new MainWindow();
        mainWindow->show();
        loginWindow.hide();
    });

    return app.exec();
}
