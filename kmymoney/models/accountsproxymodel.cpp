/*
    SPDX-FileCopyrightText: 2010-2014 Cristian Oneț <onet.cristian@gmail.com>
    SPDX-FileCopyrightText: 2017-2018 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
    SPDX-FileCopyrightText: 2019-2020 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "accountsproxymodel.h"
#include "accountsproxymodel_p.h"

// ----------------------------------------------------------------------------
// QT Includes

// ----------------------------------------------------------------------------
// KDE Includes

#include <KLocalizedString>

// ----------------------------------------------------------------------------
// Project Includes

#include "accountsmodel.h"
#include "journalmodel.h"
#include "mymoneyenums.h"
#include "mymoneyfile.h"
#include "mymoneymoney.h"
#include "schedulesjournalmodel.h"
#include "securitiesmodel.h"

AccountsProxyModel::AccountsProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    d_ptr(new AccountsProxyModelPrivate)
{
    setObjectName("AccountsProxyModel");
    setRecursiveFilteringEnabled(true);
    setDynamicSortFilter(true);
    setSortLocaleAware(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

AccountsProxyModel::AccountsProxyModel(AccountsProxyModelPrivate &dd, QObject *parent) :
    QSortFilterProxyModel(parent), d_ptr(&dd)
{
    setRecursiveFilteringEnabled(true);
}

AccountsProxyModel::~AccountsProxyModel()
{
}

void AccountsProxyModel::setSourceModel(QAbstractItemModel* model)
{
    if (sourceModel() != nullptr) {
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &QSortFilterProxyModel::invalidate);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &QSortFilterProxyModel::invalidate);
    }
    QSortFilterProxyModel::setSourceModel(model);
    if (sourceModel() != nullptr) {
        connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &QSortFilterProxyModel::invalidate);
        connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &QSortFilterProxyModel::invalidate);
    }
}

/**
  * This function was re-implemented so we could have a special display order (favorites first)
  */
bool AccountsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;
    // different sorting based on the column which is being sorted
    switch (left.column()) {
    // for the accounts column sort based on the DisplayOrderRole
    default:
    case AccountsModel::Column::AccountName: {
        const auto leftData = sourceModel()->data(left, eMyMoney::Model::Roles::AccountDisplayOrderRole);
        const auto rightData = sourceModel()->data(right, eMyMoney::Model::Roles::AccountDisplayOrderRole);

        if (leftData.toInt() == rightData.toInt()) {
            // sort items of the same display order alphabetically
            return QSortFilterProxyModel::lessThan(left, right);
        }
        return leftData.toInt() < rightData.toInt();
    }
    // the balance and total value columns are sorted based on the value of the account
    case AccountsModel::Column::Balance:
    case AccountsModel::Column::TotalPostedValue: {
        const auto leftData = sourceModel()->data(sourceModel()->index(left.row(), AccountsModel::Column::AccountName, left.parent()), eMyMoney::Model::Roles::AccountTotalValueRole);
        const auto rightData = sourceModel()->data(sourceModel()->index(right.row(), AccountsModel::Column::AccountName, right.parent()), eMyMoney::Model::Roles::AccountTotalValueRole);
        return leftData.value<MyMoneyMoney>() < rightData.value<MyMoneyMoney>();
    }
    break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

/**
  * This function was re-implemented to consider all the filtering aspects that we need in the application.
  */
bool AccountsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_D(const AccountsProxyModel);
    if (d->m_hideAllEntries)
        return false;

    if (!source_parent.isValid() && (source_row == 0) && hideFavoriteAccounts())
        return false;

    const auto index = sourceModel()->index(source_row, AccountsModel::Column::AccountName, source_parent);
    return acceptSourceItem(index) && filterAcceptsRowOrChildRows(source_row, source_parent);
}

/**
  * This function implements a recursive matching. It is used to match a row even if it's values
  * doesn't match the current filtering criteria but it has at least one child row that does match.
  */
bool AccountsProxyModel::filterAcceptsRowOrChildRows(int source_row, const QModelIndex &source_parent) const
{
    if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent))
        return true;

    const auto index = sourceModel()->index(source_row, AccountsModel::Column::AccountName, source_parent);
    for (auto i = 0; i < sourceModel()->rowCount(index); ++i) {
        if (filterAcceptsRowOrChildRows(i, index))
            return true;
    }
    return false;
}

/**
  * Add the given account group to the filter.
  * @param group The account group to be added.
  * @see eMyMoney::Account
  */
