/***************************************************************************
                          mymoneyfile.cpp
                             -------------------
    copyright    : (C) 2000 by Michael Edwardes <mte@users.sourceforge.net>
                   (C) 2002, 2007-2011 by Thomas Baumgart <ipwizard@users.sourceforge.net>

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mymoneyfile.h"

#include <utility>

// ----------------------------------------------------------------------------
// QT Includes

#include <QString>
#include <QDateTime>
#include <QList>
#include <QtGlobal>
#include <QVariant>
#include <QUuid>
#include <QSharedPointer>

// ----------------------------------------------------------------------------
// KDE Includes

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>

// ----------------------------------------------------------------------------
// Project Includes
#include "storage/mymoneyseqaccessmgr.h"
#include "mymoneyaccount.h"
#include "mymoneyreport.h"
#include "mymoneybalancecache.h"
#include "mymoneybudget.h"
#include "mymoneyprice.h"
#include "mymoneyobjectcontainer.h"
#include "mymoneypayee.h"
//#include "mymoneytag.h"

// include the following line to get a 'cout' for debug purposes
// #include <iostream>

const QString MyMoneyFile::AccountSeparator = QChar(':');

MyMoneyFile MyMoneyFile::file;

typedef QList<std::pair<QString, QDate> > BalanceNotifyList;
typedef QMap<QString, bool> CacheNotifyList;

class MyMoneyNotification
{
public:
  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneyAccount& acc) :
      m_objType(MyMoneyFile::notifyAccount),
      m_notificationMode(mode),
      m_id(acc.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneyInstitution& institution) :
      m_objType(MyMoneyFile::notifyInstitution),
      m_notificationMode(mode),
      m_id(institution.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneyPayee& payee) :
      m_objType(MyMoneyFile::notifyPayee),
      m_notificationMode(mode),
      m_id(payee.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneyTag& tag) :
      m_objType(MyMoneyFile::notifyTag),
      m_notificationMode(mode),
      m_id(tag.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneySchedule& schedule) :
      m_objType(MyMoneyFile::notifySchedule),
      m_notificationMode(mode),
      m_id(schedule.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const MyMoneySecurity& security) :
      m_objType(MyMoneyFile::notifySecurity),
      m_notificationMode(mode),
      m_id(security.id()) {
  }

  MyMoneyNotification(MyMoneyFile::notificationModeT mode, const onlineJob& job) :
      m_objType(MyMoneyFile::notifyOnlineJob),
      m_notificationMode(mode),
      m_id(job.id()) {
  }

  MyMoneyFile::notificationObjectT objectType() const {
    return m_objType;
  }
  MyMoneyFile::notificationModeT notificationMode() const {
    return m_notificationMode;
  }
  const QString& id() const {
    return m_id;
  }

protected:
  MyMoneyNotification(MyMoneyFile::notificationObjectT obj,
                      MyMoneyFile::notificationModeT mode,
                      const QString& id) :
      m_objType(obj),
      m_notificationMode(mode),
      m_id(id) {}

private:
  MyMoneyFile::notificationObjectT   m_objType;
  MyMoneyFile::notificationModeT     m_notificationMode;
  QString                            m_id;
};





class MyMoneyFile::Private
{
public:
  Private() :
      m_storage(0),
      m_inTransaction(false) {}

  ~Private() {
    delete m_storage;
  }
  /**
    * This method is used to add an id to the list of objects
    * to be removed from the cache. If id is empty, then nothing is added to the list.
    *
    * @param id id of object to be notified
    * @param reload reload the object (@c true) or not (@c false). The default is @c true
    * @see attach, detach
    */
  void addCacheNotification(const QString& id, bool reload = true) {
    if (!id.isEmpty())
      m_notificationList[id] = reload;
  }

  void addCacheNotification(const QString& id, const QDate& date, bool reload = true) {
    if (!id.isEmpty()) {
      m_notificationList[id] = reload;
      m_balanceNotifyList.append(std::make_pair(id, date));
    }
  }

  /**
    * This method is used to clear the notification list
    */
  void clearCacheNotification() {
    // reset list to be empty
    m_notificationList.clear();
    m_balanceNotifyList.clear();
  }

  /**
    * This method is used to clear all
    * objects mentioned in m_notificationList from the cache.
    */
  void notify() {
    QMap<QString, bool>::ConstIterator it = m_notificationList.constBegin();
    while (it != m_notificationList.constEnd()) {
      if (*it)
        m_cache.refresh(it.key());
      else
        m_cache.clear(it.key());
      ++it;
    }

    foreach (const BalanceNotifyList::value_type & i, m_balanceNotifyList) {
      m_balanceChangedSet += i.first;
      if (i.second.isValid()) {
        m_balanceCache.clear(i.first, i.second);
      } else {
        m_balanceCache.clear(i.first);
      }
    }

    clearCacheNotification();
  }

  /**
    * This method checks if a storage object is attached and
    * throws and exception if not.
    */
  inline void checkStorage() const {
    if (m_storage == 0)
      throw MYMONEYEXCEPTION("No storage object attached to MyMoneyFile");
  }

  /**
    * This method checks that a transaction has been started with
    * startTransaction() and throws an exception otherwise. Calls
    * checkStorage() to make sure a storage object is present and attached.
    */
  void checkTransaction(const char* txt) const {
    checkStorage();
    if (!m_inTransaction) {
      throw MYMONEYEXCEPTION(QString("No transaction started for %1").arg(txt));
    }
  }

  void priceChanged(const MyMoneyFile& file, const MyMoneyPrice price) {
    // get all affected accounts and add them to the m_valueChangedSet
    QList<MyMoneyAccount> accList;
    file.accountList(accList);
    QList<MyMoneyAccount>::const_iterator account_it;
    for (account_it = accList.constBegin(); account_it != accList.constEnd(); ++account_it) {
      QString currencyId = account_it->currencyId();
      if (currencyId != file.baseCurrency().id() && (currencyId == price.from() || currencyId == price.to())) {
        // this account is not in the base currency and the price affects it's value
        m_valueChangedSet.insert(account_it->id());
      }
    }
  }

  /**
    * This member points to the storage strategy
    */
  IMyMoneyStorage *m_storage;


  bool                   m_inTransaction;
  MyMoneySecurity        m_baseCurrency;

  /**
   * @brief Cache for MyMoneyObjects
   *
   * It is also used to emit the objectAdded() and objectModified() signals.
   * => If one of these signals is used, you must use this cache.
   */
  MyMoneyObjectContainer m_cache;
  MyMoneyPriceList       m_priceCache;
  MyMoneyBalanceCache    m_balanceCache;

  /**
    * This member keeps a list of ids to notify after a single
    * operation is completed. The boolean is used as follows
    * during processing of the list:
    *
    * false - don't reload the object immediately
    * true  - reload the object immediately
    */
  CacheNotifyList   m_notificationList;

  /**
    * This member keeps a list of account ids to notify
    * after a single operation is completed. The balance cache
    * is cleared for that account and all dates on or after
    * the one supplied. If the date is invalid, the entire
    * balance cache is cleared for that account.
    */
  BalanceNotifyList m_balanceNotifyList;

  /**
    * This member keeps a list of account ids for which
    * a balanceChanged() signal needs to be emitted when
    * a set of operations has been committed.
    *
    * @sa MyMoneyFile::commitTransaction()
    */
  QSet<QString>     m_balanceChangedSet;

  /**
    * This member keeps a list of account ids for which
    * a valueChanged() signal needs to be emitted when
    * a set of operations has been committed.
    *
    * @sa MyMoneyFile::commitTransaction()
    */
  QSet<QString>     m_valueChangedSet;

  /**
    * This member keeps the list of changes in the engine
    * in historical order. The type can be 'added', 'modified'
    * or removed.
    */
  QList<MyMoneyNotification> m_changeSet;
};


class MyMoneyNotifier
{
public:
  MyMoneyNotifier(MyMoneyFile::Private* file) {
    m_file = file; m_file->clearCacheNotification();
  };
  ~MyMoneyNotifier() {
    m_file->notify();
  };
private:
  MyMoneyFile::Private* m_file;
};



MyMoneyFile::MyMoneyFile() :
    d(new Private)
{
}

MyMoneyFile::~MyMoneyFile()
{
  delete d;
}

MyMoneyFile::MyMoneyFile(IMyMoneyStorage *storage) :
    d(new Private)
{
  attachStorage(storage);
}

void MyMoneyFile::attachStorage(IMyMoneyStorage* const storage)
{
  if (d->m_storage != 0)
    throw MYMONEYEXCEPTION("Storage already attached");

  if (storage == 0)
    throw MYMONEYEXCEPTION("Storage must not be 0");

  d->m_storage = storage;

  // force reload of base currency
  d->m_baseCurrency = MyMoneySecurity();

  // and the whole cache
  d->m_balanceCache.clear();
  d->m_cache.clear(storage);
  d->m_priceCache.clear();
  preloadCache();

  // notify application about new data availability
  emit dataChanged();
}

void MyMoneyFile::detachStorage(IMyMoneyStorage* const /* storage */)
{
  d->m_balanceCache.clear();
  d->m_cache.clear();
  d->m_priceCache.clear();
  d->m_storage = 0;
}

IMyMoneyStorage* MyMoneyFile::storage() const
{
  return d->m_storage;
}

bool MyMoneyFile::storageAttached() const
{
  return d->m_storage != 0;
}

void MyMoneyFile::startTransaction()
{
  d->checkStorage();
  if (d->m_inTransaction) {
    throw MYMONEYEXCEPTION("Already started a transaction!");
  }

  d->m_storage->startTransaction();
  d->m_inTransaction = true;
  d->m_changeSet.clear();
}

bool MyMoneyFile::hasTransaction() const
{
  return d->m_inTransaction;
}

void MyMoneyFile::commitTransaction()
{
  d->checkTransaction(Q_FUNC_INFO);

  // commit the transaction in the storage
  bool changed = d->m_storage->commitTransaction();
  d->m_inTransaction = false;

  // Now it's time to send out some signals to the outside world
  // First we go through the d->m_changeSet and emit respective
  // signals about addition, modification and removal of engine objects
  QList<MyMoneyNotification>::const_iterator it = d->m_changeSet.constBegin();
  while (it != d->m_changeSet.constEnd()) {
    if ((*it).notificationMode() == notifyRemove) {
      emit objectRemoved((*it).objectType(), (*it).id());
      // if there is a balance change recorded for this account remove it since the account itself will be removed
      // this can happen when deleting categories that have transactions and the reassign category feature was used
      d->m_balanceChangedSet.remove((*it).id());
    } else {
      const MyMoneyObject * const obj = d->m_cache.object((*it).id());
      if (obj) {
        if ((*it).notificationMode() == notifyAdd) {
          emit objectAdded((*it).objectType(), obj);

        } else {
          emit objectModified((*it).objectType(), obj);
        }
      }
    }
    ++it;
  }

  // we're done with the change set, so we clear it
  d->m_changeSet.clear();

  // now send out the balanceChanged signal for all those
  // accounts for which we have an indication about a possible
  // change.
  foreach (const QString& id, d->m_balanceChangedSet) {
    // if we notify about balance change we don't need to notify about value change
    // for the same account since a balance change implies a value change
    d->m_valueChangedSet.remove(id);
    const MyMoneyAccount& acc = d->m_cache.account(id);
    emit balanceChanged(acc);
  }
  d->m_balanceChangedSet.clear();

  // now notify about the remaining value changes
  foreach (const QString& id, d->m_valueChangedSet) {
    const MyMoneyAccount& acc = d->m_cache.account(id);
    emit valueChanged(acc);
  }
  d->m_valueChangedSet.clear();

  // as a last action, send out the global dataChanged signal
  if (changed) {
    emit dataChanged();
  }
}

void MyMoneyFile::rollbackTransaction()
{
  d->checkTransaction(Q_FUNC_INFO);

  d->m_storage->rollbackTransaction();
  d->m_inTransaction = false;
  preloadCache();
  d->m_balanceChangedSet.clear();
  d->m_valueChangedSet.clear();
  d->m_changeSet.clear();
}

void MyMoneyFile::addInstitution(MyMoneyInstitution& institution)
{
  // perform some checks to see that the institution stuff is OK. For
  // now we assume that the institution must have a name, the ID is not set
  // and it does not have a parent (MyMoneyFile).

  if (institution.name().length() == 0
      || institution.id().length() != 0)
    throw MYMONEYEXCEPTION("Not a new institution");

  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addInstitution(institution);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadInstitution(institution);

  d->m_changeSet += MyMoneyNotification(notifyAdd, institution);
}

