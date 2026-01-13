#include "reporttabrowcolpivot.h"
#include "ui_reporttabrowcolpivot.h"

#include "mymoneybudget.h"
#include "mymoneyenums.h"

#include <QDate>

ReportTabRowColPivot::ReportTabRowColPivot(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_ReportTabRowColPivot)
{
    ui->setupUi(this);
    connect(ui->m_comboRows, QOverload<int>::of(&QComboBox::activated), this, &ReportTabRowColPivot::slotRowTypeChanged);
}

void ReportTabRowColPivot::slotRowTypeChanged(int row)
{
    ui->m_checkTotalColumn->setEnabled(row == 0);
    Q_EMIT rowTypeChanged(row);
}

void ReportTabRowColPivot::load(const MyMoneyReport& report, const QVector<MyMoneyBudget>& budgets)
{
    KComboBox* combo = ui->m_comboDetail;
    switch (report.detailLevel()) {
    case eMyMoney::Report::DetailLevel::None:
    case eMyMoney::Report::DetailLevel::End:
    case eMyMoney::Report::DetailLevel::All:
        combo->setCurrentItem(i18nc("All accounts", "All"), false);
        break;
    case eMyMoney::Report::DetailLevel::Top:
        combo->setCurrentItem(i18n("Top-Level"), false);
        break;
    case eMyMoney::Report::DetailLevel::Group:
        combo->setCurrentItem(i18n("Groups"), false);
        break;
    case eMyMoney::Report::DetailLevel::Total:
        combo->setCurrentItem(i18n("Totals"), false);
        break;
    }

    combo = ui->m_comboRows;
    switch (report.rowType()) {
    case eMyMoney::Report::RowType::ExpenseIncome:
    case eMyMoney::Report::RowType::Budget:
    case eMyMoney::Report::RowType::BudgetActual:
        combo->setCurrentItem(i18n("Income & Expenses"), false); // income / expense
        break;
    default:
        combo->setCurrentItem(i18n("Assets & Liabilities"), false); // asset / liability
        break;
    }
    ui->m_checkTotalColumn->setChecked(report.isShowingRowTotals());
    ui->m_checkTotalRow->setChecked(report.isShowingColumnTotals());
    ui->m_propagateRemainder->setEnabled(report.rowType() == eMyMoney::Report::RowType::BudgetActual);
    ui->m_propagateRemainder->setChecked(report.isPropagateBudgetDifference());
    ui->m_checkTotalRow->setDisabled(report.isPropagateBudgetDifference());

    connect(ui->m_propagateRemainder, &QCheckBox::stateChanged, this, [&](int _state) {
        const auto state = static_cast<Qt::CheckState>(_state);
        ui->m_checkTotalColumn->setDisabled(state == Qt::Checked);
        switch (state) {
        case Qt::Checked:
            ui->m_checkTotalColumn->setChecked(false);
            break;
        default:
            break;
        }
    });

    // prevent sending public signal
    ui->m_comboRows->blockSignals(true);
    slotRowTypeChanged(ui->m_comboRows->currentIndex());
    ui->m_comboRows->blockSignals(false);

    //load budgets combo
    ui->m_comboBudget->setDisabled(true);
    if (report.rowType() == eMyMoney::Report::RowType::Budget
            || report.rowType() == eMyMoney::Report::RowType::BudgetActual) {
        ui->m_comboBudget->setEnabled(true);
        ui->m_comboRows->setEnabled(false);
        ui->m_rowsLabel->setEnabled(false);
        ui->m_budgetFrame->setEnabled(!budgets.empty());
        auto i = 0;
        for (QVector<MyMoneyBudget>::const_iterator it_b = budgets.cbegin(); it_b != budgets.cend(); ++it_b) {
            ui->m_comboBudget->insertItem((*it_b).name(), i);
            //set the current selected item
            if ((report.budget() == "Any" && (*it_b).budgetStart().year() == QDate::currentDate().year())
                    || report.budget() == (*it_b).id())
                ui->m_comboBudget->setCurrentItem(i);
            i++;
        }
    }

    // set moving average days spinbox
    QSpinBox* spinbox = ui->m_movingAverageDays;
    spinbox->setEnabled(report.isIncludingMovingAverage());
    ui->m_movingAverageLabel->setEnabled(report.isIncludingMovingAverage());

    if (report.isIncludingMovingAverage()) {
        spinbox->setValue(report.movingAverageDays());
    }

    ui->m_checkScheduled->setChecked(report.isIncludingSchedules());
    ui->m_checkTransfers->setChecked(report.isIncludingTransfers());
    ui->m_checkUnused->setChecked(report.isIncludingUnusedAccounts());
}

void ReportTabRowColPivot::save(MyMoneyReport& report, QVector<MyMoneyBudget>& budgets, bool isBudgetActual) const
{
    eMyMoney::Report::DetailLevel dl[4] = {eMyMoney::Report::DetailLevel::All,
                                           eMyMoney::Report::DetailLevel::Top,
                                           eMyMoney::Report::DetailLevel::Group,
                                           eMyMoney::Report::DetailLevel::Total};

    report.setDetailLevel(dl[ui->m_comboDetail->currentIndex()]);

    // modify the rowtype only if the widget is enabled
    if (ui->m_comboRows->isEnabled()) {
        eMyMoney::Report::RowType rt[2] = {eMyMoney::Report::RowType::ExpenseIncome, eMyMoney::Report::RowType::AssetLiability};
        report.setRowType(rt[ui->m_comboRows->currentIndex()]);
    }

    report.setShowingRowTotals(false);
    if (ui->m_comboRows->currentIndex() == 0)
        report.setShowingRowTotals(ui->m_checkTotalColumn->isChecked());

    report.setShowingColumnTotals(ui->m_checkTotalRow->isChecked());
    report.setIncludingSchedules(ui->m_checkScheduled->isChecked());
    report.setPropagateBudgetDifference(ui->m_propagateRemainder->isChecked());
    report.setIncludingTransfers(ui->m_checkTransfers->isChecked());

    report.setIncludingUnusedAccounts(ui->m_checkUnused->isChecked());

    // set moving average days
    if (ui->m_movingAverageDays->isEnabled()) {
        report.setMovingAverageDays(ui->m_movingAverageDays->value());
    }
    if (ui->m_comboBudget->isEnabled() && (budgets.count() > 0)) {
        report.setBudget(budgets[ui->m_comboBudget->currentItem()].id(), isBudgetActual);
    } else {
        report.setBudget(QString(), false);
    }
}

bool ReportTabRowColPivot::isBudgetComboEnabled() const
{
    return ui->m_comboBudget->isEnabled();
}

void ReportTabRowColPivot::setTransfersEnabled(bool state)
{
    ui->m_checkTransfers->setEnabled(state);
}

void ReportTabRowColPivot::setTransfersChecked(bool state)
{
    ui->m_checkTransfers->setChecked(state);
}
