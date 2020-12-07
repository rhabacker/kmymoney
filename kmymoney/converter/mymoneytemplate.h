/***************************************************************************
                          mymoneytemplate.h  -  description
                             -------------------
    begin                : Sat Aug 14 2004
    copyright            : (C) 2004 by Thomas Baumgart
    email                : ipwizard@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MYMONEYTEMPLATE_H
#define MYMONEYTEMPLATE_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QtXml/QDomDocument>
#include <QtXml/QDomNode>

class QFile;
class QTreeWidgetItem;

// ----------------------------------------------------------------------------
// KDE Includes

#include <KDE/KSaveFile>
#include <KDE/KUrl>

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoneyaccount.h"
#include "mymoneyfile.h"

/**
  * @author Thomas Baumgart
  */

/**
  * This class represents an account template handler. It is capable
  * to read an XML formatted account template file and import it into
  * the current engine. Also, it can save the current account structure
  * of the engine to an XML formatted template file.
  */
class MyMoneyTemplate
{
public:
  MyMoneyTemplate();
  explicit MyMoneyTemplate(const KUrl& url);
  ~MyMoneyTemplate();

  bool loadTemplate(const KUrl& url);
  bool saveTemplate(const KUrl& url);
  bool importTemplate(void(*callback)(int, int, const QString&));
  bool exportTemplate(void(*callback)(int, int, const QString&));

  const QString& title() const;
  const QString& shortDescription() const;
  const QString& longDescription() const;

  void setTitle(const QString &s);
  void setShortDescription(const QString &s);
  void setLongDescription(const QString &s);

  void hierarchy(QMap<QString, QTreeWidgetItem*>& list);

protected:
  bool loadDescription();
  bool createAccounts(MyMoneyAccount& parent, QDomNode account);
  bool setFlags(MyMoneyAccount& acc, QDomNode flags);
  bool saveToLocalFile(KSaveFile* qfile);
  bool addAccountStructure(QDomElement& parent, const MyMoneyAccount& acc);
  bool hierarchy(QMap<QString, QTreeWidgetItem*>& list, const QString& parent, QDomNode account);

  /**
    * This method is used to update the progress information. It
    * checks if an appropriate function is known and calls it.
    *
    * For a parameter description see KMyMoneyView::progressCallback().
    */
  void signalProgress(int current, int total, const QString& = "");

  QString mappedVatAccount(const QString &vatAccounts);
private:
  QDomDocument    m_doc;
  QDomNode        m_accounts;
  QString         m_title;
  QString         m_shortDesc;
  QString         m_longDesc;
  KUrl            m_source;
  void (*m_progressCallback)(int, int, const QString&);
  int             m_accountsRead;
  QMap<QString,QString> m_vatAccountMap;
};

#endif