void MyMoneyFile::modifyInstitution(const MyMoneyInstitution& institution)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->modifyInstitution(institution);

  d->addCacheNotification(institution.id());
  d->m_changeSet += MyMoneyNotification(notifyModify, institution);
}

void MyMoneyFile::modifyTransaction(const MyMoneyTransaction& transaction)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyTransaction tCopy(transaction);

  // now check the splits
  bool loanAccountAffected = false;
  QList<MyMoneySplit>::ConstIterator it_s;
  for (it_s = transaction.splits().constBegin(); it_s != transaction.splits().constEnd(); ++it_s) {
    // the following line will throw an exception if the
    // account does not exist
    MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());
    if (acc.id().isEmpty())
      throw MYMONEYEXCEPTION("Cannot store split with no account assigned");
    if (isStandardAccount((*it_s).accountId()))
      throw MYMONEYEXCEPTION("Cannot store split referencing standard account");
    if (acc.isLoan() && ((*it_s).action() == MyMoneySplit::ActionTransfer))
      loanAccountAffected = true;
  }

  // change transfer splits between asset/liability and loan accounts
  // into amortization splits
  if (loanAccountAffected) {
    QList<MyMoneySplit> list = transaction.splits();
    for (it_s = list.constBegin(); it_s != list.constEnd(); ++it_s) {
      if ((*it_s).action() == MyMoneySplit::ActionTransfer) {
        MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());

        if (acc.isAssetLiability()) {
          MyMoneySplit s = (*it_s);
          s.setAction(MyMoneySplit::ActionAmortization);
          tCopy.modifySplit(s);
        }
      }
    }
  }

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // get the current setting of this transaction
  MyMoneyTransaction tr = MyMoneyFile::transaction(transaction.id());

  // scan the splits again to update notification list
  // and mark all accounts that are referenced
  for (it_s = tr.splits().constBegin(); it_s != tr.splits().constEnd(); ++it_s) {
    d->addCacheNotification((*it_s).accountId(), tr.postDate());
    d->addCacheNotification((*it_s).payeeId());
    //FIXME-ALEX Do I need to add d->addCacheNotification((*it_s).tagList()); ??
  }

  // make sure the value is rounded to the accounts precision
  fixSplitPrecision(tCopy);

  // perform modification
  d->m_storage->modifyTransaction(tCopy);

  // and mark all accounts that are referenced
  for (it_s = tCopy.splits().constBegin(); it_s != tCopy.splits().constEnd(); ++it_s) {
    d->addCacheNotification((*it_s).accountId(), tCopy.postDate());
    d->addCacheNotification((*it_s).payeeId());
    //FIXME-ALEX Do I need to add d->addCacheNotification((*it_s).tagList()); ??
  }
}

void MyMoneyFile::modifyAccount(const MyMoneyAccount& _account)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyAccount account(_account);

  MyMoneyAccount acc = MyMoneyFile::account(account.id());

  // check that for standard accounts only specific parameters are changed
  if (isStandardAccount(account.id())) {
    // make sure to use the stuff we found on file
    account = acc;

    // and only use the changes that are allowed
    account.setName(_account.name());
    account.setCurrencyId(_account.currencyId());

    // now check that it is the same
    if (!(account == _account))
      throw MYMONEYEXCEPTION("Unable to modify the standard account groups");
  }

  if (account.accountType() != acc.accountType() &&
      !account.isLiquidAsset() && !acc.isLiquidAsset())
    throw MYMONEYEXCEPTION("Unable to change account type");

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // if the account was moved to another institution, we notify
  // the old one as well as the new one and the structure change
  if (acc.institutionId() != account.institutionId()) {
    MyMoneyInstitution inst;
    if (!acc.institutionId().isEmpty()) {
      inst = institution(acc.institutionId());
      inst.removeAccountId(acc.id());
      modifyInstitution(inst);
      // modifyInstitution updates d->m_changeSet already
    }
    if (!account.institutionId().isEmpty()) {
      inst = institution(account.institutionId());
      inst.addAccountId(acc.id());
      modifyInstitution(inst);
      // modifyInstitution updates d->m_changeSet already
    }
    d->addCacheNotification(acc.institutionId());
    d->addCacheNotification(account.institutionId());
  }

  d->m_storage->modifyAccount(account);

  d->addCacheNotification(account.id());
  d->m_changeSet += MyMoneyNotification(notifyModify, account);
}

void MyMoneyFile::reparentAccount(MyMoneyAccount &acc, MyMoneyAccount& parent)
{
  d->checkTransaction(Q_FUNC_INFO);

  // check that it's not one of the standard account groups
  if (isStandardAccount(acc.id()))
    throw MYMONEYEXCEPTION("Unable to reparent the standard account groups");

  if (acc.accountGroup() == parent.accountGroup()
      || (acc.accountType() == MyMoneyAccount::Income && parent.accountType() == MyMoneyAccount::Expense)
      || (acc.accountType() == MyMoneyAccount::Expense && parent.accountType() == MyMoneyAccount::Income)) {

    if (acc.isInvest() && parent.accountType() != MyMoneyAccount::Investment)
      throw MYMONEYEXCEPTION("Unable to reparent Stock to non-investment account");

    if (parent.accountType() == MyMoneyAccount::Investment && !acc.isInvest())
      throw MYMONEYEXCEPTION("Unable to reparent non-stock to investment account");

    // clear all changed objects from cache
    MyMoneyNotifier notifier(d);

    // keep a notification of the current parent
    MyMoneyAccount curParent = account(acc.parentAccountId());

    d->addCacheNotification(curParent.id());

    d->m_storage->reparentAccount(acc, parent);

    // and also keep one for the account itself and the new parent
    d->addCacheNotification(acc.id());
    d->addCacheNotification(parent.id());

    d->m_changeSet += MyMoneyNotification(notifyModify, curParent);
    d->m_changeSet += MyMoneyNotification(notifyModify, parent);
    d->m_changeSet += MyMoneyNotification(notifyModify, acc);

  } else
    throw MYMONEYEXCEPTION("Unable to reparent to different account type");
}

const MyMoneyInstitution& MyMoneyFile::institution(const QString& id) const
{
  return d->m_cache.institution(id);
}

const MyMoneyAccount& MyMoneyFile::account(const QString& id) const
{
  return d->m_cache.account(id);
}

const MyMoneyAccount& MyMoneyFile::subAccountByName(const MyMoneyAccount& acc, const QString& name) const
{
  static MyMoneyAccount nullAccount;

  QList<QString>::const_iterator it_a;
  for (it_a = acc.accountList().constBegin(); it_a != acc.accountList().constEnd(); ++it_a) {
    const MyMoneyAccount& sacc = account(*it_a);
    if (sacc.name() == name)
      return sacc;
  }
  return nullAccount;
}

const MyMoneyAccount& MyMoneyFile::accountByName(const QString& name) const
{
  return d->m_cache.accountByName(name);
}

void MyMoneyFile::removeTransaction(const MyMoneyTransaction& transaction)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // get the engine's idea about this transaction
  MyMoneyTransaction tr = MyMoneyFile::transaction(transaction.id());
  QList<MyMoneySplit>::ConstIterator it_s;

  // scan the splits again to update notification list
  for (it_s = tr.splits().constBegin(); it_s != tr.splits().constEnd(); ++it_s) {
    MyMoneyAccount acc = account((*it_s).accountId());
    if (acc.isClosed())
      throw MYMONEYEXCEPTION(i18n("Cannot remove transaction that references a closed account."));
    d->addCacheNotification((*it_s).accountId(), tr.postDate());
    d->addCacheNotification((*it_s).payeeId());
    //FIXME-ALEX Do I need to add d->addCacheNotification((*it_s).tagList()); ??
  }

  d->m_storage->removeTransaction(transaction);
}


bool MyMoneyFile::hasActiveSplits(const QString& id) const
{
  d->checkStorage();

  return d->m_storage->hasActiveSplits(id);
}

bool MyMoneyFile::isStandardAccount(const QString& id) const
{
  d->checkStorage();

  return d->m_storage->isStandardAccount(id);
}

void MyMoneyFile::setAccountName(const QString& id, const QString& name) const
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyNotifier notifier(d);

  MyMoneyAccount acc = account(id);
  d->m_storage->setAccountName(id, name);
  d->addCacheNotification(id);

  d->m_changeSet += MyMoneyNotification(notifyModify, acc);
}

void MyMoneyFile::removeAccount(const MyMoneyAccount& account)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyAccount parent;
  MyMoneyAccount acc;
  MyMoneyInstitution institution;

  // check that the account and its parent exist
  // this will throw an exception if the id is unknown
  acc = MyMoneyFile::account(account.id());
  parent = MyMoneyFile::account(account.parentAccountId());
  if (!acc.institutionId().isEmpty())
    institution = MyMoneyFile::institution(acc.institutionId());

  // check that it's not one of the standard account groups
  if (isStandardAccount(account.id()))
    throw MYMONEYEXCEPTION("Unable to remove the standard account groups");

  if (hasActiveSplits(account.id())) {
    throw MYMONEYEXCEPTION("Unable to remove account with active splits");
  }

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // collect all sub-ordinate accounts for notification
  foreach (const QString& id, acc.accountList()) {
    d->addCacheNotification(id);
    const MyMoneyAccount& acc = MyMoneyFile::account(id);
    d->m_changeSet += MyMoneyNotification(notifyModify, acc);
  }
  // don't forget the parent and a possible institution
  d->addCacheNotification(parent.id());
  d->addCacheNotification(account.institutionId());

  if (!institution.id().isEmpty()) {
    institution.removeAccountId(account.id());
    d->m_storage->modifyInstitution(institution);
    d->m_changeSet += MyMoneyNotification(notifyModify, institution);
  }
  acc.setInstitutionId(QString());

  d->m_storage->removeAccount(acc);

  d->addCacheNotification(acc.id(), false);
  d->m_cache.clear(acc.id());
  d->m_balanceCache.clear(acc.id());

  d->m_changeSet += MyMoneyNotification(notifyModify, parent);
  d->m_changeSet += MyMoneyNotification(notifyRemove, acc);
}

void MyMoneyFile::removeAccountList(const QStringList& account_list, unsigned int level)
{
  if (level > 100)
    throw MYMONEYEXCEPTION("Too deep recursion in [MyMoneyFile::removeAccountList]!");

  d->checkTransaction(Q_FUNC_INFO);

  // upon entry, we check that we could proceed with the operation
  if (!level) {
    if (!hasOnlyUnusedAccounts(account_list, 0)) {
      throw MYMONEYEXCEPTION("One or more accounts cannot be removed");
    }
  }

  // process all accounts in the list and test if they have transactions assigned
  for (QStringList::ConstIterator it = account_list.constBegin(); it != account_list.constEnd(); ++it) {
    MyMoneyAccount a = d->m_storage->account(*it);
    //kDebug() << "Deleting account '"<< a.name() << "'";

    // first remove all sub-accounts
    if (!a.accountList().isEmpty()) {
      removeAccountList(a.accountList(), level + 1);

      // then remove account itself, but we first have to get
      // rid of the account list that is still stored in
      // the MyMoneyAccount object. Easiest way is to get a fresh copy.
      a = d->m_storage->account(*it);
    }

    // make sure to remove the item from the cache
    d->m_cache.clear(a.id());
    removeAccount(a);
  }
}

bool MyMoneyFile::hasOnlyUnusedAccounts(const QStringList& account_list, unsigned int level)
{
  if (level > 100)
    throw MYMONEYEXCEPTION("Too deep recursion in [MyMoneyFile::hasOnlyUnusedAccounts]!");
  // process all accounts in the list and test if they have transactions assigned
  for (QStringList::ConstIterator it = account_list.constBegin(); it != account_list.constEnd(); ++it) {
    if (transactionCount(*it) != 0)
      return false; // the current account has a transaction assigned
    if (!hasOnlyUnusedAccounts(account(*it).accountList(), level + 1))
      return false; // some sub-account has a transaction assigned
  }
  return true; // all subaccounts unused
}


void MyMoneyFile::removeInstitution(const MyMoneyInstitution& institution)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  QList<QString>::ConstIterator it_a;
  MyMoneyInstitution inst = MyMoneyFile::institution(institution.id());

  bool blocked = signalsBlocked();
  blockSignals(true);
  for (it_a = inst.accountList().constBegin(); it_a != inst.accountList().constEnd(); ++it_a) {
    MyMoneyAccount acc = account(*it_a);
    acc.setInstitutionId(QString());
    modifyAccount(acc);
    d->m_changeSet += MyMoneyNotification(notifyModify, acc);
  }
  blockSignals(blocked);

  d->m_storage->removeInstitution(institution);

  d->m_changeSet += MyMoneyNotification(notifyRemove, institution);

  d->addCacheNotification(institution.id(), false);
}

