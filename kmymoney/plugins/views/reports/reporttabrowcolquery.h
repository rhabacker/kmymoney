#pragma once

#include "mymoneyreport.h"

#include <QWidget>

class ReportTabRowColQuery : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ReportTabRowColQuery)
public:
    explicit ReportTabRowColQuery(QWidget* parent = nullptr);
    ~ReportTabRowColQuery() override = default;

    void load(const MyMoneyReport& report);
    void save(MyMoneyReport& report) const;

    void enforcePriceColumn(bool mandatory, bool hasPrices);

private Q_SLOTS:
    void slotHideTransactionsChanged(bool checked);

private:
    class Ui_ReportTabRowColQuery* ui;
};
