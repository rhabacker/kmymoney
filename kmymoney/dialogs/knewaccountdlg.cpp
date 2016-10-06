/***************************************************************************
                          knewaccountdlg.cpp
                             -------------------
    copyright            : (C) 2000 by Michael Edwardes <mte@users.sourceforge.net>
                               2004 by Thomas Baumgart <ipwizard@users.sourceforge.net>

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "knewaccountdlg.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QPixmap>
#include <QPushButton>
#include <QLabel>
#include <QToolTip>
#include <QButtonGroup>
#include <QCheckBox>
#include <QTimer>
#include <QTabWidget>
#include <QRadioButton>
#include <QLayout>
#include <QList>
#include <QResizeEvent>

// ----------------------------------------------------------------------------
// KDE Headers

#include <klocale.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kled.h>
#include <kdebug.h>
#include <kglobalsettings.h>

// ----------------------------------------------------------------------------
// Project Includes

#include <kmymoneyedit.h>
#include <kmymoneydateinput.h>
#include <mymoneyexception.h>
#include <mymoneyfile.h>
#include <kmymoneyglobalsettings.h>
#include <mymoneyreport.h>
#include <kguiutils.h>
#include "kmymoneycurrencyselector.h"
#include "mymoneykeyvaluecontainer.h"
#include "knewbankdlg.h"
#include "kmymoneyfile.h"
#include "kmymoneyutils.h"
#include "models.h"

HierarchyFilterProxyModel::HierarchyFilterProxyModel(QObject *parent)
    : AccountsFilterProxyModel(parent)
{
}

/**
  * The current account and all it's children are not selectable because the view is used to select a possible parent account.
  */
Qt::ItemFlags HierarchyFilterProxyModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags flags = AccountsFilterProxyModel::flags(index);
  QModelIndex currentIndex = index;
  while (currentIndex.isValid()) {
    QVariant accountId = data(currentIndex, AccountsModel::AccountIdRole);
    if (accountId.isValid() && accountId.toString() == m_currentAccountId) {
      flags = flags & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled;
    }
    currentIndex = currentIndex.parent();
  }
  return flags;
}

/**
  * Set the account for which to select a parent.
  *
  * @param currentAccountId The current account.
  */
void HierarchyFilterProxyModel::setCurrentAccountId(const QString &currentAccountId)
{
  m_currentAccountId = currentAccountId;
}

/**
  * Get the index of the selected parent account.
  *
  * @return The model index of the selected parent account.
  */
QModelIndex HierarchyFilterProxyModel::getSelectedParentAccountIndex() const
{
  QModelIndexList list = match(index(0, 0), AccountsModel::AccountIdRole, m_currentAccountId, -1, Qt::MatchFlags(Qt::MatchExactly | Qt::MatchCaseSensitive | Qt::MatchRecursive));
  if (!list.empty()) {
    return list.front().parent();
  }
  return QModelIndex();
}

/**
  * Filter the favorites accounts group.
  */
bool HierarchyFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  if (!source_parent.isValid()) {
    QVariant data = sourceModel()->index(source_row, 0, source_parent).data(AccountsModel::AccountIdRole);
    if (data.isValid() && data.toString() == AccountsModel::favoritesAccountId)
      return false;
  }
  return AccountsFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

/**
  * Filter all but the first column.
  */
bool HierarchyFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
  Q_UNUSED(source_parent)
  if (source_column == 0)
    return true;
  return false;
}

