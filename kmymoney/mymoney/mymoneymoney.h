/***************************************************************************
                          mymoneymoney.h
                             -------------------
    copyright            : (C) 2000-2002 by Michael Edwardes
    email                : mte@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef MYMONEYMONEY_H
#define MYMONEYMONEY_H

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config-kmymoney.h>
#endif

// #include <cmath>

//FIXME workaround for dealing with lond double
#include <sstream>

// So we can save this object
#include <QChar>
#include <QString>
#include <QMetaType>

#include "kmm_mymoney_export.h"
#include "mymoneyunittestable.h"
#include "mymoneyexception.h"
#include "mymoneyutils.h"

// Check for standard definitions
#ifdef HAVE_STDINT_H
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS         // force definition of min and max values
#endif
#include <stdint.h>
#else
#include <limits.h>
#define INT64_MAX LLONG_MAX
#define INT64_MIN LLONG_MIN
#endif

// Including the AlkValue class
#include <alkimia/alkvalue.h>

typedef qint64 signed64;
typedef quint64 unsigned64;


/**
  * This class represents a value within the MyMoney Engine
  *
  * @author Michael Edwardes
  * @author Thomas Baumgart
  */
class KMM_MYMONEY_EXPORT MyMoneyMoney : public AlkValue
{
  KMM_MYMONEY_UNIT_TESTABLE

public:
  enum fileVersionE {
    FILE_4_BYTE_VALUE = 0,
    FILE_8_BYTE_VALUE
  };

  enum signPosition {
    // keep those in sync with the ones defined in klocale.h
    ParensAround = 0,
    BeforeQuantityMoney = 1,
    AfterQuantityMoney = 2,
    BeforeMoney = 3,
    AfterMoney = 4
  };

  enum roundingMethod {
    RndNever = 0,
    RndFloor,
    RndCeil,
    RndTrunc,
    RndPromote,
    RndHalfDown,
    RndHalfUp,
    RndRound
  };

  // construction
  MyMoneyMoney();
  explicit MyMoneyMoney(const int iAmount, const signed64 denom);
  explicit MyMoneyMoney(const long int iAmount, const signed64 denom);
  explicit MyMoneyMoney(const QString& pszAmount);
  explicit MyMoneyMoney(const signed64 Amount, const signed64 denom);
  explicit MyMoneyMoney(const double dAmount, const signed64 denom = 100);

  // copy constructor
  MyMoneyMoney(const MyMoneyMoney& Amount);
  MyMoneyMoney(const AlkValue& Amount);

  virtual ~MyMoneyMoney();

  const MyMoneyMoney abs() const {
    return AlkValue::abs();
  };

  /**
    * This method returns a formatted string according to the settings
    * of _thousandSeparator, _decimalSeparator, _negativeMonetarySignPosition,
    * _positiveMonetaryPosition, _negativePrefixCurrencySymbol and
    * _positivePrefixCurrencySymbol. Those values can be modified using
    * the appropriate set-methods.
    *
    * @param currency The currency symbol
    * @param prec The number of fractional digits
    * @param showThousandSeparator should the thousandSeparator symbol
    *                               be inserted (@a true)
    *                               or not (@a false) (default true)
    */
  QString formatMoney(const QString& currency, const int prec, bool showThousandSeparator = true) const;

  /**
   * This is a convenience method. It behaves exactly as the above one,
   * but takes the information about precision out of the denomination
   * @a denom. No currency symbol is shown. If you want to add a currency
   * symbol, please use MyMoneyUtils::formatMoney(const MyMoneyAccount& acc, const MyMoneySecurity& sec, bool showThousandSeparator)
   * instead.
   *
   * @note denom is often set to account.fraction(security).
   */
  QString formatMoney(int denom, bool showThousandSeparator = true) const;

  /**
    * This method is used to convert the smallest fraction information into
    * the corresponding number of digits used for precision.
    *
    * @param fract smallest fractional part (e.g. 100 for cents)
    * @return number of precision digits (e.g. 2 for cents)
    */
  static int denomToPrec(signed64 fract);

  MyMoneyMoney convert(const signed64 denom = 100, const roundingMethod how = RndRound) const;
  static signed64 precToDenom(int prec);
  double toDouble() const;

  static void setThousandSeparator(const QChar &);
  static void setDecimalSeparator(const QChar &);
  static void setNegativeMonetarySignPosition(const signPosition pos);
  static void setPositiveMonetarySignPosition(const signPosition pos);
  static void setNegativePrefixCurrencySymbol(const bool flags);
  static void setPositivePrefixCurrencySymbol(const bool flags);

