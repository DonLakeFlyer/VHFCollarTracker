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

// Mavlink DEBUG messages are used to communicate with QGC in both directions.
// 	DEBUG.time_boot_msg is used to hold a command id
//	DEBUG.index/value are then command specific

// Pulse value
//	DEBUG.index - not used
//	DEBUG.value = pulse value
static const int DEBUG_COMMAND_ID_PULSE = 	0;

// Set gain
//	DEBUG.index - new gain
//	DEBUG.value = not used
static const int DEBUG_COMMAND_ID_SET_GAIN = 1;

// Set frequency
//	DEBUG.index - new frequency
//	DEBUG.value = not used
static const int DEBUG_COMMAND_ID_SET_FREQ = 2;

// Ack for SET commands
//	DEBUG.index - command being acked
//	DEBUG.value - gain/freq value which was chaned to
static const int DEBUG_COMMAND_ID_ACK = 			3;
static const int DEBUG_COMMAND_ACK_SET_GAIN_INDEX =	0;
static const int DEBUG_COMMAND_ACK_SET_FREQ_INDEX =	1;

QGC_LOGGING_CATEGORY(VHFTrackerQGCPluginLog, "VHFTrackerQGCPluginLog")

VHFTrackerQGCPlugin::VHFTrackerQGCPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _strengthsAvailable   (false)
    , _flightMachineActive  (false)
    , _beepStrength         (0)
    , _bpm                  (0)
{
    _showAdvancedUI = true;

    _delayTimer.setSingleShot(true);
    _targetValueTimer.setSingleShot(true);

    connect(&_delayTimer,       &QTimer::timeout, this, &VHFTrackerQGCPlugin::_delayComplete);
    connect(&_targetValueTimer, &QTimer::timeout, this, &VHFTrackerQGCPlugin::_targetValueFailed);
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
    case MAVLINK_MSG_ID_DEBUG_VECT:
        return _handleDebugVect(vehicle,link, message);
    }

    return true;
}

bool VHFTrackerQGCPlugin::_handleDebugVect(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(link);

    mavlink_debug_vect_t debugVect;

    mavlink_msg_debug_vect_decode(&message, &debugVect);

    switch (static_cast<int>(debugVect.x)) {
    case DEBUG_COMMAND_ID_PULSE:
        static int count = 0;
        qDebug() << "DEBUG" << count++ << debugVect.y;
        _beepStrength = static_cast<double>(debugVect.y);
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
    case DEBUG_COMMAND_ID_ACK:
        int ackCommand =    static_cast<int>(debugVect.y);
        int ackValue =      static_cast<int>(debugVect.z);
        if (ackCommand == DEBUG_COMMAND_ACK_SET_FREQ_INDEX) {
            int freq = ackValue;
            int numerator = freq / 1000;
            int denominator = freq - (numerator * 1000);
            _say(QStringLiteral("Frequency changed %1.%2").arg(numerator).arg(denominator, 3, 10, QChar('0')));
        } else if (ackCommand== DEBUG_COMMAND_ACK_SET_GAIN_INDEX) {
            _say(QStringLiteral("Gain changed to %2").arg(ackValue));
        }
        break;
    }

    return false;
}

void VHFTrackerQGCPlugin::_updateFlightMachineActive(bool flightMachineActive)
{
    _flightMachineActive = flightMachineActive;
    emit flightMachineActiveChanged(flightMachineActive);
}

void VHFTrackerQGCPlugin::cancelAndReturn(void)
{
    _say("Cancelling flight.");
    _resetStateAndRTL();
}


