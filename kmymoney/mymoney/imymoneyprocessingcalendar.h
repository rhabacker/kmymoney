/***************************************************************************
                          imymoneyprocessingcalendar.h  -  description
                             -------------------
    begin                : Sun May 5 2002
    copyright            : (C) 2009 by Ian Neal <ianrsn70@users.sourceforge.net>
                           (C) 2009 by Thomas Baumgart <ipwizard@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef IMYMONEYPROCESSINGCALENDAR_H
#define IMYMONEYPROCESSINGCALENDAR_H

// ----------------------------------------------------------------------------
// QT Includes

class MyMoneyDate;

// ----------------------------------------------------------------------------
// KDE Headers

// ----------------------------------------------------------------------------
// Project Includes

/**
  * @author Ian Neal
  *
  * The IMyMoneyProcessingCalendar class describes the interface to the
  * various parts that make up the processing days calendar.
  */
class IMyMoneyProcessingCalendar
{
public:
  IMyMoneyProcessingCalendar() {}
  virtual ~IMyMoneyProcessingCalendar() {}

  /**
    * returns if a given day is used by an institution to process
    * transactions or not
    */
  virtual bool isProcessingDate(const MyMoneyDate& date) const = 0;
};

#endif
