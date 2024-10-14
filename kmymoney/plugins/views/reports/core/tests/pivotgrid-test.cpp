/*
    SPDX-FileCopyrightText: 2005-2018 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-FileCopyrightText: 2005-2006 Ace Jones <acejones@users.sourceforge.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pivotgrid-test.h"

#include <KLocalizedString>

#include <QTest>

#include <alkimia/alkdom.h>

#include "tests/testutilities.h"

#include "pivotgrid.h"
#include "mymoneyinstitution.h"
#include "mymoneysecurity.h"
#include "mymoneypayee.h"
#include "mymoneyenums.h"

using namespace reports;
using namespace test;

QTEST_GUILESS_MAIN(PivotGridTest)

void PivotGridTest::initTestCase()
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kmymoney"));
}

void PivotGridTest::init()
{
    file = MyMoneyFile::instance();

    MyMoneyFileTransaction ft;
    file->addCurrency(MyMoneySecurity("CAD", "Canadian Dollar",        "C$"));
    file->addCurrency(MyMoneySecurity("USD", "US Dollar",              "$"));
    file->addCurrency(MyMoneySecurity("JPY", "Japanese Yen",           QChar(0x00A5), 1));
    file->addCurrency(MyMoneySecurity("GBP", "British Pound",           "#"));
    file->setBaseCurrency(file->currency("USD"));

    MyMoneyPayee payeeTest;
    payeeTest.setName("Test Payee");
    file->addPayee(payeeTest);
    MyMoneyPayee payeeTest2;
    payeeTest2.setName("Thomas Baumgart");
    file->addPayee(payeeTest2);

    acAsset = (MyMoneyFile::instance()->asset().id());
    acLiability = (MyMoneyFile::instance()->liability().id());
    acExpense = (MyMoneyFile::instance()->expense().id());
    acIncome = (MyMoneyFile::instance()->income().id());
    acChecking = makeAccount(QString("Checking Account"), eMyMoney::Account::Type::Checkings, moCheckingOpen, QDate(2004, 5, 15), acAsset);
    acCredit = makeAccount(QString("Credit Card"), eMyMoney::Account::Type::CreditCard, moCreditOpen, QDate(2004, 7, 15), acLiability);
    acSolo = makeAccount(QString("Solo"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 1, 11), acExpense);
    acParent = makeAccount(QString("Parent"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 1, 11), acExpense);
    acChild = makeAccount(QString("Child"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 2, 11), acParent);
    acForeign = makeAccount(QString("Foreign"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 1, 11), acExpense);

    acSecondChild = makeAccount(QString("Second Child"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 2, 11), acParent);
    acGrandChild1 = makeAccount(QString("Grand Child 1"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 2, 11), acChild);
    acGrandChild2 = makeAccount(QString("Grand Child 2"), eMyMoney::Account::Type::Expense, MyMoneyMoney(), QDate(2004, 2, 11), acChild);

    MyMoneyInstitution i("Bank of the World", "", "", "", "", "", "");
    file->addInstitution(i);
    inBank = i.id();
    ft.commit();
}

void PivotGridTest::cleanup()
{
}

void PivotGridTest::testCellAddValue()
{
    PivotCell a;
    QVERIFY(a == MyMoneyMoney());
    QVERIFY(a.m_stockSplit == MyMoneyMoney::ONE);
    QVERIFY(a.m_postSplit == MyMoneyMoney());
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney().formatMoney("", 2));

    PivotCell b(MyMoneyMoney(13, 10));
    QVERIFY(b == MyMoneyMoney(13, 10));
    QVERIFY(b.m_stockSplit == MyMoneyMoney::ONE);
    QVERIFY(b.m_postSplit == MyMoneyMoney());
    QVERIFY(b.formatMoney("", 2) == MyMoneyMoney(13, 10).formatMoney("", 2));

    PivotCell s(b);
    QVERIFY(s == MyMoneyMoney(13, 10));
    QVERIFY(s.m_stockSplit == MyMoneyMoney::ONE);
    QVERIFY(s.m_postSplit == MyMoneyMoney());
    QVERIFY(s.formatMoney("", 2) == MyMoneyMoney(13, 10).formatMoney("", 2));

    s = PivotCell::stockSplit(MyMoneyMoney(1, 2));
    QVERIFY(s == MyMoneyMoney());
    QVERIFY(s.m_stockSplit == MyMoneyMoney(1, 2));
    QVERIFY(s.m_postSplit == MyMoneyMoney());
    QVERIFY(s.formatMoney("", 2) == MyMoneyMoney().formatMoney("", 2));

    a += MyMoneyMoney::ONE;
    a += MyMoneyMoney(2, 1);
    QVERIFY(a == MyMoneyMoney(3, 1));
    QVERIFY(a.m_stockSplit == MyMoneyMoney::ONE);
    QVERIFY(a.m_postSplit == MyMoneyMoney());
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(3, 1).formatMoney("", 2));

    a += s;
    QVERIFY(a == MyMoneyMoney(3, 1));
    QVERIFY(a.m_stockSplit == MyMoneyMoney(1, 2));
    QVERIFY(a.m_postSplit == MyMoneyMoney());
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(15, 10).formatMoney("", 2));

    a += MyMoneyMoney(3, 1);
    a += MyMoneyMoney(3, 1);
    QVERIFY(a == MyMoneyMoney(3, 1));
    QVERIFY(a.m_stockSplit == MyMoneyMoney(1, 2));
    QVERIFY(a.m_postSplit == MyMoneyMoney(6, 1));
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(75, 10).formatMoney("", 2));
}

void PivotGridTest::testCellAddCell()
{
    PivotCell a, b;

    a += MyMoneyMoney(3, 1);
    a += PivotCell::stockSplit(MyMoneyMoney(2, 1));
    a += MyMoneyMoney(4, 1);

    QVERIFY(a == MyMoneyMoney(3, 1));
    QVERIFY(a.m_stockSplit == MyMoneyMoney(2, 1));
    QVERIFY(a.m_postSplit == MyMoneyMoney(4, 1));
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(10, 1).formatMoney("", 2));

    b += MyMoneyMoney(4, 1);
    b += PivotCell::stockSplit(MyMoneyMoney(4, 1));
    b += MyMoneyMoney(16, 1);

    QVERIFY(b == MyMoneyMoney(4, 1));
    QVERIFY(b.m_stockSplit == MyMoneyMoney(4, 1));
    QVERIFY(b.m_postSplit == MyMoneyMoney(16, 1));
    QVERIFY(b.formatMoney("", 2) == MyMoneyMoney(32, 1).formatMoney("", 2));

    a += b;

    QVERIFY(a == MyMoneyMoney(3, 1));
    QVERIFY(a.m_stockSplit == MyMoneyMoney(8, 1));
    QVERIFY(a.m_postSplit == MyMoneyMoney(48, 1));
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(72, 1).formatMoney("", 2));
}

void PivotGridTest::testCellRunningSum()
{
    PivotCell a;
    MyMoneyMoney runningSum(12, 10);

    a += MyMoneyMoney(3, 1);
    a += PivotCell::stockSplit(MyMoneyMoney(125, 100));
    a += MyMoneyMoney(134, 10);

    QVERIFY(a.m_stockSplit != MyMoneyMoney::ONE);
    QVERIFY(a.m_postSplit != MyMoneyMoney());

    runningSum = a.calculateRunningSum(runningSum);

    QVERIFY(runningSum == MyMoneyMoney(1865, 100));
    QVERIFY(a.formatMoney("", 2) == MyMoneyMoney(1865, 100).formatMoney("", 2));
    QVERIFY(a.m_stockSplit == MyMoneyMoney::ONE);
    QVERIFY(a.m_postSplit == MyMoneyMoney());
}

void PivotGridTest::testToXml()
{
#ifdef Q_CC_MSVC
    QSKIP("MSVC compiler fails to compile such long strings");
#else
    PivotCell cell;
    PivotGridRow gridRow;
    gridRow.append(cell);
    PivotGridRowSet gridRowSet;
    gridRowSet[eActual] = gridRow;
    ReportAccount account(acAsset);
    PivotInnerGroup innerGroup;
    innerGroup[account] = gridRowSet;
    PivotOuterGroup outerGroup;
    outerGroup["test"] = innerGroup;
    PivotGrid grid;
    grid["test"] = outerGroup;

    AlkDomDocument doc;
    AlkDomElement el = doc.createElement("root");
    QVERIFY(grid.saveToXml(doc, el));
    doc.appendChild(el);
    QCOMPARE(
        doc.toString(false),
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<root>\n<PivotGrid>\n<PivotGridMap>\n<PivotGridMapEntry>\n<PivotGridMapKey>\n<String id=\"test\" "
        "/>\n</PivotGridMapKey>\n<PivotGridMapValue>\n<PivotOuterGroup displayName=\"\" inverted=\"0\" "
        "sortOrder=\"100\">\n<PivotGridRowSet>\n<PivotGridRowSetMap>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType id=\"eActual\" "
        "/>\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eBudget\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eBudgetDiff\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eForecast\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eAverage\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"ePrice\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n</PivotGridRowSetMap>\n</"
        "PivotGridRowSet>\n<PivotOuterGroupMap>\n<PivotOuterGroupMapEntry>\n<PivotOuterGroupMapKey>\n<String id=\"test\" "
        "/>\n</"
        "PivotOuterGroupMapKey>\n<PivotOuterGroupMapValue>\n<PivotInnerGroup>\n<PivotGridRowSet>\n<PivotGridRowSetMap>\n<PivotGridRowSetMapEntry>\n<"
        "PivotGridRowSetMapKey>\n<ERowType id=\"eActual\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow "
        "total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eBudget\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eBudgetDiff\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eForecast\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eAverage\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"ePrice\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n</PivotGridRowSetMap>\n</"
        "PivotGridRowSet>\n<PivotInnerGroupMap>\n<PivotInnerGroupMapEntry>\n<PivotInnerGroupMapKey>\n<ReportAccount id=\"AStd::Asset\" "
        "/>\n</"
        "PivotInnerGroupMapKey>\n<PivotInnerGroupMapValue>\n<PivotGridRowSet>\n<PivotGridRowSetMap>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<"
        "ERowType id=\"eActual\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList>\n<PivotCell "
        "isUsed=\"0\" postSplit=\"0/1\" stockSplit=\"1/1\" value=\"0/1\" "
        "/>\n</PivotGridRowList>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</"
        "PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType id=\"eBudget\" "
        "/>\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eBudgetDiff\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eForecast\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"eAverage\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n<PivotGridRowSetMapEntry>\n<PivotGridRowSetMapKey>\n<ERowType "
        "id=\"ePrice\" />\n</PivotGridRowSetMapKey>\n<PivotGridRowSetMapValue>\n<PivotGridRow total=\"0/1\">\n<PivotGridRowList "
        "/>\n</PivotGridRow>\n</PivotGridRowSetMapValue>\n</PivotGridRowSetMapEntry>\n</PivotGridRowSetMap>\n</PivotGridRowSet>\n</PivotInnerGroupMapValue>\n</"
        "PivotInnerGroupMapEntry>\n</PivotInnerGroupMap>\n</PivotInnerGroup>\n</PivotOuterGroupMapValue>\n</PivotOuterGroupMapEntry>\n</PivotOuterGroupMap>\n</"
        "PivotOuterGroup>\n</PivotGridMapValue>\n</PivotGridMapEntry>\n</PivotGridMap>\n</PivotGrid>\n</root>\n");
#endif
}
