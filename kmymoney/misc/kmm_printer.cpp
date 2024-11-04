/*
    SPDX-FileCopyrightText: 2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmm_printer.h"

#include <QPointer>
#include <QPrintDialog>
#include <QPrinter>
#include <QScopedPointer>
#include <QUrl>

// Q_LOGGING_CATEGORY(Print, "Printing")

static QUrl s_fileName;

KMyMoneyPrinter::KMyMoneyPrinter()
{
}

QPrinter* KMyMoneyPrinter::instance(QPrinter::PrinterMode mode)
{
    static QPrinter* printer(nullptr);

    if (printer == nullptr) {
        printer = new QPrinter(mode);
    }
    return printer;
}

QPrintDialog* KMyMoneyPrinter::dialog()
{
    static QPrintDialog* dialog(nullptr);

    if (dialog == nullptr) {
        dialog = new QPrintDialog(instance());
        dialog->setWindowTitle(QString());
    }
    return dialog;
}

QPrinter* KMyMoneyPrinter::startPrint(QPrinter::PrinterMode mode)
{
    QPrinter *printer = instance(mode);

    if (dialog()->exec() != QDialog::Accepted)
        return nullptr;
    return printer;
}

void KMyMoneyPrinter::cleanup()
{
    auto printer = instance();
    auto dlg = dialog();

    delete dlg;
    delete printer;
}

QUrl KMyMoneyPrinter::fileName()
{
    return s_fileName;
}

void KMyMoneyPrinter::setFileName(const QUrl& url)
{
    s_fileName = url;
}
