/*
    SPDX-FileCopyrightText: 2016-2020 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "newspliteditor.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QCompleter>
#include <QDate>
#include <QDebug>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStringList>

// ----------------------------------------------------------------------------
// KDE Includes

#include <KConcatenateRowsProxyModel>
#include <KLocalizedString>

// ----------------------------------------------------------------------------
// Project Includes

#include "accountsmodel.h"
#include "amounteditcurrencyhelper.h"
#include "costcentermodel.h"
#include "creditdebithelper.h"
#include "icons.h"
#include "journalmodel.h"
#include "kcurrencycalculator.h"
#include "kmymoneyaccountcombo.h"
#include "kmymoneyutils.h"
#include "mymoneyaccount.h"
#include "mymoneyenums.h"
#include "mymoneyexception.h"
#include "mymoneyfile.h"
#include "mymoneyprice.h"
#include "mymoneysecurity.h"
#include "payeesmodel.h"
#include "splitmodel.h"
#include "splitview.h"
#include "ui_newspliteditor.h"
#include "widgethintframe.h"

using namespace Icons;

struct NewSplitEditor::Private
{
    Private(NewSplitEditor* parent)
        : q(parent)
        , ui(new Ui_NewSplitEditor)
        , accountsModel(new AccountNamesFilterProxyModel(parent))
        , payeesModel(new QSortFilterProxyModel(parent))
        , costCenterModel(new QSortFilterProxyModel(parent))
        , splitModel(0)
        , accepted(false)
        , costCenterRequired(false)
        , showValuesInverted(false)
        , haveShares(false)
        , loadingSplit(false)
        , isIncomeExpense(false)
        , readOnly(false)
        , postDate(QDate::currentDate())
        , amountHelper(nullptr)
    {
        accountsModel->setObjectName("AccountNamesFilterProxyModel");
        costCenterModel->setObjectName("SortedCostCenterModel");
        payeesModel->setObjectName("SortedPayeesModel");

        costCenterModel->setSortLocaleAware(true);
        costCenterModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        payeesModel->setSortLocaleAware(true);
        payeesModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    }

    ~Private()
    {
        delete ui;
    }

    bool checkForValidSplit(bool doUserInteraction = true);

    bool costCenterChanged(int costCenterIndex);
    bool categoryChanged(const QString& accountId);
    bool numberChanged(const QString& newNumber);
    bool amountChanged(CreditDebitHelper* valueHelper);

    void checkMultiCurrency();

    NewSplitEditor* q;
    Ui_NewSplitEditor* ui;
    AccountNamesFilterProxyModel* accountsModel;
    QSortFilterProxyModel* payeesModel;
    QSortFilterProxyModel* costCenterModel;
    SplitModel* splitModel;
    bool accepted;
    bool costCenterRequired;
    bool showValuesInverted;
    bool haveShares;
    bool loadingSplit;
    bool isIncomeExpense;
    bool readOnly;
    MyMoneyAccount counterAccount;
    MyMoneyAccount category;
    MyMoneySecurity commodity;
    MyMoneyMoney shares;
    MyMoneyMoney value;
    MyMoneyMoney price;
    QDate postDate;
    CreditDebitHelper* amountHelper;
    WidgetHintFrameCollection* frameCollection;
};

bool NewSplitEditor::Private::checkForValidSplit(bool doUserInteraction)
{
    QStringList infos;
    bool rc = true;
    if(!costCenterChanged(ui->costCenterCombo->currentIndex())) {
        infos << ui->costCenterCombo->toolTip();
        rc = false;
    }

    if(doUserInteraction) {
        /// @todo add dialog here that shows the @a infos
    }
    return rc;
}

bool NewSplitEditor::Private::costCenterChanged(int costCenterIndex)
{
    bool rc = true;
    WidgetHintFrame::hide(ui->costCenterCombo, i18n("The cost center this transaction should be assigned to."));
    if(costCenterIndex != -1) {
        if(costCenterRequired && ui->costCenterCombo->currentText().isEmpty()) {
            WidgetHintFrame::show(ui->costCenterCombo, i18n("A cost center assignment is required for a transaction in the selected category."));
            rc = false;
        }
    }
    return rc;
}

bool NewSplitEditor::Private::categoryChanged(const QString& accountId)
{
    bool rc = true;
    isIncomeExpense = false;
    if(!accountId.isEmpty()) {
        try {
            const auto model = MyMoneyFile::instance()->accountsModel();
            QModelIndex index = model->indexById(accountId);
            category = model->itemByIndex(index);
            isIncomeExpense = category.isIncomeExpense();
            ui->costCenterCombo->setEnabled(isIncomeExpense);
            ui->costCenterLabel->setEnabled(isIncomeExpense);
            ui->numberEdit->setDisabled(isIncomeExpense);
            ui->numberLabel->setDisabled(isIncomeExpense);
            if (isIncomeExpense) {
                ui->numberEdit->clear();
            } else {
                numberChanged(ui->numberEdit->text());
            }

            checkMultiCurrency();

            costCenterRequired = category.isCostCenterRequired();
            rc &= costCenterChanged(ui->costCenterCombo->currentIndex());
        } catch (MyMoneyException &e) {
            qDebug() << "Ooops: invalid account id" << accountId << "in" << Q_FUNC_INFO;
        }
    }
    return rc;
}

bool NewSplitEditor::Private::numberChanged(const QString& newNumber)
{
    bool rc = true;
    WidgetHintFrame::hide(ui->numberEdit, i18n("The check number used for this transaction."));
    if(!newNumber.isEmpty()) {
        const auto model = MyMoneyFile::instance()->journalModel();
        const auto rows = model->rowCount();
        const auto accountId = ui->accountCombo->getSelected();
        for (int row = 0; row < rows; ++row) {
            const auto idx = model->index(row, 0);
            if (idx.data(eMyMoney::Model::JournalSplitAccountIdRole).toString() == accountId) {
                if (idx.data(eMyMoney::Model::JournalSplitNumberRole).toString() == newNumber) {
                    WidgetHintFrame::show(ui->numberEdit, i18n("The check number <b>%1</b> has already been used in this account.", newNumber));
                    rc = false;
                    break;
                }
            }
        }
    }
    return rc;
}

bool NewSplitEditor::Private::amountChanged(CreditDebitHelper* valueHelper)
{
    Q_UNUSED(valueHelper);
    if (valueHelper->haveValue()) {
        if (isIncomeExpense) {
            shares = valueHelper->value();
        } else {
            value = valueHelper->value();
        }
    }
    bool rc = true;
    checkMultiCurrency();
    return rc;
}

void NewSplitEditor::Private::checkMultiCurrency()
{
    // skip interaction during loading operation
    if (loadingSplit)
        return;

    const auto categoryId = q->accountId();
    const auto file = MyMoneyFile::instance();
    auto const model = file->accountsModel();
    auto account = model->itemById(categoryId);
    auto security = MyMoneyFile::instance()->security(account.currencyId());
    if (security.id() != commodity.id() && !amountHelper->value().isZero()) {
        QPointer<KCurrencyCalculator> calc;
        if (isIncomeExpense) {
            if (price == MyMoneyMoney::ONE) {
                price = file->price(security.id(), commodity.id(), QDate()).rate(commodity.id());
            }
            calc = new KCurrencyCalculator(security,
                                           commodity,
                                           amountHelper->value(),
                                           amountHelper->value() * price,
                                           postDate,
                                           security.smallestAccountFraction(),
                                           q);
        } else {
            if (price == MyMoneyMoney::ONE) {
                price = file->price(commodity.id(), security.id(), QDate()).rate(security.id());
            }
            calc = new KCurrencyCalculator(commodity,
                                           security,
                                           amountHelper->value(),
                                           amountHelper->value() * price,
                                           postDate,
                                           security.smallestAccountFraction(),
                                           q);
        }
        if (calc->exec() == QDialog::Accepted && calc) {
            price = calc->price();
        }
        delete calc;

    } else {
        price = MyMoneyMoney::ONE;
    }
    if (isIncomeExpense) {
        value = shares * price;
    } else {
        shares = value * price;
    }
}


NewSplitEditor::NewSplitEditor(QWidget* parent, const MyMoneySecurity& commodity, const QString& counterAccountId)
    : QFrame(parent, Qt::FramelessWindowHint /* | Qt::X11BypassWindowManagerHint */)
    , d(new Private(this))
{
    d->commodity = commodity;
    auto const file = MyMoneyFile::instance();
    auto view = qobject_cast<SplitView*>(parent->parentWidget());
    Q_ASSERT(view != 0);
    d->splitModel = qobject_cast<SplitModel*>(view->model());

    auto const model = MyMoneyFile::instance()->accountsModel();
    d->counterAccount = model->itemById(counterAccountId);

    d->ui->setupUi(this);
    d->ui->enterButton->setIcon(Icons::get(Icon::DialogOK));
    d->ui->cancelButton->setIcon(Icons::get(Icon::DialogCancel));

    auto concatModel = new KConcatenateRowsProxyModel(parent);
    concatModel->addSourceModel(file->payeesModel()->emptyPayee());
    concatModel->addSourceModel(file->payeesModel());
    d->payeesModel->setSortRole(Qt::DisplayRole);
    d->payeesModel->setSourceModel(concatModel);
    d->payeesModel->sort(0);

    d->ui->payeeEdit->setEditable(true);
    d->ui->payeeEdit->setModel(d->payeesModel);
    d->ui->payeeEdit->setModelColumn(0);
    d->ui->payeeEdit->completer()->setFilterMode(Qt::MatchContains);

    d->accountsModel->addAccountGroup(QVector<eMyMoney::Account::Type> {eMyMoney::Account::Type::Asset, eMyMoney::Account::Type::Liability, eMyMoney::Account::Type::Income, eMyMoney::Account::Type::Expense, eMyMoney::Account::Type::Equity,});
    d->accountsModel->setHideEquityAccounts(false);
    d->accountsModel->setHideZeroBalancedEquityAccounts(false);
    d->accountsModel->setSourceModel(model);
    d->accountsModel->sort(AccountsModel::Column::AccountName);
    d->ui->accountCombo->setModel(d->accountsModel);
    d->ui->accountCombo->setSplitActionVisible(false);

    d->costCenterModel->setSortRole(Qt::DisplayRole);
    d->costCenterModel->setSourceModel(MyMoneyFile::instance()->costCenterModel());
    d->costCenterModel->sort(AccountsModel::Column::AccountName);

    d->ui->costCenterCombo->setEditable(true);
    d->ui->costCenterCombo->setModel(d->costCenterModel);
    d->ui->costCenterCombo->setModelColumn(0);
    d->ui->costCenterCombo->completer()->setFilterMode(Qt::MatchContains);

    d->frameCollection = new WidgetHintFrameCollection(this);
    d->frameCollection->addFrame(new WidgetHintFrame(d->ui->costCenterCombo));
    d->frameCollection->addFrame(new WidgetHintFrame(d->ui->numberEdit, WidgetHintFrame::Warning));
    d->frameCollection->addWidget(d->ui->enterButton);

    d->ui->amountEditCredit->setAllowEmpty(true);
    d->ui->amountEditDebit->setAllowEmpty(true);

    // determine order of credit and debit edit widgets
    // based on their visual order in the ledger
    int creditColumn = SplitModel::Column::Payment;
    int debitColumn = SplitModel::Column::Deposit;

    QWidget* w(this);
    do {
        w = w->parentWidget();
        const auto view = qobject_cast<const QTableView*>(w);
        if (view) {
            creditColumn = view->horizontalHeader()->visualIndex(creditColumn);
            debitColumn = view->horizontalHeader()->visualIndex(debitColumn);
            break;
        }
    } while (w);

    // in case they are in the opposite order, we swap the edit widgets
    if (debitColumn < creditColumn) {
        std::swap(d->ui->amountEditCredit, d->ui->amountEditDebit);
    }

    d->amountHelper = new CreditDebitHelper(this, d->ui->amountEditCredit, d->ui->amountEditDebit);

    new AmountEditCurrencyHelper(d->ui->accountCombo, d->amountHelper, commodity.id());

    connect(d->ui->numberEdit, &QLineEdit::textChanged, this, &NewSplitEditor::numberChanged);
    connect(d->ui->costCenterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewSplitEditor::costCenterChanged);
    connect(d->ui->accountCombo, &KMyMoneyAccountCombo::accountSelected, this, &NewSplitEditor::categoryChanged);
    connect(d->amountHelper, &CreditDebitHelper::valueChanged, this, &NewSplitEditor::amountChanged);

    connect(d->ui->cancelButton, &QToolButton::clicked, this, &NewSplitEditor::reject);
    connect(d->ui->enterButton, &QToolButton::clicked, this, &NewSplitEditor::acceptEdit);

    // setup the tab order
    if (debitColumn < creditColumn) {
        QWidget::setTabOrder(d->ui->amountEditDebit, d->ui->amountEditCredit);
        QWidget::setTabOrder(d->ui->amountEditCredit, d->ui->payeeEdit);
    } else {
        QWidget::setTabOrder(d->ui->amountEditCredit, d->ui->amountEditDebit);
        QWidget::setTabOrder(d->ui->amountEditDebit, d->ui->payeeEdit);
    }
    QWidget::setTabOrder(d->ui->payeeEdit, d->ui->numberEdit);
    QWidget::setTabOrder(d->ui->numberEdit, d->ui->accountCombo);
    QWidget::setTabOrder(d->ui->accountCombo, d->ui->costCenterCombo);
    QWidget::setTabOrder(d->ui->costCenterCombo, d->ui->tagCombo);
    QWidget::setTabOrder(d->ui->tagCombo, d->ui->memoEdit);
    QWidget::setTabOrder(d->ui->memoEdit, d->ui->enterButton);
    QWidget::setTabOrder(d->ui->enterButton, d->ui->cancelButton);
}

