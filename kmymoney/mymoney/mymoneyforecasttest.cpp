/***************************************************************************
                          mymoneyforecasttest.cpp
                          -------------------
    copyright            : (C) 2007 by Alvaro Soliverez
    email                : asoliverez@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mymoneyforecasttest.h"

#include <iostream>
#include <QList>
#include <QtTest/QtTest>

#include "mymoneybudget.h"

#include "mymoneyexception.h"

#include "mymoneystoragedump.h"
#include "mymoneystoragexml.h"
#include "reportstestcommon.h"

using namespace test;

QTEST_MAIN(MyMoneyForecastTest)

MyMoneyForecastTest::MyMoneyForecastTest()
{
  this->moT1 = MyMoneyMoney(57, 1);
  this->moT2 = MyMoneyMoney(63, 1);
  this->moT3 = MyMoneyMoney(84, 1);
  this->moT4 = MyMoneyMoney(62, 1);
  this->moT5 = MyMoneyMoney(104, 1);
}

void MyMoneyForecastTest::init()
{

  //all this has been taken from pivottabletest.cpp, by Thomas Baumgart and Ace Jones

  storage = new MyMoneySeqAccessMgr;
  file = MyMoneyFile::instance();
  file->attachStorage(storage);

  MyMoneyFileTransaction ft;
  file->addCurrency(MyMoneySecurity("CAD", "Canadian Dollar",        "C$"));
  file->addCurrency(MyMoneySecurity("USD", "US Dollar",              "$"));
  file->addCurrency(MyMoneySecurity("JPY", "Japanese Yen",           QChar(0x00A5), 100, 1));
  file->addCurrency(MyMoneySecurity("GBP", "British Pound",           "#"));
  file->setBaseCurrency(file->currency("USD"));

  MyMoneyPayee payeeTest("Test Payee");
  file->addPayee(payeeTest);
  MyMoneyPayee payeeTest2("Alvaro Soliverez");
  file->addPayee(payeeTest2);

  acAsset = (MyMoneyFile::instance()->asset().id());
  acLiability = (MyMoneyFile::instance()->liability().id());
  acExpense = (MyMoneyFile::instance()->expense().id());
  acIncome = (MyMoneyFile::instance()->income().id());
  acChecking = makeAccount(QString("Checking Account"), MyMoneyAccount::Checkings, moCheckingOpen, MyMoneyDate(2004, 5, 15), acAsset, "USD");
  acCredit = makeAccount(QString("Credit Card"), MyMoneyAccount::CreditCard, moCreditOpen, MyMoneyDate(2004, 7, 15), acLiability, "USD");
  acSolo = makeAccount(QString("Solo"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 1, 11), acExpense, "USD");
  acParent = makeAccount(QString("Parent"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 1, 11), acExpense, "USD");
  acChild = makeAccount(QString("Child"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 2, 11), acParent, "USD");
  acForeign = makeAccount(QString("Foreign"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 1, 11), acExpense, "USD");
  acInvestment = makeAccount("Investment", MyMoneyAccount::Investment, moZero, MyMoneyDate(2004, 1, 1), acAsset, "USD");

  acSecondChild = makeAccount(QString("Second Child"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 2, 11), acParent, "USD");
  acGrandChild1 = makeAccount(QString("Grand Child 1"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 2, 11), acChild, "USD");
  acGrandChild2 = makeAccount(QString("Grand Child 2"), MyMoneyAccount::Expense, MyMoneyMoney(), MyMoneyDate(2004, 2, 11), acChild, "USD");

  //this account added to have an account to test opening date calculations
  acCash = makeAccount(QString("Cash"), MyMoneyAccount::Cash, moCreditOpen, MyMoneyDate::currentDate().addDays(-2), acAsset, "USD");


  MyMoneyInstitution i("Bank of the World", "", "", "", "", "", "");
  file->addInstitution(i);
  inBank = i.id();
  ft.commit();

}

void MyMoneyForecastTest::cleanup()
{
  file->detachStorage(storage);
  delete storage;
}

void MyMoneyForecastTest::testEmptyConstructor()
{
  MyMoneyForecast a;
  MyMoneyAccount b;

  QVERIFY(a.forecastBalance(b, MyMoneyDate::currentDate()).isZero());
  QVERIFY(!a.isForecastAccount(b));
  QVERIFY(a.forecastBalance(b, MyMoneyDate::currentDate()) == MyMoneyMoney());
  QVERIFY(a.daysToMinimumBalance(b) == -1);
  QVERIFY(a.daysToZeroBalance(b) == -2);
  QVERIFY(a.forecastDays() == 90);
  QVERIFY(a.accountsCycle() == 30);
  QVERIFY(a.forecastCycles() == 3);
  QVERIFY(a.historyStartDate() == MyMoneyDate::currentDate().addDays(-3*30));
  QVERIFY(a.historyEndDate() == MyMoneyDate::currentDate().addDays(-1));
  QVERIFY(a.historyDays() == 30 * 3);
}


void MyMoneyForecastTest::testDoForecastInit()
{
  MyMoneyForecast a;

  a.doForecast();
  /*
  //check the illegal argument validation
  try {
    KMyMoneyGlobalSettings::setForecastDays(-10);
    a.doForecast();
  }
  catch (const MyMoneyException &e)
  {
    QFAIL("Unexpected exception");
  }
  try {
    KMyMoneyGlobalSettings::setForecastAccountCycle(-20);
      a.doForecast();
    }
    catch (const MyMoneyException &e) {
      QFAIL("Unexpected exception");
  }
  try {
    KMyMoneyGlobalSettings::setForecastCycles(-10);
    a.doForecast();
  }
  catch (const MyMoneyException &e) {
    QFAIL("Unexpected exception");
  }

  try {
    KMyMoneyGlobalSettings::setForecastAccountCycle(0);
    a.doForecast();
  }
  catch (const MyMoneyException &e) {
    QFAIL("Unexpected exception");
  }
  try {
    KMyMoneyGlobalSettings::setForecastDays(0);
    KMyMoneyGlobalSettings::setForecastCycles(0);
    KMyMoneyGlobalSettings::setForecastAccountCycle(0);
    a.doForecast();
  }
  catch (const MyMoneyException &e) {
    QVERIFY("Unexpected exception");
  }*/
}

