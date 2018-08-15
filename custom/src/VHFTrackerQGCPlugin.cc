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

VHFTrackerQGCPlugin::VHFTrackerQGCPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _strengthsAvailable   (false)
    , _beepStrength         (0)
    , _bpm                  (0)
    , _latitude             (0)
    , _longitude            (0)
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

    int divisions = _vhfSettings->divisions()->rawValue().toInt();

    double divisionIncrement = 100.0 / divisions * 2;
    double strength = 0;
    for (int i=0; i<8; i++) {
        _angleStrengths << QString("%1").arg((int)strength);
        strength += divisionIncrement;
    }
    strength -= divisionIncrement;
    for (int i=0; i<8; i++) {
        strength -= divisionIncrement;
        _angleStrengths << QString("%1").arg((int)(strength));
    }
    _strongestAngle = 7;
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

#if 0
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
#endif

bool VHFTrackerQGCPlugin::_handleDebug(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(link);

    mavlink_debug_t debugMsg;

    mavlink_msg_debug_decode(&message, &debugMsg);

    if (debugMsg.ind == 1) {
        _beepStrength = debugMsg.value;
        emit beepStrengthChanged(_beepStrength);

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
    } else if (debugMsg.ind == 0) {
#if 0
        qDebug() << "Strong";
        int strongHeading = debugMsg.time_boot_ms;
        int pulseStrength = debugMsg.value;
        QGeoCoordinate coordCenter = vehicle->coordinate();
        QGeoCoordinate coordAnimal = coordCenter.atDistanceAndAzimuth(1000,  strongHeading);

        _mapItems.append(new DirectionMapItem(coordCenter, strongHeading, pulseStrength, this));
        _mapItems.append(new LineMapItem(coordCenter, strongHeading, pulseStrength, this));

        QPair<QGeoCoordinate, QGeoCoordinate> linePair(coordCenter, coordAnimal);
        _rgStrongLines.append(linePair);
        if (_rgStrongLines.count() > 2) {
            _rgStrongLines.removeFirst();
        }

        if (_rgStrongLines.count() == 2) {
            double x, y, z;
            QPointF pointCenter, pointAnimal;
            QGeoCoordinate tangentOrigin = vehicle->coordinate();

            convertGeoToNed(_rgStrongLines[0].first, tangentOrigin, &x, &y, &z);
            pointCenter = QPointF(x, y);
            convertGeoToNed(_rgStrongLines[0].second, tangentOrigin, &x, &y, &z);
            pointAnimal = QPointF(x, y);
            QLineF line1(pointCenter, pointAnimal);

            convertGeoToNed(_rgStrongLines[1].first, tangentOrigin, &x, &y, &z);
            pointCenter = QPointF(x, y);
            convertGeoToNed(_rgStrongLines[1].second, tangentOrigin, &x, &y, &z);
            pointAnimal = QPointF(x, y);
            QLineF line2(pointCenter, pointAnimal);

            QPointF intersectPoint;
            line1.intersect(line2,  &intersectPoint);

            QGeoCoordinate intersectCoord;
            convertNedToGeo(intersectPoint.x(), intersectPoint.y(), 0, tangentOrigin, &intersectCoord);

            _latitude = intersectCoord.latitude();
            _longitude = intersectCoord.longitude();
            emit latitudeChanged(_latitude);
            emit longitudeChanged(_longitude);
        }
#endif
    }


    return false;
}

void VHFTrackerQGCPlugin::startDetection(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!activeVehicle) {
        return;
    }

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
        vehicleState.fact =             NULL;
        vehicleState.targetValue =      msecsWait;
        vehicleState.targetVariance =   0;
        _vehicleStates.append(vehicleState);
    }

    vehicleState.command =          MAV_CMD_NAV_RETURN_TO_LAUNCH;
    vehicleState.fact =             NULL;
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
        activeVehicle->guidedModeTakeoff(currentState.targetValue);
        break;
    case MAV_CMD_DO_REPOSITION:
        qDebug() << "rotate target" << currentState.targetValue;
        _rotateVehicle(activeVehicle, currentState.targetValue);
        break;
    case MAV_CMD_NAV_DELAY:
        _vehicleStateIndex++;
        QTimer::singleShot(currentState.targetValue, this, &VHFTrackerQGCPlugin::_nextVehicleState);
        break;
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
        qDebug() << "RTL";
        _vehicleStateIndex++;
        activeVehicle->setFlightMode(activeVehicle->rtlFlightMode());
        _strengthsAvailable = true;
        emit strengthsAvailableChanged(true);
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

    if (qAbs(rawValue.toDouble() - currentState.targetValue) < currentState.targetVariance) {
        disconnect(currentState.fact, &Fact::rawValueChanged, this, &VHFTrackerQGCPlugin::_vehicleStateRawValueChanged);
        _vehicleStateIndex++;
        if (_vehicleStateIndex < _vehicleStates.count()) {
            _nextVehicleState();
        }
    }
}
