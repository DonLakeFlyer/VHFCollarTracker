#pragma once

#include "QGCCorePlugin.h"

class VHFTrackerQGCPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    VHFTrackerQGCPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~VHFTrackerQGCPlugin();

    // Overrides from QGCCorePlugin
    QString         brandImageIndoor    (void) const final;
    QString         brandImageOutdoor   (void) const final;
    QVariantList&   settingsPages       (void) final;
    QVariantList&   instrumentPages     (void) final;
    bool            mavlinkMessage      (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;

private:
    bool _handleMemoryVect(Vehicle* vehicle, LinkInterface* link, mavlink_message_t& message);

    QVariantList    _settingsPages;
    QVariantList    _instrumentPages;
};
