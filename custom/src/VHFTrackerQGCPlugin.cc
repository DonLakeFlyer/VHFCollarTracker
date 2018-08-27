#include "VHFTrackerQGCPlugin.h"
#include "DirectionMapItem.h"
#include "LineMapItem.h"
#include "Vehicle.h"
#include "VHFTrackerSettings.h"
#include "QGCGeo.h"
#include "QGCApplication.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>

static const int DEBUG_TS_COMMAND_ACK =             0;
static const int DEBUG_INDEX_COMMAND_ACK_START = 	0;
static const int DEBUG_INDEX_COMMAND_ACK_STOP = 	1;

static const int DEBUG_TS_PULSE =       1;
static const int DEBUG_INDEX_PULSE =	0;

QGC_LOGGING_CATEGORY(VHFTrackerQGCPluginLog, "VHFTrackerQGCPluginLog")

VHFTrackerQGCPlugin::VHFTrackerQGCPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _strengthsAvailable   (false)
    , _beepStrength         (0)
    , _bpm                  (0)
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
#if 0
    case MAVLINK_MSG_ID_MEMORY_VECT:
        return _handleMemoryVect(vehicle, link, message);
#endif
    case MAVLINK_MSG_ID_DEBUG:
        return _handleDebug(vehicle,link, message);
    }

    return true;
}

bool VHFTrackerQGCPlugin::_handleDebug(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(link);

    mavlink_debug_t debugMsg;

    mavlink_msg_debug_decode(&message, &debugMsg);

    switch (debugMsg.time_boot_ms) {
    case DEBUG_TS_PULSE:
        _beepStrength = debugMsg.value;
        emit beepStrengthChanged(_beepStrength);
        _rgPulseValues.append(_beepStrength);
        if (_beepStrength == 0) {
            _elapsedTimer.invalidate();
        } else {
            if (_elapsedTimer.isValid()) {
                qint64 elapsed = _elapsedTimer.elapsed();
                if (elapsed > 0) {
                    _bpm = (60 * 1000) / _elapsedTimer.elapsed();
                    emit bpmChanged(_bpm);
                }
            }
            _elapsedTimer.restart();
        }
        break;
    case DEBUG_TS_COMMAND_ACK:
        if (debugMsg.ind == DEBUG_INDEX_COMMAND_ACK_START) {
            int freq = debugMsg.value;
            int numerator = freq / 1000;
            int denominator = freq - (numerator * 1000);
            _say(QStringLiteral("Start capture %1.%2").arg(numerator).arg(denominator, 3, 10, QChar('0')));
        } else if (debugMsg.ind == DEBUG_INDEX_COMMAND_ACK_STOP) {
            _say(QStringLiteral("Stop capture"));
        }
        break;
    }

    return false;
}

void VHFTrackerQGCPlugin::takeoff(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!activeVehicle) {
        return;
    }

    startDetection();

    VehicleState_t vehicleState;
    double targetAltitude = _vhfSettings->altitude()->rawValue().toDouble();

    _vehicleStates.clear();
    _vehicleStateIndex = 0;

    // Reach target height
    vehicleState.command =          MAV_CMD_NAV_TAKEOFF;
    vehicleState.fact =             activeVehicle->altitudeRelative();
    vehicleState.targetValue =      targetAltitude;
    vehicleState.targetVariance =   0.3;
    _vehicleStates.append(vehicleState);

    // Rotate
    int msecsWait = 8000;
    int subDivide = _vhfSettings->divisions()->rawValue().toInt();
    double headingIncrement = 360.0 / subDivide;
    double nextHeading = 0 - headingIncrement;
    for (int i=0; i<subDivide; i++) {
        nextHeading += headingIncrement;

        vehicleState.command =          MAV_CMD_DO_REPOSITION;
        vehicleState.fact =             activeVehicle->heading();
        vehicleState.targetValue =      nextHeading;
        vehicleState.targetVariance =   1;
        _vehicleStates.append(vehicleState);

        vehicleState.command =          MAV_CMD_NAV_DELAY;
        vehicleState.fact =             nullptr;
        vehicleState.targetValue =      msecsWait;
        vehicleState.targetVariance =   0;
        _vehicleStates.append(vehicleState);
    }

    vehicleState.command =          MAV_CMD_NAV_RETURN_TO_LAUNCH;
    vehicleState.fact =             nullptr;
    vehicleState.targetValue =      0;
    vehicleState.targetVariance =   0;
    _vehicleStates.append(vehicleState);

    _nextVehicleState();
}