void AccountsProxyModel::addAccountGroup(const QVector<eMyMoney::Account::Type> &groups)
{
    Q_D(AccountsProxyModel);
    for (const auto& group : groups) {
        switch (group) {
        case eMyMoney::Account::Type::Asset:
            d->m_typeList << eMyMoney::Account::Type::Checkings;
            d->m_typeList << eMyMoney::Account::Type::Savings;
            d->m_typeList << eMyMoney::Account::Type::Cash;
            d->m_typeList << eMyMoney::Account::Type::AssetLoan;
            d->m_typeList << eMyMoney::Account::Type::CertificateDep;
            d->m_typeList << eMyMoney::Account::Type::Investment;
            d->m_typeList << eMyMoney::Account::Type::Stock;
            d->m_typeList << eMyMoney::Account::Type::MoneyMarket;
            d->m_typeList << eMyMoney::Account::Type::Asset;
            d->m_typeList << eMyMoney::Account::Type::Currency;
            break;
        case eMyMoney::Account::Type::Liability:
            d->m_typeList << eMyMoney::Account::Type::CreditCard;
            d->m_typeList << eMyMoney::Account::Type::Loan;
            d->m_typeList << eMyMoney::Account::Type::Liability;
            break;
        case eMyMoney::Account::Type::Income:
            d->m_typeList << eMyMoney::Account::Type::Income;
            break;
        case eMyMoney::Account::Type::Expense:
            d->m_typeList << eMyMoney::Account::Type::Expense;
            break;
        case eMyMoney::Account::Type::Equity:
            d->m_typeList << eMyMoney::Account::Type::Equity;
            break;
        default:
            d->m_typeList << group;
            break;
        }
    }
    invalidateFilter();
}

/**
  * Add the given account type to the filter.
  * @param type The account type to be added.
  * @see eMyMoney::Account
  */
void AccountsProxyModel::addAccountType(eMyMoney::Account::Type type)
{
    Q_D(AccountsProxyModel);
    if (!d->m_typeList.contains(type)) {
        d->m_typeList << type;
        invalidateFilter();
    }
}

/**
  * Remove the given account type from the filter.
  * @param type The account type to be removed.
  * @see eMyMoney::Account
  */
void AccountsProxyModel::removeAccountType(eMyMoney::Account::Type type)
{
    Q_D(AccountsProxyModel);
    if (d->m_typeList.removeAll(type) > 0) {
        invalidateFilter();
    }
}

/**
  * Use this to reset the filter.
  */
void AccountsProxyModel::clear()
{
    Q_D(AccountsProxyModel);
    d->m_typeList.clear();
    d->m_notSelectableId.clear();
    invalidateFilter();
}

void AccountsProxyModel::setNotSelectable(const QString& accountId)
{
    Q_D(AccountsProxyModel);
    d->m_notSelectableId = accountId;
}

void AccountsProxyModel::setClosedSelectable(bool selectable)
{
    Q_D(AccountsProxyModel);
    d->m_canSelectClosedAccounts = selectable;
}

Qt::ItemFlags AccountsProxyModel::flags(const QModelIndex& index) const
{
    Q_D(const AccountsProxyModel);
    auto flags = QSortFilterProxyModel::flags(index);
    if (d->m_notSelectableId == index.data(eMyMoney::Model::IdRole).toString()) {
        flags.setFlag(Qt::ItemIsSelectable, false);
        flags.setFlag(Qt::ItemIsEnabled, false);
    }
    if (!d->m_canSelectClosedAccounts && index.data(eMyMoney::Model::AccountIsClosedRole).toBool()) {
        flags.setFlag(Qt::ItemIsSelectable, false);
    }
    if (!d->m_selectableAccountTypes.empty()
        && (d->m_selectableAccountTypes.find(index.data(eMyMoney::Model::AccountTypeRole).value<eMyMoney::Account::Type>())
            == d->m_selectableAccountTypes.end())) {
        flags.setFlag(Qt::ItemIsEnabled, false);
        flags.setFlag(Qt::ItemIsSelectable, false);
    }
    return flags;
}

QVariant AccountsProxyModel::data(const QModelIndex& index, int role) const
{
    Q_D(const AccountsProxyModel);
    if ((d->m_showSecuritySymbol == true) && (role == Qt::DisplayRole || role == Qt::EditRole) && (index.column() == AccountsModel::Column::AccountName)) {
        auto name = QSortFilterProxyModel::data(index, role).toString();
        const auto securityId = QSortFilterProxyModel::data(index, eMyMoney::Model::AccountCurrencyIdRole).toString();
        const auto security = MyMoneyFile::instance()->securitiesModel()->itemById(securityId);
        if (!security.id().isEmpty() && !security.isCurrency()) {
            return QStringLiteral("%1 (%2)").arg(name, security.tradingSymbol());
        }
        return name;
    }
    return QSortFilterProxyModel::data(index, role);
}

