// sankeylayoutengine.h
#ifndef SANKEYLAYOUTENGINE_H
#define SANKEYLAYOUTENGINE_H

#include <QRectF>
#include <QVector>

struct SankeyNode;
struct SankeyLink;

class SankeyLayoutEngine
{
public:
    void setNodes(QVector<SankeyNode>* nodes);
    void setLinks(QVector<SankeyLink>* links);
    void setRect(const QRectF& rect);

    void layout();

private:
    void computeNodeValues();
    void assignColumns();
    void computeNodeRects();
    void computeLinkPaths();

    QVector<SankeyNode>* m_nodes = nullptr;
    QVector<SankeyLink>* m_links = nullptr;
    QRectF m_rect;

    QVector<int> m_nodeColumn;
    QVector<qreal> m_nodeHeight;
};

#endif // SANKEYLAYOUTENGINE_H
