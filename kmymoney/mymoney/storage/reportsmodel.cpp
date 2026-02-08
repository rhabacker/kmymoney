/*
    SPDX-FileCopyrightText: 2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "reportsmodel.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QDebug>
#include <QString>

// ----------------------------------------------------------------------------
// KDE Includes

#include <KLocalizedString>

// ----------------------------------------------------------------------------
// Project Includes

#include "reportgroup.h"

struct ReportsModelPrivate {
    ReportsModelPrivate()
        : m_useGroups(false)
    {
    }
    bool m_useGroups;
};

ReportsModel::ReportsModel(QObject* parent, QUndoStack* undoStack)
    : MyMoneyModel<MyMoneyReport>(parent, QStringLiteral("R"), ReportsModel::ID_SIZE, undoStack)
    , d(new ReportsModelPrivate)
{
    setObjectName(QLatin1String("ReportsModel"));
}

ReportsModel::~ReportsModel()
{
}

int ReportsModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ReportsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case ReportName:
            return i18nc("Reportname", "Name");
            break;
        case Comment:
            return i18nc("Report comment", "Comment");
            break;
        }
    }
    return MyMoneyModelBase::headerData(section, orientation, role);
}

QVariant ReportsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
        return QVariant();

    const MyMoneyReport& report = static_cast<TreeItem<MyMoneyReport>*>(index.internalPointer())->constDataRef();
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch(index.column()) {
        case ReportName:
            return report.name();
        case Comment:
            return report.comment();
        default:
            return QStringLiteral("not yet implemented");
        }
        break;

    case Qt::TextAlignmentRole:
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        break;

    case eMyMoney::Model::Roles::IdRole:
        return report.id();
        break;
    }
    return QVariant();
}

bool ReportsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid()) {
        return false;
    }

    qDebug() << "setData(" << index.row() << index.column() << ")" << value << role;
    return QAbstractItemModel::setData(index, value, role);
}

void ReportsModel::load(const QMap<QString, MyMoneyReport>& reports)
{
    MyMoneyModel<MyMoneyReport>::load(reports);
    setUseGroups(false);
}

void ReportsModel::load(const QList<ReportGroup>& reportGroups)
{
    QElapsedTimer t;
    t.start();

    beginResetModel();

    // clear any previous items
    clearModelItems();

    // map to hold group nodes
    QMap<QString, TreeItem<MyMoneyReport>*> groupItems;

    for (const auto& group : reportGroups) {
        MyMoneyReport groupReport;
        groupReport.setName(group.name()); // store group name
        TreeItem<MyMoneyReport>* groupItem = new TreeItem<MyMoneyReport>(groupReport, m_rootItem);
        m_rootItem->appendChild(groupItem); // pass pointer
        groupItems[group.name()] = groupItem;
        for (const auto& report : group) {
            TreeItem<MyMoneyReport>* reportItem = new TreeItem<MyMoneyReport>(report, groupItem);
            groupItem->appendChild(reportItem); // pass pointer

            // add to ID mapper
            if (m_idToItemMapper)
                m_idToItemMapper->insert(report.id(), reportItem);
        }
    }
    // reset dirty flag
    setDirty(false);
    m_nextId = 0;

    endResetModel();
    Q_EMIT modelLoaded();

    qDebug() << "ReportsModel loaded with" << rowCount() << "groups in" << t.elapsed() << "ms";
    setUseGroups(true);
}

void ReportsModel::loadWithGroups(const QMap<QString, MyMoneyReport>& reports)
{
    QElapsedTimer t;
    t.start();

    beginResetModel();

    // clear any previous items
    clearModelItems();

    // map to hold group nodes
    QMap<QString, TreeItem<MyMoneyReport>*> groupItems;

    // create group items and reports
    for (const auto& report : reports) {
        const QString groupName = report.group();

        // create group TreeItem if it doesn't exist yet
        TreeItem<MyMoneyReport>* groupItem = nullptr;
        if (!groupItems.contains(groupName)) {
            // dummy report for the group node
            MyMoneyReport groupReport;
            groupReport.setName(groupName); // store group name
            groupItem = new TreeItem<MyMoneyReport>(groupReport, m_rootItem);
            m_rootItem->appendChild(groupItem); // pass pointer
            groupItems[groupName] = groupItem;
        } else {
            groupItem = groupItems[groupName];
        }

        // create TreeItem for the actual report
        TreeItem<MyMoneyReport>* reportItem = new TreeItem<MyMoneyReport>(report, groupItem);
        groupItem->appendChild(reportItem); // pass pointer

        // add to ID mapper
        if (m_idToItemMapper)
            m_idToItemMapper->insert(report.id(), reportItem);
    }

    // reset dirty flag
    setDirty(false);
    m_nextId = 0;

    endResetModel();
    Q_EMIT modelLoaded();

    qDebug() << "ReportsModel loaded with" << rowCount() << "groups in" << t.elapsed() << "ms";
    setUseGroups(true);
}

Qt::ItemFlags ReportsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
        return Qt::NoItemFlags;

    if (useGroups()) {
        auto* item = static_cast<TreeItem<MyMoneyReport>*>(index.internalPointer());

        if (item->dataRef().id().isEmpty()) {
            // enabled (so it expands), but not selectable
            return Qt::ItemIsEnabled;
        }
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ReportsModel::useGroups() const
{
    Q_D(const ReportsModel);
    return d->m_useGroups;
}

void ReportsModel::setUseGroups(bool state)
{
    Q_D(ReportsModel);
    d->m_useGroups = state;
}
