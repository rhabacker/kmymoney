/*
    SPDX-FileCopyrightText: 2022 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlstorage-test.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QTest>

// ----------------------------------------------------------------------------
// KDE Includes

#include <KPluginMetaData>

// ----------------------------------------------------------------------------
// Project Includes

#include "../xmlstorage.h"

QTEST_MAIN(XMLStorageTest)

void XMLStorageTest::init()
{
    _testFile = QUrl::fromLocalFile(QLatin1String(CMAKE_CURRENT_SOURCE_DIR "/testfile1.xml"));
}

void XMLStorageTest::cleanup()
{
}

void XMLStorageTest::testOpen()
{
    KPluginMetaData metaData;
    const QVariantList args;
    XMLStorage storage(nullptr, metaData, args);

    QCOMPARE(storage.open(_testFile), true);
    storage.close();
}

void XMLStorageTest::testFileLocking()
{
    KPluginMetaData metaData;
    const QVariantList args;
    XMLStorage storage1(nullptr, metaData, args);
    XMLStorage storage2(nullptr, metaData, args);

    QCOMPARE(storage1.open(_testFile), true);
    QCOMPARE(storage2.open(_testFile), false);
    storage1.close();
    storage2.close();
}

void XMLStorageTest::testStaleLockFile()
{
    KPluginMetaData metaData;
    const QVariantList args;

    {
        XMLStorage storage1(nullptr, metaData, args);
        QCOMPARE(storage1.open(_testFile), true);
    }

    XMLStorage storage2(nullptr, metaData, args);
    QCOMPARE(storage2.open(_testFile), true);
    storage2.close();
}

void XMLStorageTest::testLockFilePermissionError()
{
    KPluginMetaData metaData;
    const QVariantList args;
    XMLStorage storage1(nullptr, metaData, args);
    XMLStorage storage2(nullptr, metaData, args);

    QFile f(_testFile.toLocalFile() + QLatin1String(".lck"));
    QCOMPARE(f.open(QIODevice::WriteOnly), true);
    f.close();
    f.setPermissions(QFileDevice::ReadOwner);
    QCOMPARE(storage1.open(_testFile), true);
    QCOMPARE(storage2.open(_testFile), false);
    storage1.close();
    storage2.close();
}
