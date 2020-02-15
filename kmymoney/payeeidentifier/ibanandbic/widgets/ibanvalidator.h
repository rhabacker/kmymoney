/*
 * This file is part of KMyMoney, A Personal Finance Manager for KDE
 * Copyright (C) 2014 Christian David <christian-david@web.de>
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

#ifndef IBANVALIDATOR_H
#define IBANVALIDATOR_H

#include "payeeidentifier_iban_bic_widgets_export.h"

#include <QValidator>

#include "kmymoneyvalidationfeedback.h"

class PAYEEIDENTIFIER_IBAN_BIC_WIDGETS_EXPORT ibanValidator : public QValidator
{
  Q_OBJECT

public:
  explicit ibanValidator(QObject* parent = 0);
  virtual State validate(QString& , int&) const;
  State validate(const QString&) const;
  virtual void fixup(QString&) const;

  static QPair<KMyMoneyValidationFeedback::MessageType, QString> validateWithMessage(const QString&);
};

#endif // IBANVALIDATOR_H
