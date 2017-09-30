/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "VHFTrackerQGCOptions.h"
#include "VHFTrackerQGCOptions.h"

VHFTrackerQGCOptions::VHFTrackerQGCOptions(VHFTrackerQGCPlugin* plugin, QObject* parent)
    : QGCOptions    (parent)
    , _vhfQGCPlugin (plugin)
{

}
