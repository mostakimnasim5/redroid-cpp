#ifndef SIMULATION_PANEL_H
#define SIMULATION_PANEL_H
#include <QWidget>
class SimulationPanel : public QWidget {
    Q_OBJECT
public:
    explicit SimulationPanel(QWidget* parent = nullptr);
signals:
    void applyRequested(const QString& deviceId);
};
#endif