KNewAccountDlg::KNewAccountDlg(const MyMoneyAccount& account, bool isEditing, bool categoryEditor, QWidget *parent, const QString& title)
    : KNewAccountDlgDecl(parent),
    m_account(account),
    m_categoryEditor(categoryEditor),
    m_isEditing(isEditing)
{
  QString columnName = ((categoryEditor) ? i18n("Categories") : i18n("Accounts"));

  MyMoneyFile *file = MyMoneyFile::instance();

  // initialize the m_parentAccount member
  MyMoneyAccount::accountTypeE filterAccountGroup = m_account.accountGroup();
  switch (m_account.accountGroup()) {
    case MyMoneyAccount::Asset:
      m_parentAccount = file->asset();
      break;
    case MyMoneyAccount::Liability:
      m_parentAccount = file->liability();
      break;
    case MyMoneyAccount::Income:
      m_parentAccount = file->income();
      break;
    case MyMoneyAccount::Expense:
      m_parentAccount = file->expense();
      break;
    case MyMoneyAccount::Equity:
      m_parentAccount = file->equity();
      break;
    default:
      qDebug("Seems we have an account that hasn't been mapped to the top five");
      if (m_categoryEditor) {
        m_parentAccount = file->income();
        filterAccountGroup = MyMoneyAccount::Income;
      } else {
        m_parentAccount = file->asset();
        filterAccountGroup = MyMoneyAccount::Asset;
      }
  }

  m_amountGroup->setId(m_grossAmount, 0);
  m_amountGroup->setId(m_netAmount, 1);

  // the proxy filter model
  m_filterProxyModel = new HierarchyFilterProxyModel(this);
  m_filterProxyModel->setHideClosedAccounts(true);
  m_filterProxyModel->setHideEquityAccounts(!KMyMoneyGlobalSettings::expertMode());
  m_filterProxyModel->addAccountGroup(filterAccountGroup);
  m_filterProxyModel->setCurrentAccountId(m_account.id());
  m_filterProxyModel->setSourceModel(Models::instance()->accountsModel());
  m_filterProxyModel->setDynamicSortFilter(true);

  m_parentAccounts->setAlternatingRowColors(true);
  m_parentAccounts->setIconSize(QSize(22, 22));
  m_parentAccounts->setSortingEnabled(true);
  m_parentAccounts->setAllColumnsShowFocus(true);
  m_parentAccounts->setModel(m_filterProxyModel);
  m_parentAccounts->sortByColumn(0, Qt::AscendingOrder);

  m_subAccountLabel->setText(i18n("Is a sub account"));

  accountNameEdit->setText(account.name());
  descriptionEdit->setText(account.description());

  typeCombo->setEnabled(true);

  // load the price mode combo
  m_priceMode->insertItem(i18nc("default price mode", "(default)"), 0);
  m_priceMode->insertItem(i18n("Price per share"), 1);
  m_priceMode->insertItem(i18n("Total for all shares"), 2);

  int priceMode = 0;
  if (m_account.accountType() == MyMoneyAccount::Investment) {
    m_priceMode->setEnabled(true);
    if (!m_account.value("priceMode").isEmpty())
      priceMode = m_account.value("priceMode").toInt();
  }
  m_priceMode->setCurrentItem(priceMode);

  bool haveMinBalance = false;
  bool haveMaxCredit = false;
  if (!m_account.openingDate().isValid()) {
    m_account.setOpeningDate(KMyMoneyGlobalSettings::firstFiscalDate());
  }
  m_openingDateEdit->setDate(m_account.openingDate());

  if (categoryEditor) {
    // get rid of the tabs that are not used for categories
    int tab = m_tab->indexOf(m_institutionTab);
    if (tab != -1)
      m_tab->removeTab(tab);
    tab = m_tab->indexOf(m_limitsTab);
    if (tab != -1)
      m_tab->removeTab(tab);

    //m_qlistviewParentAccounts->setEnabled(true);
    accountNoEdit->setEnabled(false);

    m_institutionBox->hide();
    m_qcheckboxNoVat->hide();

    typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Income));
    typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Expense));

    // Hardcoded but acceptable - if above we set the default to income do the same here
    switch (account.accountType()) {
      case MyMoneyAccount::Expense:
        typeCombo->setCurrentItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Expense), false);
        break;

      case MyMoneyAccount::Income:
      default:
        typeCombo->setCurrentItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Income), false);
        break;
    }
    m_currency->setEnabled(true);
    if (m_isEditing) {
      typeCombo->setEnabled(false);
      m_currency->setDisabled(MyMoneyFile::instance()->isReferenced(m_account));
    }
    m_qcheckboxPreferred->hide();

    m_qcheckboxTax->setChecked(account.value("Tax") == "Yes");
    loadVatAccounts();
  } else {
    // get rid of the tabs that are not used for accounts
    int taxtab = m_tab->indexOf(m_taxTab);
    if (taxtab != -1) {
        m_vatCategory->setText(i18n("VAT account"));
        m_qcheckboxTax->setChecked(account.value("Tax") == "Yes");
        loadVatAccounts();
    } else {
        m_tab->removeTab(taxtab);
    }

    switch (m_account.accountType()) {
      case MyMoneyAccount::Savings:
      case MyMoneyAccount::Cash:
        haveMinBalance = true;
        break;

      case MyMoneyAccount::Checkings:
        haveMinBalance = true;
        haveMaxCredit = true;
        break;

      case MyMoneyAccount::CreditCard:
        haveMaxCredit = true;
        break;

      default:
        // no limit available, so we might get rid of the tab
        int tab = m_tab->indexOf(m_limitsTab);
        if (tab != -1)
          m_tab->removeTab(tab);
        // don't try to hide the widgets we just wiped
        // in the next step
        haveMaxCredit = haveMinBalance = true;
        break;
    }

    if (!haveMaxCredit) {
      m_maxCreditLabel->setEnabled(false);
      m_maxCreditLabel->hide();
      m_maxCreditEarlyEdit->hide();
      m_maxCreditAbsoluteEdit->hide();
    }
    if (!haveMinBalance) {
      m_minBalanceLabel->setEnabled(false);
      m_minBalanceLabel->hide();
      m_minBalanceEarlyEdit->hide();
      m_minBalanceAbsoluteEdit->hide();
    }

    QString typeString = KMyMoneyUtils::accountTypeToString(account.accountType());

    if (m_isEditing) {
      if (account.isLiquidAsset()) {
        typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Checkings));
        typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Savings));
        typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Cash));
      } else {
        typeCombo->addItem(typeString);
        // Once created, accounts of other account types are not
        // allowed to be changed.
        typeCombo->setEnabled(false);
      }
      // Once created, a currency cannot be changed if it is referenced.
      m_currency->setDisabled(MyMoneyFile::instance()->isReferenced(m_account));
    } else {
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Checkings));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Savings));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Cash));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::CreditCard));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Loan));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Investment));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Asset));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Liability));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Stock));
      /*
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::CertificateDep));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::MoneyMarket));
      typeCombo->addItem(KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Currency));
      */
      // Do not create account types that are not supported
      // by the current engine.
      if (account.accountType() == MyMoneyAccount::UnknownAccountType ||
          account.accountType() == MyMoneyAccount::CertificateDep ||
          account.accountType() == MyMoneyAccount::MoneyMarket ||
          account.accountType() == MyMoneyAccount::Currency)
        typeString = KMyMoneyUtils::accountTypeToString(MyMoneyAccount::Checkings);
    }

    typeCombo->setCurrentItem(typeString, false);

    if (account.isInvest())
      m_institutionBox->hide();

    accountNoEdit->setText(account.number());
    m_qcheckboxPreferred->setChecked(account.value("PreferredAccount") == "Yes");
    m_qcheckboxNoVat->setChecked(account.value("NoVat") == "Yes");
    loadKVP("iban", ibanEdit);
    loadKVP("minBalanceAbsolute", m_minBalanceAbsoluteEdit);
    loadKVP("minBalanceEarly", m_minBalanceEarlyEdit);
    loadKVP("maxCreditAbsolute", m_maxCreditAbsoluteEdit);
    loadKVP("maxCreditEarly", m_maxCreditEarlyEdit);
    // reverse the sign for display purposes
    if (!m_maxCreditAbsoluteEdit->lineedit()->text().isEmpty())
      m_maxCreditAbsoluteEdit->setValue(m_maxCreditAbsoluteEdit->value()*MyMoneyMoney::MINUS_ONE);
    if (!m_maxCreditEarlyEdit->lineedit()->text().isEmpty())
      m_maxCreditEarlyEdit->setValue(m_maxCreditEarlyEdit->value()*MyMoneyMoney::MINUS_ONE);
    loadKVP("lastNumberUsed", m_lastCheckNumberUsed);

    if (m_account.isInvest()) {
      typeCombo->setEnabled(false);
      m_qcheckboxPreferred->hide();
      m_currencyText->hide();
      m_currency->hide();
    } else {
      // use the old field and override a possible new value
      if (!MyMoneyMoney(account.value("minimumBalance")).isZero()) {
        m_minBalanceAbsoluteEdit->setValue(MyMoneyMoney(account.value("minimumBalance")));
      }
    }

