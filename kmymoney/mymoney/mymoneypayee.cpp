/***************************************************************************
                          mymoneypayee.cpp
                             -------------------
    copyright            : (C) 2000 by Michael Edwardes <mte@users.sourceforge.net>
                           (C) 2008 by Thomas Baumgart <ipwizard@users.sourceforge.net>

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mymoneypayee.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QStringList>
#include <QDebug> //! @todo remove QDebugs
#include <QUrl>

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoneyutils.h"
#include <mymoneyexception.h>
#include <mymoneysplit.h>
#include <mymoneytransaction.h>
#include <pluginloader.h>

MyMoneyPayee MyMoneyPayee::null;

MyMoneyPayee::MyMoneyPayee() :
    m_matchingEnabled(false),
    m_usingMatchKey(false),
    m_matchKeyIgnoreCase(true)
{
}

MyMoneyPayee::MyMoneyPayee(const QString& id, const MyMoneyPayee& payee) :
    m_matchingEnabled(false),
    m_usingMatchKey(false),
    m_matchKeyIgnoreCase(true)
{
  *this = payee;
  m_id = id;
}

MyMoneyPayee::MyMoneyPayee(const QString& name, const QString& address,
                           const QString& city, const QString& state, const QString& postcode,
                           const QString& telephone, const QString& email,
                           const QString& idPattern, const QString& urlTemplate) :
    m_matchingEnabled(false),
    m_usingMatchKey(false),
    m_matchKeyIgnoreCase(true)
{
  m_name      = name;
  m_address   = address;
  m_city      = city;
  m_state     = state;
  m_postcode  = postcode;
  m_telephone = telephone;
  m_email     = email;
  m_idPattern = idPattern;
  m_urlTemplate = urlTemplate;
}

MyMoneyPayee::MyMoneyPayee(const QDomElement& node)
    : MyMoneyObject(node),
      m_usingMatchKey(false),
      m_matchKeyIgnoreCase(true)
{
  if ("PAYEE" != node.tagName()) {
    throw MYMONEYEXCEPTION("Node was not PAYEE");
  }

  m_name = node.attribute("name");
  m_reference = node.attribute("reference");
  m_email = node.attribute("email");

  m_matchingEnabled = node.attribute("matchingenabled", "0").toUInt();
  if (m_matchingEnabled) {
    setMatchData((node.attribute("usingmatchkey", "0").toUInt() != 0) ? matchKey : matchName,
                 node.attribute("matchignorecase", "0").toUInt(),
                 node.attribute("matchkey"));
  }

  if (node.hasAttribute("notes")) {
    m_notes = node.attribute("notes");
  }

  if (node.hasAttribute("defaultaccountid")) {
    m_defaultAccountId = node.attribute("defaultaccountid");
  }

  if (node.hasAttribute("idpattern")) {
      m_idPattern = node.attribute("idpattern");
      m_urlTemplate = node.attribute("urltemplate");
  }

  // Load Address
  QDomNodeList nodeList = node.elementsByTagName("ADDRESS");
  if (nodeList.count() == 0) {
    QString msg = QString("No ADDRESS in payee %1").arg(m_name);
    throw MYMONEYEXCEPTION(msg);
  }

  QDomElement addrNode = nodeList.item(0).toElement();
  m_address = addrNode.attribute("street");
  m_city = addrNode.attribute("city");
  m_postcode = addrNode.attribute("postcode");
  m_state = addrNode.attribute("state");
  m_telephone = addrNode.attribute("telephone");

  MyMoneyPayeeIdentifierContainer::loadXML(node);
}

MyMoneyPayee::~MyMoneyPayee()
{
}

MyMoneyPayee::MyMoneyPayee(const MyMoneyPayee& right)
    : MyMoneyObject(right)
{
  *this = right;
}

bool MyMoneyPayee::operator == (const MyMoneyPayee& right) const
{
  return (MyMoneyObject::operator==(right) &&
          ((m_name.length() == 0 && right.m_name.length() == 0) || (m_name == right.m_name)) &&
          ((m_address.length() == 0 && right.m_address.length() == 0) || (m_address == right.m_address)) &&
          ((m_city.length() == 0 && right.m_city.length() == 0) || (m_city == right.m_city)) &&
          ((m_state.length() == 0 && right.m_state.length() == 0) || (m_state == right.m_state)) &&
          ((m_postcode.length() == 0 && right.m_postcode.length() == 0) || (m_postcode == right.m_postcode)) &&
          ((m_telephone.length() == 0 && right.m_telephone.length() == 0) || (m_telephone == right.m_telephone)) &&
          ((m_email.length() == 0 && right.m_email.length() == 0) || (m_email == right.m_email)) &&
          (m_matchingEnabled == right.m_matchingEnabled) &&
          (m_usingMatchKey == right.m_usingMatchKey) &&
          (m_matchKeyIgnoreCase == right.m_matchKeyIgnoreCase) &&
          ((m_matchKey.length() == 0 && right.m_matchKey.length() == 0) || m_matchKey == right.m_matchKey) &&
          ((m_reference.length() == 0 && right.m_reference.length() == 0) || (m_reference == right.m_reference)) &&
          ((m_defaultAccountId.length() == 0 && right.m_defaultAccountId.length() == 0) || m_defaultAccountId == right.m_defaultAccountId)) &&
          ((m_idPattern.length() == 0 && right.m_idPattern.length() == 0) || (m_idPattern == right.m_idPattern)) &&
          ((m_urlTemplate.length() == 0 && right.m_urlTemplate.length() == 0) || (m_urlTemplate == right.m_urlTemplate));
}

bool MyMoneyPayee::operator < (const MyMoneyPayee& right) const
{
  return m_name < right.name();
}

void MyMoneyPayee::writeXML(QDomDocument& document, QDomElement& parent) const
{
  QDomElement el = document.createElement("PAYEE");

  writeBaseXML(document, el);

  el.setAttribute("name", m_name);
  el.setAttribute("reference", m_reference);
  el.setAttribute("email", m_email);
  if (!m_notes.isEmpty())
    el.setAttribute("notes", m_notes);

  el.setAttribute("matchingenabled", m_matchingEnabled);
  if (m_matchingEnabled) {
    el.setAttribute("usingmatchkey", m_usingMatchKey);
    el.setAttribute("matchignorecase", m_matchKeyIgnoreCase);
    el.setAttribute("matchkey", m_matchKey);
  }

  if (!m_defaultAccountId.isEmpty()) {
    el.setAttribute("defaultaccountid", m_defaultAccountId);
  }

  if (!m_idPattern.isEmpty() || !m_urlTemplate.isEmpty()) {
    el.setAttribute("idpattern", m_idPattern);
    el.setAttribute("urltemplate", m_urlTemplate);
  }
  // Save address
  QDomElement address = document.createElement("ADDRESS");
  address.setAttribute("street", m_address);
  address.setAttribute("city", m_city);
  address.setAttribute("postcode", m_postcode);
  address.setAttribute("state", m_state);
  address.setAttribute("telephone", m_telephone);

  el.appendChild(address);

  // Save payeeIdentifiers (account numbers)
  MyMoneyPayeeIdentifierContainer::writeXML(document, el);

  parent.appendChild(el);
}

bool MyMoneyPayee::hasReferenceTo(const QString& id) const
{
  return id == m_defaultAccountId;

}

MyMoneyPayee::payeeMatchType MyMoneyPayee::matchData(bool& ignorecase, QStringList& keys) const
{
  payeeMatchType type = matchDisabled;
  keys.clear();
  ignorecase = m_matchKeyIgnoreCase;

  if (m_matchingEnabled) {
    type = m_usingMatchKey ? matchKey : matchName;
    if (type == matchKey)
      keys = m_matchKey.split(';');
  }

  return type;
}

MyMoneyPayee::payeeMatchType MyMoneyPayee::matchData(bool& ignorecase, QString& keyString) const
{
  QStringList keys;
  payeeMatchType type = matchData(ignorecase, keys);
  keyString = keys.join(";");
  return type;
}

void MyMoneyPayee::setMatchData(payeeMatchType type, bool ignorecase, const QStringList& keys)
{
  m_matchingEnabled = (type != matchDisabled);
  m_matchKeyIgnoreCase = ignorecase;
  m_matchKey.clear();

  if (m_matchingEnabled) {
    m_usingMatchKey = (type == matchKey);
    if (m_usingMatchKey) {
      QRegExp validKeyRegExp("[^ ]");
      QStringList filteredKeys = keys.filter(validKeyRegExp);
      m_matchKey = filteredKeys.join(";");
    }
  }
}

void MyMoneyPayee::setMatchData(payeeMatchType type, bool ignorecase, const QString& keys)
{
  setMatchData(type, ignorecase, keys.split(';'));
}

QString MyMoneyPayee::idPattern() const
{
  return m_idPattern;
}

void MyMoneyPayee::setIdPattern(const QString& idPattern)
{
  m_idPattern = idPattern;
}

QString MyMoneyPayee::urlTemplate() const
{
  return m_urlTemplate;
}

void MyMoneyPayee::setUrlTemplate(const QString& urlTemplate)
{
  m_urlTemplate = urlTemplate;
}

QUrl MyMoneyPayee::payeeLink(const QString& text) const
{
  return payeeLink(idPattern(), urlTemplate(), text);
}

QUrl MyMoneyPayee::payeeLink(const MyMoneySplit& split, const MyMoneyTransaction &t) const
{
  QUrl url = payeeLink(idPattern(), urlTemplate(), split.memo());
  if (url.scheme() == "file" && url.toString().contains("%t")) {
    QString p = url.toLocalFile();
    p.replace("%tyear", QString("%1").arg(t.postDate().year()));
    url = QUrl::fromLocalFile(p);
  }
  return url;
}

QString MyMoneyPayee::decorateLink(const QString& text) const
{
  QStringList matches = matchingLinks(idPattern(), urlTemplate(), text);
  if (matches.size() == 0)
    return text;
  QRegExp rx(QString("(%1)").arg(matches[0]));

  QString result = text;
  result.replace(rx, "<u>\\1</u>");
  return result;
}

QStringList MyMoneyPayee::matchingLinks(const QString& idPattern, const QString& urlTemplate, const QString& text)
{
  QStringList result;
  if (idPattern.isEmpty() || urlTemplate.isEmpty())
    return result;
  QRegExp rx(idPattern.contains("(") ? QString("%1").arg(idPattern) : QString("(%1)").arg(idPattern));
  if (rx.indexIn(text) == -1)
    return result;
  result = rx.capturedTexts();
  return result;
}

QString fillUrlTemplate(const QString &templateStr, const QStringList &matches)
{
    QString result = templateStr;

    // Iterate in reverse to avoid partial replacements (%10 before %1)
    for (int i = matches.size() - 1; i >= 1; --i) {
        const QString placeholder = "%" + QString::number(i);

        // Only replace if the template actually contains the placeholder
        if (result.contains(placeholder)) {
            result.replace(placeholder, matches[i]);
        }
    }

    return result;
}

QUrl MyMoneyPayee::payeeLink(const QString& idPattern, const QString& urlTemplate, const QString& text)
{
    QStringList matches = matchingLinks(idPattern, urlTemplate, text);
    QString result;
    if (matches.size() >= 2) {
        return QUrl(fillUrlTemplate(urlTemplate, matches));
    } else {
        return QUrl();
    }
}

// vim:cin:si:ai:et:ts=2:sw=2:
