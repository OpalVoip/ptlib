/*
 * contain.cxx
 *
 * Container Classes
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>
#include <ctype.h>

#include <ostream>
#include <limits>
#include <locale>
#include <codecvt>
#include <math.h>

#define new PNEW
#undef  __CLASS__
#define __CLASS__ GetClass()


///////////////////////////////////////////////////////////////////////////////

void PCharArray::PrintOn(ostream & strm) const
{
  PINDEX width = (int)strm.width();
  if (width > GetSize())
    width -= GetSize();
  else
    width = 0;

  bool left = (strm.flags()&ios::adjustfield) == ios::left;
  if (left)
    strm.write(GetPointer(), GetSize());

  while (width-- > 0)
    strm << (char)strm.fill();

  if (!left)
    strm.write(GetPointer(), GetSize());
}


void PCharArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  char* ptr = GetPointer();
  while (strm.good()) {
    strm >> ptr[size++];
    if (size >= GetSize())
      ptr = GetPointer(size + 100);
  }

  SetSize(size);
}


static void PrintBytes(ostream & strm, const uint8_t * buffer, size_t size)
{
  int line_width = (int)strm.width();
  if (line_width == 0)
    line_width = 16;
  strm.width(0);

  int indent = (int)strm.precision();

  int val_width = ((strm.flags()&ios::basefield) == ios::hex) ? 2 : 3;

  ios::fmtflags oldFlags = strm.flags();
  if (strm.fill() == '0')
    strm.setf(ios::right, ios::adjustfield);

  PINDEX i = 0;
  while (i < size) {
    if (i > 0)
      strm << '\n';
    int j;
    for (j = 0; j < indent; j++)
      strm << ' ';
    for (j = 0; j < line_width; j++) {
      if (indent >= 0 && j == line_width/2)
        strm << ' ';
      if (i+j < size)
        strm << setw(val_width) << (buffer[i+j]&0xff);
      else {
        for (int k = 0; k < val_width; k++)
          strm << ' ';
      }
      if (indent >= 0)
        strm << ' ';
    }
    if ((strm.flags()&ios::floatfield) != ios::fixed) {
      strm << "  ";
      for (j = 0; j < line_width; j++) {
        if (i+j < size) {
          unsigned val = buffer[i+j]&0xff;
          if (isprint(val))
            strm << (char)val;
          else
            strm << '.';
        }
      }
    }
    i += line_width;
  }

  strm.flags(oldFlags);
}


void PBYTEArray::PrintOn(ostream & strm) const
{
  PrintBytes(strm, GetPointer(), GetSize());
}


void PBYTEArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  while (strm.good()) {
    unsigned v;
    strm >> v;
    SetAt(size, (uint8_t)v);
    if (!strm.fail()) {
      size++;
      if (size >= GetSize())
        SetSize(size+100);
    }
  }

  SetSize(size);
}


void PBYTEView::PrintOn(ostream & strm) const
{
  PrintBytes(strm, data(), size());
}


void PHexDump::PrintOn(ostream & strm) const
{
  char oldFill = strm.fill('0');
  std::streamsize oldPrec = strm.precision();
  ios::fmtflags oldFlags = strm.setf(ios::hex, ios::basefield);

  if (m_compact) {
    strm.precision(0);
    strm.setf(ios::fixed, ios::floatfield);
    strm.width(GetSize());
  }

  PrintBytes(strm, data(), size());

  strm.flags(oldFlags);
  strm.precision(oldPrec);
  strm.fill(oldFill);
}


///////////////////////////////////////////////////////////////////////////////

PString::PString()
  : m_caseless(false)
{
}


PString::PString(const PString & str)
  : std::string(str)
  , m_caseless(str.m_caseless)
{
}


PString::PString(const PCaselessString & str)
  : std::string(str)
  , m_caseless(str.m_caseless)
{ }


PString::PString(const PCharArray & buf)
  : std::string(buf)
  , m_caseless(false)
{
}


PString::PString(const PBYTEArray & buf)
  : m_caseless(false)
{
  PINDEX bufSize = buf.GetSize();
  if (bufSize > 0) {
    if (buf[bufSize-1] == '\0')
      --bufSize;
    const char * ptr = (const char*)(const BYTE*)buf;
    assign(ptr, ptr+bufSize);
  }
}


PString::PString(const std::string & str)
  : std::string(str)
  , m_caseless(false)
{
}


PString::PString(char c)
  : std::string(1, c)
  , m_caseless(false)
{
}


PString::PString(bool b)
  : std::string(b ? "true" : "false")
  , m_caseless(false)
{
}


const PString & PString::Empty()
{
  static int EmptyStringMemory[(sizeof(PConstString)+sizeof(int)-1)/sizeof(int)];
#undef new
  static PConstString const * EmptyString = new (EmptyStringMemory) PConstString("");
#define new PNEW
  return *EmptyString;
}


PString::PString(const char * cstr)
  : std::string(cstr != NULL ? cstr : "")
  , m_caseless(false)
{
}


PString::PString(const wchar_t * wstr)
  : m_caseless(false)
{
  if (wstr == NULL)
    MakeEmpty();
  else
    InternalFromWChar(wstr, wcslen(wstr));
}

PString::PString(const wchar_t * wstr, PINDEX len)
  : m_caseless(false)
{
  InternalFromWChar(wstr, len);
}


PString::PString(const PWCharArray & wstr)
  : m_caseless(false)
{
  PINDEX size = wstr.GetSize();
  if (size > 0 && wstr[size-1] == 0) // Stip off trailing NULL if present
    size--;
  InternalFromWChar(wstr, size);
}


PString::PString(const char * cstr, PINDEX len)
  : std::string(cstr, len)
  , m_caseless(false)
{
}


static int TranslateHex(char x)
{
  if (x >= 'a')
    return x - 'a' + 10;

  if (x >= 'A')
    return x - 'A' + '\x0a';

  return x - '0';
}


static const unsigned char PStringEscapeCode[]  = {  'a',  'b',  'f',  'n',  'r',  't',  'v' };
static const unsigned char PStringEscapeValue[] = { '\a', '\b', '\f', '\n', '\r', '\t', '\v' };

static void TranslateEscapes(const char * & src, char * dst)
{
  bool hadLeadingQuote = *src == '"';
  if (hadLeadingQuote)
    src++;

  while (*src != '\0') {
    int c = *src++ & 0xff;
    if (c == '"' && hadLeadingQuote) {
      *dst = '\0'; // Trailing '"' and remaining string is ignored
      break;
    }

    if (c == '\\') {
      c = *src++ & 0xff;
      for (PINDEX i = 0; i < PARRAYSIZE(PStringEscapeCode); i++) {
        if (c == PStringEscapeCode[i])
          c = PStringEscapeValue[i];
      }

      if (c == 'x' && isxdigit(*src & 0xff)) {
        c = TranslateHex(*src++);
        if (isxdigit(*src & 0xff))
          c = (c << 4) + TranslateHex(*src++);
      }
      else if (c >= '0' && c <= '7') {
        int count = c <= '3' ? 3 : 2;
        src--;
        c = 0;
        do {
          c = (c << 3) + *src++ - '0';
        } while (--count > 0 && *src >= '0' && *src <= '7');
      }
    }

    *dst++ = (char)c;
  }
}


PString::PString(ConversionType type, const char * str, ...)
  : m_caseless(false)
{
  switch (type) {
    case Pascal :
      if (*str != '\0') {
        PINDEX length = *str & 0xff;
        assign(str+1, str+length+1);
      }
      break;

    case Basic :
      if (str[0] != '\0' && str[1] != '\0') {
        PINDEX length = (str[0] & 0xff) | ((str[1] & 0xff) << 8);
        assign(str+2, str+length+2);
      }
      break;

    case Literal :
      reserve(strlen(str)+1);
      TranslateEscapes(str, const_cast<char *>(data()));
      MakeMinimumSize();
      break;

    case Printf : {
      va_list args;
      va_start(args, str);
      vsprintf(str, args);
      va_end(args);
      break;
    }

    default :
      PAssertAlways(PInvalidParameter);
  }
}


template <typename T> void p_unsigned2string(T value, unsigned base, std::string & str)
{
  if (value >= base)
    p_unsigned2string<T>((T)(value/base), base, str);
  value %= base;
  str += (char)(value < 10 ? (value + '0') : (value + 'A'-10));
}


template <typename S, typename U> void p_signed2string(S value, unsigned base, std::string & str)
{
  if (value >= 0)
    p_unsigned2string<U>(value, base, str);
  else {
    str += '-';
    p_unsigned2string<U>(-value, base, str);
  }
}


PString::PString(short n)
  : m_caseless(false)
{
  p_signed2string<signed int, unsigned>(n, 10, *this);
}


PString::PString(unsigned short n)
  : m_caseless(false)
{
  p_unsigned2string<unsigned int>(n, 10, *this);
}


PString::PString(int n)
  : m_caseless(false)
{
  p_signed2string<signed int, unsigned>(n, 10, *this);
}


PString::PString(unsigned int n)
  : m_caseless(false)
{
  p_unsigned2string<unsigned int>(n, 10, *this);
}


PString::PString(long n)
  : m_caseless(false)
{
  p_signed2string<signed long, unsigned long>(n, 10, *this);
}


PString::PString(unsigned long n)
  : m_caseless(false)
{
  p_unsigned2string<unsigned long>(n, 10, *this);
}


#ifdef HAVE_LONG_LONG_INT
PString::PString(long long n)
  : m_caseless(false)
{
  p_signed2string<signed long long, unsigned long long>(n, 10, *this);
}
#endif


#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PString::PString(unsigned long long n)
  : m_caseless(false)
{
  p_unsigned2string<unsigned long long>(n, 10, *this);
}
#endif


static const char siTable[][2] = { "f", "p", "n", "u", "m", "", "k", "M", "G", "T", "E" };
static const size_t siCount = sizeof(siTable)/2;
static const size_t siZero = siCount/2;

static void InternalConvertScaleSI(int64_t value, unsigned precision, std::string & str)
{
  // Scale it according to SI multipliers
  if (value > -1000 && value < 1000)
    return p_signed2string<int64_t, uint64_t>(value, 10, str);

  if (precision > 21)
    precision = 21;

  int64_t absValue = value;
  if (absValue < 0) {
    absValue = -absValue;
    ++precision;
  }

  PINDEX length = 0;
  int64_t multiplier = 1;
  for (size_t i = siZero+1; i < siCount; ++i) {
    multiplier *= 1000;
    if (absValue < multiplier*1000) {
      p_signed2string<int64_t, uint64_t>(value/multiplier, 10, str);
      precision -= length;
      if (precision > 0 && absValue%multiplier != 0) {
        str += '.';
        do {
          multiplier /= 10;
          str += (absValue/multiplier)%10 + '0';
        } while (--precision > 0 && absValue%multiplier != 0);
      }
      str += siTable[i][0];
      break;
    }
  }
}

template <typename S, typename U>
  void p_convert(PString::ConversionType type, S value, unsigned param, std::string & str)
{
#define GetClass() NULL
  switch (type) {
    case PString::Signed :
      PAssert(param != 1 && param <= 36, PInvalidParameter);
      return p_signed2string<S, U>(value, param, str);

    case PString::Unsigned :
      PAssert(param != 1 && param <= 36, PInvalidParameter);
      return p_unsigned2string<U>(value, param, str);

    case PString::ScaleSI :
      return InternalConvertScaleSI(value, param, str);

    default :
      break;
  }

  PAssertAlways(PInvalidParameter);
#undef GetClass
}

#define PSTRING_CONV_CTOR(paramType, signedType, unsignedType) \
PString::PString(ConversionType type, paramType value, unsigned param) \
  : m_caseless(false) \
{ \
  p_convert<signedType, unsignedType>(type, value, param, *this); \
}

PSTRING_CONV_CTOR(unsigned char,  char,   unsigned char);
PSTRING_CONV_CTOR(         short, short,  unsigned short);
PSTRING_CONV_CTOR(unsigned short, short,  unsigned short);
PSTRING_CONV_CTOR(         int,   int,    unsigned int);
PSTRING_CONV_CTOR(unsigned int,   int,    unsigned int);
PSTRING_CONV_CTOR(         long,  long,   unsigned long);
PSTRING_CONV_CTOR(unsigned long,  long,   unsigned long);
#ifdef HAVE_LONG_LONG_INT
PSTRING_CONV_CTOR(         long long, long long, unsigned long long);
#endif
#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PSTRING_CONV_CTOR(unsigned long long, long long, unsigned long long);
#endif


PString::PString(ConversionType type, double value, unsigned places)
  : m_caseless(false)
{
  switch (type) {
    case Decimal :
      sprintf("%0.*f", (int)places, value);
      break;

    case Exponent :
      sprintf("%0.*e", (int)places, value);
      break;

    case ScaleSI :
      if (value == 0)
        sprintf("%0.*f", (int)places, value);
      else {
        // Scale it according to SI multipliers
        double multiplier = 1e-15;
        double absValue = fabs(value);
        size_t i;
        for (i = 0; i < siCount-1; ++i) {
          double nextMultiplier = multiplier * 1000;
          if (absValue < nextMultiplier)
            break;
          multiplier = nextMultiplier;
        }
        value /= multiplier;
        // Want places to be significant figures
        if (places >= 3 && value >= 100)
          places -= 3;
        else if (places >= 2 && value >= 10)
          places -= 2;
        else if (places >= 1)
          --places;
        sprintf("%0.*f%s", (int)places, value, siTable[i]);
      }
      break;

    default :
      PAssertAlways(PInvalidParameter);
      MakeEmpty();
  }
}


PString & PString::operator=(const PString & str)
{
  std::string::operator=(str);
  m_caseless = str.m_caseless;
  return *this;
}


PString & PString::operator=(const PCaselessString & str)
{
  std::string::operator=(str);
  m_caseless = str.m_caseless;
  return *this;
}


PString & PString::operator=(short n)
{
  clear();
  p_signed2string<signed int, unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned short n)
{
  clear();
  p_unsigned2string<unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(int n)
{
  clear();
  p_signed2string<signed int, unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned int n)
{
  clear();
  p_unsigned2string<unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(long n)
{
  clear();
  p_signed2string<signed long,  unsigned long>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned long n)
{
  clear();
  p_unsigned2string<unsigned long>(n, 10, *this);
  return *this;
}


#ifdef HAVE_LONG_LONG_INT
PString & PString::operator=(long long n)
{
  clear();
  p_signed2string<signed long long, unsigned long long>(n, 10, *this);
  return *this;
}
#endif


#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PString & PString::operator=(unsigned long long n)
{
  clear();
  p_unsigned2string<unsigned long long>(n, 10, *this);
  return *this;
}
#endif


PObject * PString::Clone() const
{
  return new PString(*this);
}


void PString::PrintOn(ostream &strm) const
{
  strm << c_str();
}


void PString::ReadFrom(istream &strm)
{
  std::getline(strm, *this);
}


PObject::Comparison PString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PString), PInvalidCast);
  return InternalCompare(0, P_MAX_INDEX, ((const PString &)obj).c_str());
}


PINDEX PString::HashFunction() const
{
  /* Hash function from "Data Structures and Algorithm Analysis in C++" by
     Mark Allen Weiss, with limit of only executing over at most the first and
     last characters to increase speed when dealing with large strings. */

  PINDEX length = GetLength(); // Use virtual function so PStringStream recalculates length
  switch (length) {
    case 0:
      return 0;
    case 1:
      return tolower(at(0) & 0xff) % 127;
  }

  static const PINDEX MaxCount = 18; // Make sure big enough to cover whole PGloballyUniqueID::AsString()
  PINDEX count = std::min(length / 2, MaxCount);
  unsigned hash = 0;
  PINDEX i;
  for (i = 0; i < count; i++)
    hash = (hash << 5) ^ tolower(at(i) & 0xff) ^ hash;
  for (i = length - count - 1; i < length; i++)
    hash = (hash << 5) ^ tolower(at(i) & 0xff) ^ hash;
  return hash % 127;
}


