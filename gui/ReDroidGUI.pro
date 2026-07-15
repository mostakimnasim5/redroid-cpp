QT       += core gui widgets network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    devicecard.cpp \
    dashboard.cpp \
    anti_detection_panel.cpp \
    device_profile_manager.cpp \
    vnc_viewer.cpp \
    settings_dialog.cpp \
    about_dialog.cpp \
    log_viewer.cpp \
    network_panel.cpp \
    simulation_panel.cpp

HEADERS += \
    mainwindow.h \
    devicecard.h \
    dashboard.h \
    anti_detection_panel.h \
    device_profile_manager.h \
    vnc_viewer.h \
    settings_dialog.h \
    about_dialog.h \
    log_viewer.h \
    network_panel.h \
    simulation_panel.h

FORMS += \
    mainwindow.ui \
    settings_dialog.ui \
    about_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
