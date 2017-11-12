import QtQuick      2.3
import QtLocation   5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0

MapPolyline {
    line.width: 3
    line.color: "green"
    path:       [ _coordinate1, _coordinate2 ]

    property var customMapObject

    property var _coordinate1: customMapObject.coordinate
    property var _coordinate2: _coordinate1.atDistanceAndAzimuth(1000,  customMapObject.heading)
}
