/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class VHFTrackerSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    VHFTrackerSettings(QObject* parent = nullptr);

    Q_PROPERTY(Fact* altitude       READ altitude       CONSTANT)
    Q_PROPERTY(Fact* divisions      READ divisions      CONSTANT)
    Q_PROPERTY(Fact* frequency      READ frequency      CONSTANT)
    Q_PROPERTY(Fact* frequencyDelta READ frequencyDelta CONSTANT)
    Q_PROPERTY(Fact* maxPulse       READ maxPulse       CONSTANT)
    Q_PROPERTY(Fact* gain           READ gain           CONSTANT)

    Fact* altitude      (void);
    Fact* divisions     (void);
    Fact* frequency     (void);
    Fact* frequencyDelta(void);
    Fact* maxPulse      (void);
    Fact* gain          (void);

private:
    SettingsFact* _altitudeFact;
    SettingsFact* _divisionsFact;
    SettingsFact* _frequencyFact;
    SettingsFact* _frequencyDeltaFact;
    SettingsFact* _maxPulseFact;
    SettingsFact* _gainFact;

    static const char* _settingsGroup;
    static const char* _altitudeFactName;
    static const char* _divisionsFactName;
    static const char* _frequencyFactName;
    static const char* _frequencyDeltaFactName;
    static const char* _maxPulseFactName;
    static const char* _gainFactName;
};