PString PString::operator+(const char * cstr) const
{
  if (!cstr)
    return *this;

  PString str = *this;
  str += cstr;
  return str;
}


PString PString::operator+(char c) const
{
  PString str = *this;
  str += c;
  return str;
}


PString & PString::operator+=(const char * cstr)
{
  if (!cstr)
    return *this;

  std::string::operator+=(cstr);
  return *this;
}


PString & PString::operator+=(char ch)
{
  std::string::operator+=(ch);
  return *this;
}


PString PString::operator&(const char * cstr) const
{
  PString str;
  str &= cstr;
  return str;
}


PString PString::operator&(char c) const
{
  PString str;
  str &= c;
  return str;
}


PString & PString::operator&=(const char * cstr)
{
  if (cstr == NULL)
    return *this;

  PINDEX alen = strlen(cstr);
  if (alen == 0)
    return *this;

  PINDEX olen = GetLength();
  if (olen > 0 && at(olen-1) != ' ' && *cstr != ' ')
    std::string::operator+=(' ');
  std::string::operator+=(cstr);
  return *this;
}


PString & PString::operator&=(char ch)
{
  PINDEX olen = GetLength();
  if (olen > 0 && at(olen-1) != ' ' && ch != ' ')
    std::string::operator+=(' ');
  std::string::operator+=(ch);
  return *this;
}


