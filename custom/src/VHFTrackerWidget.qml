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
    width:      pageWidth
    spacing:    ScreenTools.defaultFontPixelHeight

    property bool showSettingsIcon: true

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property int _beepCount:        0
    property int _beepStrength:     0

    function showSettings() {
        qgcView.showDialog(settingsDialog, qsTr("Settings"), qgcView.showDialogDefaultWidth, StandardButton.Ok)
    }

    Connections {
        target: QGroundControl.corePlugin

        onBeepStrengthChanged: {
            _beepCount++
            _beepStrength = beepStrength
        }
    }

    QGCButton {
        anchors.horizontalCenter:   parent.horizontalCenter
        text:                       qsTr("Detect Animal")
        onClicked:                  _activeVehicle.sendCommand(158 /* MAV_COMP_ID_PERIPHERAL */, 31010 /* MAV_CMD_USER_1 */, true)
        enabled:                    _activeVehicle
    }

    QGCButton {
        anchors.horizontalCenter:   parent.horizontalCenter
        text:                       qsTr("Cancel    ")
        onClicked:                  _activeVehicle.sendCommand(158 /* MAV_COMP_ID_PERIPHERAL */, 31011 /* MAV_CMD_USER_2 */, true)
        enabled:                    _activeVehicle
    }

    ProgressBar {
        anchors.left:   parent.left
        anchors.right:  parent.right
        minimumValue:   0
        maximumValue:   500
        value:          _beepStrength

        onValueChanged: beepResetTimer.start()

        Behavior on value {
            PropertyAnimation {
                duration:       250
                easing.type:    Easing.InOutQuad
            }
        }

        Timer {
            id:             beepResetTimer
            interval:       500
            repeat:         false
            onTriggered:    _beepStrength = 0
        }
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel { text: "Beep" }
        QGCLabel {
            font.pointSize: ScreenTools.largeFontPointSize
            text: QGroundControl.corePlugin.beepStrength
        }
    }


    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel { text: "Count" }
        QGCLabel {
            font.pointSize: ScreenTools.largeFontPointSize
            text: _beepCount
        }
    }

    Component {
        id: settingsDialog

        QGCViewDialog {
        }
    }
}
