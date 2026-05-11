/*
    SPDX-FileCopyrightText: 2026 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "statementbalancedelegate.h"

#include <QApplication>
#include <QPainter>

#include <KColorScheme>
#include <KLocalizedString>

#include "journalmodel.h"
#include "ledgerview.h"
#include "mymoneyfile.h"
#include "mymoneyutils.h"

class StatementBalanceDelegate::Private
{
public:
    Private()
        : m_margin(2)
        , m_lineHeight(12)
    {
    }

    int m_margin;
    int m_lineHeight;
};

StatementBalanceDelegate::StatementBalanceDelegate(LedgerView* parent)
    : KMMStyledItemDelegate(parent)
    , d(new Private)
{
}

StatementBalanceDelegate::~StatementBalanceDelegate()
{
    delete d;
}

void StatementBalanceDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_HasFocus;
    if (opt.state & QStyle::State_Selected) {
        opt.state |= QStyle::State_Active;
    }
    opt.state &= ~QStyle::State_Selected;
    opt.state |= QStyle::State_Enabled;

    painter->save();

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(parent());
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin);
    const QRect textArea = QRect(opt.rect.x() + margin, opt.rect.y() + margin, opt.rect.width() - 2 * margin, opt.rect.height() - 2 * margin);

    const auto statementBalanceDate = index.data(eMyMoney::Model::TransactionPostDateRole).toDate();
    const auto statementBalanceValue = index.data(eMyMoney::Model::ReconciliationAmountRole).value<MyMoneyMoney>();
    const auto accountBalance = MyMoneyFile::instance()->balance(index.data(eMyMoney::Model::SplitAccountIdRole).toString(), statementBalanceDate);

    KColorScheme::BackgroundRole role = (accountBalance == statementBalanceValue) ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground;

    KColorScheme::adjustBackground(opt.palette, role, QPalette::Base, KColorScheme::View, KSharedConfigPtr());
    opt.backgroundBrush = opt.palette.base();

    opt.rect.setX(opt.rect.x() - 2);
    opt.rect.setWidth(opt.rect.width() + 5);
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    switch (index.column()) {
    case JournalModel::Column::Detail:
        if (view && view->viewport()) {
            opt.rect.setX(0);
            opt.rect.setWidth(view->viewport()->width());
        }
        painter->setPen(opt.palette.color(QPalette::Normal, QPalette::Text));
        painter->drawText(opt.rect, Qt::AlignCenter, i18nc("Ledger marker showing a statement balance entry", "Statement balance"));
        break;
    case JournalModel::Column::Date:
        painter->setPen(opt.palette.color(QPalette::Normal, QPalette::Text));
        painter->drawText(textArea, opt.displayAlignment, MyMoneyUtils::formatDate(statementBalanceDate));
        break;
    case JournalModel::Column::Balance:
        painter->setPen(opt.palette.color(QPalette::Normal, QPalette::Text));
        painter->drawText(textArea, opt.displayAlignment, index.data(eMyMoney::Model::ReconciliationBalanceRole).toString());
        break;
    }

    painter->restore();
}

QSize StatementBalanceDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    d->m_margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin);
    d->m_lineHeight = opt.fontMetrics.lineSpacing();

    return QSize(10, d->m_lineHeight + 2 * d->m_margin);
}

bool StatementBalanceDelegate::eventFilter(QObject* o, QEvent* event)
{
    return QAbstractItemDelegate::eventFilter(o, event);
}
