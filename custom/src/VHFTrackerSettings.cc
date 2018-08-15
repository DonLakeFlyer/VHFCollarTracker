/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VHFTrackerSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>
#include <QStandardPaths>

const char* VHFTrackerSettings::_settingsGroup =        "VHFTracker";
const char* VHFTrackerSettings::_altitudeFactName =     "Altitude";
const char* VHFTrackerSettings::_divisionsFactName =    "Divisions";
const char* VHFTrackerSettings::_frequencyFactName =    "frequency";
const char* VHFTrackerSettings::_gainFactName =         "gain";

VHFTrackerSettings::VHFTrackerSettings(QObject* parent)
    : SettingsGroup     (_settingsGroup, _settingsGroup, parent)
    , _altitudeFact     (nullptr)
    , _divisionsFact    (nullptr)
    , _frequencyFact    (nullptr)
    , _gainFact         (nullptr)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<VHFTrackerSettings>("QGroundControl.SettingsManager", 1, 0, "VHFTrackerSettings", "Reference only");
}

Fact* VHFTrackerSettings::altitude(void)
{
    if (!_altitudeFact) {
        _altitudeFact = _createSettingsFact(_altitudeFactName);
    }

    return _altitudeFact;
}

Fact* VHFTrackerSettings::divisions(void)
{
    if (!_divisionsFact) {
        _divisionsFact = _createSettingsFact(_divisionsFactName);
    }

    return _divisionsFact;
}

Fact* VHFTrackerSettings::frequency(void)
{
    if (!_frequencyFact) {
        _frequencyFact = _createSettingsFact(_frequencyFactName);
    }

    return _frequencyFact;
}

Fact* VHFTrackerSettings::gain(void)
{
    if (!_gainFact) {
        _gainFact = _createSettingsFact(_gainFactName);
    }

    return _gainFact;
}
