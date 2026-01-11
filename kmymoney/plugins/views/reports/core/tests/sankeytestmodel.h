#ifndef SANKEYTESTMODEL_H
#define SANKEYTESTMODEL_H

#include <QAbstractTableModel>

class SankeyTestModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SankeyTestModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    enum Roles { SourceRole = Qt::UserRole + 1, TargetRole, ValueRole };

private:
    struct Link {
        int source;
        int target;
        qreal value;
    };

    QVector<Link> m_links;
};

#endif
