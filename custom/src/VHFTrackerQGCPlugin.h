#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "SettingsFact.h"
#include "VHFTrackerQGCOptions.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>

class VHFTrackerSettings;

class VHFTrackerQGCPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    VHFTrackerQGCPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~VHFTrackerQGCPlugin();

    Q_PROPERTY(VHFTrackerSettings*  vhfSettings     MEMBER _vhfSettings     CONSTANT)
    Q_PROPERTY(int                  beepStrength    MEMBER _beepStrength    NOTIFY beepStrengthChanged)
    Q_PROPERTY(int                  bpm             MEMBER _bpm             NOTIFY bpmChanged)
    Q_PROPERTY(double               latitude        MEMBER _latitude        NOTIFY latitudeChanged)
    Q_PROPERTY(double               longitude       MEMBER _longitude       NOTIFY longitudeChanged)

    // Overrides from QGCCorePlugin
    QString             brandImageIndoor    (void) const final;
    QString             brandImageOutdoor   (void) const final;
    QVariantList&       settingsPages       (void) final;
    QVariantList&       instrumentPages     (void) final;
    bool                mavlinkMessage      (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QmlObjectListModel* customMapItems      (void) final { return &_mapItems; }
    QGCOptions*         options             (void) final { return qobject_cast<QGCOptions*>(_vhfQGCOptions); }

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void beepStrengthChanged(int beepStrength);
    void bpmChanged         (int bpm);
    void latitudeChanged    (double latitude);
    void longitudeChanged   (double longitude);

private:
    //bool _handleMemoryVect  (Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);
    bool _handleDebug       (Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    QmlObjectListModel      _mapItems;
    int                     _beepStrength;
    int                     _bpm;
    double                  _latitude;
    double                  _longitude;
    QElapsedTimer           _elapsedTimer;
    VHFTrackerQGCOptions*   _vhfQGCOptions;
    VHFTrackerSettings*     _vhfSettings;
    QList<QPair<QGeoCoordinate, QGeoCoordinate>> _rgStrongLines;
};
