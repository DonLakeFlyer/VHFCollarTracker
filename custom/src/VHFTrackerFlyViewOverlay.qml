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
import QtCharts             2.2

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controllers       1.0
import QGroundControl.SettingsManager   1.0

Rectangle {
    id:             flyOverlay
    anchors.fill:   parent
    color:          qgcPal.window

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState: "topMode"

    property var    _corePlugin:        QGroundControl.corePlugin
    property var    _vhfSettings:       _corePlugin.vhfSettings
    property var    _divisions:         _vhfSettings.divisions.rawValue
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _leftMargin:        ScreenTools.defaultFontPixelWidth * 10
    property real   _rightMargin:       ScreenTools.defaultFontPixelWidth * 35
    property int    _pulseCount:        0
    property real   _pulseStrength:     0
    property int    _bpm:               _corePlugin.bpm
    property int    _strongestAngle:    _corePlugin.strongestAngle
    property bool   _takeoffStage2:     false
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property real   _altitude:          _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0

    readonly property real _sliceSize: 360 / _divisions

    DeadMouseArea { anchors.fill: parent }

    Connections {
        target: QGroundControl.corePlugin

        onBeepStrengthChanged: {
            _pulseCount++
            _pulseStrength = beepStrength
        }
    }

    RowLayout {
        anchors.margins:    _margins
        anchors.fill:       parent
        spacing:            _margins

        ColumnLayout {
            Layout.fillHeight:  true

            QGCLabel {
                id:     altLabel
                text:   "Altitude"
            }

            Rectangle {
                width:              altLabel.width
                Layout.fillHeight:  true
                border.width:       1
                border.color:       qgcPal.text

                Rectangle {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.bottom: parent.bottom
                    height:         parent.height * (_altitude / _vhfSettings.altitude.rawValue)
                    color:          qgcPal.text
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth:   true
            Layout.fillHeight:  true

            RowLayout {
                Layout.fillWidth: true

                Rectangle {
                    Layout.fillWidth:   true
                    height:             ScreenTools.defaultFontPixelHeight * 2
                    border.color:       "green"

                    Rectangle {
                        anchors.margins:        1
                        anchors.rightMargin:    _rightMargin
                        anchors.fill:           parent
                        color:                  "green"

                        property real   _maximumPulse:   _corePlugin.vhfSettings.maxPulse.rawValue
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

                ColumnLayout {
                    QGCLabel {
                        width:                      (ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio) * 4
                        text:                       _corePlugin.beepStrength.toFixed(2)
                        color:                      "black"
                        font.pointSize:             ScreenTools.largeFontPointSize
                    }

                    RowLayout {
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
            }

            Item {
                Layout.fillHeight:  true
                Layout.fillWidth:   true

                Item {
                    anchors.centerIn:   parent
                    height:             parent.height
                    width:              height

                    QGCLabel {
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        text:                       "N"
                        font.pointSize:             ScreenTools.largeFontPointSize
                    }

                    QGCLabel {
                        anchors.bottom:             parent.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        text:                       "S"
                        font.pointSize:             ScreenTools.largeFontPointSize
                    }

                    QGCLabel {
                        anchors.left:               parent.left
                        anchors.verticalCenter:     parent.verticalCenter
                        text:                       "E"
                        font.pointSize:             ScreenTools.largeFontPointSize
                    }

                    QGCLabel {
                        anchors.right:          parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text:                   "W"
                        font.pointSize:         ScreenTools.largeFontPointSize
                    }

                    Rectangle {
                        anchors.margins:    (ScreenTools.defaultFontPixelHeight * ScreenTools.largeFontPointRatio) + 10
                        anchors.fill:       parent
                        border.color:       "black"
                        radius:             width / 2

                        Repeater {
                            model: _divisions

                            Canvas {
                                id:             arcCanvas
                                anchors.fill:   parent
                                visible:        !isNaN(strengthRatio)

                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();

                                    ctx.beginPath();
                                    ctx.fillStyle = "black";
                                    ctx.strokeStyle = "red";
                                    ctx.moveTo(centerX, centerY);
                                    ctx.arc(centerX, centerY, (width / 2) * arcCanvas.strengthRatio, 0, arcRadians, false);
                                    ctx.lineTo(centerX, centerY);
                                    ctx.fill();
                                    ctx.stroke();
                                }

                                transform: Rotation {
                                    origin.x:   arcCanvas.centerX
                                    origin.y:   arcCanvas.centerY
                                    angle:      -90 - (360 / _divisions / 2) + ((360 / _divisions) * index)
                                }

                                property real centerX:      width / 2
                                property real centerY:      height / 2
                                property real arcRadians:   (Math.PI * 2) / _divisions
                                property real strengthRatio:    _corePlugin.angleRatios[index]

                                Connections {
                                    target:                 _corePlugin
                                    onAngleRatiosChanged:   arcCanvas.requestPaint()
                                }
                            }
                        }
                    }

                    Image {
                        id:                 vehicleIcon
                        anchors.centerIn:   parent
                        width:              _uavSize
                        source:             "/qmlimages/vehicleArrowOpaque.svg"
                        mipmap:             true
                        sourceSize.width:   _uavSize
                        fillMode:           Image.PreserveAspectFit
                        visible:            _activeVehicle

                        transform: Rotation {
                            origin.x:       vehicleIcon.width  / 2
                            origin.y:       vehicleIcon.height / 2
                            angle:          isNaN(_heading) ? 0 : _heading

                            property real   _heading:   _activeVehicle ? _activeVehicle.heading.rawValue : 0
                        }

                        property real   _uavSize:   ScreenTools.defaultFontPixelHeight * 5
                    }
                }
            }
        }

        Column {
            width:      ScreenTools.defaultFontPixelWidth * 25
            spacing:    _margins

            QGCLabel {
                text:           "Freq: " + (_corePlugin.vehicleFrequency / 1000).toFixed(0)
                font.pointSize: ScreenTools.largeFontPointSize
            }

            QGCButton {
                id:         setFreqButton
                width:      parent.width
                text:       qsTr("Set Frequency")
                enabled:    !_corePlugin.flightMachineActive
                onClicked:  visible = false
            }

            RowLayout {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth / 2
                visible:    !setFreqButton.visible

                FactTextField {
                    Layout.fillWidth:   true
                    fact:               _corePlugin.vhfSettings.frequency
                }

                QGCButton {
                    text: qsTr("Set")
                    onClicked: {
                        setFreqButton.visible = true
                        _corePlugin.setFrequency(_corePlugin.vhfSettings.frequency.rawValue)
                    }
                }
            }

            QGCButton {
                width:      parent.width
                text:       qsTr("Takeoff")
                visible:    !_takeoffStage2
                enabled:    !_corePlugin.flightMachineActive
                onClicked:  _takeoffStage2 = true
            }

            RowLayout {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth / 2
                visible:    _takeoffStage2

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Really Takeoff")
                    onClicked: {
                        _takeoffStage2 = false
                        _corePlugin.start()
                    }
                }

                QGCButton {
                    text:       qsTr("Cancel")
                    onClicked:  _takeoffStage2 = false
                }
            }

            QGCButton {
                width:      parent.width
                text:       qsTr("Cancel And Return")
                enabled:    _corePlugin.flightMachineActive
                onClicked:  _corePlugin.cancelAndReturn()
            }

            QGCButton {
                width:      parent.width
                text:       "Close"
                onClicked:  flyOverlay.visible = false
            }
        }
    }
}
