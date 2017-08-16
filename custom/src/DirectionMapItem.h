#pragma once

#include "QmlComponentInfo.h"

#include <QVariantList>

class DirectionMapItem : public QmlComponentInfo
{
    Q_OBJECT

    Q_PROPERTY(QVariantList signalStrength READ signalStrength NOTIFY signalStrengthChanged)

public:
    DirectionMapItem(QObject* parent = NULL);
    ~DirectionMapItem();

    QVariantList signalStrength(void) const;

    void setSignalStrengthColors(QList<QColor> colors);

signals:
    void signalStrengthChanged(void);

private:
    QVariantList _signalStrength;
};