PString & PString::Delete(PINDEX start, PINDEX len)
{
  std::string::erase(start, len);
  return *this;
}


PString PString::operator()(PINDEX start, PINDEX end) const
{
#if PINDEX_SIGNED
  if (end < 0 || start < 0)
    return Empty();
#endif

  if (end < start)
    return Empty();

  PINDEX len = GetLength();
  if (start > len)
    return Empty();

  if (end >= len) {
    if (start == 0)
      return *this;
    end = len-1;
  }

  return PString(substr(start, end - start + 1));
}


PString PString::Left(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  if (len >= GetLength())
    return *this;

  return PString(substr(0, len));
}


PString PString::Right(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  PINDEX srclen = GetLength();
  if (len >= srclen)
    return *this;

  return PString(substr(srclen - len, len));
}


PString PString::Mid(PINDEX start, PINDEX len) const
{
#if PINDEX_SIGNED
  if (len <= 0 || start < 0)
#else
  if (len == 0)
#endif
    return Empty();

  if (len == P_MAX_INDEX || start+len < start) // If open ended or check for wraparound
    return operator()(start, P_MAX_INDEX);
  else
    return operator()(start, start+len-1);
}

PString PString::Ellipsis(PINDEX maxLength, PINDEX fromEnd, bool ansi) const
{
  if (GetLength() <= maxLength)
    return *this;

  if (ansi) {
    maxLength -= 3;
    if (fromEnd > maxLength)
      fromEnd = maxLength;

    return Left(maxLength-fromEnd) + "..." + Right(fromEnd);
  }

  PWCharArray wide = AsWide();
  PINDEX wideLength = wide.GetSize()-1;
  if (wideLength <= maxLength)
    return *this;

  --maxLength;
  if (fromEnd > maxLength)
    fromEnd = maxLength;

  PString left(wide.GetPointer(), maxLength - fromEnd);
  static PConstString const ellipsis("\xE2\x80\xA6");
  PString right(wide.GetPointer()+wideLength-fromEnd, fromEnd);
  return left + ellipsis + right;
}


bool PString::operator*=(const char * cstr) const
{
  if (cstr == NULL)
    return IsEmpty();

  return strcasecmp(c_str(), cstr) == 0;
}


PObject::Comparison PString::NumCompare(const PString & str, PINDEX count, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0 || count < 0)
    return LessThan;
#endif

  return InternalCompare(std::min(offset, GetLength()), std::min(count, str.GetLength()), str);
}


