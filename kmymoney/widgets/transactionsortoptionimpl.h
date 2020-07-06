/*  This file is part of the KDE project
    Copyright (C) 2009 Laurent Montel <montel@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef TRANSACTIONSORTOPTIONIMPL_H
#define TRANSACTIONSORTOPTIONIMPL_H

// ----------------------------------------------------------------------------
// QT Includes
#include <QListWidgetItem>

// ----------------------------------------------------------------------------
// KDE Includes
#include <KListWidget>

// ----------------------------------------------------------------------------
// Project Includes
namespace Ui
{
class TransactionSortOptionDecl;
};

class TransactionSortOption : public QWidget
{
  Q_OBJECT
public:
  TransactionSortOption(QWidget *parent);
  ~TransactionSortOption();
  QString settings() const;
  bool supportsCorrectBalance();
public slots:
  void setSettings(const QString& settings);
  void toggleDirection(QListWidgetItem * item);

protected:
  QListWidgetItem * addEntry(KListWidget * p, QListWidgetItem * after, int idx);
  void setDirectionIcon(QListWidgetItem* item);
protected slots:
  void slotAvailableSelected();
  void slotSelectedSelected();
  void slotAddItem();
  void slotRemoveItem();
  void slotUpItem();
  void slotDownItem();
  void slotFocusChanged(QWidget *o, QWidget *n);
private:
  void init();
signals:
  void settingsChanged(const QString&);

private:
  Ui::TransactionSortOptionDecl *ui;
};

#endif /* TRANSACTIONSORTOPTIONIMPL_H */
