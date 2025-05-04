#include "xmlstorage-test.h"

#include "../xmlstorage.h"

#include <KPluginMetaData>
#include <QTest>

XmlStorageTest::XmlStorageTest()
{
}

void XmlStorageTest::testOpenFile()
{
    KPluginMetaData metaData;
    QVariantList args;
    XMLStorage storage(nullptr, metaData, args);
    const QUrl url = QUrl::fromLocalFile(qApp->arguments().size() > 1 ? qApp->arguments().at(1) : "testfile.xml");
    QCOMPARE(storage.open(url), true);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    XmlStorageTest testFuncs;
    testFuncs.testOpenFile();
    return 0;
}
// QTEST_GUILESS_MAIN(XmlStorageTest)
