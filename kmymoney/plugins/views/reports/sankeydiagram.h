#ifndef SANKEYDIAGRAM_H
#define SANKEYDIAGRAM_H

#include <KChartAbstractDiagram>

class SankeyDiagramPrivate;

/**
 * SankeyDiagram
 *
 * A flow diagram showing weighted connections between nodes.
 * Intended for financial flow visualization (income → expenses → savings).
 */
class SankeyDiagram : public KChart::AbstractDiagram
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SankeyDiagram)

public:
    explicit SankeyDiagram(QWidget* parent = nullptr);
    ~SankeyDiagram() override;

    // KChart::AbstractDiagram interface
    void paint(KChart::PaintContext* paintContext) override;
    void resize(const QSizeF& size) override;

    // Model handling
    void setModel(QAbstractItemModel* model) override;

protected Q_SLOTS:
    // Called when model or geometry changes
    void updateLayout();

protected:
    const QPair<QPointF, QPointF> calculateDataBoundaries() const override;
    void rebuildDataFromModel();
    void computeLinkPaths();
    void computeNodeRects(const QRectF& area);

private:
    SankeyDiagramPrivate* const d_ptr;
};

#endif // SANKEYDIAGRAM_H
