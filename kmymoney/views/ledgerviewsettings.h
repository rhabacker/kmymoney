/*
    SPDX-FileCopyrightText: 2021 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LEDGERVIEWSETTINGS_H
#define LEDGERVIEWSETTINGS_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QObject>
#include <QScopedPointer>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

class LedgerViewSettingsPrivate;
class LedgerViewSettings : public QObject
{
    Q_OBJECT

public:
    static LedgerViewSettings* instance();
    ~LedgerViewSettings();

    bool showLedgerLens() const;
    bool showTransactionDetails() const;
    bool showAllSplits() const;
    bool hideReconciledTransactions() const;
    QDate hideTransactionsBefore() const;

public Q_SLOTS:
    void setShowLedgerLens(bool show);
    void setShowTransactionDetails(bool show);
    void setShowAllSplits(bool show);

    /**
     * This method changes the global filter for
     * all instances of LedgerFilterBase and derived objects to
     * filter out any transaction with a post date prior to
     * @a date. If @a date is an invalid QDate, then the
     * filter is inactive.
     */
    void setHideTransactionsBefore(const QDate& date);

    /**
     * This method changes the global filter for
     * all instances of LedgerFilterBase and derived objects to
     * filter out any reconciled transaction.
     */
    void setHideReconciledTransactions(bool hide);

Q_SIGNALS:
    void settingsChanged();

private:
    LedgerViewSettings();
    LedgerViewSettingsPrivate* const d;

    Q_DISABLE_COPY(LedgerViewSettings)
};

#endif // LEDGERVIEWSETTINGS_H
