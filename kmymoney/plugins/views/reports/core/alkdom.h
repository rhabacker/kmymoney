/*
    SPDX-FileCopyrightText: 2024 Ralf Habacker <ralf.habacker@freenet.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ALKDOM_H
#define ALKDOM_H

#include <QString>
#include <QStringList>

class AlkDomDocument;

/**
 * The class AlkDomElement is a replacement for QDomElement
 * with the possibility to store attributes sorted.
 *
 * Sorted attributes are important for a textual comparison.
 *
 * @author Ralf Habacker <ralf.habacker@freenet.de>
 */
class AlkDomElement
{
public:
    AlkDomElement(const QString& tag = QString());
    virtual ~AlkDomElement();

    void setAttribute(const QString& name, const QString& value);
    void setAttribute(const QString& name, double value);
    void appendChild(const AlkDomElement& element);
    virtual QString toString(bool withIndentation = true, int level = 0) const;

protected:
    QString m_tag;
    QStringList m_attributes;
    QList<AlkDomElement> m_childs;
};

/**
 * The class AlkDomDocument is a simple replacement for QDomDocument.
 *
 * @author Ralf Habacker <ralf.habacker@freenet.de>
 */
class AlkDomDocument : public AlkDomElement
{
public:
    AlkDomDocument(const QString& type = QString());
    virtual ~AlkDomDocument();

    AlkDomElement createElement(const QString& name);
    QString toString(bool withIndentation = true, int level = 0) const override;

protected:
    QString m_type;
};

#endif // ALKDOM_H
