/*
 * pdns.cxx
 *
 * Portable Tools Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 *
 * Copyright 2003 Equivalence Pty. Ltd.
 */

#ifdef P_USE_PRAGMA
#pragma implementation "pdns.h"
#endif

#include <ptlib.h>
#include <ptclib/pdns.h>
#include <ptclib/url.h>
#include <ptlib/ipsock.h>

#define new PNEW

#define RESOLVER_CACHE_TIMEOUT  30000

#if P_DNS_RESOLVER

#ifdef _MSC_VER
  #pragma comment(lib, "DnsAPI.Lib")
#endif


/////////////////////////////////////////////////

static PMutex dns_mutex(PDebugLocation(__FILE__, __LINE__, "DNS"));

struct DNSCacheInfo {
  DNSCacheInfo() : m_results(NULL), m_status(-1) { }
  PTime         m_time;
  PDNS_RECORD   m_results;
  DNS_STATUS    m_status;
};

typedef std::map<std::string, DNSCacheInfo> DNSCache;

static PTime g_lastAgeTime(0);
static DNSCache g_dnsCache;


#ifdef P_HAS_RESOLV_H

static bool GetDN(const uint8_t * reply, const uint8_t * replyEnd, uint8_t * & cp, char * buff)
{
  int len = dn_expand(reply, replyEnd, cp, buff, MAXDNAME);
  if (len < 0)
    return false;
  cp += len;
  return true;
}

static bool ProcessDNSRecords(
        const uint8_t * reply,
        const uint8_t * replyEnd,
              uint8_t * cp,
            PINDEX anCount,
            PINDEX nsCount,
            PINDEX arCount,
     PDNS_RECORD * results)
{
  PDNS_RECORD lastRecord = NULL;

  PINDEX rrCount = anCount + nsCount + arCount;
  nsCount += anCount;
  arCount += nsCount;

  PINDEX i;
  for (i = 0; i < rrCount; i++) {

    int section;
    if (i < anCount)
      section = DnsSectionAnswer;
    else if (i < nsCount)
      section = DnsSectionAuthority;
    else // if (i < arCount)
      section = DnsSectionAdditional;

    // get the name
    char pName[MAXDNAME];
    if (!GetDN(reply, replyEnd, cp, pName))
      return false;

    // get other common parts of the record
    uint16_t  type;
    //uint16_t  dnsClass;
    //uint32_t ttl;
    uint16_t  dlen;

    GETSHORT(type, cp);
    cp += 2; // GETSHORT(dnsClass, cp);
    cp += 4; // GETLONG (ttl,      cp);
    GETSHORT(dlen, cp);

    uint8_t * data = cp;
    cp += dlen;

    PDNS_RECORD newRecord  = NULL;

    switch (type) {
      default:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord) + sizeof(uint32_t) + dlen);
        newRecord->Data.Null.dwByteCount = dlen;
        memcpy(&newRecord->Data, data, dlen);
        break;

      case T_SRV:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETSHORT(newRecord->Data.SRV.wPriority, data);
        GETSHORT(newRecord->Data.SRV.wWeight, data);
        GETSHORT(newRecord->Data.SRV.wPort, data);
        if (!GetDN(reply, replyEnd, data, newRecord->Data.SRV.pNameTarget)) {
          free(newRecord);
          return false;
        }
        break;

      case T_MX:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETSHORT(newRecord->Data.MX.wPreference,  data);
        if (!GetDN(reply, replyEnd, data, newRecord->Data.MX.pNameExchange)) {
          free(newRecord);
          return false;
        }
        break;

      case T_A:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETLONG(newRecord->Data.A.IpAddress, data);
        break;

      case T_AAAA:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETLONG(newRecord->Data.AAAA.Ip6Address[0], data);
        GETLONG(newRecord->Data.AAAA.Ip6Address[1], data);
        GETLONG(newRecord->Data.AAAA.Ip6Address[2], data);
        GETLONG(newRecord->Data.AAAA.Ip6Address[3], data);
        break;

      case T_NS:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        if (!GetDN(reply, replyEnd, data, newRecord->Data.NS.pNameHost)) {
          delete newRecord;
          return false;
        }
        break;
    }

    // initialise the new record
    if (newRecord != NULL) {
      newRecord->wType = type;
      newRecord->Flags.S.Section = section;
      newRecord->pNext = NULL;
      strcpy(newRecord->pName, pName);

      if (*results == NULL)
        *results = newRecord;

      if (lastRecord != NULL)
        lastRecord->pNext = newRecord;

      lastRecord = newRecord;
      newRecord = NULL;
    }
  }

  return true;
}

