/***************************************************************************
                          pivotgrid.cpp
                             -------------------
    begin                : Mon May 17 2004
    copyright            : (C) 2004-2005 by Ace Jones
    email                : <ace.j@hotpop.com>
                           Thomas Baumgart <ipwizard@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <pivotgrid.h>

// ----------------------------------------------------------------------------
// QT Includes

#include <QtDebug>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

namespace reports
{

const unsigned PivotOuterGroup::m_kDefaultSortOrder = 100;

PivotCell::PivotCell(const MyMoneyMoney& value) :
    MyMoneyMoney(value),
    m_stockSplit(MyMoneyMoney::ONE),
    m_cellUsed(!value.isZero())
{
}

PivotCell::~PivotCell()
{
}

PivotCell PivotCell::operator += (const PivotCell& right)
{
  const MyMoneyMoney& r = static_cast<const MyMoneyMoney&>(right);
  *this += r;
  m_postSplit = m_postSplit * right.m_stockSplit;
  m_stockSplit = m_stockSplit * right.m_stockSplit;
  m_postSplit += right.m_postSplit;
  m_cellUsed |= right.m_cellUsed;
  return *this;
}

PivotCell PivotCell::operator += (const MyMoneyMoney& value)
{
  m_cellUsed |= !value.isZero();
  if (m_stockSplit != MyMoneyMoney::ONE)
    m_postSplit += value;
  else
    MyMoneyMoney::operator += (value);
  return *this;
}

PivotCell PivotCell::stockSplit(const MyMoneyMoney& factor)
{
  PivotCell s;
  s.m_stockSplit = factor;
  return s;
}

const QString PivotCell::formatMoney(int fraction, bool showThousandSeparator) const
{
  return formatMoney("", MyMoneyMoney::denomToPrec(fraction), showThousandSeparator);
}

const QString PivotCell::formatMoney(const QString& currency, const int prec, bool showThousandSeparator) const
{
  // construct the result
  MyMoneyMoney res = (*this * m_stockSplit) + m_postSplit;
  return res.formatMoney(currency, prec, showThousandSeparator);
}

MyMoneyMoney PivotCell::calculateRunningSum(const MyMoneyMoney& runningSum)
{
  MyMoneyMoney::operator += (runningSum);
  MyMoneyMoney::operator = ((*this * m_stockSplit) + m_postSplit);
  m_postSplit = MyMoneyMoney();
  m_stockSplit = MyMoneyMoney::ONE;
  return *this;
}

MyMoneyMoney PivotCell::cellBalance(const MyMoneyMoney& _balance)
{
  MyMoneyMoney balance(_balance);
  balance += *this;
  balance = (balance * m_stockSplit) + m_postSplit;
  return balance;
}

PivotGridRowSet::PivotGridRowSet(unsigned _numcolumns)
{
  insert(eActual, PivotGridRow(_numcolumns));
  insert(eBudget, PivotGridRow(_numcolumns));
  insert(eBudgetDiff, PivotGridRow(_numcolumns));
  insert(eForecast, PivotGridRow(_numcolumns));
  insert(eAverage, PivotGridRow(_numcolumns));
  insert(ePrice, PivotGridRow(_numcolumns));
}

PivotGridRowSet PivotGrid::rowSet(QString id)
{

  //go through the data and get the row that matches the id
  PivotGrid::iterator it_outergroup = begin();
  while (it_outergroup != end()) {
    PivotOuterGroup::iterator it_innergroup = (*it_outergroup).begin();
    while (it_innergroup != (*it_outergroup).end()) {
      PivotInnerGroup::iterator it_row = (*it_innergroup).begin();
      while (it_row != (*it_innergroup).end()) {
        if (it_row.key().id() == id)
          return it_row.value();

        ++it_row;
      }
      ++it_innergroup;
    }
    ++it_outergroup;
  }
  return PivotGridRowSet();
}

} // namespace


QDebug operator<<(QDebug dbg, reports::PivotGrid &a)
{
  dbg.nospace() << "reports::PivotGrid("
   << "QMap(";
   foreach(const QString &key, a.keys()) {
    dbg.nospace() << key << ": " << a[key] << ",";
  }
  dbg << ")"
      << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const reports::PivotCell &a)
{
  dbg.space() << "reports::PivotCell("
  //;dbg = operator<<(dbg, static_cast<const MyMoneyMoney&>(a))
      << "MyMoneyMoney("
      << "isAutoCalc" << a.isAutoCalc()
      << "isNegative" << a.isNegative()
      << "isPositive" << a.isPositive()
      << "isZero" << a.isZero()
      << "value" << a.toString()
      << ")"
      << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const reports::ReportAccount &a)
{
  dbg.space() << "reports::ReportAccount("
  //;dbg = operator<<(dbg, static_cast<const MyMoneyAccount&>(a))
      << "id" << a.id()
      << "accountType" << MyMoneyAccount::accountTypeToString(a.accountType())
    << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const QMap<QString, reports::PivotOuterGroup> &a)
{
  dbg.space() << "reports::PivotOuterGroup("
      << "QMap(";
  foreach(const QString &key, a.keys()) {
    dbg << key << ":" << a[key] << ",";
  }
  dbg << ")"
      << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const QMap<QString, reports::PivotInnerGroup> &a)
{
  dbg.space() << "reports::PivotInnerGroup("
      << "QMap(";
  foreach(const QString &key, a.keys()) {
    dbg << key << ":" << a[key] << ",";
  }
  dbg << ")"
      << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const QMap<reports::ReportAccount, reports::PivotGridRowSet> &a)
{
  dbg.space() << "reports::PivotGridRowSet("
      << "QMap(";
  foreach(const reports::ReportAccount &key, a.keys()) {
    dbg << key << ":" << a[key] << ",";
  }
  dbg << ")"
      << ")";
  return dbg;
}

QDebug operator<<(QDebug dbg, const QMap<reports::ERowType, reports::PivotGridRow> &a)
{
  dbg.space() << "reports::PivotGridRow("
      << "QMap(";
  foreach(const reports::ERowType &key, a.keys()) {
    dbg << key << ":" << a[key] << ",";
  }
  dbg << ")"
      << ")";
  return dbg;
}

