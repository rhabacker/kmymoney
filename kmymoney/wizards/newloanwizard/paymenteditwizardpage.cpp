/***************************************************************************
                         paymenteditwizardpage  -  description
                            -------------------
   begin                : Sun Jul 4 2010
   copyright            : (C) 2010 by Fernando Vilas
   email                : kmymoney-devel@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "paymenteditwizardpage.h"

// ----------------------------------------------------------------------------
// QT Includes


// ----------------------------------------------------------------------------
// KDE Includes


// ----------------------------------------------------------------------------
// Project Includes

PaymentEditWizardPage::PaymentEditWizardPage(QWidget *parent)
    : PaymentEditWizardPageDecl(parent)
{
  // Register the fields with the QWizard and connect the
  // appropriate signals to update the "Next" button correctly
  registerField("newPaymentEdit", m_newPaymentEdit, "value", SIGNAL(textChanged()));
  registerField("newPaymentEditValid", m_newPaymentEdit, "valid", SIGNAL(textChanged()));

  connect(m_newPaymentEdit, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

}
