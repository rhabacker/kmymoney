/***************************************************************************
                             kmymoneyselector.cpp
                             -------------------
    begin                : Thu Jun 29 2006
    copyright            : (C) 2006 by Thomas Baumgart
    email                : Thomas Baumgart <ipwizard@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config-kmymoney.h>

#include "kmymoneyselector.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QLayout>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QStyle>
#include <QRegExp>
#include <QHBoxLayout>
#include <QApplication>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "kmymoneyglobalsettings.h"

KMyMoneySelector::KMyMoneySelector(QWidget *parent, Qt::WFlags flags) :
    QWidget(parent, flags)
{
  setAutoFillBackground(true);

  m_selMode = QTreeWidget::SingleSelection;

  m_treeWidget = new QTreeWidget(this);
  // don't show horizontal scroll bar
  m_treeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_treeWidget->setSortingEnabled(false);
  m_treeWidget->setAlternatingRowColors(true);

  m_treeWidget->setAllColumnsShowFocus(true);

  m_layout = new QHBoxLayout(this);
  m_layout->setSpacing(6);
  m_layout->setMargin(0);

  m_treeWidget->header()->hide();

  m_layout->addWidget(m_treeWidget);

  // force init
  m_selMode = QTreeWidget::MultiSelection;
  setSelectionMode(QTreeWidget::MultiSelection);

  connect(m_treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)), this, SLOT(slotItemPressed(QTreeWidgetItem*,int)));
  connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SIGNAL(stateChanged()));
}

KMyMoneySelector::~KMyMoneySelector()
{
}

void KMyMoneySelector::clear()
{
  m_treeWidget->clear();
}

void KMyMoneySelector::setSelectable(QTreeWidgetItem *item, bool selectable)
{
  if (selectable) {
    item->setFlags(item->flags() | Qt::ItemIsSelectable);
  } else {
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
  }
}

void KMyMoneySelector::setSelectionMode(const QTreeWidget::SelectionMode mode)
{
  if (m_selMode != mode) {
    m_selMode = mode;
    clear();

    // make sure, it's either Multi or Single
    if (mode != QTreeWidget::MultiSelection) {
      m_selMode = QTreeWidget::SingleSelection;
      connect(m_treeWidget, SIGNAL(itemSelectionChanged()), this, SIGNAL(stateChanged()));
      connect(m_treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(slotItemSelected(QTreeWidgetItem*)));
      connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(slotItemSelected(QTreeWidgetItem*)));
    } else {
      disconnect(m_treeWidget, SIGNAL(itemSelectionChanged()), this, SIGNAL(stateChanged()));
      disconnect(m_treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(slotItemSelected(QTreeWidgetItem*)));
      disconnect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(slotItemSelected(QTreeWidgetItem*)));
    }
  }
  QWidget::update();
}

void KMyMoneySelector::slotItemSelected(QTreeWidgetItem *item)
{
  if (m_selMode == QTreeWidget::SingleSelection) {
    if (item && item->flags().testFlag(Qt::ItemIsSelectable)) {
      emit itemSelected(item->data(0, IdRole).toString());
    }
  }
}

QTreeWidgetItem* KMyMoneySelector::newItem(const QString& name, QTreeWidgetItem* after, const QString& key, const QString& id)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget, after);

  item->setText(0, name);
  item->setData(0, KeyRole, key);
  item->setData(0, IdRole, id);
  item->setText(1, key); // hidden, but used for sorting
  item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

  if (id.isEmpty()) {
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);
    setSelectable(item, false);
  }
  item->setExpanded(true);
  return item;
}

QTreeWidgetItem* KMyMoneySelector::newItem(const QString& name, const QString& key, const QString& id)
{
  return newItem(name, 0, key, id);
}

QTreeWidgetItem* KMyMoneySelector::newTopItem(const QString& name, const QString& key, const QString& id)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);

  item->setText(0, name);
  item->setData(0, KeyRole, key);
  item->setData(0, IdRole, id);
  item->setText(1, key); // hidden, but used for sorting
  item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

  if (m_selMode == QTreeWidget::MultiSelection) {
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
  }
  return item;
}

QTreeWidgetItem* KMyMoneySelector::newItem(QTreeWidgetItem* parent, const QString& name, const QString& key, const QString& id)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(parent);

  item->setText(0, name);
  item->setData(0, KeyRole, key);
  item->setData(0, IdRole, id);
  item->setText(1, key); // hidden, but used for sorting
  item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

  if (m_selMode == QTreeWidget::MultiSelection) {
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
  }
  return item;
}

void KMyMoneySelector::protectItem(const QString& itemId, const bool protect)
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* it_v;

  // scan items
  while ((it_v = *it) != 0) {
    if (it_v->data(0, IdRole).toString() == itemId) {
      setSelectable(it_v, !protect);
      break;
    }
    ++it;
  }
}

QTreeWidgetItem* KMyMoneySelector::item(const QString& id) const
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* it_v;

  while ((it_v = *it) != 0) {
    if (it_v->data(0, IdRole).toString() == id)
      break;
    ++it;
  }
  return it_v;
}

bool KMyMoneySelector::allItemsSelected() const
{
  QTreeWidgetItem* rootItem = m_treeWidget->invisibleRootItem();

  if (m_selMode == QTreeWidget::SingleSelection)
    return false;

  for (int i = 0; i < rootItem->childCount(); ++i) {
    QTreeWidgetItem* item = rootItem->child(i);
    if (item->flags().testFlag(Qt::ItemIsUserCheckable)) {
      if (!(item->checkState(0) == Qt::Checked && allItemsSelected(item)))
        return false;
    } else {
      if (!allItemsSelected(item))
        return false;
    }
  }
  return true;
}

bool KMyMoneySelector::allItemsSelected(const QTreeWidgetItem *item) const
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem* child = item->child(i);
    if (child->flags().testFlag(Qt::ItemIsUserCheckable)) {
      if (!(child->checkState(0) == Qt::Checked && allItemsSelected(child)))
        return false;
    }
  }
  return true;
}

void KMyMoneySelector::removeItem(const QString& id)
{
  QTreeWidgetItem* it_v;
  QTreeWidgetItemIterator it(m_treeWidget);

  while ((it_v = *it) != 0) {
    if (id == it_v->data(0, IdRole).toString()) {
      if (it_v->childCount() > 0) {
        setSelectable(it_v, false);
      } else {
        delete it_v;
      }
    }
    it++;
  }

  // get rid of top items that just lost the last children (e.g. Favorites)
  it = QTreeWidgetItemIterator(m_treeWidget, QTreeWidgetItemIterator::NotSelectable);
  while ((it_v = *it) != 0) {
    if (it_v->childCount() == 0)
      delete it_v;
    it++;
  }
}


void KMyMoneySelector::selectAllItems(const bool state)
{
  selectAllSubItems(m_treeWidget->invisibleRootItem(), state);
  emit stateChanged();
}

void KMyMoneySelector::selectItems(const QStringList& itemList, const bool state)
{
  selectSubItems(m_treeWidget->invisibleRootItem(), itemList, state);
  emit stateChanged();
}

void KMyMoneySelector::selectSubItems(QTreeWidgetItem* item, const QStringList& itemList, const bool state)
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem* child = item->child(i);
    if (child->flags().testFlag(Qt::ItemIsUserCheckable) && itemList.contains(child->data(0, IdRole).toString())) {
      child->setCheckState(0, state ? Qt::Checked : Qt::Unchecked);
    }
    selectSubItems(child, itemList, state);
  }
  emit stateChanged();
}

void KMyMoneySelector::selectAllSubItems(QTreeWidgetItem* item, const bool state)
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem* child = item->child(i);
    if (child->flags().testFlag(Qt::ItemIsUserCheckable)) {
      child->setCheckState(0, state ? Qt::Checked : Qt::Unchecked);
    }
    selectAllSubItems(child, state);
  }
  emit stateChanged();
}

void KMyMoneySelector::selectedItems(QStringList& list) const
{
  list.clear();
  if (m_selMode == QTreeWidget::SingleSelection) {
    QTreeWidgetItem* it_c = m_treeWidget->currentItem();
    if (it_c != 0)
      list << it_c->data(0, IdRole).toString();
  } else {
    QTreeWidgetItem* rootItem = m_treeWidget->invisibleRootItem();
    for (int i = 0; i < rootItem->childCount(); ++i) {
      QTreeWidgetItem* child = rootItem->child(i);
      if (child->flags().testFlag(Qt::ItemIsUserCheckable)) {
        if (child->checkState(0) == Qt::Checked)
          list << child->data(0, IdRole).toString();
      }
      selectedItems(list, child);
    }
  }
}

void KMyMoneySelector::selectedItems(QStringList& list, QTreeWidgetItem* item) const
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem* child = item->child(i);
    if (child->flags().testFlag(Qt::ItemIsUserCheckable)) {
      if (child->checkState(0) == Qt::Checked)
        list << child->data(0, IdRole).toString();
    }
    selectedItems(list, child);
  }
}

void KMyMoneySelector::itemList(QStringList& list) const
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* it_v;

  while ((it_v = *it) != 0) {
    list << it_v->data(0, IdRole).toString();
    it++;
  }
}

void KMyMoneySelector::setSelected(const QString& id, const bool state)
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* item;
  QTreeWidgetItem* it_visible = 0;

  while ((item = *it) != 0) {
    if (item->data(0, IdRole).toString() == id) {
      if (item->flags().testFlag(Qt::ItemIsUserCheckable)) {
        item->setCheckState(0, state ? Qt::Checked : Qt::Unchecked);
      }
      m_treeWidget->setCurrentItem(item);
      if (!it_visible)
        it_visible = item;
    }
    it++;
  }

  // make sure the first one found is visible
  if (it_visible)
    m_treeWidget->scrollToItem(it_visible);
}

int KMyMoneySelector::slotMakeCompletion(const QString& _txt)
{
  QString txt(QRegExp::escape(_txt));
  if (KMyMoneyGlobalSettings::stringMatchFromStart() && QLatin1String(this->metaObject()->className()) == QLatin1String("KMyMoneySelector"))
    txt.prepend('^');
  return slotMakeCompletion(QRegExp(txt, Qt::CaseInsensitive));
}

bool KMyMoneySelector::match(const QRegExp& exp, QTreeWidgetItem* item) const
{
  return exp.indexIn(item->text(0)) != -1;
}

int KMyMoneySelector::slotMakeCompletion(const QRegExp& exp)
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);

  QTreeWidgetItem* it_v;

  // The logic used here seems to be awkward. The problem is, that
  // QListViewItem::setVisible works recursively on all it's children
  // and grand-children.
  //
  // The way out of this is as follows: Make all items visible.
  // Then go through the list again and perform the checks.
  // If an item does not have any children (last leaf in the tree view)
  // perform the check. Then check recursively on the parent of this
  // leaf that it has no visible children. If that is the case, make the
  // parent invisible and continue this check with it's parent.
  while ((it_v = *it) != 0) {
    it_v->setHidden(false);
    ++it;
  }

  QTreeWidgetItem* firstMatch = 0;

  if (!exp.pattern().isEmpty()) {
    it = QTreeWidgetItemIterator(m_treeWidget, QTreeWidgetItemIterator::Selectable);
    while ((it_v = *it) != 0) {
      if (it_v->childCount() == 0) {
        if (!match(exp, it_v)) {
          // this is a node which does not contain the
          // text and does not have children. So we can
          // safely hide it. Then we check, if the parent
          // has more children which are still visible. If
          // none are found, the parent node is hidden also. We
          // continue until the top of the tree or until we
          // find a node that still has visible children.
          bool hide = true;
          while (hide) {
            it_v->setHidden(true);
            it_v = it_v->parent();
            if (it_v && (it_v->flags() & Qt::ItemIsSelectable)) {
              hide = !match(exp, it_v);
              for (int i = 0; hide && i < it_v->childCount(); ++i) {
                if (!it_v->child(i)->isHidden())
                  hide = false;
              }
            } else
              hide = false;
          }
        } else if (!firstMatch) {
          firstMatch = it_v;
        }
        ++it;

      } else if (match(exp, it_v)) {
        if (!firstMatch) {
          firstMatch = it_v;
        }
        // a node with children contains the text. We want
        // to display all child nodes in this case, so we need
        // to advance the iterator to the next sibling of the
        // current node. This could well be the sibling of a
        // parent or grandparent node.
        QTreeWidgetItem* curr = it_v;
        QTreeWidgetItem* item;
        while ((item = curr->treeWidget()->itemBelow(curr)) == 0) {
          curr = curr->parent();
          if (curr == 0)
            break;
          if (match(exp, curr))
            firstMatch = curr;
        }
        do {
          ++it;
        } while (*it && *it != item);
      } else {
        // It's a node with children that does not match. We don't
        // change it's status here.
        ++it;
      }
    }
  }

  // make the first match the one that is selected
  // if we have no match, make sure none is selected
  if (m_selMode == QTreeWidget::SingleSelection) {
    if (firstMatch) {
      m_treeWidget->setCurrentItem(firstMatch);
      m_treeWidget->scrollToItem(firstMatch);
    } else
      m_treeWidget->clearSelection();
  }

  // Get the number of visible nodes for the return code
  int cnt = 0;

  it = QTreeWidgetItemIterator(m_treeWidget, QTreeWidgetItemIterator::Selectable | QTreeWidgetItemIterator::NotHidden);
  while ((it_v = *it) != 0) {
    cnt++;
    it++;
  }
  return cnt;
}

bool KMyMoneySelector::contains(const QString& txt) const
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* it_v;
  while ((it_v = *it) != 0) {
    if (it_v->text(0) == txt) {
      return true;
    }
    it++;
  }
  return false;
}

void KMyMoneySelector::slotItemPressed(QTreeWidgetItem* item, int /* col */)
{
  if (QApplication::mouseButtons() != Qt::RightButton)
    return;

  if (item->flags().testFlag(Qt::ItemIsUserCheckable)) {
    QStyleOptionButton opt;
    opt.rect = m_treeWidget->visualItemRect(item);
    QRect rect = m_treeWidget->style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, m_treeWidget);
    if (rect.contains(m_treeWidget->mapFromGlobal(QCursor::pos()))) {
      // we get down here, if we have a right click onto the checkbox
      item->setCheckState(0, item->checkState(0) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
      selectAllSubItems(item, item->checkState(0) == Qt::Checked);
    }
  }
}

QStringList KMyMoneySelector::selectedItems() const
{
  QStringList list;
  selectedItems(list);
  return list;
}

QStringList KMyMoneySelector::itemList() const
{
  QStringList list;
  itemList(list);
  return list;
}
