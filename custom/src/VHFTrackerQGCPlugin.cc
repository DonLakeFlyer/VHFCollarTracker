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

// DEBUG_FLOAT_ARRAY
//  time_usec - send index (used to detect lost telemetry)
//  array_id - command id

#define COMMAND_ID_ACK                          1   // Ack response to command
#define COMMAND_ID_TAG                          2   // Tag info
#define COMMAND_ID_START_DETECTION              3   // Start pulse detection
#define COMMAND_ID_STOP_DETECTION               4   // Stop pulse detection
#define COMMAND_ID_PULSE                        5   // Detected pulse value

#define ACK_IDX_COMMAND                         0   // Command being acked
#define ACK_IDX_RESULT                          1   // Command result - 1 success, 0 failure

#define PULSE_IDX_DETECTION_STATUS              0   // Detection status (uint 32)
#define PULSE_IDX_STRENGTH                      1   // Pulse strength [0-100] (float)
#define PULSE_IDX_GROUP_INDEX                   2   // Group index 0/1/2 (uint 32)

#define PULSE_DETECTION_STATUS_SUPER_THRESHOLD  1
#define PULSE_DETECTION_STATUS_CONFIRMED        2

#define TAG_IDX_ID                              0   // Tag id (uint 32)
#define TAG_IDX_FREQUENCY                       1   // Frequency - 6 digits shifted by three decimals, NNNNNN means NNN.NNN000 Mhz (uint 32)
#define TAG_IDX_DURATION_MSECS                  2   // Pulse duration
#define TAG_IDX_INTRA_PULSE1_MSECS              3   // Intra-pulse duration 1
#define TAG_IDX_INTRA_PULSE2_MSECS              4   // Intra-pulse duration 2
#define TAG_IDX_INTRA_PULSE_UNCERTAINTY         5   // Intra-pulse uncertainty
#define TAG_IDX_INTRA_PULSE_JITTER              6   // Intra-pulse jitter
#define TAG_IDX_MAX_PULSE                       7   // Max pulse value