  static const QChar thousandSeparator();
  static const QChar decimalSeparator();
  static signPosition negativeMonetarySignPosition();
  static signPosition positiveMonetarySignPosition();
  static void setFileVersion(const fileVersionE version);

  const MyMoneyMoney& operator=(const QString& pszAmount);
  const MyMoneyMoney& operator=(const AlkValue& val);

  // comparison
  bool operator==(const MyMoneyMoney& Amount) const;
  bool operator!=(const MyMoneyMoney& Amount) const;
  bool operator<(const MyMoneyMoney& Amount) const;
  bool operator>(const MyMoneyMoney& Amount) const;
  bool operator<=(const MyMoneyMoney& Amount) const;
  bool operator>=(const MyMoneyMoney& Amount) const;

  bool operator==(const QString& pszAmount) const;
  bool operator!=(const QString& pszAmount) const;
  bool operator<(const QString& pszAmount) const;
  bool operator>(const QString& pszAmount) const;
  bool operator<=(const QString& pszAmount) const;
  bool operator>=(const QString& pszAmount) const;

  // calculation
  const MyMoneyMoney operator+(const MyMoneyMoney& Amount) const;
  const MyMoneyMoney operator-(const MyMoneyMoney& Amount) const;
  const MyMoneyMoney operator*(const MyMoneyMoney& factor) const;
  const MyMoneyMoney operator/(const MyMoneyMoney& Amount) const;
  const MyMoneyMoney operator-() const;
  const MyMoneyMoney operator*(int factor) const;

  static MyMoneyMoney maxValue;
  static MyMoneyMoney minValue;
  static MyMoneyMoney autoCalc;

  bool isNegative() const {
    return (valueRef() < 0) ? true : false;
  }
  bool isPositive() const {
    return (valueRef() > 0) ? true : false;
  }
  bool isZero() const {
    return valueRef() == 0;
  }
  bool isAutoCalc() const {
    return (*this == autoCalc);
  }

  MyMoneyMoney reduce() const;

  static const MyMoneyMoney ONE;
  static const MyMoneyMoney MINUS_ONE;

private:

  static QChar _thousandSeparator;
  static QChar _decimalSeparator;
  static signPosition _negativeMonetarySignPosition;
  static signPosition _positiveMonetarySignPosition;
  static bool _negativePrefixCurrencySymbol;
  static bool _positivePrefixCurrencySymbol;
  static MyMoneyMoney::fileVersionE _fileVersion;
};