PObject::Comparison PString::NumCompare(const char * cstr, PINDEX count, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0 || count < 0)
    return LessThan;
#endif

  return InternalCompare(std::min(offset, GetLength()), std::min(count, PINDEX(::strlen(cstr))), cstr);
}


PObject::Comparison PString::InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const
{
#if PINDEX_SIGNED
  if (offset < 0 || length < 0)
    return LessThan;
#endif

  if (offset == 0 && c_str() == cstr)
    return EqualTo;

  const char * s1 = c_str()+offset;
  const char * s2 = cstr != NULL ? cstr : "";
  int comparison;
  if (m_caseless)
    comparison = length != P_MAX_INDEX ? strncasecmp(s1, s2, length) : strcasecmp(s1, s2);
  else
    comparison = length != P_MAX_INDEX ? strncmp(s1, s2, length) : strcmp(s1, s2);
  return Compare2(comparison, 0);
}


PINDEX PString::Find(char ch, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX len = GetLength();
  while (offset < len) {
    if (InternalCompare(offset, 1, &ch) == EqualTo)
      return offset;
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::Find(const char * cstr, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cstr == NULL || *cstr == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    return P_MAX_INDEX;

  if (len - clen < 10) {
    while (offset+clen <= len) {
      if (InternalCompare(offset, clen, cstr) == EqualTo)
        return offset;
      offset++;
    }
    return P_MAX_INDEX;
  }

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(at(offset+i) & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (offset+clen < len) {
    if (strSum == cstrSum && InternalCompare(offset, clen, cstr) == EqualTo)
      return offset;
    strSum += toupper(at(offset+clen) & 0xff);
    strSum -= toupper(at(offset) & 0xff);
    offset++;
  }

  return P_MAX_INDEX;
}


PINDEX PString::FindLast(char ch, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX len = GetLength();
  if (len == 0)
    return P_MAX_INDEX;

  if (offset >= len)
    offset = len-1;

  while (InternalCompare(offset, 1, &ch) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    offset--;
  }

  return offset;
}


PINDEX PString::FindLast(const char * cstr, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cstr == NULL || *cstr == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    offset = len - clen;

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(at(offset+i) & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (strSum != cstrSum || InternalCompare(offset, clen, cstr) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    --offset;
    strSum += toupper(at(offset) & 0xff);
    strSum -= toupper(at(offset+clen) & 0xff);
  }

  return offset;
}


PINDEX PString::FindOneOf(const char * cset, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cset == NULL || *cset == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (*p != '\0') {
      if (InternalCompare(offset, 1, p) == EqualTo)
        return offset;
      p++;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindSpan(const char * cset, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cset == NULL || *cset == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (InternalCompare(offset, 1, p) != EqualTo) {
      if (*++p == '\0')
        return offset;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindRegEx(const PRegularExpression & regex, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX pos = 0;
  PINDEX len = 0;
  if (FindRegEx(regex, pos, len, offset))
    return pos;

  return P_MAX_INDEX;
}


bool PString::FindRegEx(const PRegularExpression & regex,
                        PINDEX & pos,
                        PINDEX & len,
                        PINDEX offset,
                        PINDEX maxPos) const
{
#if PINDEX_SIGNED
  if (offset < 0 || maxPos < 0)
    return false;
#endif

  PINDEX olen = GetLength();
  if (offset > olen)
    return false;

  if (offset == olen) {
    if (!regex.Execute("", pos, len))
      return false;
  }
  else {
    if (!regex.Execute(c_str() + offset, pos, len))
      return false;
  }

  pos += offset;
  if (pos+len > maxPos)
    return false;

  return true;
}

bool PString::MatchesRegEx(const PRegularExpression & regex) const
{
  PINDEX pos = 0;
  PINDEX len = 0;

  if (!regex.Execute(c_str(), pos, len))
    return false;

  return (pos == 0) && (len == GetLength());
}

PString & PString::Replace(const PString & target, const PString & subs, bool all, PINDEX offset)
{
#if PINDEX_SIGNED
  if (offset < 0)
    return *this;
#endif
    
  PINDEX tlen = target.GetLength();
  PINDEX slen = subs.GetLength();
  do {
    PINDEX pos = Find(target, offset);
    if (pos == P_MAX_INDEX)
      return *this;
    Splice(subs, pos, tlen);
    offset = pos + slen;
  } while (all);

  return *this;
}


PString & PString::Splice(const char * cstr, PINDEX pos, PINDEX len)
{
#if PINDEX_SIGNED
  if (len < 0 || pos < 0)
    return *this;
#endif

  if (pos >= GetLength())
    return operator+=(cstr);

  if (len > 0)
    erase(pos, len);
  insert(pos, cstr);
  return *this;
}


bool PString::InternalSplit(const PString & delimiter, PString & before, PString & after, SplitOptions_Bits options) const
{
  PINDEX pos = Find(delimiter);

  if (pos != P_MAX_INDEX) {
    before = (options&SplitTrimBefore) ? Left(pos).Trim() : Left(pos);
    pos += delimiter.GetLength();
    after = (options&SplitTrimAfter) ? Mid(pos).Trim() : Mid(pos);
  }
  else {
    if (!(options&(SplitDefaultToBefore|SplitDefaultToAfter)))
      return false;

    if (options&SplitDefaultToBefore)
      before = (options&SplitTrimBefore) ? Trim() : *this;

    if (options&SplitDefaultToAfter)
      after = (options&SplitTrimBefore) ? Trim() : *this;
  }

  if (before.IsEmpty() && (options&SplitBeforeNonEmpty))
    return false;

  if (after.IsEmpty() && (options&SplitAfterNonEmpty))
    return false;

  return true;
}


PStringArray PString::Tokenise(const char * separators, bool onePerSeparator) const
{
  PStringArray tokens;
  
  if (separators == NULL || IsEmpty())  // No tokens
    return tokens;

  PINDEX p1 = 0;
  PINDEX p2 = FindOneOf(separators);

  if (p2 == 0) {
    if (onePerSeparator) { // first character is a token separator
      tokens.push_back(Empty());
      p1 = 1;
      p2 = FindOneOf(separators, 1);
    }
    else {
      do {
        p1 = p2 + 1;
      } while ((p2 = FindOneOf(separators, p1)) == p1);
    }
  }

  while (p2 != P_MAX_INDEX) {
    if (p2 > p1)
      tokens.push_back(operator()(p1, p2-1));
    else
      tokens.push_back(Empty());

    // Get next separator. If not one token per separator then continue
    // around loop to skip over all the consecutive separators.
    do {
      p1 = p2 + 1;
    } while ((p2 = FindOneOf(separators, p1)) == p1 && !onePerSeparator);
  }

  tokens.push_back(operator()(p1, P_MAX_INDEX));

  return tokens;
}


PStringArray PString::Lines() const
{
  PStringArray lines;
  
  if (IsEmpty())
    return lines;

  PINDEX p1 = 0;
  PINDEX p2;
  while ((p2 = FindOneOf("\r\n", p1)) != P_MAX_INDEX) {
    lines.push_back(operator()(p1, p2-1));
    p1 = p2 + 1;
    if (at(p2) == '\r' && at(p1) == '\n') // CR LF pair
      p1++;
  }
  if (p1 < GetLength())
    lines.push_back(operator()(p1, P_MAX_INDEX));
  return lines;
}


PString PString::LeftTrim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * lpos = c_str();
  while (std::isspace(*lpos, std::locale("C")))
    lpos++;
  return PString(lpos);
}


PString PString::RightTrim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * rpos = c_str()+GetLength()-1;
  if (!std::isspace(*rpos, std::locale("C")))
    return *this;

  while (std::isspace(*rpos, std::locale("C"))) {
    if (rpos == c_str())
      return Empty();
    rpos--;
  }

  // make Apple & Tornado gnu compiler happy
  PString retval(c_str(), rpos - c_str() + 1);
  return retval;
}


PString PString::Trim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * lpos = c_str();
  while (std::isspace(*lpos, std::locale("C")))
    lpos++;
  if (*lpos == '\0')
    return Empty();

  const char * rpos = c_str()+GetLength()-1;
  if (!std::isspace(*rpos, std::locale("C"))) {
    if (lpos == c_str())
      return *this;
    else
      return PString(lpos);
  }

  while (std::isspace(*rpos, std::locale("C")))
    rpos--;
  return PString(lpos, rpos - lpos + 1);
}


PString PString::ToLower() const
{
  PString newStr = *this;
  for (auto & it : newStr)
    it = std::tolower(it, std::locale("C"));
  return newStr;
}


PString PString::ToUpper() const
{
  PString newStr = *this;
  for (auto& it : newStr)
    it = std::toupper(it, std::locale("C"));
  return newStr;
}


long PString::AsInteger(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtol(c_str(), &dummy, base);
}


uint32_t PString::AsUnsigned(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoul(c_str(), &dummy, base);
}


int64_t PString::AsInteger64(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoll(c_str(), &dummy, base);
}


uint64_t PString::AsUnsigned64(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoull(c_str(), &dummy, base);
}


double PString::AsReal() const
{
#ifndef __HAS_NO_FLOAT
  char * dummy;
  return strtod(c_str(), &dummy);
#else
  return 0.0;
#endif
}


PWCharArray PString::AsWide() const
{
  if (IsEmpty())
    return PWCharArray(1); // Null terminated empty string

  std::wstring_convert<std::codecvt_utf8<wchar_t>> ucs2conv;
  auto wide = ucs2conv.from_bytes(*this);
  PTRACE_IF(2, ucs2conv.converted() < length(), "Error to converting UTF-8 (" << length() << ") to UCS-2 (" << ucs2conv.converted() << ')');
  return PWCharArray(wide.c_str(), wide.length()+1);
}


void PString::InternalFromWChar(const wchar_t * wstr, PINDEX len)
{
  if (wstr == NULL || len <= 0)
    MakeEmpty();
  else {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> ucs2conv;
    assign(ucs2conv.to_bytes(wstr, wstr+len));
    PTRACE_IF(2, ucs2conv.converted() < size_t(len), "Error converting UCS-2 (" << len << ") to UTF-8 (" << ucs2conv.converted() << ')');
  }
}


PBYTEArray PString::ToPascal() const
{
  PINDEX len = GetLength();
  PAssert(len < 256, "Cannot convert to PASCAL string");
  uint8_t buf[256];
  buf[0] = (uint8_t)len;
  memcpy(&buf[1], c_str(), len);
  return PBYTEArray(buf, len);
}


PString PString::ToLiteral(bool ascii) const
{
  PStringStream strm;
  for (const char * p = c_str(); *p != '\0'; p++) {
    if (*p == '"')
      strm << "\\\"";
    else if (*p == '\\')
      strm << "\\\\";
    else if (isprint(*p & 0xff) || (!ascii && (*p & 0x80) != 0))
      strm << *p;
    else {
      PINDEX i;
      for (i = 0; i < PARRAYSIZE(PStringEscapeValue); i++) {
        if (*p == PStringEscapeValue[i]) {
          strm << '\\' << (char)PStringEscapeCode[i];
          break;
        }
      }
      if (i >= PARRAYSIZE(PStringEscapeValue))
        strm << '\\' << std::oct << setfill('0') << setw(3) << (*p & 0xff);
    }
  }
  strm << '"';
  return strm;
}


PString PString::FromLiteral(PINDEX & offset) const
{
  if (offset >= GetLength())
    return PString::Empty();

  PString str;
  const char * cstr = c_str() + offset;
  TranslateEscapes(cstr, str.GetPointerAndSetLength(GetLength()-offset));
  str.MakeMinimumSize();
  offset = cstr - c_str();

  return str;
}


bool PString::IsTrue() const
{
  return IsTrue(c_str());
}


bool PString::IsTrue(const char * str)
{
  if (str == NULL || *str == '\0')
    return false;

  return strcasecmp(str, "true") == 0 ||
         strcasecmp(str, "t") == 0 ||
         strcasecmp(str, "on") == 0 ||
         strcasecmp(str, "yes") == 0 ||
         strcasecmp(str, "y") == 0 ||
         strcasecmp(str, "enabled") == 0 ||
         atoi(str) != 0;
}


bool PString::IsFalse() const
{
  return IsFalse(c_str());
}


bool PString::IsFalse(const char * str)
{
  if (str == NULL || *str == '\0')
    return false;

  return strcasecmp(str, "false") == 0 ||
         strcasecmp(str, "f") == 0 ||
         strcasecmp(str, "off") == 0 ||
         strcasecmp(str, "no") == 0 ||
         strcasecmp(str, "n") == 0 ||
         strcasecmp(str, "disabled") == 0 ||
         strspn(str, "-0") == strlen(str);
}


PString & PString::sprintf(const char * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  return vsprintf(fmt, args);
}

#if defined(__GNUC__) || defined(__SUNPRO_CC)
#define _vsnprintf vsnprintf
#endif

PString & PString::vsprintf(const char * fmt, va_list arg)
{
  PINDEX len = GetLength();
  int providedSpace = 0;
  int requiredSpace;
  do {
    providedSpace += 1000;
    requiredSpace = vsnprintf(GetPointerAndSetLength(providedSpace+len), providedSpace-1, fmt, arg);
  } while (requiredSpace == -1 || requiredSpace >= providedSpace);
  resize(len + requiredSpace);

  return *this;
}


PString psprintf(const char * fmt, ...)
{
  PString str;
  va_list args;
  va_start(args, fmt);
  return str.vsprintf(fmt, args);
}


PString pvsprintf(const char * fmt, va_list arg)
{
  PString str;
  return str.vsprintf(fmt, arg);
}


bool PString::MakeMinimumSize(PINDEX newLength)
{
  if (newLength == 0)
    newLength = strlen(c_str());
  resize(newLength);
  return true;
}


char * PString::GetPointerAndSetLength(PINDEX len)
{
  resize(len);
  return const_cast<char *>(data());
}


///////////////////////////////////////////////////////////////////////////////

PObject * PCaselessString::Clone() const
{
  return new PCaselessString(*this);
}


///////////////////////////////////////////////////////////////////////////////

PStringArray::PStringArray(PINDEX count, char const * const * strarr, bool caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  if (count == P_MAX_INDEX) {
    count = 0;
    while (strarr[count] != NULL)
      count++;
  }

  reserve(count);
  for (PINDEX i = 0; i < count; i++) {
    if (caseless)
      push_back(PCaselessString(strarr[i]));
    else
      emplace_back(strarr[i]);
  }
}


PStringArray PStringArray::operator+(const PStringArray & v) const
{
  PStringArray arr = *this;
  arr += v;
  return arr;
}


PStringArray PStringArray::operator+(const PString & v) const
{
  PStringArray arr = *this;
  arr += v;
  return arr;
}


void PStringArray::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PObject * PStringArray::Clone() const
{
  return new PStringArray(*this);
}


PINDEX PStringArray::GetStringsIndex(const PString & str) const
{
  PINDEX index = 0;
  for (const auto & it : *this) {
    if (it == str)
      return index;
    ++index;
  }
  return P_MAX_INDEX;
}


static void strcpy_with_increment(char * & strPtr, const PString & str)
{
  PINDEX len = str.GetLength()+1;
  memcpy(strPtr, (const char *)str, len);
  strPtr += len;
}

char ** PStringArray::ToCharArray(PCharArray * storage) const
{
  PINDEX i;

  PINDEX mySize = GetSize();
  PINDEX storageSize = (mySize+1)*sizeof(char *);
  for (i = 0; i < mySize; i++)
    storageSize += (*this)[i].GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[mySize+1];

  for (i = 0; i < mySize; i++) {
    storagePtr[i] = strPtr;
    strcpy_with_increment(strPtr, (*this)[i]);
  }

  storagePtr[i] = NULL;

  return storagePtr;
}


PString PStringArray::ToString(char separator) const
{
  PStringStream str;

  for (PINDEX i = 0; i < GetSize(); ++i) {
    if (i > 0)
      str << separator;
    str << (*this)[i];
  }

  return str;
}


///////////////////////////////////////////////////////////////////////////////

PStringList::PStringList(PINDEX count, char const * const * strarr, bool caseless)
{
  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count && strarr[count] != NULL; i++) {
    if (caseless)
      push_back(PCaselessString(strarr[i]));
    else
      emplace_back(strarr[i]);
  }
}


void PStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PObject * PStringList::Clone() const
{
  return new PStringList(*this);
}


PINDEX PStringList::GetStringsIndex(const PString & str) const
{
  PINDEX index = 0;
  for (const auto & it : *this) {
    if (it == str)
      return index;
    ++index;
  }
  return P_MAX_INDEX;
}

///////////////////////////////////////////////////////////////////////////////

PSortedStringList::PSortedStringList(PINDEX count,
                                     char const * const * strarr,
                                     bool caseless)
{
  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count && strarr[count] != NULL; i++) {
    if (caseless)
      insert(PCaselessString(strarr[i]));
    else
      emplace(strarr[i]);
  }
}


void PSortedStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PObject * PSortedStringList::Clone() const
{
  return new PSortedStringList(*this);
}


///////////////////////////////////////////////////////////////////////////////

PStringSet::PStringSet(PINDEX count, char const * const * strarr, bool caseless)
{
  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count && strarr[i] != NULL; i++) {
    if (caseless)
      insert(PCaselessString(strarr[i]));
    else
      emplace(strarr[i]);
  }
}


void PStringSet::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    Include(str);
  }
}