#define START_DETECTION_IDX_TAG_ID              0   // Tag to start detection on

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
    , _simulate             (true)
    , _vehicleFrequency     (0)
    , _lastPulseSendIndex   (-1)
    , _missedPulseCount     (0)
{
    _showAdvancedUI = false;

    _delayTimer.setSingleShot(true);
    _targetValueTimer.setSingleShot(true);
    _vhfCommandAckTimer.setSingleShot(true);
    _vhfCommandAckTimer.setInterval(1000);

    connect(&_delayTimer,           &QTimer::timeout, this, &VHFTrackerQGCPlugin::_delayComplete);
    connect(&_targetValueTimer,     &QTimer::timeout, this, &VHFTrackerQGCPlugin::_targetValueFailed);
    connect(&_simPulseTimer,        &QTimer::timeout, this, &VHFTrackerQGCPlugin::_simulatePulse);
    connect(&_vhfCommandAckTimer,      &QTimer::timeout, this, &VHFTrackerQGCPlugin::_vhfCommandAckFailed);
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

bool VHFTrackerQGCPlugin::mavlinkMessage(Vehicle*, LinkInterface*, mavlink_message_t message)
{
    if (message.msgid == MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY) {
        mavlink_debug_float_array_t debug_float_array;

        mavlink_msg_debug_float_array_decode(&message, &debug_float_array);

        uint32_t messageId = static_cast<uint32_t>(debug_float_array.array_id);

        switch (messageId) {
        case COMMAND_ID_ACK:
            _handleVHFCommandAck(debug_float_array);
            break;
        case COMMAND_ID_PULSE:
            _handleVHFPulse(debug_float_array);
            break;
        }

        return false;
    } else {
        return true;
    }
}

void VHFTrackerQGCPlugin::_handleVHFCommandAck(const mavlink_debug_float_array_t& debug_float_array)
{
    uint32_t vhfCommand = static_cast<uint32_t>(debug_float_array.data[ACK_IDX_COMMAND]);
    uint32_t result     = static_cast<uint32_t>(debug_float_array.data[ACK_IDX_RESULT]);

    if (vhfCommand == _vhfCommandAckExpected) {
        _vhfCommandAckExpected = 0;
        _vhfCommandAckTimer.stop();
        qCDebug(VHFTrackerQGCPluginLog) << "VHF command ack received - command:result" << _vhfCommandIdToText(vhfCommand) << result;
        if (result == 1) {
            if (vhfCommand == COMMAND_ID_START_DETECTION) {
                _startFlight();
            }
        } else {
            _say(QStringLiteral("%1 command failed").arg(_vhfCommandIdToText(vhfCommand)));
        }
    } else {
        qWarning() << "_handleVHFCommandAck: Received unexpected ack expected:actual" << _vhfCommandAckExpected << vhfCommand;
    }
}

void VHFTrackerQGCPlugin::_handleVHFPulse(const mavlink_debug_float_array_t& debug_float_array)
{
    double pulseStrength = static_cast<double>(debug_float_array.data[PULSE_IDX_STRENGTH]);

    if (pulseStrength < 0 || pulseStrength > 100) {
        qCDebug(VHFTrackerQGCPluginLog) << "PULSE outside range";
    }
    qCDebug(VHFTrackerQGCPluginLog) << "PULSE strength" << pulseStrength;
    pulseStrength = qMax(0.0, qMin(100.0, pulseStrength));

    _beepStrength = pulseStrength;
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

void VHFTrackerQGCPlugin::_startFlight(void)
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

void VHFTrackerQGCPlugin::start(void)
{
    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    LinkInterface*              priorityLink        = vehicle->priorityLink();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();

    memset(&debug_float_array, 0, sizeof(debug_float_array));

    debug_float_array.array_id                              = COMMAND_ID_START_DETECTION;
    debug_float_array.data[START_DETECTION_IDX_TAG_ID]      = _vhfSettings->tagId()->rawValue().toFloat();

    mavlink_msg_debug_float_array_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                priorityLink->mavlinkChannel(),
                &msg,
                &debug_float_array);
    _sendVHFCommand(vehicle, priorityLink, COMMAND_ID_START_DETECTION, msg);
}

void VHFTrackerQGCPlugin::_sendCommandAndVerify(Vehicle* vehicle, MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    connect(vehicle, &Vehicle::mavCommandResult, this, &VHFTrackerQGCPlugin::_mavCommandResult);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
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

    //qCDebug(VHFTrackerQGCPluginLog) << "Waiting for value actual:wait:variance" << rawValue.toDouble() << currentState.targetValue << currentState.targetVariance;

    if (qAbs(rawValue.toDouble() - currentState.targetValue) <= currentState.targetVariance) {
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

    _strongestPulsePct = maxPulse;
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
    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    LinkInterface*              priorityLink        = vehicle->priorityLink();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();

    memset(&debug_float_array, 0, sizeof(debug_float_array));

    debug_float_array.array_id = COMMAND_ID_STOP_DETECTION;

    mavlink_msg_debug_float_array_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                priorityLink->mavlinkChannel(),
                &msg,
                &debug_float_array);
    _sendVHFCommand(vehicle, priorityLink, COMMAND_ID_STOP_DETECTION, msg);
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
        double pulse = 100.0 * pulseRatio;

        double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
        double maxAlt = _vhfSettings->altitude()->rawValue().toDouble();
        double altRatio = currentAltRel / maxAlt;
        pulse *= altRatio;

        //qDebug() << heading << pulseRatio << pulse;

        mavlink_message_t           msg;
        mavlink_debug_float_array_t debug_float_array;

        debug_float_array.array_id = COMMAND_ID_PULSE;
        debug_float_array.data[PULSE_IDX_STRENGTH] = pulse;
        mavlink_msg_debug_float_array_encode(static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &msg, &debug_float_array);

        mavlinkMessage(vehicle, vehicle->priorityLink(), msg);
    }
}

void VHFTrackerQGCPlugin::_handleSimulatedTagCommand(const mavlink_debug_float_array_t& debug_float_array)
{
    _simulatorTagId                 = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_ID]);
    _simulatorFrequency             = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_FREQUENCY]);
    _simulatorPulseDuration         = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_DURATION_MSECS]);
    _simulatorIntraPulse1           = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_INTRA_PULSE1_MSECS]);
    _simulatorIntraPulse2           = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_INTRA_PULSE2_MSECS]);
    _simulatorIntraPulseUncertainty = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_INTRA_PULSE_UNCERTAINTY]);
    _simulatorIntraPulseJitter      = static_cast<uint32_t>(debug_float_array.data[TAG_IDX_INTRA_PULSE_JITTER]);
    _simulatorMaxPulse              = debug_float_array.data[TAG_IDX_MAX_PULSE];
}

