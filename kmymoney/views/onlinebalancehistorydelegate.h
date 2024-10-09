/*
    SPDX-FileCopyrightText: 2015-2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ONLINEBALANCEHISTORYDELEGATE_H
#define ONLINEBALANCEHISTORYDELEGATE_H

// ----------------------------------------------------------------------------
// QT Includes

class QColor;

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "kmmstyleditemdelegate.h"
#include "mymoneyenums.h"

class LedgerView;
class MyMoneyMoney;

class OnlineBalanceHistoryDelegate : public KMMStyledItemDelegate
{
    Q_OBJECT
public:
    explicit OnlineBalanceHistoryDelegate(LedgerView* parent = 0);
    virtual ~OnlineBalanceHistoryDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const final override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const final override;

    void setOnlineBalance(const QDate& date, const MyMoneyMoney& amount, int fraction = 0);

    /**
     * Which data (@a role) shall be displayed in the detail column
     * when only a single line is shown. The default is the payee.
     */
    void setSingleLineRole(eMyMoney::Model::Roles role);

    static void setErroneousColor(const QColor& color);
    static void setImportedColor(const QColor& color);

    static QColor erroneousColor();

protected:
    bool eventFilter(QObject* o, QEvent* event) final override;

private:
    class Private;
    Private* const d;

    static QColor m_erroneousColor;
    static QColor m_importedColor;
    static QColor m_separatorColor;
};

#endif // ONLINEBALANCEHISTORYDELEGATE_H
