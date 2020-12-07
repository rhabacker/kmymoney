/***************************************************************************
                          kmymoneyaccountselector.cpp  -  description
                             -------------------
    begin                : Thu Sep 18 2003
    copyright            : (C) 2003 by Thomas Baumgart
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

#include "kmymoneyaccountselector.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QLayout>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QStyle>
#include <QRect>
#include <QList>
#include <QVBoxLayout>
#include <QPixmapCache>

// ----------------------------------------------------------------------------
// KDE Includes

#include <klocale.h>
#include <kpushbutton.h>

// ----------------------------------------------------------------------------
// Project Includes

#include <mymoneyutils.h>
#include <mymoneyfile.h>
#include "kmymoneyutils.h"
#include "kmymoneyglobalsettings.h"

kMyMoneyAccountSelector::kMyMoneyAccountSelector(QWidget *parent, Qt::WFlags flags, const bool createButtons) :
    KMyMoneySelector(parent, flags),
    m_allAccountsButton(0),
    m_noAccountButton(0),
    m_incomeCategoriesButton(0),
    m_expenseCategoriesButton(0)
{

  if (createButtons) {
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(6);

    m_allAccountsButton = new KPushButton(this);
    m_allAccountsButton->setObjectName("m_allAccountsButton");
    m_allAccountsButton->setText(i18nc("Select all accounts", "All"));
    buttonLayout->addWidget(m_allAccountsButton);

    m_incomeCategoriesButton = new KPushButton(this);
    m_incomeCategoriesButton->setObjectName("m_incomeCategoriesButton");
    m_incomeCategoriesButton->setText(i18n("Income"));
    buttonLayout->addWidget(m_incomeCategoriesButton);

    m_expenseCategoriesButton = new KPushButton(this);
    m_expenseCategoriesButton->setObjectName("m_expenseCategoriesButton");
    m_expenseCategoriesButton->setText(i18n("Expense"));
    buttonLayout->addWidget(m_expenseCategoriesButton);

    m_noAccountButton = new KPushButton(this);
    m_noAccountButton->setObjectName("m_noAccountButton");
    m_noAccountButton->setText(i18nc("No account", "None"));
    buttonLayout->addWidget(m_noAccountButton);

    QSpacerItem* spacer = new QSpacerItem(0, 67, QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonLayout->addItem(spacer);
    m_layout->addLayout(buttonLayout);

    connect(m_allAccountsButton, SIGNAL(clicked()), this, SLOT(slotSelectAllAccounts()));
    connect(m_noAccountButton, SIGNAL(clicked()), this, SLOT(slotDeselectAllAccounts()));
    connect(m_incomeCategoriesButton, SIGNAL(clicked()), this, SLOT(slotSelectIncomeCategories()));
    connect(m_expenseCategoriesButton, SIGNAL(clicked()), this, SLOT(slotSelectExpenseCategories()));
  }
}

kMyMoneyAccountSelector::~kMyMoneyAccountSelector()
{
}

void kMyMoneyAccountSelector::removeButtons()
{
  delete m_allAccountsButton;
  delete m_incomeCategoriesButton;
  delete m_expenseCategoriesButton;
  delete m_noAccountButton;
}

void kMyMoneyAccountSelector::selectCategories(const bool income, const bool expense)
{
  QTreeWidgetItemIterator it_v(m_treeWidget);

  for (; *it_v != 0; ++it_v) {
    if ((*it_v)->text(0) == i18n("Income categories"))
      selectAllSubItems(*it_v, income);
    else if ((*it_v)->text(0) == i18n("Expense categories"))
      selectAllSubItems(*it_v, expense);
  }
  emit stateChanged();
}

void kMyMoneyAccountSelector::setSelectionMode(QTreeWidget::SelectionMode mode)
{
  m_incomeCategoriesButton->setHidden(mode == QTreeWidget::MultiSelection);
  m_expenseCategoriesButton->setHidden(mode == QTreeWidget::MultiSelection);
  KMyMoneySelector::setSelectionMode(mode);
}

QStringList kMyMoneyAccountSelector::accountList(const  QList<MyMoneyAccount::accountTypeE>& filterList) const
{
  QStringList    list;
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);

  while (*it) {
    QVariant id = (*it)->data(0, KMyMoneySelector::IdRole);
    MyMoneyAccount acc = MyMoneyFile::instance()->account(id.toString());
    if (filterList.count() == 0 || filterList.contains(acc.accountType()))
      list << id.toString();
    it++;
  }
  return list;
}

bool kMyMoneyAccountSelector::match(const QRegExp& exp, QTreeWidgetItem* item) const
{
  if (!item->flags().testFlag(Qt::ItemIsSelectable))
    return false;
  return exp.indexIn(item->data(0, KMyMoneySelector::KeyRole).toString().mid(1)) != -1;
}

bool kMyMoneyAccountSelector::contains(const QString& txt) const
{
  QTreeWidgetItemIterator it(m_treeWidget, QTreeWidgetItemIterator::Selectable);
  QTreeWidgetItem* it_v;

  QString baseName = i18n("Asset") + '|' +
                     i18n("Liability") + '|' +
                     i18n("Income") + '|' +
                     i18n("Expense") + '|' +
                     i18n("Equity") + '|' +
                     i18n("Security");

  while ((it_v = *it) != 0) {
    QRegExp exp(QString("^(?:%1):%2$").arg(baseName).arg(QRegExp::escape(txt)));
    if (exp.indexIn(it_v->data(0, KMyMoneySelector::KeyRole).toString().mid(1)) != -1) {
      return true;
    }
    it++;
  }
  return false;
}

AccountSet::AccountSet() :
    m_count(0),
    m_file(MyMoneyFile::instance()),
    m_favorites(0),
    m_hideClosedAccounts(true)
{
}

void AccountSet::addAccountGroup(MyMoneyAccount::accountTypeE group)
{
  if (group == MyMoneyAccount::Asset) {
    m_typeList << MyMoneyAccount::Checkings;
    m_typeList << MyMoneyAccount::Savings;
    m_typeList << MyMoneyAccount::Cash;
    m_typeList << MyMoneyAccount::AssetLoan;
    m_typeList << MyMoneyAccount::CertificateDep;
    m_typeList << MyMoneyAccount::Investment;
    m_typeList << MyMoneyAccount::Stock;
    m_typeList << MyMoneyAccount::MoneyMarket;
    m_typeList << MyMoneyAccount::Asset;
    m_typeList << MyMoneyAccount::Currency;

  } else if (group == MyMoneyAccount::Liability) {
    m_typeList << MyMoneyAccount::CreditCard;
    m_typeList << MyMoneyAccount::Loan;
    m_typeList << MyMoneyAccount::Liability;

  } else if (group == MyMoneyAccount::Income) {
    m_typeList << MyMoneyAccount::Income;

  } else if (group == MyMoneyAccount::Expense) {
    m_typeList << MyMoneyAccount::Expense;

  } else if (group == MyMoneyAccount::Equity) {
    m_typeList << MyMoneyAccount::Equity;
  }
}

void AccountSet::addAccountType(MyMoneyAccount::accountTypeE type)
{
  m_typeList << type;
}

void AccountSet::removeAccountType(MyMoneyAccount::accountTypeE type)
{
  int index = m_typeList.indexOf(type);
  if (index != -1) {
    m_typeList.removeAt(index);
  }
}

void AccountSet::clear()
{
  m_typeList.clear();
}

int AccountSet::load(kMyMoneyAccountSelector* selector)
{
  QStringList list;
  QStringList::ConstIterator it_l;
  int count = 0;
  int typeMask = 0;
  QString currentId;

  if (selector->selectionMode() == QTreeWidget::SingleSelection) {
    QStringList list;
    selector->selectedItems(list);
    if (!list.isEmpty())
      currentId = list.first();
  }
  if (m_typeList.contains(MyMoneyAccount::Checkings)
      || m_typeList.contains(MyMoneyAccount::Savings)
      || m_typeList.contains(MyMoneyAccount::Cash)
      || m_typeList.contains(MyMoneyAccount::AssetLoan)
      || m_typeList.contains(MyMoneyAccount::CertificateDep)
      || m_typeList.contains(MyMoneyAccount::Investment)
      || m_typeList.contains(MyMoneyAccount::Stock)
      || m_typeList.contains(MyMoneyAccount::MoneyMarket)
      || m_typeList.contains(MyMoneyAccount::Asset)
      || m_typeList.contains(MyMoneyAccount::Currency))
    typeMask |= KMyMoneyUtils::asset;

  if (m_typeList.contains(MyMoneyAccount::CreditCard)
      || m_typeList.contains(MyMoneyAccount::Loan)
      || m_typeList.contains(MyMoneyAccount::Liability))
    typeMask |= KMyMoneyUtils::liability;

  if (m_typeList.contains(MyMoneyAccount::Income))
    typeMask |= KMyMoneyUtils::income;

  if (m_typeList.contains(MyMoneyAccount::Expense))
    typeMask |= KMyMoneyUtils::expense;

  if (m_typeList.contains(MyMoneyAccount::Equity))
    typeMask |= KMyMoneyUtils::equity;

  selector->clear();
  QTreeWidget* lv = selector->listView();
  m_count = 0;
  QString key;
  QTreeWidgetItem* after = 0;

  // create the favorite section first and sort it to the beginning
  key = QString("A%1").arg(i18n("Favorites"));
  m_favorites = selector->newItem(i18n("Favorites"), key);

  //get the account icon from cache or insert it if it is not there
  QPixmap accountPixmap;
  if (!QPixmapCache::find("account", accountPixmap)) {
    accountPixmap = DesktopIcon("view-bank-account");
    QPixmapCache::insert("account", accountPixmap);
  }
  m_favorites->setIcon(0, QIcon(accountPixmap));

  for (int mask = 0x01; mask != KMyMoneyUtils::last; mask <<= 1) {
    QTreeWidgetItem* item = 0;
    if ((typeMask & mask & KMyMoneyUtils::asset) != 0) {
      ++m_count;
      key = QString("B%1").arg(i18n("Asset"));
      item = selector->newItem(i18n("Asset accounts"), key);
      item->setIcon(0, m_file->asset().accountPixmap());
      list = m_file->asset().accountList();
    }

    if ((typeMask & mask & KMyMoneyUtils::liability) != 0) {
      ++m_count;
      key = QString("C%1").arg(i18n("Liability"));
      item = selector->newItem(i18n("Liability accounts"), key);
      item->setIcon(0, m_file->liability().accountPixmap());
      list = m_file->liability().accountList();
    }

    if ((typeMask & mask & KMyMoneyUtils::income) != 0) {
      ++m_count;
      key = QString("D%1").arg(i18n("Income"));
      item = selector->newItem(i18n("Income categories"), key);
      item->setIcon(0, m_file->income().accountPixmap());
      list = m_file->income().accountList();
      if (selector->selectionMode() == QTreeWidget::MultiSelection) {
        //selector->m_incomeCategoriesButton->show();
      }
    }

    if ((typeMask & mask & KMyMoneyUtils::expense) != 0) {
      ++m_count;
      key = QString("E%1").arg(i18n("Expense"));
      item = selector->newItem(i18n("Expense categories"), key);
      item->setIcon(0, m_file->expense().accountPixmap());
      list = m_file->expense().accountList();
      if (selector->selectionMode() == QTreeWidget::MultiSelection) {
        //selector->m_expenseCategoriesButton->show();
      }
    }

    if ((typeMask & mask & KMyMoneyUtils::equity) != 0) {
      ++m_count;
      key = QString("F%1").arg(i18n("Equity"));
      item = selector->newItem(i18n("Equity accounts"), key);
      item->setIcon(0, m_file->equity().accountPixmap());
      list = m_file->equity().accountList();
    }

    if (!after)
      after = item;

    if (item != 0) {
      // scan all matching accounts found in the engine
      for (it_l = list.constBegin(); it_l != list.constEnd(); ++it_l) {
        const MyMoneyAccount& acc = m_file->account(*it_l);
        ++m_count;
        ++count;
        //this will include an account if it matches the account type and
        //if it is still open or it has been set to show closed accounts
        if (includeAccount(acc)
            && (!isHidingClosedAccounts() || !acc.isClosed())) {
          QString tmpKey;
          tmpKey = key + MyMoneyFile::AccountSeparator + acc.name();
          QTreeWidgetItem* subItem = selector->newItem(item, acc.name(), tmpKey, acc.id());
          subItem->setIcon(0, acc.accountPixmap());
          if (acc.value("PreferredAccount") == "Yes"
              && m_typeList.contains(acc.accountType())) {
            selector->newItem(m_favorites, acc.name(), tmpKey, acc.id())->setIcon(0, acc.accountPixmap());;
          }
          if (acc.accountList().count() > 0) {
            subItem->setExpanded(true);
            count += loadSubAccounts(selector, subItem, tmpKey, acc.accountList());
          }

          // the item is not selectable if it has been added only because a subaccount matches the type
          if (!m_typeList.contains(acc.accountType())) {
            selector->setSelectable(subItem, false);
          }
          subItem->sortChildren(1, Qt::AscendingOrder);
        }
      }
      item->sortChildren(1, Qt::AscendingOrder);
    }
  }
  m_favorites->sortChildren(1, Qt::AscendingOrder);
  lv->invisibleRootItem()->sortChildren(1, Qt::AscendingOrder);

  // if we don't have a favorite account or the selector is for multi-mode
  // we get rid of the favorite entry and subentries.
  if (m_favorites->childCount() == 0 || selector->selectionMode() == QTreeWidget::MultiSelection) {
    delete m_favorites;
    m_favorites = 0;
  }

  if (lv->itemAt(0, 0)) {
    if (currentId.isEmpty()) {
      lv->setCurrentItem(lv->itemAt(0, 0));
      lv->clearSelection();
    } else {
      selector->setSelected(currentId);
    }
  }
  selector->update();
  return count;
}

int AccountSet::load(kMyMoneyAccountSelector* selector, const QString& baseName, const QList<QString>& accountIdList, const bool clear)
{
  int count = 0;
  QTreeWidgetItem* item = 0;

  m_typeList.clear();
  if (clear) {
    m_count = 0;
    selector->clear();
  }

  item = selector->newItem(baseName);
  ++m_count;

  QList<QString>::ConstIterator it;
  for (it = accountIdList.constBegin(); it != accountIdList.constEnd(); ++it)   {
    const MyMoneyAccount& acc = m_file->account(*it);
    if (acc.isClosed())
      continue;
    QString tmpKey;
    // the first character must be preset. Since we don't know any sort order here, we just use A
    tmpKey = QString("A%1%2%3").arg(baseName, MyMoneyFile::AccountSeparator, acc.name());
    selector->newItem(item, acc.name(), tmpKey, acc.id())->setIcon(0, acc.accountPixmap());
    ++m_count;
    ++count;
  }

  QTreeWidget* lv = selector->listView();
  if (lv->itemAt(0, 0)) {
    lv->setCurrentItem(lv->itemAt(0, 0));
    lv->clearSelection();
  }

  selector->update();
  return count;
}

int AccountSet::loadSubAccounts(kMyMoneyAccountSelector* selector, QTreeWidgetItem* parent, const QString& key, const QStringList& list)
{
  QStringList::ConstIterator it_l;
  int count = 0;

  for (it_l = list.constBegin(); it_l != list.constEnd(); ++it_l) {
    const MyMoneyAccount& acc = m_file->account(*it_l);
    // don't include stock accounts if not in expert mode
    if (acc.isInvest() && !KMyMoneyGlobalSettings::expertMode())
      continue;

    //this will include an account if it matches the account type and
    //if it is still open or it has been set to show closed accounts
    if (includeAccount(acc)
        && (!isHidingClosedAccounts() || !acc.isClosed())) {
      QString tmpKey;
      tmpKey = key + MyMoneyFile::AccountSeparator + acc.name();
      ++count;
      ++m_count;
      QTreeWidgetItem* item = selector->newItem(parent, acc.name(), tmpKey, acc.id());
      item->setIcon(0, acc.accountPixmap());
      if (acc.value("PreferredAccount") == "Yes"
          && m_typeList.contains(acc.accountType())) {
        selector->newItem(m_favorites, acc.name(), tmpKey, acc.id())->setIcon(0, acc.accountPixmap());
      }
      if (acc.accountList().count() > 0) {
        item->setExpanded(true);
        count += loadSubAccounts(selector, item, tmpKey, acc.accountList());
      }

      // the item is not selectable if it has been added only because a subaccount matches the type
      if (!m_typeList.contains(acc.accountType())) {
        selector->setSelectable(item, false);
      }
      item->sortChildren(1, Qt::AscendingOrder);
    }
  }
  return count;
}

bool AccountSet::includeAccount(const MyMoneyAccount& acc)
{
  if (m_typeList.contains(acc.accountType()))
    return true;

  QStringList accounts = acc.accountList();

  if (accounts.size() > 0) {
    QStringList::ConstIterator it_acc;
    for (it_acc = accounts.constBegin(); it_acc != accounts.constEnd(); ++it_acc) {
      MyMoneyAccount account = m_file->account(*it_acc);
      if (includeAccount(account))
        return true;
    }
  }
  return false;
}