DNS_STATUS DnsQuery_A(const char * service,
                              uint16_t requestType,
                             uint32_t options,
                            void *,
                     PDNS_RECORD * results,
                            void *)
{
#if defined(P_NETBSD)
  struct __res_state myRes;
#endif
  if (results == NULL)
    return -1;

  *results = NULL;

#if P_HAS_RES_NINIT
#if defined(P_NETBSD)
  res_ninit(&myRes);
#else
  res_ninit(&_res);
#endif
#else
  res_init();
  dns_mutex.Wait();
#endif

  union {
    HEADER hdr;
    uint8_t buf[PACKETSZ];
  } reply;

#if P_HAS_RES_NINIT
  int replyLen = res_nsearch(
#if defined(P_NETBSD)
      &myRes,
#else
      &_res,
#endif
      service, C_IN, requestType, (uint8_t *)&reply, sizeof(reply));
#else
  int replyLen = res_search(service, C_IN, requestType, (uint8_t *)&reply, sizeof(reply));
  dns_mutex.Signal();
#endif

  if (replyLen < 1)
    return -1;

  uint8_t * replyStart = reply.buf;
  uint8_t * replyEnd   = reply.buf + replyLen;
  uint8_t * cp         = reply.buf + sizeof(HEADER);

  // ignore questions in response
  uint16_t i;
  for (i = 0; i < ntohs(reply.hdr.qdcount); i++) {
    char qName[MAXDNAME];
    if (!GetDN(replyStart, replyEnd, cp, qName))
      return -1;
    cp += QFIXEDSZ;
  }

  if (!ProcessDNSRecords(
       replyStart,
       replyEnd,
       cp,
       ntohs(reply.hdr.ancount),
       ntohs(reply.hdr.nscount),
       ntohs(reply.hdr.arcount),
       results)) {
    DnsRecordListFree(*results, DnsFreeRecordList);
    return -1;
  }

  return 0;
}


PDNS_RECORD DnsRecordSetCopy(PDNS_RECORD src)
{
  PDNS_RECORD result = NULL;
  PDNS_RECORD dst = NULL;
  PDNS_RECORD rec;

  while (src != NULL) {
    rec = (PDNS_RECORD)malloc(sizeof(DNS_RECORD));
    memcpy(rec, src, sizeof(DNS_RECORD));
    if (result == NULL)
      result = rec;
    rec->pNext = NULL;
    if (dst != NULL)
      dst->pNext = rec;
    src = src->pNext;
    dst = rec;
  }

  return result;
}


void DnsRecordListFree(PDNS_RECORD rec, int /* FreeType */)
{
  while (rec != NULL) {
    PDNS_RECORD next = rec->pNext;
    free(rec);
    rec = next;
  }
}


#endif // P_HAS_RESOLV_H


PObject::Comparison PDNS::SRVRecord::Compare(const PObject & obj) const
{
  const SRVRecord * other = dynamic_cast<const SRVRecord *>(&obj);

  if (other == NULL)
    return LessThan;

  if (priority < other->priority)
    return LessThan;
  else if (priority > other->priority)
    return GreaterThan;

  if (weight < other->weight)
    return LessThan;
  else if (weight > other->weight)
    return GreaterThan;

  return EqualTo;
}

void PDNS::SRVRecord::PrintOn(ostream & strm) const
{
  strm << "host=" << hostName << ":" << port << "(" << hostAddress << "), "
       << "priority=" << priority << ", "
       << "weight=" << weight;
}

/////////////////////////////////////////////////

