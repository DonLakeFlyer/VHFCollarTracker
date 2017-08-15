#include "DirectionMapItem.h"

#include <QColor>
#include <QDebug>

DirectionMapItem::DirectionMapItem(QObject* parent)
    : QmlComponentInfo(QString(), QUrl::fromUserInput("qrc:/qml/DirectionMapItem.qml"), QUrl(), parent)
{

}

DirectionMapItem::~DirectionMapItem()
{

}

void DirectionMapItem::setSignalStrengthColors(QList<QColor> colors)
{
    _signalStrength.clear();
    foreach (QColor color, colors) {
        _signalStrength.append(QVariant::fromValue(color));
    }
    emit signalStrengthChanged();
}

QVariantList DirectionMapItem::signalStrength(void) const
{
    return _signalStrength;
}
