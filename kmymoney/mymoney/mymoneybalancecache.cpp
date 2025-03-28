/*
    SPDX-FileCopyrightText: 2011 Fernando Vilas <fvilas@iname.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mymoneybalancecache.h"

// ----------------------------------------------------------------------------
// QT Includes

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

MyMoneyBalanceCacheItem::MyMoneyBalanceCacheItem(const MyMoneyMoney& balance, const QDate& date)
    : m_balance(balance), m_date(date)
{}

const MyMoneyMoney& MyMoneyBalanceCacheItem::balance() const
{
    return m_balance;
}

const QDate& MyMoneyBalanceCacheItem::date() const
{
    return m_date;
}

bool MyMoneyBalanceCacheItem::isValid() const
{
    return !(!m_date.isValid() && m_balance == MyMoneyMoney::minValue);
}

void MyMoneyBalanceCache::clear()
{
    m_cache.clear();
}

void MyMoneyBalanceCache::clear(const QString& accountId)
{
    m_cache.remove(accountId);
}

void MyMoneyBalanceCache::clear(const QString& accountId, const QDate& date)
{
    BalanceCacheType::Iterator acctPos = m_cache.find(accountId);
    if (m_cache.end() == acctPos)
        return;

    // Always remove QDate()
    BalanceCacheType::mapped_type::Iterator datePos = (*acctPos).find(QDate());
    if ((*acctPos).end() != datePos) {
        datePos = (*acctPos).erase(datePos);
    }

    // Now look for the actual value and remove it
    if (date.isValid()) {
        datePos = (*acctPos).lowerBound(date);

        while ((*acctPos).end() != datePos) {
            datePos = (*acctPos).erase(datePos);
        }
    }
}

bool MyMoneyBalanceCache::isEmpty() const
{
    return m_cache.isEmpty();
}

int MyMoneyBalanceCache::size() const
{
    int sum = 0;

    for (BalanceCacheType::const_iterator i = m_cache.cbegin(); i != m_cache.cend(); ++i) {
        sum += (*i).size();
    }
    return sum;
}

void MyMoneyBalanceCache::insert(const QString& accountId, const QDate& date, const MyMoneyMoney& balance)
{
    m_cache[accountId].insert(date, balance);
}

MyMoneyBalanceCacheItem MyMoneyBalanceCache::balance(const QString& accountId, const QDate& date) const
{
    BalanceCacheType::const_iterator acctPos = m_cache.constFind(accountId);
    if (m_cache.cend() == acctPos)
        return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, QDate());

    BalanceCacheType::mapped_type::const_iterator datePos = (*acctPos).constFind(date);

    if ((*acctPos).cend() == datePos)
        return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, QDate());

    return MyMoneyBalanceCacheItem(datePos.value(), datePos.key());
}

MyMoneyBalanceCacheItem MyMoneyBalanceCache::mostRecentBalance(const QString& accountId, const QDate& date) const
{
    BalanceCacheType::const_iterator acctPos = m_cache.constFind(accountId);
    if (m_cache.cend() == acctPos)
        return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, QDate());

    BalanceCacheType::mapped_type::const_iterator datePos = (*acctPos).lowerBound(date);

    while ((*acctPos).cend() == datePos || ((*acctPos).cbegin() != datePos && datePos.key() > date)) {
        --datePos;
    }

    if ((*acctPos).cbegin() == datePos && datePos.key() > date)
        return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, QDate());

    return MyMoneyBalanceCacheItem(datePos.value(), datePos.key());
}

