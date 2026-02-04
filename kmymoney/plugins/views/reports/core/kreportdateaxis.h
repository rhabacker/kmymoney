#pragma once

// ----------------------------------------------------------------------------
// QT Includes
#include <QDate>
#include <QLocale>

// ----------------------------------------------------------------------------
// KDE / KChart Includes
#include <KChartCartesianAxis>

class KReportDateAxis : public KChart::CartesianAxis
{
public:
    explicit KReportDateAxis(const QLocale& locale, KChart::AbstractCartesianDiagram* diagram = nullptr);

    enum class DateGranularity { Year, Quarter, Month };

    void setDateRange(const QDate& from, const QDate& to);

    DateGranularity granularity() const;

    // Grid hints (used by the coordinate plane)
    double majorStepWidth() const;
    double minorStepWidth() const;

protected:
    const QString customizedLabel(const QString& label) const override;

private:
    void updateGranularity();

    QLocale m_locale;
    QDate m_from;
    QDate m_to;
    DateGranularity m_granularity = DateGranularity::Year;
};
