/*
  This file is part of KMyMoney, A Personal Finance Manager for KDE
  Copyright (C) 2013 Christian Dávid <christian-david@web.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SEPAONLINETRANSFER_H
#define SEPAONLINETRANSFER_H

#include "misc/validators.h"
#include "onlinetasks/interfaces/tasks/onlinetask.h"
#include "onlinetasks/interfaces/tasks/credittransfer.h"
#include "onlinetasks/interfaces/tasks/ionlinetasksettings.h"
#include "payeeidentifier/ibanandbic/ibanbic.h"

/**
 * @brief SEPA Credit Transfer
 */
class sepaOnlineTransfer : public onlineTask, public creditTransfer
{
  Q_INTERFACES(creditTransfer);

public:
  ONLINETASK_META(sepaOnlineTransfer, "org.kmymoney.creditTransfer.sepa");
  sepaOnlineTransfer() : onlineTask(), creditTransfer() {}
  sepaOnlineTransfer(const sepaOnlineTransfer &other) : onlineTask(other), creditTransfer(other) {}

  virtual QString responsibleAccount() const = 0;
  virtual void setOriginAccount(const QString& accountId) = 0;

  virtual MyMoneyMoney value() const = 0;
  virtual void setValue(MyMoneyMoney value) = 0;

  virtual void setBeneficiary(const payeeIdentifiers::ibanBic& accountIdentifier) = 0;
  virtual payeeIdentifiers::ibanBic beneficiaryTyped() const = 0;

  virtual void setPurpose(const QString purpose) = 0;
  virtual QString purpose() const = 0;

  virtual void setEndToEndReference(const QString& reference) = 0;
  virtual QString endToEndReference() const = 0;

  /**
   * @brief Returns the origin account identifier
   * @return you are owner of the object
   */
  virtual payeeIdentifier originAccountIdentifier() const = 0;

  /**
   * National account can handle the currency of the related account only.
   */
  virtual MyMoneySecurity currency() const = 0;

  virtual bool isValid() const = 0;

  virtual QString jobTypeName() const = 0;

  virtual unsigned short int textKey() const = 0;
  virtual unsigned short int subTextKey() const = 0;

  virtual bool hasReferenceTo(const QString& id) const = 0;

  class settings : public IonlineTaskSettings
  {
  public:
    // Limits getter
    virtual int purposeMaxLines() const = 0;
    virtual int purposeLineLength() const = 0;
    virtual int purposeMinLength() const = 0;

    virtual int recipientNameLineLength() const = 0;
    virtual int recipientNameMinLength() const = 0;

    virtual int payeeNameLineLength() const = 0;
    virtual int payeeNameMinLength() const = 0;

    virtual QString allowedChars() const = 0;

    // Checker
    virtual bool checkPurposeCharset(const QString& string) const = 0;
    virtual bool checkPurposeLineLength(const QString& purpose) const = 0;
    virtual validators::lengthStatus checkPurposeLength(const QString& purpose) const = 0;
    virtual bool checkPurposeMaxLines(const QString& purpose) const = 0;

    virtual validators::lengthStatus checkNameLength(const QString& name) const = 0;
    virtual bool checkNameCharset(const QString& name) const = 0;

    virtual validators::lengthStatus checkRecipientLength(const QString& name) const = 0;
    virtual bool checkRecipientCharset(const QString& name) const = 0;

    virtual int endToEndReferenceLength() const = 0;
    virtual validators::lengthStatus checkEndToEndReferenceLength(const QString& reference) const = 0;

    virtual bool checkRecipientBic(const QString& bic) const = 0;

    /**
     * @brief Checks if the bic is mandatory for the given iban
     *
     * For the check usually only the first two chars are needed. So you do not
     * need to validate the IBAN.
     *
     * @todo LOW: Implement, should be simple to test: if the country code in iban is the same as in origin iban and
     * the iban belongs to a sepa country a bic is not necessary. Will change 1. Feb 2016.
     */
    virtual bool isBicMandatory(const QString& payeeiban, const QString& beneficiaryIban) const = 0;
  };

  virtual QSharedPointer<const settings> getSettings() const = 0;

protected:
  virtual sepaOnlineTransfer* clone() const = 0;

  virtual sepaOnlineTransfer* createFromXml(const QDomElement &element) const = 0;
  virtual void writeXML(QDomDocument& document, QDomElement& parent) const = 0;

};

Q_DECLARE_INTERFACE(sepaOnlineTransfer, "org.kmymoney.creditTransfer.sepa")

#endif // SEPAONLINETRANSFER_H
