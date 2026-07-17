#include "MainWindow.h"
#include "PhoneWindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
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
    
    QCommandLineOption phoneModeOption(
        QStringList() << "p" << "phone",
        "Open a specific phone instance (default: 1)"
    );
    parser.addOption(phoneModeOption);
    
    parser.process(app);
    
    // Demo mode - show a sample phone window
    if (parser.isSet(demoModeOption)) {
        // Create a demo profile
        VirtualPhonePro::DeviceProfile demoProfile;
        demoProfile.manufacturer = "Samsung";
        demoProfile.model = "Galaxy S24 Ultra";
        demoProfile.androidVersion = 14;
        demoProfile.sdkVersion = 34;
        
        VirtualPhonePro::PhoneWindow phoneWindow("demo_1", demoProfile);
        phoneWindow.show();
        return app.exec();
    }
    
    // Normal mode - show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
