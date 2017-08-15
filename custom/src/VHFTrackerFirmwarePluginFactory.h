#pragma once

#include "FirmwarePlugin.h"

class PX4FirmwarePlugin;

class VHFTrackerFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT
public:
    VHFTrackerFirmwarePluginFactory(void);

    QList<MAV_AUTOPILOT>    supportedFirmwareTypes      (void) const final;
    FirmwarePlugin*         firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;
    QList<MAV_TYPE>         supportedVehicleTypes       (void) const final;

private:
    PX4FirmwarePlugin*   _pluginInstance;
};
