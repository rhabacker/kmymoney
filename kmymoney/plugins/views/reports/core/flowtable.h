#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include "listtable.h"

class QStandardItemModel;
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

Q_SIGNALS:
    void sankeyGenerated(const QUrl& url) const;

protected:
    void init();
    void constructFlowTable();
    void drawChart(KReportChartView& chartView) const override;
    bool linkEntries() const final override
    {
        return true;
    }

    QString xmlTagName() const override
    {
        return QStringLiteral("FlowTable");
    }
    void buildSankeyModel(QStandardItemModel& model) const;
};

}

#endif // FLOWTABLE_H