NewSplitEditor::~NewSplitEditor()
{
}

void NewSplitEditor::setAmountPlaceHolderText(const QAbstractItemModel* model)
{
    d->ui->amountEditCredit->setPlaceholderText(model->headerData(SplitModel::Column::Payment, Qt::Horizontal).toString());
    d->ui->amountEditDebit->setPlaceholderText(model->headerData(SplitModel::Column::Deposit, Qt::Horizontal).toString());
}

void NewSplitEditor::setPostDate(const QDate& date)
{
    d->postDate = date;
}

void NewSplitEditor::setShowValuesInverted(bool inverse)
{
    d->showValuesInverted = inverse;
}

bool NewSplitEditor::showValuesInverted()
{
    return d->showValuesInverted;
}

bool NewSplitEditor::accepted() const
{
    return d->accepted;
}

void NewSplitEditor::acceptEdit()
{
    if(d->checkForValidSplit()) {
        d->accepted = true;
        emit done();
    }
}

void NewSplitEditor::reject()
{
    emit done();
}

void NewSplitEditor::keyPressEvent(QKeyEvent* e)
{
    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            if(focusWidget() == d->ui->cancelButton) {
                reject();
            } else {
                if (d->ui->enterButton->isEnabled() && !d->readOnly) {
                    d->ui->enterButton->setFocus();
                    d->ui->enterButton->click();
                }
                return;
            }
        }
        break;

        case Qt::Key_Escape:
            reject();
            break;

        default:
            e->ignore();
            return;
        }
    } else {
        e->ignore();
    }
}