void VHFTrackerQGCPlugin::_handleSimulatedStartDetection(const mavlink_debug_float_array_t&)
{
    _simPulseTimer.start(2000);
}

void VHFTrackerQGCPlugin::_handleSimulatedStopDetection(const mavlink_debug_float_array_t&)
{
    _simPulseTimer.stop();
}

QString VHFTrackerQGCPlugin::_vhfCommandIdToText(uint32_t vhfCommandId)
{
    switch (vhfCommandId) {
    case COMMAND_ID_TAG:
        return QStringLiteral("tag");
    case COMMAND_ID_START_DETECTION:
        return QStringLiteral("start detection");
    case COMMAND_ID_STOP_DETECTION:
        return QStringLiteral("stop detection");
    default:
        return QStringLiteral("unknown command");
    }
}


void VHFTrackerQGCPlugin::_vhfCommandAckFailed(void)
{
    _say(QStringLiteral("%1 failed. no response from vehicle.").arg(_vhfCommandIdToText(_vhfCommandAckExpected)));
    _vhfCommandAckExpected = 0;
}

void VHFTrackerQGCPlugin::_sendSimulatedVHFCommandAck(uint32_t vhfCommandId)
{
    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    LinkInterface*              priorityLink        = vehicle->priorityLink();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();

    memset(&debug_float_array, 0, sizeof(debug_float_array));

    debug_float_array.array_id              = COMMAND_ID_ACK;
    debug_float_array.data[ACK_IDX_COMMAND] = vhfCommandId;
    debug_float_array.data[ACK_IDX_RESULT]  = 1;

    mavlink_msg_debug_float_array_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                priorityLink->mavlinkChannel(),
                &msg,
                &debug_float_array);

    mavlinkMessage(vehicle, priorityLink, msg);
}

void VHFTrackerQGCPlugin::_sendVHFCommand(Vehicle* vehicle, LinkInterface* link, uint32_t vhfCommandId, const mavlink_message_t& msg)
{
    _vhfCommandAckExpected = vhfCommandId;
    _vhfCommandAckTimer.start();

    if (_simulate) {
        mavlink_debug_float_array_t debug_float_array;

        mavlink_msg_debug_float_array_decode(&msg, &debug_float_array);

        switch (debug_float_array.array_id) {
        case COMMAND_ID_TAG:
            _handleSimulatedTagCommand(debug_float_array);
            _sendSimulatedVHFCommandAck(COMMAND_ID_TAG);
            break;
        case COMMAND_ID_START_DETECTION:
            _handleSimulatedStartDetection(debug_float_array);
            _sendSimulatedVHFCommandAck(COMMAND_ID_START_DETECTION);
            break;
        case COMMAND_ID_STOP_DETECTION:
            _handleSimulatedStopDetection(debug_float_array);
            _sendSimulatedVHFCommandAck(COMMAND_ID_STOP_DETECTION);
            break;
        default:
            qWarning() << "Internal error: Unknown command id" << debug_float_array.array_id;
        }
    } else {
        vehicle->sendMessageOnLink(link, msg);
    }
}

void VHFTrackerQGCPlugin::sendTag(void)
{
    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    LinkInterface*              priorityLink        = vehicle->priorityLink();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();

    memset(&debug_float_array, 0, sizeof(debug_float_array));

    debug_float_array.array_id                              = COMMAND_ID_TAG;
    debug_float_array.data[TAG_IDX_ID]                      = _vhfSettings->tagId()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_FREQUENCY]               = _vhfSettings->frequency()->rawValue().toFloat() -  _vhfSettings->frequencyDelta()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_DURATION_MSECS]          = _vhfSettings->pulseDuration()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_INTRA_PULSE1_MSECS]      = _vhfSettings->intraPulse1()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_INTRA_PULSE2_MSECS]      = _vhfSettings->intraPulse2()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_INTRA_PULSE_UNCERTAINTY] = _vhfSettings->intraPulseUncertainty()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_INTRA_PULSE_JITTER]      = _vhfSettings->intraPulseJitter()->rawValue().toFloat();
    debug_float_array.data[TAG_IDX_MAX_PULSE]               = _vhfSettings->maxPulse()->rawValue().toFloat();

    mavlink_msg_debug_float_array_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                priorityLink->mavlinkChannel(),
                &msg,
                &debug_float_array);
    _sendVHFCommand(vehicle, priorityLink, COMMAND_ID_TAG, msg);
}
