#ifndef SANKEYTYPES_H
#define SANKEYTYPES_H

#include <QColor>
#include <QPainterPath>
#include <QRectF>
#include <QString>

struct SankeyNode {
    QString label;
    QRectF rect;
    QColor color;
};

struct SankeyLink {
    int source = -1;
    int target = -1;
    qreal value = 0.0;
    QPainterPath path;
    QColor color;
};

#endif // SANKEYTYPES_H
