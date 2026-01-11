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
            rebuildDataFromModel();
            // 1. Build nodes + links FROM MODEL
            // (for now, keep your dummy code or move it to a helper)
            // IMPORTANT: nodes and links must already be filled here

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
        QPen pen(link.color, 4.0, Qt::SolidLine, Qt::RoundCap);
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
