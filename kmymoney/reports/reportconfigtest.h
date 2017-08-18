#ifndef REPORTCONFIGTEST_H
#define REPORTCONFIGTEST_H

#include <QObject>
class MyMoneyFile;

class ReportConfigTest : public QObject
{
    Q_OBJECT
public:
    MyMoneyFile *m;

    explicit ReportConfigTest(QObject *parent = nullptr);

    void init();

public slots:
};

#endif // REPORTCONFIGTEST_H
