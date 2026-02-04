#include "kreportdateaxis.h"

// ----------------------------------------------------------------------------
// QT Includes
#include <QDateTime>

KReportDateAxis::KReportDateAxis(const QLocale& locale, KChart::AbstractCartesianDiagram* diagram)
    : CartesianAxis(diagram)
    , m_locale(locale)
{
}

void KReportDateAxis::setDateRange(const QDate& from, const QDate& to)
{
    m_from = from;
    m_to = to;
    updateGranularity();
}

void KReportDateAxis::updateGranularity()
{
    if (!m_from.isValid() || !m_to.isValid())
        return;

    const int days = m_from.daysTo(m_to);

    if (days > 3650) // > 10 years
        m_granularity = DateGranularity::Year;
    else if (days > 1100) // ~3–10 years
        m_granularity = DateGranularity::Quarter;
    else
        m_granularity = DateGranularity::Month;
}

KReportDateAxis::DateGranularity KReportDateAxis::granularity() const
{
    return m_granularity;
}

const QString KReportDateAxis::customizedLabel(const QString& label) const
{
    return CartesianAxis::customizedLabel(label);

    bool ok = false;
    const qreal value = label.toDouble(&ok);
    if (!ok)
        return label;

    // KChart date axes usually pass values as Julian days or seconds
    // This assumes "days since epoch" — adjust if needed
    const QDate date = QDate::fromJulianDay(static_cast<int>(value) + m_from.toJulianDay());
    if (!date.isValid())
        return label;

    switch (m_granularity) {
    case DateGranularity::Year:
        return QString::number(date.year());

    case DateGranularity::Quarter: {
        const int q = (date.month() - 1) / 3 + 1;
        return QStringLiteral("Q%1 %2").arg(q).arg(date.year());
    }

    case DateGranularity::Month:
        return m_locale.toString(date, QStringLiteral("MMM yyyy"));
    }

    return label;
}

double KReportDateAxis::majorStepWidth() const
{
    switch (m_granularity) {
    case DateGranularity::Year:
        return 365.0;
    case DateGranularity::Quarter:
        return 91.0;
    case DateGranularity::Month:
        return 30.0;
    }
    return 0.0;
}

double KReportDateAxis::minorStepWidth() const
{
    switch (m_granularity) {
    case DateGranularity::Year:
        return 0.0; // no subgrid
    case DateGranularity::Quarter:
        return 30.0; // months
    case DateGranularity::Month:
        return 7.0; // weeks (optional)
    }
    return 0.0;
}
