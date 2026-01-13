#include "flowtable.h"

#include "mymoneyaccount.h"
#include "mymoneyenums.h"
#include "mymoneyfile.h"
#include "mymoneymoney.h"
#include "mymoneyprice.h"
#include "mymoneysecurity.h"
#include "mymoneysplit.h"
#include "mymoneytransaction.h"
#include "reportaccount.h"

#include <QDate>

#include <KLocalizedString>

constexpr QChar tagSeparator = QChar(QChar::ParagraphSeparator);

namespace reports {

FlowTable::FlowTable(const MyMoneyReport& report)
    : ListTable(report)
{
    init();
}

FlowTable::~FlowTable()
{
}

QString reports::FlowTable::FlowTable::toNodeName(const ReportAccount& acc) const
{
    // usually identical logic
    return fromNodeName(acc);
}

QString reports::FlowTable::FlowTable::fromNodeName(const ReportAccount& acc) const
{
    switch (m_config.detailLevel()) {
    case eMyMoney::Report::DetailLevel::All:
        return acc.fullName();

    case eMyMoney::Report::DetailLevel::Top:
        return acc.topParentName();

    case eMyMoney::Report::DetailLevel::Group:
        return MyMoneyAccount::accountTypeToString(acc.accountGroup());

    case eMyMoney::Report::DetailLevel::Total:
        return i18n("Total");

    default:
        return QString();
    }
}

void FlowTable::init()
{
    m_columns.clear();
    m_group.clear();
    m_subtotal.clear();
    m_postcolumns.clear();
    constructFlowTable();
}

void FlowTable::constructFlowTable()
{
    MyMoneyFile* file = MyMoneyFile::instance();

    // 1. Build transaction filter (same logic as reports)
    MyMoneyTransactionFilter filter;
    filter.setDateFilter(m_config.fromDate(), m_config.toDate());

    if (m_config.accounts().size() > 0)
        filter.addAccount(m_config.accounts());

    filter.setReportAllSplits(false);

    QList<MyMoneyTransaction> transactions;
    file->transactionList(transactions, filter);

    const auto baseCurrency = file->baseCurrency();
    const auto fraction = baseCurrency.smallestAccountFraction();

    // 2. Iterate transactions
    for (const MyMoneyTransaction& tx : qAsConst(transactions)) {
        const auto splits = tx.splits();
        if (splits.count() < 2)
            continue;

        // For Sankey we only consider pairwise flows
        for (auto it = splits.cbegin(); it != splits.cend(); ++it) {
            const MyMoneySplit& fromSplit = *it;
            if (fromSplit.shares().isZero())
                continue;

            if (!fromSplit.shares().isNegative())
                continue;

            ReportAccount fromAcc(fromSplit.accountId());
            // if (!fromAcc.is)
            //     continue;

            // Find matching counterpart splits
            for (auto jt = splits.cbegin(); jt != splits.cend(); ++jt) {
                if (it == jt)
                    continue;

                const MyMoneySplit& toSplit = *jt;
                if (toSplit.shares().isZero())
                    continue;

                if (!toSplit.shares().isPositive())
                    continue;

                ReportAccount toAcc(toSplit.accountId());
                // if (!toAcc.isValid())
                //     continue;

                // Ignore same-account noise
                if (fromAcc.id() == toAcc.id())
                    continue;

                // 3. Build table row
                TableRow row;
                row[ctID] = tx.id();
                row[ctPostDate] = tx.postDate().toString(Qt::ISODate);

                row[ctFromAccountID] = fromAcc.id();
                row[ctFromAccount] = fromNodeName(fromAcc);
                row[ctFromCurrency] = fromAcc.currencyId();
                row[ctFromTopAccount] = fromAcc.topParentName();
                row[csFromID] = fromSplit.id();
                row[ctFromTag] = fromSplit.tagIdList().join(tagSeparator);

                row[ctToAccountID] = toAcc.id();
                row[ctToAccount] = toNodeName(toAcc);
                row[ctToCurrency] = toAcc.currencyId();
                row[ctToTopAccount] = toAcc.topParentName();
                row[csToID] = toSplit.id();
                row[ctCommodity] = tx.commodity();
                row[ctToTag] = toSplit.tagIdList().join(tagSeparator);

                // Convert to report currency

                MyMoneyMoney value = toSplit.shares().abs();
                if (m_config.isConvertCurrency()) {
                    MyMoneyMoney rate = (fromAcc.deepCurrencyPrice(tx.postDate()) * fromAcc.baseCurrencyPrice(tx.postDate())).reduce();
                    value *= rate;
                }

                row[ctValue] = value.convert(fraction).toString();
                row[ctRank] = FIRST_SPLIT_RANK;
                row.addSourceLine(ctValue, __LINE__);

                m_rows += row;
            }
        }
    }

    switch (m_config.detailLevel()) {
    case eMyMoney::Report::DetailLevel::All:
        m_group << ctFromAccount << ctToAccount;
        break;

    case eMyMoney::Report::DetailLevel::Top:
        m_group << ctFromAccount << ctToAccount;
        break;

    case eMyMoney::Report::DetailLevel::Group:
        m_group << ctFromAccount << ctToAccount;
        break;

    case eMyMoney::Report::DetailLevel::Total:
        // single aggregated flow
        break;

    default:
        return;
    }

    m_columns << ctFromAccount << ctToAccount;
    m_subtotal << ctValue;

    QVector<cellTypeE> sort = QVector<cellTypeE>::fromList(m_group) << QVector<cellTypeE>::fromList(m_columns) << ctID << ctRank << csID;

    TableRow::setSortCriteria(sort);
    std::sort(m_rows.begin(), m_rows.end());
}

}
