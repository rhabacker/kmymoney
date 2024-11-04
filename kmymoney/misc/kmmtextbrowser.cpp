/*
    SPDX-FileCopyrightText: 2021 Dawid Wróbel <me@dawidwrobel.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// ----------------------------------------------------------------------------
// QT Includes

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTextObject>

#include <private/qabstracttextdocumentlayout_p.h>
#include <private/qfont_p.h>
#include <private/qpagedpaintdevice_p.h>
#include <private/qtextdocument_p.h>

// ----------------------------------------------------------------------------
// Project Includes

#include "kmm_printer.h"
#include "kmmtextbrowser.h"

// included from QtGui sources 5.15.8
static void printPage(int index, QPainter* painter, const QTextDocument* doc, const QRectF& body, const QPointF& pageNumberPos)
{
    painter->save();
    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    QRectF view(0, (index - 1) * body.height(), body.width(), body.height());
    // painter->setPen(Qt::black);
    // painter->drawRect(body);

    QAbstractTextDocumentLayout* layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setClipRect(view);
    ctx.clip = view;

    // don't use the system palette text as default text color, on HP/UX
    // for example that's white, and white text on white paper doesn't
    // look that nice
    ctx.palette.setColor(QPalette::Text, Qt::black);

    layout->draw(painter, ctx);

    if (!pageNumberPos.isNull()) {
        painter->setClipping(false);
        painter->setFont(QFont(doc->defaultFont()));
        const QString pageString = QString::number(index);

        painter->drawText(qRound(pageNumberPos.x() - painter->fontMetrics().horizontalAdvance(pageString)), qRound(pageNumberPos.y() + view.top()), pageString);
        painter->drawText(body.left(), qRound(pageNumberPos.y() + view.top()), KMyMoneyPrinter::fileName().toDisplayString());
    }

    painter->restore();
}

class KMMTextDocument : public QTextDocument
{
public:
    void print(QPrinter* printer)
    {
        if (!printer)
            return;

        bool documentPaginated = pageSize().isValid() && !pageSize().isNull() && pageSize().height() != INT_MAX;

        // ### set page size to paginated size?
        QMarginsF m = printer->pageLayout().margins(QPageLayout::Millimeter);
        if (!documentPaginated && m.left() == 0 && m.right() == 0 && m.top() == 0 && m.bottom() == 0) {
            m.setLeft(2.);
            m.setRight(2.);
            m.setTop(2.);
            m.setBottom(2.);
            printer->setPageMargins(m, QPageLayout::Millimeter);
        }
        // ### use the margins correctly
        QPainter p(printer);

        // Check that there is a valid device to print to.
        if (!p.isActive())
            return;

        const QTextDocument* doc = this;
        QScopedPointer<QTextDocument> clonedDoc;
        (void)doc->documentLayout(); // make sure that there is a layout

        QRectF body = QRectF(QPointF(0, 0), pageSize());
        QPointF pageNumberPos;

        qreal sourceDpiX = qt_defaultDpiX();
        qreal sourceDpiY = qt_defaultDpiY();
        const qreal dpiScaleX = qreal(printer->logicalDpiX()) / sourceDpiX;
        const qreal dpiScaleY = qreal(printer->logicalDpiY()) / sourceDpiY;

        if (documentPaginated) {
            QPaintDevice* dev = doc->documentLayout()->paintDevice();
            if (dev) {
                sourceDpiX = dev->logicalDpiX();
                sourceDpiY = dev->logicalDpiY();
            }

            // scale to dpi
            p.scale(dpiScaleX, dpiScaleY);

            QSizeF scaledPageSize = pageSize();
            scaledPageSize.rwidth() *= dpiScaleX;
            scaledPageSize.rheight() *= dpiScaleY;

            const QSizeF printerPageSize(printer->width(), printer->height());

            // scale to page
            p.scale(printerPageSize.width() / scaledPageSize.width(), printerPageSize.height() / scaledPageSize.height());
        } else {
            doc = clone(const_cast<KMMTextDocument*>(this));
            clonedDoc.reset(const_cast<QTextDocument*>(doc));

            for (QTextBlock srcBlock = doc->firstBlock(), dstBlock = clonedDoc->firstBlock(); srcBlock.isValid() && dstBlock.isValid();
                 srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {
                dstBlock.layout()->setFormats(srcBlock.layout()->formats());
            }

            QAbstractTextDocumentLayout* layout = doc->documentLayout();
            layout->setPaintDevice(p.device());

            // copy the custom object handlers
            // TODO
            // layout->d_func()->handlers = documentLayout()->d_func()->handlers;

            // 2 cm margins, scaled to device in QTextDocumentLayoutPrivate::layoutFrame
            const int horizontalMargin = int((2 / 2.54) * sourceDpiX);
            const int verticalMargin = int((2 / 2.54) * sourceDpiY);
            QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
            fmt.setLeftMargin(horizontalMargin);
            fmt.setRightMargin(horizontalMargin);
            fmt.setTopMargin(verticalMargin);
            fmt.setBottomMargin(verticalMargin);
            doc->rootFrame()->setFrameFormat(fmt);

            // pageNumberPos must be in device coordinates, so scale to device here
            const int dpiy = p.device()->logicalDpiY();
            body = QRectF(0, 0, printer->width(), printer->height());
            pageNumberPos = QPointF(body.width() - horizontalMargin * dpiScaleX,
                                    body.height() - verticalMargin * dpiScaleY + QFontMetrics(doc->defaultFont(), p.device()).ascent() + 5 * dpiy / 72.0);
            clonedDoc->setPageSize(body.size());
        }
        int fromPage = printer->fromPage();
        int toPage = printer->toPage();

        if (fromPage == 0 && toPage == 0) {
            fromPage = 1;
            toPage = doc->pageCount();
        }
        // paranoia check
        fromPage = qMax(1, fromPage);
        toPage = qMin(doc->pageCount(), toPage);

        if (toPage < fromPage) {
            // if the user entered a page range outside the actual number
            // of printable pages, just return
            return;
        }

        int page = fromPage;
        while (true) {
            printPage(page, &p, doc, body, pageNumberPos);

            if (page == toPage)
                break;
            ++page;
            if (!printer->newPage())
                return;
        }
    }
};

KMMTextBrowser::KMMTextBrowser(QWidget* parent)
    : QTextBrowser(parent)
{
}
/*
 * This implementation can be simplified with the port to Qt 6.3 by using
 * QTextDocument::setMetaInformation(QTextDocument::CssMedia, "print"),
 * see https://doc.qt.io/qt-6/qtextdocument.html#MetaInformation-enum
 */
void KMMTextBrowser::print(QPrinter* printer)
{
    KMMTextDocument documentCopy;
    documentCopy.setDefaultStyleSheet("");
    documentCopy.setHtml(m_html);
    // Add space between columns
    for (QTextBlock it = documentCopy.begin(); it != documentCopy.end(); it = it.next()) {
        QTextCursor cursor(it);
        QTextBlockFormat tbf = it.blockFormat();
        tbf.setTextIndent(0.1);
        cursor.setBlockFormat(tbf);
    }
    documentCopy.print(printer);
}

void KMMTextBrowser::setHtml(const QString& text)
{
    m_html = text;
    m_html.replace("@media screen", "@media _screen").replace("@media print", "@media screen");
    if (m_content != text) {
        m_content = text;
        QTextBrowser::setHtml(m_content);
    }
}
