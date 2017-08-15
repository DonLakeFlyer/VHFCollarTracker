#include "VHFTrackerFirmwarePluginFactory.h"
#include "VHFTrackerFirmwarePlugin.h"

VHFTrackerFirmwarePluginFactory VHFTrackerFirmwarePluginFactoryImp;

VHFTrackerFirmwarePluginFactory::VHFTrackerFirmwarePluginFactory(void)
    : _pluginInstance(NULL)
{
}

QList<MAV_AUTOPILOT> VHFTrackerFirmwarePluginFactory::supportedFirmwareTypes(void) const
{
    QList<MAV_AUTOPILOT> list;
    list.append(MAV_AUTOPILOT_PX4);
    return list;
}

FirmwarePlugin* VHFTrackerFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new VHFTrackerFirmwarePlugin;
        }
        return _pluginInstance;
    }
    return NULL;
}

QList<MAV_TYPE> VHFTrackerFirmwarePluginFactory::supportedVehicleTypes(void) const
{
    QList<MAV_TYPE> mavTypes;
    mavTypes.append(MAV_TYPE_QUADROTOR);
    return mavTypes;
}