//    m_qcheckboxTax->hide(); TODO should only be visible for VAT category/account
  }

  m_currency->setSecurity(file->currency(account.currencyId()));

  // Load the institutions
  // then the accounts
  QString institutionName;

  try {
    if (m_isEditing && !account.institutionId().isEmpty())
      institutionName = file->institution(account.institutionId()).name();
    else
      institutionName.clear();
  } catch (const MyMoneyException &e) {
    qDebug("exception in init for account dialog: %s", qPrintable(e.what()));
  }

  if (m_account.isInvest())
    m_parentAccounts->setEnabled(false);

  if (!categoryEditor)
    slotLoadInstitutions(institutionName);

  accountNameEdit->setFocus();

  if (!title.isEmpty())
    setWindowTitle(title);

  // load button icons
  cancelButton->setGuiItem(KStandardGuiItem::cancel());
  createButton->setGuiItem(KStandardGuiItem::ok());

  connect(cancelButton, SIGNAL(clicked()), SLOT(reject()));
  connect(createButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(m_parentAccounts->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)));
  connect(m_qbuttonNew, SIGNAL(clicked()), this, SLOT(slotNewClicked()));
  connect(typeCombo, SIGNAL(activated(QString)),
          this, SLOT(slotAccountTypeChanged(QString)));

  connect(accountNameEdit, SIGNAL(textChanged(QString)), this, SLOT(slotCheckFinished()));

  connect(m_vatCategory, SIGNAL(toggled(bool)), this, SLOT(slotVatChanged(bool)));
  connect(m_vatAssignment, SIGNAL(toggled(bool)), this, SLOT(slotVatAssignmentChanged(bool)));
  connect(m_vatCategory, SIGNAL(toggled(bool)), this, SLOT(slotCheckFinished()));
  connect(m_vatAssignment, SIGNAL(toggled(bool)), this, SLOT(slotCheckFinished()));
  connect(m_vatRate, SIGNAL(textChanged(QString)), this, SLOT(slotCheckFinished()));
  connect(m_vatAccount, SIGNAL(stateChanged()), this, SLOT(slotCheckFinished()));

  connect(m_minBalanceEarlyEdit, SIGNAL(valueChanged(QString)), this, SLOT(slotAdjustMinBalanceAbsoluteEdit(QString)));
  connect(m_minBalanceAbsoluteEdit, SIGNAL(valueChanged(QString)), this, SLOT(slotAdjustMinBalanceEarlyEdit(QString)));
  connect(m_maxCreditEarlyEdit, SIGNAL(valueChanged(QString)), this, SLOT(slotAdjustMaxCreditAbsoluteEdit(QString)));
  connect(m_maxCreditAbsoluteEdit, SIGNAL(valueChanged(QString)), this, SLOT(slotAdjustMaxCreditEarlyEdit(QString)));

  connect(m_qcomboboxInstitutions, SIGNAL(activated(QString)), this, SLOT(slotLoadInstitutions(QString)));

  QModelIndex parentIndex = m_filterProxyModel->getSelectedParentAccountIndex();
  m_parentAccounts->expand(parentIndex);
  m_parentAccounts->selectionModel()->select(parentIndex, QItemSelectionModel::SelectCurrent);
  m_parentAccounts->scrollTo(parentIndex, QAbstractItemView::PositionAtTop);

  m_vatCategory->setChecked(false);
  m_vatAssignment->setChecked(false);

  // make sure our account does not have an id and no parent assigned
  // and certainly no children in case we create a new account
  if (!m_isEditing) {
    m_account.clearId();
    m_account.setParentAccountId(QString());
    QStringList::ConstIterator it;
    while ((it = m_account.accountList().begin()) != m_account.accountList().end())
      m_account.removeAccountId(*it);

  } else {
    if (!m_account.value("VatRate").isEmpty()) {
      m_vatCategory->setChecked(true);
      m_vatRate->setValue(MyMoneyMoney(m_account.value("VatRate"))*MyMoneyMoney(100, 1));
    } else {
      if (!m_account.value("VatAccount").isEmpty()) {
        QString accId = m_account.value("VatAccount").toLatin1();
        try {
          // make sure account exists
          MyMoneyFile::instance()->account(accId);
          m_vatAssignment->setChecked(true);
          m_vatAccount->setSelected(accId);
          m_grossAmount->setChecked(true);
          if (m_account.value("VatAmount") == "Net")
            m_netAmount->setChecked(true);
        } catch (const MyMoneyException &) {
        }
      }
    }
  }
  slotVatChanged(m_vatCategory->isChecked());
  slotVatAssignmentChanged(m_vatAssignment->isChecked());
  slotCheckFinished();

  kMandatoryFieldGroup* requiredFields = new kMandatoryFieldGroup(this);
  requiredFields->setOkButton(createButton); // button to be enabled when all fields present
  requiredFields->add(accountNameEdit);
}

