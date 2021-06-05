/*
    SPDX-FileCopyrightText: 2021 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RECONCILIATIONENTRYDLG_H
#define RECONCILIATIONENTRYDLG_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QDialog>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoneymoney.h"

/**
  *@author Ralf Habacker
  */

class ReconciliationEntryDlgPrivate;
class ReconciliationEntryDlg : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(ReconciliationEntryDlg)

public:
    explicit ReconciliationEntryDlg(const QDate& date, const MyMoneyMoney &balance, QWidget *parent = nullptr);
    ~ReconciliationEntryDlg();
    QDate date() const;
    MyMoneyMoney balance() const;

private:
    ReconciliationEntryDlgPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ReconciliationEntryDlg)
};

#endif // RECONCILIATIONENTRYDLG_H
