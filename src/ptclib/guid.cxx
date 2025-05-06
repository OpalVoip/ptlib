/*
 * guid.cxx
 *
 * Globally Unique Identifier
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "guid.h"
#endif

#include <ptclib/guid.h>

#if P_GUID

#include <ptlib/sockets.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>

#if P_ASN
#include <ptclib/asner.h>
#endif


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

static PEthSocket::Address MacAddress()
{
  PString str = PIPSocket::GetInterfaceMACAddress();
  if (!str.empty())
    return str;

  PEthSocket::Address fake;
  PRandom::Octets(fake.b, sizeof(fake.b));
  fake.b[0] |= '\x80';
  return fake;
}


static void Construct(PGloballyUniqueID::Version version, BYTE * theArray, const void * digest)
{
  switch (version) {
    case PGloballyUniqueID::max:
      memset(theArray, 0xff, PGloballyUniqueID::Size);
      return;

    case PGloballyUniqueID::nil:
      return;

    case PGloballyUniqueID::v1:
    case PGloballyUniqueID::v2:
    case PGloballyUniqueID::v6:
    {
      // Want time of UTC in 0.1 microseconds since 15 Oct 1582.
      static PTimeInterval const delta(0, 0, 0, 0,
                                       16              // Days from 15th October
                                       + 31              // Days in December 1583
                                       + 30              // Days in November 1583
                                       + (1970-1583)*365 // Days in years
                                       + (1970-1583)/4   // Leap days
                                       - 3);             // Allow for 1700, 1800, 1900 not leap years
      int64_t timestamp = (PTime() + delta).GetTimestamp()*10;

      *(PInt64l *)theArray = timestamp;

      static WORD clockSequence = (WORD)PRandom::Number();
      static PInt64 lastTimestamp = 0;
      static PMutex s_mutex(PDebugLocation(__FILE__, __LINE__, "GUID"));
      s_mutex.Wait();
      if (lastTimestamp < timestamp)
        lastTimestamp = timestamp;
      else
        clockSequence++;
      s_mutex.Signal();

      *(PUInt16b *)(theArray+8) = clockSequence;

      if (version == 6)
        PRandom::Octets(theArray+10, 6);
      else {
        static PEthSocket::Address s_macAddress = MacAddress();
        memcpy(theArray+10, s_macAddress.b, 6);
        if (version == 2) {
          theArray[9] = 0;
        }
      }
      break;
    }

    case PGloballyUniqueID::v3:
    #if P_SSL
    case PGloballyUniqueID::v5:
    #endif // P_SSL
      memcpy(theArray, digest, PGloballyUniqueID::Size);
      break;

    case PGloballyUniqueID::v4:
      PRandom::Octets(theArray, PGloballyUniqueID::Size);
      break;

    case PGloballyUniqueID::v7:
      *(PInt64b *)theArray = PTime().GetTimestamp()/1000;
      PRandom::Octets(theArray+6, PGloballyUniqueID::Size-6);
      break;

    default:
      PAssertAlways(PInvalidParameter);
      return;
  }

  theArray[6] = (BYTE)((theArray[6]&0x0f) + (version << 4));  // Version number
  theArray[8] = (BYTE)((theArray[8]&0x3f) + (0b10 << 6));     // RFC9562 Variant
}


PGloballyUniqueID::PGloballyUniqueID(Version version)
  : PBYTEArray(Size)
{
  Construct(version, GetPointer(Size), NULL);
}


PGloballyUniqueID::PGloballyUniqueID(const PGloballyUniqueID & nspace,
                                     const void * name,
                                     size_t size,
                                     Version version)
  : PBYTEArray(Size)
{
  PMessageDigest * digestor;

  switch (version) {
    case v3:
      digestor = new PMessageDigest5();
      break;

    #if P_SSL
    case v5:
      digestor = new PMessageDigestSHA1();
      break;
    #endif // P_SSL

    default:
      Construct(version, GetPointer(Size), NULL);
      return;
  }

  digestor->Start();
  digestor->Process(nspace);
  digestor->Process(name, size);
  PMessageDigest::Result result;
  digestor->Complete(result);

  Construct(version, GetPointer(Size), result);

  delete digestor;
}


PGloballyUniqueID::PGloballyUniqueID(const char * cstr)
  : PBYTEArray(Size)
{
  if (cstr != NULL && *cstr != '\0') {
    PStringStream strm(cstr);
    ReadFrom(strm);
  }
}


PGloballyUniqueID::PGloballyUniqueID(const PString & str)
  : PBYTEArray(Size)
{
  PStringStream strm(str);
  ReadFrom(strm);
}


PGloballyUniqueID::PGloballyUniqueID(const void * data, PINDEX size)
  : PBYTEArray(Size)
{
  if (PAssertNULL(data))
    memcpy(GetPointer(), data, std::min(size, GetSize()));
}


#if P_ASN
PGloballyUniqueID::PGloballyUniqueID(const PASN_OctetString & newId)
  : PBYTEArray(newId)
{
  PAssert(GetSize() == Size, PInvalidParameter);
  PBYTEArray::SetSize(Size);
}
#endif


#ifdef GUID_DEFINED
PGloballyUniqueID::PGloballyUniqueID(const GUID & guid)
  : PBYTEArray(reinterpret_cast<const BYTE *>(&guid), sizeof(GUID))
{
}
#endif

PObject * PGloballyUniqueID::Clone() const
{
  PAssert(GetSize() == Size, "PGloballyUniqueID is invalid size");

  return new PGloballyUniqueID(*this);
}


PINDEX PGloballyUniqueID::HashFunction() const
{
  PAssert(GetSize() == Size, "PGloballyUniqueID is invalid size");

  static PINDEX NumBuckets = 53; // Should be prime number

#if P_64BIT
  uint64_t * qwords = (uint64_t *)GetPointer();
  return (qwords[0] ^ qwords[1]) % NumBuckets;
#else
  uint32_t * dwords = (uint32_t *)GetPointer();
  return (dwords[0] ^ dwords[1] ^ dwords[2] ^ dwords[3]) % NumBuckets;
#endif
}


void PGloballyUniqueID::PrintOn(ostream & strm) const
{
  PAssert(GetSize() == Size, "PGloballyUniqueID is invalid size");

  ios::fmtflags flags = strm.flags();
  bool dashes = ((flags&ios::basefield) != ios::hex);

  const BYTE * theArray = GetPointer();
  char fillchar = strm.fill();
  strm << hex << setfill('0')
       << setw(2) << (unsigned)(BYTE)theArray[0]
       << setw(2) << (unsigned)(BYTE)theArray[1]
       << setw(2) << (unsigned)(BYTE)theArray[2]
       << setw(2) << (unsigned)(BYTE)theArray[3];
  if (dashes)
    strm << '-';
  strm << setw(2) << (unsigned)(BYTE)theArray[4]
       << setw(2) << (unsigned)(BYTE)theArray[5];
  if (dashes)
    strm << '-';
  strm << setw(2) << (unsigned)(BYTE)theArray[6]
       << setw(2) << (unsigned)(BYTE)theArray[7];
  if (dashes)
    strm  << '-';
  strm << setw(2) << (unsigned)(BYTE)theArray[8]
       << setw(2) << (unsigned)(BYTE)theArray[9];
  if (dashes)
    strm << '-';
  strm << setw(2) << (unsigned)(BYTE)theArray[10]
       << setw(2) << (unsigned)(BYTE)theArray[11]
       << setw(2) << (unsigned)(BYTE)theArray[12]
       << setw(2) << (unsigned)(BYTE)theArray[13]
       << setw(2) << (unsigned)(BYTE)theArray[14]
       << setw(2) << (unsigned)(BYTE)theArray[15]
       << dec << setfill(fillchar);

  strm.flags(flags);
}


void PGloballyUniqueID::ReadFrom(istream & strm)
{
  PAssert(GetSize() == Size, "PGloballyUniqueID is invalid size");
  PBYTEArray::SetSize(Size);

  strm >> ws;

  PINDEX count = 0;
  BYTE * theArray = GetPointer(Size);

  while (count < 2*Size) {
    if (isxdigit(strm.peek())) {
      char digit = (char)(strm.get() - '0');
      if (digit >= 10) {
        digit -= 'A'-('9'+1);
        if (digit >= 16)
          digit -= 'a'-'A';
      }
      theArray[count/2] = (BYTE)((theArray[count/2] << 4) | digit);
      count++;
    }
    else if (strm.peek() == '-') {
      if (count != 8 && count != 12 && count != 16 && count != 20)
        break;
      strm.get(); // Ignore the dash if it was in the right place
    }
    else if (strm.peek() == ' ')
      strm.get(); // Ignore spaces
    else
      break;
  }

  if (count < 2*Size) {
    memset(theArray, 0, Size);
    strm.clear(ios::failbit);
  }
}


PString PGloballyUniqueID::AsString() const
{
  PStringStream strm;
  PrintOn(strm);
  return strm;
}


PBoolean PGloballyUniqueID::IsNULL() const
{
  PAssert(GetSize() == Size, "PGloballyUniqueID is invalid size");
  static BYTE const zeros[Size] = {};
  return memcmp(GetPointer(), zeros, Size) == 0;
}


#endif // P_GUID


/////////////////////////////////////////////////////////////////////////////
