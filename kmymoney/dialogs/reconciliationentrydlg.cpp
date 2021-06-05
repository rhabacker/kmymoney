/*
    SPDX-FileCopyrightText: 2021 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "reconciliationentrydlg.h"

// ----------------------------------------------------------------------------
// Project Includes

#include "ui_reconciliationentrydlg.h"

class ReconciliationEntryDlgPrivate
{
    Q_DISABLE_COPY(ReconciliationEntryDlgPrivate)

public:
    ReconciliationEntryDlgPrivate() :
        ui(new Ui::ReconciliationEntryDlg)
    {
    }

    ~ReconciliationEntryDlgPrivate()
    {
        delete ui;
    }

    Ui::ReconciliationEntryDlg *ui;
};


ReconciliationEntryDlg::ReconciliationEntryDlg(const QDate& date, const MyMoneyMoney& balance, QWidget *parent) :
    QDialog(parent),
    d_ptr(new ReconciliationEntryDlgPrivate)
{
    Q_D(ReconciliationEntryDlg);
    d->ui->setupUi(this);
    d->ui->m_date->setDate(date);
    if (!balance.isZero())
        d->ui->m_balance->setValue(balance);
    d->ui->m_balance->setFocus();
    setModal(true);
}

ReconciliationEntryDlg::~ReconciliationEntryDlg()
{
}

QDate ReconciliationEntryDlg::date() const
{
    Q_D(const ReconciliationEntryDlg);
    return d->ui->m_date->date();
}

MyMoneyMoney ReconciliationEntryDlg::balance() const
{
    Q_D(const ReconciliationEntryDlg);
    return d->ui->m_balance->value();
}
