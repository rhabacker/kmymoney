/***************************************************************************
                          mymoneysecuritytest.cpp
                          -------------------
    copyright            : (C) 2002 by Kevin Tambascio
    email                : ipwizard@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mymoneysecuritytest.h"

#include <QtTest/QtTest>

QTEST_MAIN(MyMoneySecurityTest)

void MyMoneySecurityTest::init()
{
  m = new MyMoneySecurity();
}

void MyMoneySecurityTest::cleanup()
{
  delete m;
}

void MyMoneySecurityTest::testEmptyConstructor()
{
  QVERIFY(m->id().isEmpty());
  QVERIFY(m->name().isEmpty());
  QVERIFY(m->tradingSymbol().isEmpty());
  QVERIFY(m->securityType() == MyMoneySecurity::SECURITY_NONE);
  QVERIFY(m->tradingMarket().isEmpty());
  QVERIFY(m->tradingCurrency().isEmpty());
  QVERIFY(m->smallestCashFraction() == 100);
  QVERIFY(m->smallestAccountFraction() == 100);
  QVERIFY(m->partsPerUnit() == 100);
}

void MyMoneySecurityTest::testCopyConstructor()
{
  MyMoneySecurity* n1 = new MyMoneySecurity("GUID1", *m);
  MyMoneySecurity n2(*n1);

  // QVERIFY(*n1 == n2);

  delete n1;
}

void MyMoneySecurityTest::testNonemptyConstructor()
{
  MyMoneyDate date(2004, 4, 1);
  MyMoneyMoney val("1234/100");

  m->setName("name");
  m->setTradingSymbol("symbol");
  m->setSecurityType(MyMoneySecurity::SECURITY_CURRENCY);
  // m->addPriceHistory(date, val);

  MyMoneySecurity n("id", *m);

  QVERIFY(n.id() == QString("id"));
  QVERIFY(n.tradingSymbol() == QString("symbol"));
  QVERIFY(n.securityType() == MyMoneySecurity::SECURITY_CURRENCY);
  // QVERIFY(n.priceHistory().count() == 1);
}


void MyMoneySecurityTest::testSetFunctions()
{
  m->setName("Name");
  m->setTradingSymbol("Symbol");
  m->setTradingMarket("Market");
  m->setTradingCurrency("Currency");
  m->setSecurityType(MyMoneySecurity::SECURITY_STOCK);
  m->setSmallestAccountFraction(50);
  m->setSmallestCashFraction(2);
  m->setPartsPerUnit(30);

  QVERIFY(m->name() == "Name");
  QVERIFY(m->tradingSymbol() == "Symbol");
  QVERIFY(m->tradingMarket() == "Market");
  QVERIFY(m->tradingCurrency() == "Currency");
  QVERIFY(m->securityType() == MyMoneySecurity::SECURITY_STOCK);
  QVERIFY(m->smallestAccountFraction() == 50);
  QVERIFY(m->smallestCashFraction() == 2);
  QVERIFY(m->partsPerUnit() == 30);
}

/*
void MyMoneySecurityTest::testMyMoneyFileConstructor() {
 MyMoneySecurity *t = new MyMoneySecurity("GUID", *n);

 QVERIFY(t->id() == "GUID");

 delete t;
}
*/

void MyMoneySecurityTest::testEquality()
{
  testSetFunctions();
  m->setValue("Key", "Value");

  MyMoneySecurity n;
  n = *m;

  QVERIFY(n == *m);
  n.setName("NewName");
  QVERIFY(!(n == *m));
  n = *m;
  n.setTradingSymbol("NewSymbol");
  QVERIFY(!(n == *m));
  n = *m;
  n.setTradingMarket("NewMarket");
  QVERIFY(!(n == *m));
  n = *m;
  n.setTradingCurrency("NewCurrency");
  QVERIFY(!(n == *m));
  n = *m;
  n.setSecurityType(MyMoneySecurity::SECURITY_CURRENCY);
  QVERIFY(!(n == *m));
  n = *m;
  n.setSmallestAccountFraction(40);
  QVERIFY(!(n == *m));
  n = *m;
  n.setSmallestCashFraction(20);
  QVERIFY(!(n == *m));
  n = *m;
  n.setPartsPerUnit(3);
  QVERIFY(!(n == *m));
  n = *m;
  n.setValue("Key", "NewValue");
  QVERIFY(!(n == *m));
}

void MyMoneySecurityTest::testInequality()
{
  testSetFunctions();
  m->setValue("Key", "Value");

  MyMoneySecurity n;
  n = *m;

  QVERIFY(!(n != *m));
  n.setName("NewName");
  QVERIFY(n != *m);
  n = *m;
  n.setTradingSymbol("NewSymbol");
  QVERIFY(n != *m);
  n = *m;
  n.setTradingMarket("NewMarket");
  QVERIFY(n != *m);
  n = *m;
  n.setTradingCurrency("NewCurrency");
  QVERIFY(n != *m);
  n = *m;
  n.setSecurityType(MyMoneySecurity::SECURITY_CURRENCY);
  QVERIFY(n != *m);
  n = *m;
  n.setSmallestAccountFraction(40);
  QVERIFY(n != *m);
  n = *m;
  n.setSmallestCashFraction(20);
  QVERIFY(n != *m);
  n = *m;
  n.setPartsPerUnit(3);
  QVERIFY(n != *m);
  n = *m;
  n.setValue("Key", "NewValue");
  QVERIFY(n != *m);
}

/*
void MyMoneySecurityTest::testAccountIDList () {
 MyMoneySecurity equity;
 QStringList list;
 QString id;

 // list must be empty
 list = institution.accountList();
 QVERIFY(list.count() == 0);

 // add one account
 institution.addAccountId("A000002");
 list = institution.accountList();
 QVERIFY(list.count() == 1);
 QVERIFY(list.contains("A000002") == 1);

 // adding same account shouldn't make a difference
 institution.addAccountId("A000002");
 list = institution.accountList();
 QVERIFY(list.count() == 1);
 QVERIFY(list.contains("A000002") == 1);

 // now add another account
 institution.addAccountId("A000001");
 list = institution.accountList();
 QVERIFY(list.count() == 2);
 QVERIFY(list.contains("A000002") == 1);
 QVERIFY(list.contains("A000001") == 1);

 id = institution.removeAccountId("A000001");
 QVERIFY(id == "A000001");
 list = institution.accountList();
 QVERIFY(list.count() == 1);
 QVERIFY(list.contains("A000002") == 1);

}
*/
