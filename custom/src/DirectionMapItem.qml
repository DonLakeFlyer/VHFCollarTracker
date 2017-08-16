import QtQuick      2.3
import QtLocation   5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

MapQuickItem {
    coordinate:     QGroundControl.multiVehicleManager.activeVehicle.coordinate
    anchorPoint.x:  0
    anchorPoint.y:  0

    property var customMapObject

    property real _roseRadius: ScreenTools.defaultFontPixelWidth * 10

    sourceItem: Item {
        Repeater {
            model: customMapObject.signalStrength

            Item {
                width:              0
                height:             0
                transformOrigin:    Item.Bottom
                rotation:           (360.0 / 16.0) * index

                Rectangle {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    y:                          -_roseRadius - _radius
                    width:                      _radius * 2
                    height:                     _radius * 2
                    radius:                     _radius
                    color:                      modelData
                    border.color:               "white"
                    border.width:               1

                    property real _radius: ScreenTools.defaultFontPixelWidth
                }
            }
        }
    }
}
