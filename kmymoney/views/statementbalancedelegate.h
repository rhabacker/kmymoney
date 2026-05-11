/*
    SPDX-FileCopyrightText: 2026 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef STATEMENTBALANCEDELEGATE_H
#define STATEMENTBALANCEDELEGATE_H

#include "kmmstyleditemdelegate.h"

class LedgerView;

class StatementBalanceDelegate : public KMMStyledItemDelegate
{
    Q_OBJECT
public:
    explicit StatementBalanceDelegate(LedgerView* parent = nullptr);
    ~StatementBalanceDelegate() override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const final override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const final override;

protected:
    bool eventFilter(QObject* o, QEvent* event) final override;

private:
    class Private;
    Private* const d;
};

#endif
