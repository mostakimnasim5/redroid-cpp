#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application info
    QApplication::setApplicationName("ReDroidCPP");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("ReDroidCPP");
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