void KNewAccountDlg::setOpeningBalance(const MyMoneyMoney& balance)
{
  m_openingBalanceEdit->setValue(balance);
}

void KNewAccountDlg::setOpeningBalanceShown(bool shown)
{
  m_openingBalanceLabel->setVisible(shown);
  m_openingBalanceEdit->setVisible(shown);
}

void KNewAccountDlg::setOpeningDateShown(bool shown)
{
  m_openingDateLabel->setVisible(shown);
  m_openingDateEdit->setVisible(shown);
}

void KNewAccountDlg::okClicked()
{
  MyMoneyFile* file = MyMoneyFile::instance();

  QString accountNameText = accountNameEdit->text();
  if (accountNameText.isEmpty()) {
    KMessageBox::error(this, i18n("You have not specified a name.\nPlease fill in this field."));
    accountNameEdit->setFocus();
    return;
  }

  MyMoneyAccount parent = parentAccount();
  if (parent.name().length() == 0) {
    KMessageBox::error(this, i18n("Please select a parent account."));
    return;
  }

  if (!m_categoryEditor) {
    QString institutionNameText = m_qcomboboxInstitutions->currentText();
    if (institutionNameText != i18n("(No Institution)")) {
      try {
        MyMoneyFile *file = MyMoneyFile::instance();

        QList<MyMoneyInstitution> list = file->institutionList();
        QList<MyMoneyInstitution>::ConstIterator institutionIterator;
        for (institutionIterator = list.constBegin(); institutionIterator != list.constEnd(); ++institutionIterator) {
          if ((*institutionIterator).name() == institutionNameText)
            m_account.setInstitutionId((*institutionIterator).id());
        }
      } catch (const MyMoneyException &e) {
        qDebug("Exception in account institution set: %s", qPrintable(e.what()));
      }
    } else {
      m_account.setInstitutionId(QString());
    }
  }

  m_account.setName(accountNameText);
  m_account.setNumber(accountNoEdit->text());
  storeKVP("iban", ibanEdit);
  storeKVP("minBalanceAbsolute", m_minBalanceAbsoluteEdit);
  storeKVP("minBalanceEarly", m_minBalanceEarlyEdit);

  // the figures for credit line with reversed sign
  if (!m_maxCreditAbsoluteEdit->lineedit()->text().isEmpty())
    m_maxCreditAbsoluteEdit->setValue(m_maxCreditAbsoluteEdit->value()*MyMoneyMoney::MINUS_ONE);
  if (!m_maxCreditEarlyEdit->lineedit()->text().isEmpty())
    m_maxCreditEarlyEdit->setValue(m_maxCreditEarlyEdit->value()*MyMoneyMoney::MINUS_ONE);
  storeKVP("maxCreditAbsolute", m_maxCreditAbsoluteEdit);
  storeKVP("maxCreditEarly", m_maxCreditEarlyEdit);
  if (!m_maxCreditAbsoluteEdit->lineedit()->text().isEmpty())
    m_maxCreditAbsoluteEdit->setValue(m_maxCreditAbsoluteEdit->value()*MyMoneyMoney::MINUS_ONE);
  if (!m_maxCreditEarlyEdit->lineedit()->text().isEmpty())
    m_maxCreditEarlyEdit->setValue(m_maxCreditEarlyEdit->value()*MyMoneyMoney::MINUS_ONE);

  storeKVP("lastNumberUsed", m_lastCheckNumberUsed);
  // delete a previous version of the minimumbalance information
  storeKVP("minimumBalance", QString(), QString());

  MyMoneyAccount::accountTypeE acctype;
  if (!m_categoryEditor) {
    acctype = KMyMoneyUtils::stringToAccountType(typeCombo->currentText());
    // If it's a loan, check if the parent is asset or liability. In
    // case of asset, we change the account type to be AssetLoan
    if (acctype == MyMoneyAccount::Loan
        && parent.accountGroup() == MyMoneyAccount::Asset)
      acctype = MyMoneyAccount::AssetLoan;
  } else {
    acctype = parent.accountGroup();
    QString newName;
    if (!MyMoneyFile::instance()->isStandardAccount(parent.id())) {
      newName = MyMoneyFile::instance()->accountToCategory(parent.id()) + MyMoneyFile::AccountSeperator;
    }
    newName += accountNameText;
    if (!file->categoryToAccount(newName, acctype).isEmpty()
        && (file->categoryToAccount(newName, acctype) != m_account.id())) {
      KMessageBox::error(this, QString("<qt>") + i18n("A category named <b>%1</b> already exists. You cannot create a second category with the same name.", newName) + QString("</qt>"));
      return;
    }
  }
  m_account.setAccountType(acctype);

  m_account.setDescription(descriptionEdit->toPlainText());

  m_account.setOpeningDate(m_openingDateEdit->date());

  if (!m_categoryEditor) {
    m_account.setCurrencyId(m_currency->security().id());

    if (m_qcheckboxPreferred->isChecked())
      m_account.setValue("PreferredAccount", "Yes");
    else
      m_account.deletePair("PreferredAccount");
    if (m_qcheckboxNoVat->isChecked())
      m_account.setValue("NoVat", "Yes");
    else
      m_account.deletePair("NoVat");

    if (m_minBalanceAbsoluteEdit->isVisible()) {
      m_account.setValue("minimumBalance", m_minBalanceAbsoluteEdit->value().toString());
    }
  } else {
    if (KMyMoneyGlobalSettings::hideUnusedCategory() && !m_isEditing) {
      KMessageBox::information(this, i18n("You have selected to suppress the display of unused categories in the KMyMoney configuration dialog. The category you just created will therefore only be shown if it is used. Otherwise, it will be hidden in the accounts/categories view."), i18n("Hidden categories"), "NewHiddenCategory");
    }
  }

  if (m_qcheckboxTax->isChecked())
    m_account.setValue("Tax", "Yes");
  else
    m_account.deletePair("Tax");

  m_account.deletePair("VatAccount");
  m_account.deletePair("VatAmount");
  m_account.deletePair("VatRate");

  if (m_vatCategory->isChecked()) {
    m_account.setValue("VatRate", (m_vatRate->value().abs() / MyMoneyMoney(100, 1)).toString());
  } else {
    if (m_vatAssignment->isChecked() && !m_vatAccount->selectedItems().isEmpty()) {
      m_account.setValue("VatAccount", m_vatAccount->selectedItems().first());
      if (m_netAmount->isChecked())
        m_account.setValue("VatAmount", "Net");
    }
  }

  accept();
}