void MyMoneyFile::addAccount(MyMoneyAccount& account, MyMoneyAccount& parent)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyInstitution institution;

  // perform some checks to see that the account stuff is OK. For
  // now we assume that the account must have a name, has no
  // transaction and sub-accounts and parent account
  // it's own ID is not set and it does not have a pointer to (MyMoneyFile)

  if (account.name().length() == 0)
    throw MYMONEYEXCEPTION("Account has no name");

  if (account.id().length() != 0)
    throw MYMONEYEXCEPTION("New account must have no id");

  if (account.accountList().count() != 0)
    throw MYMONEYEXCEPTION("New account must have no sub-accounts");

  if (!account.parentAccountId().isEmpty())
    throw MYMONEYEXCEPTION("New account must have no parent-id");

  if (account.accountType() == MyMoneyAccount::UnknownAccountType)
    throw MYMONEYEXCEPTION("Account has invalid type");

  // make sure, that the parent account exists
  // if not, an exception is thrown. If it exists,
  // get a copy of the current data
  MyMoneyAccount acc = MyMoneyFile::account(parent.id());

#if 0
  // TODO: remove the following code as we now can have multiple accounts
  // with the same name even in the same hierarchy position of the account tree
  //
  // check if the selected name is currently not among the child accounts
  // if we find one, then return it as the new account
  QStringList::const_iterator it_a;
  for (it_a = acc.accountList().begin(); it_a != acc.accountList().end(); ++it_a) {
    MyMoneyAccount a = MyMoneyFile::account(*it_a);
    if (account.name() == a.name()) {
      account = a;
      return;
    }
  }
#endif

  // FIXME: make sure, that the parent has the same type
  // I left it out here because I don't know, if there is
  // a tight coupling between e.g. checking accounts and the
  // class asset. It certainly does not make sense to create an
  // expense account under an income account. Maybe it does, I don't know.

  // We enforce, that a stock account can never be a parent and
  // that the parent for a stock account must be an investment. Also,
  // an investment cannot have another investment account as it's parent
  if (parent.isInvest())
    throw MYMONEYEXCEPTION("Stock account cannot be parent account");

  if (account.isInvest() && parent.accountType() != MyMoneyAccount::Investment)
    throw MYMONEYEXCEPTION("Stock account must have investment account as parent ");

  if (!account.isInvest() && parent.accountType() == MyMoneyAccount::Investment)
    throw MYMONEYEXCEPTION("Investment account can only have stock accounts as children");

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // if an institution is set, verify that it exists
  if (account.institutionId().length() != 0) {
    // check the presence of the institution. if it
    // does not exist, an exception is thrown
    institution = MyMoneyFile::institution(account.institutionId());
  }

  // if we don't have a valid opening date use today
  if (!account.openingDate().isValid()) {
    account.setOpeningDate(QDate::currentDate());
  }

  // make sure to set the opening date for categories to a
  // fixed date (1900-1-1). See #313793 on b.k.o for details
  if (account.isIncomeExpense()) {
    account.setOpeningDate(QDate(1900, 1, 1));
  }

  // if we don't have a currency assigned use the base currency
  if (account.currencyId().isEmpty()) {
    account.setCurrencyId(baseCurrency().id());
  }

  // make sure the parent id is setup
  account.setParentAccountId(parent.id());

  d->m_storage->addAccount(account);
  d->m_changeSet += MyMoneyNotification(notifyAdd, account);

  d->m_storage->addAccount(parent, account);
  d->m_changeSet += MyMoneyNotification(notifyModify, parent);

  if (account.institutionId().length() != 0) {
    institution.addAccountId(account.id());
    d->m_storage->modifyInstitution(institution);
    d->m_changeSet += MyMoneyNotification(notifyModify, institution);
    d->addCacheNotification(institution.id());
  }

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadAccount(account);

  d->addCacheNotification(parent.id());
}

MyMoneyTransaction MyMoneyFile::createOpeningBalanceTransaction(const MyMoneyAccount& acc, const MyMoneyMoney& balance)
{
  MyMoneyTransaction t;
  // if the opening balance is not zero, we need
  // to create the respective transaction
  if (!balance.isZero()) {
    d->checkTransaction(Q_FUNC_INFO);

    MyMoneySecurity currency = security(acc.currencyId());
    MyMoneyAccount openAcc = openingBalanceAccount(currency);

    if (openAcc.openingDate() > acc.openingDate()) {
      openAcc.setOpeningDate(acc.openingDate());
      modifyAccount(openAcc);
    }

    MyMoneySplit s;

    t.setPostDate(acc.openingDate());
    t.setCommodity(acc.currencyId());

    s.setAccountId(acc.id());
    s.setShares(balance);
    s.setValue(balance);
    t.addSplit(s);

    s.clearId();
    s.setAccountId(openAcc.id());
    s.setShares(-balance);
    s.setValue(-balance);
    t.addSplit(s);

    addTransaction(t);
  }
  return t;
}

QString MyMoneyFile::openingBalanceTransaction(const MyMoneyAccount& acc) const
{
  QString result;

  MyMoneySecurity currency = security(acc.currencyId());
  MyMoneyAccount openAcc;

  try {
    openAcc = openingBalanceAccount(currency);
  } catch (const MyMoneyException &) {
    return result;
  }

  // Iterate over all the opening balance transactions for this currency
  MyMoneyTransactionFilter filter;
  filter.addAccount(openAcc.id());
  QList<MyMoneyTransaction> transactions = transactionList(filter);
  QList<MyMoneyTransaction>::const_iterator it_t = transactions.constBegin();
  while (it_t != transactions.constEnd()) {
    try {
      // Test whether the transaction also includes a split into
      // this account
      (*it_t).splitByAccount(acc.id(), true /*match*/);

      // If so, we have a winner!
      result = (*it_t).id();
      break;
    } catch (const MyMoneyException &) {
      // If not, keep searching
      ++it_t;
    }
  }

  return result;
}

const MyMoneyAccount MyMoneyFile::openingBalanceAccount(const MyMoneySecurity& security)
{
  if (!security.isCurrency())
    throw MYMONEYEXCEPTION("Opening balance for non currencies not supported");

  try {
    return openingBalanceAccount_internal(security);
  } catch (const MyMoneyException &) {
    MyMoneyFileTransaction ft;
    MyMoneyAccount acc;

    try {
      acc = createOpeningBalanceAccount(security);
      ft.commit();

    } catch (const MyMoneyException &) {
      qDebug("Unable to create opening balance account for security %s", qPrintable(security.id()));
    }
    return acc;
  }
}

const MyMoneyAccount MyMoneyFile::openingBalanceAccount(const MyMoneySecurity& security) const
{
  return openingBalanceAccount_internal(security);
}

const MyMoneyAccount MyMoneyFile::openingBalanceAccount_internal(const MyMoneySecurity& security) const
{
  if (!security.isCurrency())
    throw MYMONEYEXCEPTION("Opening balance for non currencies not supported");

  MyMoneyAccount acc;
  QList<MyMoneyAccount> accounts;
  QList<MyMoneyAccount>::ConstIterator it;

  accountList(accounts, equity().accountList(), true);

  for (it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
    if (it->value("OpeningBalanceAccount") == QLatin1String("Yes")
        && it->currencyId() == security.id()) {
      acc = *it;
      break;
    }
  }

  if (acc.id().isEmpty()) {
    for (it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
      if (it->name().startsWith(MyMoneyFile::openingBalancesPrefix())
          && it->currencyId() == security.id()) {
        acc = *it;
        break;
      }
    }
  }

  if (acc.id().isEmpty()) {
    throw MYMONEYEXCEPTION(QString("No opening balance account for %1").arg(security.tradingSymbol()));
  }

  return acc;
}

const MyMoneyAccount MyMoneyFile::createOpeningBalanceAccount(const MyMoneySecurity& security)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyAccount acc;
  QList<MyMoneyAccount> accounts;
  QList<MyMoneyAccount>::ConstIterator it;

  accountList(accounts, equity().accountList(), true);

  // find present opening balance accounts without containing '('
  QString name;
  QString parentAccountId;
  QRegExp exp(QString("\\([A-Z]{3}\\)"));

  for (it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
    if (it->value("OpeningBalanceAccount") == QLatin1String("Yes")
        && exp.indexIn(it->name()) == -1) {
      name = it->name();
      parentAccountId = it->parentAccountId();
      break;
    }
  }

  if (name.isEmpty())
    name = MyMoneyFile::openingBalancesPrefix();
  if (security.id() != baseCurrency().id()) {
    name += QString(" (%1)").arg(security.id());
  }
  acc.setName(name);
  acc.setAccountType(MyMoneyAccount::Equity);
  acc.setCurrencyId(security.id());
  acc.setValue("OpeningBalanceAccount", "Yes");

  MyMoneyAccount parent = !parentAccountId.isEmpty() ? account(parentAccountId) : equity();
  this->addAccount(acc, parent);
  return acc;
}

void MyMoneyFile::addTransaction(MyMoneyTransaction& transaction)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // perform some checks to see that the transaction stuff is OK. For
  // now we assume that
  // * no ids are assigned
  // * the date valid (must not be empty)
  // * the referenced accounts in the splits exist

  // first perform all the checks
  if (!transaction.id().isEmpty())
    throw MYMONEYEXCEPTION("Unable to add transaction with id set");
  if (!transaction.postDate().isValid())
    throw MYMONEYEXCEPTION("Unable to add transaction with invalid postdate");

  // now check the splits
  bool loanAccountAffected = false;
  QList<MyMoneySplit>::ConstIterator it_s;
  for (it_s = transaction.splits().constBegin(); it_s != transaction.splits().constEnd(); ++it_s) {
    // the following line will throw an exception if the
    // account does not exist or is one of the standard accounts
    MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());
    if (acc.id().isEmpty())
      throw MYMONEYEXCEPTION("Cannot add split with no account assigned");
    if (acc.isLoan())
      loanAccountAffected = true;
    if (isStandardAccount((*it_s).accountId()))
      throw MYMONEYEXCEPTION("Cannot add split referencing standard account");
  }

  // change transfer splits between asset/liability and loan accounts
  // into amortization splits
  if (loanAccountAffected) {
    QList<MyMoneySplit> list = transaction.splits();
    for (it_s = list.constBegin(); it_s != list.constEnd(); ++it_s) {
      if ((*it_s).action() == MyMoneySplit::ActionTransfer) {
        MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());

        if (acc.isAssetLiability()) {
          MyMoneySplit s = (*it_s);
          s.setAction(MyMoneySplit::ActionAmortization);
          transaction.modifySplit(s);
        }
      }
    }
  }

  // check that we have a commodity
  if (transaction.commodity().isEmpty()) {
    transaction.setCommodity(baseCurrency().id());
  }

  // make sure the value is rounded to the accounts precision
  fixSplitPrecision(transaction);

  // then add the transaction to the file global pool
  d->m_storage->addTransaction(transaction);

  // scan the splits again to update notification list
  for (it_s = transaction.splits().constBegin(); it_s != transaction.splits().constEnd(); ++it_s) {
    d->addCacheNotification((*it_s).accountId(), transaction.postDate());
    d->addCacheNotification((*it_s).payeeId());
    //FIXME-ALEX Do I need to add d->addCacheNotification((*it_s).tagList()); ??
  }
}

const MyMoneyTransaction MyMoneyFile::transaction(const QString& id) const
{
  d->checkStorage();

  return d->m_storage->transaction(id);
}

const MyMoneyTransaction MyMoneyFile::transaction(const QString& account, const int idx) const
{
  d->checkStorage();

  return d->m_storage->transaction(account, idx);
}

void MyMoneyFile::addPayee(MyMoneyPayee& payee)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addPayee(payee);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadPayee(payee);

  d->m_changeSet += MyMoneyNotification(notifyAdd, payee);
}

const MyMoneyPayee& MyMoneyFile::payee(const QString& id) const
{
  return d->m_cache.payee(id);
}

const MyMoneyPayee& MyMoneyFile::payeeByName(const QString& name) const
{
  d->checkStorage();

  return d->m_cache.payee(d->m_storage->payeeByName(name).id());
}

