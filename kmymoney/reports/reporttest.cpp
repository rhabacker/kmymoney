/***************************************************************************
                          reporttest.cpp
                          -------------------
    copyright            : (C) 2018 by Ralf Habacker
    email                : ralf.habacker@freenet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "reporttest.h"

#include <QFile>

#include <qtest_kde.h>

#include "reportstestcommon.h"
#include "querytable.h"
#include "mymoneystoragexml.h"

using namespace reports;
using namespace test;

#define QCOMPARE_NO_RETURN(actual, expected) \
  QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

TableTestBase::TableTestBase()
 : storage(0),
   file(0)
{
}

bool TableTestBase::saveReference(QueryTable &table, const QString &prefix)
{
  return table.saveToXml(prefix + fileName(table.report().name(), "reference"));
}

bool TableTestBase::compareReference(QueryTable &table, const QString &prefix)
{
  bool result = true;
  QueryTable ref(table.report());
  ref.loadFromXml(prefix + fileName(table.report().name(), "reference"));
  const QList<ListTable::TableRow> &rowsRef = ref.rows();
  const QList<ListTable::TableRow>  &rowsActual = table.rows();
  QStringList keys = rowsRef.at(0).keys();
  qDebug() << "testing" << table.report().name();
  if (!QCOMPARE_NO_RETURN(rowsRef.size(), rowsActual.size()))
    result = false;
  int size = rowsRef.size();
  foreach(const QString &key, keys) {
    for(int i =0; i < size; i++) {
      QString prefix = QString("row-[%1]-column-[%2]-").arg(i+1).arg(key);
      const ListTable::TableRow &a = rowsRef.at(i);
      const ListTable::TableRow &b = rowsActual.at(i);
      QString aStr = QString("%1%2").arg(prefix, a[key]);
      QString bStr = QString("%1%2").arg(prefix, b[key]);
      if (QCOMPARE_NO_RETURN(aStr, bStr))
        result = false;
    }
  }
  if (!result)
    table.saveToXml(prefix + fileName(table.report().name(), "current"));

  return result;
}

QString TableTestBase::fileName(const QString &baseName, const QString &suffix)
{
  QString name = QString("%1-%2.xml").arg(baseName, suffix);
  name.replace(" ", "-");
  return name;
}

bool TableTestBase::loadKMyMoneyFile(const QString &filename)
{
  QFile g(filename);
  g.open(QIODevice::ReadOnly);
  MyMoneyStorageXML xml;
  IMyMoneyStorageFormat& interface = xml;
  interface.readFile(&g, dynamic_cast<IMyMoneySerialize*>(MyMoneyFile::instance()->storage()));
  g.close();
  return true;
}

void TableTestBase::testSpecificFile(const QString &dir, const QString &_fileName)
{
  try {
  loadKMyMoneyFile(dir + "/" + _fileName);
  foreach(const MyMoneyReport &report, MyMoneyFile::instance()->reportList()) {
    if (report.reportType() == MyMoneyReport::Report::QueryTable) {
      QueryTable test(report);
      QFileInfo fn(_fileName);
      QString prefix = dir + "/" + fn.completeBaseName() + "-";
      QString file = prefix + fileName(report.name(), "reference");
      QFileInfo fi(file);
      //qDebug() << "testing" << fi.absoluteFilePath();
      if (!fi.exists()) {
        saveReference(test, prefix);
        //qDebug() << "saving reference";
      } else {
        //qDebug() << "comparing reference";
        compareReference(test, prefix);
      }
      //::writeTabletoHTML(test, "test.html");
    } else {
      qDebug() << "Report" << _fileName << "contains unsupported report type" << MyMoneyReport::Report::toString(report.reportType());
    }
  }
  } catch (const MyMoneyException &e) {
    QFAIL(qPrintable(e.what()));
  }
}

QTEST_KDEMAIN_CORE_WITH_COMPONENTNAME(ReportTest, "kmymoney")

void ReportTest::init()
{
  // force en_us locale
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  KGlobal::setLocale(new KLocale("", "en", "us"));
  storage = new MyMoneySeqAccessMgr;
  storage->setFileFixVersion(storage->currentFixVersion());
  file = MyMoneyFile::instance();
  file->attachStorage(storage);

  MyMoneyFileTransaction ft;
  file->setValue("kmm-id", file->storageId());
  file->addCurrency(MyMoneySecurity("USD", "US Dollar",              "$"));
  file->setBaseCurrency(file->currency("USD"));
  ft.commit();
}

void ReportTest::cleanup()
{
  file->detachStorage(storage);
  delete storage;
}

void ReportTest::testSpecificFiles()
{
  QStringList files = QStringList() << "395327.xml";
  QString dir = KMYMONEY_TESTFILES_DIR;
  foreach(const QString &file, files) {
    testSpecificFile(dir, file);
  }
}