PObject * PStringSet::Clone() const
{
  return new PStringSet(*this);
}


///////////////////////////////////////////////////////////////////////////////

PStringToString::PStringToString(PINDEX count,
                                 const Initialiser * init,
                                 bool caselessKeys,
                                 bool caselessValues)
{
  if (PAssertNULL(init) == NULL)
    return;

  while (count-- > 0) {
    if (caselessValues)
      if (caselessKeys)
        SetAt(PCaselessString(init->key), PCaselessString(init->value));
      else
        SetAt(init->key, PCaselessString(init->value));
    else
      if (caselessKeys)
        SetAt(PCaselessString(init->key), init->value);
      else
        SetAt(init->key, init->value);
    init++;
  }
}


void PStringToString::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    if (str.IsEmpty())
      continue;

    PString key, value;
    str.Split('=', key, value, PString::SplitDefaultToBefore);

    auto it = find(key);
    if (it != end())
      it->second += '\n' + value;
    else
      at(key) = value;
  }
}


void PStringToString::FromString(const PString & str)
{
  RemoveAll();

  PStringStream strm(str);
  strm >> *this;
}


void PStringToString::Merge(const PStringToString & other, MergeAction action)
{
  for (const_iterator it = other.begin(); it != other.end(); ++it) {
    auto str = find(it->first);
    if (str == end() || action == e_MergeOverwrite)
      SetAt(it->first, it->second);
    else if (action == e_MergeAppend)
      str->second += '\n' + it->second;
  }
}


