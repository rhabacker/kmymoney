/***************************************************************************
                          ksettingsonlinequotes.cpp
                             -------------------
    begin                : Thu Dec 30 2004
    copyright            : (C) 2004 by Thomas Baumgart
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

#include "ksettingsonlinequotes.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QRegExp>
#include <QCheckBox>
#include <QDesktopServices>

// ----------------------------------------------------------------------------
// KDE Includes

#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kguiitem.h>
#include <KMessageBox>

// ----------------------------------------------------------------------------
// Project Includes

#include "kmymoney/converter/webpricequote.h"

KSettingsOnlineQuotes::KSettingsOnlineQuotes(QWidget *parent)
    : KSettingsOnlineQuotesDecl(parent),
    m_quoteInEditing(false)
{
  QStringList groups = WebPriceQuote::quoteSources();

  loadList(true /*updateResetList*/);

  m_updateButton->setEnabled(false);

  KGuiItem updateButtenItem(i18nc("Accepts the entered data and stores it", "&Accept"),
                            KIcon("dialog-ok"),
                            i18n("Accepts the entered data and stores it"),
                            i18n("Use this to accept the modified data."));
  m_updateButton->setGuiItem(updateButtenItem);

  KGuiItem deleteButtenItem(i18n("&Delete"),
                            KIcon("edit-delete"),
                            i18n("Delete the selected source entry"),
                            i18n("Use this to delete the selected online source entry"));
  m_deleteButton->setGuiItem(deleteButtenItem);

  KGuiItem checkButtonItem(i18nc("Check the selected source entry", "&Check Source"),
                            KIcon("document-edit-verify"),
                            i18n("Check the selected source entry"),
                            i18n("Use this to check the selected online source entry"));
  m_checkButton->setGuiItem(checkButtonItem);

  KGuiItem showButtonItem(i18nc("Show the selected source entry in a web browser", "&Show page"),
                            KIcon("applications-internet"),
                            i18n("Show the selected source entry in a web browser"),
                            i18n("Use this to show the selected online source entry"));
  m_showButton->setGuiItem(showButtonItem);

  KGuiItem newButtenItem(i18nc("Create a new source entry for online quotes", "&New..."),
                         KIcon("document-new"),
                         i18n("Create a new source entry for online quotes"),
                         i18n("Use this to create a new entry for online quotes"));
  m_newButton->setGuiItem(newButtenItem);

  connect(m_updateButton, SIGNAL(clicked()), this, SLOT(slotUpdateEntry()));
  connect(m_newButton, SIGNAL(clicked()), this, SLOT(slotNewEntry()));
  connect(m_checkButton, SIGNAL(clicked()), this, SLOT(slotCheckEntry()));
  connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteEntry()));
  connect(m_showButton, SIGNAL(clicked()), this, SLOT(slotShowEntry()));

  connect(m_quoteSourceList, SIGNAL(itemSelectionChanged()), this, SLOT(slotLoadWidgets()));
  connect(m_quoteSourceList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slotEntryRenamed(QListWidgetItem*)));
  connect(m_quoteSourceList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotStartRename(QListWidgetItem*)));

  connect(m_editURL, SIGNAL(textChanged(QString)), this, SLOT(slotEntryChanged()));
  connect(m_editSymbol, SIGNAL(textChanged(QString)), this, SLOT(slotEntryChanged()));
  connect(m_editDate, SIGNAL(textChanged(QString)), this, SLOT(slotEntryChanged()));
  connect(m_editDateFormat, SIGNAL(textChanged(QString)), this, SLOT(slotEntryChanged()));
  connect(m_editPrice, SIGNAL(textChanged(QString)), this, SLOT(slotEntryChanged()));
  connect(m_skipStripping, SIGNAL(toggled(bool)), this, SLOT(slotEntryChanged()));

  m_logWindow->setVisible(false);
}

void KSettingsOnlineQuotes::loadList(const bool updateResetList)
{
  //disconnect the slot while items are being loaded and reconnect at the end
  disconnect(m_quoteSourceList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slotEntryRenamed(QListWidgetItem*)));
  m_quoteInEditing = false;
  QStringList groups = WebPriceQuote::quoteSources();

  if (updateResetList)
    m_resetList.clear();
  m_quoteSourceList->clear();
  QStringList::Iterator it;
  for (it = groups.begin(); it != groups.end(); ++it) {
    QListWidgetItem* item = new QListWidgetItem(*it);
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_quoteSourceList->addItem(item);
    if (updateResetList)
      m_resetList += WebPriceQuoteSource(*it);
  }
  m_quoteSourceList->sortItems();

  QListWidgetItem* first = m_quoteSourceList->item(0);
  if (first)
    m_quoteSourceList->setCurrentItem(first);
  slotLoadWidgets();

  m_newButton->setEnabled((m_quoteSourceList->findItems(i18n("New Quote Source"), Qt::MatchExactly)).count() == 0);
  connect(m_quoteSourceList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slotEntryRenamed(QListWidgetItem*)));
}

