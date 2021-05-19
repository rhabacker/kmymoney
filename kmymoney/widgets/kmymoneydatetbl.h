/***************************************************************************
                          kmymoneydatetbl.h  -  description
                             -------------------
    begin                : Thu Jul 3 2003
    copyright            : (C) 2000-2003 by Michael Edwardes
    email                : mte@users.sourceforge.net
                           Javier Campos Morales <javi_c@users.sourceforge.net>
                           Felix Rodriguez <frodriguez@users.sourceforge.net>
                           John C <thetacoturtle@users.sourceforge.net>
                           Thomas Baumgart <ipwizard@users.sourceforge.net>
                           Kevin Tambascio <ktambascio@users.sourceforge.net>
 ***************************************************************************/

/****************************************************************************
 Contains code from the KDateTable class ala kdelibs-3.1.2.  Original license:

    This file is part of the KDE libraries
    Copyright (C) 1997 Tim D. Gilman (tdgilman@best.org)
              (C) 1998-2001 Mirko Boehm (mirko@kde.org)
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KMYMONEYDATETBL_H
#define KMYMONEYDATETBL_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QTableWidget>
#include <QDateTime>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QStyledItemDelegate>

// ----------------------------------------------------------------------------
// KDE Includes

#include "mymoneyutils.h"

// ----------------------------------------------------------------------------
// Project Includes

class kMyMoneyDateTbl;

/**
  * @author Cristian Onet
  */
class KMyMoneyDateTbDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  explicit KMyMoneyDateTbDelegate(kMyMoneyDateTbl *parent = 0);
  ~KMyMoneyDateTbDelegate();

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
  kMyMoneyDateTbl *m_parent;
};

/**
  * @author Michael Edwardes
  */
class kMyMoneyDateTbl : public QTableWidget
{
  Q_OBJECT
public:
  enum calendarType { WEEKLY,
                      MONTHLY
                    };

public:
  /**
   * The constructor.
   */
  explicit kMyMoneyDateTbl(QWidget *parent = 0,
                           MyMoneyDate date = MyMoneyDate::currentDate());
  /**
   * Set the font size of the date table.
   */
  virtual void setFontSize(int size);
  /**
   * Select and display this date.
   */
  virtual bool setDate(const MyMoneyDate&);
  virtual const MyMoneyDate& getDate() const;

  virtual void setType(calendarType type);
  virtual calendarType type() const {
    return m_type;
  }

signals:
  /**
   * The selected date changed.
   */
  void dateChanged(MyMoneyDate);
  /**
   * A date has been selected by clicking on the table.
   */
  void tableClicked();

  /**
    * A date is being howerd over with the mouse.
    **/
  void hoverDate(MyMoneyDate);

protected:
  /**
   * Handle the resize events.
   */
  virtual void resizeEvent(QResizeEvent *);
  /**
   * React on mouse clicks that select a date.
   */
  virtual void mouseReleaseEvent(QMouseEvent *);
  virtual void wheelEvent(QWheelEvent * e);
  virtual void keyPressEvent(QKeyEvent *e);
  virtual void focusInEvent(QFocusEvent *e);
  virtual void focusOutEvent(QFocusEvent *e);

  virtual void drawCellContents(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, const MyMoneyDate& theDate) = 0;

  virtual void mouseMoveEvent(QMouseEvent* e);

  /**
   * The font size of the displayed text.
   */
  int fontsize;
  /**
   * The currently selected date.
   */
  MyMoneyDate date;
  /**
   * The day of the first day in the month [1..7].
   */
  int firstday;
  /**
   * The number of days in the current month.
   */
  int numdays;
  /**
   * The number of days in the previous month.
   */
  int numDaysPrevMonth;

  /**
    * Type related variables
    **/
  calendarType m_type;
  int m_colCount;
  int m_rowCount;

  ///
  MyMoneyDate m_drawDateOrig;

  KMyMoneyDateTbDelegate *m_itemDelegate;
  friend class KMyMoneyDateTbDelegate;
};

#endif