void MyMoneyFile::modifyPayee(const MyMoneyPayee& payee)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->addCacheNotification(payee.id());

  d->m_storage->modifyPayee(payee);

  d->m_changeSet += MyMoneyNotification(notifyModify, payee);
}

void MyMoneyFile::removePayee(const MyMoneyPayee& payee)
{
  d->checkTransaction(Q_FUNC_INFO);

  // FIXME we need to make sure, that the payee is not referenced anymore

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removePayee(payee);

  d->addCacheNotification(payee.id(), false);

  d->m_changeSet += MyMoneyNotification(notifyRemove, payee);
}

void MyMoneyFile::addTag(MyMoneyTag& tag)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addTag(tag);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadTag(tag);

  d->m_changeSet += MyMoneyNotification(notifyAdd, tag);
}

const MyMoneyTag& MyMoneyFile::tag(const QString& id) const
{
  return d->m_cache.tag(id);
}

const MyMoneyTag& MyMoneyFile::tagByName(const QString& name) const
{
  d->checkStorage();

  return d->m_cache.tag(d->m_storage->tagByName(name).id());
}

void MyMoneyFile::modifyTag(const MyMoneyTag& tag)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->addCacheNotification(tag.id());

  d->m_storage->modifyTag(tag);

  d->m_changeSet += MyMoneyNotification(notifyModify, tag);
}

void MyMoneyFile::removeTag(const MyMoneyTag& tag)
{
  d->checkTransaction(Q_FUNC_INFO);

  // FIXME we need to make sure, that the tag is not referenced anymore

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removeTag(tag);

  d->addCacheNotification(tag.id(), false);

  d->m_changeSet += MyMoneyNotification(notifyRemove, tag);
}

void MyMoneyFile::accountList(QList<MyMoneyAccount>& list, const QStringList& idlist, const bool recursive) const
{
  if (idlist.isEmpty()) {
    d->m_cache.account(list);

#if 0
    // TODO: I have no idea what this was good for, but it caused the networth report
    //       to show double the numbers so I commented it out (ipwizard, 2008-05-24)
    if (d->m_storage && (list.isEmpty() || list.size() != d->m_storage->accountCount())) {
      d->m_storage->accountList(list);
      d->m_cache.preloadAccount(list);
    }
#endif

    QList<MyMoneyAccount>::Iterator it;
    for (it = list.begin(); it != list.end();) {
      if (isStandardAccount((*it).id())) {
        it = list.erase(it);
      } else {
        ++it;
      }
    }
  } else {
    QList<MyMoneyAccount>::ConstIterator it;
    QList<MyMoneyAccount> list_a;
    d->m_cache.account(list_a);

    for (it = list_a.constBegin(); it != list_a.constEnd(); ++it) {
      if (!isStandardAccount((*it).id())) {
        if (idlist.indexOf((*it).id()) != -1) {
          list.append(*it);
          if (recursive == true && !(*it).accountList().isEmpty()) {
            accountList(list, (*it).accountList(), true);
          }
        }
      }
    }
  }
}

void MyMoneyFile::institutionList(QList<MyMoneyInstitution>& list) const
{
  d->m_cache.institution(list);
}

const QList<MyMoneyInstitution> MyMoneyFile::institutionList() const
{
  QList<MyMoneyInstitution> list;
  institutionList(list);
  return list;
}

// general get functions
const MyMoneyPayee& MyMoneyFile::user() const
{
  d->checkStorage();
  return d->m_storage->user();
}

// general set functions
void MyMoneyFile::setUser(const MyMoneyPayee& user)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->setUser(user);
}

bool MyMoneyFile::dirty() const
{
  if (!d->m_storage)
    return false;

  return d->m_storage->dirty();
}

void MyMoneyFile::setDirty() const
{
  d->checkStorage();

  d->m_storage->setDirty();
}

unsigned int MyMoneyFile::accountCount() const
{
  d->checkStorage();

  return d->m_storage->accountCount();
}

void MyMoneyFile::ensureDefaultCurrency(MyMoneyAccount& acc) const
{
  if (acc.currencyId().isEmpty()) {
    if (!baseCurrency().id().isEmpty())
      acc.setCurrencyId(baseCurrency().id());
  }
}

const MyMoneyAccount& MyMoneyFile::liability() const
{
  d->checkStorage();

  return d->m_cache.account(STD_ACC_LIABILITY);
}

const MyMoneyAccount& MyMoneyFile::asset() const
{
  d->checkStorage();

  return d->m_cache.account(STD_ACC_ASSET);
}

const MyMoneyAccount& MyMoneyFile::expense() const
{
  d->checkStorage();

  return d->m_cache.account(STD_ACC_EXPENSE);
}

const MyMoneyAccount& MyMoneyFile::income() const
{
  d->checkStorage();

  return d->m_cache.account(STD_ACC_INCOME);
}

const MyMoneyAccount& MyMoneyFile::equity() const
{
  d->checkStorage();

  return d->m_cache.account(STD_ACC_EQUITY);
}

unsigned int MyMoneyFile::transactionCount(const QString& account) const
{
  d->checkStorage();

  return d->m_storage->transactionCount(account);
}

const QMap<QString, unsigned long> MyMoneyFile::transactionCountMap() const
{
  d->checkStorage();

  return d->m_storage->transactionCountMap();
}

unsigned int MyMoneyFile::institutionCount() const
{
  d->checkStorage();

  return d->m_storage->institutionCount();
}

const MyMoneyMoney MyMoneyFile::balance(const QString& id, const QDate& date) const
{
  if (date.isValid()) {
    MyMoneyBalanceCacheItem bal = d->m_balanceCache.balance(id, date);
    if (bal.isValid())
      return bal.balance();
  }

  d->checkStorage();

  MyMoneyMoney returnValue = d->m_storage->balance(id, date);

  if (date.isValid()) {
    d->m_balanceCache.insert(id, date, returnValue);
  }

  return returnValue;
}

const MyMoneyMoney MyMoneyFile::clearedBalance(const QString &id, const QDate& date) const
{
  MyMoneyMoney cleared;
  QList<MyMoneyTransaction> list;

  cleared = balance(id, date);

  MyMoneyAccount account = this->account(id);
  MyMoneyMoney factor(1, 1);
  if (account.accountGroup() == MyMoneyAccount::Liability || account.accountGroup() == MyMoneyAccount::Equity)
    factor = -factor;

  MyMoneyTransactionFilter filter;
  filter.addAccount(id);
  filter.setDateFilter(QDate(), date);
  filter.setReportAllSplits(false);
  filter.addState(MyMoneyTransactionFilter::notReconciled);
  transactionList(list, filter);

  for (QList<MyMoneyTransaction>::const_iterator it_t = list.constBegin(); it_t != list.constEnd(); ++it_t) {
    const QList<MyMoneySplit>& splits = (*it_t).splits();
    for (QList<MyMoneySplit>::const_iterator it_s = splits.constBegin(); it_s != splits.constEnd(); ++it_s) {
      const MyMoneySplit &split = (*it_s);
      if (split.accountId() != id)
        continue;
      cleared -= split.shares();
    }
  }
  return cleared * factor;
}

const MyMoneyMoney MyMoneyFile::totalBalance(const QString& id, const QDate& date) const
{
  d->checkStorage();

  return d->m_storage->totalBalance(id, date);
}

void MyMoneyFile::warningMissingRate(const QString& fromId, const QString& toId) const
{
  MyMoneySecurity from, to;
  try {
    from = security(fromId);
    to = security(toId);
    qWarning("Missing price info for conversion from %s to %s", qPrintable(from.name()), qPrintable(to.name()));

  } catch (const MyMoneyException &e) {
    qWarning("Missing security caught in MyMoneyFile::warningMissingRate(): %s(%ld) %s", qPrintable(e.file()), e.line(), qPrintable(e.what()));
  }
}

void MyMoneyFile::transactionList(QList<QPair<MyMoneyTransaction, MyMoneySplit> >& list, MyMoneyTransactionFilter& filter) const
{
  d->checkStorage();
  d->m_storage->transactionList(list, filter);
}

void MyMoneyFile::transactionList(QList<MyMoneyTransaction>& list, MyMoneyTransactionFilter& filter) const
{
  d->checkStorage();
  d->m_storage->transactionList(list, filter);
}

const QList<MyMoneyTransaction> MyMoneyFile::transactionList(MyMoneyTransactionFilter& filter) const
{
  QList<MyMoneyTransaction> list;
  transactionList(list, filter);
  return list;
}

const QList<MyMoneyPayee> MyMoneyFile::payeeList() const
{
  QList<MyMoneyPayee> list;
  d->m_cache.payee(list);
  return list;
}

const QList<MyMoneyTag> MyMoneyFile::tagList() const
{
  QList<MyMoneyTag> list;
  d->m_cache.tag(list);
  return list;
}

QString MyMoneyFile::accountToCategory(const QString& accountId, bool includeStandardAccounts) const
{
  MyMoneyAccount acc;
  QString rc;

  if (!accountId.isEmpty()) {
    acc = account(accountId);
    do {
      if (!rc.isEmpty())
        rc = AccountSeparator + rc;
      rc = acc.name() + rc;
      acc = account(acc.parentAccountId());
    } while (!acc.id().isEmpty() && (includeStandardAccounts || !isStandardAccount(acc.id())));
  }
  return rc;
}

QString MyMoneyFile::categoryToAccount(const QString& category, MyMoneyAccount::accountTypeE type) const
{
  QString id;

  // search the category in the expense accounts and if it is not found, try
  // to locate it in the income accounts
  if (type == MyMoneyAccount::UnknownAccountType
      || type == MyMoneyAccount::Expense) {
    id = locateSubAccount(MyMoneyFile::instance()->expense(), category);
  }

  if ((id.isEmpty() && type == MyMoneyAccount::UnknownAccountType)
      || type == MyMoneyAccount::Income) {
    id = locateSubAccount(MyMoneyFile::instance()->income(), category);
  }

  return id;
}

QString MyMoneyFile::nameToAccount(const QString& name) const
{
  QString id;

  // search the category in the asset accounts and if it is not found, try
  // to locate it in the liability accounts
  id = locateSubAccount(MyMoneyFile::instance()->asset(), name);
  if (id.isEmpty())
    id = locateSubAccount(MyMoneyFile::instance()->liability(), name);

  return id;
}

QString MyMoneyFile::parentName(const QString& name) const
{
  return name.section(AccountSeparator, 0, -2);
}

QString MyMoneyFile::locateSubAccount(const MyMoneyAccount& base, const QString& category) const
{
  MyMoneyAccount nextBase;
  QString level, remainder;
  level = category.section(AccountSeparator, 0, 0);
  remainder = category.section(AccountSeparator, 1);

  QStringList list = base.accountList();
  QStringList::ConstIterator it_a;

  for (it_a = list.constBegin(); it_a != list.constEnd(); ++it_a) {
    nextBase = account(*it_a);
    if (nextBase.name() == level) {
      if (remainder.isEmpty()) {
        return nextBase.id();
      }
      return locateSubAccount(nextBase, remainder);
    }
  }
  return QString();
}

QString MyMoneyFile::value(const QString& key) const
{
  d->checkStorage();

  return d->m_storage->value(key);
}

void MyMoneyFile::setValue(const QString& key, const QString& val)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->setValue(key, val);
}

void MyMoneyFile::deletePair(const QString& key)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->deletePair(key);
}

void MyMoneyFile::addSchedule(MyMoneySchedule& sched)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyTransaction transaction = sched.transaction();
  QList<MyMoneySplit>::ConstIterator it_s;
  for (it_s = transaction.splits().constBegin(); it_s != transaction.splits().constEnd(); ++it_s) {
    // the following line will throw an exception if the
    // account does not exist or is one of the standard accounts
    MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());
    if (acc.id().isEmpty())
      throw MYMONEYEXCEPTION("Cannot add split with no account assigned");
    if (isStandardAccount((*it_s).accountId()))
      throw MYMONEYEXCEPTION("Cannot add split referencing standard account");
  }

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addSchedule(sched);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadSchedule(sched);

  d->m_changeSet += MyMoneyNotification(notifyAdd, sched);
}

void MyMoneyFile::modifySchedule(const MyMoneySchedule& sched)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyTransaction transaction = sched.transaction();
  QList<MyMoneySplit>::ConstIterator it_s;
  for (it_s = transaction.splits().constBegin(); it_s != transaction.splits().constEnd(); ++it_s) {
    // the following line will throw an exception if the
    // account does not exist or is one of the standard accounts
    MyMoneyAccount acc = MyMoneyFile::account((*it_s).accountId());
    if (acc.id().isEmpty())
      throw MYMONEYEXCEPTION("Cannot store split with no account assigned");
    if (isStandardAccount((*it_s).accountId()))
      throw MYMONEYEXCEPTION("Cannot store split referencing standard account");
  }

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->modifySchedule(sched);

  d->addCacheNotification(sched.id());
  d->m_changeSet += MyMoneyNotification(notifyModify, sched);
}

