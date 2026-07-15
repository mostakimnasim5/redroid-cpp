#ifndef NETWORK_PANEL_H
#define NETWORK_PANEL_H
#include <QWidget>
class NetworkPanel : public QWidget {
    Q_OBJECT
public:
    explicit NetworkPanel(QWidget* parent = nullptr);
signals:
    void applyRequested(const QString& deviceId);
};
#endif
