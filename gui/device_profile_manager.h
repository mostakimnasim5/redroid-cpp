// device_profile_manager.h
#ifndef DEVICE_PROFILE_MANAGER_H
#define DEVICE_PROFILE_MANAGER_H
#include <QWidget>
class DeviceProfileManager : public QWidget {
    Q_OBJECT
public:
    explicit DeviceProfileManager(QWidget* parent = nullptr);
signals:
    void profileSelected(const QString& profile);
};
#endif
