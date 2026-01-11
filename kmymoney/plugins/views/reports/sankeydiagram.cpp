#include "sankeydiagram.h"
#include "sankeyroles.h"
#include "sankeytypes.h"

#include "sankeylayoutengine.h"

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>

#include <KChartPaintContext>

////////////////////////////////////////////////////////////
// Private implementation
////////////////////////////////////////////////////////////

class SankeyDiagramPrivate
{
public:
    explicit SankeyDiagramPrivate(SankeyDiagram* qq)
        : q(qq)
    {
    }

    void connectModel(QAbstractItemModel* model)
    {
        if (!model)
            return;

        QObject::connect(model, &QAbstractItemModel::modelReset, q, &SankeyDiagram::updateLayout);
        QObject::connect(model, &QAbstractItemModel::dataChanged, q, &SankeyDiagram::updateLayout);
        QObject::connect(model, &QAbstractItemModel::rowsInserted, q, &SankeyDiagram::updateLayout);
        QObject::connect(model, &QAbstractItemModel::rowsRemoved, q, &SankeyDiagram::updateLayout);
    }

    void clear()
    {
        nodes.clear();
        links.clear();
    }

    // Cached layout data
    QVector<SankeyNode> nodes;
    QVector<SankeyLink> links;

    QSizeF lastSize;
    bool layoutDirty = true;

    SankeyDiagram* q;
    SankeyLayoutEngine layoutEngine;
};

////////////////////////////////////////////////////////////
// SankeyDiagram implementation
////////////////////////////////////////////////////////////

SankeyDiagram::SankeyDiagram(QWidget* parent)
    : KChart::AbstractDiagram(parent)
    , d_ptr(new SankeyDiagramPrivate(this))
{
}

SankeyDiagram::~SankeyDiagram()
{
    delete d_ptr;
}

void SankeyDiagram::setModel(QAbstractItemModel* model)
{
    Q_D(SankeyDiagram);

    if (model == this->model())
        return;

    d->clear();
    d->layoutDirty = true;

    KChart::AbstractDiagram::setModel(model);
    d->connectModel(model);

    update();
}

void SankeyDiagram::resize(const QSizeF& size)
{
    Q_D(SankeyDiagram);

    if (d->lastSize != size) {
        d->lastSize = size;
        d->layoutDirty = true;
    }

    KChart::AbstractDiagram::resize(size);
}

void SankeyDiagram::updateLayout()
{
    Q_D(SankeyDiagram);

    d->layoutDirty = true;
    update();
}

void SankeyDiagram::paint(KChart::PaintContext* paintContext)
{
    Q_D(SankeyDiagram);

    if (!model())
        return;

    if (d->layoutDirty) {
        const QRectF rect = paintContext->rectangle();

        if (d->layoutDirty) {
            d->nodes.clear();
            d->links.clear();

            rebuildDataFromModel(); // nodes + links (values only)
            computeNodeRects(paintContext->rectangle());
            computeLinkPaths(); // <-- THIS IS WHERE THE FIX GOES

            // 2. Run the layout engine
            d->layoutEngine.setNodes(&d->nodes);
            d->layoutEngine.setLinks(&d->links);
            d->layoutEngine.setRect(paintContext->rectangle());
            d->layoutEngine.layout();

            d->layoutDirty = false;
        }
    }

    QPainter* painter = paintContext->painter();
    painter->save();

    // Paint links first (none yet)
    for (const SankeyLink& link : d->links) {
        QPen pen(link.color);
        pen.setWidthF(link.thickness);
        pen.setCapStyle(Qt::FlatCap);
        painter->setPen(pen);
        painter->drawPath(link.path);
    }

    // Paint nodes
    painter->setPen(Qt::NoPen);
    for (const SankeyNode& node : d->nodes) {
        painter->setBrush(node.color);
        painter->drawRoundedRect(node.rect, 4, 4);
    }

    painter->restore();
}

const QPair<QPointF, QPointF> SankeyDiagram::calculateDataBoundaries() const
{
    // Sankey diagrams are not data-coordinate based.
    // We return a dummy, stable rectangle.
    return qMakePair(QPointF(0.0, 0.0), QPointF(1.0, 1.0));
}