void KNewAccountDlg::loadKVP(const QString& key, kMyMoneyEdit* widget)
{
  if (!widget)
    return;

  if (m_account.value(key).isEmpty()) {
    widget->clearText();
  } else {
    widget->setValue(MyMoneyMoney(m_account.value(key)));
  }
}

void KNewAccountDlg::loadKVP(const QString& key, KLineEdit* widget)
{
  if (!widget)
    return;

  widget->setText(m_account.value(key));
}

void KNewAccountDlg::storeKVP(const QString& key, const QString& text, const QString& value)
{
  if (text.isEmpty())
    m_account.deletePair(key);
  else
    m_account.setValue(key, value);
}

void KNewAccountDlg::storeKVP(const QString& key, kMyMoneyEdit* widget)
{
  storeKVP(key, widget->lineedit()->text(), widget->text());
}

void KNewAccountDlg::storeKVP(const QString& key, KLineEdit* widget)
{
  storeKVP(key, widget->text(), widget->text());
}

const MyMoneyAccount& KNewAccountDlg::account()
{
  // assign the right currency to the account
  m_account.setCurrencyId(m_currency->security().id());

  // and the price mode
  switch (m_priceMode->currentItem()) {
    case 0:
      m_account.deletePair("priceMode");
      break;
    case 1:
    case 2:
      m_account.setValue("priceMode", QString("%1").arg(m_priceMode->currentItem()));
      break;
  }

  return m_account;
}

