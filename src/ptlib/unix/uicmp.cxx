/*
 * uicmp.cxx
 *
 * ICMP socket class implementation.
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
 */

#pragma implementation "icmpsock.h"

#include <ptlib.h>
#include <ptlib/sockets.h>

#define  MAX_IP_LEN  60
#define  MAX_ICMP_LEN  76
#define  ICMP_DATA_LEN  (64-8)
#define  RX_BUFFER_SIZE  (MAX_IP_LEN+MAX_ICMP_LEN+ICMP_DATA_LEN)

#define  ICMP_ECHO_REPLY  0
#define  ICMP_ECHO  8

#define  ICMP_TIMXCEED  11


typedef struct {
  uint8_t   type;
  uint8_t   code;
  uint16_t   checksum;

  uint16_t   id;
  uint16_t   sequence;

  int64_t sendtime;
  uint8_t   data[ICMP_DATA_LEN-sizeof(int64_t)];
} ICMPPacket;


typedef struct {
  uint8_t verIhl;
  uint8_t typeOfService;
  uint16_t totalLength;
  uint16_t identification;
  uint16_t fragOff;
  uint8_t timeToLive;
  uint8_t protocol;
  uint16_t checksum;
  uint8_t sourceAddr[4];
  uint8_t destAddr[4];
} IPHdr;


static uint16_t CalcChecksum(void * p, PINDEX len)
{
  uint16_t * ptr = (uint16_t *)p;
  uint32_t sum = 0;
  while (len > 1) {
    sum += *ptr++;
    len-=2;
  }

  if (len > 0) {
    uint16_t t = *(uint8_t *)ptr;
    sum += t;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}


PICMPSocket::PICMPSocket()
{
  OpenSocket();
}


bool PICMPSocket::Ping(const PString & host)
{
  PingInfo info;
  return Ping(host, info);
}


bool PICMPSocket::Ping(const PString & host, PingInfo & info)
{
  // find address of the host
  PIPSocket::Address addr;
  if (!GetHostAddress(host, addr))
    return SetErrorValues(BadParameter, EINVAL);

  // create the ICMP packet
  ICMPPacket packet;

  // clear the packet including data area
  memset(&packet, 0, sizeof(packet));

  packet.type       = ICMP_ECHO;
  packet.sequence   = info.sequenceNum;
  packet.id         = info.identifier;

#ifndef BE_BONELESS
  if (info.ttl != 0) {
    char ttl = (char)info.ttl;
    if (::setsockopt(os_handle, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0)
      return false;
  }
#endif

  // set the send time
  packet.sendtime = PTimer::Tick().GetMilliSeconds();

  // calculate the checksum
  packet.checksum = CalcChecksum(&packet, sizeof(packet));

  // send the packet
  if (!WriteTo(&packet, sizeof(packet), addr, 0))
    return false;

  // receive a packet
  uint8_t reply[RX_BUFFER_SIZE];
  IPHdr      * ipHdr;
  ICMPPacket * icmpPacket;
  uint16_t port;
  int64_t now;
  PTimer timeout(GetReadTimeout());

  for (;;) {
    memset(&reply, 0, sizeof(reply));

    if (!ReadFrom(reply, sizeof(reply), info.remoteAddr, port))
      return false;

    now  = PTimer::Tick().GetMilliSeconds();
    ipHdr      = (IPHdr *)reply;
    icmpPacket = (ICMPPacket *)(reply + ((ipHdr->verIhl & 0xf) << 2));

    if ((icmpPacket->type == ICMP_ECHO_REPLY) && 
        ((uint16_t)icmpPacket->id   == info.identifier)) {
      info.status = PingSuccess;
      break;
    }

    if (icmpPacket->type == ICMP_TIMXCEED) {
      info.status = TtlExpiredTransmit;
      break;
    }

    if (!timeout.IsRunning())
      return false;
  }

  info.remoteAddr = Address(ipHdr->sourceAddr[0], ipHdr->sourceAddr[1],
                            ipHdr->sourceAddr[2], ipHdr->sourceAddr[3]);
  info.localAddr  = Address(ipHdr->destAddr[0], ipHdr->destAddr[1],
                            ipHdr->destAddr[2], ipHdr->destAddr[3]);

  // calc round trip time. Be careful, as unaligned "long long" ints
  // can cause problems on some platforms
  info.delay.SetInterval(now - icmpPacket->sendtime);
  info.sequenceNum = icmpPacket->sequence;

  return true;
}


bool PICMPSocket::OpenSocket()
{
  struct protoent * p = ::getprotobyname(GetProtocolName());
  if (p == NULL)
    return ConvertOSError(-1);
  return ConvertOSError(os_handle = os_socket(AF_INET, SOCK_RAW, p->p_proto));
}


const char * PICMPSocket::GetProtocolName() const
{
  return "icmp";
}


PICMPSocket::PingInfo::PingInfo(uint16_t id)
{
  identifier = id;
  sequenceNum = 0;
  ttl = 255;
  buffer = NULL;
  status = PingSuccess;
}


// End Of File ///////////////////////////////////////////////////////////////
