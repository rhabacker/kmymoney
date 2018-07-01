/***************************************************************************
                          reporttable.h
                             -------------------
    begin                : Mon May  7 2007
    copyright            : (C) 2007 Thomas Baumgart
    email                : Thomas Baumgart <ipwizard@users.sourceforge.net>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef REPORTTABLE_H
#define REPORTTABLE_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QFile>

// ----------------------------------------------------------------------------
// KDE Includes

#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>

// ----------------------------------------------------------------------------
// Project Includes

#include "kmymoneyutils.h"
#include "mymoneyfile.h"
#include "mymoneyreport.h"

namespace reports
{

class KReportChartView;

/**
  * This class serves as base class definition for the concrete report classes
  * This class is abstract but it contains common code used by all children classes
  */
class ReportTable
{
private:

  /**
   * Tries to find a css file for the report.
   *
   * Search is done in following order:
   * <ol>
   *  <li> report specific stylesheet
   *  <li> configured stylesheet
   *  <li> installation default of stylesheet
   * </ol>
   *
   * @retval css-filename  if a css-file was found
   * @retval empty-string  if no css-file was found
   */
  QString cssFileNameGet();

  /**
   * Name of application resource type.
   *
   * @see KGlobal::dirs()->findResource()
   */
  const char* m_resourceType;

  /**
   * Subdirectory for html-resources of application.
   *
   * @see KGlobal::dirs()->findResource()
   */
  QString m_resourceHtml;

  /**
   * Notation of @c reportstylesheet as used by:
   * @code
   *  MyMoneyFile::instance()::value();
   * @endcode
  */
  QString m_reportStyleSheet;

  /**
   * Filename of default css file.
   */
  QString m_cssFileDefault;

  /**
   * Character set encoding for the report.
   */
  QByteArray m_encoding;

protected:
  ReportTable();

  /**
   * Constructs html header.
   *
   * @param title html title of report
   * @param[in] includeCSS  flag, whether the generated html has to include the css inline or whether
   * the css is referenced as a link to a file
   * @return  html header
   */
  QString renderHeader(const QString& title, bool includeCSS);

  /**
   * Constructs html footer.
   *
   * @return  html footer
   */
  QString renderFooter();

  /**
   * Constructs the body of the report. Implemented by the concrete classes
   * @see PivotTable
   * @see ListTable
   * @return QString with the html body of the report
   */
  virtual QString renderBody() const = 0;

public:
  virtual ~ReportTable() {}

  /**
   * Constructs a comma separated-file of the report. Implemented by the concrete classes
   * @see PivotTable
   * @see ListTable
   */
  virtual QString renderCSV() const = 0;

  /**
   * Renders a graph from the report. Implemented by the concrete classes
   * @see PivotTable
   */
  virtual void drawChart(KReportChartView& view) const = 0;
  virtual void dump(const QString& file, const QString& context = QString()) const = 0;

  /**
   * Creates the complete html document.
   *
   * @param widget      parent widget
   * @param encoding    character set encoding
   * @param title       html title of report
   * @param includeCSS  flag, whether the generated html has
   *                        to include the css inline or whether
   *                        the css is referenced as a link to a file
   *
   * @return complete html document
   */
  QString renderHTML(QWidget* widget, const QByteArray& encoding,
                     const QString& title, bool includeCSS = false);
};

}

KMM_MYMONEY_EXPORT QDebug operator<<(QDebug dbg, const reports::ReportTable& a);
#endif
// REPORTTABLE_H
