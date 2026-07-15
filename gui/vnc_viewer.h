#ifndef VNC_VIEWER_H
#define VNC_VIEWER_H
#include <QWidget>
class VNCViewer : public QWidget {
    Q_OBJECT
public:
    explicit VNCViewer(QWidget* parent = nullptr);
    void connectToDevice(int port, const QString& name);
};
#endif