const MyMoneyAccount& KNewAccountDlg::parentAccount()
{
  return m_parentAccount;
}

void KNewAccountDlg::slotSelectionChanged(const QItemSelection &current, const QItemSelection &previous)
{
  Q_UNUSED(previous)
  if (!current.indexes().empty()) {
    QVariant account = m_parentAccounts->model()->data(current.indexes().front(), AccountsModel::AccountRole);
    if (account.isValid()) {
      m_parentAccount = account.value<MyMoneyAccount>();
      m_subAccountLabel->setText(i18n("Is a sub account of %1", m_parentAccount.name()));
    }
  }
}

void KNewAccountDlg::loadVatAccounts()
{
  QList<MyMoneyAccount> list;
  MyMoneyFile::instance()->accountList(list);
  QList<MyMoneyAccount>::Iterator it;
  QStringList loadListExpense;
  QStringList loadListIncome;
  QStringList loadListAsset;
  QStringList loadListLiability;
  for (it = list.begin(); it != list.end(); ++it) {
    if (!(*it).value("VatRate").isEmpty()) {
      if ((*it).accountType() == MyMoneyAccount::Expense)
        loadListExpense += (*it).id();
      else if ((*it).accountType() == MyMoneyAccount::Income)
        loadListIncome += (*it).id();
      else if ((*it).accountType() == MyMoneyAccount::Asset)
        loadListAsset += (*it).id();
      else if ((*it).accountType() == MyMoneyAccount::Liability)
        loadListLiability += (*it).id();
    }
  }
  AccountSet vatSet;
  if (!loadListAsset.isEmpty())
    vatSet.load(m_vatAccount, i18n("Asset"), loadListAsset, true);
  if (!loadListLiability.isEmpty())
    vatSet.load(m_vatAccount, i18n("Liability"), loadListLiability, false);
  if (!loadListIncome.isEmpty())
    vatSet.load(m_vatAccount, i18n("Income"), loadListIncome, false);
  if (!loadListExpense.isEmpty())
    vatSet.load(m_vatAccount, i18n("Expense"), loadListExpense, false);
}