//test that it forecasts correctly with transactions in the period of forecast
void MyMoneyForecastTest::testDoForecast()
{
  //set up environment
  MyMoneyForecast a;

  MyMoneyAccount a_checking = file->account(acChecking);
  MyMoneyAccount a_credit = file->account(acCredit);

  //test empty forecast
  a.doForecast(); //this is just to check nothing goes wrong if forecast is run agains an empty template

  //setup some transactions
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT1, acChecking, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionDeposit, -(this->moT2), acCredit, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionTransfer, this->moT1, acCredit, acChecking);

  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.setBeginForecastDay(0);
  a.setHistoryMethod(0); //moving average
  a.doForecast();

  //checking didn't have balance variations, so the forecast should be equal to the current balance
  MyMoneyMoney b_checking = file->balance(a_checking.id(), MyMoneyDate::currentDate());

  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(1)) == b_checking);
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(2)) == b_checking);
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(3)) == b_checking);
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate()) == b_checking);
  //credit had a variation so the forecast should be different for each day
  MyMoneyMoney b_credit = file->balance(a_credit.id(), MyMoneyDate::currentDate());

  QVERIFY(a.forecastBalance(a_credit, 0) == b_credit);
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(1)) == (b_credit + (moT2 - moT1)));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(2)) == (b_credit + ((moT2 - moT1)*2)));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(3)) == b_credit + ((moT2 - moT1)*3));

  a.setHistoryMethod(1); //weighted moving average
  a.doForecast();

  QVERIFY(a.forecastBalance(a_credit, 0) == b_credit);
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(1)) == (b_credit + (moT2 - moT1)));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(2)) == (b_credit + ((moT2 - moT1)*2)));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(3)) == b_credit + ((moT2 - moT1)*3));

  //insert transactions outside the forecast period. The calculation should be the same.
  TransactionHelper t4(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionDeposit, -moT2, acCredit, acParent);
  TransactionHelper t5(MyMoneyDate::currentDate().addDays(-10), MyMoneySplit::ActionDeposit, -moT2, acCredit, acParent);
  TransactionHelper t6(MyMoneyDate::currentDate().addDays(-3), MyMoneySplit::ActionDeposit, -moT2, acCredit, acParent);

  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.setBeginForecastDay(0);
  a.setHistoryMethod(0); //moving average
  a.doForecast();
  //check forecast
  b_credit = file->balance(a_credit.id(), MyMoneyDate::currentDate());
  MyMoneyMoney b_credit_1_exp = (b_credit + ((moT2 - moT1)));
  MyMoneyMoney b_credit_2 = a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(2));
  MyMoneyMoney b_credit_2_exp = (b_credit + ((moT2 - moT1) * 2));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate()) == file->balance(a_credit.id(), MyMoneyDate::currentDate()));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(1)) == b_credit + (moT2 - moT1));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(2)) == b_credit + ((moT2 - moT1)*2));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(3)) == b_credit + ((moT2 - moT1)*3));

  //test weighted moving average
  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(3);
  a.setBeginForecastDay(0);
  a.setHistoryMethod(1);
  a.doForecast();

  QVERIFY(a.forecastBalance(a_credit, 0) == b_credit);
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(1)) == (b_credit + (((moT2 - moT1)*3 + moT2*2 + moT2) / MyMoneyMoney(6, 1))));

}

