/*
    SPDX-FileCopyrightText: 2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "reportsmodel.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QColor>
#include <QDebug>
#include <QFont>
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
    return useGroups() ? 2 : 5;
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
        case Favorite:
            return i18nc("Report favorite", "Favorite");
            break;
        case Group:
            return i18nc("Report group", "Group");
            break;
        case Modified:
            return i18nc("Report state", "Modified");
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
        case Favorite:
            return report.isFavorite() ? QStringLiteral("\u2605") : QString();
        case Group:
            return i18n(report.group().toLatin1());
        case Modified:
            return report.isModified() ? QStringLiteral("\u2605") : QString();
        default:
            return QStringLiteral("not yet implemented");
        }
        break;

    case Qt::ForegroundRole:
        return report.isModified() ? QColor(Qt::red) : QVariant();
    case Qt::FontRole: {
        QFont f;
        f.setBold(report.isModified());
        return f;
    }

    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case Favorite:
        case Modified:
            return Qt::AlignCenter;
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
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

int ReportsModel::processItems(Worker* worker)
{
    QModelIndexList indexes = match(index(0, 0), eMyMoney::Model::Roles::IdRole, m_idLeadin, -1, Qt::MatchStartsWith | Qt::MatchRecursive);
    int result = MyMoneyModel::processItems(worker, indexes);
    bool changedModel = false;
    for (const auto& idx : indexes) {
        auto& report = static_cast<TreeItem<MyMoneyReport>*>(idx.internalPointer())->dataRef();
        if (report.isModified()) {
            report.setModified(false);
            changedModel = true;
            int nCols = columnCount(idx.parent());
            QModelIndex first = index(idx.row(), 0, idx.parent());
            QModelIndex last = index(idx.row(), nCols - 1, idx.parent());

            Q_EMIT dataChanged(first, last, {Qt::DisplayRole, Qt::ForegroundRole, Qt::FontRole});
        }
    }
    if (changedModel)
        Q_EMIT modelChanged();
    return result;
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
    // group number, this will be used as sort key for reportgroup items
    // we have:
    // 1st some default groups
    // 2nd a chart group
    // 3rd maybe a favorite group
    // 4th maybe an orphan group (for old reports)
    int groupNo = 1;

    for (const auto& group : reportGroups) {
        MyMoneyReport groupReport;
        groupReport.setName(QString("%1. %2").arg(groupNo++).arg(i18n(group.name().toLatin1().data()))); // store group name
        TreeItem<MyMoneyReport>* groupItem = new TreeItem<MyMoneyReport>(groupReport, m_rootItem);
        m_rootItem->appendChild(groupItem);
        groupItems[group.name()] = groupItem;
        for (const auto& report : group) {
            if (!report.isBuiltIn())
                continue;
            TreeItem<MyMoneyReport>* reportItem = new TreeItem<MyMoneyReport>(report, groupItem);
            groupItem->appendChild(reportItem);

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
