/***************************************************************************
                          datev2kmt.cpp
                          -------------------
    copyright            : (C) 2016 by Ralf Habacker <ralf.habacker@freenet.de>

****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../kmymoney/mymoney/mymoneyaccount.h"

#include <QFile>
#include <QStringList>
#include <QMap>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QtDebug>


QDebug operator <<(QDebug out, const QXmlStreamNamespaceDeclaration &a)
{
    out << "QXmlStreamNamespaceDeclaration("
        << "prefix:" << a.prefix().toString()
        << "namespaceuri:" << a.namespaceUri().toString()
        << ")";
    return out;
}

QDebug operator <<(QDebug out, const QXmlStreamAttribute &a)
{
    out << "QXmlStreamAttribute("
        << "prefix:" << a.prefix().toString()
        << "namespaceuri:" << a.namespaceUri().toString()
        << "name:" << a.name().toString()
        << " value:" << a.value().toString()
        << ")";
    return out;
}

bool debug = false;
bool withID = false;
bool noLevel1Names = false;
bool withTax = false;
bool prefixNameWithCode = false;

int toKMyMoneyAccountType(const QString &type)
{
    if(type == "ROOT") return MyMoneyAccount::UnknownAccountType;
    else if (type == "BANK") return MyMoneyAccount::Checkings;
    else if (type == "CASH") return MyMoneyAccount::Cash;
    else if (type == "CREDIT") return MyMoneyAccount::CreditCard;
    else if (type == "INVEST") return MyMoneyAccount::Investment;
    else if (type == "RECEIVABLE") return MyMoneyAccount::Asset;
    else if (type == "ASSET") return MyMoneyAccount::Asset;
    else if (type == "PAYABLE") return MyMoneyAccount::Liability;
    else if (type == "LIABILITY") return MyMoneyAccount::Liability;
    else if (type == "CURRENCY") return MyMoneyAccount::Currency;
    else if (type == "INCOME") return MyMoneyAccount::Income;
    else if (type == "EXPENSE") return MyMoneyAccount::Expense;
    else if (type == "STOCK") return MyMoneyAccount::Stock;
    else if (type == "MUTUAL") return MyMoneyAccount::Stock;
    else if (type == "EQUITY") return MyMoneyAccount::Equity;
    else return 99; // unknown
}

class TemplateAccount {
public:
    typedef QList<TemplateAccount> List;
    typedef QList<TemplateAccount*> PointerList;
    typedef QMap<QString,QString> SlotList;

    QString id;
    QString type;
    QString name;
    QString code;
    QString parent;

    TemplateAccount()
    {
    }

    TemplateAccount(const TemplateAccount &b)
      : id(b.id),
        type(b.type),
        name(b.name),
        code(b.code),
        parent(b.parent)
    {
    }

    void clear()
    {
        id = "";
        type = "";
        name = "";
        code = "";
        parent = "";
    }
};

QDebug operator <<(QDebug out, const TemplateAccount &a)
{
    out << "TemplateAccount("
        << "name:" << a.name
        << "id:" << a.id
        << "type:" << a.type
        << "code:" << a.code
        << "parent:" << a.parent
        << ")\n";
    return out;
}

QDebug operator <<(QDebug out, const TemplateAccount::PointerList &a)
{
    out << "TemplateAccount::List(";
    foreach(const TemplateAccount *account, a)
            out << *account;
    out << ")";
    return out;
}

class TemplateFile {
public:
    QString title;
    QString longDescription;
    QString shortDescription;
    TemplateAccount::List accounts;

    bool read(const QString &line)
    {
        return true;
    }

    bool writeAsXml(QXmlStreamWriter &xml)
    {
        xml.writeStartElement("","title");
        xml.writeCharacters(title);
        xml.writeEndElement();
        xml.writeStartElement("","shortdesc");
        xml.writeCharacters(shortDescription);
        xml.writeEndElement();
        xml.writeStartElement("","longdesc");
        xml.writeCharacters(longDescription);
        xml.writeEndElement();
        xml.writeStartElement("","accounts");
        bool result = writeAccountsAsXml(xml);
        xml.writeEndElement();
        return result;
    }

    bool writeAccountsAsXml(QXmlStreamWriter &xml, const QString &id="", int index=0)
    {
        TemplateAccount::PointerList list;

        if (index == 0)
            list = accountsByType("ROOT");
        else
            list = accountsByParentID(id);

        foreach(TemplateAccount *account, list)
        {
            if (account->type != "ROOT")
            {
                xml.writeStartElement("","account");
                xml.writeAttribute("type", QString::number(toKMyMoneyAccountType(account->type)));
                xml.writeAttribute("name", noLevel1Names && index < 2 ? "" : account->name);
                if (withID)
                    xml.writeAttribute("id", account->id);
//                if (withTax) {
//                    if (account->slotList.contains("tax-related")) {
//                        xml.writeStartElement("flag");
//                        xml.writeAttribute("name","Tax");
//                        xml.writeAttribute("value",account->slotList["tax-related"]);
//                        xml.writeEndElement();
//                    }
//                }
            }
            index++;
            writeAccountsAsXml(xml, account->id, index);
            index--;
            xml.writeEndElement();
        }
        return true;
    }

    TemplateAccount *account(const QString &id)
    {
        for(int i=0; i < accounts.size(); i++)
        {
            TemplateAccount &account = accounts[i];
            if (account.id == id)
                return &account;
        }
        return 0;
    }

    TemplateAccount::PointerList accountsByType(const QString &type)
    {
        TemplateAccount::PointerList list;
        for(int i=0; i < accounts.size(); i++)
        {
            TemplateAccount &account = accounts[i];
            if (account.type == type)
                list.append(&account);
        }
        return list;
    }


    static bool nameLessThan(TemplateAccount *a1, TemplateAccount *a2)
    {
        return a1->name < a2->name;
    }

    TemplateAccount::PointerList accountsByParentID(const QString &parentID)
    {
        TemplateAccount::PointerList list;

        for(int i=0; i < accounts.size(); i++)
        {
            TemplateAccount &account = accounts[i];
            if (account.parent == parentID)
                list.append(&account);
        }
        qSort(list.begin(), list.end(), nameLessThan);
        return list;
    }

    bool dumpTemplates(const QString &id="", int index=0)
    {
        TemplateAccount::PointerList list;

        if (index == 0)
            list = accountsByType("ROOT");
        else
            list = accountsByParentID(id);

        foreach(TemplateAccount *account, list)
        {
            QString a;
            a.fill(' ', index);
            qDebug() << a << account->name << toKMyMoneyAccountType(account->type);
            index++;
            dumpTemplates(account->id, index);
            index--;
        }
        return true;
    }
};

QDebug operator <<(QDebug out, const TemplateFile &a)
{
    out << "TemplateFile("
        << "title:" << a.title
        << "short description:" << a.shortDescription
        << "long description:" << a.longDescription
        << "accounts:";
    foreach(const TemplateAccount &account, a.accounts)
        out << account;
    out << ")";
    return out;
}

class DATEVAccountTemplateReader 
{
public:
    DATEVAccountTemplateReader()
    {
    }

    bool read(const QString &filename)
    {
        QFile file(filename);
        QTextStream in(&file);
        in.setCodec("utf-8");

        if(!file.open(QIODevice::ReadOnly))
            return false;
        inFileName = filename;
        return read(in.device());
    }

    TemplateFile &result()
    {
        return _template;
    }

    bool dumpTemplates()
    {
        return _template.dumpTemplates();
    }

    bool writeAsXml(const QString &filename=QString())
    {
        if (filename.isEmpty())
        {
            QTextStream stream(stdout);
            return writeAsXml(stream.device());
        }
        else
        {
            QFile file(filename);
            if(!file.open(QIODevice::WriteOnly))
                return false;
            return writeAsXml(&file);
        }
    }

protected:

    bool checkAndUpdateAvailableNamespaces(QXmlStreamReader &xml)
    {
        if (xml.namespaceDeclarations().size() < 5)
        {
            qWarning() << "gnucash template file is missing required name space declarations; adding by self";
        }
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("act", "http://www.gnucash.org/XML/act"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("gnc", "http://www.gnucash.org/XML/gnc"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("gnc-act", "http://www.gnucash.org/XML/gnc-act"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("cmdty","http://www.gnucash.org/XML/cmdty"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("slot","http://www.gnucash.org/XML/slot"));
        return true;
    }

    bool read(QIODevice *device)
    {
        QTextStream text(device);
        text.setCodec("UTF-8");
        int columns[6] = { 1, 47, 92, 104, 125, 0 };

        while(!text.atEnd())
        {
            QString line = text.readLine().replace("\n","");
            if (line[0] != ' ')
                continue;
            if (line.trimmed().startsWith("Erl"))
                return true;
            
            QStringList parts;
            for(unsigned int i = 0; i < sizeof(columns)/sizeof(int) - 1; i++)
            {
                int start = columns[i];
                int size = columns[i+1] != 0 ? columns[i+1]-columns[i] -1 : -1;
                //qDebug() << i << start << size;
                if (line.size() >= start + size) {
                    QString column = line.mid(start, size);
                    if (column[0] == ' ' && column[1].isDigit())
                        parts.append(column.mid(1));
                    else
                        parts.append(column);
                }
            }
            qDebug() << parts;
        }
        return true;
    }

    bool writeAsXml(QIODevice *device)
    {
            QXmlStreamWriter xml(device);
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(1);
        xml.setCodec("utf-8");
        xml.writeStartDocument();

        QString fileName = inFileName.replace(QRegExp(".*/accounts"),"accounts");
        xml.writeComment(QString("\n"
            "     Converted using xea2kmt from GnuCash sources\n"
            "\n"
            "        %1\n"
            "\n"
            "     Please check the source file for possible copyright\n"
            "     and license information.\n"
        ).arg(fileName));
        xml.writeDTD("<!DOCTYPE KMYMONEY-TEMPLATE>");
        xml.writeStartElement("","kmymoney-account-template");
        bool result = _template.writeAsXml(xml);
        xml.writeEndElement();
        xml.writeEndDocument();
        return result;
    }

    TemplateFile _template;
    QString inFileName;
};