void MyMoneyForecastTest::testGetForecastAccountList()
{
  MyMoneyForecast a;
  MyMoneyAccount a_checking = file->account(acChecking);
  MyMoneyAccount a_parent = file->account(acParent);
  QList<MyMoneyAccount> b;

  b = a.forecastAccountList();
  //check that it contains asset account, but not expense accounts
  QVERIFY(b.contains(a_checking));
  QVERIFY(!b.contains(a_parent));

}

void MyMoneyForecastTest::testCalculateAccountTrend()
{
  //set up environment
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-3), MyMoneySplit::ActionDeposit, -moT2, acChecking, acSolo);
  MyMoneyAccount a_checking = file->account(acChecking);

  //test invalid arguments

  try {
    MyMoneyForecast::calculateAccountTrend(a_checking, 0);
  } catch (const MyMoneyException &e) {
    QVERIFY(e.what().compare("Illegal arguments when calling calculateAccountTrend. trendDays must be higher than 0") == 0);
  }
  try {
    MyMoneyForecast::calculateAccountTrend(a_checking, -10);
  } catch (const MyMoneyException &e) {
    QVERIFY(e.what().compare("Illegal arguments when calling calculateAccountTrend. trendDays must be higher than 0") == 0);
  }

  //test that it calculates correctly
  QVERIFY(MyMoneyForecast::calculateAccountTrend(a_checking , 3) == moT2 / MyMoneyMoney(3, 1));

  //test that it works for all kind of accounts
  MyMoneyAccount a_solo = file->account(acSolo);
  MyMoneyMoney soloTrend = MyMoneyForecast::calculateAccountTrend(a_solo, 3);
  MyMoneyMoney soloTrendExp = -moT2 / MyMoneyMoney(3, 1);
  QVERIFY(MyMoneyForecast::calculateAccountTrend(a_solo, 3) == -moT2 / MyMoneyMoney(3, 1));

  //test that it does not take into account the transactions of the opening date of the account
  MyMoneyAccount a_cash = file->account(acCash);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionDeposit, moT2, acCash, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionDeposit, moT1, acCash, acParent);
  QVERIFY(MyMoneyForecast::calculateAccountTrend(a_cash, 3) == -moT1);

}

void MyMoneyForecastTest::testGetForecastBalance()
{
  //set up environment
  MyMoneyForecast a;

  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT1, acChecking, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionDeposit, -(this->moT2), acCredit, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionTransfer, this->moT1, acCredit, acChecking);

  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.setHistoryMethod(0);
  a.doForecast();

  MyMoneyAccount a_checking = file->account(acChecking);
  MyMoneyAccount a_credit = file->account(acCredit);

  //test invalid arguments
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(-1)) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(-10)) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, -1) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, -100) == MyMoneyMoney());

  //test a date outside the forecast days
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(4)) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, 4) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, MyMoneyDate::currentDate().addDays(10)) == MyMoneyMoney());
  QVERIFY(a.forecastBalance(a_checking, 10) == MyMoneyMoney());

  //test it returns valid results
  MyMoneyMoney b_credit = file->balance(a_credit.id(), MyMoneyDate::currentDate());
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate()) == file->balance(a_credit.id(), MyMoneyDate::currentDate()));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(1)) == b_credit + (moT2 - moT1));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(2)) == b_credit + ((moT2 - moT1)*2));
  QVERIFY(a.forecastBalance(a_credit, MyMoneyDate::currentDate().addDays(3)) == b_credit + ((moT2 - moT1)*3));
}

void MyMoneyForecastTest::testIsForecastAccount()
{
  MyMoneyForecast a;

  MyMoneyAccount a_checking = file->account(acChecking);
  MyMoneyAccount a_solo = file->account(acSolo);
  MyMoneyAccount a_investment = file->account(acInvestment);

  //test an invalid account
  QVERIFY(a.isForecastAccount(a_solo) == false);
  QVERIFY(a.isForecastAccount(a_investment) == true);

  //test a valid account
  QVERIFY(a.isForecastAccount(a_checking) == true);

}

void MyMoneyForecastTest::testDoFutureScheduledForecast()
{
  //set up future transactions
  MyMoneyForecast a;

  MyMoneyAccount a_cash = file->account(acCash);
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(1), MyMoneySplit::ActionDeposit, -moT1, acCash, acParent);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(2), MyMoneySplit::ActionDeposit, -moT2, acCash, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(3), MyMoneySplit::ActionDeposit, -moT3, acCash, acParent);
  TransactionHelper t4(MyMoneyDate::currentDate().addDays(10), MyMoneySplit::ActionDeposit, -moT4, acCash, acParent);

  a.setForecastMethod(0);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.doForecast();

  MyMoneyMoney b_cash = file->balance(a_cash.id(), MyMoneyDate::currentDate());

  //test valid results
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate()) == b_cash);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(1)) == b_cash + moT1);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(2)) == b_cash + moT1 + moT2);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(3)) == b_cash + moT1 + moT2 + moT3);
}

