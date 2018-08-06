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

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState: "topMode"

    property var    _corePlugin:        QGroundControl.corePlugin
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property int    _pulseCount:        0
    property int    _pulseStrength:     0
    property int    _bpm:               _corePlugin.bpm

    Connections {
        target: QGroundControl.corePlugin

        onBeepStrengthChanged: {
            _pulseCount++
            _pulseStrength = beepStrength
        }
    }

    Rectangle {
        anchors.topMargin:      _margins
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 10
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 30
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          parent.right
        height:                 valueColumn.y +  valueColumn.height + _margins
        color:                  "white"

        Column {
            id:                 valueColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.right:      parent.right

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      (ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio) * 4
                horizontalAlignment:        Text.AlignHCenter
                text:                       _corePlugin.beepStrength
                color:                      "black"
                font.pointSize:             ScreenTools.largeFontPointSize
            }

            Row {
                spacing: _margins

                QGCLabel {
                    text:                       _bpm
                    color:                      "black"
                    font.pointSize:             ScreenTools.largeFontPointSize
                }

                QGCLabel {
                    text:                       _pulseCount
                    color:                      "black"
                }
            }
        }

        Rectangle {
            anchors.margins:        _margins
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.right:          valueColumn.left
            height:                 ScreenTools.defaultFontPixelHeight * 2
            border.color:           "green"

            Rectangle {
                anchors.margins:        1
                anchors.rightMargin:    _rightMargin
                anchors.fill:           parent
                color:                  "green"

                property real   _maximumPulse:   78
                property real   _value:         _pulseStrength
                property real   _rightMargin:   (parent.width - 2) - ((parent.width - 2) * (Math.min(_pulseStrength, _maximumPulse) / _maximumPulse))

                on_ValueChanged: pulseResetTimer.start()

                Behavior on _value {
                    PropertyAnimation {
                        duration:       250
                        easing.type:    Easing.InOutQuad
                    }
                }

                Timer {
                    id:             pulseResetTimer
                    interval:       500
                    repeat:         false
                    onTriggered:    _pulseStrength = 0
                }
            }
        }
    }
}