void MyMoneyFile::removeSchedule(const MyMoneySchedule& sched)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removeSchedule(sched);

  d->addCacheNotification(sched.id(), false);
  d->m_changeSet += MyMoneyNotification(notifyRemove, sched);
}

const MyMoneySchedule MyMoneyFile::schedule(const QString& id) const
{
  return d->m_cache.schedule(id);
}

const QList<MyMoneySchedule> MyMoneyFile::scheduleList(
  const QString& accountId,
  const MyMoneySchedule::typeE type,
  const MyMoneySchedule::occurrenceE occurrence,
  const MyMoneySchedule::paymentTypeE paymentType,
  const QDate& startDate,
  const QDate& endDate,
  const bool overdue) const
{
  d->checkStorage();

  return d->m_storage->scheduleList(accountId, type, occurrence, paymentType, startDate, endDate, overdue);
}


const QStringList MyMoneyFile::consistencyCheck()
{
  QList<MyMoneyAccount> list;
  QList<MyMoneyAccount>::Iterator it_a;
  QList<MyMoneySchedule>::Iterator it_sch;
  QList<MyMoneyPayee>::Iterator it_p;
  QList<MyMoneyTransaction>::Iterator it_t;
  QList<MyMoneyReport>::Iterator it_r;
  QStringList accountRebuild;
  QStringList::ConstIterator it_c;

  QMap<QString, bool> interestAccounts;

  MyMoneyAccount parent;
  MyMoneyAccount child;
  MyMoneyAccount toplevel;

  QString parentId;
  QStringList rc;

  int problemCount = 0;
  int unfixedCount = 0;
  QString problemAccount;

  // check that we have a storage object
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // get the current list of accounts
  accountList(list);
  // add the standard accounts
  list << MyMoneyFile::instance()->asset();
  list << MyMoneyFile::instance()->liability();
  list << MyMoneyFile::instance()->income();
  list << MyMoneyFile::instance()->expense();

  for (it_a = list.begin(); it_a != list.end(); ++it_a) {
    // no more checks for standard accounts
    if (isStandardAccount((*it_a).id())) {
      continue;
    }

    switch ((*it_a).accountGroup()) {
      case MyMoneyAccount::Asset:
        toplevel = asset();
        break;
      case MyMoneyAccount::Liability:
        toplevel = liability();
        break;
      case MyMoneyAccount::Expense:
        toplevel = expense();
        break;
      case MyMoneyAccount::Income:
        toplevel = income();
        break;
      case MyMoneyAccount::Equity:
        toplevel = equity();
        break;
      default:
        qWarning("%s:%d This should never happen!", __FILE__ , __LINE__);
        break;
    }

    // check for loops in the hierarchy
    parentId = (*it_a).parentAccountId();
    try {
      bool dropOut = false;
      while (!isStandardAccount(parentId) && !dropOut) {
        parent = account(parentId);
        if (parent.id() == (*it_a).id()) {
          // parent loops, so we need to re-parent to toplevel account
          // find parent account in our list
          problemCount++;
          QList<MyMoneyAccount>::Iterator it_b;
          for (it_b = list.begin(); it_b != list.end(); ++it_b) {
            if ((*it_b).id() == parent.id()) {
              if (problemAccount != (*it_a).name()) {
                problemAccount = (*it_a).name();
                rc << i18n("* Problem with account '%1'", problemAccount);
                rc << i18n("  * Loop detected between this account and account '%1'.", (*it_b).name());
                rc << i18n("    Reparenting account '%2' to top level account '%1'.", toplevel.name(), (*it_a).name());
                (*it_a).setParentAccountId(toplevel.id());
                if (accountRebuild.contains(toplevel.id()) == 0)
                  accountRebuild << toplevel.id();
                if (accountRebuild.contains((*it_a).id()) == 0)
                  accountRebuild << (*it_a).id();
                dropOut = true;
                break;
              }
            }
          }
        }
        parentId = parent.parentAccountId();
      }

    } catch (const MyMoneyException &) {
      // if we don't know about a parent, we catch it later
    }

    // check that the parent exists
    parentId = (*it_a).parentAccountId();
    try {
      parent = account(parentId);
      if ((*it_a).accountGroup() != parent.accountGroup()) {
        problemCount++;
        if (problemAccount != (*it_a).name()) {
          problemAccount = (*it_a).name();
          rc << i18n("* Problem with account '%1'", problemAccount);
        }
        // the parent belongs to a different group, so we reconnect to the
        // master group account (asset, liability, etc) to which this account
        // should belong and update it in the engine.
        rc << i18n("  * Parent account '%1' belongs to a different group.", parent.name());
        rc << i18n("    New parent account is the top level account '%1'.", toplevel.name());
        (*it_a).setParentAccountId(toplevel.id());

        // make sure to rebuild the sub-accounts of the top account
        // and the one we removed this account from
        if (accountRebuild.contains(toplevel.id()) == 0)
          accountRebuild << toplevel.id();
        if (accountRebuild.contains(parent.id()) == 0)
          accountRebuild << parent.id();
      } else if (!parent.accountList().contains((*it_a).id())) {
        problemCount++;
        if (problemAccount != (*it_a).name()) {
          problemAccount = (*it_a).name();
          rc << i18n("* Problem with account '%1'", problemAccount);
        }
        // parent exists, but does not have a reference to the account
        rc << i18n("  * Parent account '%1' does not contain '%2' as sub-account.", parent.name(), problemAccount);
        if (accountRebuild.contains(parent.id()) == 0)
          accountRebuild << parent.id();
      }
    } catch (const MyMoneyException &) {
      // apparently, the parent does not exist anymore. we reconnect to the
      // master group account (asset, liability, etc) to which this account
      // should belong and update it in the engine.
      problemCount++;
      if (problemAccount != (*it_a).name()) {
        problemAccount = (*it_a).name();
        rc << i18n("* Problem with account '%1'", problemAccount);
      }
      rc << i18n("  * The parent with id %1 does not exist anymore.", parentId);
      rc << i18n("    New parent account is the top level account '%1'.", toplevel.name());
      (*it_a).setParentAccountId(toplevel.id());

      d->addCacheNotification((*it_a).id());

      // make sure to rebuild the sub-accounts of the top account
      if (accountRebuild.contains(toplevel.id()) == 0)
        accountRebuild << toplevel.id();
    }

    // now check that all the children exist and have the correct type
    for (it_c = (*it_a).accountList().begin(); it_c != (*it_a).accountList().end(); ++it_c) {
      // check that the child exists
      try {
        child = account(*it_c);
        if (child.parentAccountId() != (*it_a).id()) {
          throw MYMONEYEXCEPTION("Child account has a different parent");
        }
      } catch (const MyMoneyException &) {
        problemCount++;
        if (problemAccount != (*it_a).name()) {
          problemAccount = (*it_a).name();
          rc << i18n("* Problem with account '%1'", problemAccount);
        }
        rc << i18n("  * Child account with id %1 does not exist anymore.", *it_c);
        rc << i18n("    The child account list will be reconstructed.");
        if (accountRebuild.contains((*it_a).id()) == 0)
          accountRebuild << (*it_a).id();
      }
    }

    // see if it is a loan account. if so, remember the assigned interest account
    if ((*it_a).isLoan()) {
      MyMoneyAccountLoan loan(*it_a);
      if (!loan.interestAccountId().isEmpty()) {
        interestAccounts[loan.interestAccountId()] = true;
      }
      try {
        payee(loan.payee());
      } catch (const MyMoneyException &) {
        problemCount++;
        if (problemAccount != (*it_a).name()) {
          problemAccount = (*it_a).name();
          rc << i18n("* Problem with account '%1'", problemAccount);
        }
        rc << i18n("  * The payee with id %1 referenced by the loan does not exist anymore.", loan.payee());
        rc << i18n("    The payee will be removed.");
        // remove the payee - the account will be modified in the engine later
        (*it_a).deletePair("payee");
      }
    }

    // check if it is a category and set the date to 1900-01-01 if different
    if ((*it_a).isIncomeExpense()) {
      if (((*it_a).openingDate().isValid() == false) || ((*it_a).openingDate() != QDate(1900, 1, 1))) {
        (*it_a).setOpeningDate(QDate(1900, 1, 1));
      }
    }

    // check for clear text online password in the online settings
    if (!(*it_a).onlineBankingSettings().value("password").isEmpty()) {
      if (problemAccount != (*it_a).name()) {
        problemAccount = (*it_a).name();
        rc << i18n("* Problem with account '%1'", problemAccount);
      }
      rc << i18n("  * Older versions of KMyMoney stored an OFX password for this account in cleartext.");
      rc << i18n("    Please open it in the account editor (Account/Edit account) once and press OK.");
      rc << i18n("    This will store the password in the KDE wallet and remove the cleartext version.");
      ++unfixedCount;
    }

    // if the account was modified, we need to update it in the engine
    if (!(d->m_storage->account((*it_a).id()) == (*it_a))) {
      try {
        d->m_storage->modifyAccount(*it_a, true);
        d->addCacheNotification((*it_a).id());
      } catch (const MyMoneyException &) {
        rc << i18n("  * Unable to update account data in engine.");
        return rc;
      }
    }
  }

  if (accountRebuild.count() != 0) {
    rc << i18n("* Reconstructing the child lists for");
  }

  // clear the affected lists
  for (it_a = list.begin(); it_a != list.end(); ++it_a) {
    if (accountRebuild.contains((*it_a).id())) {
      rc << QString("  %1").arg((*it_a).name());
      // clear the account list
      for (it_c = (*it_a).accountList().begin(); it_c != (*it_a).accountList().end();) {
        (*it_a).removeAccountId(*it_c);
        it_c = (*it_a).accountList().begin();
      }
    }
  }

  // reconstruct the lists
  for (it_a = list.begin(); it_a != list.end(); ++it_a) {
    QList<MyMoneyAccount>::Iterator it;
    parentId = (*it_a).parentAccountId();
    if (accountRebuild.contains(parentId)) {
      for (it = list.begin(); it != list.end(); ++it) {
        if ((*it).id() == parentId) {
          (*it).addAccountId((*it_a).id());
          break;
        }
      }
    }
  }

  // update the engine objects
  for (it_a = list.begin(); it_a != list.end(); ++it_a) {
    if (accountRebuild.contains((*it_a).id())) {
      try {
        d->m_storage->modifyAccount(*it_a, true);
        d->addCacheNotification((*it_a).id());
      } catch (const MyMoneyException &) {
        rc << i18n("  * Unable to update account data for account %1 in engine", (*it_a).name());
      }
    }
  }

  // For some reason, files exist with invalid ids. This has been found in the payee id
  // so we fix them here
  QList<MyMoneyPayee> pList = payeeList();
  QMap<QString, QString>payeeConversionMap;

  for (it_p = pList.begin(); it_p != pList.end(); ++it_p) {
    if ((*it_p).id().length() > 7) {
      // found one of those with an invalid ids
      // create a new one and store it in the map.
      MyMoneyPayee payee = (*it_p);
      payee.clearId();
      d->m_storage->addPayee(payee);
      payeeConversionMap[(*it_p).id()] = payee.id();
      rc << i18n("  * Payee %1 recreated with fixed id", payee.name());
      ++problemCount;
    }
  }

  // Fix the transactions
  QList<MyMoneyTransaction> tList;
  MyMoneyTransactionFilter filter;
  filter.setReportAllSplits(false);
  d->m_storage->transactionList(tList, filter);
  // Generate the list of interest accounts
  for (it_t = tList.begin(); it_t != tList.end(); ++it_t) {
    const MyMoneyTransaction& t = (*it_t);
    QList<MyMoneySplit>::const_iterator it_s;
    for (it_s = t.splits().begin(); it_s != t.splits().end(); ++it_s) {
      if ((*it_s).action() == MyMoneySplit::ActionInterest)
        interestAccounts[(*it_s).accountId()] = true;
    }
    if (t.splits().count() < 2) {
      rc << i18n("  * transaction '%1' has only %2 of 2 expected splits.", t.id(), t.splits().count());
      ++unfixedCount;
    }
  }
  QSet<MyMoneyAccount::accountTypeE> supportedAccountTypes;
  supportedAccountTypes << MyMoneyAccount::Checkings
  << MyMoneyAccount::Savings
  << MyMoneyAccount::Cash
  << MyMoneyAccount::CreditCard
  << MyMoneyAccount::Asset
  << MyMoneyAccount::Liability;
  QSet<QString> reportedUnsupportedAccounts;

  for (it_t = tList.begin(); it_t != tList.end(); ++it_t) {
    MyMoneyTransaction t = (*it_t);
    QList<MyMoneySplit> splits = t.splits();
    QList<MyMoneySplit>::const_iterator it_s;
    bool tChanged = false;
    QDate accountOpeningDate;
    QStringList accountList;
    for (it_s = splits.constBegin(); it_s != splits.constEnd(); ++it_s) {
      bool sChanged = false;
      MyMoneySplit s = (*it_s);
      if (payeeConversionMap.find((*it_s).payeeId()) != payeeConversionMap.end()) {
        s.setPayeeId(payeeConversionMap[s.payeeId()]);
        sChanged = true;
        rc << i18n("  * Payee id updated in split of transaction '%1'.", t.id());
        ++problemCount;
      }

      try {
        const MyMoneyAccount& acc = this->account(s.accountId());
        // compute the newest opening date of all accounts involved in the transaction
        // in case the newest opening date is newer than the transaction post date, do one
        // of the following:
        //
        // a) for category and stock accounts: update the opening date of the account
        // b) for account types where the user cannot modify the opening date through
        //    the UI issue a warning (for each account only once)
        // c) others will be caught later
        if (!acc.isIncomeExpense() && !acc.isInvest()) {
          if (acc.openingDate() > t.postDate()) {
            if (!accountOpeningDate.isValid() || acc.openingDate() > accountOpeningDate) {
              accountOpeningDate = acc.openingDate();
            }
            accountList << this->accountToCategory(acc.id());
            if (!supportedAccountTypes.contains(acc.accountType())
                && !reportedUnsupportedAccounts.contains(acc.id())) {
              rc << i18n("  * Opening date of Account '%1' cannot be changed to support transaction '%2' post date.",
                         this->accountToCategory(acc.id()), t.id());
              reportedUnsupportedAccounts << acc.id();
              ++unfixedCount;
            }
          }
        } else {
          if (acc.openingDate() > t.postDate()) {
            rc << i18n("  * Transaction '%1' post date '%2' is older than opening date '%4' of account '%3'.",
                       t.id(), t.postDate().toString(Qt::ISODate), this->accountToCategory(acc.id()), acc.openingDate().toString(Qt::ISODate));

            rc << i18n("    Account opening date updated.");
            MyMoneyAccount newAcc = acc;
            newAcc.setOpeningDate(t.postDate());
            this->modifyAccount(newAcc);
            ++problemCount;
          }
        }

        // make sure, that shares and value have the same number if they
        // represent the same currency.
        if (t.commodity() == acc.currencyId() && s.shares().reduce() != s.value().reduce()) {
          // use the value as master if the transaction is balanced
          if (t.splitSum().isZero()) {
            s.setShares(s.value());
            rc << i18nc("Finance", "  * shares set to value in split of transaction '%1'.", t.id());
          } else {
            s.setValue(s.shares());
            rc << i18nc("Finance", "  * value set to shares in split of transaction '%1'.", t.id());
          }
          sChanged = true;
          ++problemCount;
        }
      } catch (const MyMoneyException &) {
        rc << i18n("  * Split %2 in transaction '%1' contains a reference to invalid account %3. Please fix manually.", t.id(), (*it_s).id(), (*it_s).accountId());
        ++unfixedCount;
      }

      // make sure the interest splits are marked correct as such
      if (interestAccounts.find(s.accountId()) != interestAccounts.end()
          && s.action() != MyMoneySplit::ActionInterest) {
        s.setAction(MyMoneySplit::ActionInterest);
        sChanged = true;
        rc << i18n("  * action marked as interest in split of transaction '%1'.", t.id());
        ++problemCount;
      }

      if (sChanged) {
        tChanged = true;
        t.modifySplit(s);
      }
    }

    // make sure that the transaction's post date is valid
    if (!t.postDate().isValid()) {
      tChanged = true;
      t.setPostDate(t.entryDate().isValid() ? t.entryDate() : QDate::currentDate());
      rc << i18n("  * Transaction '%1' has an invalid post date.", t.id());
      rc << i18n("    The post date was updated to '%1'.", KGlobal::locale()->formatDate(t.postDate(), KLocale::ShortDate));
      ++problemCount;
    }
    // check if the transaction's post date is after the opening date
    // of all accounts involved in the transaction. In case it is not,
    // issue a warning with the details about the transaction incl.
    // the account names and dates involved
    if (accountOpeningDate.isValid() && t.postDate() < accountOpeningDate) {
      QDate originalPostDate = t.postDate();
#if 0
      // for now we do not activate the logic to move the post date to a later
      // point in time. This could cause some severe trouble if you have lots
      // of ancient data collected with older versions of KMyMoney that did not
      // enforce certain conditions like we do now.
      t.setPostDate(accountOpeningDate);
      tChanged = true;
      // copy the price information for investments to the new date
      QList<MyMoneySplit>::const_iterator it_t;
      for (it_t = t.splits().constBegin(); it_t != t.splits().constEnd(); ++it_t) {
        if (((*it_t).action() != "Buy") &&
            ((*it_t).action() != "Reinvest")) {
          continue;
        }
        QString id = (*it_t).accountId();
        MyMoneyAccount acc = this->account(id);
        MyMoneySecurity sec = this->security(acc.currencyId());
        MyMoneyPrice price(acc.currencyId(),
                           sec.tradingCurrency(),
                           t.postDate(),
                           (*it_t).price(), "Transaction");
        this->addPrice(price);
        break;
      }
#endif
      rc << i18n("  * Transaction '%1' has a post date '%2' before one of the referenced account's opening date.", t.id(), KGlobal::locale()->formatDate(originalPostDate, KLocale::ShortDate));
      rc << i18n("    Referenced accounts: %1", accountList.join(","));
      rc << i18n("    The post date was not updated to '%1'.", KGlobal::locale()->formatDate(accountOpeningDate, KLocale::ShortDate));
      ++unfixedCount;
    }

    if (tChanged) {
      d->m_storage->modifyTransaction(t);
    }
  }

  // Fix the schedules
  QList<MyMoneySchedule> schList = scheduleList();
  for (it_sch = schList.begin(); it_sch != schList.end(); ++it_sch) {
    MyMoneySchedule sch = (*it_sch);
    MyMoneyTransaction t = sch.transaction();
    QList<MyMoneySplit> splits = t.splits();
    bool tChanged = false;
    QList<MyMoneySplit>::const_iterator it_s;
    for (it_s = splits.constBegin(); it_s != splits.constEnd(); ++it_s) {
      MyMoneySplit s = (*it_s);
      bool sChanged = false;
      if (payeeConversionMap.find((*it_s).payeeId()) != payeeConversionMap.end()) {
        s.setPayeeId(payeeConversionMap[s.payeeId()]);
        sChanged = true;
        rc << i18n("  * Payee id updated in split of schedule '%1'.", (*it_sch).name());
        ++problemCount;
      }
      if (!(*it_s).value().isZero() && (*it_s).shares().isZero()) {
        s.setShares(s.value());
        sChanged = true;
        rc << i18nc("Finance", "  * Split in scheduled transaction '%1' contained value != 0 and shares == 0.", (*it_sch).name());
        rc << i18nc("Finance", "    Shares set to value.");
        ++problemCount;
      }

      // make sure, we don't have a bankid stored with a split in a schedule
      if (!(*it_s).bankID().isEmpty()) {
        s.setBankID(QString());
        sChanged = true;
        rc << i18n("  * Removed bankid from split in scheduled transaction '%1'.", (*it_sch).name());
        ++problemCount;
      }

      // make sure, that shares and value have the same number if they
      // represent the same currency.
      try {
        const MyMoneyAccount& acc = this->account(s.accountId());
        if (t.commodity() == acc.currencyId()
            && s.shares().reduce() != s.value().reduce()) {
          // use the value as master if the transaction is balanced
          if (t.splitSum().isZero()) {
            s.setShares(s.value());
            rc << i18nc("Finance", "  * shares set to value in split in schedule '%1'.", (*it_sch).name());
          } else {
            s.setValue(s.shares());
            rc << i18nc("Finance", "  * value set to shares in split in schedule '%1'.", (*it_sch).name());
          }
          sChanged = true;
          ++problemCount;
        }
      } catch (const MyMoneyException &) {
        rc << i18n("  * Split %2 in schedule '%1' contains a reference to invalid account %3. Please fix manually.", (*it_sch).name(), (*it_s).id(), (*it_s).accountId());
        ++unfixedCount;
      }
      if (sChanged) {
        t.modifySplit(s);
        tChanged = true;
      }
    }
    if (tChanged) {
      sch.setTransaction(t);
      d->m_storage->modifySchedule(sch);
    }
  }

  // Fix the reports
  QList<MyMoneyReport> rList = reportList();
  for (it_r = rList.begin(); it_r != rList.end(); ++it_r) {
    MyMoneyReport r = *it_r;
    QStringList pList;
    QStringList::Iterator it_p;
    (*it_r).payees(pList);
    bool rChanged = false;
    for (it_p = pList.begin(); it_p != pList.end(); ++it_p) {
      if (payeeConversionMap.find(*it_p) != payeeConversionMap.end()) {
        rc << i18n("  * Payee id updated in report '%1'.", (*it_r).name());
        ++problemCount;
        r.removeReference(*it_p);
        r.addPayee(payeeConversionMap[*it_p]);
        rChanged = true;
      }
    }
    if (rChanged) {
      d->m_storage->modifyReport(r);
    }
  }

  // erase old payee ids
  QMap<QString, QString>::Iterator it_m;
  for (it_m = payeeConversionMap.begin(); it_m != payeeConversionMap.end(); ++it_m) {
    MyMoneyPayee payee = this->payee(it_m.key());
    removePayee(payee);
    rc << i18n("  * Payee '%1' removed.", payee.id());
    ++problemCount;
  }

  //look for accounts which have currencies other than the base currency but no price on the opening date
  //all accounts using base currency are excluded, since that's the base used for foreing currency calculation
  //thus it is considered as always present
  //accounts that represent Income/Expense categories are also excluded as price is irrelevant for their
  //fake opening date since a forex rate is required for all multi-currency transactions

  //get all currencies in use
  QStringList currencyList;
  QList<MyMoneyAccount> accountForeignCurrency;
  QList<MyMoneyAccount> accList;
  accountList(accList);
  QList<MyMoneyAccount>::const_iterator account_it;
  for (account_it = accList.constBegin(); account_it != accList.constEnd(); ++account_it) {
    MyMoneyAccount account = *account_it;
    if (!account.isIncomeExpense()
        && !currencyList.contains(account.currencyId())
        && account.currencyId() != baseCurrency().id()
        && !account.currencyId().isEmpty()) {
      //add the currency and the account-currency pair
      currencyList.append(account.currencyId());
      accountForeignCurrency.append(account);
    }
  }

  MyMoneyPriceList pricesList = priceList();
  QMap<MyMoneySecurityPair, QDate> securityPriceDate;

  //get the first date of the price for each security
  MyMoneyPriceList::const_iterator prices_it;
  for (prices_it = pricesList.constBegin(); prices_it != pricesList.constEnd(); ++prices_it) {
    MyMoneyPrice firstPrice = (*((*prices_it).constBegin()));

    //only check the price if the currency is in use
    if (currencyList.contains(firstPrice.from()) || currencyList.contains(firstPrice.to())) {
      //check the security in the from field
      //if it is there, check if it is older
      QPair<QString, QString> pricePair = qMakePair(firstPrice.from(), firstPrice.to());
      securityPriceDate[pricePair] = firstPrice.date();
    }
  }

  //compare the dates with the opening dates of the accounts using each currency
  QList<MyMoneyAccount>::const_iterator accForeignList_it;
  bool firstInvProblem = true;
  for (accForeignList_it = accountForeignCurrency.constBegin(); accForeignList_it != accountForeignCurrency.constEnd(); ++accForeignList_it) {
    //setup the price pair correctly
    QPair<QString, QString> pricePair;
    //setup the reverse, which can also be used for rate conversion
    QPair<QString, QString> reversePricePair;
    if ((*accForeignList_it).isInvest()) {
      //if it is a stock, we have to search for a price from its stock to the currency of the account
      QString securityId = (*accForeignList_it).currencyId();
      QString tradingCurrencyId = security(securityId).tradingCurrency();
      pricePair = qMakePair(securityId, tradingCurrencyId);
      reversePricePair = qMakePair(tradingCurrencyId, securityId);
    } else {
      //if it is a regular account we search for a price from the currency of the account to the base currency
      QString currency = (*accForeignList_it).currencyId();
      QString baseCurrencyId = baseCurrency().id();
      pricePair = qMakePair(currency, baseCurrencyId);
      reversePricePair = qMakePair(baseCurrencyId, currency);
    }

    //compare the first price with the opening date of the account
    if ((!securityPriceDate.contains(pricePair) || securityPriceDate.value(pricePair) > (*accForeignList_it).openingDate())
        && (!securityPriceDate.contains(reversePricePair) || securityPriceDate.value(reversePricePair) > (*accForeignList_it).openingDate())) {
      if (firstInvProblem) {
        firstInvProblem = false;
        rc << i18n("* Potential problem with investments/currencies");
      }
      QDate openingDate = (*accForeignList_it).openingDate();
      MyMoneySecurity secError = security((*accForeignList_it).currencyId());
      if (!(*accForeignList_it).isInvest()) {
        rc << i18n("  * The account '%1' in currency '%2' has no price set for the opening date '%3'.", (*accForeignList_it).name(), secError.name(), openingDate.toString(Qt::ISODate));
        rc << i18n("    Please enter a price for the currency on or before the opening date.");
      } else {
        rc << i18n("  * The investment '%1' has no price set for the opening date '%2'.", (*accForeignList_it).name(), openingDate.toString(Qt::ISODate));
        rc << i18n("    Please enter a price for the investment on or before the opening date.");
      }
      ++unfixedCount;
    }
  }

  // Fix the budgets that somehow still reference invalid accounts
  QString problemBudget;
  QList<MyMoneyBudget> bList = budgetList();
  for (QList<MyMoneyBudget>::const_iterator it_b = bList.constBegin(); it_b != bList.constEnd(); ++it_b) {
    MyMoneyBudget b = *it_b;
    QList<MyMoneyBudget::AccountGroup> baccounts = b.getaccounts();
    bool bChanged = false;
    for (QList<MyMoneyBudget::AccountGroup>::const_iterator it_bacc = baccounts.constBegin(); it_bacc != baccounts.constEnd(); ++it_bacc) {
      try {
        account((*it_bacc).id());
      } catch (const MyMoneyException &) {
        problemCount++;
        if (problemBudget != b.name()) {
          problemBudget = b.name();
          rc << i18n("* Problem with budget '%1'", problemBudget);
        }
        rc << i18n("  * The account with id %1 referenced by the budget does not exist anymore.", (*it_bacc).id());
        rc << i18n("    The account reference will be removed.");
        // remove the reference to the account
        b.removeReference((*it_bacc).id());
        bChanged = true;
      }
    }
    if (bChanged) {
      d->m_storage->modifyBudget(b);
    }
  }

  // add more checks here

  if (problemCount == 0 && unfixedCount == 0) {
    rc << i18n("Finished: data is consistent.");
  } else {
    const QString problemsCorrected = i18np("%1 problem corrected.", "%1 problems corrected.", problemCount);
    const QString problemsRemaining = i18np("%1 problem still present.", "%1 problems still present.", unfixedCount);

    rc << QString();
    rc << i18nc("%1 is a string, e.g. 7 problems corrected; %2 is a string, e.g. 3 problems still present", "Finished: %1 %2", problemsCorrected, problemsRemaining);
  }
  return rc;
}

