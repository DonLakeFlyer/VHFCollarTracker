#pragma once

#include "QGCOptions.h"

class VHFTrackerQGCPlugin;

class VHFTrackerQGCOptions : public QGCOptions
{
public:
    VHFTrackerQGCOptions(VHFTrackerQGCPlugin* plugin, QObject* parent = NULL);

    QUrl flyViewOverlay  (void) const { return QUrl::fromUserInput("qrc:/qml/VHFTrackerFlyViewOverlay.qml"); }

private:
    VHFTrackerQGCPlugin*  _vhfQGCPlugin;
};
