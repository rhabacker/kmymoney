/*
    SPDX-FileCopyrightText: 2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ONLINEBALANCEHISTORYPROXYMODEL_H
#define ONLINEBALANCEHISTORYPROXYMODEL_H

#include "kmm_models_export.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QSortFilterProxyModel>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

class OnlineBalanceHistoryProxyModelPrivate;
class KMM_MODELS_EXPORT OnlineBalanceHistoryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum Column {
        Symbol = 0,
        Quantity,
        Price,
        Value,
    };

    OnlineBalanceHistoryProxyModel(QObject* parent = nullptr);
    ~OnlineBalanceHistoryProxyModel();

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QVariant data(const QModelIndex& idx, int role) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    void init();
    void load();

private:
    OnlineBalanceHistoryProxyModelPrivate* d_ptr;
    Q_DECLARE_PRIVATE(OnlineBalanceHistoryProxyModel);
};

#endif // ONLINEBALANCEHISTORYPROXYMODEL_H
