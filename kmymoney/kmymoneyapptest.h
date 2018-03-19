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

#ifndef KMYMONEYAPPTEST_H
#define KMYMONEYAPPTEST_H

#include <QtCore/QObject>
#include <QtCore/QList>

class KMyMoneyApp;

extern KMyMoneyApp *kmymoney;
extern bool timersOn;

class KMyMoneyAppTest : public QObject
{
  Q_OBJECT
protected:
    KMyMoneyApp *m_app;

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void testFindAccount();
};

#endif
