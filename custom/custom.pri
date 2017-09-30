message("Adding Sentera plugin")

DEFINES += CUSTOMHEADER=\"\\\"VHFTrackerQGCPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=VHFTrackerQGCPlugin

DEFINES += QGC_APPLICATION_NAME='"\\\"VHF Animal Tracker\\\""'
DEFINES += QGC_ORG_NAME=\"\\\"LatestFiasco.org\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"org.latestfiasco\\\"\"

CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY QGC_DISABLE_PX4_PLUGIN_FACTORY

QGC_ORG_NAME        = "LatestFiasco.org"
QGC_ORG_DOMAIN      = "org.latestfiasco"
QGC_APP_DESCRIPTION = "VHF Animal Tracker"
QGC_APP_COPYRIGHT   = "Copyright (C) 2017 Don Gagne. All rights reserved."
QGC_APP_NAME        = "VHF Animal Tracker"
TARGET              = "VHFAnimalTracker"

WindowsBuild {
    RC_ICONS                    = $$PWD/resources/SenteraBurstGreen.ico
    QGC_INSTALLER_ICON          = "custom\\resources\\SenteraBurstGreen.ico"
    QGC_INSTALLER_HEADER_BITMAP = "custom\\resources\\InstallerHeader.bmp"
}

RESOURCES += \
    $$PWD/VHFTrackerQGCPlugin.qrc \

INCLUDEPATH += \
    $$PWD/src \

HEADERS += \
    $$PWD/src/DirectionMapItem.h \
    $$PWD/src/VHFTrackerFirmwarePlugin.h \
    $$PWD/src/VHFTrackerFirmwarePluginFactory.h \
    $$PWD/src/VHFTrackerQGCOptions.h \
    $$PWD/src/VHFTrackerQGCPlugin.h \
    $$PWD/src/VHFTrackerSettings.h \

SOURCES += \
    $$PWD/src/DirectionMapItem.cc \
    $$PWD/src/VHFTrackerFirmwarePlugin.cc \
    $$PWD/src/VHFTrackerFirmwarePluginFactory.cc \
    $$PWD/src/VHFTrackerQGCOptions.cc \
    $$PWD/src/VHFTrackerQGCPlugin.cc \
    $$PWD/src/VHFTrackerSettings.cc \
