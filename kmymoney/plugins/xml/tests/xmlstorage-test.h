#ifndef XMLSTORAGE - TEST_H
#define XMLSTORAGE -TEST_H

#include "mymoneytestutils.h"

class XmlStorageTest : public QObject, public MyMoneyHideDebugTestBase
{
public:
    XmlStorageTest();

    /*private*/ public Q_SLOTS:
    void testOpenFile();
};

#endif // XMLSTORAGE-TEST_H
