#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H
#include <QWidget>
#include <QTextEdit>
class LogViewer : public QWidget {
    Q_OBJECT
public:
    explicit LogViewer(QWidget* parent = nullptr);
    void addLog(const QString& message);
};
#endif
