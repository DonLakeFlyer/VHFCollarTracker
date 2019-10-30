#include "VHFTrackerQGCPlugin.h"
#include "DirectionMapItem.h"
#include "LineMapItem.h"
#include "Vehicle.h"
#include "VHFTrackerSettings.h"
#include "QGCGeo.h"
#include "QGCApplication.h"
#include "AppSettings.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>

// Mavlink DEBUG_VECT messages are used to communicate with QGC in both directions.
// 	DEBUG_VECT.name is used to hold a command type
//	DEBUG_VECT.x/y/z are then command specific

// We can't store the full 9 digit frequency values in a DEBUG_VECT.* value without
// running into floating point precision problems changing the integer value to an incorrect
// value. So all frequency values sent in DEBUG_VECT are only 6 digits with the last three
// assumed to be 0: NNN.NNN000 mHz
#define FREQ_DIVIDER 1000

        // Pulse value
//	DEBUG_VECT.name = "PULSE"
//	DEBUG_VECT.x = pulse value
//	DEBUG_VECT.y - frequency
//	DEBUG_VECT.z - pulse send index
#define DEBUG_COMMAND_ID_PULSE "PULSE"

// Set gain
//	DEBUG_VECT.name = "GAIN"
//	DEBUG_VECT.x - new gain
#define DEBUG_COMMAND_ID_SET_GAIN "SET-GAIN"

// Set frequency
//	DEBUG_VECT.name = "FREQ"
//	DEBUG_VECT.x - new frequency
#define DEBUG_COMMAND_ID_SET_FREQ "SET-FREQ"

// Ack for SET commands
//	DEBUG_VECT.name = "CMD-ACK"
//	DEBUG_VECT.x - command being acked (DEBUG_COMMAND_ACK_SET_*)
//	DEBUG_VECT.y - gain/freq value which was changed to
#define DEBUG_COMMAND_ID_ACK "CMD-ACK"
static const int DEBUG_COMMAND_ACK_SET_GAIN =	0;
static const int DEBUG_COMMAND_ACK_SET_FREQ =	1;

QGC_LOGGING_CATEGORY(VHFTrackerQGCPluginLog, "VHFTrackerQGCPluginLog")

VHFTrackerQGCPlugin::VHFTrackerQGCPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _strongestAngle       (0)
    , _strongestPulsePct    (0)
    , _flightMachineActive  (false)
    , _beepStrength         (0)
    , _temp                 (0)
    , _bpm                  (0)
    , _simulate             (false)
    , _vehicleFrequency     (0)
    , _lastPulseSendIndex   (-1)
    , _missedPulseCount     (0)
{
    _showAdvancedUI = false;

    _delayTimer.setSingleShot(true);
    _targetValueTimer.setSingleShot(true);
    _freqChangeAckTimer.setSingleShot(true);
    _freqChangeAckTimer.setInterval(1000);
    _freqChangePulseTimer.setSingleShot(true);
    _freqChangePulseTimer.setInterval(8000);

    if (_simulate) {
        _simPulseTimer.start(2000);
    }

    connect(&_delayTimer,           &QTimer::timeout, this, &VHFTrackerQGCPlugin::_delayComplete);
    connect(&_targetValueTimer,     &QTimer::timeout, this, &VHFTrackerQGCPlugin::_targetValueFailed);
    connect(&_simPulseTimer,        &QTimer::timeout, this, &VHFTrackerQGCPlugin::_simulatePulse);
    connect(&_freqChangeAckTimer,   &QTimer::timeout, this, &VHFTrackerQGCPlugin::_freqChangeAckFailed);
    connect(&_freqChangePulseTimer, &QTimer::timeout, this, &VHFTrackerQGCPlugin::_freqChangePulseFailed);
}

VHFTrackerQGCPlugin::~VHFTrackerQGCPlugin()
{

}

void VHFTrackerQGCPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    _vhfSettings = new VHFTrackerSettings(nullptr);
    _vhfQGCOptions = new VHFTrackerQGCOptions(this, nullptr);

    int subDivisions = _vhfSettings->divisions()->rawValue().toInt();
    _rgAngleStrengths.clear();
    _rgAngleRatios.clear();
    for (int i=0; i<subDivisions; i++) {
        _rgAngleStrengths.append(qQNaN());
        _rgAngleRatios.append(QVariant::fromValue(qQNaN()));
    }

    //_simPulseTimer.start(2000);
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