QString NewSplitEditor::accountId() const
{
    return d->ui->accountCombo->getSelected();
}

void NewSplitEditor::setAccountId(const QString& id)
{
    d->ui->accountCombo->clearEditText();
    d->ui->accountCombo->setSelected(id);
}


QString NewSplitEditor::memo() const
{
    return d->ui->memoEdit->toPlainText();
}

void NewSplitEditor::setMemo(const QString& memo)
{
    d->ui->memoEdit->setPlainText(memo);
}

MyMoneyMoney NewSplitEditor::shares() const
{
    return d->shares;
}

void NewSplitEditor::setShares(const MyMoneyMoney& shares)
{
    d->shares = shares;
    d->haveShares = true;
}

MyMoneyMoney NewSplitEditor::value() const
{
    return d->value;
}

void NewSplitEditor::setValue(const MyMoneyMoney& value)
{
    d->price = MyMoneyMoney::ONE;
    d->value = value;
    if (!d->haveShares) {
        qDebug() << "NewSplitEditor::setValue(): call to setShares() missing, price invalid";
    } else if (!(d->shares.isZero() || value.isZero())) {
        d->price = d->shares/value;
    }

    if (d->isIncomeExpense) {
        d->amountHelper->setValue(d->shares);
        if (!(d->shares.isZero() || value.isZero())) {
            d->price = value/d->shares;
        }
    } else {
        d->amountHelper->setValue(value);
    }
}