void MyMoneyForecastTest::testScheduleForecast()
{
  //set up schedule environment for testing
  MyMoneyAccount a_cash = file->account(acCash);
  MyMoneyAccount a_parent = file->account(acParent);

  MyMoneyFileTransaction ft;
  MyMoneySchedule sch("A Name",
                      MyMoneySchedule::TYPE_BILL,
                      MyMoneySchedule::OCCUR_WEEKLY, 1,
                      MyMoneySchedule::STYPE_DIRECTDEBIT,
                      MyMoneyDate::currentDate().addDays(1),
                      MyMoneyDate(),
                      true,
                      true);

  MyMoneyTransaction t;
  t.setPostDate(MyMoneyDate::currentDate().addDays(1));
  t.setEntryDate(MyMoneyDate::currentDate().addDays(1));
  //t.setId("T000000000000000001");
  t.setBankID("BID");
  t.setMemo("Wohnung:Miete");
  t.setCommodity("USD");
  t.setValue("key", "value");

  MyMoneySplit s;
  s.setPayeeId("P000001");
  s.setShares(moT2);
  s.setValue(moT2);
  s.setAccountId(a_parent.id());
  s.setBankID("SPID1");
  s.setReconcileFlag(MyMoneySplit::Reconciled);
  t.addSplit(s);

  s.setPayeeId("P000001");
  s.setShares(-moT2);
  s.setValue(-moT2);
  s.setAccountId(a_cash.id());
  s.setBankID("SPID2");
  s.setReconcileFlag(MyMoneySplit::Cleared);
  s.clearId();
  t.addSplit(s);

  sch.setTransaction(t);

  file->addSchedule(sch);
  ft.commit();

  MyMoneyFileTransaction ft3;
  MyMoneySchedule sch3("A Name1",
                       MyMoneySchedule::TYPE_BILL,
                       MyMoneySchedule::OCCUR_WEEKLY, 1,
                       MyMoneySchedule::STYPE_DIRECTDEBIT,
                       MyMoneyDate::currentDate().addDays(5),
                       MyMoneyDate(),
                       true,
                       true);

  //sch.setLastPayment(MyMoneyDate::currentDate());
  //sch.recordPayment(MyMoneyDate::currentDate().addDays(1));
  //sch.setId("SCH0001");

  MyMoneyTransaction t3;
  t3.setPostDate(MyMoneyDate::currentDate().addDays(5));
  t3.setEntryDate(MyMoneyDate::currentDate().addDays(5));
  //t.setId("T000000000000000001");
  t3.setBankID("BID");
  t3.setMemo("Wohnung:Miete");
  t3.setCommodity("USD");
  t3.setValue("key", "value");

  MyMoneySplit s3;
  s3.setPayeeId("P000001");
  s3.setShares(moT2);
  s3.setValue(moT2);
  s3.setAccountId(a_parent.id());
  s3.setBankID("SPID1");
  s3.setReconcileFlag(MyMoneySplit::Reconciled);
  t3.addSplit(s3);

  s3.setPayeeId("P000001");
  s3.setShares(-moT2);
  s3.setValue(-moT2);
  s3.setAccountId(a_cash.id());
  s3.setBankID("SPID2");
  s3.setReconcileFlag(MyMoneySplit::Cleared);
  s3.clearId();
  t3.addSplit(s3);

  sch3.setTransaction(t3);

  file->addSchedule(sch3);
  ft3.commit();


  MyMoneyFileTransaction ft2;
  MyMoneySchedule sch2("A Name2",
                       MyMoneySchedule::TYPE_BILL,
                       MyMoneySchedule::OCCUR_WEEKLY, 1,
                       MyMoneySchedule::STYPE_DIRECTDEBIT,
                       MyMoneyDate::currentDate().addDays(2),
                       MyMoneyDate(),
                       true,
                       true);

  //sch.setLastPayment(MyMoneyDate::currentDate());
  //sch.recordPayment(MyMoneyDate::currentDate().addDays(1));
  //sch.setId("SCH0001");

  MyMoneyTransaction t2;
  t2.setPostDate(MyMoneyDate::currentDate().addDays(2));
  t2.setEntryDate(MyMoneyDate::currentDate().addDays(2));
  //t.setId("T000000000000000001");
  t2.setBankID("BID");
  t2.setMemo("Wohnung:Miete");
  t2.setCommodity("USD");
  t2.setValue("key", "value");

  MyMoneySplit s2;
  s2.setPayeeId("P000001");
  s2.setShares(moT1);
  s2.setValue(moT1);
  s2.setAccountId(a_parent.id());
  s2.setBankID("SPID1");
  s2.setReconcileFlag(MyMoneySplit::Reconciled);
  t2.addSplit(s2);

  s2.setPayeeId("P000001");
  s2.setShares(-moT1);
  s2.setValue(-moT1);
  s2.setAccountId(a_cash.id());
  s2.setBankID("SPID2");
  s2.setReconcileFlag(MyMoneySplit::Cleared);
  s2.clearId();
  t2.addSplit(s2);

  sch2.setTransaction(t2);

  file->addSchedule(sch2);

  ft2.commit();

  //run forecast
  MyMoneyForecast a;
  a.setForecastMethod(0);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.doForecast();

  //check result for single schedule
  MyMoneyMoney b_cash = file->balance(a_cash.id(), MyMoneyDate::currentDate());
  MyMoneyMoney b_cash1 = a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(1));

  //test valid results
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate()) == b_cash);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(1)) == b_cash - moT2);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(2)) == b_cash - moT2 - moT1);
}