void VHFTrackerQGCPlugin::takeoff(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    _updateFlightMachineActive(true);

    if (!_armVehicleAndValidate(vehicle)) {
        _resetStateAndRTL();
        return;
    }

    VehicleState_t vehicleState;
    double targetAltitude = _vhfSettings->altitude()->rawValue().toDouble();

    _vehicleStates.clear();
    _vehicleStateIndex = 0;

    // Reach target height
    vehicleState.command =              CommandTakeoff;
    vehicleState.fact =                 vehicle->altitudeRelative();
    vehicleState.targetValueWaitSecs =  120;
    vehicleState.targetValue =          targetAltitude;
    vehicleState.targetVariance =       0.3;
    _vehicleStates.append(vehicleState);

    // Rotate
    int subDivide = _vhfSettings->divisions()->rawValue().toInt();
    double headingIncrement = 360.0 / subDivide;
    double nextHeading = vehicle->heading()->rawValue().toDouble() - headingIncrement;
    for (int i=0; i<subDivide; i++) {
        nextHeading += headingIncrement;
        if (nextHeading > 360) {
            nextHeading -= 360;
        }

        vehicleState.command =              CommandSetHeading;
        vehicleState.fact =                 vehicle->heading();
        vehicleState.targetValueWaitSecs =  10;
        vehicleState.targetValue =          nextHeading;
        vehicleState.targetVariance =       1;
        _vehicleStates.append(vehicleState);

        vehicleState.command =              CommandDelay;
        vehicleState.fact =                 nullptr;
        vehicleState.targetValueWaitSecs =  8;
        _vehicleStates.append(vehicleState);
    }

    _nextVehicleState();
}

void VHFTrackerQGCPlugin::_sendCommandAndVerify(Vehicle* vehicle, MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    connect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
    vehicle->sendMavCommand(MAV_COMP_ID_ALL,
                            command,
                            false /* showError */,
                            static_cast<float>(param1),
                            static_cast<float>(param2),
                            static_cast<float>(param3),
                            static_cast<float>(param4),
                            static_cast<float>(param5),
                            static_cast<float>(param6),
                            static_cast<float>(param7));
}

void VHFTrackerQGCPlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noResponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);

    Vehicle* vehicle = dynamic_cast<Vehicle*>(sender());
    if (!vehicle) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    if (!_flightMachineActive) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    if (currentState.command == CommandTakeoff && command == MAV_CMD_NAV_TAKEOFF) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
        if (noResponseFromVehicle) {
            _say(QStringLiteral("Vehicle did not respond to takeoff command"));
            _updateFlightMachineActive(false);
        } else if (result != MAV_RESULT_ACCEPTED) {
            _say(QStringLiteral("takeoff command failed"));
            _updateFlightMachineActive(false);
        }
    } else if (currentState.command == CommandSetHeading && command == MAV_CMD_DO_REPOSITION) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
        if (noResponseFromVehicle) {
            _say(QStringLiteral("Vehicle did not response to Rotate command. Flight cancelled. Vehicle returning."));
            _resetStateAndRTL();
        } else if (result != MAV_RESULT_ACCEPTED) {
            _say(QStringLiteral("Rotate command failed. Flight cancelled. Vehicle returning."));
            _resetStateAndRTL();
        }
    }
}

