/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2026 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "reporttabrowcolperformance.h"
#include "mymoneyenums.h"

#include <QCheckBox>

#include <ui_reporttabperformancecolumns.h>

ReportTabRowColPerformance::ReportTabRowColPerformance(QWidget* parent)
    : QWidget(parent)
    , m_isModified(false)
    , m_isLoading(false)
{
    ui = new Ui::ReportTabPerformanceColumns;
    ui->setupUi(this);

    const auto markModified = [this]() {
        if (!m_isLoading)
            m_isModified = true;
    };
    connect(ui->m_checkStartingMarketValue, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkBuys, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkSells, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkReinvestIncome, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkCashIncome, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkEndingMarketValue, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkReturn, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkReturnInvestment, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkAnnualizedReturn, &QCheckBox::toggled, this, markModified);
    connect(ui->m_checkExtendedInternalRateOfReturn, &QCheckBox::toggled, this, markModified);
}

ReportTabRowColPerformance::~ReportTabRowColPerformance()
{
    delete ui;
}

bool ReportTabRowColPerformance::apply(MyMoneyReport* reportState)
{
    unsigned qc = reportState->queryColumns() & ~(eMyMoney::Report::performanceColumns() | eMyMoney::Report::QueryColumn::Performance);
    if (ui->m_checkStartingMarketValue->isVisible() && ui->m_checkStartingMarketValue->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceStartingMarketValue;
    if (ui->m_checkBuys->isVisible() && ui->m_checkBuys->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceBuys;
    if (ui->m_checkSells->isVisible() && ui->m_checkSells->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceSells;
    if (ui->m_checkReinvestIncome->isVisible() && ui->m_checkReinvestIncome->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceReinvestIncome;
    if (ui->m_checkCashIncome->isVisible() && ui->m_checkCashIncome->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceCashIncome;
    if (ui->m_checkEndingMarketValue->isVisible() && ui->m_checkEndingMarketValue->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceEndingMarketValue;
    if (ui->m_checkReturn->isVisible() && ui->m_checkReturn->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceReturn;
    if (ui->m_checkReturnInvestment->isVisible() && ui->m_checkReturnInvestment->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceReturnInvestment;
    if (ui->m_checkAnnualizedReturn->isVisible() && ui->m_checkAnnualizedReturn->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceAnnualizedReturn;
    if (ui->m_checkExtendedInternalRateOfReturn->isVisible() && ui->m_checkExtendedInternalRateOfReturn->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceExtendedInternalRateOfReturn;
    if (ui->m_checkNetCashFlow->isVisible() && ui->m_checkNetCashFlow->isChecked())
        qc |= eMyMoney::Report::QueryColumn::PerformanceNetCashFlow;

    qc = eMyMoney::Report::collapsePerformanceColumns(qc);

    reportState->setQueryColumns(static_cast<eMyMoney::Report::QueryColumn>(qc));
    m_isModified = false;
    return true;
}

bool ReportTabRowColPerformance::load(MyMoneyReport* report)
{
    m_isLoading = true;
    unsigned qc = report->queryColumns();
    // unset of default value
    qc = eMyMoney::Report::expandPerformanceColumns(qc);
    ui->m_checkStartingMarketValue->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceStartingMarketValue);
    ui->m_checkBuys->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceBuys);
    ui->m_checkSells->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceSells);
    ui->m_checkReinvestIncome->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceReinvestIncome);
    ui->m_checkCashIncome->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceCashIncome);
    ui->m_checkEndingMarketValue->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceEndingMarketValue);
    ui->m_checkReturn->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceReturn);
    ui->m_checkReturnInvestment->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceReturnInvestment);
    ui->m_checkAnnualizedReturn->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceAnnualizedReturn);
    ui->m_checkExtendedInternalRateOfReturn->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceExtendedInternalRateOfReturn);
    ui->m_checkNetCashFlow->setChecked(qc & eMyMoney::Report::QueryColumn::PerformanceNetCashFlow);
    slotUpdatePerformanceColumnVisibility(report->investmentSum());
    m_isModified = false;
    m_isLoading = false;
    return true;
}

bool ReportTabRowColPerformance::isModified() const
{
    return m_isModified;
}

void ReportTabRowColPerformance::slotUpdatePerformanceColumnVisibility(eMyMoney::Report::InvestmentSum sumType)
{
    const auto setColumnVisible = [](QCheckBox* checkBox, bool visible) {
        checkBox->setVisible(visible);
        if (!visible)
            checkBox->setChecked(false);
    };

    setColumnVisible(ui->m_checkStartingMarketValue, sumType == eMyMoney::Report::InvestmentSum::Period);
    setColumnVisible(ui->m_checkSells, sumType != eMyMoney::Report::InvestmentSum::Owned);
    setColumnVisible(ui->m_checkReinvestIncome, sumType != eMyMoney::Report::InvestmentSum::Sold);
    setColumnVisible(ui->m_checkEndingMarketValue, sumType != eMyMoney::Report::InvestmentSum::Sold);
}
