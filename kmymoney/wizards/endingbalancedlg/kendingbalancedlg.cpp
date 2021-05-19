/***************************************************************************
                          kendingbalancedlg.cpp
                             -------------------
    copyright            : (C) 2000,2003 by Michael Edwardes, Thomas Baumgart
    email                : mte@users.sourceforge.net
                           ipwizard@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kendingbalancedlg.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QPixmap>
#include <QList>
#include <QBitArray>

// ----------------------------------------------------------------------------
// KDE Includes

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kstandardguiitem.h>

// ----------------------------------------------------------------------------
// Project Includes

#include "kmymoneyedit.h"
#include "mymoneysplit.h"
#include "mymoneyfile.h"
#include "kmymoneyglobalsettings.h"
#include "kmymoneycategory.h"
#include "kmymoneyaccountselector.h"
#include "ktoolinvocation.h"
#include "kmymoneyutils.h"
#include "kcurrencycalculator.h"

class KEndingBalanceDlg::Private
{
public:

  explicit Private(int numPages)
      : m_pages(numPages, true) {}

  MyMoneyTransaction        m_tInterest;
  MyMoneyTransaction        m_tCharges;
  MyMoneyAccount            m_account;
  QMap<QWidget*, QString>   m_helpAnchor;
  QBitArray m_pages;
};

KEndingBalanceDlg::KEndingBalanceDlg(const MyMoneyAccount& account, QWidget *parent) :
    KEndingBalanceDlgDecl(parent),
    d(new Private(Page_InterestChargeCheckings + 1))
{
  setModal(true);
  QString value;
  MyMoneyMoney endBalance, startBalance;

  d->m_account = account;

  MyMoneySecurity currency = MyMoneyFile::instance()->security(account.currencyId());
  //FIXME: port
  m_statementInfoPageCheckings->m_enterInformationLabel->setText(QString("<qt>") + i18n("Please enter the following fields with the information as you find them on your statement. Make sure to enter all values in <b>%1</b>.", currency.name()) + QString("</qt>"));

  // If the previous reconciliation was postponed,
  // we show a different first page
  value = account.value("lastReconciledBalance");
  if (value.isEmpty()) {
    // if the last statement has been entered long enough ago (more than one month),
    // then take the last statement date and add one month and use that as statement
    // date.
    MyMoneyDate lastStatementDate = account.lastReconciliationDate();
    if (lastStatementDate.addMonths(1) < MyMoneyDate::currentDate()) {
      setField("statementDate", lastStatementDate.addMonths(1));
    }

    slotUpdateBalances();

    d->m_pages.clearBit(Page_PreviousPostpone);
  } else {
    d->m_pages.clearBit(Page_CheckingStart);
    d->m_pages.clearBit(Page_InterestChargeCheckings);
    //removePage(m_interestChargeCheckings);
    // make sure, we show the correct start page
    setStartId(Page_PreviousPostpone);

    MyMoneyMoney factor(1, 1);
    if (d->m_account.accountGroup() == MyMoneyAccount::Liability)
      factor = -factor;

    startBalance = MyMoneyMoney(value) * factor;
    value = account.value("statementBalance");
    endBalance = MyMoneyMoney(value) * factor;

    //FIXME: port
    m_statementInfoPageCheckings->m_previousBalance->setValue(startBalance);
    m_statementInfoPageCheckings->m_endingBalance->setValue(endBalance);
  }

  // We don't need to add the default into the list (see ::help() why)
  // m_helpAnchor[m_startPageCheckings] = QString("");
  d->m_helpAnchor[m_interestChargeCheckings] = QString("details.reconcile.wizard.interest");
  d->m_helpAnchor[m_statementInfoPageCheckings] = QString("details.reconcile.wizard.statement");

  value = account.value("statementDate");
  if (!value.isEmpty())
    setField("statementDate", MyMoneyDate::fromString(value, Qt::ISODate));

  //FIXME: port
  m_statementInfoPageCheckings->m_lastStatementDate->setText(QString());
  if (account.lastReconciliationDate().isValid()) {
    m_statementInfoPageCheckings->m_lastStatementDate->setText(i18n("Last reconciled statement: %1", MyMoneyLocale::formatDate(account.lastReconciliationDate())));
  }

  // connect the signals with the slots
  connect(MyMoneyFile::instance(), SIGNAL(dataChanged()), this, SLOT(slotReloadEditWidgets()));
  connect(m_statementInfoPageCheckings->m_statementDate, SIGNAL(dateChanged(MyMoneyDate)), this, SLOT(slotUpdateBalances()));
  connect(m_interestChargeCheckings->m_interestCategoryEdit, SIGNAL(createItem(QString,QString&)), this, SLOT(slotCreateInterestCategory(QString,QString&)));
  connect(m_interestChargeCheckings->m_chargesCategoryEdit, SIGNAL(createItem(QString,QString&)), this, SLOT(slotCreateChargesCategory(QString,QString&)));
  connect(m_interestChargeCheckings->m_payeeEdit, SIGNAL(createItem(QString,QString&)), this, SIGNAL(createPayee(QString,QString&)));

  KMyMoneyMVCCombo::setSubstringSearchForChildren(m_interestChargeCheckings, !KMyMoneySettings::stringMatchFromStart());

  slotReloadEditWidgets();

  // preset payee if possible
  try {
    // if we find a payee with the same name as the institution,
    // than this is what we use as payee.
    if (!d->m_account.institutionId().isEmpty()) {
      MyMoneyInstitution inst = MyMoneyFile::instance()->institution(d->m_account.institutionId());
      MyMoneyPayee payee = MyMoneyFile::instance()->payeeByName(inst.name());
      setField("payeeEdit", payee.id());
    }
  } catch (const MyMoneyException &) {
  }

  KMyMoneyUtils::updateWizardButtons(this);

  // setup different text and icon on finish button
  setButtonText(QWizard::FinishButton, KStandardGuiItem::cont().text());
  button(QWizard::FinishButton)->setIcon(KStandardGuiItem::cont().icon());
}

KEndingBalanceDlg::~KEndingBalanceDlg()
{
  delete d;
}

void KEndingBalanceDlg::slotUpdateBalances()
{
  MYMONEYTRACER(tracer);

  // determine the beginning balance and ending balance based on the following
  // forumulas:
  //
  // end balance   = current balance - sum(all non cleared transactions)
  //                                 - sum(all cleared transactions posted
  //                                        after statement date)
  // start balance = end balance - sum(all cleared transactions
  //                                        up to statement date)
  MyMoneyTransactionFilter filter(d->m_account.id());
  filter.addState(MyMoneyTransactionFilter::notReconciled);
  filter.setReportAllSplits(true);

  QList<QPair<MyMoneyTransaction, MyMoneySplit> > transactionList;
  QList<QPair<MyMoneyTransaction, MyMoneySplit> >::const_iterator it;

  // retrieve the list from the engine
  MyMoneyFile::instance()->transactionList(transactionList, filter);

  //first retrieve the oldest not reconciled transaction
  MyMoneyDate oldestTransactionDate;
  it = transactionList.constBegin();
  if (it != transactionList.constEnd()) {
    oldestTransactionDate = (*it).first.postDate();
    m_statementInfoPageCheckings->m_oldestTransactionDate->setText(i18n("Oldest unmarked transaction: %1", MyMoneyLocale::formatDate(oldestTransactionDate)));
  }

  filter.addState(MyMoneyTransactionFilter::cleared);

  // retrieve the list from the engine to calculate the starting and ending balance
  MyMoneyFile::instance()->transactionList(transactionList, filter);

  MyMoneyMoney balance = MyMoneyFile::instance()->balance(d->m_account.id());
  MyMoneyMoney factor(1, 1);
  if (d->m_account.accountGroup() == MyMoneyAccount::Liability)
    factor = -factor;

  MyMoneyMoney endBalance, startBalance;
  balance = balance * factor;
  endBalance = startBalance = balance;

  tracer.printf("total balance = %s", qPrintable(endBalance.formatMoney("", 2)));

  for (it = transactionList.constBegin(); it != transactionList.constEnd(); ++it) {
    const MyMoneySplit& split = (*it).second;
    balance -= split.shares() * factor;
    if ((*it).first.postDate() > field("statementDate").toDate()) {
      tracer.printf("Reducing balances by %s because postdate of %s/%s(%s) is past statement date", qPrintable((split.shares() * factor).formatMoney("", 2)), qPrintable((*it).first.id()), qPrintable(split.id()), qPrintable((*it).first.postDate().toString(Qt::ISODate)));
      endBalance -= split.shares() * factor;
      startBalance -= split.shares() * factor;
    } else {
      switch (split.reconcileFlag()) {
        case MyMoneySplit::NotReconciled:
          tracer.printf("Reducing balances by %s because %s/%s(%s) is not reconciled", qPrintable((split.shares() * factor).formatMoney("", 2)), qPrintable((*it).first.id()), qPrintable(split.id()), qPrintable((*it).first.postDate().toString(Qt::ISODate)));
          endBalance -= split.shares() * factor;
          startBalance -= split.shares() * factor;
          break;
        case MyMoneySplit::Cleared:
          tracer.printf("Reducing start balance by %s because %s/%s(%s) is cleared", qPrintable((split.shares() * factor).formatMoney("", 2)), qPrintable((*it).first.id()), qPrintable(split.id()), qPrintable((*it).first.postDate().toString(Qt::ISODate)));
          startBalance -= split.shares() * factor;
          break;
        default:
          break;
      }
    }
  }
  //FIXME: port
  m_statementInfoPageCheckings->m_previousBalance->setValue(startBalance);
  m_statementInfoPageCheckings->m_endingBalance->setValue(endBalance);
  tracer.printf("total balance = %s", qPrintable(endBalance.formatMoney("", 2)));
  tracer.printf("start balance = %s", qPrintable(startBalance.formatMoney("", 2)));

  setField("interestDateEdit", field("statementDate").toDate());
  setField("chargesDateEdit", field("statementDate").toDate());
}

void KEndingBalanceDlg::accept()
{
  if ((!field("interestEditValid").toBool() || createTransaction(d->m_tInterest, -1, field("interestEdit").value<MyMoneyMoney>(), field("interestCategoryEdit").toString(), field("interestDateEdit").toDate()))
      && (!field("chargesEditValid").toBool() || createTransaction(d->m_tCharges, 1, field("chargesEdit").value<MyMoneyMoney>(), field("chargesCategoryEdit").toString(), field("chargesDateEdit").toDate())))
    KEndingBalanceDlgDecl::accept();
}

void KEndingBalanceDlg::slotCreateInterestCategory(const QString& txt, QString& id)
{
  createCategory(txt, id, MyMoneyFile::instance()->income());
}

void KEndingBalanceDlg::slotCreateChargesCategory(const QString& txt, QString& id)
{
  createCategory(txt, id, MyMoneyFile::instance()->expense());
}

void KEndingBalanceDlg::createCategory(const QString& txt, QString& id, const MyMoneyAccount& parent)
{
  MyMoneyAccount acc;
  acc.setName(txt);

  emit createCategory(acc, parent);

  id = acc.id();
}

const MyMoneyMoney KEndingBalanceDlg::endingBalance() const
{
  return adjustedReturnValue(m_statementInfoPageCheckings->m_endingBalance->value());
}

const MyMoneyMoney KEndingBalanceDlg::previousBalance() const
{
  return adjustedReturnValue(m_statementInfoPageCheckings->m_previousBalance->value());
}

const MyMoneyMoney KEndingBalanceDlg::adjustedReturnValue(const MyMoneyMoney& v) const
{
  return d->m_account.accountGroup() == MyMoneyAccount::Liability ? -v : v;
}

void KEndingBalanceDlg::slotReloadEditWidgets()
{
  QString payeeId, interestId, chargesId;

  // keep current selected items
  payeeId = field("payeeEdit").toString();
  interestId = field("interestCategoryEdit").toString();
  chargesId = field("chargesCategoryEdit").toString();

  // load the payee and category widgets with data from the engine
  //FIXME: port
  m_interestChargeCheckings->m_payeeEdit->loadPayees(MyMoneyFile::instance()->payeeList());

  // a user request to show all categories in both selectors due to a valid use case.
  AccountSet aSet;
  aSet.addAccountGroup(MyMoneyAccount::Expense);
  aSet.addAccountGroup(MyMoneyAccount::Income);
  //FIXME: port
  aSet.load(m_interestChargeCheckings->m_interestCategoryEdit->selector());
  aSet.load(m_interestChargeCheckings->m_chargesCategoryEdit->selector());

  // reselect currently selected items
  if (!payeeId.isEmpty())
    setField("payeeEdit", payeeId);
  if (!interestId.isEmpty())
    setField("interestCategoryEdit", interestId);
  if (!chargesId.isEmpty())
    setField("chargesCategoryEdit", chargesId);
}

const MyMoneyTransaction KEndingBalanceDlg::interestTransaction()
{
  return d->m_tInterest;
}

const MyMoneyTransaction KEndingBalanceDlg::chargeTransaction()
{
  return d->m_tCharges;
}

bool KEndingBalanceDlg::createTransaction(MyMoneyTransaction &t, const int sign, const MyMoneyMoney& amount, const QString& category, const MyMoneyDate& date)
{
  t = MyMoneyTransaction();

  if (category.isEmpty() || !date.isValid())
    return true;

  MyMoneySplit s1, s2;
  MyMoneyMoney val = amount * MyMoneyMoney(sign, 1);
  try {
    t.setPostDate(date);
    t.setCommodity(d->m_account.currencyId());

    s1.setPayeeId(field("payeeEdit").toString());
    s1.setReconcileFlag(MyMoneySplit::Cleared);
    s1.setAccountId(d->m_account.id());
    s1.setValue(-val);
    s1.setShares(-val);

    s2 = s1;
    s2.setAccountId(category);
    s2.setValue(val);

    t.addSplit(s1);
    t.addSplit(s2);

    QMap<QString, MyMoneyMoney> priceInfo; // just empty
    MyMoneyMoney shares;
    if (!KCurrencyCalculator::setupSplitPrice(shares, t, s2, priceInfo, this)) {
      t = MyMoneyTransaction();
      return false;
    }

    s2.setShares(shares);
    t.modifySplit(s2);

  } catch (const MyMoneyException &e) {
    qDebug("%s", qPrintable(e.what()));
    t = MyMoneyTransaction();
    return false;
  }

  return true;
}

void KEndingBalanceDlg::help()
{
  QString anchor = d->m_helpAnchor[currentPage()];
  if (anchor.isEmpty())
    anchor = QString("details.reconcile.whatis");

  KToolInvocation::invokeHelp(anchor);
}

int KEndingBalanceDlg::nextId() const
{
  // Starting from the current page, look for the first enabled page
  // and return that value
  // If the end of the list is encountered first, then return -1.
  for (int i = currentId() + 1; i < d->m_pages.size() && i < pageIds().size(); ++i) {
    if (d->m_pages.testBit(i))
      return pageIds()[i];
  }
  return -1;
}