char ** PStringToString::ToCharArray(bool withEqualSign, PCharArray * storage) const
{
  const_iterator it;

  PINDEX mySize = GetSize();
  PINDEX numPointers = mySize*(withEqualSign ? 1 : 2) + 1;
  PINDEX storageSize = numPointers*sizeof(char *);
  for (it = begin(); it != end(); ++it)
    storageSize += it->first.GetLength()+1 + it->second.GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[numPointers];
  PINDEX strIndex = 0;

  for (it = begin(); it != end(); ++it) {
    storagePtr[strIndex++] = strPtr;
    if (withEqualSign)
      strcpy_with_increment(strPtr, it->first + '=' + it->second);
    else {
      strcpy_with_increment(strPtr, it->first);
      storagePtr[strIndex++] = strPtr;
      strcpy_with_increment(strPtr, it->second);
    }
  }

  storagePtr[strIndex] = NULL;

  return storagePtr;
}


///////////////////////////////////////////////////////////////////////////////


PString PStringOptions::GetString(const PCaselessString & key, const char * dflt) const
{
  auto it = find(key);
  return it != end() ? it->second : PString(dflt);
}


bool PStringOptions::GetBoolean(const PCaselessString & key, bool dflt) const
{
  auto it = find(key);
  return it != end() ? it->second.IsTrue() : dflt;
}


