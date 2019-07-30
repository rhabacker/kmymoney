#ifndef BUSINESSDLG_H
#define BUSINESSDLG_H

#include <QDialog>

namespace Ui { class BusinessDlg; }

class BusinessDlg : public QDialog
{
public:
    BusinessDlg();

private:
    Ui::BusinessDlg* ui;
};

#endif // BUSINESSDLG_H
