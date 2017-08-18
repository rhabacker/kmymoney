/*
    Copyright 2017 ralf.habacker <ralf.habacker@freenet.de>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DEBUGINDENTER_H
#define DEBUGINDENTER_H

#include <QtDebug>

#include <typeinfo>
#include <cxxabi.h>

#include "mymoneyobject.h"

class DebugIndenter : public QDebug
{
public:
  DebugIndenter(QDebug &dbg, const char *className)
   : QDebug(dbg), m_dbg(dbg)
  {
    init(className);
  }

  DebugIndenter(QDebug &dbg, MyMoneyObject *o)
   : QDebug(dbg), m_dbg(dbg)
  {
    init(typeid(*o).name());
  }

  DebugIndenter(QDebug &dbg, const MyMoneyObject &o)
   : QDebug(dbg), m_dbg(dbg)
  {
    init(typeid(o).name());
  }

  void init(const char *name)
  {
    int status;
    m_level++;
    m_dbg.nospace() << "\n" << fill().toLatin1().constData() << abi::__cxa_demangle(name, 0, 0, &status) << "(";
    m_dbg.space();
  }
  
  QString fill()
  {
    return QString().fill(QLatin1Char(' '), m_level*2);
  }

  ~DebugIndenter()
  {
    --m_level;
    m_dbg.nospace() << ")\n" << fill().toLatin1().constData();
    m_dbg.space();
  }

protected:
  static int m_level;
  QDebug &m_dbg;
};

#endif // DEBUGINDENTER_H
