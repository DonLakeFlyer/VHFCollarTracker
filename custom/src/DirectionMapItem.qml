import QtQuick      2.3
import QtLocation   5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

MapQuickItem {
    coordinate:     customMapObject.coordinate
    anchorPoint.x:  sourceItem.width / 2
    anchorPoint.y:  sourceItem.height / 2

    property var customMapObject

    property real _roseRadius: ScreenTools.defaultFontPixelWidth * 10

    sourceItem: Rectangle {
        width:  _radius * 2
        height: _radius * 2
        radius: _radius
        color:  "white"

        property real _radius: ScreenTools.defaultFontPixelWidth
    }
}
