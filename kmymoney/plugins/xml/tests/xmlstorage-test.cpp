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
}
