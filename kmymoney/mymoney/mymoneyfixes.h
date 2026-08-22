/*
    SPDX-FileCopyrightText: 2000-2003 Michael Edwardes <mte@users.sourceforge.net>
    SPDX-FileCopyrightText: 2001-2002 Felix Rodriguez <frodriguez@users.sourceforge.net>
    SPDX-FileCopyrightText: 2002-2004 Kevin Tambascio <ktambascio@users.sourceforge.net>
    SPDX-FileCopyrightText: 2004-2005 Ace Jones <acejones@users.sourceforge.net>
    SPDX-FileCopyrightText: 2006-2020 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-FileCopyrightText: 2006 Darren Gould <darren_gould@gmx.de>
    SPDX-FileCopyrightText: 2017-2018 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
    SPDX-FileCopyrightText: 2020 Robert Szczesiak <dev.rszczesiak@gmail.com>
    SPDX-FileCopyrightText: 2025 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MYMONEYFIXES_H
#define MYMONEYFIXES_H

#include "mymoneyexception.h"

class MyMoneyFixes
{
public:
    MyMoneyFixes()
    {
    }

    virtual ~MyMoneyFixes()
    {
    }

    virtual int fixVersion() const = 0;
    virtual int availableFixVersion() const = 0;
    virtual void* initFix() = 0;
    virtual void commitFix(void* p) = 0;
    virtual void setFixVersion(int version) = 0;
    virtual bool applyFixes(bool expertMode) = 0;
    virtual int upgradeToV11() = 0;
    virtual int upgradeToV10() = 0;
    virtual int upgradeToV9() = 0;
    virtual int upgradeToV8() = 0;
    virtual int upgradeToV7() = 0;
    virtual int upgradeToV6() = 0;
    virtual int upgradeToV5() = 0;
    virtual int upgradeToV4() = 0;
    virtual int upgradeToV3() = 0;
    virtual int upgradeToV2(bool expertMode = false) = 0;
    virtual int upgradeToV1() = 0;
};

#endif // MYMONEYFIXES_H
