#include "DirectionMapItem.h"

#include <QColor>
#include <QDebug>

DirectionMapItem::DirectionMapItem(const QGeoCoordinate& coordinate, const QList<QColor>& signalStrengthColors, QObject* parent)
    : QmlComponentInfo  (QString(), QUrl::fromUserInput("qrc:/qml/DirectionMapItem.qml"), QUrl(), parent)
    , _coordinate       (coordinate)
{
    foreach (QColor color, signalStrengthColors) {
        _signalStrength.append(QVariant::fromValue(color));
    }
}

DirectionMapItem::~DirectionMapItem()
{

}