/**
  * Implementation function that performs the actual filtering.
  */
bool AccountsProxyModel::acceptSourceItem(const QModelIndex &source) const
{
    Q_D(const AccountsProxyModel);

    if (source.isValid()) {
        const auto accountTypeValue = source.data(eMyMoney::Model::Roles::AccountTypeRole);
        const bool isValidAccountEntry = accountTypeValue.isValid();
        const bool isValidInstititonEntry = source.data(eMyMoney::Model::Roles::InstitutionBankCodeRole).isValid();

        if (isValidAccountEntry) {
            const auto accountType = static_cast<eMyMoney::Account::Type>(accountTypeValue.toInt());

            switch (state()) {
            case State::Any:
                if (hideClosedAccounts() && source.data(eMyMoney::Model::Roles::AccountIsClosedRole).toBool())
                    return false;

                // we hide stock accounts if not in expert mode
                // we hide equity accounts if not in expert mode
                if (hideEquityAccounts()) {
                    if (accountType == eMyMoney::Account::Type::Equity)
                        return false;

                    if (source.data(eMyMoney::Model::Roles::AccountIsInvestRole).toBool())
                        return false;
                }

                // we hide unused income and expense accounts if the specific flag is set
                if (hideUnusedIncomeExpenseAccounts()) {
                    if ((accountType == eMyMoney::Account::Type::Income) || (accountType == eMyMoney::Account::Type::Expense)) {
                        const auto totalValue = source.data(eMyMoney::Model::Roles::AccountTotalValueRole);
                        if (totalValue.isValid() && totalValue.value<MyMoneyMoney>().isZero()) {
                            Q_EMIT const_cast<AccountsProxyModel*>(this)->unusedIncomeExpenseAccountHidden();
                            return false;
                        }
                    }
                }

                // we don't hide the top level accounts
                if (source.parent() != QModelIndex()) {
                    // we hide zero balance investment accounts
                    if (hideZeroBalancedEquityAccounts()) {
                        if (accountType == eMyMoney::Account::Type::Equity || source.data(eMyMoney::Model::Roles::AccountIsInvestRole).toBool()) {
                            const auto totalValue = source.data(eMyMoney::Model::Roles::AccountTotalValueRole);
                            if (totalValue.isValid() && totalValue.value<MyMoneyMoney>().isZero()) {
                                return false;
                            }
                        }
                    }

                    // check if we hide zero balanced accounts
                    if (hideZeroBalancedAccounts()) {
                        const auto totalValue = source.data(eMyMoney::Model::Roles::AccountTotalValueRole);
                        if (totalValue.isValid() && totalValue.value<MyMoneyMoney>().isZero()) {
                            return false;
                        }
                    }
                }
                break;

            case State::Unused: {
                // if an account has sub-accounts it is still used
                if (sourceModel()->rowCount(source) > 0) {
                    return false;
                }
                const auto accountId = source.data(eMyMoney::Model::IdRole).toString();
                auto journalModel = MyMoneyFile::instance()->journalModel();
                auto indexes = journalModel->match(journalModel->index(0, 0), eMyMoney::Model::JournalSplitAccountIdRole, accountId);
                if (!indexes.isEmpty())
                    return false;
                journalModel = MyMoneyFile::instance()->schedulesJournalModel();
                indexes = journalModel->match(journalModel->index(0, 0), eMyMoney::Model::JournalSplitAccountIdRole, accountId);
                if (!indexes.isEmpty())
                    return false;
                break;
            }
            case State::Closed:
                if (source.data(eMyMoney::Model::AccountIsClosedRole).toBool() == false) {
                    return false;
                }
                break;
            }

            // in case we should hide the favorite accounts section, we filter it out here
            if (d->m_hideFavoriteAccounts && source.data(eMyMoney::Model::AccountIsFavoriteIndexRole).toBool()) {
                return false;
            }

            if (d->m_typeList.contains(accountType)) {
                return true;
            }

        } else if (isValidInstititonEntry) {
            if (sourceModel()->rowCount(source) == 0) {
                // if this is an institution that has no children show it only if hide unused institutions
                // (hide closed accounts for now) is not checked
                return !hideClosedAccounts();
            }
            return true;
        }

        // all parents that have at least one visible child must be visible
        const auto rowCount = sourceModel()->rowCount(source);
        for (auto i = 0; i < rowCount; ++i) {
            const auto index = sourceModel()->index(i, AccountsModel::Column::AccountName, source);
            if (acceptSourceItem(index)) {
                return true;
            }
        }
    }
    return false;
}

