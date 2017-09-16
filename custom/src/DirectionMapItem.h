#pragma once

#include "QmlComponentInfo.h"

#include <QVariantList>
#include <QGeoCoordinate>

class DirectionMapItem : public QmlComponentInfo
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate   coordinate      MEMBER _coordinate      CONSTANT)
    Q_PROPERTY(QVariantList     signalStrength  MEMBER _signalStrength  CONSTANT)

public:
    DirectionMapItem(const QGeoCoordinate& coordinate, const QList<QColor>& signalStrengthColors, QObject* parent = NULL);
    ~DirectionMapItem();

private:
    QGeoCoordinate  _coordinate;
    QVariantList    _signalStrength;
};