void KSettingsOnlineQuotes::resetConfig()
{
  QStringList::ConstIterator it;
  QStringList groups = WebPriceQuote::quoteSources();

  // delete all currently defined entries
  for (it = groups.constBegin(); it != groups.constEnd(); ++it) {
    WebPriceQuoteSource(*it).remove();
  }

  // and write back the one's from the reset list
  QList<WebPriceQuoteSource>::ConstIterator itr;
  for (itr = m_resetList.constBegin(); itr != m_resetList.constEnd(); ++itr) {
    (*itr).write();
  }

  loadList();
}

void KSettingsOnlineQuotes::slotLoadWidgets()
{
  m_quoteInEditing = false;
  QListWidgetItem* item = m_quoteSourceList->currentItem();

  m_editURL->setEnabled(true);
  m_editSymbol->setEnabled(true);
  m_editPrice->setEnabled(true);
  m_editDate->setEnabled(true);
  m_editDateFormat->setEnabled(true);
  m_skipStripping->setEnabled(true);
  m_editURL->setText(QString());
  m_editSymbol->setText(QString());
  m_editPrice->setText(QString());
  m_editDate->setText(QString());
  m_editDateFormat->setText(QString());

  if (item) {
    m_currentItem = WebPriceQuoteSource(item->text());
    m_editURL->setText(m_currentItem.m_url);
    m_editSymbol->setText(m_currentItem.m_sym);
    m_editPrice->setText(m_currentItem.m_price);
    m_editDate->setText(m_currentItem.m_date);
    m_editDateFormat->setText(m_currentItem.m_dateformat);
    m_skipStripping->setChecked(m_currentItem.m_skipStripping);
  } else {
    m_editURL->setEnabled(false);
    m_editSymbol->setEnabled(false);
    m_editPrice->setEnabled(false);
    m_editDate->setEnabled(false);
    m_editDateFormat->setEnabled(false);
    m_skipStripping->setEnabled(false);
  }

  m_updateButton->setEnabled(false);

}

void KSettingsOnlineQuotes::slotEntryChanged()
{
  clearIcons();
  bool modified = m_editURL->text() != m_currentItem.m_url
                  || m_editSymbol->text() != m_currentItem.m_sym
                  || m_editDate->text() != m_currentItem.m_date
                  || m_editDateFormat->text() != m_currentItem.m_dateformat
                  || m_editPrice->text() != m_currentItem.m_price
                  || m_skipStripping->isChecked() != m_currentItem.m_skipStripping;

  m_updateButton->setEnabled(modified);
  m_checkButton->setEnabled(!modified);
}

void KSettingsOnlineQuotes::slotDeleteEntry()
{
  QListWidgetItem* item = m_quoteSourceList->findItems(m_currentItem.m_name, Qt::MatchExactly).at(0);
  if (!item)
    return;

  int ret = KMessageBox::warningContinueCancel(this,
                i18n("Are you sure to delete this online quote ?"),
                i18n("Delete online quote"),
                KStandardGuiItem::cont(),
                KStandardGuiItem::cancel(),
                QString("DeletingOnlineQuote"));
  if (ret == KMessageBox::Cancel)
    return;

  delete item;
  m_currentItem.remove();
  slotEntryChanged();
}

void KSettingsOnlineQuotes::slotShowEntry()
{
  if (m_currentItem.m_url.contains("%2"))
    QDesktopServices::openUrl(m_currentItem.m_url.arg("BTC","GBP"));
  else
    QDesktopServices::openUrl(m_currentItem.m_url.arg("GBP"));
}

void KSettingsOnlineQuotes::slotUpdateEntry()
{
  WebPriceQuote::Errors errors;
  QRegExp symbolRegexp(m_editSymbol->text());
  if (!symbolRegexp.isValid()) {
    slotLogError(i18n("invalid regular expression in symbol field"));
    errors |= WebPriceQuote::Errors::Symbol;
  }
  QRegExp priceRegexp(m_editPrice->text());
  if (!priceRegexp.isValid()) {
    slotLogError(i18n("invalid regular expression in price field"));
    errors |= WebPriceQuote::Errors::Price;
  }
  QRegExp dateRegexp(m_editDate->text());
  if (!dateRegexp.isValid()) {
    slotLogError(i18n("invalid regular expression in date field"));
    errors |= WebPriceQuote::Errors::Date;
  }
  if (!errors.isEmpty()) {
    setupIcons(errors);
    return;
  }
  slotLogStatus(i18n("regular expression are valid"));
  m_currentItem.m_url = m_editURL->text();
  m_currentItem.m_sym = m_editSymbol->text();
  m_currentItem.m_date = m_editDate->text();
  m_currentItem.m_dateformat = m_editDateFormat->text();
  m_currentItem.m_price = m_editPrice->text();
  m_currentItem.m_skipStripping = m_skipStripping->isChecked();
  m_currentItem.write();
  m_checkButton->setEnabled(true);
  slotEntryChanged();
}

