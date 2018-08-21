#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "SettingsFact.h"
#include "VHFTrackerQGCOptions.h"
#include "QGCLoggingCategory.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>

class VHFTrackerSettings;

Q_DECLARE_LOGGING_CATEGORY(VHFTrackerQGCPluginLog)

class VHFTrackerQGCPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    VHFTrackerQGCPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~VHFTrackerQGCPlugin();

    Q_PROPERTY(VHFTrackerSettings*  vhfSettings         MEMBER _vhfSettings             CONSTANT)
    Q_PROPERTY(int                  beepStrength        MEMBER _beepStrength            NOTIFY beepStrengthChanged)
    Q_PROPERTY(int                  bpm                 MEMBER _bpm                     NOTIFY bpmChanged)
    Q_PROPERTY(QStringList          angleStrengths      MEMBER _rgStringAngleStrengths  NOTIFY angleStrengthsChanged)
    Q_PROPERTY(int                  strongestAngle      MEMBER _strongestAngle          NOTIFY strongestAngleChanged)
    Q_PROPERTY(bool                 strengthsAvailable  MEMBER _strengthsAvailable      NOTIFY strengthsAvailableChanged)

    Q_INVOKABLE void takeoff            (void);
    Q_INVOKABLE void startDetection     (void);
    Q_INVOKABLE void stopDetection     (void);
    Q_INVOKABLE void calibrateMaxPulse  (void);

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
    void beepStrengthChanged        (int beepStrength);
    void bpmChanged                 (int bpm);
    void angleStrengthsChanged      (void);
    void strongestAngleChanged      (int strongestAngle);
    void strengthsAvailableChanged  (bool strengthsAvailable);

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _nextVehicleState              (void);
    void _detectComplete                (void);
    void _singleCaptureComplete         (void);

private:
    typedef struct {
        MAV_CMD command;
        Fact*   fact;
        double  targetValue;
        double  targetVariance;
    } VehicleState_t;

    bool _handleDebug   (Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);
    void _rotateVehicle (Vehicle* vehicle, double headingDegrees);
    void _say           (QString text);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<int>              _rgPulseValues;
    QList<int>              _rgAngleStrengths;
    QStringList             _rgStringAngleStrengths;
    int                     _strongestAngle;
    bool                    _strengthsAvailable;

    int                     _beepStrength;
    int                     _bpm;
    QElapsedTimer           _elapsedTimer;
    VHFTrackerQGCOptions*   _vhfQGCOptions;
    VHFTrackerSettings*     _vhfSettings;    
};
