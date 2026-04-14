/*
    SPDX-FileCopyrightText: 2026 The KMyMoney Authors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "transactionbuilder.h"

#include "mymoneymoney.h"
#include "mymoneysplit.h"
#include "mymoneytransaction.h"

#include <QDate>
#include <QString>

namespace KMyMoney { namespace Python {

TransactionBuilder::TransactionBuilder()
    : m_transaction(new MyMoneyTransaction)
{
}

TransactionBuilder::~TransactionBuilder()
{
    delete m_transaction;
}

void TransactionBuilder::setMemo(const std::string& memo)
{
    m_transaction->setMemo(QString::fromStdString(memo));
}

std::string TransactionBuilder::memo() const
{
    return m_transaction->memo().toStdString();
}

void TransactionBuilder::setCommodity(const std::string& commodityId)
{
    m_transaction->setCommodity(QString::fromStdString(commodityId));
}

std::string TransactionBuilder::commodity() const
{
    return m_transaction->commodity().toStdString();
}

void TransactionBuilder::setPostDate(int year, int month, int day)
{
    m_transaction->setPostDate(QDate(year, month, day));
}

void TransactionBuilder::setEntryDate(int year, int month, int day)
{
    m_transaction->setEntryDate(QDate(year, month, day));
}

void TransactionBuilder::addSplit(const std::string& accountId, const std::string& value, const std::string& memo)
{
    MyMoneySplit split;
    split.setAccountId(QString::fromStdString(accountId));
    split.setMemo(QString::fromStdString(memo));
    split.setValue(MyMoneyMoney(QString::fromStdString(value)));
    m_transaction->addSplit(split);
}

unsigned int TransactionBuilder::splitCount() const
{
    return m_transaction->splitCount();
}

bool TransactionBuilder::isBalanced() const
{
    return m_transaction->splitSum().isZero();
}

} // namespace Python
} // namespace KMyMoney
