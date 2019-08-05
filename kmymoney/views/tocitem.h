/***************************************************************************
                          tocitem.h  -  description
                             -------------------
    begin                : Sat Jul 03 2010
    copyright            : (C) Bernd Gonsior
    email                : bernd.gonsior@googlemail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef TOCITEM_H
#define TOCITEM_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QDebug>
#include <QStringList>
#include <QPointer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>

// ----------------------------------------------------------------------------
// KDE Includes


// ----------------------------------------------------------------------------
// Project Includes

/**
 * Base class for items in reports table of contents (TOC).
 * It provides the type of the item (reportgroup or report)
 * and an operator for sorting.
 */
class TocItem
{
public:

  /** Type of TOC-item */
  enum ItemType {
    /** item represents a reportgroup */
    GROUP  = QTreeWidgetItem::UserType + 10,
    /** item represents a report */
    REPORT = QTreeWidgetItem::UserType + 20
  } type;

  /** Constructor.
   *
   * @param parent pointer to the parent QWidget
   * @param columns List of texts in columns
   */
  TocItem(TocItem* parent, QStringList columns);

  /** Indicates, whether the item represents a report or a reportgroup.
   *
   * @retval true  the item represents a report
   * @retval false the item represents a reportgroup
   */
  bool isReport();

  TocItem* parent() { return m_parent; }
  TocItem* child(int row) { return m_childs.at(row); }
  QString column(int row) { return m_columns.at(row); }
  QVariant data(int column) { return m_data.at(column); }
  int row() { return m_parent ? m_parent->m_childs.indexOf(this) : 0; }
  int columnCount() const { return m_data.count(); }
  void appendChild(TocItem *child);
  int childCount() const { return m_childs.count(); }

protected:
  TocItem* m_parent;
  QList<TocItem*> m_childs;
  QList<QVariant> m_data;
  QStringList m_columns;
};

#endif
