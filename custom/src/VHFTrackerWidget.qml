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
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

/// VHF Tracker page for Instrument Panel PageView
Column {
    anchors.margins:    _margins
    anchors.top:        parent.top
    anchors.left:       parent.left
    width:              pageWidth - _margins
    spacing:            ScreenTools.defaultFontPixelHeight / 2
    enabled:            _activeVehicle

    property bool showSettingsIcon: false

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _margins:          ScreenTools.defaultFontPixelWidth
    property var _corePlugin:       QGroundControl.corePlugin
    property var _vhfSettings:      _corePlugin.vhfSettings

    GridLayout {
        anchors.left:   parent.left
        anchors.right:  parent.right
        columns:        2

        QGCLabel { text: qsTr("Freq") }
        FactTextField {
            Layout.fillWidth:   true
            fact:               _vhfSettings.frequency
        }

        QGCLabel { text: qsTr("Alt") }
        FactTextField {
            Layout.fillWidth:   true
            fact:               _vhfSettings.altitude
        }

        QGCLabel { text: qsTr("Max Pulse") }
        FactTextField {
            Layout.fillWidth:   true
            fact:               _vhfSettings.maxPulse
        }

        QGCLabel { text: qsTr("Rot Divisions") }
        QGCComboBox {
            id:                 divisionsCombo
            Layout.fillWidth:   true
            model:              [ 4, 8, 16 ]

            Component.onCompleted: {
                var divisions = _vhfSettings.divisions.rawValue
                if (divisions == 4) {
                    divisionsCombo.currentIndex = 0
                } else if (divisions == 8) {
                    divisionsCombo.currentIndex = 1
                } else if (divisions == 16) {
                    divisionsCombo.currentIndex = 2
                } else {
                    _vhfSettings.divisions.rawValue = 16
                    divisionsCombo.currentIndex = 2
                }
            }


            onActivated: _vhfSettings.divisions.rawValue = model[index]
        }
    }

    QGCButton {
        text:       qsTr("Start")
        onClicked:  _corePlugin.startDetection()
    }

    QGCButton {
        text:       qsTr("Calibrate Max Pulse")
        onClicked:  _corePlugin.calibrateMaxPulse()
    }

     Item { width: 1; height: 1 }
}
