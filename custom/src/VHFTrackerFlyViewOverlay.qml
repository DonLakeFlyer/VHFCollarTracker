/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Layouts      1.2
import QtQuick.Controls     1.2
import QtPositioning        5.2

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controllers       1.0
import QGroundControl.SettingsManager   1.0

Item {
    id:             flyOVerlay
    anchors.fill:   parent

    Rectangle {
        anchors.topMargin:      _margins
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 10
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 30
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          parent.right
        height:                 ScreenTools.defaultFontPixelHeight * 2
        color:                  "white"

        QGCPalette { id: qgcPal; colorGroupEnabled: true }

        readonly property string scaleState: "topMode"

        property var    _corePlugin:    QGroundControl.corePlugin
        property real   _margins:       ScreenTools.defaultFontPixelWidth

        QGCLabel {
            id:                     pulseText
            anchors.rightMargin:    _margins
            anchors.verticalCenter: parent.verticalCenter
            anchors.right:          parent.right
            horizontalAlignment:    Text.AlignHCenter
            width:                  (ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio) * 4
            text:                   _corePlugin.beepStrength
            color:                  "black"
            font.pointSize:         ScreenTools.largeFontPointSize
        }

        Rectangle {
            anchors.margins:        1
            anchors.rightMargin:    _margins
            anchors.left:           parent.left
            anchors.right:          pulseText.left
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            color:                  "green"
        }

        QGCLabel {

        }
    }
}