void KNewAccountDlg::slotLoadInstitutions(const QString& name)
{
  m_qcomboboxInstitutions->clear();
  QString bic;
  // Are we forcing the user to use institutions?
  m_qcomboboxInstitutions->addItem(i18n("(No Institution)"));
  m_bicValue->setText(" ");
  ibanEdit->setEnabled(false);
  accountNoEdit->setEnabled(false);
  try {
    MyMoneyFile *file = MyMoneyFile::instance();

    QList<MyMoneyInstitution> list = file->institutionList();
    QList<MyMoneyInstitution>::ConstIterator institutionIterator;
    for (institutionIterator = list.constBegin(); institutionIterator != list.constEnd(); ++institutionIterator) {
      if ((*institutionIterator).name() == name) {
        ibanEdit->setEnabled(true);
        accountNoEdit->setEnabled(true);
        m_bicValue->setText((*institutionIterator).value("bic"));
      }
      m_qcomboboxInstitutions->addItem((*institutionIterator).name());
    }

    m_qcomboboxInstitutions->setCurrentItem(name, false);
  } catch (const MyMoneyException &e) {
    qDebug("Exception in institution load: %s", qPrintable(e.what()));
  }
}

void KNewAccountDlg::slotNewClicked()
{
  MyMoneyInstitution institution;

  QPointer<KNewBankDlg> dlg = new KNewBankDlg(institution, this);
  if (dlg->exec()) {
    MyMoneyFileTransaction ft;
    try {
      MyMoneyFile *file = MyMoneyFile::instance();

      institution = dlg->institution();
      file->addInstitution(institution);
      ft.commit();
      slotLoadInstitutions(institution.name());
    } catch (const MyMoneyException &) {
      KMessageBox::information(this, i18n("Cannot add institution"));
    }
  }
  delete dlg;
}

