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

protected:
    void init();
    void constructFlowTable();
    bool linkEntries() const final override
    {
        return true;
    }
};

}

#endif // FLOWTABLE_H