/**
  * Set if closed accounts should be hidden or not.
  * @param hideClosedAccounts
  */
void AccountsProxyModel::setHideClosedAccounts(bool hideClosedAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideClosedAccounts ^ hideClosedAccounts) {
        d->m_hideClosedAccounts = hideClosedAccounts;
        invalidateFilter();
    }
}

/**
  * Check if closed accounts are hidden or not.
  */
bool AccountsProxyModel::hideClosedAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideClosedAccounts && !d->m_showAllEntries;
}

/**
  * Set if equity and investment accounts should be hidden or not.
  * @param hideEquityAccounts
  */
void AccountsProxyModel::setHideEquityAccounts(bool hideEquityAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideEquityAccounts ^ hideEquityAccounts) {
        d->m_hideEquityAccounts = hideEquityAccounts;
        invalidateFilter();
    }
}

/**
  * Check if equity and investment accounts are hidden or not.
  */
bool AccountsProxyModel::hideEquityAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideEquityAccounts && !d->m_showAllEntries;
}

/**
 * Set if equity or investment accounts should be hidden if their balance is zero.
 * @param hideZeroBalancedEquityAccounts
 */
void AccountsProxyModel::setHideZeroBalancedEquityAccounts(bool hideZeroBalancedEquityAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideZeroBalanceEquityAccounts ^ hideZeroBalancedEquityAccounts) {
        d->m_hideZeroBalanceEquityAccounts = hideZeroBalancedEquityAccounts;
        invalidateFilter();
    }
}

/**
 * Check if equity or investment accounts are hidden when their balance is zero.
 */
bool AccountsProxyModel::hideZeroBalancedEquityAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideZeroBalanceEquityAccounts && !d->m_showAllEntries;
}

/**
 * Set if accounts should be hidden if their balance is zero.
 * @param hideZeroBalancedAccounts
 */
void AccountsProxyModel::setHideZeroBalancedAccounts(bool hideZeroBalancedAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideZeroBalanceAccounts ^ hideZeroBalancedAccounts) {
        d->m_hideZeroBalanceAccounts = hideZeroBalancedAccounts;
        invalidateFilter();
    }
}

/**
 * Check if accounts are hidden when their balance is zero.
 */
bool AccountsProxyModel::hideZeroBalancedAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideZeroBalanceAccounts && !d->m_showAllEntries;
}

/**
  * Set if empty categories should be hidden or not.
  * @param hideUnusedIncomeExpenseAccounts
  */
void AccountsProxyModel::setHideUnusedIncomeExpenseAccounts(bool hideUnusedIncomeExpenseAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideUnusedIncomeExpenseAccounts ^ hideUnusedIncomeExpenseAccounts) {
        d->m_hideUnusedIncomeExpenseAccounts = hideUnusedIncomeExpenseAccounts;
        invalidateFilter();
    }
}

/**
  * Check if empty categories are hidden or not.
  */
bool AccountsProxyModel::hideUnusedIncomeExpenseAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideUnusedIncomeExpenseAccounts && !d->m_showAllEntries;
}

/**
 * Set if favorite accounts should be hidden or not.
 * @param hideFavoriteAccounts
 */
void AccountsProxyModel::setHideFavoriteAccounts(bool hideFavoriteAccounts)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideFavoriteAccounts ^ hideFavoriteAccounts) {
        d->m_hideFavoriteAccounts = hideFavoriteAccounts;
        invalidateFilter();
    }
}

/**
 * Check if empty categories are hidden or not.
 */
bool AccountsProxyModel::hideFavoriteAccounts() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideFavoriteAccounts;
}

void AccountsProxyModel::setHideAllEntries(bool hideAllEntries)
{
    Q_D(AccountsProxyModel);
    if (d->m_hideAllEntries ^ hideAllEntries) {
        d->m_hideAllEntries = hideAllEntries;
        invalidateFilter();
    }
}

bool AccountsProxyModel::hideAllEntries() const
{
    Q_D(const AccountsProxyModel);
    return d->m_hideAllEntries;
}

