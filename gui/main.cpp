/**
 * @file main.cpp
 * @brief ReDroidCPP GUI Application Entry Point
 * @version 3.0 Ultimate Banking Edition
 */

#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Application Information
    QApplication::setApplicationName("ReDroidCPP");
    QApplication::setApplicationVersion("3.0.0");
    QApplication::setOrganizationName("VirtualPhonePro");
    QApplication::setOrganizationDomain("virtualphonepro.com");
    
    // Set application attributes
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    
    // Load stylesheet (optional - will work without it)
    QFile styleFile(":/styles/dark.css");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        // Fallback minimal dark style
        app.setStyleSheet(R"(
            QMainWindow { background: #0d1117; color: #c9d1d9; }
            QWidget { background: #0d1117; color: #c9d1d9; }
            QPushButton { background: #21262d; color: #c9d1d9; border: 1px solid #30363d; border-radius: 6px; padding: 6px 12px; }
            QPushButton:hover { background: #30363d; }
        )");
    }
    
    // Create and show main window
    MainWindow w;
    w.show();
    
    return app.exec();
}
