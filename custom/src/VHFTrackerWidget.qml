/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.2
import QtQuick.Dialogs  1.2
import QtQuick.Controls 1.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// VHF Tracker page for Instrument Panel PageView
Column {
    anchors.margins:    _margins
    anchors.top:        parent.top
    anchors.left:       parent.left
    width:              pageWidth
    spacing:            ScreenTools.defaultFontPixelHeight / 2

    property bool showSettingsIcon: false

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _margins:          ScreenTools.defaultFontPixelWidth

    QGCButton {
        text:       qsTr("Detect Animal")
        enabled:    _activeVehicle
        onClicked:  _activeVehicle.sendCommand(158 /* MAV_COMP_ID_PERIPHERAL */, 31010 /* MAV_CMD_USER_1 */, true)
    }

    QGCButton {
        text:       qsTr("Cancel")
        enabled:    _activeVehicle
        onClicked:  _activeVehicle.sendCommand(158 /* MAV_COMP_ID_PERIPHERAL */, 31011 /* MAV_CMD_USER_2 */, true)
    }

    RowLayout {
        spacing: _margins

        QGCTextField {
            id:                     freqNumerator
            anchors.baseline:       decimalPoint.baseline
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 6
            inputMethodHints:       Qt.ImhFormattedNumbersOnly
            validator:              IntValidator { bottom: 1 }
        }

        QGCLabel {
            id:     decimalPoint
            text:   "."
        }

        QGCTextField {
            id:                     freqDenominator
            anchors.baseline:       decimalPoint.baseline
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 6
            inputMethodHints:       Qt.ImhFormattedNumbersOnly
            validator:              IntValidator { bottom: 1 }
        }
    }


    QGCButton {
        text: qsTr("Set Freq")
        enabled:    _activeVehicle
        onClicked:  _activeVehicle.sendCommand(158,     // MAV_COMP_ID_PERIPHERAL
                                               31012,   // MAV_CMD_USER_3
                                               true,    // showError
                                               parseInt(freqNumerator.text),
                                               parseInt(freqDenominator.text))
    }

    QGCTextField {
        id:                 gainField
        inputMethodHints:   Qt.ImhFormattedNumbersOnly
        validator:          IntValidator { bottom: 0 }
    }

    QGCButton {
        text:       qsTr("Set Gain")
        enabled:    _activeVehicle
        onClicked:  _activeVehicle.sendCommand(158,     // MAV_COMP_ID_PERIPHERAL
                                               31013,   // MAV_CMD_USER_4
                                               true,    // showError
                                               parseInt(gainField.text))
    }
}