void MyMoneyForecastTest::testDaysToMinimumBalance()
{
  //setup environment
  MyMoneyForecast a;

  MyMoneyAccount a_cash = file->account(acCash);
  MyMoneyAccount a_credit = file->account(acCredit);
  MyMoneyAccount a_parent = file->account(acParent);
  a_cash.setValue("minBalanceAbsolute", "50");
  a_credit.setValue("minBalanceAbsolute", "50");
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionDeposit, -moT1, acCash, acParent);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(2), MyMoneySplit::ActionDeposit, moT2, acCash, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, -moT1, acCredit, acParent);
  TransactionHelper t4(MyMoneyDate::currentDate().addDays(4), MyMoneySplit::ActionWithdrawal, moT5, acCredit, acParent);

  a.setForecastMethod(0);
  a.setForecastDays(3);
  a.setAccountsCycle(1);
  a.setForecastCycles(1);
  a.setBeginForecastDay(0);
  a.doForecast();

  //test invalid arguments
  MyMoneyAccount nullAcc;
  QVERIFY(a.daysToMinimumBalance(nullAcc) == -1);

  //test when not a forecast account
  QVERIFY(a.daysToMinimumBalance(a_parent) == -1);

  //test it warns when inside the forecast period
  QVERIFY(a.daysToMinimumBalance(a_cash) == 2);

  //test it does not warn when it will be outside of the forecast period
  QVERIFY(a.daysToMinimumBalance(a_credit) == -1);
}
void MyMoneyForecastTest::testDaysToZeroBalance()
{
  //set up environment
  MyMoneyAccount a_Solo = file->account(acSolo);
  MyMoneyAccount a_Cash = file->account(acCash);
  MyMoneyAccount a_Credit = file->account(acCredit);

  //MyMoneyFileTransaction ft;
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(2), MyMoneySplit::ActionWithdrawal, -moT1, acChecking, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(2), MyMoneySplit::ActionTransfer, (moT5), acCash, acCredit);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(2), MyMoneySplit::ActionWithdrawal, (moT5*100), acCredit, acParent);
  //ft.commit();

  MyMoneyForecast a;
  a.setForecastMethod(0);
  a.setForecastDays(30);
  a.setAccountsCycle(1);
  a.setForecastCycles(3);
  a.doForecast();

  //test invalid arguments
  MyMoneyAccount nullAcc;
  try {
    a.daysToZeroBalance(nullAcc);
  } catch (const MyMoneyException &) {
    QFAIL("Unexpected exception");
  }

  //test when not a forecast account
  MyMoneyAccount a_solo = file->account(acSolo);
  int iSolo = a.daysToZeroBalance(a_Solo);

  QVERIFY(iSolo == -2);

  //test it warns when inside the forecast period

  MyMoneyMoney fCash = a.forecastBalance(a_Cash, MyMoneyDate::currentDate().addDays(2));

  QVERIFY(a.daysToZeroBalance(a_Cash) == 2);

  //test it does not warn when it will be outside of the forecast period

}

void MyMoneyForecastTest::testSkipOpeningDate()
{
  //set up environment
  MyMoneyForecast a;

  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acSolo);

  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(2);
  a.setForecastCycles(1);
  a.setHistoryMethod(0);
  a.doForecast();

  MyMoneyAccount a_cash = file->account(acCash);

  //test it has no variation because it skipped the variation of the opening date
  MyMoneyMoney b_cash = file->balance(a_cash.id(), MyMoneyDate::currentDate());
  QVERIFY(a.skipOpeningDate() == true);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate()) == b_cash);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(1)) == b_cash);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(2)) == b_cash - moT2);
  QVERIFY(a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(3)) == b_cash - moT2);
}

