#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include "listtable.h"

namespace reports {

class ReportAccount;

class FlowTable : public ListTable
{
    Q_OBJECT
public:
    explicit FlowTable(const MyMoneyReport& report);
    virtual ~FlowTable();

    QString fromNodeName(const ReportAccount& acc) const;
    QString toNodeName(const ReportAccount& acc) const;

protected:
    void init();
    void constructFlowTable();
    bool linkEntries() const final override
    {
        return true;
    }

    QString xmlTagName() const override
    {
        return QStringLiteral("FlowTable");
    }
};

}

#endif // FLOWTABLE_H
