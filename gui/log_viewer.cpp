#include "log_viewer.h"
#include <QVBoxLayout>
LogViewer::LogViewer(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Log Viewer");
    resize(800, 500);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QTextEdit* logEdit = new QTextEdit();
    logEdit->setReadOnly(true);
    logEdit->setStyleSheet("background: #1a1a2e; color: #00ff00;");
    layout->addWidget(logEdit);
}
void LogViewer::addLog(const QString& message) {
    // Add log entry
}