void MyMoneyForecastTest::testAccountMinimumBalanceDateList()
{

  //set up environment
  MyMoneyForecast a;

  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acSolo);

  a.setForecastMethod(1);
  a.setForecastDays(6);
  a.setAccountsCycle(2);
  a.setForecastCycles(3);
  a.setHistoryMethod(0);
  a.setBeginForecastDay(MyMoneyDate::currentDate().addDays(1).day());
  a.doForecast();

  MyMoneyAccount a_cash = file->account(acCash);

  //test
  QList<MyMoneyDate> dateList;
  dateList = a.accountMinimumBalanceDateList(a_cash);

  QList<MyMoneyDate>::iterator it = dateList.begin();

  MyMoneyDate minDate = *it;

  QVERIFY(minDate == MyMoneyDate::currentDate().addDays(2));
  it++;
  minDate = *it;
  QVERIFY(minDate == MyMoneyDate::currentDate().addDays(4));
  it++;
  minDate = *it;
  QVERIFY(minDate == MyMoneyDate::currentDate().addDays(6));

}

void MyMoneyForecastTest::testAccountMaximumBalanceDateList()
{
  //set up environment
  MyMoneyForecast a;

  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acSolo);

  a.setForecastMethod(1);
  a.setForecastDays(6);
  a.setAccountsCycle(2);
  a.setForecastCycles(3);
  a.setHistoryMethod(0);
  a.setBeginForecastDay(MyMoneyDate::currentDate().addDays(1).day());
  a.doForecast();

  MyMoneyAccount a_cash = file->account(acCash);

  //test
  QList<MyMoneyDate> dateList;
  dateList = a.accountMaximumBalanceDateList(a_cash);

  QList<MyMoneyDate>::iterator it = dateList.begin();

  MyMoneyDate maxDate = *it;

  QVERIFY(maxDate == MyMoneyDate::currentDate().addDays(1));
  it++;
  maxDate = *it;
  QVERIFY(maxDate == MyMoneyDate::currentDate().addDays(3));
  it++;
  maxDate = *it;
  QVERIFY(maxDate == MyMoneyDate::currentDate().addDays(5));


}

void MyMoneyForecastTest::testAccountAverageBalance()
{
  //set up environment
  MyMoneyForecast a;

  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-2), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acSolo);

  a.setForecastMethod(1);
  a.setForecastDays(3);
  a.setAccountsCycle(2);
  a.setForecastCycles(1);
  a.setBeginForecastDay(0);
  a.doForecast();

  MyMoneyAccount a_cash = file->account(acCash);

  //test
  MyMoneyMoney b_cash1 = a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(1));
  MyMoneyMoney b_cash2 = a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(2));
  MyMoneyMoney b_cash3 = a.forecastBalance(a_cash, MyMoneyDate::currentDate().addDays(3));

  MyMoneyMoney average = (b_cash1 + b_cash2 + b_cash3) / MyMoneyMoney(3, 1);


  QVERIFY(a.accountAverageBalance(a_cash) == average);
}

