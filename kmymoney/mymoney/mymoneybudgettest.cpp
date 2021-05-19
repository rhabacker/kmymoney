/***************************************************************************
                          mymoneybudgettest.cpp
                          -------------------
    copyright            : (C) 2010 by Thomas Baumgart
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

#include "mymoneybudgettest.h"

#include <QtTest/QtTest>

#define KMM_MYMONEY_UNIT_TESTABLE friend class MyMoneyBudgetTest;

#include "mymoneybudget.h"

QTEST_MAIN(MyMoneyBudgetTest)

void MyMoneyBudgetTest::init()
{
}

void MyMoneyBudgetTest::cleanup()
{
}

void MyMoneyBudgetTest::addMonthlyToMonthly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a0.addPeriod(MyMoneyDate(2010, 1, 1), period);
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthly);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthly);
  QVERIFY(a0.getPeriods().count() == 1);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.balance() == MyMoneyMoney(200, 1));
  QVERIFY(a1.balance() == MyMoneyMoney(100, 1));
}

void MyMoneyBudgetTest::addMonthlyToYearly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a0.addPeriod(MyMoneyDate(2010, 1, 1), period);
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);

  QVERIFY(a0.totalBalance() == MyMoneyMoney(100, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eYearly);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthly);
  QVERIFY(a0.getPeriods().count() == 1);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(1300, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));
}

void MyMoneyBudgetTest::addMonthlyToMonthByMonth()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);
  MyMoneyDate date(2010, 1, 1);
  for (int i = 0; i < 12; ++i) {
    period.setAmount(MyMoneyMoney((i + 1) * 100, 1));
    a0.addPeriod(date, period);
    date = date.addMonths(1);
    period.setStartDate(date);
  }
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthly);
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(9000, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));
}

void MyMoneyBudgetTest::addYearlyToMonthly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a0.addPeriod(MyMoneyDate(2010, 1, 1), period);
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);

  QVERIFY(a0.totalBalance() == MyMoneyMoney(100, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));

  a1 += a0;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eYearly);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthly);
  QVERIFY(a0.getPeriods().count() == 1);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(100, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1300, 1));
}

void MyMoneyBudgetTest::addYearlyToYearly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a0.addPeriod(MyMoneyDate(2010, 1, 1), period);
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eYearly);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eYearly);
  QVERIFY(a0.getPeriods().count() == 1);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.balance() == MyMoneyMoney(200, 1));
  QVERIFY(a1.balance() == MyMoneyMoney(100, 1));
}

void MyMoneyBudgetTest::addYearlyToMonthByMonth()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);
  MyMoneyDate date(2010, 1, 1);
  for (int i = 0; i < 12; ++i) {
    period.setAmount(MyMoneyMoney((i + 1) * 100, 1));
    a0.addPeriod(date, period);
    date = date.addMonths(1);
    period.setStartDate(date);
  }
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(100, 1));

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eYearly);
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 1);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7900, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(100, 1));
}

void MyMoneyBudgetTest::addMonthByMonthToMonthly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);
  MyMoneyDate date(2010, 1, 1);
  for (int i = 0; i < 12; ++i) {
    period.setAmount(MyMoneyMoney((i + 1) * 100, 1));
    a0.addPeriod(date, period);
    date = date.addMonths(1);
    period.setStartDate(date);
  }
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(1200, 1));

  a1 += a0;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(9000, 1));
}

void MyMoneyBudgetTest::addMonthByMonthToYearly()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eYearly);
  MyMoneyBudget::PeriodGroup period;
  period.setStartDate(MyMoneyDate(2010, 1, 1));
  period.setAmount(MyMoneyMoney(100, 1));
  a1.addPeriod(MyMoneyDate(2010, 1, 1), period);
  MyMoneyDate date(2010, 1, 1);
  for (int i = 0; i < 12; ++i) {
    period.setAmount(MyMoneyMoney((i + 1) * 100, 1));
    a0.addPeriod(date, period);
    date = date.addMonths(1);
    period.setStartDate(date);
  }
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(100, 1));

  a1 += a0;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(7900, 1));
}

void MyMoneyBudgetTest::addMonthByMonthToMonthByMonth()
{
  MyMoneyBudget::AccountGroup a0, a1;
  a0.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  a1.setBudgetLevel(MyMoneyBudget::AccountGroup::eMonthByMonth);
  MyMoneyBudget::PeriodGroup period;
  MyMoneyDate date(2010, 1, 1);
  for (int i = 0; i < 12; ++i) {
    period.setStartDate(date);
    period.setAmount(MyMoneyMoney((i + 1) * 100, 1));
    a0.addPeriod(date, period);
    period.setAmount(MyMoneyMoney((i + 1) * 200, 1));
    a1.addPeriod(date, period);
    date = date.addMonths(1);
  }
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(7800, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(15600, 1));

  a0 += a1;

  QVERIFY(a0.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a1.budgetLevel() == MyMoneyBudget::AccountGroup::eMonthByMonth);
  QVERIFY(a0.getPeriods().count() == 12);
  QVERIFY(a1.getPeriods().count() == 12);
  QVERIFY(a0.totalBalance() == MyMoneyMoney(23400, 1));
  QVERIFY(a1.totalBalance() == MyMoneyMoney(15600, 1));
}