QString NewSplitEditor::costCenterId() const
{
    const int row = d->ui->costCenterCombo->currentIndex();
    QModelIndex index = d->ui->costCenterCombo->model()->index(row, 0);
    return d->ui->costCenterCombo->model()->data(index, eMyMoney::Model::Roles::IdRole).toString();
}

void NewSplitEditor::setCostCenterId(const QString& id)
{
    const auto baseIdx = MyMoneyFile::instance()->costCenterModel()->indexById(id);
    if (baseIdx.isValid()) {
        const auto index = MyMoneyFile::baseModel()->mapFromBaseSource(d->costCenterModel, baseIdx);
        if(index.isValid()) {
            d->ui->costCenterCombo->setCurrentIndex(index.row());
        }
    }
}

QString NewSplitEditor::number() const
{
    return d->ui->numberEdit->text();
}

void NewSplitEditor::setNumber(const QString& number)
{
    d->ui->numberEdit->setText(number);
}

QString NewSplitEditor::payeeId() const
{
    const auto idx = d->payeesModel->index(d->ui->payeeEdit->currentIndex(), 0);
    return idx.data(eMyMoney::Model::IdRole).toString();
}

void NewSplitEditor::setPayeeId(const QString& id)
{
    QModelIndexList indexes = d->payeesModel->match(d->payeesModel->index(0, 0), eMyMoney::Model::IdRole, QVariant(id), 1, Qt::MatchFlags(Qt::MatchFlags(Qt::MatchExactly | Qt::MatchCaseSensitive | Qt::MatchRecursive)));
    int row(0);
    if (!indexes.isEmpty()) {
        row = indexes.first().row();
    }
    d->ui->payeeEdit->setCurrentIndex(row);
}

void NewSplitEditor::numberChanged(const QString& newNumber)
{
    d->numberChanged(newNumber);
}

void NewSplitEditor::categoryChanged(const QString& accountId)
{
    d->categoryChanged(accountId);
}

void NewSplitEditor::costCenterChanged(int costCenterIndex)
{
    d->costCenterChanged(costCenterIndex);
}

void NewSplitEditor::amountChanged()
{
    d->amountChanged(d->amountHelper);
}

void NewSplitEditor::startLoadingSplit()
{
    d->loadingSplit = true;
}

void NewSplitEditor::finishLoadingSplit()
{
    d->loadingSplit = false;
}

void NewSplitEditor::setReadOnly(bool readOnly)
{
    if (d->readOnly != readOnly) {
        d->readOnly = readOnly;
        if (readOnly) {
            d->frameCollection->removeWidget(d->ui->enterButton);
            d->ui->enterButton->setDisabled(true);
        } else {
            // no need to enable the enter button here as the
            // framewidget will take care of it anyway
            d->frameCollection->addWidget(d->ui->enterButton);
        }
    }
}