PDNS::SRVRecord * PDNS::SRVRecordList::HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results)
{
  PDNS::SRVRecord * record = NULL;

  if (
      (dnsRecord->Flags.S.Section == DnsSectionAnswer) && 
      (dnsRecord->wType == DNS_TYPE_SRV) &&
      (strlen(dnsRecord->Data.SRV.pNameTarget) > 0) &&
      (strcmp(dnsRecord->Data.SRV.pNameTarget, ".") != 0)
      ) {
    record = new SRVRecord();
    record->hostName = PString(dnsRecord->Data.SRV.pNameTarget);
    record->port     = dnsRecord->Data.SRV.wPort;
    record->priority = dnsRecord->Data.SRV.wPriority;
    record->weight   = dnsRecord->Data.SRV.wWeight;

    // see if any A or AAAA records match this hostname
    PDNS_RECORD aRecord = results;
    while (aRecord != NULL) {
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_A)) {
        record->hostAddress = PIPSocket::Address(dnsRecord->Data.A.IpAddress);
        break;
      }
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_AAAA)) {
        record->hostAddress = PIPSocket::Address(16, (uint8_t *)&dnsRecord->Data.AAAA.Ip6Address);
        break;
      }
      aRecord = aRecord->pNext;
    }

    // if no A or AAAA record found, then get address the hard way
    if (aRecord == NULL)
      PIPSocket::GetHostAddress(record->hostName, record->hostAddress);
  }

  return record;
}

void PDNS::SRVRecordList::PrintOn(ostream & strm) const
{
  for (auto & it : *this) 
    strm << it << '\n';
}

PDNS::SRVRecord * PDNS::SRVRecordList::GetFirst()
{
  if (empty())
    return NULL;

  // create a list of all prioities, to save time
  priPos = 0;
  priList.SetSize(0);

  priList.SetSize(1);
  auto & front = *begin();
  uint16_t lastPri = front.priority;
  priList[0] = lastPri;
  front.used = false;
  for (auto & entry : *this) {
    entry.used = false;
    if (entry.priority != lastPri) {
      priPos++;
      priList.SetSize(priPos);
      lastPri = entry.priority;
      priList[priPos] = lastPri;
    }
  }
  
  priPos = 0;
  return GetNext();
}

PDNS::SRVRecord * PDNS::SRVRecordList::GetNext()
{
  if (priList.GetSize() == 0)
    return NULL;

  while (priPos < priList.GetSize()) {

    uint16_t currentPri = priList[priPos];

    // find first record at current priority
    iterator firstPos;
    for (firstPos = begin(); firstPos != end() && firstPos->priority != currentPri; ++firstPos) 
      ;
    if (firstPos == end())
      return NULL;

    // calculate total of all unused weights at this priority
    unsigned totalWeight = firstPos->weight;
    iterator it = firstPos;
    PINDEX count = 1;
    while (++it != end() && it->priority == currentPri) {
      if (!it->used) {
        totalWeight += it->weight;
        count ++;
      }
    }

    // if no matches found, go to the next priority level
    if (count == 0) {
      priPos++;
      continue;
    }

    // selected the correct item
    if (totalWeight > 0) {
      unsigned targetWeight = PRandom::Number() % (totalWeight+1);
      totalWeight = 0;
      for (it = begin(); it != end() && it->priority == currentPri; ++it) {
        if (!it->used) {
          totalWeight += it->weight;
          if (totalWeight >= targetWeight) {
            it->used = true;
            return &*it;
          }
        }
      }
    }

    // pick a random item at this priority
    PINDEX rnd = (count <= 1) ? 0 : (PRandom::Number() % count);
    count = 0;
    for (it = firstPos; it != end() && (it->priority == currentPri); ++it) {
      if (!it->used) {
        if (count == rnd) {
          it->used = true;
          return &*it;
        }
        count++;
      }
    }

    // go to the next priority level
    priPos++;
  }

  return NULL;
}

