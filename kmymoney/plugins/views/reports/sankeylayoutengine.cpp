// sankeylayoutengine.cpp
#include "sankeylayoutengine.h"

#include <QPainterPath>
#include <QtMath>

#include "sankeydiagram.h" // for SankeyNode
#include "sankeytypes.h" // SankeyLink

static constexpr qreal ColumnSpacing = 80.0;
static constexpr qreal NodeWidth = 18.0;
static constexpr qreal NodeSpacing = 10.0;
static constexpr qreal LinkCurvature = 0.5;

void SankeyLayoutEngine::setNodes(QVector<SankeyNode>* nodes)
{
    m_nodes = nodes;
}

void SankeyLayoutEngine::setLinks(QVector<SankeyLink>* links)
{
    m_links = links;
}

void SankeyLayoutEngine::setRect(const QRectF& rect)
{
    m_rect = rect;
}

void SankeyLayoutEngine::layout()
{
    if (!m_nodes || !m_links || m_nodes->isEmpty())
        return;

    computeNodeValues();
    assignColumns();
    computeNodeRects();
    computeLinkPaths();
}

void SankeyLayoutEngine::computeNodeValues()
{
    const int n = m_nodes->size();
    QVector<qreal> in(n, 0.0), out(n, 0.0);

    for (const SankeyLink& link : *m_links) {
        if (link.source >= 0 && link.source < n)
            out[link.source] += link.value;
        if (link.target >= 0 && link.target < n)
            in[link.target] += link.value;
    }

    m_nodeHeight.resize(n);
    for (int i = 0; i < n; ++i)
        m_nodeHeight[i] = qMax(in[i], out[i]);
}

void SankeyLayoutEngine::assignColumns()
{
    const int n = m_nodes->size();
    m_nodeColumn.fill(0, n);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const SankeyLink& link : *m_links) {
            const int s = link.source;
            const int t = link.target;
            if (m_nodeColumn[t] <= m_nodeColumn[s]) {
                m_nodeColumn[t] = m_nodeColumn[s] + 1;
                changed = true;
            }
        }
    }
}

void SankeyLayoutEngine::computeNodeRects()
{
    const int n = m_nodes->size();
    QHash<int, QVector<int>> columns;

    for (int i = 0; i < n; ++i)
        columns[m_nodeColumn[i]].append(i);

    const qreal scale = (m_rect.height() - NodeSpacing * n) / std::accumulate(m_nodeHeight.begin(), m_nodeHeight.end(), 0.0);

    for (auto it = columns.begin(); it != columns.end(); ++it) {
        qreal y = m_rect.top();

        for (int index : it.value()) {
            const qreal h = m_nodeHeight[index] * scale;
            (*m_nodes)[index].rect = QRectF(m_rect.left() + it.key() * ColumnSpacing, y, NodeWidth, h);
            y += h + NodeSpacing;
        }
    }
}

void SankeyLayoutEngine::computeLinkPaths()
{
    const int n = m_nodes->size();
    QVector<qreal> outOffset(n, 0.0);
    QVector<qreal> inOffset(n, 0.0);

    for (SankeyLink& link : *m_links) {
        const QRectF& src = (*m_nodes)[link.source].rect;
        const QRectF& dst = (*m_nodes)[link.target].rect;

        const qreal sy = src.top() + outOffset[link.source];
        const qreal ty = dst.top() + inOffset[link.target];

        const qreal thickness = qMax(1.0, link.value);

        outOffset[link.source] += thickness;
        inOffset[link.target] += thickness;

        const qreal dx = (dst.left() - src.right()) * LinkCurvature;

        QPainterPath path;
        path.moveTo(src.right(), sy);
        path.cubicTo(src.right() + dx, sy, dst.left() - dx, ty, dst.left(), ty);

        link.path = path;
    }
}
