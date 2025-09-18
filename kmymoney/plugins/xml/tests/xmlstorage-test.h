/*
    SPDX-FileCopyrightText: 2022 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

// ----------------------------------------------------------------------------
// QT Includes

#include <QObject>
#include <QUrl>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoneytestutils.h"

class XMLStorageTest : public QObject, public MyMoneyHideDebugTestBase
{
    Q_OBJECT

private:
    QUrl _testFile;

private Q_SLOTS:
    void init();
    void cleanup();
    void testOpen();
    void testFileLocking();
    void testStaleLockFile();
    void testLockFilePermissionError();
};
