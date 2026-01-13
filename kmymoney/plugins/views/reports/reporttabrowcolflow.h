#pragma once

#include "mymoneyreport.h"

#include <QWidget>

class ReportTabRowColFlow : public QWidget
{
    Q_OBJECT
public:
    explicit ReportTabRowColFlow(QWidget* parent = nullptr);

    void load(const MyMoneyReport& report);
    void save(MyMoneyReport& report) const;

private:
    class Ui_ReportTabRowColFlow* ui;
};