QString MyMoneyFile::createCategory(const MyMoneyAccount& base, const QString& name)
{
  d->checkTransaction(Q_FUNC_INFO);

  MyMoneyAccount parent = base;
  QString categoryText;

  if (base.id() != expense().id() && base.id() != income().id())
    throw MYMONEYEXCEPTION("Invalid base category");

  QStringList subAccounts = name.split(AccountSeparator);
  QStringList::Iterator it;
  for (it = subAccounts.begin(); it != subAccounts.end(); ++it) {
    MyMoneyAccount categoryAccount;

    categoryAccount.setName(*it);
    categoryAccount.setAccountType(base.accountType());

    if (it == subAccounts.begin())
      categoryText += *it;
    else
      categoryText += (AccountSeparator + *it);

    // Only create the account if it doesn't exist
    try {
      QString categoryId = categoryToAccount(categoryText);
      if (categoryId.isEmpty())
        addAccount(categoryAccount, parent);
      else {
        categoryAccount = account(categoryId);
      }
    } catch (const MyMoneyException &e) {
      qDebug("Unable to add account %s, %s, %s: %s",
             qPrintable(categoryAccount.name()),
             qPrintable(parent.name()),
             qPrintable(categoryText),
             qPrintable(e.what()));
    }

    parent = categoryAccount;
  }

  return categoryToAccount(name);
}

