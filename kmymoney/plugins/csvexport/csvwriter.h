/***************************************************************************
                        csvwriter.h  -  description
                            ------------------
    begin                : Wed Mar 20 2013
    copyright            : (C) 2013-03-20 by Allan Anderson
    email                : Allan Anderson agander93@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CSVWRITER_H
#define CSVWRITER_H

#include "csvexporterplugin.h"

// ----------------------------------------------------------------------------
// QT Headers

#include <QObject>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>

// ----------------------------------------------------------------------------
// KDE Headers

// ----------------------------------------------------------------------------
// Project Headers

class MyMoneyTransaction;
class MyMoneySplit;
class CsvExporterPlugin;

/**
  * @author Thomas Baumgart
  * @author Allan Anderson
  */

/**
  * This class represents the CSV writer. All conversion between the
  * internal representation of accounts, transactions is handled in this
  * object.
  */
class CsvWriter : public QObject
{
  Q_OBJECT

public:
  CsvWriter();
  ~CsvWriter();

  CsvExporterPlugin* m_plugin;

  /**
    * This method is used to start the conversion. The parameters control
    * the destination of the data and the parts that will be exported.
    * Individual errors will be reported using message boxes.
    *
    * @param filename The name of the output file with full path information
    * @param accountId The id of the account that will be exported
    * @param accountData If true, the transactions will be exported
    * @param categoryData If true, the categories will be exported as well
    * @param startDate Transactions before this date will not be exported
    * @param endDate Transactions after this date will not be exported
    * @param fieldSeparator Current field separator
    */
  void write(const QString& filename,
             const QString& accountId, const bool accountData,
             const bool categoryData, const MyMoneyDate& startDate, const MyMoneyDate& endDate,
             const QString& separator);

private:
  bool m_firstSplit;

  QMap<QString, QString> m_map;
  /**
    * This method writes the entries necessary for an account. First
    * the leadin, and then the transactions that are in the account
    * specified by @p accountId in the range from @p startDate to @p
    * endDate.
    *
    * @param s reference to textstream
    * @param accountId id of the account to be written
    * @param startDate date from which entries are written
    * @param endDate date until which entries are written
    */
  void writeAccountEntry(QTextStream &s, const QString &accountId, const MyMoneyDate &startDate, const MyMoneyDate &endDate);

  /**
    * This method writes the category entries to the stream
    * @p s. It writes the leadin and uses writeCategoryEntries()
    * to write the entries and emits signalProgess() where needed.
    *
    * @param s reference to textstream
    */
  void writeCategoryEntries(QTextStream &s);

  /**
    * This method writes the category entry for account with
    * the ID @p accountId to the stream @p s. All subaccounts
    * are processed as well.
    *
    * @param s reference to textstream
    * @param accountId id of the account to be written
    * @param leadIn constant text that will be prepended to the account's name
    */
  void writeCategoryEntry(QTextStream &s, const QString &accountId, const QString &leadIn);
  void writeTransactionEntry(const MyMoneyTransaction& t, const QString& accountId, const int count);
  void writeSplitEntry(QString& str, const MyMoneySplit& split, const int splitCount, const int lastEntry);
  void extractInvestmentEntries(const QString& accountId, const MyMoneyDate& startDate, const MyMoneyDate& endDate);
  void writeInvestmentEntry(const MyMoneyTransaction& t, const int count);

signals:
  /**
    * This signal is emitted while the operation progresses.
    * When the operation starts, the signal is emitted with
    * @p current being 0 and @p max having the maximum value.
    *
    * During the operation, the signal is emitted with @p current
    * containing the current value on the way to the maximum value.
    * @p max will be 0 in this case.
    *
    * When the operation is finished, the signal is emitted with
    * @p current and @p max set to -1 to identify the end of the
    * operation.
    *
    * @param current see above
    * @param max see above
    */
  void signalProgress(int current, int max);

private:
  QStringList m_headerLine;

  QString m_separator;

  int m_highestSplitCount;

  bool m_noError;

  QString format(const QString &s, bool withSeparator = true);
  QString format(const MyMoneyMoney &value, int prec = 2, bool withSeparator = true);
};

#endif