void VHFTrackerQGCPlugin::_takeoff(Vehicle* vehicle, double takeoffAltRel)
{
    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->showMessage(tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    double takeoffAltAMSL = takeoffAltRel + vehicleAltitudeAMSL;

    _sendCommandAndVerify(
                vehicle,
                MAV_CMD_NAV_TAKEOFF,
                -1,                             // No pitch requested
                0, 0,                           // param 2-4 unused
                qQNaN(), qQNaN(), qQNaN(),      // No yaw, lat, lon
                takeoffAltAMSL);                // AMSL altitude
}



void VHFTrackerQGCPlugin::_rotateVehicle(Vehicle* vehicle, double headingDegrees)
{
    _sendCommandAndVerify(
                vehicle,
                MAV_CMD_DO_REPOSITION,
                -1,                                     // no change in ground speed
                MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,    // switch to guided mode
                0,                                      //reserved
                qDegreesToRadians(headingDegrees),      // change heading
                qQNaN(), qQNaN(), qQNaN());             // no change lat, lon, alt
}

void VHFTrackerQGCPlugin::_nextVehicleState(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    if (_vehicleStateIndex != 0 && vehicle->flightMode() != "Takeoff" && vehicle->flightMode() != "Hold") {
        // User cancel
        _updateFlightMachineActive(false);
        return;
    }

    if (_vehicleStateIndex >= _vehicleStates.count()) {
        _say(QStringLiteral("Collection complete. returning."));
        _resetStateAndRTL();
        _detectComplete();
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    switch (currentState.command) {
    case CommandTakeoff:
        // Takeoff to specified altitude
        _say(QStringLiteral("Waiting for takeoff to %1 %2.").arg(FactMetaData::metersToAppSettingsDistanceUnits(currentState.targetValue).toDouble()).arg(FactMetaData::appSettingsDistanceUnitsString()));
        _takeoff(vehicle, currentState.targetValue);
        break;
    case CommandSetHeading:
        _say(QStringLiteral("Waiting for rotate to %1 degrees.").arg(qRound(currentState.targetValue)));
        _rotateVehicle(vehicle, currentState.targetValue);
        break;
    case CommandDelay:
        _say(QStringLiteral("Collecting data for %1 seconds.").arg(currentState.targetValueWaitSecs));
        _vehicleStateIndex++;
        _delayTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _delayTimer.start();
        break;
    }

    if (currentState.fact) {
        _targetValueTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _targetValueTimer.start();
        connect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
    }
}

void VHFTrackerQGCPlugin::_vehicleStateRawValueChanged(QVariant rawValue)
{
    if (!_flightMachineActive) {
        Fact* fact = dynamic_cast<Fact*>(sender());
        if (!fact) {
            qWarning() << "Dynamic cast failed!";
            return;
        }
        disconnect(fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    qCDebug(VHFTrackerQGCPluginLog) << "Waiting for value actual:wait:variance" << rawValue.toDouble() << currentState.targetValue << currentState.targetVariance;

    if (qAbs(rawValue.toDouble() - currentState.targetValue) < currentState.targetVariance) {
        _targetValueTimer.stop();
        disconnect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
        _vehicleStateIndex++;
        _nextVehicleState();
    }
}

void VHFTrackerQGCPlugin::_say(QString text)
{
    qCDebug(VHFTrackerQGCPluginLog) << text;
    _toolbox->audioOutput()->say(text.toLower());
}

void VHFTrackerQGCPlugin::_delayComplete(void)
{
    double maxPulse = 0;
    foreach(double pulse, _rgPulseValues) {
        maxPulse = qMax(maxPulse, pulse);
    }
    _rgPulseValues.clear();
    _rgAngleStrengths.append(maxPulse);
    _nextVehicleState();
}

void VHFTrackerQGCPlugin::_targetValueFailed(void)
{
    _say("Failed to reach target.");
    cancelAndReturn();
}

void VHFTrackerQGCPlugin::_detectComplete(void)
{
    // Determine strongest angle and convert to string from Qml

    double maxPulse = 0;
    int currentAngle = 0;
    _strongestAngle = 0;
    _rgStringAngleStrengths.clear();

    foreach(double pulse, _rgAngleStrengths) {
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

bool VHFTrackerQGCPlugin::_armVehicleAndValidate(Vehicle* vehicle)
{
    if (vehicle->armed()) {
        return true;
    }

    bool armedChanged = false;

    // We try arming 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setArmed(true);

        // Wait for vehicle to return armed state
        for (int i=0; i<10; i++) {
            if (vehicle->armed()) {
                armedChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (armedChanged) {
            break;
        }
    }

    if (!armedChanged) {
        _say("Vehicle failed to arm");
    }

    return armedChanged;
}

bool VHFTrackerQGCPlugin::_setRTLFlightModeAndValidate(Vehicle* vehicle)
{
    QString rtlFlightMode = vehicle->rtlFlightMode();

    if (vehicle->flightMode() == rtlFlightMode) {
        return true;
    }

    bool flightModeChanged = false;

    // We try 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setFlightMode(rtlFlightMode);

        // Wait for vehicle to return flight mode
        for (int i=0; i<10; i++) {
            if (vehicle->flightMode() == rtlFlightMode) {
                flightModeChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (flightModeChanged) {
            break;
        }
    }

    if (!flightModeChanged) {
        _say("Vehicle failed to respond to Return command");
    }

    return flightModeChanged;
}

void VHFTrackerQGCPlugin::_resetStateAndRTL(void)
{
    _delayTimer.stop();
    _targetValueTimer.stop();

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (vehicle) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
    }

    for (const VehicleState_t& vehicleState: _vehicleStates) {
        if (vehicleState.fact) {
            disconnect(vehicleState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
        }
    }

    _setRTLFlightModeAndValidate(vehicle);

    _updateFlightMachineActive(false);
}
