#include "VHFTrackerQGCPlugin.h"
#include "DirectionMapItem.h"
#include "Vehicle.h"
#include "VHFTrackerSettings.h"

#include <QDebug>

VHFTrackerQGCPlugin::VHFTrackerQGCPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin (app, toolbox)
    , _beepStrength (0)
    , _bpm          (0)
{
    _showAdvancedUI = true;
}

VHFTrackerQGCPlugin::~VHFTrackerQGCPlugin()
{

}

void VHFTrackerQGCPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    _vhfSettings = new VHFTrackerSettings(this);
    _vhfQGCOptions = new VHFTrackerQGCOptions(this, this);
}

QString VHFTrackerQGCPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/res/PaintedDogsLogo.png");
}

QString VHFTrackerQGCPlugin::brandImageOutdoor(void) const
{
    return QStringLiteral("/res/PaintedDogsLogo.png");
}

QVariantList& VHFTrackerQGCPlugin::settingsPages(void)
{
    if(_settingsPages.size() == 0) {
        _settingsPages = QGCCorePlugin::settingsPages();
    }

    return _settingsPages;
}

QVariantList& VHFTrackerQGCPlugin::instrumentPages(void)
{
    if(_instrumentPages.size() == 0) {
        _instrumentPages = QGCCorePlugin::instrumentPages();
        _instrumentPages.prepend(QVariant::fromValue(new QmlComponentInfo(tr("Tracker"), QUrl::fromUserInput("qrc:/qml/VHFTrackerWidget.qml"), QUrl(), this)));
    }

    return _instrumentPages;
}

bool VHFTrackerQGCPlugin::mavlinkMessage(Vehicle* vehicle, LinkInterface* link, mavlink_message_t message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MEMORY_VECT:
        return _handleMemoryVect(vehicle, link, message);
    case MAVLINK_MSG_ID_DEBUG:
        return _handleDebug(vehicle,link, message);
    }

    return true;
}

bool VHFTrackerQGCPlugin::_handleMemoryVect(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message)
{
    Q_UNUSED(link);

    mavlink_memory_vect_t memoryVect;

    mavlink_msg_memory_vect_decode(&message, &memoryVect);

    QList<QColor> signalStrengthColors;
    for (int i=0; i<16; i++) {
        signalStrengthColors.append(QColor(0, (uint8_t)memoryVect.value[i] * 2, 0));
    }

    _mapItems.append(new DirectionMapItem(vehicle->coordinate(), signalStrengthColors, this));

    return false;
}


bool VHFTrackerQGCPlugin::_handleDebug(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(link);

    mavlink_debug_t debugMsg;

    mavlink_msg_debug_decode(&message, &debugMsg);

    _beepStrength = debugMsg.value;
    emit beepStrengthChanged(_beepStrength);

    if (_beepStrength == 0) {
        _elapsedTimer.invalidate();
    } else {
        if (_elapsedTimer.isValid()) {
            _bpm = (60 * 1000) / _elapsedTimer.elapsed();
            emit bpmChanged(_bpm);
        }
        _elapsedTimer.restart();
    }

    return false;
}
