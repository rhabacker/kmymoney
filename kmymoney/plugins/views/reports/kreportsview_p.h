/*
    SPDX-FileCopyrightText: 2000-2004 Michael Edwardes <mte@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 Javier Campos Morales <javi_c@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 Felix Rodriguez <frodriguez@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 John C <thetacoturtle@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 Thomas Baumgart <ipwizard@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 Kevin Tambascio <ktambascio@users.sourceforge.net>
    SPDX-FileCopyrightText: 2000-2004 Ace Jones <ace.j@hotpop.com>
    SPDX-FileCopyrightText: 2017 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
    SPDX-FileCopyrightText: 2018 Michael Kiefer <Michael-Kiefer@web.de>
    SPDX-FileCopyrightText: 2021 Dawid Wróbel <me@dawidwrobel.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KREPORTSVIEW_P_H
#define KREPORTSVIEW_P_H

#include "kreportsview.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QPrintPreviewDialog>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWheelEvent>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif

// ----------------------------------------------------------------------------
// KDE Includes

#include <KChartAbstractCoordinatePlane>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KMessageBox>
#include <QPainter>

// ----------------------------------------------------------------------------
// Project Includes

#include "ui_kreportsview.h"
#include "ui_reportcontrol.h"

#include "icons.h"
#include "kmm_codec.h"
#include "kmm_printer.h"
#include "kmmtextbrowser.h"
#include "kmymoneysettings.h"
#include "kmymoneyviewbase_p.h"
#include "kreportchartview.h"
#include "kreportconfigurationfilterdlg.h"
#include "mymoneyenums.h"
#include "mymoneyexception.h"
#include "mymoneyfile.h"
#include "mymoneyreport.h"
#include "objectinfotable.h"
#include "pivottable.h"
#include "querytable.h"
#include "reportcontrolimpl.h"
#include "reportgroup.h"
#include "reportsmodel.h"
#include "reporttable.h"
#include "tocitem.h"
#include "tocitemgroup.h"
#include "tocitemreport.h"

using namespace reports;
using namespace eMyMoney;
using namespace Icons;

#define VIEW_LEDGER         "ledger"
#define VIEW_SCHEDULE       "schedule"
#define VIEW_WELCOME        "welcome"
#define VIEW_HOME           "home"
#define VIEW_REPORTS        "reports"

/**
  * Helper class for KReportView.
  *
  * This is the widget which displays a single report in the TabWidget that comprises this view.
  *
  * @author Ace Jones
  */

class KReportTab: public QWidget
{
private:
    KMMTextBrowser* m_tableView;
    reports::KReportChartView *m_chartView;
    ReportControl             *m_control;
    QVBoxLayout               *m_layout;
    MyMoneyReport m_report;
    bool m_deleteMe;
    bool m_chartEnabled;
    bool m_showingChart;
    bool m_needReload;
    bool m_isChartViewValid;
    bool m_isTableViewValid;
    QPointer<reports::ReportTable> m_table;

    /**
     * Users character set encoding.
     */
    QByteArray m_encoding;

public:
    KReportTab(QTabWidget* parent, const MyMoneyReport& report, const KReportsView* eventHandler, KReportsView::OpenOption openOption);
    ~KReportTab();
    const MyMoneyReport& report() const {
        return m_report;
    }
    void print();
    void printPreview();
    void toggleChart();
    /**
     * Updates information about plotted chart in report's data
     */
    void updateDataRange();
    void copyToClipboard();
    void saveAs(const QString& filename, const QString& selectedMimeType);
    void updateReport();
    QString createTable(const QString& links = QString());
    const ReportControl* control() const
    {
        return m_control;
    }

    bool isReadyToDelete() const
    {
        return m_deleteMe;
    }

    void setReadyToDelete(bool f)
    {
        m_deleteMe = f;
    }

    void modifyReport(const MyMoneyReport& report)
    {
        m_report = report;
        updateReport();
    }

    void enableAllReportActions()
    {
        pActions[eMenu::Action::ReportNew]->setEnabled(true);
        pActions[eMenu::Action::ReportConfigure]->setEnabled(true);
        pActions[eMenu::Action::ReportExport]->setEnabled(true);
        pActions[eMenu::Action::ReportDelete]->setEnabled(true);
        pActions[eMenu::Action::ReportClose]->setEnabled(true);
    }

