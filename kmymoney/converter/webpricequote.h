/***************************************************************************
                          webpricequote.h
                             -------------------
    begin                : Thu Dec 30 2004
    copyright            : (C) 2004 by Ace Jones
    email                : Ace Jones <acejones@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WEBPRICEQUOTE_H
#define WEBPRICEQUOTE_H

// ----------------------------------------------------------------------------
// QT Headers

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QMap>

// ----------------------------------------------------------------------------
// KDE Headers

#include <kprocess.h>
#include <kurl.h>

// ----------------------------------------------------------------------------
// Project Headers

#include "mymoneymoney.h"

/**
Helper class to attend the process which is running the script, in the case
of a local script being used to fetch the quote.

@author Thomas Baumgart <thb@net-bembel.de> & Ace Jones <acejones@users.sourceforge.net>
*/
class WebPriceQuoteProcess: public KProcess
{
  Q_OBJECT
public:
  WebPriceQuoteProcess();
  inline void setSymbol(const QString& _symbol) {
    m_symbol = _symbol; m_string.truncate(0);
  }

public slots:
  void slotReceivedDataFromFilter();
  void slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus);

signals:
  void processExited(const QString&);

private:
  QString m_symbol;
  QString m_string;
};

/**
Helper class to run the Finance::Quote process. This is used only for the purpose of obtaining
a list of valid sources. The actual price quotes are obtained thru WebPriceQuoteProcess.
The class also contains functions to convert between the rather cryptic source names used
by the Finance::Quote package, and more user-friendly names.

@author Thomas Baumgart <thb@net-bembel.de> & Ace Jones <acejones@users.sourceforge.net>, Tony B<tonybloom@users.sourceforge.net>
 */
class FinanceQuoteProcess: public KProcess
{
  Q_OBJECT
public:
  FinanceQuoteProcess();
  void launch(const QString& scriptPath);
  bool isFinished() const {
    return(m_isDone);
  };
  const QStringList getSourceList() const;
  const QString crypticName(const QString& niceName) const;
  const QString niceName(const QString& crypticName) const;

public slots:
  void slotReceivedDataFromFilter();
  void slotProcessExited();

private:
  bool m_isDone;
  QString m_string;
  typedef QMap<QString, QString> fqNameMap;
  fqNameMap m_fqNames;
};

/**
  * @author Thomas Baumgart & Ace Jones
  *
  * This is a helper class to store information about an online source
  * for stock prices or currency exchange rates.
  */
struct WebPriceQuoteSource {
  WebPriceQuoteSource() : m_skipStripping(false) {}
  explicit WebPriceQuoteSource(const QString& name);
  WebPriceQuoteSource(const QString& name, const QString& url, const QString& sym, const QString& price, const QString& date, const QString& dateformat, bool skipStripping = false);
  ~WebPriceQuoteSource() {}

  void write() const;
  void rename(const QString& name);
  void remove() const;

  QString    m_name;
  QString    m_url;
  QString    m_sym;
  QString    m_price;
  QString    m_date;
  QString    m_dateformat;
  bool       m_skipStripping;
};

/**
Retrieves a price quote from a web-based quote source

@author Ace Jones <acejones@users.sourceforge.net>
*/
class WebPriceQuote: public QObject
{
  Q_OBJECT
public:
  explicit WebPriceQuote(QObject* = 0);
  ~WebPriceQuote();

  /**
   * Hold errors reported from price quote fetching and parsing
   *
   * The implementation provides a type safe way to use
   * bit operations like '|=' for combining values and '&'
   * for checking value presence.
   */
  class Errors {
  public:
    enum Type {
      None,
      Data,
      Date,
      DateFormat,
      Price,
      Script,
      Source,
      Symbol,
      Success,
      URL,
    };

    inline Errors() { }
    inline Errors(Type type) { m_type.append(type); }
    inline Errors(const Errors &e) { m_type = e.m_type; }
    inline Errors &operator |=(Type t) { if (!m_type.contains(t)) m_type.append(t); return *this; }
    inline bool operator &(Type t) const { return m_type.contains(t); }

  protected:
    QList<Type> m_type;
  };

  typedef enum _quoteSystemE {
    Native = 0,
    FinanceQuote
  } quoteSystemE;

  /**
    * This launches a web-based quote update for the given @p _symbol.
    * When the quote is received back from the web source, it will be
    * emitted on the 'quote' signal.
    *
    * @param _symbol the trading symbol of the stock to fetch a price for
    * @param _id an arbitrary identifier, which will be emitted in the quote
    *                signal when a price is sent back.
    * @param _source the source of the quote (must be a valid value returned
    *                by quoteSources().  Send QString() to use the default
    *                source.
    * @return bool Whether the quote fetch process was launched successfully
    *              In case of failures it returns false and @ref errors()
    *              could be used to get error details.
    */
  bool launch(const QString& _symbol, const QString& _id, const QString& _source = QString());

  /**
    * If @ref launch(), @ref launchNative() or @ref launchFinanceQuote() returns false,
    * this method can be used to get details about the errors that occurred.
    *
    * @return bit map of errors, see class @ref Errors for details
   */
  const Errors &errors() { return m_errors; }

  /**
    * This returns a list of the names of the quote sources
    * currently defined.
    *
   * @param _system whether to return Native or Finance::Quote source list
   * @return QStringList of quote source names
    */
  static const QStringList quoteSources(const _quoteSystemE _system = Native);

signals:
  void quote(const QString&, const QString&, const QDate&, const double&);
  void failed(const QString&, const QString&);
  void status(const QString&);
  void error(const QString&);

protected slots:
  bool slotParseQuote(const QString& _quotedata);

protected:
  static const QMap<QString, WebPriceQuoteSource> defaultQuoteSources();

private:
  Errors m_errors;
  bool launchNative(const QString& _symbol, const QString& _id, const QString& _source);
  bool launchFinanceQuote(const QString& _symbol, const QString& _id, const QString& _source);
  void enter_loop();

  static const QStringList quoteSourcesNative();
  static const QStringList quoteSourcesFinanceQuote();

private:
  /// \internal d-pointer class.
  class Private;
  /// \internal d-pointer instance.
  Private* const d;

  static QString m_financeQuoteScriptPath;
  static QStringList m_financeQuoteSources;

};

class MyMoneyDateFormat
{
public:
  explicit MyMoneyDateFormat(const QString& _format): m_format(_format) {}
  const QString convertDate(const QDate& _in) const;
  const QDate convertString(const QString& _in, bool _strict = true, unsigned _centurymidpoint = QDate::currentDate().year()) const;
  const QString& format() const {
    return m_format;
  }
private:
  QString m_format;
};

namespace convertertest
{

/**
Simple class to handle signals/slots for unit tests

@author Ace Jones <acejones@users.sourceforge.net>
*/
class QuoteReceiver : public QObject
{
  Q_OBJECT
public:
  explicit QuoteReceiver(WebPriceQuote* q, QObject *parent = 0);
  ~QuoteReceiver();
public slots:
  void slotGetQuote(const QString&, const QString&, const QDate&, const double&);
  void slotStatus(const QString&);
  void slotError(const QString&);
public:
  QStringList m_statuses;
  QStringList m_errors;
  MyMoneyMoney m_price;
  QDate m_date;
};

} // end namespace convertertest


#endif // WEBPRICEQUOTE_H