void MyMoneyForecastTest::testBeginForecastDate()
{
  //set up environment
  MyMoneyForecast a;
  MyMoneyDate beginDate;
  int beginDay;

  a.setForecastMethod(1);
  a.setForecastDays(90);
  a.setAccountsCycle(14);
  a.setForecastCycles(3);
  a.setBeginForecastDay(0);
  a.doForecast();

  //test when using old method without begin day
  QVERIFY(MyMoneyDate::currentDate() == a.beginForecastDate());

  //setup begin to last day of month
  a.setBeginForecastDay(31);
  beginDay = a.beginForecastDay();
  a.doForecast();

  //test
  if (MyMoneyDate::currentDate().day() < beginDay) {
    if (MyMoneyDate::currentDate().daysInMonth() < beginDay)
      beginDay = MyMoneyDate::currentDate().daysInMonth();

    beginDate = MyMoneyDate(MyMoneyDate::currentDate().year(), MyMoneyDate::currentDate().month(), beginDay);

    QVERIFY(beginDate == a.beginForecastDate());
  }

  //setup begin day to same date
  a.setBeginForecastDay(MyMoneyDate::currentDate().day());
  beginDay = a.beginForecastDay();
  a.doForecast();

  QVERIFY(MyMoneyDate::currentDate() == a.beginForecastDate());

  //setup to first day of month with small interval
  a.setBeginForecastDay(1);
  a.setAccountsCycle(1);
  beginDay = a.beginForecastDay();
  a.doForecast();

  //test
  if (MyMoneyDate::currentDate() == a.beginForecastDate()) {
    QVERIFY(MyMoneyDate::currentDate() == a.beginForecastDate());
  } else {
    beginDay = ((((MyMoneyDate::currentDate().day() - beginDay) / a.accountsCycle()) + 1) * a.accountsCycle()) + beginDay;
    if (beginDay > MyMoneyDate::currentDate().daysInMonth())
      beginDay = MyMoneyDate::currentDate().daysInMonth();
    beginDate = MyMoneyDate(MyMoneyDate::currentDate().year(), MyMoneyDate::currentDate().month(), beginDay);
    if (MyMoneyDate::currentDate().day() == MyMoneyDate::currentDate().daysInMonth()) {
      std::cout << std::endl << "testBeginForecastDate(): test of first day of month with small interval skipped because it is the last day of month" << std::endl;
    } else {
      QVERIFY(beginDate == a.beginForecastDate());
    }
  }

  //setup to test when current date plus cycle equals begin day
  a.setAccountsCycle(14);
  beginDay = MyMoneyDate::currentDate().addDays(14).day();
  a.setBeginForecastDay(beginDay);
  beginDate = MyMoneyDate::currentDate().addDays(14);
  a.doForecast();

  //test
  QVERIFY(beginDate == a.beginForecastDate());

  //setup to test when the begin day will be next month
  a.setBeginForecastDay(1);
  a.setAccountsCycle(40);
  a.doForecast();

  beginDate = MyMoneyDate(MyMoneyDate::currentDate().addMonths(1).year(), MyMoneyDate::currentDate().addMonths(1).month(), 1);

  //test
  if (MyMoneyDate::currentDate().day() > 1) {
    QVERIFY(beginDate == a.beginForecastDate());
  } else {
    //test is not valid if today is 1st of month
    std::cout << std::endl << "testBeginForecastDate(): test of first day of month skipped because current day is 1st of month" << std::endl;
  }
}

void MyMoneyForecastTest::testHistoryDays()
{
  MyMoneyForecast a;

  QVERIFY(a.historyStartDate() == MyMoneyDate::currentDate().addDays(-a.forecastCycles()*a.accountsCycle()));
  QVERIFY(a.historyEndDate() == MyMoneyDate::currentDate().addDays(-1));
  QVERIFY(a.historyDays() == a.forecastCycles()*a.accountsCycle());

  a.setForecastMethod(1);
  a.setForecastDays(90);
  a.setAccountsCycle(14);
  a.setForecastCycles(3);
  a.setBeginForecastDay(0);
  a.doForecast();

  QVERIFY(a.historyStartDate() == MyMoneyDate::currentDate().addDays(-14*3));
  QVERIFY(a.historyDays() == (14*3));
  QVERIFY(a.historyEndDate() == (MyMoneyDate::currentDate().addDays(-1)));
}

