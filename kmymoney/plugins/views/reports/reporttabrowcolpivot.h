#pragma once

#include "mymoneyreport.h"

#include <QWidget>

class MyMoneyBudget;
class QCheckBox;

class ReportTabRowColPivot : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ReportTabRowColPivot)
public:
    explicit ReportTabRowColPivot(QWidget* parent = nullptr);
    ~ReportTabRowColPivot() override = default;

    void load(const MyMoneyReport& report, const QVector<MyMoneyBudget>& budgets);
    void save(MyMoneyReport& report, QVector<MyMoneyBudget>& budgets, bool isBudgetActual) const;

    bool isBudgetComboEnabled() const;
    void setTransfersEnabled(bool state);
    void setTransfersChecked(bool state);

Q_SIGNALS:
    void rowTypeChanged(int row);

private:
    class Ui_ReportTabRowColPivot* ui;

protected Q_SLOTS:
    void slotRowTypeChanged(int);
};
