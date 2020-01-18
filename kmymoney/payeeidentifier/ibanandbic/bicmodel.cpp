/*
 * This file is part of KMyMoney, A Personal Finance Manager for KDE
 * Copyright (C) 2014 Christian Dávid <christian-david@web.de>
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

#include "bicmodel.h"

#include <KDebug>
#include <KServiceTypeTrader>
#include <KStandardDirs>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * @warning At the moment the completion may fail if bicModel was created in more than one thread
 * (it uses a QSqlDatabase object over all instances of bicModel, so the first created bicModel defines
 * the thread)
 *
 * @todo Make thread safe.
 */
bicModel::bicModel(QObject* parent)
    : QSqlQueryModel(parent)
{

  QSqlDatabase db = QSqlDatabase::database("bicModel", true);
  // Save if the database was opened before
  bool attachDatabases = false;

  if (!db.isValid()) {
    db = QSqlDatabase::addDatabase("QSQLITE", "bicModel");
    db.setDatabaseName(":memory:");
    db.setConnectOptions("QSQLITE_OPEN_READONLY=1;QSQLITE_ENABLE_SHARED_CACHE=1;");
    db.open();
    // Database was not opened before
    attachDatabases = true;
  }

  if (!db.isOpen()) {
    kWarning() << QString("Could not open in-memory database for bic data.");
  }
  QSqlQuery query(db);

  // Get services which support iban2bic and have a database entry
  KService::List services = KServiceTypeTrader::self()->query("KToblzCheck/IbanBicData",
                            QString("exist [X-KMyMoney-Bankdata-Database]")
                                                             );

  if (services.isEmpty()) {
    // Set a valid query
    query.exec("SELECT null;");
    setQuery(query);
    return;
  }

  QStringList databases;
  QStringList dbNames;

  unsigned int databaseCount = 0;

  foreach (KService::Ptr service, services) {
    QString database = service->property(QLatin1String("X-KMyMoney-Bankdata-Database")).toString();

    // Locate database
    QString path = KGlobal::dirs()->locate("data", QLatin1String("ktoblzcheck/ibanbicdata/") + database);
    if (path.isEmpty()) {
      kWarning() << QString("Could not locate database file \"%1\" to receive BIC data.").arg(database);
    } else {
      databases << path;
      dbNames << QString("db%1").arg(++databaseCount);
    }
  }

  if (attachDatabases) {
    query.prepare("ATTACH DATABASE ? AS ?");
    query.addBindValue(databases);
    query.addBindValue(dbNames);
    if (!query.execBatch()) {
      qWarning() << "Could not init bic for bicModel, last error:" << query.lastError().text();
      dbNames = QStringList(); // clear so no query will be set
    }
  }

  QStringList queries;
  foreach (QString dbName, dbNames) {
    queries.append(QString("SELECT bic, name FROM %1.institutions").arg(dbName));
  }

  query.exec(queries.join(QLatin1String(" UNION ")));
  setQuery(query);
}

QVariant bicModel::data(const QModelIndex& item, int role) const
{
  if (role == InstitutionNameRole)
    return QSqlQueryModel::data(createIndex(item.row(), 1), Qt::DisplayRole);
  return QSqlQueryModel::data(item, role);
}