void AccountsProxyModel::setShowAllEntries(bool showAllEntries)
{
    Q_D(AccountsProxyModel);
    if (d->m_showAllEntries ^ showAllEntries) {
        d->m_showAllEntries = showAllEntries;
        invalidateFilter();
    }
}

bool AccountsProxyModel::showAllEntries() const
{
    Q_D(const AccountsProxyModel);
    return d->m_showAllEntries;
}

/**
  * Returns the number of visible items after filtering. In case @a includeBaseAccounts
  * is set to @c true, the 5 base accounts (asset, liability, income, expense and equity)
  * will also be counted. The default is @c false.
  */
int AccountsProxyModel::visibleItems(bool includeBaseAccounts) const
{
    auto rows = 0;
    for (auto i = 0; i < rowCount(QModelIndex()); ++i) {
        if(includeBaseAccounts) {
            ++rows;
        }
        const auto childIndex = index(i, 0);
        if (hasChildren(childIndex)) {
            rows += visibleItems(childIndex);
        }
    }
    return rows;
}

/**
  * Returns the number of visible items under the given @a index.
  * The column of the @a index must be 0, otherwise no count will
  * be returned (returns 0).
  */
int AccountsProxyModel::visibleItems(const QModelIndex& index) const
{
    auto rows = 0;
    if (index.isValid() && index.column() == AccountsModel::Column::AccountName) { // CAUTION! Assumption is being made that Account column number is always 0
        const auto *model = index.model();
        const auto rowCount = model->rowCount(index);
        for (auto i = 0; i < rowCount; ++i) {
            ++rows;
            const auto childIndex = model->index(i, index.column(), index);
            if (model->hasChildren(childIndex))
                rows += visibleItems(childIndex);
        }
    }
    return rows;
}

QVector<eMyMoney::Account::Type> AccountsProxyModel::assetLiability()
{
    return QVector<eMyMoney::Account::Type>({eMyMoney::Account::Type::Asset, eMyMoney::Account::Type::Liability});
}

QVector<eMyMoney::Account::Type> AccountsProxyModel::assetLiabilityEquity()
{
    return QVector<eMyMoney::Account::Type>({ eMyMoney::Account::Type::Asset, eMyMoney::Account::Type::Liability, eMyMoney::Account::Type::Equity });
}

QVector<eMyMoney::Account::Type> AccountsProxyModel::incomeExpense()
{
    return QVector<eMyMoney::Account::Type>({ eMyMoney::Account::Type::Income, eMyMoney::Account::Type::Expense });
}

QVector<eMyMoney::Account::Type> AccountsProxyModel::assetLiabilityEquityIncomeExpense()
{
    return assetLiabilityEquity() << incomeExpense();
}

void AccountsProxyModel::setState(AccountsProxyModel::State state)
{
    Q_D(AccountsProxyModel);
    if (d->m_state != state) {
        d->m_state = state;
        invalidateFilter();
    }
}

AccountsProxyModel::State AccountsProxyModel::state() const
{
    Q_D(const AccountsProxyModel);
    return d->m_state;
}

void AccountsProxyModel::setFilterComboBox(QComboBox* comboBox)
{
    Q_D(AccountsProxyModel);
    d->m_filterComboBox = comboBox;
    d->m_filterComboBox->clear();
    d->m_filterComboBox->insertItem(static_cast<int>(State::Any), i18nc("Account filter", "Any status"));
    d->m_filterComboBox->insertItem(static_cast<int>(State::Unused), i18nc("Account filter", "Unused"));
    d->m_filterComboBox->insertItem(static_cast<int>(State::Closed), i18nc("Account filter", "Closed"));

    connect(comboBox, QOverload<int>::of(&QComboBox::activated), this, [&](int idx) {
        setState(static_cast<AccountsProxyModel::State>(idx));
    });
    connect(comboBox, &QComboBox::destroyed, this, [&]() {
        Q_D(AccountsProxyModel);
        d->m_filterComboBox = nullptr;
    });
}

void AccountsProxyModel::setSelectableAccountTypes(KMMSet<eMyMoney::Account::Type> selectableAccountTypes)
{
    Q_D(AccountsProxyModel);
    d->m_selectableAccountTypes = selectableAccountTypes;
}

void AccountsProxyModel::setShowSecuritySymbols(bool showSecuritySymbols)
{
    Q_D(AccountsProxyModel);
    d->m_showSecuritySymbol = showSecuritySymbols;
}

bool AccountsProxyModel::showSecuritySymbols() const
{
    Q_D(const AccountsProxyModel);
    return d->m_showSecuritySymbol;
}
