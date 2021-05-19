/***************************************************************************
                          kmymoneybriefschedule.h  -  description
                             -------------------
    begin                : Sun Jul 6 2003
    copyright            : (C) 2000-2003 by Michael Edwardes
    email                : mte@users.sourceforge.net
                           Javier Campos Morales <javi_c@users.sourceforge.net>
                           Felix Rodriguez <frodriguez@users.sourceforge.net>
                           John C <thetacoturtle@users.sourceforge.net>
                           Thomas Baumgart <ipwizard@users.sourceforge.net>
                           Kevin Tambascio <ktambascio@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KMYMONEYBRIEFSCHEDULE_H
#define KMYMONEYBRIEFSCHEDULE_H


// ----------------------------------------------------------------------------
// QT Includes

#include <QWidget>
#include <QStringList>
#include <QList>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "ui_kschedulebriefwidget.h"
#include "mymoneyfile.h"

/**
  *@author Michael Edwardes
  */

class kScheduleBriefWidget : public QWidget, public Ui::kScheduleBriefWidget
{
public:
  kScheduleBriefWidget(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};

class KMyMoneyBriefSchedule : public kScheduleBriefWidget
{
  Q_OBJECT
public:
  KMyMoneyBriefSchedule(QWidget *parent = 0);
  ~KMyMoneyBriefSchedule();
  void setSchedules(QList<MyMoneySchedule> list, const MyMoneyDate& date);

signals:
  void enterClicked(const MyMoneySchedule&, const MyMoneyDate&);
  void skipClicked(const MyMoneySchedule&, const MyMoneyDate&);

protected slots:
  void slotPrevClicked();
  void slotNextClicked();
  void slotEnterClicked();
  void slotSkipClicked();

private:
  QList<MyMoneySchedule> m_scheduleList;
  int m_index;
  MyMoneyDate m_date;

  void loadSchedule();
};

#endif
