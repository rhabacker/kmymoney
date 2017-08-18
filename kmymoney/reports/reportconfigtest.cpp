#include "reportconfigtest.h"

#include "mymoneyfile.h"
#include "mymoneyseqaccessmgr.h"
#include "mymoneystoragexml.h"
#include "kreportsview.h"

#include <QFile>

class MyTestMoneyStorageXML : public MyMoneyStorageXML {
public:
  bool readFile(const QString &filename, IMyMoneyStorage *storage)
  {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly), true) {
      MyMoneyStorageXML::readFile(&file, dynamic_cast<IMyMoneySerialize*>(storage));
      file.close();
      return true;
    }
    return false;
  }
};

ReportConfigTest::ReportConfigTest(QObject *parent) : QObject(parent)
{
}

void ReportConfigTest::init()
{
  IMyMoneyStorage *storage = new MyMoneySeqAccessMgr;
  MyTestMoneyStorageXML reader;
  m->attachStorage(storage);
  QList<KReportsView::ReportGroup> groups;
  KReportsView::defaultReports(groups);
  reader.readFile("reporttest.xml", storage);
}
