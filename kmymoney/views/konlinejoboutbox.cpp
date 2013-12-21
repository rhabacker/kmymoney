/*
 * This file is part of KMyMoney, A Personal Finance Manager for KDE
 * Copyright (C) 2013 Christian Dávid <christian-david@web.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "konlinejoboutbox.h"
#include "ui_konlinejoboutbox.h"

#include <KMessageBox>

#include "models/models.h"
#include "models/onlinejobmodel.h"

#include "mymoney/mymoneyfile.h"

#include <QDebug>

KOnlineJobOutbox::KOnlineJobOutbox(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KOnlineJobOutbox)
{
    ui->setupUi(this);

    onlineJobModel* model = new onlineJobModel(this);
    ui->m_onlineJobView->setModel( model );
    connect(ui->m_buttonSend, SIGNAL( clicked() ), this, SLOT( slotSendJobs() ));
    connect(ui->m_buttonRemove, SIGNAL(clicked()), this, SLOT( slotRemoveJob() ));
    connect(ui->m_buttonEdit, SIGNAL(clicked()), this, SLOT( slotEditJob() ));
    connect(ui->m_onlineJobView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotEditJob(QModelIndex)));
    connect(ui->m_buttonNewCreditTransfer, SIGNAL(clicked()), this, SIGNAL(newCreditTransfer()));
}

KOnlineJobOutbox::~KOnlineJobOutbox()
{
    delete ui;
}

void KOnlineJobOutbox::slotRemoveJob()
{
  QModelIndexList indexes = ui->m_onlineJobView->selectionModel()->selectedRows();

  if (indexes.isEmpty())
    return;

  QAbstractItemModel* model = ui->m_onlineJobView->model();
  const int count = indexes.count();
  for ( int i = count-1; i >= 0; --i ) {
    model->removeRow( indexes.at(i).row() );
  }
}

void KOnlineJobOutbox::slotSendJobs()
{
  QList<onlineJob> validJobs;
  foreach( onlineJob job, MyMoneyFile::instance()->onlineJobList()) {
    if (job.isValid())
      validJobs.append(job);
  }
  qDebug() << "I shall send " << validJobs.count() << "/" << MyMoneyFile::instance()->onlineJobList().count() << " onlineJobs";
  if ( !validJobs.isEmpty() )
    emit sendJobs( validJobs );
}

void KOnlineJobOutbox::slotEditJob()
{
  QModelIndexList indexes = ui->m_onlineJobView->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    QString jobId = ui->m_onlineJobView->model()->data(indexes.first(), onlineJobModel::OnlineJobId).toString();
    Q_ASSERT( !jobId.isEmpty() );
    emit editJob(jobId);
  }
}

void KOnlineJobOutbox::slotEditJob(const QModelIndex &index)
{
  QString jobId = ui->m_onlineJobView->model()->data(index, onlineJobModel::OnlineJobId).toString();
  emit editJob(jobId);
}
