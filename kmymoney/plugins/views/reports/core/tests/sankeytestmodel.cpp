#include "sankeytestmodel.h"

#include "../../sankeyroles.h"

SankeyTestModel::SankeyTestModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Node indices:
    // 0 Salary
    // 1 Checking
    // 2 Rent
    // 3 Food
    // 4 Savings

    m_links = {{0, 1, 3000.0}, {1, 2, 1200.0}, {1, 3, 500.0}, {1, 4, 800.0}};
}

int SankeyTestModel::rowCount(const QModelIndex&) const
{
    return m_links.size();
}

int SankeyTestModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant SankeyTestModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const Link& link = m_links.at(index.row());

    switch (role) {
    case SankeyRoles::SourceRole:
        return link.source;
    case SankeyRoles::TargetRole:
        return link.target;
    case SankeyRoles::ValueRole:
        return link.value;
    default:
        return {};
    }
}
