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

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// VHF Tracker page for Instrument Panel PageView
Column {
    width:      pageWidth
    spacing:    ScreenTools.defaultFontPixelHeight

    property bool showSettingsIcon: true

    function showSettings() {
        qgcView.showDialog(settingsDialog, qsTr("Settings"), qgcView.showDialogDefaultWidth, StandardButton.Ok)
    }

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCButton {
        anchors.horizontalCenter:   parent.horizontalCenter
        text:                       qsTr("Detect Animal")
        onClicked:                  _activeVehicle.sendCommand(150 /* MAV_COMP_ID_PERIPHERAL */, 31010 /* MAV_CMD_USER_1 */, true)
        enabled:                    _activeVehicle
    }

    Component {
        id: settingsDialog

        QGCViewDialog {
        }
    }
}