void SankeyDiagram::rebuildDataFromModel()
{
    Q_D(SankeyDiagram);

    d->nodes.clear();
    d->links.clear();

    if (!model())
        return;

    // --- build nodes ---
    // For the test app we know node count = max(source,target)+1
    int maxNode = -1;
    for (int r = 0; r < model()->rowCount(); ++r) {
        const QModelIndex idx = model()->index(r, 0);
        maxNode = qMax(maxNode, idx.data(SankeyRoles::SourceRole).toInt());
        maxNode = qMax(maxNode, idx.data(SankeyRoles::TargetRole).toInt());
    }

    d->nodes.resize(maxNode + 1);
    for (int i = 0; i <= maxNode; ++i) {
        d->nodes[i].label = QStringLiteral("Node %1").arg(i);
        d->nodes[i].color = QColor::fromHsv((i * 40) % 360, 160, 200);
    }

    // --- build links ---
    for (int r = 0; r < model()->rowCount(); ++r) {
        const QModelIndex idx = model()->index(r, 0);

        SankeyLink link;
        link.source = idx.data(SankeyRoles::SourceRole).toInt();
        link.target = idx.data(SankeyRoles::TargetRole).toInt();
        link.value = idx.data(SankeyRoles::ValueRole).toDouble();
        link.color = d->nodes[link.source].color;

        d->links.append(link);
    }
}

void SankeyDiagram::computeLinkPaths()
{
    Q_D(SankeyDiagram);

    const int nodeCount = d->nodes.size();
    if (nodeCount == 0)
        return;

    QVector<qreal> outOffset(nodeCount, 0.0);
    QVector<qreal> inOffset(nodeCount, 0.0);

    // Compute total flow per node (outgoing)
    QVector<qreal> nodeOut(nodeCount, 0.0);
    for (const SankeyLink& link : qAsConst(d->links)) {
        if (link.source >= 0)
            nodeOut[link.source] += link.value;
    }

    // Build paths
    for (SankeyLink& link : d->links) {
        if (link.source < 0 || link.target < 0)
            continue;

        const SankeyNode& src = d->nodes[link.source];
        const SankeyNode& dst = d->nodes[link.target];

        const qreal scale = nodeOut[link.source] > 0 ? src.rect.height() / nodeOut[link.source] : 0;

        link.thickness = link.value * scale;

        const QPointF start(src.rect.right(), src.rect.top() + outOffset[link.source] + link.thickness / 2);

        const QPointF end(dst.rect.left(), dst.rect.top() + inOffset[link.target] + link.thickness / 2);

        const qreal dx = (end.x() - start.x()) * 0.5;

        QPainterPath path(start);
        path.cubicTo(start + QPointF(dx, 0), end - QPointF(dx, 0), end);

        link.path = path;

        outOffset[link.source] += link.thickness;
        inOffset[link.target] += link.thickness;
    }
}

void SankeyDiagram::computeNodeRects(const QRectF& area)
{
    Q_D(SankeyDiagram);

    const int n = d->nodes.size();
    if (n == 0)
        return;

    // 1. Compute total flow per node (max of in/out)
    QVector<qreal> inFlow(n, 0.0);
    QVector<qreal> outFlow(n, 0.0);

    for (const SankeyLink& link : qAsConst(d->links)) {
        if (link.source >= 0 && link.source < n)
            outFlow[link.source] += link.value;
        if (link.target >= 0 && link.target < n)
            inFlow[link.target] += link.value;
    }

    QVector<qreal> nodeValue(n, 0.0);
    qreal totalValue = 0.0;

    for (int i = 0; i < n; ++i) {
        nodeValue[i] = qMax(inFlow[i], outFlow[i]);
        totalValue += nodeValue[i];
    }

    if (totalValue <= 0)
        return;

    // 2. Vertical layout parameters
    const qreal spacing = 8.0;
    const qreal usableHeight = area.height() - spacing * (n - 1);

    qreal y = area.top();

    const qreal nodeWidth = 80.0; // temporary fixed width

    // 3. Assign rectangles
    for (int i = 0; i < n; ++i) {
        const qreal h = usableHeight * (nodeValue[i] / totalValue);

        d->nodes[i].rect = QRectF(area.left() + 20, // left margin
                                  y,
                                  nodeWidth,
                                  qMax<qreal>(h, 6.0) // minimum height
        );

        y += h + spacing;
    }
}