long PStringOptions::GetInteger(const PCaselessString & key, long dflt) const
{
  auto it = find(key);
  return it != end() ? it->second.AsInteger() : dflt;
}


void PStringOptions::SetInteger(const PCaselessString & key, long value)
{
  PStringToString::SetAt(key, PString(PString::Signed, value));
}


double PStringOptions::GetReal(const PCaselessString & key, double dflt) const
{
  auto it = find(key);
  return it != end() ? it->second.AsReal() : dflt;
}


void PStringOptions::SetReal(const PCaselessString & key, double value, int decimals)
{
  PStringToString::SetAt(key, PString(decimals < 0 ? PString::Exponent : PString::Decimal, value, decimals));
}


///////////////////////////////////////////////////////////////////////////////

PRegularExpression::PRegularExpression()
  : m_lastErrorCode(NotCompiled)
  , m_lastErrorType(std::regex_constants::error_stack)
{
}


PRegularExpression::PRegularExpression(const PString & pattern, CompileOptions options)
  : PRegularExpression(pattern.c_str(), options)
{
}


PRegularExpression::PRegularExpression(const char * pattern, CompileOptions options)
  : m_pattern(pattern)
{
  PAssert(Compile(pattern, options), PSTRSTRM("Regular expression " << m_pattern.ToLiteral() << " failed to compile: " << GetErrorText()));
}


void PRegularExpression::PrintOn(ostream &strm) const
{
  strm << m_pattern;
}


std::regex::flag_type PRegularExpression::MapFlags(CompileOptions opts)
{
  flag_type flags = basic;
  if (opts&Extended)
    flags |= extended;
  if (opts&IgnoreCase)
    flags |= icase;
  return flags;
}


PRegularExpression::match_flag PRegularExpression::MapFlags(ExecOptions opts)
{
  match_flag flags = std::regex_constants::match_default;
  if (opts&NotBeginningOfLine)
    flags |= std::regex_constants::match_not_bol;
  if (opts&NotEndofLine)
    flags |= std::regex_constants::match_not_eol;
  return flags;
}


PString PRegularExpression::GetErrorText() const
{
  switch (m_lastErrorType) {
    case std::regex_constants::error_collate:
      return "the expression contains an invalid collating element name";
    case std::regex_constants::error_ctype:
      return "the expression contains an invalid character class name";
    case std::regex_constants::error_escape:
      return "the expression contains an invalid escaped character or a trailing escape";
    case std::regex_constants::error_backref:
      return "the expression contains an invalid back reference";
    case std::regex_constants::error_brack:
      return "the expression contains mismatched square brackets ('[' and ']')";
    case std::regex_constants::error_paren:
      return "the expression contains mismatched parentheses ('(' and ')')";
    case std::regex_constants::error_brace:
      return "the expression contains mismatched curly braces ('{' and '}')";
    case std::regex_constants::error_badbrace:
      return "the expression contains an invalid range in a {} expression";
    case std::regex_constants::error_range:
      return "the expression contains an invalid character range (e.g. [b-a])";
    case std::regex_constants::error_space:
      return "there was not enough memory to convert the expression into a finite state machine";
    case std::regex_constants::error_badrepeat:
      return "one of *?+{ was not preceded by a valid regular expression";
    case std::regex_constants::error_complexity:
      return "the complexity of an attempted match exceeded a predefined level";
    case std::regex_constants::error_stack:
      return "there was not enough memory to perform a match";
    default :
      return "unknown error";
  }
}


bool PRegularExpression::Compile(const PString & pattern, CompileOptions options)
{
  return Compile(pattern, MapFlags(options));
}


bool PRegularExpression::Compile(const char * pattern, CompileOptions options)
{
  return Compile(pattern, MapFlags(options));
}


bool PRegularExpression::Compile(const PString & pattern, flag_type flags)
{
  return Compile(pattern.c_str(), flags);
}


bool PRegularExpression::Compile(const char * pattern, flag_type flags)
{
  m_pattern = pattern;
  if (m_pattern.IsEmpty()) {
    m_lastErrorCode = NotCompiled;
    return false;
  }

  try {
    assign(pattern, flags);
    m_lastErrorCode = NoError;
    return true;
  }
  catch (const std::regex_error& e) {
    m_lastErrorType = e.code();
  }

  switch (m_lastErrorType) {
    case std::regex_constants::error_collate:
      m_lastErrorCode = CollateError;
      break;
    case std::regex_constants::error_ctype:
      m_lastErrorCode = BadClassType;
      break;
    case std::regex_constants::error_escape:
      m_lastErrorCode = BadEscape;
      break;
    case std::regex_constants::error_backref:
      m_lastErrorCode = BadSubReg;
      break;
    case std::regex_constants::error_brack:
      m_lastErrorCode = UnmatchedBracket;
      break;
    case std::regex_constants::error_paren:
      m_lastErrorCode = UnmatchedParen;
      break;
    case std::regex_constants::error_brace:
      m_lastErrorCode = UnmatchedBrace;
      break;
    case std::regex_constants::error_badbrace:
      m_lastErrorCode = BadBR;
      break;
    case std::regex_constants::error_range:
      m_lastErrorCode = RangeError;
      break;
    case std::regex_constants::error_stack:
    case std::regex_constants::error_space:
      m_lastErrorCode = OutOfMemory;
      break;
    case std::regex_constants::error_badrepeat:
      m_lastErrorCode = BadRepitition;
      break;
    case std::regex_constants::error_complexity:
      m_lastErrorCode = TooBig;
      break;
    default :
      m_lastErrorCode = BadPattern;
  }

  return false;
}


bool PRegularExpression::Execute(const PString & str, PINDEX & start, ExecOptions options) const
{
  PINDEX dummy;
  return Execute((const char *)str, start, dummy, options);
}