void KSettingsOnlineQuotes::slotNewEntry()
{
  WebPriceQuoteSource newSource(i18n("New Quote Source"));
  newSource.write();
  loadList();
  QListWidgetItem* item = m_quoteSourceList->findItems(i18n("New Quote Source"), Qt::MatchExactly).at(0);
  if (item) {
    m_quoteSourceList->setCurrentItem(item);
    slotLoadWidgets();
  }
}

void KSettingsOnlineQuotes::clearIcons()
{
  QPixmap emptyIcon;
  m_urlCheckLabel->setPixmap(emptyIcon);
  m_dateCheckLabel->setPixmap(emptyIcon);
  m_priceCheckLabel->setPixmap(emptyIcon);
  m_symbolCheckLabel->setPixmap(emptyIcon);
  m_dateFormatCheckLabel->setPixmap(emptyIcon);
}

void KSettingsOnlineQuotes::setupIcons(const WebPriceQuote::Errors &errors)
{
  QPixmap okIcon(BarIcon("dialog-ok-apply"));
  QPixmap failIcon(BarIcon("dialog-cancel"));

  if (errors & WebPriceQuote::Errors::URL)
    m_urlCheckLabel->setPixmap(failIcon);
  else {
    m_urlCheckLabel->setPixmap(okIcon);
    m_symbolCheckLabel->setPixmap(errors & WebPriceQuote::Errors::Symbol ? failIcon : okIcon);
    m_priceCheckLabel->setPixmap(errors & WebPriceQuote::Errors::Price ? failIcon : okIcon);
    if (errors & WebPriceQuote::Errors::Date)
      m_dateCheckLabel->setPixmap(failIcon);
    else {
      m_dateCheckLabel->setPixmap(okIcon);
      m_dateFormatCheckLabel->setPixmap(errors & WebPriceQuote::Errors::DateFormat? failIcon : okIcon);
    }
  }
}

void KSettingsOnlineQuotes::slotCheckEntry()
{
  WebPriceQuote quote;
  m_logWindow->setVisible(true);
  m_logWindow->clear();
  clearIcons();

  connect(&quote, SIGNAL(status(const QString&)), this, SLOT(slotLogStatus(const QString&)));
  connect(&quote, SIGNAL(error(const QString&)), this, SLOT(slotLogError(const QString&)));
  connect(&quote, SIGNAL(failed(const QString&)), this, SLOT(slotLogStatus(const QString&)));
  connect(&quote, SIGNAL(quote(const QString&, const QString&, const QString&, double)), this, SLOT(slotLogQuote(const QString&, const QString&, const QString&, double)));
  if (m_currentItem.m_url.contains("%2"))
    quote.launch("BTC > GBP", "BTC GBP", m_currentItem.m_name);
  else
    quote.launch("GBP", "GBP", m_currentItem.m_name);
  setupIcons(quote.errors());
}

void KSettingsOnlineQuotes::slotLogStatus(const QString &s)
{
  new QListWidgetItem(s, m_logWindow);
  m_logWindow->scrollToBottom();
}

void KSettingsOnlineQuotes::slotLogError(const QString &s)
{
  QListWidgetItem *item = new QListWidgetItem(s, m_logWindow);
  item->setForeground(Qt::red);
  m_logWindow->scrollToBottom();
}

void KSettingsOnlineQuotes::slotLogQuote(const QString &id, const QString &symbol, const QString &date, double price)
{
  slotLogStatus(QString("%1 %2 %3 %4").arg(id, symbol, date).arg(price));
}

void KSettingsOnlineQuotes::slotStartRename(QListWidgetItem* item)
{
  m_quoteInEditing = true;
  m_quoteSourceList->editItem(item);
}

void KSettingsOnlineQuotes::slotEntryRenamed(QListWidgetItem* item)
{
  //if there is no current item selected, exit
  if (m_quoteInEditing == false || !m_quoteSourceList->currentItem() || item != m_quoteSourceList->currentItem())
    return;

  m_quoteInEditing = false;
  QString text = item->text();
  int nameCount = 0;
  for (int i = 0; i < m_quoteSourceList->count(); ++i) {
    if (m_quoteSourceList->item(i)->text() == text)
      ++nameCount;
  }

  // Make sure we get a non-empty and unique name
  if (text.length() > 0 && nameCount == 1) {
    m_currentItem.rename(text);
  } else {
    item->setText(m_currentItem.m_name);
  }
  m_quoteSourceList->sortItems();
  m_newButton->setEnabled(m_quoteSourceList->findItems(i18n("New Quote Source"), Qt::MatchExactly).count() == 0);
}
