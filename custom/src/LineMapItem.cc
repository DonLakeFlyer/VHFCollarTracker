#include "LineMapItem.h"

#include <QColor>
#include <QDebug>

LineMapItem::LineMapItem(const QGeoCoordinate& coordinate, double heading, double signalStrength, QObject* parent)
    : QmlComponentInfo  (QString(), QUrl::fromUserInput("qrc:/qml/LineMapItem.qml"), QUrl(), parent)
    , _coordinate       (coordinate)
    , _heading          (heading)
    , _signalStrength   (signalStrength)
{

}

LineMapItem::~LineMapItem()
{

}