bool PDNS::GetSRVRecords(
  const PString & _service,
  const PString & type,
  const PString & domain,
  PDNS::SRVRecordList & recordList
)
{
  if (_service.IsEmpty())
    return false;

  PStringStream service;
  if (_service[0] != '_')
    service << '_';

  service << _service << "._" << type << '.' << domain;

  return GetSRVRecords(service, recordList);
}


#if P_URL

bool PDNS::LookupSRV(
           const PURL & url,
        const PString & service,
          PStringList & returnList)
{
  uint16_t defaultPort = url.GetPort();
  PIPSocketAddressAndPortVector info;

  if (!LookupSRV(url.GetHostName(), service, defaultPort, info)) {
    PTRACE(2,"DNS\tSRV Lookup Fail no domain " << url );
    return false;
  }

  PString user = url.GetUserName();
  if (user.GetLength() > 0)
    user = user + "@";

  PIPSocketAddressAndPortVector::const_iterator r;
  for (r = info.begin(); r != info.end(); ++r) {
    if (r->GetAddress().GetVersion() == 6)
      returnList.AppendString(user + "[" + r->GetAddress().AsString() + "]:" + PString(r->GetPort()));
    else
      returnList.AppendString(user + r->AsString(':'));
  }

  return returnList.GetSize() != 0;;
}

#endif // P_URL


bool PDNS::LookupSRV(
         const PString & domain,            ///< domain to lookup
         const PString & service,           ///< service to use
                    uint16_t defaultPort,       ///< default port to use
         PIPSocketAddressAndPortVector & addrList  ///< list of sockets and ports
)
{
  if (domain.IsEmpty()) {
    PTRACE(1,"DNS\tSRV lookup failed - no domain specified");
    return false;
  }

  PString addr = domain;
  PINDEX colon = addr.FindLast(':');
  if (colon != P_MAX_INDEX && (addr.Find('[') == P_MAX_INDEX || addr.Find(']') < colon))
    addr.Delete(colon, P_MAX_INDEX);

  if (PIPSocket::Address(addr).IsValid()) {
    PTRACE(4, "DNS\tSRV lookup failed - domain is IP address");
    return false;
  }

  PString srvLookupStr = service;
  if (srvLookupStr[srvLookupStr.GetLength()-1] != '.')
    srvLookupStr += '.';
  srvLookupStr += addr;
  PTRACE(4,"DNS\tSRV Lookup \"" << srvLookupStr << '"');
  return LookupSRV(srvLookupStr, defaultPort, addrList);
}

bool PDNS::LookupSRV(
              const PString & srvLookupStr,
              uint16_t defaultPort,
              PIPSocketAddressAndPortVector & addrList
)
{
  PDNS::SRVRecordList srvRecords;
  bool found = false;
  if (PDNS::GetRecords(srvLookupStr, srvRecords)) {
    PTRACE(5,"DNS\tSRV Record found \"" << srvLookupStr << '"');
    PDNS::SRVRecord * recPtr = srvRecords.GetFirst();
    while (recPtr != NULL) {
      PIPSocketAddressAndPort addrAndPort;
      addrAndPort.SetAddress(recPtr->hostAddress, recPtr->port > 0 ? recPtr->port : defaultPort);
      addrList.push_back(addrAndPort);
      found = true;
      recPtr = srvRecords.GetNext();
    }
  } 

  return found;
}

///////////////////////////////////////////////////////

PObject::Comparison PDNS::MXRecord::Compare(const PObject & obj) const
{
  const MXRecord * other = dynamic_cast<const MXRecord *>(&obj);
  if (other == NULL)
    return LessThan;

  if (preference < other->preference)
    return LessThan;
  else if (preference > other->preference)
    return GreaterThan;

  return EqualTo;
}

void PDNS::MXRecord::PrintOn(ostream & strm) const
{
  strm << "host=" << hostName << "(" << hostAddress << "), "
       << "preference=" << preference;
}

///////////////////////////////////////////////////////

