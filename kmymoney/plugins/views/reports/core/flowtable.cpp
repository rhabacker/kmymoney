#include "flowtable.h"

#include "mymoneyaccount.h"
#include "mymoneyfile.h"
#include "mymoneymoney.h"
#include "mymoneyprice.h"
#include "mymoneysecurity.h"
#include "mymoneysplit.h"
#include "mymoneytransaction.h"
#include "reportaccount.h"

#include <QDate>

namespace reports {

FlowTable::FlowTable(const MyMoneyReport& report)
    : ListTable(report)
{
    init();
}

FlowTable::~FlowTable()
{
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
    for (const auto& tx : qAsConst(transactions)) {
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

                row[ctFromAccountID] = fromAcc.id();
                row[ctFromAccount] = fromAcc.fullName();
                row[ctFromTopAccount] = fromAcc.topParentName();

                row[ctToAccountID] = toAcc.id();
                row[ctToAccount] = toAcc.fullName();
                row[ctToTopAccount] = toAcc.topParentName();

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
    m_group << ctFromAccountID << ctToAccountID;
    m_columns << ctFromAccount << ctToAccount;
    m_subtotal << ctValue;

    QVector<cellTypeE> sort = QVector<cellTypeE>::fromList(m_group) << QVector<cellTypeE>::fromList(m_columns) << ctID << ctRank << csID;

    TableRow::setSortCriteria(sort);
    std::sort(m_rows.begin(), m_rows.end());
}

}