void VHFTrackerQGCPlugin::_rotateVehicle(Vehicle* vehicle, double headingDegrees)
{
    vehicle->sendCommand(0,                                               // component
                         MAV_CMD_DO_REPOSITION,
                         true,                                            // show error
                         -1,                                              // no change in ground speed
                         MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,             // reposition flags not used
                         0,                                               //reserved
                         qDegreesToRadians(headingDegrees),    // change heading
                         qQNaN(), qQNaN(), qQNaN());                      // no change lat, lon, alt
}

void VHFTrackerQGCPlugin::_nextVehicleState(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!activeVehicle) {
        return;
    }

    if (_vehicleStateIndex != 0 && activeVehicle->flightMode() != "Takeoff" && activeVehicle->flightMode() != "Hold") {
        // User cancel
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    switch (currentState.command) {
    case MAV_CMD_NAV_TAKEOFF:
        // Takeoff to specified altitude
        _say(QStringLiteral("Waiting for takeoff to %1 %2").arg(FactMetaData::metersToAppSettingsDistanceUnits(currentState.targetValue).toDouble()).arg(FactMetaData::appSettingsDistanceUnitsString()));
        activeVehicle->guidedModeTakeoff(currentState.targetValue);
        break;
    case MAV_CMD_DO_REPOSITION:
        _say(QStringLiteral("Waiting for rotate to %1 degrees").arg(qRound(currentState.targetValue)));
        _rotateVehicle(activeVehicle, currentState.targetValue);
        break;
    case MAV_CMD_NAV_DELAY:
        _say(QStringLiteral("Collecting data for %1 seconds").arg(currentState.targetValue / 1000.0));
        _vehicleStateIndex++;
        QTimer::singleShot(currentState.targetValue, this, &VHFTrackerQGCPlugin::_singleCaptureComplete);
        break;
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
        _say(QStringLiteral("Collection complete returning"));
        _vehicleStateIndex++;
        activeVehicle->setFlightMode(activeVehicle->rtlFlightMode());
        _detectComplete();
        break;
    default:
        qgcApp()->showMessage(tr("VHFTrackerQGCPlugin::_nextVehicleState bad command %1").arg(currentState.command));
        break;
    }

    if (currentState.fact) {
        connect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
    }
}

void VHFTrackerQGCPlugin::_vehicleStateRawValueChanged(QVariant rawValue)
{
    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    qCDebug(VHFTrackerQGCPluginLog) << "Waiting for value actual:wait:variance" << rawValue.toDouble() << currentState.targetValue << currentState.targetVariance;

    if (qAbs(rawValue.toDouble() - currentState.targetValue) < currentState.targetVariance) {
        disconnect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
        _vehicleStateIndex++;
        if (_vehicleStateIndex < _vehicleStates.count()) {
            _nextVehicleState();
        }
    }
}

void VHFTrackerQGCPlugin::_say(QString text)
{
    qCDebug(VHFTrackerQGCPluginLog) << text;
    _toolbox->audioOutput()->say(text.toLower());
}

void VHFTrackerQGCPlugin::calibrateMaxPulse(void)
{

}

void VHFTrackerQGCPlugin::_singleCaptureComplete(void)
{
    float maxPulse = 0;
    foreach(float pulse, _rgPulseValues) {
        maxPulse = qMax(maxPulse, pulse);
    }
    _rgPulseValues.clear();
    _rgAngleStrengths.append(maxPulse);
    _nextVehicleState();
}

void VHFTrackerQGCPlugin::_detectComplete(void)
{
    // Determine strongest angle and convert to string from Qml

    float maxPulse = 0;
    int currentAngle = 0;
    _strongestAngle = 0;
    _rgStringAngleStrengths.clear();

    foreach(float pulse, _rgAngleStrengths) {
        if (pulse > maxPulse) {
            maxPulse = pulse;
            _strongestAngle = currentAngle;
        }
        currentAngle++;

        _rgStringAngleStrengths.append(QStringLiteral("%1").arg(pulse));
    }

    emit angleStrengthsChanged();
    emit strongestAngleChanged(_strongestAngle);

    _strengthsAvailable = true;
    emit strengthsAvailableChanged(true);
}

void VHFTrackerQGCPlugin::startDetection(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (vehicle) {
        vehicle->sendCommand(0, MAV_CMD_USER_1, true, _vhfSettings->frequency()->rawValue().toInt());
    }
}

void VHFTrackerQGCPlugin::stopDetection(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (vehicle) {
        vehicle->sendCommand(0, MAV_CMD_USER_2, true);
    }
}