    void showEvent(QShowEvent * event) final override;
    void loadTab();

protected:
    void wheelEvent(QWheelEvent *event) override;

};

/**
  * KReportTab Implementation
  */
KReportTab::KReportTab(QTabWidget* parent, const MyMoneyReport& report, const KReportsView* eventHandler, KReportsView::OpenOption openOption)
    : QWidget(parent)
    , m_tableView(new KMMTextBrowser(this))
    , m_chartView(new KReportChartView(this))
    , m_control(new ReportControl(this))
    , m_layout(new QVBoxLayout(this))
    , m_report(report)
    , m_deleteMe(false)
    , m_chartEnabled(false)
    , m_showingChart(report.isChartByDefault())
    , m_needReload(openOption == KReportsView::OpenImmediately)
    , m_isChartViewValid(false)
    , m_isTableViewValid(false)
    , m_table(nullptr)
{
    m_layout->setSpacing(6);
    // TODO
    //    m_tableView->setZoomFactor(KMyMoneySettings::zoomFactor());

    // set button icons
    m_control->ui->buttonChart->setIcon(Icons::get(Icon::OfficeCharBar));
    m_control->ui->buttonConfigure->setIcon(Icons::get(Icon::Configure));
    m_control->ui->buttonDelete->setIcon(Icons::get(Icon::EditRemove));
    m_control->ui->buttonExport->setIcon(Icons::get(Icon::DocumentExport));
    m_control->ui->buttonNew->setIcon(Icons::get(Icon::DocumentNew));

    // and actions
    m_control->ui->buttonConfigure->setDefaultAction(pActions[eMenu::Action::ReportConfigure]);
    m_control->ui->buttonDelete->setDefaultAction(pActions[eMenu::Action::ReportDelete]);
    m_control->ui->buttonExport->setDefaultAction(pActions[eMenu::Action::ReportExport]);
    m_control->ui->buttonNew->setDefaultAction(pActions[eMenu::Action::ReportNew]);

    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartView->hide();
    m_tableView->hide();
    m_tableView->setOpenLinks(false);

    m_layout->addWidget(m_control);
    m_layout->addWidget(m_tableView);
    m_layout->addWidget(m_chartView);
    m_layout->setStretch(1, 10);
    m_layout->setStretch(2, 10);

    connect(m_control->ui->buttonChart, &QAbstractButton::clicked, eventHandler, &KReportsView::slotToggleChart);

    // setup style for all buttons to contain icon and text
    const auto buttons = m_control->findChildren<QToolButton*>();
    for (auto* button : qAsConst(buttons)) {
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    enableAllReportActions();

    connect(m_tableView, &KMMTextBrowser::anchorClicked, eventHandler, &KReportsView::slotOpenUrl);

    // if this is a default report, then you can't delete it!
    if (report.id().isEmpty())
        m_control->ui->buttonDelete->setEnabled(false);

    int tabNr = parent->addTab(this,
                               Icons::get(Icon::Report),
                               report.name());
    parent->setTabEnabled(tabNr, true);
    parent->setCurrentIndex(tabNr);

    // get users character set encoding
    m_encoding = KMM_Codec::encodingForLocale();
}

KReportTab::~KReportTab()
{
    delete m_table;
}


void KReportTab::wheelEvent(QWheelEvent* event)
{
    // Zoom text on Ctrl + Scroll
    if (event->modifiers() & Qt::CTRL) {
        if (!m_showingChart) {
            // TODO
            //            qreal factor = m_tableView->zoomFactor();
            if (event->angleDelta().y() > 0)
                //                factor += 0.1;
                //            else if (event->delta() < 0)
                //                factor -= 0.1;
                //            m_tableView->setZoomFactor(factor);
                event->accept();
            return;
        }
    }
}


void KReportTab::print()
{
    if (m_tableView) {
        auto printer = KMyMoneyPrinter::startPrint();
        if (printer != nullptr) {
            if (m_showingChart) {
                QPainter painter(printer);
                m_chartView->paint(&painter, painter.window());
                QFont font = painter.font();
                font.setPointSizeF(font.pointSizeF() * 0.8);
                painter.setFont(font);
                painter.drawText(0, 0, MyMoneyUtils::formatDate(QDate::currentDate()));

                /// @todo extract url from KMyMoneyApp
                QUrl file;
                if (file.isValid()) {
                    painter.drawText(0, painter.window().height(), file.toLocalFile());
                }
            } else {
                m_tableView->print(printer);
            }
        }
    }
}

void KReportTab::printPreview()
{
    if (m_showingChart) {
        if (m_chartView) {
            QPrintPreviewDialog dlg(KMyMoneyPrinter::instance(), m_chartView);
            connect(&dlg, &QPrintPreviewDialog::paintRequested, m_tableView, [&](QPrinter* printer) {
                QPainter painter(printer);
                m_chartView->paint(&painter, painter.window());
            });
            dlg.exec();
        }
    } else {
        if (m_tableView) {
            QPrintPreviewDialog dlg(KMyMoneyPrinter::instance(), m_tableView);
            connect(&dlg, &QPrintPreviewDialog::paintRequested, m_tableView, [&](QPrinter* printer) {
                m_tableView->print(printer);
            });
            dlg.exec();
        }
    }
}

void KReportTab::saveAs(const QString& filename, const QString& selectedMimeType)
{
    if (selectedMimeType == QStringLiteral("application/pdf")) {
        auto printer = KMyMoneyPDFPrinter::startPrint(m_report.name());
        if (printer != nullptr) {
            printer->setOutputFileName(filename);
            m_tableView->print(printer);
        }
        return;
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        if (selectedMimeType == QStringLiteral("text/csv")) {
            QTextStream(&file) << m_table->renderReport(QLatin1String("csv"), m_encoding, QString());
        } else if (selectedMimeType == QStringLiteral("text/html")) {
            QString table = m_table->renderReport(QLatin1String("html"), m_encoding, m_report.name());
            // remove the background information only needed by Qt
            QRegularExpression removeBackgroundExp(R"(\n\s+background: .+bg-texture.png.+fixed;)");
            table.replace(removeBackgroundExp, QString());
            QTextStream stream(&file);
            stream << table;
        } else if (selectedMimeType == QStringLiteral("application/xml")) {
            QTextStream(&file) << m_table->toXml();
        }
        file.close();
    }
}

void KReportTab::loadTab()
{
    m_needReload = true;
    if (isVisible()) {
        m_needReload = false;
        updateReport();
    }
}

void KReportTab::showEvent(QShowEvent * event)
{
    if (m_needReload) {
        m_needReload = false;
        updateReport();
    }
    QWidget::showEvent(event);
}

void KReportTab::updateReport()
{
    m_isChartViewValid = false;
    m_isTableViewValid = false;
    // reload the report from the engine. It might have
    // been changed by the user

    try {
        // Don't try to reload default reports from the engine
        if (!m_report.id().isEmpty())
            m_report = MyMoneyFile::instance()->report(m_report.id());
    } catch (const MyMoneyException &) {
    }

    delete m_table;
    m_table = nullptr;

    if (m_report.reportType() == eMyMoney::Report::ReportType::PivotTable) {
        m_table = new PivotTable(m_report);
        m_chartEnabled = true;
    } else if (m_report.reportType() == eMyMoney::Report::ReportType::QueryTable) {
        m_table = new QueryTable(m_report);
        m_chartEnabled = false;
    } else if (m_report.reportType() == eMyMoney::Report::ReportType::InfoTable) {
        m_table = new ObjectInfoTable(m_report);
        m_chartEnabled = false;
    }

    m_control->ui->buttonChart->setEnabled(m_chartEnabled);

    m_showingChart = !m_showingChart;
    toggleChart();
}

void KReportTab::toggleChart()
{
    // for now it will just SHOW the chart.  In the future it actually has to toggle it.

    if (m_showingChart) {
        if (!m_isTableViewValid) {
            m_tableView->setHtml(m_table->renderReport(QLatin1String("html"), m_encoding, m_report.name()));
        }
        m_isTableViewValid = true;
        m_tableView->show();
        m_chartView->hide();

        m_control->ui->buttonChart->setText(i18n("Chart"));
        m_control->ui->buttonChart->setToolTip(i18n("Show the chart version of this report"));
        m_control->ui->buttonChart->setIcon(Icons::get(Icon::OfficeCharBar));
    } else {
        if (!m_isChartViewValid)
            m_table->drawChart(*m_chartView);
        m_isChartViewValid = true;
        m_tableView->hide();
        m_chartView->show();

        m_control->ui->buttonChart->setText(i18n("Report"));
        m_control->ui->buttonChart->setToolTip(i18n("Show the report version of this chart"));
        m_control->ui->buttonChart->setIcon(Icons::get(Icon::Report));
    }
    m_showingChart = ! m_showingChart;
}

void KReportTab::updateDataRange()
{
    static const QRegularExpression trailingsZeroesRegEx("0+$");

    QList<DataDimension> grids = m_chartView->coordinatePlane()->gridDimensionsList();    // get dimensions of plotted graph
    if (grids.isEmpty())
        return;
    auto separator = locale().groupSeparator(); // QChar in Qt5, QString in Qt6
    auto decimalPoint = locale().decimalPoint(); // QChar in Qt5, QString in Qt6
    int precision = m_report.yLabelsPrecision();
    QList<QPair<QString, qreal>> dims;  // create list of dimension values in string and qreal

    // get qreal values
    dims.append(qMakePair(QString(), grids.at(1).start));
    dims.append(qMakePair(QString(), grids.at(1).end));
    dims.append(qMakePair(QString(), grids.at(1).stepWidth));
    dims.append(qMakePair(QString(), grids.at(1).subStepWidth));

    // convert qreal values to string variables
    for (int i = 0; i < 4; ++i) {
        if (i > 2)
            ++precision;
        if (precision == 0)
            dims[i].first = locale().toString(qRound(dims.at(i).second));
        else
            dims[i].first = locale()
                                .toString(dims.at(i).second, 'f', precision)
                                .remove(separator)
                                .remove(trailingsZeroesRegEx)
                                .remove(QRegularExpression("\\" + decimalPoint + "$"));
    }

    // save string variables in report's data
    m_report.setDataRangeStart(dims.at(0).first);
    m_report.setDataRangeEnd(dims.at(1).first);
    m_report.setDataMajorTick(dims.at(2).first);
    m_report.setDataMinorTick(dims.at(3).first);
}

class NaturalSortProxy : public QSortFilterProxyModel
{
public:
    explicit NaturalSortProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        QString l = sourceModel()->data(left).toString();
        QString r = sourceModel()->data(right).toString();

        static QRegularExpression re(R"(^(\d+))");
        auto ml = re.match(l);
        auto mr = re.match(r);

        if (ml.hasMatch() && mr.hasMatch()) {
            return ml.captured(1).toInt() < mr.captured(1).toInt();
        }
        return QString::localeAwareCompare(l, r) < 0;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

        // 1. Direct match in any column
        const int columns = sourceModel()->columnCount(index);
        for (int col = 0; col < columns; ++col) {
            const QModelIndex idx = sourceModel()->index(sourceRow, col, sourceParent);
            if (sourceModel()->data(idx).toString().contains(filterRegularExpression()))
                return true;
        }

        // 2. Accept parent if any child matches
        const int rows = sourceModel()->rowCount(index);
        for (int i = 0; i < rows; ++i) {
            if (filterAcceptsRow(i, index))
                return true;
        }

        return false;
    }
};

