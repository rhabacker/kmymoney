/*
    SPDX-FileCopyrightText: 2026 The KMyMoney Authors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMYMONEY_PYTHON_TRANSACTIONBUILDER_H
#define KMYMONEY_PYTHON_TRANSACTIONBUILDER_H

#include <string>

class MyMoneyTransaction;

namespace KMyMoney { namespace Python {

class TransactionBuilder
{
public:
    TransactionBuilder();
    ~TransactionBuilder();

    TransactionBuilder(const TransactionBuilder&) = delete;
    TransactionBuilder& operator=(const TransactionBuilder&) = delete;
    TransactionBuilder(TransactionBuilder&&) = delete;
    TransactionBuilder& operator=(TransactionBuilder&&) = delete;

    void setMemo(const std::string& memo);
    std::string memo() const;

    void setCommodity(const std::string& commodityId);
    std::string commodity() const;

    void setPostDate(int year, int month, int day);
    void setEntryDate(int year, int month, int day);

    void addSplit(const std::string& accountId, const std::string& value, const std::string& memo = std::string());

    unsigned int splitCount() const;
    bool isBalanced() const;

private:
    MyMoneyTransaction* m_transaction;
};

} // namespace Python
} // namespace KMyMoney

#endif
