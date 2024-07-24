/*
 * contain.inl
 *
 * Container Class Inline Function Definitions
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
 *
 * $Id: contain.inl 19266 2008-01-16 03:51:00Z rjongbloed $
 */


///////////////////////////////////////////////////////////////////////////////

PINLINE PString & PString::operator=(const PString & str)
  { std::string::operator=(str); return *this; }

PINLINE PString & PString::operator=(const std::string & str)
  { std::string::operator=(str); return *this; }

PINLINE PString & PString::operator=(const char * cstr)
{
  if (cstr != NULL)
    std::string::operator=(cstr);
  else
    clear();
  return *this;
}

PINLINE PString & PString::operator=(char ch)
  { std::string::operator=(ch); return *this; }

PINLINE PString PString::operator+(const PString & str) const
  { return operator+((const char *)str); }

PINLINE PString operator+(const char * cstr, const PString & str)
  { return PString(cstr) + str; }
  
PINLINE PString operator+(char c, const PString & str)
  { return PString(c) + str; }
  
PINLINE PString & PString::operator+=(const PString & str)
  { return operator+=((const char *)str); }

PINLINE PString PString::operator&(const PString & str) const
  { return operator&((const char *)str); }

PINLINE PString operator&(const char * cstr, const PString & str)
  { return PString(cstr) & str; }
  
PINLINE PString operator&(char c, const PString & str)
  { return PString(c) & str; }
  
PINLINE PString & PString::operator&=(const PString & str)
  { return operator&=((const char *)str); }

PINLINE bool PString::operator==(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) == EqualTo; }

PINLINE bool PString::operator!=(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) != EqualTo; }

PINLINE bool PString::operator<(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) == LessThan; }

PINLINE bool PString::operator>(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) == GreaterThan; }

PINLINE bool PString::operator<=(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) != GreaterThan; }

PINLINE bool PString::operator>=(const PString & str) const
  { return InternalCompare(0, P_MAX_INDEX, str) != LessThan; }

PINLINE bool PString::operator*=(const PString & str) const
  { return operator*=((const char *)str); }

PINLINE bool PString::operator==(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == EqualTo; }

PINLINE bool PString::operator!=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != EqualTo; }

PINLINE bool PString::operator<(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == LessThan; }

PINLINE bool PString::operator>(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == GreaterThan; }

PINLINE bool PString::operator<=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != GreaterThan; }

PINLINE bool PString::operator>=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != LessThan; }

PINLINE bool operator*=(const char * cstr, const PString & str)
  { return str *= cstr; }

PINLINE bool operator==(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) == PObject::EqualTo; }

PINLINE bool operator!=(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) != PObject::EqualTo; }

PINLINE bool operator<(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) == PObject::GreaterThan; }

PINLINE bool operator>(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) == PObject::LessThan; }

PINLINE bool operator<=(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) != PObject::LessThan; }

PINLINE bool operator>=(const char * cstr, const PString & str)
  { return str.InternalCompare(0, P_MAX_INDEX, cstr) != PObject::GreaterThan; }

PINLINE PINDEX PString::Find(const PString & str, PINDEX offset) const
  { return Find((const char *)str, offset); }

PINLINE PINDEX PString::FindLast(const PString & str, PINDEX offset) const
  { return FindLast((const char *)str, offset); }

PINLINE PINDEX PString::FindOneOf(const PString & str, PINDEX offset) const
  { return FindOneOf((const char *)str, offset); }

PINLINE PINDEX PString::FindSpan(const PString & str, PINDEX offset) const
  { return FindSpan((const char *)str, offset); }

PINLINE PString & PString::Splice(const PString & str, PINDEX pos, PINDEX len)
  { return Splice((const char *)str, pos, len); }

PINLINE PStringArray PString::Tokenise(const PString & separators, bool onePerSeparator) const
  { return Tokenise((const char *)separators, onePerSeparator); }

PINLINE PString & PString::vsprintf(const PString & fmt, va_list args)
  { return vsprintf((const char *)fmt, args); }

PINLINE PString pvsprintf(const PString & fmt, va_list args)
  { return pvsprintf((const char *)fmt, args); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PCaselessString::PCaselessString()
  : PString() { }

PINLINE PCaselessString::PCaselessString(const char * cstr)
  : PString(cstr) { }

PINLINE PCaselessString::PCaselessString(const PString & str)
  : PString(str) { }

PINLINE PCaselessString & PCaselessString::operator=(const PString & str)
  { PString::operator=(str); return *this; }

PINLINE PCaselessString & PCaselessString::operator=(const char * cstr)
  { PString::operator=(cstr); return *this; }

PINLINE PCaselessString & PCaselessString::operator=(char ch)
  { PString::operator=(ch); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringArray::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PINDEX PStringArray::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringList::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PINDEX PStringList::InsertString(
                                   const PString & before, const PString & str)
  { return Insert(before, str.Clone()); }

PINLINE PStringList PStringList::operator + (const PStringList & v)
  { PStringList arr = *this; arr += v; return arr; }

PINLINE PStringList PStringList::operator + (const PString & v)
  { PStringList arr = *this; arr += v; return arr; }

PINLINE PINDEX PStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PSortedStringList::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PINDEX PSortedStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }


// End Of File ///////////////////////////////////////////////////////////////