const QList<MyMoneySchedule> MyMoneyFile::scheduleListEx(int scheduleTypes,
    int scheduleOcurrences,
    int schedulePaymentTypes,
    QDate startDate,
    const QStringList& accounts) const
{
  d->checkStorage();

  return d->m_storage->scheduleListEx(scheduleTypes, scheduleOcurrences, schedulePaymentTypes, startDate, accounts);
}

void MyMoneyFile::addSecurity(MyMoneySecurity& security)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addSecurity(security);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadSecurity(security);

  d->m_changeSet += MyMoneyNotification(notifyAdd, security);
}

void MyMoneyFile::modifySecurity(const MyMoneySecurity& security)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->modifySecurity(security);

  d->addCacheNotification(security.id());

  d->m_changeSet += MyMoneyNotification(notifyModify, security);
}

void MyMoneyFile::removeSecurity(const MyMoneySecurity& security)
{
  d->checkTransaction(Q_FUNC_INFO);

  // FIXME check that security is not referenced by other object

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removeSecurity(security);

  d->addCacheNotification(security.id(), false);

  d->m_changeSet += MyMoneyNotification(notifyRemove, security);
}

const MyMoneySecurity& MyMoneyFile::security(const QString& id) const
{
  if (id.isEmpty())
    return baseCurrency();

  return d->m_cache.security(id);
}

const QList<MyMoneySecurity> MyMoneyFile::securityList() const
{
  d->checkStorage();

  return d->m_storage->securityList();
}

void MyMoneyFile::addCurrency(const MyMoneySecurity& currency)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addCurrency(currency);

  // The notifier mechanism only refreshes the cache but does not
  // load new objects. So we simply force loading of the new one here
  d->m_cache.preloadSecurity(currency);

  d->m_changeSet += MyMoneyNotification(notifyAdd, currency);
}

void MyMoneyFile::modifyCurrency(const MyMoneySecurity& currency)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // force reload of base currency object
  if (currency.id() == d->m_baseCurrency.id())
    d->m_baseCurrency.clearId();

  d->m_storage->modifyCurrency(currency);

  d->addCacheNotification(currency.id());

  d->m_changeSet += MyMoneyNotification(notifyModify, currency);
}

void MyMoneyFile::removeCurrency(const MyMoneySecurity& currency)
{
  d->checkTransaction(Q_FUNC_INFO);

  if (currency.id() == d->m_baseCurrency.id()) {
    throw MYMONEYEXCEPTION("Cannot delete base currency.");
  }

  // FIXME check that security is not referenced by other object

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removeCurrency(currency);

  d->addCacheNotification(currency.id(), false);

  d->m_changeSet += MyMoneyNotification(notifyRemove, currency);
}

const MyMoneySecurity& MyMoneyFile::currency(const QString& id) const
{
  if (id.isEmpty())
    return baseCurrency();

  const MyMoneySecurity& curr = d->m_cache.security(id);
  if (curr.id().isEmpty()) {
    QString msg;
    msg = QString("Currency '%1' not found.").arg(id);
    throw MYMONEYEXCEPTION(msg);
  }
  return curr;
}

const QList<MyMoneySecurity> MyMoneyFile::currencyList() const
{
  d->checkStorage();

  return d->m_storage->currencyList();
}

const QString& MyMoneyFile::foreignCurrency(const QString& first, const QString& second) const
{
  if (baseCurrency().id() == second)
    return first;
  return second;
}

const MyMoneySecurity& MyMoneyFile::baseCurrency() const
{
  if (d->m_baseCurrency.id().isEmpty()) {
    QString id = QString(value("kmm-baseCurrency"));
    if (!id.isEmpty())
      d->m_baseCurrency = currency(id);
  }

  return d->m_baseCurrency;
}

void MyMoneyFile::setBaseCurrency(const MyMoneySecurity& curr)
{
  // make sure the currency exists
  MyMoneySecurity c = currency(curr.id());

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  if (c.id() != d->m_baseCurrency.id()) {
    setValue("kmm-baseCurrency", curr.id());
    // force reload of base currency cache
    d->m_baseCurrency = MyMoneySecurity();
  }
}

void MyMoneyFile::addPrice(const MyMoneyPrice& price)
{
  if (price.rate(QString()).isZero())
    return;

  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // store the account's which are affected by this price regarding their value
  d->priceChanged(*this, price);

  d->m_storage->addPrice(price);
}

void MyMoneyFile::removePrice(const MyMoneyPrice& price)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  // store the account's which are affected by this price regarding their value
  d->priceChanged(*this, price);

  d->m_storage->removePrice(price);
}

MyMoneyPrice MyMoneyFile::price(const QString& fromId, const QString& toId, const QDate& date, const bool exactDate) const
{
  d->checkStorage();

  QString to(toId);
  if (to.isEmpty())
    to = value("kmm-baseCurrency");
  // if some id is missing, we can return an empty price object
  if (fromId.isEmpty() || to.isEmpty())
    return MyMoneyPrice();

  // we don't search our tables if someone asks stupid stuff
  if (fromId == toId) {
    return MyMoneyPrice(fromId, toId, date, MyMoneyMoney::ONE, "KMyMoney");
  }

  // if not asking for exact date, try to find the exact date match first,
  // either the requested price or its reciprocal value. If unsuccessful, it will move
  // on and look for prices of previous dates
  MyMoneyPrice rc = d->m_storage->price(fromId, to, date, true);
  if (!rc.isValid()) {
    // not found, search 'to-from' rate and use reciprocal value
    rc = d->m_storage->price(to, fromId, date, true);

    // not found, search previous dates, if exact date is not needed
    if (!exactDate && !rc.isValid()) {
      // search 'from-to' and 'to-from', select the most recent one
      MyMoneyPrice fromPrice = d->m_storage->price(fromId, to, date, exactDate);
      MyMoneyPrice toPrice = d->m_storage->price(to, fromId, date, exactDate);

      // check first whether both prices are valid
      if (fromPrice.isValid() && toPrice.isValid()) {
        if (fromPrice.date() >= toPrice.date()) {
          // if 'from-to' is newer or the same date, prefer that one
          rc = fromPrice;
        } else {
          // otherwise, use the reciprocal price
          rc = toPrice;
        }
      } else if (fromPrice.isValid()) { // check if any of the prices is valid, return that one
        rc = fromPrice;
      } else if (toPrice.isValid()) {
        rc = toPrice;
      }
    }
  }
  return rc;
}

const MyMoneyPriceList MyMoneyFile::priceList() const
{
  d->checkStorage();

  return d->m_storage->priceList();
}

bool MyMoneyFile::hasAccount(const QString& id, const QString& name) const
{
  MyMoneyAccount acc = d->m_cache.account(id);
  QStringList list = acc.accountList();
  QStringList::ConstIterator it;
  bool rc = false;
  for (it = list.constBegin(); rc == false && it != list.constEnd(); ++it) {
    MyMoneyAccount a = d->m_cache.account(*it);
    if (a.name() == name)
      rc = true;
  }
  return rc;
}

const QList<MyMoneyReport> MyMoneyFile::reportList() const
{
  d->checkStorage();

  return d->m_storage->reportList();
}

void MyMoneyFile::addReport(MyMoneyReport& report)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addReport(report);
}

void MyMoneyFile::modifyReport(const MyMoneyReport& report)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->modifyReport(report);

  d->addCacheNotification(report.id());
}

unsigned MyMoneyFile::countReports() const
{
  d->checkStorage();

  return d->m_storage->countReports();
}

const MyMoneyReport MyMoneyFile::report(const QString& id) const
{
  d->checkStorage();

  return d->m_storage->report(id);
}

void MyMoneyFile::removeReport(const MyMoneyReport& report)
{
  d->checkTransaction(Q_FUNC_INFO);
  MyMoneyNotifier notifier(d);

  d->m_storage->removeReport(report);

  d->addCacheNotification(report.id(), false);
}