PDNS::MXRecord * PDNS::MXRecordList::HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results)
{
  MXRecord * record = NULL;

  if (
      (dnsRecord->Flags.S.Section == DnsSectionAnswer) &&
      (dnsRecord->wType == DNS_TYPE_MX) &&
      (strlen(dnsRecord->Data.MX.pNameExchange) > 0)
     ) {
    record = new MXRecord();
    record->hostName   = PString(dnsRecord->Data.MX.pNameExchange);
    record->preference = dnsRecord->Data.MX.wPreference;

    // see if any A records match this hostname
    PDNS_RECORD aRecord = results;
    while (aRecord != NULL) {
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_A)) {
        record->hostAddress = PIPSocket::Address(dnsRecord->Data.A.IpAddress);
        break;
      }
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_AAAA)) {
        record->hostAddress = PIPSocket::Address(16, (uint8_t *)&dnsRecord->Data.AAAA.Ip6Address);
        break;
      }
      aRecord = aRecord->pNext;
    }

    // if no A record found, then get address the hard way
    if (aRecord == NULL)
      PIPSocket::GetHostAddress(record->hostName, record->hostAddress);
  }

  return record;
}

PDNS::MXRecordList::MXRecordList()
  : m_nextIndex(end())
{ }

void PDNS::MXRecordList::PrintOn(ostream & strm) const
{
  for (const auto & it : *this)
    strm << it << endl;
}

PDNS::MXRecord * PDNS::MXRecordList::GetFirst()
{
  for (auto & it : *this)
    it.used = false;

  m_nextIndex = begin();

  return GetNext();
}

PDNS::MXRecord * PDNS::MXRecordList::GetNext()
{
  if (empty() || m_nextIndex == end())
    return NULL;

  MXRecord & record = *m_nextIndex;
  return &record;
}

/////////////////////////////////////////////////////////////////

DNS_STATUS PDNS::Cached_DnsQuery(
    const char * name,
    uint16_t       type,
    uint32_t      options,
    void *     ,
    PDNS_RECORD * queryResults,
    void * )
{
  PTime now;
  PWaitAndSignal m(dns_mutex);

  DNSCache::iterator r;

  // age entries in cache
  if ((now - g_lastAgeTime) > RESOLVER_CACHE_TIMEOUT) {
    g_lastAgeTime = now;

    r = g_dnsCache.begin();
    while (r != g_dnsCache.end()) {
      if ((now - r->second.m_time) < RESOLVER_CACHE_TIMEOUT)
        ++r;
      else {
        PTRACE(5, "DNS\tQuery aged \"" << r->first << '"');
        DnsRecordListFree(r->second.m_results, DnsFreeRecordList);
        g_dnsCache.erase(r++);
      }
    }
  }

  // see if cache contains the entry we need
  string key;
  {
    std::stringstream strm;
    strm << name << '\t' << type << '\t' << options;
    key = strm.str();
  }

  r = g_dnsCache.find(key);
  if (r == g_dnsCache.end()) {
    PTRACE(5, "DNS\tSRV physical lookup \"" << key << '"');

    // else do the lookup and put it into the cache
    DNSCacheInfo info;
    info.m_status = DnsQuery_A((const char *)name, 
                               type,
                               DNS_QUERY_STANDARD, 
                               NULL, 
                               &info.m_results, 
                               NULL);
#if PTRACING
    if (info.m_status != 0)
      PTRACE(3, "DNS\tQuery failed: error=" << info.m_status);
    else {
      PTRACE(6, "DNS\tQuery success: " << info.m_results);
      for (PDNS_RECORD rec = info.m_results; rec != NULL; rec = rec->pNext)
        PTRACE(6, "DNS\tQuery: name=\"" << PString(rec->pName)
               << "\", type=" << rec->wType << ", len=" << rec->wDataLength);
      PTRACE(6, "DNS\tQuery done");
    }
#endif

    r = g_dnsCache.insert(DNSCache::value_type(key, info)).first;
  }

  *queryResults = DnsRecordSetCopy(r->second.m_results);
  return r->second.m_status;
}


PDNS::PDnsRecords::~PDnsRecords()
{
  if (m_records != NULL)
    DnsRecordListFree(m_records, DnsFreeRecordList);
}
#endif // P_DNS_RESOLVER