class KReportsViewPrivate : public KMyMoneyViewBasePrivate
{
    Q_DECLARE_PUBLIC(KReportsView)

public:
    explicit KReportsViewPrivate(KReportsView *qq)
        : KMyMoneyViewBasePrivate(qq)
        , m_needLoad(true)
        , m_reportListView(nullptr)
        , m_columnsAlreadyAdjusted(false)
    {
    }

    ~KReportsViewPrivate()
    {
        // delete m_defaultReports;
    }

    void setupView(QTreeView* view, ReportsModel* model)
    {
        Q_Q(KReportsView);
        auto proxyModel = new NaturalSortProxy(view->parent());
        proxyModel->setSourceModel(model);
        // proxyModel->setRecursiveSortingEnabled(true); // Qt 5.10+
        proxyModel->setDynamicSortFilter(true);
        view->setModel(proxyModel);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        // multiple selections
        view->setSelectionMode(QAbstractItemView::ExtendedSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setContextMenuPolicy(Qt::CustomContextMenu);

        // sorting
        view->setSortingEnabled(true);
        view->sortByColumn(0, Qt::AscendingOrder);
        view->header()->setSectionsClickable(true);
        view->header()->setSortIndicatorShown(true);
        view->header()->setSectionResizeMode(ReportsModel::Columns::ReportName, QHeaderView::ResizeToContents);
        view->header()->setStretchLastSection(true);

        q->connect(view, &QTreeView::doubleClicked, q, &KReportsView::slotDoubleClicked);
        q->connect(view, &QWidget::customContextMenuRequested, q, &KReportsView::slotContextMenu);

        view->installEventFilter(q);
        view->viewport()->installEventFilter(q);
    }

    void init()
    {
        Q_Q(KReportsView);
        m_needLoad = false;
        m_needsRefresh = true;

        // build reports toc

        setColumnsAlreadyAdjusted(false);
        ui.setupUi(q);

        setupView(ui.m_tocTreeViewCustom, MyMoneyFile::instance()->reportsModel());

        QList<ReportGroup> defaultGroups;
        defaultReports(defaultGroups);
        m_builtInReports = new ReportsModel();
        m_builtInReports->load(defaultGroups);
        setupView(ui.m_tocTreeViewDefault, m_builtInReports);

        ui.m_closeButton->setIcon(Icons::get(Icon::DialogClose));
        ui.m_filterContainer->hide();
        ui.m_searchWidget->installEventFilter(q);

        q->connect(ui.m_reportTabWidget, &QTabWidget::tabCloseRequested, q, &KReportsView::slotClose);

        m_focusWidget = ui.m_tocTreeViewDefault;
    }

    QMap<QString, bool> saveTocExpandState(QTreeView* view) const
    {
        QMap<QString, bool> expandStates;

        auto* model = view->model();
        if (!model)
            return expandStates;

        const int rows = model->rowCount();
        for (int row = 0; row < rows; ++row) {
            const QModelIndex idx = model->index(row, 0);
            const QString label = model->data(idx, Qt::DisplayRole).toString();
            expandStates.insert(label, view->isExpanded(idx));
        }

        return expandStates;
    }

    void restoreTocExpandState(const QMap<QString, bool>& expandStates, QTreeView* view)
    {
        auto* model = view->model();
        if (!model)
            return;

        const int rows = model->rowCount();
        for (int row = 0; row < rows; ++row) {
            const QModelIndex idx = model->index(row, 0);
            const QString label = model->data(idx, Qt::DisplayRole).toString();

            const bool expanded = expandStates.value(label, false);
            view->setExpanded(idx, expanded);
        }
    }

    /**
      * Display a dialog to confirm report deletion
      */
    int deleteReportDialog(const QString &reportName)
    {
        Q_Q(KReportsView);
        return KMessageBox::warningContinueCancel(q,
                i18n("<qt>Are you sure you want to delete report <b>%1</b>?  There is no way to recover it.</qt>",
                     reportName), i18n("Delete Report?"));
    }

    void addReportTab(const MyMoneyReport& report, KReportsView::OpenOption openOption)
    {
        Q_Q(KReportsView);
        auto reportTab = new KReportTab(ui.m_reportTabWidget, report, q, openOption);
        reportTab->installEventFilter(q);
    }

    void defaultReports(QList<ReportGroup>& groups)
    {
        {
            ReportGroup list("Income and Expenses", i18n("Income and Expenses"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentMonth,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses This Month"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses This Year"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Years),
                                         TransactionFilter::Date::All,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses By Year"),
                                         i18n("Default Report")));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setChartDataLabels(false);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses Bar Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartType(eMyMoney::Report::ChartType::StackedBar);
            list.back().setChartDataLabels(false);
            list.back().setNegExpenses(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::Group,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses Pie Chart"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartType(eMyMoney::Report::ChartType::Pie);
            list.back().setShowingRowTotals(false);

            groups.push_back(list);
        }
        {
            ReportGroup list("Net Worth", i18n("Net Worth"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Net Worth By Month"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Today,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Net Worth Today"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Years),
                                         TransactionFilter::Date::All,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Net Worth By Year"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Next7Days,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("7-day Cash Flow Forecast"),
                                         i18n("Default Report")));
            list.back().setIncludingSchedules(true);
            list.back().setColumnsAreDays(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::Total,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Net Worth Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Institution,
                                         eMyMoney::Report::QueryColumn::None,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Account Balances by Institution"),
                                         i18n("Default Report")));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountType,
                                         eMyMoney::Report::QueryColumn::None,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Account Balances by Type"),
                                         i18n("Default Report")));

            groups.push_back(list);
        }
        {
            ReportGroup list("Transactions", i18n("Transactions"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Account,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Category
                                             | eMyMoney::Report::QueryColumn::Tag | eMyMoney::Report::QueryColumn::Balance,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Account"),
                                         i18n("Default Report")));
            // list.back().setConvertCurrency(false);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Category,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Account
                                             | eMyMoney::Report::QueryColumn::Tag,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Category"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Payee,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Category | eMyMoney::Report::QueryColumn::Tag,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Payee"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Tag,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Category,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Tag"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Month,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Category
                                             | eMyMoney::Report::QueryColumn::Tag,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Month"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Week,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Category
                                             | eMyMoney::Report::QueryColumn::Tag,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Week"),
                                         i18n("Default Report")));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Account,
                                         eMyMoney::Report::QueryColumn::Loan,
                                         TransactionFilter::Date::All,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Loan Transactions"),
                                         i18n("Default Report")));
            list.back().setLoansOnly(true);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountReconcile,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Category
                                             | eMyMoney::Report::QueryColumn::Balance,
                                         TransactionFilter::Date::Last3Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Transactions by Reconciliation Status"),
                                         i18n("Default Report")));
            groups.push_back(list);
        }
        {
            ReportGroup list("CashFlow", i18n("Cash Flow"));
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::CashFlow,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Account,
                                         TransactionFilter::Date::CurrentMonth,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Cash Flow Transactions This Month"),
                                         i18n("Default Report")));
            groups.push_back(list);
        }
        {
            ReportGroup list("Investments", i18n("Investments"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::TopAccount,
                                         eMyMoney::Report::QueryColumn::Action | eMyMoney::Report::QueryColumn::Shares | eMyMoney::Report::QueryColumn::Price,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Transactions"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountByTopAccount,
                                         eMyMoney::Report::QueryColumn::Shares | eMyMoney::Report::QueryColumn::Price,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Holdings by Account"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::EquityType,
                                         eMyMoney::Report::QueryColumn::Shares | eMyMoney::Report::QueryColumn::Price,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Holdings by Type"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountByTopAccount,
                                         eMyMoney::Report::QueryColumn::Performance,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Performance by Account"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::EquityType,
                                         eMyMoney::Report::QueryColumn::Performance,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Performance by Type"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountByTopAccount,
                                         eMyMoney::Report::QueryColumn::CapitalGain,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Capital Gains by Account"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::EquityType,
                                         eMyMoney::Report::QueryColumn::CapitalGain,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Capital Gains by Type"),
                                         i18n("Default Report")));
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Today,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Holdings Pie"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Pie);
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Worth Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setColumnsAreDays(true);
            list.back().setInvestmentsOnly(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Price Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setColumnsAreDays(true);
            list.back().setInvestmentsOnly(true);
            list.back().setIncludingBudgetActuals(false);
            list.back().setIncludingPrice(true);
            list.back().setConvertCurrency(true);
            list.back().setChartDataLabels(false);
            list.back().setSkipZero(true);
            list.back().setShowingColumnTotals(false);
            list.back().setShowingRowTotals(false);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last12Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Moving Average Price Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setColumnsAreDays(true);
            list.back().setInvestmentsOnly(true);
            list.back().setIncludingBudgetActuals(false);
            list.back().setIncludingAveragePrice(true);
            list.back().setMovingAverageDays(10);
            list.back().setConvertCurrency(true);
            list.back().setChartDataLabels(false);
            list.back().setShowingColumnTotals(false);
            list.back().setShowingRowTotals(false);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last30Days,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Moving Average"),
                                         i18n("Default Report")));
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setColumnsAreDays(true);
            list.back().setInvestmentsOnly(true);
            list.back().setIncludingBudgetActuals(false);
            list.back().setIncludingMovingAverage(true);
            list.back().setMovingAverageDays(10);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Last30Days,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Investment Moving Average vs Actual"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            list.back().setColumnsAreDays(true);
            list.back().setInvestmentsOnly(true);
            list.back().setIncludingBudgetActuals(true);
            list.back().setIncludingMovingAverage(true);
            list.back().setMovingAverageDays(10);
            groups.push_back(list);
        }
        {
            ReportGroup list("Taxes", i18n("Taxes"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Category,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Account,
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Tax Transactions by Category"),
                                         i18n("Default Report")));
            list.back().setTax(true);
            list.push_back(
                MyMoneyReport(eMyMoney::Report::RowType::Payee,
                              eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Category | eMyMoney::Report::QueryColumn::Account,
                              TransactionFilter::Date::YearToDate,
                              eMyMoney::Report::DetailLevel::All,
                              eMyMoney::Report::Origin::BuiltIn,
                              i18n("Tax Transactions by Payee"),
                              i18n("Default Report")));
            list.back().setTax(true);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Category,
                                         eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Account,
                                         TransactionFilter::Date::LastFiscalYear,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Tax Transactions by Category Last Fiscal Year"),
                                         i18n("Default Report")));
            list.back().setTax(true);
            list.push_back(
                MyMoneyReport(eMyMoney::Report::RowType::Payee,
                              eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Category | eMyMoney::Report::QueryColumn::Account,
                              TransactionFilter::Date::LastFiscalYear,
                              eMyMoney::Report::DetailLevel::All,
                              eMyMoney::Report::Origin::BuiltIn,
                              i18n("Tax Transactions by Payee Last Fiscal Year"),
                              i18n("Default Report")));
            list.back().setTax(true);
            groups.push_back(list);
        }
        {
            ReportGroup list("Budgeting", i18n("Budgeting"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::BudgetActual,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::YearToDate,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Budgeted vs. Actual This Year"),
                                         i18n("Default Report")));
            list.back().setShowingRowTotals(true);
            list.back().setBudget("Any", true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::BudgetActual,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::YearToMonth,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Budgeted vs. Actual This Year (YTM)"),
                                         i18n("Default Report")));
            list.back().setShowingRowTotals(true);
            list.back().setBudget("Any", true);
            // in case we're in January, we show the last year
            if (QDate::currentDate().month() == 1) {
                list.back().setDateFilter(TransactionFilter::Date::LastYear);
            }

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::BudgetActual,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentMonth,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Monthly Budgeted vs. Actual"),
                                         i18n("Default Report")));
            list.back().setBudget("Any", true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::BudgetActual,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentFiscalYear,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Yearly Budgeted vs. Actual"),
                                         i18n("Default Report")));
            list.back().setBudget("Any", true);
            list.back().setShowingRowTotals(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Budget,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentMonth,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Monthly Budget"),
                                         i18n("Default Report")));
            list.back().setBudget("Any", false);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Budget,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentFiscalYear,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Yearly Budget"),
                                         i18n("Default Report")));
            list.back().setBudget("Any", false);
            list.back().setShowingRowTotals(true);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::BudgetActual,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentFiscalYear,
                                         eMyMoney::Report::DetailLevel::Group,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Yearly Budgeted vs Actual Graph"),
                                         i18n("Default Report")));
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setBudget("Any", true);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);

            groups.push_back(list);
        }
        {
            ReportGroup list("Forecast", i18n("Forecast"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Next12Months,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Forecast By Month"),
                                         i18n("Default Report")));
            list.back().setIncludingForecast(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::NextQuarter,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Forecast Next Quarter"),
                                         i18n("Default Report")));
            list.back().setColumnsAreDays(true);
            list.back().setIncludingForecast(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::ExpenseIncome,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::CurrentYear,
                                         eMyMoney::Report::DetailLevel::Top,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Income and Expenses Forecast This Year"),
                                         i18n("Default Report")));
            list.back().setIncludingForecast(true);

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AssetLiability,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Next3Months,
                                         eMyMoney::Report::DetailLevel::Total,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Net Worth Forecast Graph"),
                                         i18n("Default Report")));
            list.back().setColumnsAreDays(true);
            list.back().setIncludingForecast(true);
            list.back().setChartByDefault(true);
            list.back().setChartCHGridLines(false);
            list.back().setChartSVGridLines(false);
            list.back().setChartType(eMyMoney::Report::ChartType::Line);
            groups.push_back(list);
        }
        {
            ReportGroup list("Information", i18n("General Information"));

            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Schedule,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Next12Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Schedule Information"),
                                         i18n("Default Report")));
            list.back().setDetailLevel(eMyMoney::Report::DetailLevel::All);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::Schedule,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Next12Months,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Schedule Summary Information"),
                                         i18n("Default Report")));
            list.back().setDetailLevel(eMyMoney::Report::DetailLevel::Top);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountInfo,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Today,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Account Information"),
                                         i18n("Default Report")));
            list.back().setConvertCurrency(false);
            list.push_back(MyMoneyReport(eMyMoney::Report::RowType::AccountLoanInfo,
                                         static_cast<unsigned>(eMyMoney::Report::ColumnType::Months),
                                         TransactionFilter::Date::Today,
                                         eMyMoney::Report::DetailLevel::All,
                                         eMyMoney::Report::Origin::BuiltIn,
                                         i18n("Loan Information"),
                                         i18n("Default Report")));
            list.back().setConvertCurrency(false);
            groups.push_back(list);
        }
        {
            ReportGroup list("Charts", i18n("Charts"));
            for (const auto& group : groups) {
                for (const auto& report : group) {
                    if (report.isChartByDefault()) {
                        list.append(report);
                    }
                }
            }
            groups.push_back(list);
        }

        // In each report setup associated group, which is used in report configuration post steps
        for (auto& group : groups) {
            for (auto& report : group) {
                report.setGroup(group.name());
            }
        }
    }

    bool columnsAlreadyAdjusted() const
    {
        return m_columnsAlreadyAdjusted;
    }

    void setColumnsAlreadyAdjusted(bool adjusted)
    {
        m_columnsAlreadyAdjusted = adjusted;
    }

    void setFilter(const QString& text, QTreeView* view)
    {
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (text.isEmpty()) {
            proxy->setFilterRegularExpression(QRegularExpression());
            restoreTocExpandState(expandStatesBeforeSearch, view);
            expandStatesBeforeSearch.clear();
        } else {
            if (expandStatesBeforeSearch.isEmpty())
                expandStatesBeforeSearch = saveTocExpandState(view);

            proxy->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));

            // Expand visible parents (like your old code)
            for (int row = 0; row < proxy->rowCount(); ++row) {
                view->setExpanded(proxy->index(row, 0), true);
            }
        }
    }

    void setFilter(const QString& text)
    {
        setFilter(text, ui.m_tocTreeViewDefault);
        setFilter(text, ui.m_tocTreeViewCustom);
    }

    // Generate a transaction report that contains transactions for only the
    // currently selected account.
    void showTransactionReport()
    {
        Q_Q(KReportsView);
        if (!m_currentAccount.id().isEmpty()) {
            MyMoneyReport report(eMyMoney::Report::RowType::Account,
                                 eMyMoney::Report::QueryColumn::Number | eMyMoney::Report::QueryColumn::Payee | eMyMoney::Report::QueryColumn::Category,
                                 eMyMoney::TransactionFilter::Date::YearToDate,
                                 eMyMoney::Report::DetailLevel::All,
                                 eMyMoney::Report::Origin::Generated,
                                 i18n("%1 YTD Account Transactions", m_currentAccount.name()),
                                 i18n("Generated Report"));
            report.setGroup(i18n("Transactions"));
            report.addAccount(m_currentAccount.id());
            q->slotOpenReport(report);
        }
    }

    /**
      * This member holds the load state of page
      */
    bool m_needLoad;

    Ui::KReportsView ui;
    QListWidget* m_reportListView;
    QMap<QString, TocItemGroup*> m_allTocItemGroups;
    QString m_selectedExportFilter;

    bool m_columnsAlreadyAdjusted;
    MyMoneyAccount m_currentAccount;
    QMap<QString, bool> expandStatesBeforeSearch;
    ReportsModel* m_builtInReports;
};

#endif