const QList<MyMoneyBudget> MyMoneyFile::budgetList() const
{
  d->checkStorage();

  return d->m_storage->budgetList();
}

void MyMoneyFile::addBudget(MyMoneyBudget& budget)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->addBudget(budget);
}

const MyMoneyBudget MyMoneyFile::budgetByName(const QString& name) const
{
  d->checkStorage();

  return d->m_storage->budgetByName(name);
}

void MyMoneyFile::modifyBudget(const MyMoneyBudget& budget)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->modifyBudget(budget);

  d->addCacheNotification(budget.id());
}

unsigned MyMoneyFile::countBudgets() const
{
  d->checkStorage();

  return d->m_storage->countBudgets();
}

const MyMoneyBudget MyMoneyFile::budget(const QString& id) const
{
  d->checkStorage();

  return d->m_storage->budget(id);
}

void MyMoneyFile::removeBudget(const MyMoneyBudget& budget)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);

  d->m_storage->removeBudget(budget);

  d->addCacheNotification(budget.id(), false);
}

void MyMoneyFile::addOnlineJob(onlineJob& job)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);
  d->m_storage->addOnlineJob(job);
  d->m_cache.preloadOnlineJob(job);
  d->m_changeSet += MyMoneyNotification(notifyAdd, job);
}

void MyMoneyFile::modifyOnlineJob(const onlineJob job)
{
  d->checkTransaction(Q_FUNC_INFO);
  d->m_storage->modifyOnlineJob(job);
  d->m_changeSet += MyMoneyNotification(notifyModify, job);
  d->addCacheNotification(job.id());
}

const onlineJob MyMoneyFile::getOnlineJob(const QString &jobId) const
{
  d->checkStorage();
  return d->m_storage->getOnlineJob(jobId);
}

const QList<onlineJob> MyMoneyFile::onlineJobList() const
{
  d->checkStorage();
  return d->m_storage->onlineJobList();
}

/** @todo improve speed by passing count job to m_storage */
int MyMoneyFile::countOnlineJobs() const
{
  return onlineJobList().count();
}

/**
 * @brief Remove onlineJob
 * @param job onlineJob to remove
 */
void MyMoneyFile::removeOnlineJob(const onlineJob& job)
{
  d->checkTransaction(Q_FUNC_INFO);

  // clear all changed objects from cache
  MyMoneyNotifier notifier(d);
  if (job.isLocked()) {
    return;
  }
  d->addCacheNotification(job.id(), false);
  d->m_changeSet += MyMoneyNotification(notifyRemove, job);
  d->m_storage->removeOnlineJob(job);
}

void MyMoneyFile::removeOnlineJob(const QStringList onlineJobIds)
{
  foreach (QString jobId, onlineJobIds) {
    removeOnlineJob(getOnlineJob(jobId));
  }
}

bool MyMoneyFile::addVATSplit(MyMoneyTransaction& transaction, const MyMoneyAccount& account, const MyMoneyAccount& category, const MyMoneyMoney& amount)
{
  bool rc = false;

  try {
    MyMoneySplit cat;  // category
    MyMoneySplit tax;  // tax

    if (category.value("VatAccount").isEmpty())
      return false;
    MyMoneyAccount vatAcc = this->account(category.value("VatAccount").toLatin1());
    const MyMoneySecurity& asec = security(account.currencyId());
    const MyMoneySecurity& csec = security(category.currencyId());
    const MyMoneySecurity& vsec = security(vatAcc.currencyId());
    if (asec.id() != csec.id() || asec.id() != vsec.id()) {
      qDebug("Auto VAT assignment only works if all three accounts use the same currency.");
      return false;
    }

    MyMoneyMoney vatRate(vatAcc.value("VatRate"));
    MyMoneyMoney gv, nv;    // gross value, net value
    int fract = account.fraction();

    if (!vatRate.isZero()) {

      tax.setAccountId(vatAcc.id());

      // qDebug("vat amount is '%s'", category.value("VatAmount").toLatin1());
      if (category.value("VatAmount").toLower() != QString("net")) {
        // split value is the gross value
        gv = amount;
        nv = (gv / (MyMoneyMoney::ONE + vatRate)).convert(fract);
        MyMoneySplit catSplit = transaction.splitByAccount(account.id(), false);
        catSplit.setShares(-nv);
        catSplit.setValue(catSplit.shares());
        transaction.modifySplit(catSplit);

      } else {
        // split value is the net value
        nv = amount;
        gv = (nv * (MyMoneyMoney::ONE + vatRate)).convert(fract);
        MyMoneySplit accSplit = transaction.splitByAccount(account.id());
        accSplit.setValue(gv.convert(fract));
        accSplit.setShares(accSplit.value());
        transaction.modifySplit(accSplit);
      }

      tax.setValue(-(gv - nv).convert(fract));
      tax.setShares(tax.value());
      transaction.addSplit(tax);
      rc = true;
    }
  } catch (const MyMoneyException &) {
  }
  return rc;
}

bool MyMoneyFile::isReferenced(const MyMoneyObject& obj, const MyMoneyFileBitArray& skipChecks) const
{
  d->checkStorage();
  return d->m_storage->isReferenced(obj, skipChecks);
}

bool MyMoneyFile::checkNoUsed(const QString& accId, const QString& no) const
{
  // by definition, an empty string or a non-numeric string is not used
  QRegExp exp(QString("(.*\\D)?(\\d+)(\\D.*)?"));
  if (no.isEmpty() || exp.indexIn(no) == -1)
    return false;

  MyMoneyTransactionFilter filter;
  filter.addAccount(accId);
  QList<MyMoneyTransaction> transactions = transactionList(filter);
  QList<MyMoneyTransaction>::ConstIterator it_t = transactions.constBegin();
  while (it_t != transactions.constEnd()) {
    try {
      MyMoneySplit split;
      // Test whether the transaction also includes a split into
      // this account
      split = (*it_t).splitByAccount(accId, true /*match*/);
      if (!split.number().isEmpty() && split.number() == no)
        return true;
    } catch (const MyMoneyException &) {
    }
    ++it_t;
  }
  return false;
}

QString MyMoneyFile::highestCheckNo(const QString& accId) const
{
  unsigned64 lno = 0;
  unsigned64 cno;
  QString no;
  MyMoneyTransactionFilter filter;
  filter.addAccount(accId);
  QList<MyMoneyTransaction> transactions = transactionList(filter);
  QList<MyMoneyTransaction>::ConstIterator it_t = transactions.constBegin();
  while (it_t != transactions.constEnd()) {
    try {
      // Test whether the transaction also includes a split into
      // this account
      MyMoneySplit split = (*it_t).splitByAccount(accId, true /*match*/);
      if (!split.number().isEmpty()) {
        // non-numerical values stored in number will return 0 in the next line
        cno = split.number().toULongLong();
        if (cno > lno) {
          lno = cno;
          no = split.number();
        }
      }
    } catch (const MyMoneyException &) {
    }
    ++it_t;
  }
  return no;
}

bool MyMoneyFile::hasNewerTransaction(const QString& accId, const QDate& date) const
{
  MyMoneyTransactionFilter filter;
  filter.addAccount(accId);
  filter.setDateFilter(date.addDays(+1), QDate());
  QList<MyMoneyTransaction> transactions = transactionList(filter);
  return transactions.count() > 0;
}

void MyMoneyFile::clearCache()
{
  d->checkStorage();
  d->m_cache.clear();
  d->m_balanceCache.clear();
}

void MyMoneyFile::preloadCache()
{
  d->checkStorage();

  d->m_cache.clear();
  QList<MyMoneyAccount> a_list;
  d->m_storage->accountList(a_list);
  d->m_cache.preloadAccount(a_list);
  d->m_cache.preloadPayee(d->m_storage->payeeList());
  d->m_cache.preloadTag(d->m_storage->tagList());
  d->m_cache.preloadInstitution(d->m_storage->institutionList());
  d->m_cache.preloadSecurity(d->m_storage->securityList() +
                             d->m_storage->currencyList());
  d->m_cache.preloadSchedule(d->m_storage->scheduleList());
  d->m_cache.preloadOnlineJob(d->m_storage->onlineJobList());
}

bool MyMoneyFile::isTransfer(const MyMoneyTransaction& t) const
{
  bool rc = false;
  if (t.splitCount() == 2) {
    QList<MyMoneySplit>::const_iterator it_s;
    for (it_s = t.splits().begin(); it_s != t.splits().end(); ++it_s) {
      MyMoneyAccount acc = account((*it_s).accountId());
      if (acc.isIncomeExpense())
        break;
    }
    if (it_s == t.splits().end())
      rc = true;
  }
  return rc;
}

bool MyMoneyFile::referencesClosedAccount(const MyMoneyTransaction& t) const
{
  QList<MyMoneySplit>::const_iterator it_s;
  const QList<MyMoneySplit>& list = t.splits();
  for (it_s = list.begin(); it_s != list.end(); ++it_s) {
    if (referencesClosedAccount(*it_s))
      break;
  }
  return it_s != list.end();
}

bool MyMoneyFile::referencesClosedAccount(const MyMoneySplit& s) const
{
  if (s.accountId().isEmpty())
    return false;

  try {
    return account(s.accountId()).isClosed();
  } catch (const MyMoneyException &) {
  }
  return false;
}

QString MyMoneyFile::storageId()
{
  QString id = value("kmm-id");
  if (id.isEmpty()) {
    MyMoneyFileTransaction ft;
    try {
      QUuid uid = QUuid::createUuid();
      setValue("kmm-id", uid.toString());
      ft.commit();
      id = uid.toString();
    } catch (const MyMoneyException &) {
      qDebug("Unable to setup UID for new storage object");
    }
  }
  return id;
}

const QString MyMoneyFile::openingBalancesPrefix()
{
    return i18n("Opening Balances");
}

bool MyMoneyFile::hasMatchingOnlineBalance(const MyMoneyAccount& _acc) const
{
  // get current values
  MyMoneyAccount acc = account(_acc.id());

  // if there's no last transaction import data we are done
  if (acc.value("lastImportedTransactionDate").isEmpty()
      || acc.value("lastStatementBalance").isEmpty())
    return false;

  // otherwise, we compare the balances
  MyMoneyMoney balance(acc.value("lastStatementBalance"));
  MyMoneyMoney accBalance = this->balance(acc.id(), QDate::fromString(acc.value("lastImportedTransactionDate"), Qt::ISODate));

  return balance == accBalance;
}

int MyMoneyFile::countTransactionsWithSpecificReconciliationState(const QString& accId, enum MyMoneyTransactionFilter::stateOptionE state) const
{
  MyMoneyTransactionFilter filter;
  filter.addAccount(accId);
  filter.addState(state);
  return transactionList(filter).count();
}

  /**
   * Make sure that the splits value has the precision of the corresponding account
   */
void MyMoneyFile::fixSplitPrecision(MyMoneyTransaction& t) const
{
  QList<MyMoneySplit>::iterator its;
  MyMoneySecurity transactionSecurity = security(t.commodity());
  int transactionFraction = transactionSecurity.smallestAccountFraction();

  for(its = t.splits().begin(); its != t.splits().end(); ++its) {
    MyMoneyAccount acc = account((*its).accountId());
    int fraction = acc.fraction();
    if(fraction == -1) {
      MyMoneySecurity sec = security(acc.currencyId());
      fraction = acc.fraction(sec);
    }
    (*its).setShares((*its).shares().convertDenominator(fraction).canonicalize());
    (*its).setValue((*its).value().convertDenominator(transactionFraction).canonicalize());
  }
}


MyMoneyFileTransaction::MyMoneyFileTransaction() :
    m_isNested(MyMoneyFile::instance()->hasTransaction()),
    m_needRollback(!m_isNested)
{
  if (!m_isNested)
    MyMoneyFile::instance()->startTransaction();
}

MyMoneyFileTransaction::~MyMoneyFileTransaction()
{
  rollback();
}

void MyMoneyFileTransaction::restart()
{
  rollback();

  m_needRollback = !m_isNested;
  if (!m_isNested)
    MyMoneyFile::instance()->startTransaction();
}

void MyMoneyFileTransaction::commit()
{
  if (!m_isNested)
    MyMoneyFile::instance()->commitTransaction();
  m_needRollback = false;
}

void MyMoneyFileTransaction::rollback()
{
  if (m_needRollback)
    MyMoneyFile::instance()->rollbackTransaction();
  m_needRollback = false;
}
