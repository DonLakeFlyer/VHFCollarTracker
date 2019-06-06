#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "SettingsFact.h"
#include "VHFTrackerQGCOptions.h"
#include "QGCLoggingCategory.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QTimer>

class VHFTrackerSettings;

Q_DECLARE_LOGGING_CATEGORY(VHFTrackerQGCPluginLog)

class VHFTrackerQGCPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    VHFTrackerQGCPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~VHFTrackerQGCPlugin();

    Q_PROPERTY(VHFTrackerSettings*  vhfSettings         MEMBER _vhfSettings             CONSTANT)
    Q_PROPERTY(float                beepStrength        MEMBER _beepStrength            NOTIFY beepStrengthChanged)
    Q_PROPERTY(int                  bpm                 MEMBER _bpm                     NOTIFY bpmChanged)
    Q_PROPERTY(QStringList          angleStrengths      MEMBER _rgStringAngleStrengths  NOTIFY angleStrengthsChanged)
    Q_PROPERTY(int                  strongestAngle      MEMBER _strongestAngle          NOTIFY strongestAngleChanged)
    Q_PROPERTY(bool                 strengthsAvailable  MEMBER _strengthsAvailable      NOTIFY strengthsAvailableChanged)
    Q_PROPERTY(bool                 flightMachineActive MEMBER _flightMachineActive     NOTIFY flightMachineActiveChanged)

    Q_INVOKABLE void takeoff            (void);
    Q_INVOKABLE void cancelAndReturn    (void);

    // Overrides from QGCCorePlugin
    QString             brandImageIndoor    (void) const final;
    QString             brandImageOutdoor   (void) const final;
    QVariantList&       settingsPages       (void) final;
    QVariantList&       instrumentPages     (void) final;
    bool                mavlinkMessage      (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QGCOptions*         options             (void) final { return qobject_cast<QGCOptions*>(_vhfQGCOptions); }

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void beepStrengthChanged        (double beepStrength);
    void bpmChanged                 (int bpm);
    void angleStrengthsChanged      (void);
    void strongestAngleChanged      (int strongestAngle);
    void strengthsAvailableChanged  (bool strengthsAvailable);
    void flightMachineActiveChanged (bool flightMachineActive);

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _nextVehicleState              (void);
    void _detectComplete                (void);
    void _delayComplete                 (void);
    void _targetValueFailed             (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);

private:
    typedef enum {
        CommandTakeoff,
        CommandSetHeading,
        CommandDelay
    } VehicleStateCommand_t;

    typedef struct {
        VehicleStateCommand_t   command;
        Fact*                   fact;
        int                     targetValueWaitSecs;
        double                  targetValue;
        double                  targetVariance;
    } VehicleState_t;

    bool _handleDebugVect               (Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);
    void _rotateVehicle                 (Vehicle* vehicle, double headingDegrees);
    void _say                           (QString text);
    bool _armVehicleAndValidate         (Vehicle* vehicle);
    bool _setRTLFlightModeAndValidate   (Vehicle* vehicle);
    void _sendCommandAndVerify          (Vehicle* vehicle, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    void _takeoff                       (Vehicle* vehicle, double takeoffAltRel);
    void _resetStateAndRTL              (void);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<double>           _rgPulseValues;
    QList<double>           _rgAngleStrengths;
    QStringList             _rgStringAngleStrengths;
    int                     _strongestAngle;
    bool                    _strengthsAvailable;
    bool                    _flightMachineActive;

    double                  _beepStrength;
    int                     _bpm;
    QElapsedTimer           _elapsedTimer;
    QTimer                  _delayTimer;
    QTimer                  _targetValueTimer;
    VHFTrackerQGCOptions*   _vhfQGCOptions;
    VHFTrackerSettings*     _vhfSettings;    
};