void KNewAccountDlg::slotAccountTypeChanged(const QString& typeStr)
{
  MyMoneyAccount::accountTypeE type;
  MyMoneyAccount::accountTypeE oldType;

  type = KMyMoneyUtils::stringToAccountType(typeStr);
  try {
    oldType = m_account.accountType();
    if (oldType != type) {
      m_account.setAccountType(type);
      // update the account group displayed in the accounts hierarchy
      m_filterProxyModel->clear();
      m_filterProxyModel->addAccountGroup(m_account.accountGroup());
    }
  } catch (const MyMoneyException &) {
    qWarning("Unexpected exception in KNewAccountDlg::slotAccountTypeChanged()");
  }
}

void KNewAccountDlg::slotCheckFinished()
{
  bool showButton = true;

  if (accountNameEdit->text().length() == 0) {
    showButton = false;
  }

  if (m_vatCategory->isChecked() && m_vatRate->value() <= MyMoneyMoney()) {
    showButton = false;
  } else {
    if (m_vatAssignment->isChecked() && m_vatAccount->selectedItems().isEmpty())
      showButton = false;
  }
  createButton->setEnabled(showButton);
}

void KNewAccountDlg::slotVatChanged(bool state)
{
  if (state) {
    m_vatCategoryFrame->show();
    m_vatAssignmentFrame->hide();
  } else {
    m_vatCategoryFrame->hide();
    if (!m_account.isAssetLiability()) {
      m_vatAssignmentFrame->show();
    }
  }
}

void KNewAccountDlg::slotVatAssignmentChanged(bool state)
{
  m_vatAccount->setEnabled(state);
  m_amountGroupBox->setEnabled(state);
}

void KNewAccountDlg::adjustEditWidgets(kMyMoneyEdit* dst, kMyMoneyEdit* src, char mode, int corr)
{
  MyMoneyMoney factor(corr, 1);
  if (m_account.accountGroup() == MyMoneyAccount::Asset)
    factor = -factor;

  switch (mode) {
    case '<':
      if (src->value()*factor < dst->value()*factor)
        dst->setValue(src->value());
      break;

    case '>':
      if (src->value()*factor > dst->value()*factor)
        dst->setValue(src->value());
      break;
  }
}

void KNewAccountDlg::slotAdjustMinBalanceAbsoluteEdit(const QString&)
{
  adjustEditWidgets(m_minBalanceAbsoluteEdit, m_minBalanceEarlyEdit, '<', -1);
}

void KNewAccountDlg::slotAdjustMinBalanceEarlyEdit(const QString&)
{
  adjustEditWidgets(m_minBalanceEarlyEdit, m_minBalanceAbsoluteEdit, '>', -1);
}

void KNewAccountDlg::slotAdjustMaxCreditAbsoluteEdit(const QString&)
{
  adjustEditWidgets(m_maxCreditAbsoluteEdit, m_maxCreditEarlyEdit, '>', 1);
}

void KNewAccountDlg::slotAdjustMaxCreditEarlyEdit(const QString&)
{
  adjustEditWidgets(m_maxCreditEarlyEdit, m_maxCreditAbsoluteEdit, '<', 1);
}

void KNewAccountDlg::addTab(QWidget* w, const QString& name)
{
  if (w) {
    w->setParent(m_tab);
    m_tab->addTab(w, name);
  }
}