//=============================================================================
//
//  Inline functions
//
//=============================================================================

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Constructor - constructs object set to 0.
//   Returns: None
//    Throws: Nothing.
// Arguments: None
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney() :
    AlkValue()
{
}

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Constructor - constructs object from an amount in a signed64 value
//   Returns: None
//    Throws: Nothing.
// Arguments: Amount - signed 64 object containing amount
//            denom  - denominator of the object
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney(signed64 Amount, const signed64 denom)
{
  if (!denom)
    throw MYMONEYEXCEPTION("Denominator 0 not allowed!");

  *this = AlkValue(QString("%1/%2").arg(Amount).arg(denom), _decimalSeparator);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Constructor - constructs object from an amount in a double value
//   Returns: None
//    Throws: Nothing.
// Arguments: dAmount - double object containing amount
//            denom   - denominator of the object
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney(const double dAmount, const signed64 denom) :
    AlkValue(dAmount, denom)
{
}

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Constructor - constructs object from an amount in a integer value
//   Returns: None
//    Throws: Nothing.
// Arguments: iAmount - integer object containing amount
//            denom   - denominator of the object
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney(const int iAmount, const signed64 denom)
{
  if (!denom)
    throw MYMONEYEXCEPTION("Denominator 0 not allowed!");
  *this = AlkValue(iAmount, denom);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Constructor - constructs object from an amount in a long integer value
//   Returns: None
//    Throws: Nothing.
// Arguments: iAmount - integer object containing amount
//            denom   - denominator of the object
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney(const long int iAmount, const signed64 denom)
{
  if (!denom)
    throw MYMONEYEXCEPTION("Denominator 0 not allowed!");
  *this = AlkValue(QString("%1/%2").arg(iAmount).arg(denom), _decimalSeparator);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: MyMoneyMoney
//   Purpose: Copy Constructor - constructs object from another
//            MyMoneyMoney object
//   Returns: None
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be copied
//
////////////////////////////////////////////////////////////////////////////////
inline MyMoneyMoney::MyMoneyMoney(const MyMoneyMoney& Amount) :
    AlkValue(Amount)
{
}

inline MyMoneyMoney::MyMoneyMoney(const AlkValue& Amount) :
    AlkValue(Amount)
{
}

inline const MyMoneyMoney& MyMoneyMoney::operator=(const AlkValue & val)
{
  AlkValue::operator=(val);
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator=
//   Purpose: Assignment operator - modifies object from input NULL terminated
//            string
//   Returns: Const reference to the object
//    Throws: Nothing.
// Arguments: pszAmount - NULL terminated string that contains amount
//
////////////////////////////////////////////////////////////////////////////////
inline const MyMoneyMoney& MyMoneyMoney::operator=(const QString & pszAmount)
{
  AlkValue::operator=(pszAmount);
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator==
//   Purpose: Compare equal operator - compares object with input MyMoneyMoney object
//   Returns: true if equal, otherwise false
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator==(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator==(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator!=
//   Purpose: Compare not equal operator - compares object with input MyMoneyMoney object
//   Returns: true if not equal, otherwise false
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator!=(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator!=(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator<
//   Purpose: Compare less than operator - compares object with input MyMoneyMoney object
//   Returns: true if object less than input amount
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator<(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator<(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator>
//   Purpose: Compare greater than operator - compares object with input MyMoneyMoney
//            object
//   Returns: true if object greater than input amount
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator>(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator>(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator<=
//   Purpose: Compare less than equal to operator - compares object with input
//            MyMoneyMoney object
//   Returns: true if object less than or equal to input amount
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator<=(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator<=(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator>=
//   Purpose: Compare greater than equal to operator - compares object with input
//            MyMoneyMoney object
//   Returns: true if object greater than or equal to input amount
//    Throws: Nothing.
// Arguments: Amount - MyMoneyMoney object to be compared with
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator>=(const MyMoneyMoney& Amount) const
{
  return AlkValue::operator>=(Amount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator==
//   Purpose: Compare equal operator - compares object with input amount in a
//            NULL terminated string
//   Returns: true if equal, otherwise false
//    Throws: Nothing.
// Arguments: pszAmount - NULL terminated string that contains amount
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator==(const QString& pszAmount) const
{
  return *this == MyMoneyMoney(pszAmount);
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator!=
//   Purpose: Compare not equal operator - compares object with input amount in
//            a NULL terminated string
//   Returns: true if not equal, otherwise false
//    Throws: Nothing.
// Arguments: pszAmount - NULL terminated string that contains amount
//
////////////////////////////////////////////////////////////////////////////////
inline bool MyMoneyMoney::operator!=(const QString& pszAmount) const
{
  return ! operator==(pszAmount) ;
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator-
//   Purpose: Unary operator - returns the negative value from the object
//   Returns: The current object
//    Throws: Nothing.
// Arguments: None
//
////////////////////////////////////////////////////////////////////////////////
inline const MyMoneyMoney MyMoneyMoney::operator-() const
{
  return AlkValue::operator-();
}

////////////////////////////////////////////////////////////////////////////////
//      Name: operator*
//   Purpose: Multiplication operator - multiplies the object with factor
//   Returns: The current object
//    Throws: Nothing.
// Arguments: AmountInPence - long object to be multiplied
//
////////////////////////////////////////////////////////////////////////////////
inline const MyMoneyMoney MyMoneyMoney::operator*(int factor) const
{
  return AlkValue::operator*(factor);
}

/**
  * Make it possible to hold @ref MyMoneyMoney objects
  * inside @ref QVariant objects.
  */
Q_DECLARE_METATYPE(MyMoneyMoney)

QDebug operator<<(QDebug dbg, const MyMoneyMoney &a);
QDebug operator<<(QDebug dbg, QList<MyMoneyMoney> &a);
QDebug operator<<(QDebug dbg, QMap<QString, MyMoneyMoney> &a);
QDebug operator<<(QDebug dbg, QMap<MyMoneyDate, MyMoneyMoney> &a);
QDebug operator<<(QDebug dbg, QMap<int, MyMoneyMoney> &a);
#endif

