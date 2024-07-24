/*
 * pils.cxx
 *
 * Microsoft Internet Location Server Protocol interface class.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2003 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */

#ifdef P_USE_PRAGMA
#pragma implementation "pils.h"
#endif

#include <ptlib.h>

#include <ptclib/pils.h>


#define new PNEW


#if P_LDAP

// Microsoft in their infinite wisdom save the IP address as an little endian
// integer from a 32 bit integer that was in network byte order (big endian)
// which causes immense confusion. Reading into a PIPSocket::Address does not
// work as it assumes that any integer forms would be in host order.
istream & operator>>(istream & s, PILSSession::MSIPAddress & a)
{
  uint32_t u;
  s >> u;

#if PBYTE_ORDER==PLITTLE_ENDIAN
  a = u;
#else
  a = PIPSocket::Address((uint8_t)(u>>24),(uint8_t)(u>>16),(uint8_t)(u>>8),(uint8_t)u);
#endif

  return s;
}


ostream & operator<<(ostream & s, PILSSession::MSIPAddress & a)
{
#if PBYTE_ORDER==PLITTLE_ENDIAN
  uint32_t u = a;
#else
  uint32_t u = (a.Byte1()<<24)|(a.Byte2()<<16)|(a.Byte3()<<8)|a.Byte4();
#endif

  return s << u;
}


///////////////////////////////////////////////////////////////////////////////

PILSSession::PILSSession()
  : PLDAPSession("objectClass=RTPerson")
{
  m_protocolVersion = 2;
}


bool PILSSession::AddPerson(const RTPerson & person)
{
  return Add(person.GetDN(), person);
}


bool PILSSession::ModifyPerson(const RTPerson & person)
{
  return Modify(person.GetDN(), person);
}


bool PILSSession::DeletePerson(const RTPerson & person)
{
  return Delete(person.GetDN());
}


bool PILSSession::SearchPerson(const PString & canonicalName, RTPerson & person)
{
  SearchContext context;
  if (!Search(context, "cn="+canonicalName))
    return false;

  if (!GetSearchResult(context, person))
    return false;

  // Return false if there is more than one match
  return !GetNextSearchResult(context);
}


PList<PILSSession::RTPerson> PILSSession::SearchPeople(const PString & filter)
{
  PList<RTPerson> persons;

  SearchContext context;
  if (Search(context, filter)) {
    do {
      RTPerson * person = new RTPerson;
      if (GetSearchResult(context, *person))
        persons.Append(person);
      else
        delete person;
    } while (GetNextSearchResult(context));
  }

  return persons;
}


PString PILSSession::RTPerson::GetDN() const
{
  PStringStream dn;

  if (!c.IsEmpty())
    dn << "c=" << c << ", ";

  if (!o.IsEmpty())
    dn << "o=" << o << ", ";

  dn << "cn=" + cn + ", objectClass=" + objectClass;

  return dn;
}


#endif // P_LDAP


// End of file ////////////////////////////////////////////////////////////////