bool VHFTrackerQGCPlugin::_handleDebugVect(Vehicle* /* vehicle */, LinkInterface* /* link */, mavlink_message_t& message)
{
    mavlink_debug_vect_t debugVect;

    mavlink_msg_debug_vect_decode(&message, &debugVect);

    char command[MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN + 1];
    memset(command, 0, sizeof(command));
    mavlink_msg_debug_vect_get_name(&message, command);
    QString commandId(command);

    if (commandId == DEBUG_COMMAND_ID_PULSE) {
        if (debugVect.time_usec == 0) {
            // double send
            return false;
        }

        double rawPulseStrength = static_cast<double>(debugVect.x);
        int pulseSendIndex = static_cast<int>(debugVect.z);
        int rawVehicleFrequency = static_cast<int>(debugVect.y);
        qCDebug(VHFTrackerQGCPluginLog) << "PULSE strength:sendIndex:freq" << rawPulseStrength << pulseSendIndex << rawVehicleFrequency;

        if (_lastPulseSendIndex != -1 && (pulseSendIndex != _lastPulseSendIndex + 1)) {
            _missedPulseCount++;
            emit missedPulseCountChanged(_missedPulseCount);
        }
        _lastPulseSendIndex = pulseSendIndex;

        double maxPulse = _vhfSettings->maxPulse()->rawValue().toDouble();
        _beepStrength = qMin(rawPulseStrength, maxPulse) / maxPulse;
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

        int currentVehicleFrequency = rawVehicleFrequency + _vhfSettings->frequencyDelta()->rawValue().toInt();
        if (currentVehicleFrequency != _vehicleFrequency) {
            _vehicleFrequency = currentVehicleFrequency;
            emit vehicleFrequencyChanged(_vehicleFrequency);
        }

        int requestedFrequency = _vhfSettings->frequency()->rawValue().toInt();
        if (_vehicleFrequency == requestedFrequency) {
            _freqChangePulseTimer.stop();
        } else {
            if (!_freqChangeAckTimer.isActive() && !_freqChangePulseTimer.isActive()) {
                setFrequency(requestedFrequency);
            }
        }

#if 0
        if (!qFuzzyCompare(static_cast<qreal>(debugVect.z), _temp)) {
            _temp = static_cast<qreal>(debugVect.z);
            emit tempChanged(_temp);
        }
#endif
    } else if (commandId == DEBUG_COMMAND_ID_ACK) {
        int ackCommand =    static_cast<int>(debugVect.x);
        int ackValue =      static_cast<int>(debugVect.y);
        if (ackCommand == DEBUG_COMMAND_ACK_SET_FREQ) {
            int freq = ackValue + _vhfSettings->frequencyDelta()->rawValue().toInt();
            qDebug() << ackValue << freq;
            int numerator = freq / 1000;
            int denominator = freq - (numerator * 1000);
            int digit1 = denominator / 100;
            int digit2 = (denominator - (digit1 * 100)) / 10;
            int digit3 = denominator - (digit1 * 100) - (digit2 * 10);
            _say(QStringLiteral("Frequency changed to %1 point %2 %3 %4").arg(numerator).arg(digit1).arg(digit2).arg(digit3));

            _freqChangeAckTimer.stop();
            _freqChangePulseTimer.start();
        } else if (ackCommand== DEBUG_COMMAND_ACK_SET_GAIN) {
            _say(QStringLiteral("Gain changed to %2").arg(ackValue));
        }
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


void VHFTrackerQGCPlugin::start(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    _updateFlightMachineActive(true);

    // Prime angle strengths with no values
    _cSlice = _vhfSettings->divisions()->rawValue().toInt();
    _rgAngleStrengths.clear();
    _rgAngleRatios.clear();
    for (int i=0; i<_cSlice; i++) {
        _rgAngleStrengths.append(qQNaN());
        _rgAngleRatios.append(QVariant::fromValue(qQNaN()));
    }
    emit angleRatiosChanged();

    _strongestAngle = _strongestPulsePct = 0;
    emit strongestAngleChanged(0);
    emit strongestPulsePctChanged(0);

    if (!_armVehicleAndValidate(vehicle)) {
        _resetStateAndRTL();
        return;
    }

    // First heading is adjusted to be at the center of the closest subdivision
    double vehicleHeading = vehicle->heading()->rawValue().toDouble();
    double divisionDegrees = 360.0 / _cSlice;
    _firstSlice = _nextSlice = static_cast<int>((vehicleHeading + (divisionDegrees / 2.0)) / divisionDegrees);
    _firstHeading = divisionDegrees * _firstSlice;

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
    double headingIncrement = 360.0 / _cSlice;
    double nextHeading = _firstHeading - headingIncrement;
    for (int i=0; i<_cSlice; i++) {
        nextHeading += headingIncrement;
        if (nextHeading >= 360) {
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
        vehicleState.targetValueWaitSecs =  10;
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
        _rgPulseValues.clear();
        _delayTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _delayTimer.start();
        break;
    }

    if (currentState.fact) {
        _targetValueTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _targetValueTimer.start();
        connect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
        _vehicleStateRawValueChanged(currentState.fact->rawValue());
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
    qCDebug(VHFTrackerQGCPluginLog) << "_say" << text;
    _toolbox->audioOutput()->say(text.toLower());
}

int VHFTrackerQGCPlugin::_rawPulseToPct(double rawPulse)
{
    double maxPossiblePulse = static_cast<double>(_vhfSettings->maxPulse()->rawValue().toDouble());
    return static_cast<int>(100.0 * (rawPulse / maxPossiblePulse));
}

void VHFTrackerQGCPlugin::_delayComplete(void)
{
    double maxPulse = 0;
    foreach(double pulse, _rgPulseValues) {
        maxPulse = qMax(maxPulse, pulse);
    }
    qCDebug(VHFTrackerQGCPluginLog) << "_delayComplete" << maxPulse << _rgPulseValues;
    _rgPulseValues.clear();
    _rgAngleStrengths[_nextSlice] = maxPulse;

    if (++_nextSlice >= _cSlice) {
        _nextSlice = 0;
    }

    maxPulse = 0;
    int strongestSlice = 0;
    for (int i=0; i<_cSlice; i++) {
        if (_rgAngleStrengths[i] > maxPulse) {
            maxPulse = _rgAngleStrengths[i];
            strongestSlice = i;
        }
    }

    _strongestPulsePct = maxPulse * 100;
    emit strongestPulsePctChanged(_strongestPulsePct);

    double sliceAngle = 360 / _vhfSettings->divisions()->rawValue().toDouble();
    _strongestAngle = static_cast<int>(strongestSlice * sliceAngle);
    emit strongestAngleChanged(_strongestAngle);

    for (int i=0; i<_cSlice; i++) {
        double angleStrength = _rgAngleStrengths[i];
        if (!qIsNaN(angleStrength)) {
            _rgAngleRatios[i] = _rgAngleStrengths[i] / maxPulse;
        }
    }
    emit angleRatiosChanged();

    _nextVehicleState();
}

void VHFTrackerQGCPlugin::_targetValueFailed(void)
{
    _say("Failed to reach target.");
    cancelAndReturn();
}

void VHFTrackerQGCPlugin::_detectComplete(void)
{

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

bool VHFTrackerQGCPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup && metaData.name() == AppSettings::batteryPercentRemainingAnnounceName) {
        metaData.setRawDefaultValue(20);
    }

    return true;
}

void VHFTrackerQGCPlugin::_simulatePulse(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (vehicle) {
        double heading = vehicle->heading()->rawValue().toDouble();

        // Strongest pulse is at 90 degrees
        heading -= 90;
        if (heading < 0) {
            heading += 360;
        }

        double pulseRatio;
        if (heading <= 180) {
            pulseRatio = (180.0 - heading) / 180.0;
        } else {
            heading -= 180;
            pulseRatio = heading / 180.0;
        }
        double pulse = 10.0 * pulseRatio;

        double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
        double maxAlt = _vhfSettings->altitude()->rawValue().toDouble();
        double altRatio = currentAltRel / maxAlt;
        pulse *= altRatio;

        //qDebug() << heading << pulseRatio << pulse;

        mavlink_debug_vect_t    debugVect;
        mavlink_message_t       msg;

        strcpy(debugVect.name, DEBUG_COMMAND_ID_PULSE);
        debugVect.x = static_cast<float>(pulse);
        debugVect.y = 146000000;
        debugVect.z = 60;
        mavlink_msg_debug_vect_encode(static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &msg, &debugVect);
        _handleDebugVect(vehicle, vehicle->priorityLink(), msg);
    }
}

void VHFTrackerQGCPlugin::_sendFreqChange(int frequency)
{
    if (_simulate) {
        return;
    }

    int adjustFrequency = frequency - _vhfSettings->frequencyDelta()->rawValue().toInt();
    qCDebug(VHFTrackerQGCPluginLog) << "Requesting frequency change to request:Adjusted" << frequency << adjustFrequency;

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (vehicle) {
        mavlink_message_t       msg;
        MAVLinkProtocol*        mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        LinkInterface*          priorityLink = vehicle->priorityLink();
        char                    name[MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN + 1];

        memset(&name, 0, sizeof(name));
        strcpy(name, DEBUG_COMMAND_ID_SET_FREQ);
        mavlink_msg_debug_vect_pack_chan(static_cast<uint8_t>(mavlink->getSystemId()),
                                         static_cast<uint8_t>(mavlink->getComponentId()),
                                         priorityLink->mavlinkChannel(),
                                         &msg,
                                         name,
                                         0,                                     // time_usec field unused
                                         adjustFrequency,
                                         0, 0);                                 // y,z - unusued
        vehicle->sendMessageOnLink(priorityLink, msg);

        _freqChangeAckTimer.start();
    }
}

void VHFTrackerQGCPlugin::setFrequency(int frequency)
{
    _sendFreqChange(frequency);
}

void VHFTrackerQGCPlugin::_freqChangeAckFailed(void)
{
    _say("Vehicle did not respond to frequency change request");
}


void VHFTrackerQGCPlugin::_freqChangePulseFailed(void)
{
    _say("Frequency coming from pulse data incorrect");
}
