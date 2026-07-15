#include "vnc_viewer.h"
#include <QVBoxLayout>
#include <QLabel>
VNCViewer::VNCViewer(QWidget* parent) : QWidget(parent) {
    setWindowTitle("VNC Viewer");
    resize(800, 600);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("VNC Viewer - Connect to device"));
}
void VNCViewer::connectToDevice(int port, const QString& name) {
    setWindowTitle("VNC - " + name);
}
