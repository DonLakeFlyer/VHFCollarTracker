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
    width:              pageWidth
    spacing:            ScreenTools.defaultFontPixelHeight / 2

    property bool showSettingsIcon: false

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _margins:          ScreenTools.defaultFontPixelWidth
    property var _corePlugin:       QGroundControl.corePlugin
    property var _vhfSettings:      _corePlugin.vhfSettings

    GridLayout {
        anchors.left:   parent.left
        anchors.right:  parent.right
        columns:        2

        QGCLabel { text: qsTr("Alt") }
        FactTextField {
            Layout.fillWidth:   true
            fact:               _vhfSettings.altitude
        }
    }

    QGCButton {
        text:       qsTr("Start")
        enabled:    _activeVehicle
        onClicked:  _corePlugin.startDetection()
    }

     Item { width: 1; height: 1 }
}