bool PRegularExpression::Execute(const PString & str, PINDEX & start, PINDEX & len, ExecOptions options) const
{
  return Execute((const char *)str, start, len, options);
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, ExecOptions options) const
{
  PINDEX dummy;
  return Execute(cstr, start, dummy, options);
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, PINDEX & len, ExecOptions options) const
{
  return Execute(cstr, start, len, MapFlags(options));
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, match_flag flags) const
{
  PINDEX dummy;
  return Execute(cstr, start, dummy, flags);
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, PINDEX & len, match_flag flags) const
{
  if (m_lastErrorCode != NoError && m_lastErrorCode != NoMatch)
    return false;

  std::match_results<const char *> match;
  if (!std::regex_search(cstr, match, *this, flags)) {
    m_lastErrorCode = NoMatch;
    return false;
  }

  m_lastErrorCode = NoError;
  start = match[0].first - cstr;
  len = match[0].second - match[0].first + 1;
  return true;
}


bool PRegularExpression::Execute(const PString & str, PIntArray & starts, ExecOptions options) const
{
  PIntArray dummy;
  return Execute((const char *)str, starts, dummy, options);
}


bool PRegularExpression::Execute(const PString & str,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 ExecOptions options) const
{
  return Execute((const char *)str, starts, ends, options);
}


bool PRegularExpression::Execute(const char * cstr, PIntArray & starts, ExecOptions options) const
{
  PIntArray dummy;
  return Execute(cstr, starts, dummy, options);
}


bool PRegularExpression::Execute(const char * cstr,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 ExecOptions options) const
{
  return Execute(cstr, starts, ends, MapFlags(options));
}


bool PRegularExpression::Execute(const char * cstr, PIntArray & starts, match_flag flags) const
{
  PIntArray dummy;
  return Execute(cstr, starts, dummy, flags);
}


bool PRegularExpression::Execute(const char * cstr,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 match_flag flags) const
{
  if (m_lastErrorCode != NoError && m_lastErrorCode != NoMatch)
    return false;

  std::match_results<const char *> matches;
  if (!std::regex_search(cstr, matches, *this, flags)) {
    m_lastErrorCode = NoMatch;
    return false;
  }

  m_lastErrorCode = NoError;

  starts.SetSize(matches.size());
  ends.SetSize(matches.size());

  for (PINDEX i = 0; i < starts.GetSize(); i++) {
    starts[i] = (int)(matches[i].first - cstr);
    ends[i] = (int)(matches[i].second - cstr);
  }

  return true;
}


bool PRegularExpression::Execute(const char * cstr, PStringArray & substring, ExecOptions options) const
{
  return Execute(cstr, substring, MapFlags(options));
}


bool PRegularExpression::Execute(const char * cstr, PStringArray & substring, match_flag flags) const
{
  if (m_lastErrorCode != NoError && m_lastErrorCode != NoMatch)
    return false;

  std::match_results<const char *> matches;
  if (!std::regex_search(cstr, matches, *this, flags)) {
    m_lastErrorCode = NoMatch;
    return false;
  }

  m_lastErrorCode = NoError;

  substring.SetSize(matches.size());

  for (PINDEX i = 0; i < substring.GetSize(); i++)
    substring[i] = std::string(matches[i].first, matches[i].second);

  return true;
}


PString PRegularExpression::EscapeString(const PString & str)
{
  PString translated = str;

  PINDEX lastPos = 0;
  PINDEX nextPos;
  while ((nextPos = translated.FindOneOf("\\^$+?*.[]()|{}", lastPos)) != P_MAX_INDEX) {
    translated.Splice("\\", nextPos);
    lastPos = nextPos+2;
  }

  return translated;
}


///////////////////////////////////////////////////////////////////////////////

void PPrintEnum(std::ostream & strm, int value, int begin, int end, char const * const * names)
{
  if (value < begin || value >= end)
    strm << '<' << value << '>';
  else
    strm << names[value-begin];
}


int PReadEnum(std::istream & strm, int begin, int end, char const * const * names, bool matchCase)
{
  char name[100]; // If someone has an enumeration longer than this, it deserves to fail!
  strm >> ws;
  strm.get(name, sizeof(name), ' ');
  if (strm.fail() || strm.bad())
    return end;

  size_t len = strlen(name);
  int match = end;
  for (int value = begin; value < end; ++value) {
    const char * cmp = names[value-begin];

    if ((matchCase ? strcmp(name, cmp) : strcasecmp(name, cmp)) == 0) // Exact match
      return value;

    if ((matchCase ? strncmp(name, cmp, len) : strncasecmp(name, cmp, len)) == 0) {
      if (match < end) {
        match = end; // Not unique for the length
        break;
      }
      match = value;
    }
  }

  if (match < end)
    return match;

  do {
    strm.putback(name[--len]);
  } while (len > 0);

  strm.clear();
  strm.setstate(ios::failbit);

  return end;
}


int PParseEnum(const char * str, int begin, int end, char const * const * names, bool matchCase)
{
  std::stringstream strm;
  strm.str(str);
  return PReadEnum(strm, begin, end, names, matchCase);
}


void PPrintBitwiseEnum(std::ostream & strm, unsigned bits, char const * const * names)
{
  if (bits == 0) {
    strm << *names;
    return;
  }

  ++names;

  std::streamsize width = strm.width();
  if (width > 0) {
    std::streamsize len = 0;
    unsigned bit = 1;
    const char * const * name = names;
    while (*name != NULL) {
      if (bits & bit) {
        if (len > 0)
          ++len;
        len += strlen(*name);
      }
      bit <<= 1;
      ++name;
    }
    if (width > len)
      width -= len;
    else
      width = 0;
    strm.width(0);
  }

  if (width > 0 && (strm.flags()&ios::left) == 0)
    strm << setw(width) << " ";

  bool needSpace = false;
  for (unsigned bit = 1; *names != NULL; bit <<= 1, ++names) {
    if (bits & bit) {
      if (needSpace)
        strm << ' ';
      else
        needSpace = true;
      strm << *names;
    }
  }

  if (width > 0 && (strm.flags()&ios::right) == 0)
    strm << setw(width) << " ";
}


unsigned PReadBitwiseEnum(std::istream & strm, char const * const * names, bool continueOnError)
{
  unsigned bits = 0;
  while ((continueOnError || strm.good()) && !strm.eof()) {
    char name[100]; // If someone has an enumeration longer than this, it deserves to fail!
    strm >> ws;
    strm.get(name, sizeof(name), ' ');
    if (strm.fail() || strm.bad())
      break;

    if (strcmp(name, *names) == 0)
      return 0;

    bool unknown = true;
    for (unsigned i = 1; names[i] != NULL; ++i) {
      if (strcmp(name, names[i]) == 0) {
        bits |= (1 << (i-1));
        unknown = false;
        break;
      }
    }

    if (continueOnError)
      continue;

    if (unknown) {
      size_t i = strlen(name);
      do {
        strm.putback(name[--i]);
      } while (i > 0);

      break;
    }
  }
  return bits;
}


// End Of File ///////////////////////////////////////////////////////////////
