/*
 * This file is part of KMyMoney, A Personal Finance Manager for KDE
 * Copyright (C) 2017 Ralf Habacker <ralf.habacker@freenet.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kmymoneyapptest.h"

#include "QApplication"
#include "kmymoney.h"
#include "mymoneyfile.h"

#include <QtTest/QtTest>

QTEST_MAIN(KMyMoneyAppTest)

KMyMoneyApp *kmymoney = 0;
bool timersOn = false;
QApplication* a = 0;

void KMyMoneyAppTest::init()
{
  char *argv[1] = { 0 };
  int argc = 1;
  a = new QApplication(argc, argv, 0);
  m_app = new KMyMoneyApp();
  kmymoney = m_app;
}

void KMyMoneyAppTest::cleanup()
{
  delete m_app;
  kmymoney = 0;
}

void KMyMoneyAppTest::initTestCase()
{
}

void KMyMoneyAppTest::testFindAccount()
{
  MyMoneyAccount newAccount;
  MyMoneyAccount parentAccount = MyMoneyFile::instance()->asset();
  MyMoneyAccount brokerageAccount;
  MyMoneyMoney openingBalance;
  newAccount.setName("Auto");
  newAccount.setAccountType(MyMoneyAccount::Asset);
  m_app->createAccount(newAccount, parentAccount, brokerageAccount, openingBalance);
  QVERIFY(!m_app->findAccount(newAccount, parentAccount).id().isEmpty());

  MyMoneyAccount subAccount;
  subAccount.setName("Auto:Account");
  subAccount.setAccountType(MyMoneyAccount::Asset);
  m_app->createAccount(subAccount, parentAccount, brokerageAccount, openingBalance);
  QVERIFY(!m_app->findAccount(subAccount, parentAccount).id().isEmpty());

  subAccount.setAccountType(MyMoneyAccount::Liability);
  QVERIFY(!m_app->findAccount(subAccount, parentAccount).id().isEmpty());

  MyMoneyAccount subTestAccount;
  subTestAccount.setName("Account");
  subTestAccount.setAccountType(MyMoneyAccount::Asset);
  MyMoneyAccount parent = MyMoneyFile::instance()->accountByName("Auto");
  subTestAccount.setParentAccountId(parent.id());
  QVERIFY(!m_app->findAccount(subTestAccount, parent).id().isEmpty());
}
