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

const char* VHFTrackerSettings::_settingsGroup =                    "VHFTracker";
const char* VHFTrackerSettings::_altitudeFactName =                 "Altitude";
const char* VHFTrackerSettings::_divisionsFactName =                "Divisions";
const char* VHFTrackerSettings::_tagIdFactName =                    "TagId";
const char* VHFTrackerSettings::_frequencyFactName =                "Frequency";
const char* VHFTrackerSettings::_frequencyDeltaFactName =           "FrequencyDelta";
const char* VHFTrackerSettings::_pulseDurationFactName =            "PulseDuration";
const char* VHFTrackerSettings::_intraPulse1FactName =              "IntraPulse1";
const char* VHFTrackerSettings::_intraPulse2FactName =              "IntraPulse2";
const char* VHFTrackerSettings::_intraPulseUncertaintyFactName =    "IntraPulseUncertainty";
const char* VHFTrackerSettings::_intraPulseJitterFactName =         "IntraPulseJitter";
const char* VHFTrackerSettings::_maxPulseFactName =                 "MaxPulse";
const char* VHFTrackerSettings::_gainFactName =                     "gain";

VHFTrackerSettings::VHFTrackerSettings(QObject* parent)
    : SettingsGroup             (_settingsGroup, _settingsGroup, parent)
    , _altitudeFact             (nullptr)
    , _divisionsFact            (nullptr)
    , _tagIdFact                (nullptr)
    , _frequencyFact            (nullptr)
    , _frequencyDeltaFact       (nullptr)
    , _pulseDurationFact        (nullptr)
    , _intraPulse1Fact          (nullptr)
    , _intraPulse2Fact          (nullptr)
    , _intraPulseUncertaintyFact(nullptr)
    , _intraPulseJitterFact     (nullptr)
    , _maxPulseFact             (nullptr)
    , _gainFact                 (nullptr)
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

Fact* VHFTrackerSettings::tagId(void)
{
    if (!_tagIdFact) {
        _tagIdFact = _createSettingsFact(_tagIdFactName);
    }

    return _tagIdFact;
}

Fact* VHFTrackerSettings::frequency(void)
{
    if (!_frequencyFact) {
        _frequencyFact = _createSettingsFact(_frequencyFactName);
    }

    return _frequencyFact;
}

Fact* VHFTrackerSettings::frequencyDelta(void)
{
    if (!_frequencyDeltaFact) {
        _frequencyDeltaFact = _createSettingsFact(_frequencyDeltaFactName);
    }

    return _frequencyDeltaFact;
}

Fact* VHFTrackerSettings::pulseDuration(void)
{
    if (!_pulseDurationFact) {
        _pulseDurationFact = _createSettingsFact(_pulseDurationFactName);
    }

    return _pulseDurationFact;
}

Fact* VHFTrackerSettings::intraPulse1(void)
{
    if (!_intraPulse1Fact) {
        _intraPulse1Fact = _createSettingsFact(_intraPulse1FactName);
    }

    return _intraPulse1Fact;
}

Fact* VHFTrackerSettings::intraPulse2(void)
{
    if (!_intraPulse2Fact) {
        _intraPulse2Fact = _createSettingsFact(_intraPulse2FactName);
    }

    return _intraPulse2Fact;
}

Fact* VHFTrackerSettings::intraPulseUncertainty(void)
{
    if (!_intraPulseUncertaintyFact) {
        _intraPulseUncertaintyFact = _createSettingsFact(_intraPulseUncertaintyFactName);
    }

    return _intraPulseUncertaintyFact;
}

Fact* VHFTrackerSettings::intraPulseJitter(void)
{
    if (!_intraPulseJitterFact) {
        _intraPulseJitterFact = _createSettingsFact(_intraPulseJitterFactName);
    }

    return _intraPulseJitterFact;
}

Fact* VHFTrackerSettings::maxPulse(void)
{
    if (!_maxPulseFact) {
        _maxPulseFact = _createSettingsFact(_maxPulseFactName);
    }

    return _maxPulseFact;
}

Fact* VHFTrackerSettings::gain(void)
{
    if (!_gainFact) {
        _gainFact = _createSettingsFact(_gainFactName);
    }

    return _gainFact;
}