void MyMoneyForecastTest::testCreateBudget()
{
  //set up environment
  MyMoneyForecast a;
  MyMoneyForecast b;
  MyMoneyBudget budget;

  TransactionHelper t1(MyMoneyDate(2005, 1, 3), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t2(MyMoneyDate(2005, 1, 15), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acParent);
  TransactionHelper t3(MyMoneyDate(2005, 1, 30), MyMoneySplit::ActionWithdrawal, this->moT3, acCash, acSolo);
  TransactionHelper t4(MyMoneyDate(2006, 1, 25), MyMoneySplit::ActionWithdrawal, this->moT4, acCash, acParent);
  TransactionHelper t5(MyMoneyDate(2005, 4, 3), MyMoneySplit::ActionWithdrawal, this->moT1, acCash, acSolo);
  TransactionHelper t6(MyMoneyDate(2006, 5, 15), MyMoneySplit::ActionWithdrawal, this->moT2, acCash, acParent);
  TransactionHelper t7(MyMoneyDate(2005, 8, 3), MyMoneySplit::ActionWithdrawal, this->moT3, acCash, acSolo);
  TransactionHelper t8(MyMoneyDate(2006, 9, 15), MyMoneySplit::ActionWithdrawal, this->moT4, acCash, acParent);

  a.setHistoryMethod(0);
  a.setForecastMethod(1);
  a.createBudget(budget, MyMoneyDate(2005, 1, 1), MyMoneyDate(2006, 12, 31), MyMoneyDate(2007, 1, 1), MyMoneyDate(2007, 12, 31), true);

  //test
  MyMoneyAccount a_solo = file->account(acSolo);
  MyMoneyAccount a_parent = file->account(acParent);

  //test it has no variation because it skipped the variation of the opening date
  QVERIFY(a.forecastBalance(a_solo, MyMoneyDate(2007, 1, 1)) == ((moT1 + moT3) / MyMoneyMoney(2, 1)));
  QVERIFY(a.forecastBalance(a_parent, MyMoneyDate(2007, 1, 1)) == ((moT2 + moT4) / MyMoneyMoney(2, 1)));
  QVERIFY(a.forecastBalance(a_solo, MyMoneyDate(2007, 4, 1)) == ((moT1) / MyMoneyMoney(2, 1)));
  QVERIFY(a.forecastBalance(a_parent, MyMoneyDate(2007, 5, 1)) == ((moT2) / MyMoneyMoney(2, 1)));
  QVERIFY(a.forecastBalance(a_solo, MyMoneyDate(2007, 8, 1)) == ((moT3) / MyMoneyMoney(2, 1)));
  QVERIFY(a.forecastBalance(a_parent, MyMoneyDate(2007, 9, 1)) == ((moT4) / MyMoneyMoney(2, 1)));
  //test the budget object returned by the method
  QVERIFY(budget.account(a_parent.id()).period(MyMoneyDate(2007, 9, 1)).amount() == ((moT4) / MyMoneyMoney(2, 1)));

  //setup test for a length lower than a year
  b.setForecastMethod(1);
  b.setHistoryMethod(0);
  b.createBudget(budget, MyMoneyDate(2005, 1, 1), MyMoneyDate(2005, 6, 30), MyMoneyDate(2007, 1, 1), MyMoneyDate(2007, 6, 30), true);

  //test
  QVERIFY(b.forecastBalance(a_solo, MyMoneyDate(2007, 1, 1)) == (moT1 + moT3));
  QVERIFY(b.forecastBalance(a_parent, MyMoneyDate(2007, 1, 1)) == (moT2));
  QVERIFY(b.forecastBalance(a_solo, MyMoneyDate(2007, 4, 1)) == (moT1));
  QVERIFY(b.forecastBalance(a_parent, MyMoneyDate(2007, 5, 1)) == (MyMoneyMoney()));

  //set up schedule environment for testing
  MyMoneyAccount a_cash = file->account(acCash);

  MyMoneyFileTransaction ft;
  MyMoneySchedule sch("A Name",
                      MyMoneySchedule::TYPE_BILL,
                      MyMoneySchedule::OCCUR_MONTHLY, 1,
                      MyMoneySchedule::STYPE_DIRECTDEBIT,
                      MyMoneyDate::currentDate(),
                      MyMoneyDate(),
                      true,
                      true);

  MyMoneyTransaction t10;
  t10.setPostDate(MyMoneyDate::currentDate().addMonths(1));
  t10.setEntryDate(MyMoneyDate::currentDate().addMonths(1));
  //t.setId("T000000000000000001");
  t10.setBankID("BID");
  t10.setMemo("Wohnung:Miete");
  t10.setCommodity("USD");
  t10.setValue("key", "value");

  MyMoneySplit s;
  s.setPayeeId("P000001");
  s.setShares(moT2);
  s.setValue(moT2);
  s.setAccountId(a_parent.id());
  s.setBankID("SPID1");
  s.setReconcileFlag(MyMoneySplit::Reconciled);
  t10.addSplit(s);

  s.setPayeeId("P000001");
  s.setShares(-moT2);
  s.setValue(-moT2);
  s.setAccountId(a_cash.id());
  s.setBankID("SPID2");
  s.setReconcileFlag(MyMoneySplit::Cleared);
  s.clearId();
  t10.addSplit(s);

  sch.setTransaction(t10);

  file->addSchedule(sch);
  ft.commit();

  //run forecast
  MyMoneyForecast c;
  c.setForecastMethod(0);
  c.setForecastCycles(1);
  c.createBudget(budget, MyMoneyDate::currentDate().addYears(-2), MyMoneyDate::currentDate().addYears(-1), MyMoneyDate::currentDate().addMonths(-2), MyMoneyDate::currentDate().addMonths(6), true);

  MyMoneyMoney c_parent = c.forecastBalance(a_parent, MyMoneyDate(MyMoneyDate::currentDate().addMonths(1).year(), MyMoneyDate::currentDate().addMonths(1).month(), 1));

  //test valid results
  QVERIFY(c.forecastBalance(a_parent, MyMoneyDate(MyMoneyDate::currentDate().addMonths(1).year(), MyMoneyDate::currentDate().addMonths(1).month(), 1)) == (moT2));
}

void MyMoneyForecastTest::testLinearRegression()
{
  //set up environment
  MyMoneyForecast a;

  MyMoneyAccount a_checking = file->account(acChecking);
  MyMoneyAccount a_credit = file->account(acCredit);

  //setup some transactions
  TransactionHelper t1(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionWithdrawal, this->moT1, acChecking, acSolo);
  TransactionHelper t2(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionDeposit, -(this->moT2), acCredit, acParent);
  TransactionHelper t3(MyMoneyDate::currentDate().addDays(-1), MyMoneySplit::ActionTransfer, this->moT1, acCredit, acChecking);

//TODO Add tests specific for linear regression


}
