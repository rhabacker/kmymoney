/*
 * Copyright 2014  Cristian Oneț <onet.cristian@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MYMONEYCONTACT_H
#define MYMONEYCONTACT_H

#include <QObject>
#include <QString>
#include <QMetaType>

#include <kmm_mymoney_export.h>

/**
  * POD containig contact data, these fields are retrived based on an email address.
  */
struct ContactData {
  QString email;
  QString phoneNumber;
  QString street;
  QString locality;
  QString country;
  QString region;
  QString postalCode;
};

Q_DECLARE_METATYPE(ContactData);

class KJob;
/**
  * This class can be used to retrieve contact fields from the address book based on an email
  * address. It's hides the KDE PIM libraries dependency so it can be made optional.
  */
class KMM_MYMONEY_EXPORT MyMoneyContact : public QObject
{
  Q_OBJECT

public:
  MyMoneyContact(QObject *parent);
  /**
    * Properties of the default identity (the current user).
    */
  bool ownerExists() const;
  QString ownerEmail() const;
  QString ownerFullName() const;

public slots:
  /**
    * Use this slot to start retrieving contact data for an email.
    */
  void fetchContact(const QString &email);

signals:
  /**
    * This signal is emitted when the contact data was retrieved.
    */
  void contactFetched(const ContactData &identity);

private slots:
  void searchContactResult(KJob *job);
};

#endif // MYMONEYCONTACT_H
