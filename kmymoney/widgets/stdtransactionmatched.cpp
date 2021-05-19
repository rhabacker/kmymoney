/***************************************************************************
                          stdtransactionmatched.cpp
                             -------------------
    begin                : Sat May 11 2008
    copyright            : (C) 2008 by Thomas Baumgart
    email                : Thomas Baumgart <ipwizard@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "stdtransactionmatched.h"
#include "transaction.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QList>
#include <QHeaderView>

// ----------------------------------------------------------------------------
// KDE Includes

#include <klocale.h>
#include <kdebug.h>

// ----------------------------------------------------------------------------
// Project Includes

#include <kmymoneyglobalsettings.h>
#include <register.h>

using namespace KMyMoneyRegister;
using namespace KMyMoneyTransactionForm;

StdTransactionMatched::StdTransactionMatched(Register *parent, const MyMoneyTransaction& transaction, const MyMoneySplit& split, int uniqueId) :
    StdTransaction(parent, transaction, split, uniqueId)
{
  // setup initial size
  setNumRowsRegister(numRowsRegister(KMyMoneyGlobalSettings::showRegisterDetailed()));
}

bool StdTransactionMatched::paintRegisterCellSetup(QPainter *painter, QStyleOptionViewItemV4 &option, const QModelIndex &index)
{
  bool rc = Transaction::paintRegisterCellSetup(painter, option, index);

  // if not selected paint in matched background color
  if (!isSelected()) {
    option.palette.setColor(QPalette::Base, KMyMoneyGlobalSettings::matchedTransactionColor());
    option.palette.setColor(QPalette::AlternateBase, KMyMoneyGlobalSettings::matchedTransactionColor());
  }
  //TODO: the first line needs to be painted across all columns
  return rc;
}

void StdTransactionMatched::registerCellText(QString& txt, Qt::Alignment& align, int row, int col, QPainter* painter)
{
  // run through the standard
  StdTransaction::registerCellText(txt, align, row, col, painter);

  // we only cover the additional rows
  if (row >= m_rowsRegister - m_additionalRows) {
    // make row relative to the last three rows
    row += m_additionalRows - m_rowsRegister;

    // remove anything that had been added by the standard method
    txt = "";

    // and we draw this information in italics
    if (painter) {
      QFont font = painter->font();
      font.setItalic(true);
      painter->setFont(font);
    }

    MyMoneyTransaction matchedTransaction = m_split.matchedTransaction();
    MyMoneySplit matchedSplit;
    try {
      matchedSplit = matchedTransaction.splitById(m_split.value("kmm-match-split"));
    } catch (const MyMoneyException &) {
    }

    QList<MyMoneySplit>::const_iterator it_s;
    const QList<MyMoneySplit>& list = matchedTransaction.splits();
    MyMoneyMoney importedValue;
    for (it_s = list.begin(); it_s != list.end(); ++it_s) {
      if ((*it_s).accountId() == m_account.id()) {
        importedValue += (*it_s).shares();
      }
    }

    MyMoneyDate postDate;
    QString memo;
    switch (row) {
      case 0:
        if (painter && col == DetailColumn)
          txt = QString(" ") + i18n("KMyMoney has matched the two selected transactions (result above)");
        // return true for the first visible column only
        break;

      case 1:
        switch (col) {
          case DateColumn:
            align |= Qt::AlignLeft;
            txt = i18n("Bank entry:");
            break;

          case DetailColumn:
            align |= Qt::AlignLeft;
            memo = matchedTransaction.memo();
            memo.replace("\n\n", "\n");
            memo.replace('\n', ", ");
            txt = QString("%1 %2").arg(matchedTransaction.postDate().toString(Qt::ISODate)).arg(memo);
            break;

          case PaymentColumn:
            align |= Qt::AlignRight;
            if (importedValue.isNegative()) {
              txt = (-importedValue).formatMoney(m_account.fraction());
            }
            break;

          case DepositColumn:
            align |= Qt::AlignRight;
            if (!importedValue.isNegative()) {
              txt = importedValue.formatMoney(m_account.fraction());
            }
            break;
        }
        break;

      case 2:
        switch (col) {
          case DateColumn:
            align |= Qt::AlignLeft;
            txt = i18n("Your entry:");
            break;

          case DetailColumn:
            align |= Qt::AlignLeft;
            postDate = m_transaction.postDate();
            if (!m_split.value("kmm-orig-postdate").isEmpty()) {
              postDate = MyMoneyDate::fromString(m_split.value("kmm-orig-postdate"), Qt::ISODate);
            }
            memo = m_split.memo();
            if (!matchedSplit.memo().isEmpty() && memo != matchedSplit.memo()) {
              int pos = memo.lastIndexOf(matchedSplit.memo());
              if (pos != -1) {
                memo = memo.left(pos);
                // replace all new line characters because we only have one line available for the displayed data
              }
            }
            memo.replace("\n\n", "\n");
            memo.replace('\n', ", ");
            txt = QString("%1 %2").arg(postDate.toString(Qt::ISODate)).arg(memo);
            break;

          case PaymentColumn:
            align |= Qt::AlignRight;
            if (m_split.value().isNegative()) {
              txt = (-m_split.value(m_transaction.commodity(), m_splitCurrencyId)).formatMoney(m_account.fraction());
            }
            break;

          case DepositColumn:
            align |= Qt::AlignRight;
            if (!m_split.value().isNegative()) {
              txt = m_split.value(m_transaction.commodity(), m_splitCurrencyId).formatMoney(m_account.fraction());
            }
            break;

        }
        break;
    }
  }
}
