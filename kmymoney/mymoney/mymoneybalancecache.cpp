/***************************************************************************
                         mymoneybalancecache  -  description
                            -------------------
   begin                : Tue Sep 21 2010
   copyright            : (C) 2010 by Fernando Vilas
   email                : kmymoney-devel@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mymoneybalancecache.h"
#include "mymoneyutils.h"

// ----------------------------------------------------------------------------
// QT Includes

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

MyMoneyBalanceCacheItem::MyMoneyBalanceCacheItem(const MyMoneyMoney& balance, const MyMoneyDate &date)
    : m_balance(balance), m_date(date)
{}

const MyMoneyMoney& MyMoneyBalanceCacheItem::balance() const
{
  return m_balance;
}

const MyMoneyDate& MyMoneyBalanceCacheItem::date() const
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

void MyMoneyBalanceCache::clear(const QString& accountId, const MyMoneyDate &date)
{
  BalanceCacheType::Iterator acctPos = m_cache.find(accountId);
  if (m_cache.end() == acctPos)
    return;

  // Always remove MyMoneyDate()
  BalanceCacheType::mapped_type::Iterator datePos = (*acctPos).find(MyMoneyDate());
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

  for (BalanceCacheType::ConstIterator i = m_cache.constBegin(); i != m_cache.constEnd(); ++i) {
    sum += (*i).size();
  }
  return sum;
}

void MyMoneyBalanceCache::insert(const QString& accountId, const MyMoneyDate &date, const MyMoneyMoney& balance)
{
  m_cache[accountId].insert(date, balance);
}

MyMoneyBalanceCacheItem MyMoneyBalanceCache::balance(const QString& accountId, const MyMoneyDate& date) const
{
  BalanceCacheType::ConstIterator acctPos = m_cache.constFind(accountId);
  if (m_cache.constEnd() == acctPos)
    return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, MyMoneyDate());

  BalanceCacheType::mapped_type::ConstIterator datePos = (*acctPos).constFind(date);

  if ((*acctPos).constEnd() == datePos)
    return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, MyMoneyDate());

  return MyMoneyBalanceCacheItem(datePos.value(), datePos.key());
}

MyMoneyBalanceCacheItem MyMoneyBalanceCache::mostRecentBalance(const QString& accountId, const MyMoneyDate &date) const
{
  BalanceCacheType::ConstIterator acctPos = m_cache.constFind(accountId);
  if (m_cache.constEnd() == acctPos)
    return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, MyMoneyDate());

  BalanceCacheType::mapped_type::ConstIterator datePos = (*acctPos).lowerBound(date);

  while ((*acctPos).constEnd() == datePos || ((*acctPos).constBegin() != datePos && datePos.key() > date)) {
    --datePos;
  }

  if ((*acctPos).constBegin() == datePos && datePos.key() > date)
    return MyMoneyBalanceCacheItem(MyMoneyMoney::minValue, MyMoneyDate());

  return MyMoneyBalanceCacheItem(datePos.value(), datePos.key());
}

