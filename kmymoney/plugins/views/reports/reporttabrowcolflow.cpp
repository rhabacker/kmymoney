#include "reporttabrowcolflow.h"

#include "mymoneyenums.h"
#include "ui_reporttabrowcolflow.h"

ReportTabRowColFlow::ReportTabRowColFlow(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_ReportTabRowColFlow)
{
    ui->setupUi(this);
}

void ReportTabRowColFlow::load(const MyMoneyReport& report)
{
    ui->m_comboDetail->setCurrentIndex(static_cast<int>(report.detailLevel()) - 1);
    // ui->m_checkHideSplitDetails->setEnabled(!report.isHideTransactions());
    ui->m_checkUnused->setChecked(report.isIncludingUnusedAccounts());
    ui->m_checkTransfers->setChecked(report.isIncludingTransfers());
}

void ReportTabRowColFlow::save(MyMoneyReport& report) const
{
    report.setDetailLevel(static_cast<eMyMoney::Report::DetailLevel>(ui->m_comboDetail->currentIndex() + 1));
    // TODO is this correct
#if 0
    report.setHideTransactions(!ui->m_checkHideSplitDetails->isChecked());
#endif
    report.setIncludingUnusedAccounts(ui->m_checkUnused->isChecked());
    report.setIncludingTransfers(ui->m_checkTransfers->isChecked());
}