int main(int argc, char *argv[])
{
    if (argc < 2 || (argc == 2 && QLatin1String(argv[1]) == "--help"))
    {
        qWarning() << "datev2kmt: convert DATEV account template to kmymoney template file";
        qWarning() << argv[0] << "<options> <DATEV-template-file> [<kmymoney-template-output-file>]";
        qWarning() << "options:";
        qWarning() << "          --debug                   - output debug information";
        qWarning() << "          --help                    - this page";
        qWarning() << "Notes: You need to convert the original pdf into a text file for example with okular or pdf2text -layout";
        return -1;
    }

    QString inFileName;
    QString outFileName;
    for(int i = 1; i < argc; i++)
    {
        QString arg = QLatin1String(argv[i]);
        if (arg == "--debug")
            debug = true;
        else if (!arg.startsWith(QLatin1String("--")))
        {
            if (inFileName.isEmpty())
                inFileName = arg;
            else
                outFileName = arg;
        }
        else
        {
            qWarning() << "invalid command line parameter'" << arg << "'";
            return -1;
        }
    }

    DATEVAccountTemplateReader reader;
    bool result = reader.read(inFileName);
    if (debug)
    {
        qDebug() << reader.result();
        reader.dumpTemplates();
    }
    reader.writeAsXml(outFileName);
    return result ? 0 : -2;
}
