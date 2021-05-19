/***************************************************************************
                          mymoneyutils.h  -  description
                             -------------------
    begin                : Tue Jan 29 2002
    copyright            : (C) 2000-2002 by Michael Edwardes
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

#ifndef MYMONEYUTILS_H
#define MYMONEYUTILS_H

#include <QString>
#include <QDateTime>
#include <kmm_mymoney_export.h>

#ifdef __GNUC__
#  define KMM_PRINTF_FORMAT(x, y) __attribute__((format(__printf__, x, y)))
#else
#  define KMM_PRINTF_FORMAT(x, y) /*NOTHING*/
#endif

class MyMoneyMoney;
class MyMoneySecurity;
class MyMoneyAccount;

void timestamp(const char *);

//class that has utility functions to use throughout the application.
class KMM_MYMONEY_EXPORT MyMoneyUtils
{
public:
  MyMoneyUtils() {}
  ~MyMoneyUtils() {}

  //static function to add the correct file extension at the end of the file name
  static QString getFileExtension(QString strFileName);

  /**
   * This is a convenience method. It behaves exactly as
   * MyMoneyMoney::formatMoney, but takes the information
   * about currency symbol and precision out of the MyMoneySecurity
   * and MyMoneyAccount objects @a acc and @a sec. The value to be
   * formatted is passed as @a val.
   */
  static QString formatMoney(const MyMoneyMoney& val,
                             const MyMoneyAccount& acc,
                             const MyMoneySecurity& sec,
                             bool showThousandSeparator = true);

  /**
   * This is a convenience method. It behaves exactly as the above one,
   * but takes the information about currency symbol and precision out
   * of the MyMoneySecurity object @a sec.  The value to be
   * formatted is passed as @a val.
   */
  static QString formatMoney(const MyMoneyMoney& val,
                             const MyMoneySecurity& sec,
                             bool showThousandSeparator = true);

};

#define ENABLE_DATETIME_SUPPORT
#ifdef ENABLE_DATETIME_SUPPORT
#if 0
class MyMoneyDate : public QDateTime
{
public:
    MyMoneyDate(const QDateTime &date) : QDateTime(date) {}
    MyMoneyDate() : QDateTime() {}
    MyMoneyDate(int year, int month, int day) : QDateTime(MyMoneyDate(year, month, day)) {}
    int year() const { return date().year(); }
    int month() const { return date().month(); }
    int day() const { return date().day(); }
    int daysInMonth() const { return date().daysInMonth(); }
    int dayofYear() const { return date().dayOfYear(); }
    int weekNumber(int *year = 0) const { return date().weekNumber(year); }
    int dayOfWeek() const { return date().dayOfWeek(); }
    bool isValid() const { return date().isValid(); }
    static bool isValid(int year, int month, int day) { return QDate::isValid(year, month, day); }
    void setYMD(int year, int month, int day) { setDate(QDate(year, month, day)); }
    static QString shortDayName(int weekDay)  { return QDate::shortDayName(weekDay); }
    static MyMoneyDate currentDate() { return MyMoneyDate(currentDateTime()); }
};
#endif

class MyMoneyDate : public QDate
{
public:
    MyMoneyDate() {}
    MyMoneyDate(const QDate &date) : QDate(date) {}
    MyMoneyDate(const QDateTime &date) : QDate(date.date()), _dateTime(date) {}
    MyMoneyDate(int year, int month, int day) : QDate(year, month, day) {}
    static MyMoneyDate currentDate() { return MyMoneyDate(QDate::currentDate()); }
protected:
    QDateTime _dateTime;
};

#include <KLocale>

#else
typedef QDate MyMoneyDate;
#endif

class MyMoneyLocale : public KLocale
{
public:
    static QString formatDate(const MyMoneyDate &date, KLocale::DateFormat format = KLocale::LongDate);
};

class KMM_MYMONEY_EXPORT MyMoneyTracer
{
public:
  MyMoneyTracer(const char* prettyName);
#define MYMONEYTRACER(a) MyMoneyTracer a(Q_FUNC_INFO)

  MyMoneyTracer(const QString& className, const QString& methodName);
  ~MyMoneyTracer();

  /**
    * This method allows to trace a printf like formatted text
    *
    * @param format format mask
    */
  void printf(const char *format, ...) const KMM_PRINTF_FORMAT(2, 3);

  static void off();
  static void on();
  static void onOff(int onOff);

private:
  QString m_className;
  QString m_memberName;

  static int m_indentLevel;
  static int m_onoff;
};

/**
 * This function returns a date in the form specified by Qt::ISODate.
 * If the @p date is invalid an empty string will be returned.
 *
 * @param date const reference to date to be converted
 * @return QString containing the converted date
 */
KMM_MYMONEY_EXPORT QString dateToString(const MyMoneyDate& date);

/**
 * This function returns a date as MyMoneyDate object as specified by
 * the parameter @p str. @p str must be in Qt::ISODate format.
 * If @p str is empty or contains an invalid date, MyMoneyDate() is
 * returned.
 *
 * @param str date in Qt::ISODate format
 * @return MyMoneyDate object
 */
KMM_MYMONEY_EXPORT MyMoneyDate stringToDate(const QString& str);

KMM_MYMONEY_EXPORT QString QStringEmpty(const QString&);

KMM_MYMONEY_EXPORT unsigned long extractId(const QString& txt);

#endif
