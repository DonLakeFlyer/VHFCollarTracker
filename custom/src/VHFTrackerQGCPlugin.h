#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"

class VHFTrackerQGCPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    VHFTrackerQGCPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~VHFTrackerQGCPlugin();

    Q_PROPERTY(int beepStrength MEMBER _beepStrength NOTIFY beepStrengthChanged)

    // Overrides from QGCCorePlugin
    QString             brandImageIndoor    (void) const final;
    QString             brandImageOutdoor   (void) const final;
    QVariantList&       settingsPages       (void) final;
    QVariantList&       instrumentPages     (void) final;
    bool                mavlinkMessage      (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QmlObjectListModel* customMapItems      (void) final { return &_mapItems; }

signals:
    void beepStrengthChanged(int beepStrength);

private:
    bool _handleMemoryVect(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);
    bool _handleDebug(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);

    QVariantList        _settingsPages;
    QVariantList        _instrumentPages;
    QmlObjectListModel  _mapItems;
    int                 _beepStrength;
};
