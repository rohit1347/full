/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "full-wifi-mac-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullWifiMacHeader);

enum
{
  TYPE_MGT = 0,
  TYPE_CTL  = 1,
  TYPE_DATA = 2,
  TYPE_BUSY = 3,
};

enum
{
  SUBTYPE_CTL_BACKREQ = 8,
  SUBTYPE_CTL_BACKRESP = 9,
  SUBTYPE_CTL_RTS = 11,
  SUBTYPE_CTL_CTS = 12,
  SUBTYPE_CTL_ACK = 13
};

FullWifiMacHeader::FullWifiMacHeader ()
  : m_ctrlPwrMgt (0),
    m_ctrlMoreData (0),
    m_ctrlWep (0),
    m_ctrlOrder (1),
    m_amsduPresent (0)
{
}
FullWifiMacHeader::~FullWifiMacHeader ()
{
}

void
FullWifiMacHeader::SetDsFrom (void)
{
  m_ctrlFromDs = 1;
}
void
FullWifiMacHeader::SetDsNotFrom (void)
{
  m_ctrlFromDs = 0;
}
void
FullWifiMacHeader::SetDsTo (void)
{
  m_ctrlToDs = 1;
}
void
FullWifiMacHeader::SetDsNotTo (void)
{
  m_ctrlToDs = 0;
}

void
FullWifiMacHeader::SetAddr1 (Mac48Address address)
{
  m_addr1 = address;
}
void
FullWifiMacHeader::SetAddr2 (Mac48Address address)
{
  m_addr2 = address;
}
void
FullWifiMacHeader::SetAddr3 (Mac48Address address)
{
  m_addr3 = address;
}
void
FullWifiMacHeader::SetAddr4 (Mac48Address address)
{
  m_addr4 = address;
}
void
FullWifiMacHeader::SetAssocReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0;
}
void
FullWifiMacHeader::SetAssocResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 1;
}
void
FullWifiMacHeader::SetProbeReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 4;
}
void
FullWifiMacHeader::SetProbeResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 5;
}
void
FullWifiMacHeader::SetBeacon (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 8;
}
void
FullWifiMacHeader::SetBlockAckReq (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 8;
}
void
FullWifiMacHeader::SetBlockAck (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 9;
}

void
FullWifiMacHeader::SetTypeData (void)
{
  m_ctrlType = TYPE_DATA;
  m_ctrlSubtype = 0;
}
void
FullWifiMacHeader::SetAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0D;
}
void
FullWifiMacHeader::SetMultihopAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0F;
}
void
FullWifiMacHeader::SetType (enum FullWifiMacType type)
{
  switch (type)
    {
    case FULL_WIFI_MAC_CTL_BACKREQ:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKREQ;
      break;
    case FULL_WIFI_MAC_CTL_BACKRESP:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKRESP;
      break;
    case FULL_WIFI_MAC_CTL_RTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_RTS;
      break;
    case FULL_WIFI_MAC_CTL_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTS;
      break;
    case FULL_WIFI_MAC_CTL_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_ACK;
      break;
    case FULL_WIFI_MAC_MGT_ASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 0;
      break;
    case FULL_WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 1;
      break;
    case FULL_WIFI_MAC_MGT_REASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 2;
      break;
    case FULL_WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 3;
      break;
    case FULL_WIFI_MAC_MGT_PROBE_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 4;
      break;
    case FULL_WIFI_MAC_MGT_PROBE_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 5;
      break;
    case FULL_WIFI_MAC_MGT_BEACON:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 8;
      break;
    case FULL_WIFI_MAC_MGT_DISASSOCIATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 10;
      break;
    case FULL_WIFI_MAC_MGT_AUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 11;
      break;
    case FULL_WIFI_MAC_MGT_DEAUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 12;
      break;
    case FULL_WIFI_MAC_MGT_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 13;
      break;
    case FULL_WIFI_MAC_MGT_ACTION_NO_ACK:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 14;
      break;
    case FULL_WIFI_MAC_MGT_MULTIHOP_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 15;
      break;

    case FULL_WIFI_MAC_DATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 0;
      break;
    case FULL_WIFI_MAC_DATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 1;
      break;
    case FULL_WIFI_MAC_DATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 2;
      break;
    case FULL_WIFI_MAC_DATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 3;
      break;
    case FULL_WIFI_MAC_DATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 4;
      break;
    case FULL_WIFI_MAC_DATA_NULL_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 5;
      break;
    case FULL_WIFI_MAC_DATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 6;
      break;
    case FULL_WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 7;
      break;
    case FULL_WIFI_MAC_QOSDATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 8;
      break;
    case FULL_WIFI_MAC_QOSDATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 9;
      break;
    case FULL_WIFI_MAC_QOSDATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 10;
      break;
    case FULL_WIFI_MAC_QOSDATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 11;
      break;
    case FULL_WIFI_MAC_QOSDATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 12;
      break;
    case FULL_WIFI_MAC_QOSDATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 14;
      break;
    case FULL_WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 15;
      break;
    case FULL_WIFI_MAC_BUSY_TONE:
      m_ctrlType = TYPE_BUSY;
      m_ctrlSubtype = 0;
      break;
    case FULL_WIFI_MAC_RETURN_DATA:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 16;
        break;
    case FULL_WIFI_MAC_FORWARD_DATA:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 17;
        break;

    }
  m_ctrlToDs = 0;
  m_ctrlFromDs = 0;
}
void
FullWifiMacHeader::SetRawDuration (uint16_t duration)
{
  m_duration = duration;
}
void
FullWifiMacHeader::SetDuration (Time duration)
{
  int64_t duration_us = duration.GetMicroSeconds ();
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x7fff);
  m_duration = static_cast<uint16_t> (duration_us);
}

void FullWifiMacHeader::SetId (uint16_t id)
{
  m_duration = id;
}
void FullWifiMacHeader::SetSequenceNumber (uint16_t seq)
{
  m_seqSeq = seq;
}
void FullWifiMacHeader::SetFragmentNumber (uint8_t frag)
{
  m_seqFrag = frag;
}
void FullWifiMacHeader::SetNoMoreFragments (void)
{
  m_ctrlMoreFrag = 0;
}
void FullWifiMacHeader::SetMoreFragments (void)
{
  m_ctrlMoreFrag = 1;
}
void FullWifiMacHeader::SetRetry (void)
{
  m_ctrlRetry = 1;
}
void FullWifiMacHeader::SetNoRetry (void)
{
  m_ctrlRetry = 0;
}
void FullWifiMacHeader::SetQosTid (uint8_t tid)
{
  m_qosTid = tid;
}
void FullWifiMacHeader::SetQosEosp ()
{
  m_qosEosp = 1;
}
void FullWifiMacHeader::SetQosNoEosp ()
{
  m_qosEosp = 0;
}
void FullWifiMacHeader::SetQosAckPolicy (enum QosAckPolicy policy)
{
  switch (policy)
    {
    case NORMAL_ACK:
      m_qosAckPolicy = 0;
      break;
    case NO_ACK:
      m_qosAckPolicy = 1;
      break;
    case NO_EXPLICIT_ACK:
      m_qosAckPolicy = 2;
      break;
    case BLOCK_ACK:
      m_qosAckPolicy = 3;
      break;
    }
}
void
FullWifiMacHeader::SetQosNormalAck ()
{
  m_qosAckPolicy = 0;
}
void
FullWifiMacHeader::SetQosBlockAck ()
{
  m_qosAckPolicy = 3;
}
void
FullWifiMacHeader::SetQosNoAck ()
{
  m_qosAckPolicy = 1;
}
void FullWifiMacHeader::SetQosAmsdu (void)
{
  m_amsduPresent = 1;
}
void FullWifiMacHeader::SetQosNoAmsdu (void)
{
  m_amsduPresent = 0;
}
void FullWifiMacHeader::SetQosTxopLimit (uint8_t txop)
{
  m_qosStuff = txop;
}

Mac48Address
FullWifiMacHeader::GetAddr1 (void) const
{
  return m_addr1;
}
Mac48Address
FullWifiMacHeader::GetAddr2 (void) const
{
  return m_addr2;
}
Mac48Address
FullWifiMacHeader::GetAddr3 (void) const
{
  return m_addr3;
}
Mac48Address
FullWifiMacHeader::GetAddr4 (void) const
{
  return m_addr4;
}

enum FullWifiMacType
FullWifiMacHeader::GetType (void) const
{
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      switch (m_ctrlSubtype)
        {
        case 0:
          return FULL_WIFI_MAC_MGT_ASSOCIATION_REQUEST;
          break;
        case 1:
          return FULL_WIFI_MAC_MGT_ASSOCIATION_RESPONSE;
          break;
        case 2:
          return FULL_WIFI_MAC_MGT_REASSOCIATION_REQUEST;
          break;
        case 3:
          return FULL_WIFI_MAC_MGT_REASSOCIATION_RESPONSE;
          break;
        case 4:
          return FULL_WIFI_MAC_MGT_PROBE_REQUEST;
          break;
        case 5:
          return FULL_WIFI_MAC_MGT_PROBE_RESPONSE;
          break;
        case 8:
          return FULL_WIFI_MAC_MGT_BEACON;
          break;
        case 10:
          return FULL_WIFI_MAC_MGT_DISASSOCIATION;
          break;
        case 11:
          return FULL_WIFI_MAC_MGT_AUTHENTICATION;
          break;
        case 12:
          return FULL_WIFI_MAC_MGT_DEAUTHENTICATION;
          break;
        case 13:
          return FULL_WIFI_MAC_MGT_ACTION;
          break;
        case 14:
          return FULL_WIFI_MAC_MGT_ACTION_NO_ACK;
          break;
        case 15:
          return FULL_WIFI_MAC_MGT_MULTIHOP_ACTION;
          break;

        }
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_BACKREQ:
          return FULL_WIFI_MAC_CTL_BACKREQ;
          break;
        case SUBTYPE_CTL_BACKRESP:
          return FULL_WIFI_MAC_CTL_BACKRESP;
          break;
        case SUBTYPE_CTL_RTS:
          return FULL_WIFI_MAC_CTL_RTS;
          break;
        case SUBTYPE_CTL_CTS:
          return FULL_WIFI_MAC_CTL_CTS;
          break;
        case SUBTYPE_CTL_ACK:
          return FULL_WIFI_MAC_CTL_ACK;
          break;
        }
      break;
    case TYPE_BUSY:
      switch (m_ctrlSubtype)
        {
        case 0:
          return FULL_WIFI_MAC_BUSY_TONE;
          break;
        }
      break;
    case TYPE_DATA:
      switch (m_ctrlSubtype)
        {
        case 0:
          return FULL_WIFI_MAC_DATA;
          break;
        case 1:
          return FULL_WIFI_MAC_DATA_CFACK;
          break;
        case 2:
          return FULL_WIFI_MAC_DATA_CFPOLL;
          break;
        case 3:
          return FULL_WIFI_MAC_DATA_CFACK_CFPOLL;
          break;
        case 4:
          return FULL_WIFI_MAC_DATA_NULL;
          break;
        case 5:
          return FULL_WIFI_MAC_DATA_NULL_CFACK;
          break;
        case 6:
          return FULL_WIFI_MAC_DATA_NULL_CFPOLL;
          break;
        case 7:
          return FULL_WIFI_MAC_DATA_NULL_CFACK_CFPOLL;
          break;
        case 8:
          return FULL_WIFI_MAC_QOSDATA;
          break;
        case 9:
          return FULL_WIFI_MAC_QOSDATA_CFACK;
          break;
        case 10:
          return FULL_WIFI_MAC_QOSDATA_CFPOLL;
          break;
        case 11:
          return FULL_WIFI_MAC_QOSDATA_CFACK_CFPOLL;
          break;
        case 12:
          return FULL_WIFI_MAC_QOSDATA_NULL;
          break;
        case 14:
          return FULL_WIFI_MAC_QOSDATA_NULL_CFPOLL;
          break;
        case 15:
          return FULL_WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL;
          break;
        case 16:
        	return FULL_WIFI_MAC_RETURN_DATA;
        	break;
        case 17:
        	return FULL_WIFI_MAC_FORWARD_DATA;
        	break;

        }
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum FullWifiMacType)-1;
}
bool
FullWifiMacHeader::IsFromDs (void) const
{
  return m_ctrlFromDs == 1;
}
bool
FullWifiMacHeader::IsToDs (void) const
{
  return m_ctrlToDs == 1;
}

bool
FullWifiMacHeader::IsData (void) const
{
  return (m_ctrlType == TYPE_DATA);

}
bool
FullWifiMacHeader::IsQosData (void) const
{
  return (m_ctrlType == TYPE_DATA && (m_ctrlSubtype & 0x08));
}
bool
FullWifiMacHeader::IsCtl (void) const
{
  return (m_ctrlType == TYPE_CTL);
}
bool
FullWifiMacHeader::IsMgt (void) const
{
  return (m_ctrlType == TYPE_MGT);
}

bool
FullWifiMacHeader::IsBusyTone (void) const
{
  return (m_ctrlType == TYPE_BUSY);
}

bool
FullWifiMacHeader::IsCfpoll (void) const
{
  switch (GetType ())
    {
    case FULL_WIFI_MAC_DATA_CFPOLL:
    case FULL_WIFI_MAC_DATA_CFACK_CFPOLL:
    case FULL_WIFI_MAC_DATA_NULL_CFPOLL:
    case FULL_WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      return true;
      break;
    default:
      return false;
      break;
    }
}
bool
FullWifiMacHeader::IsRts (void) const
{
  return (GetType () == FULL_WIFI_MAC_CTL_RTS);
}
bool
FullWifiMacHeader::IsCts (void) const
{
  return (GetType () == FULL_WIFI_MAC_CTL_CTS);
}
bool
FullWifiMacHeader::IsAck (void) const
{
  return (GetType () == FULL_WIFI_MAC_CTL_ACK);
}
bool
FullWifiMacHeader::IsAssocReq (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_ASSOCIATION_REQUEST);
}
bool
FullWifiMacHeader::IsAssocResp (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
}
bool
FullWifiMacHeader::IsReassocReq (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_REASSOCIATION_REQUEST);
}
bool
FullWifiMacHeader::IsReassocResp (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}
bool
FullWifiMacHeader::IsProbeReq (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_PROBE_REQUEST);
}
bool
FullWifiMacHeader::IsProbeResp (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_PROBE_RESPONSE);
}
bool
FullWifiMacHeader::IsBeacon (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_BEACON);
}
bool
FullWifiMacHeader::IsDisassociation (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_DISASSOCIATION);
}
bool
FullWifiMacHeader::IsAuthentication (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_AUTHENTICATION);
}
bool
FullWifiMacHeader::IsDeauthentication (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_DEAUTHENTICATION);
}
bool
FullWifiMacHeader::IsAction (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_ACTION);
}
bool
FullWifiMacHeader::IsMultihopAction (void) const
{
  return (GetType () == FULL_WIFI_MAC_MGT_MULTIHOP_ACTION);
}
bool
FullWifiMacHeader::IsBlockAckReq (void) const
{
  return (GetType () == FULL_WIFI_MAC_CTL_BACKREQ) ? true : false;
}
bool
FullWifiMacHeader::IsBlockAck (void) const
{
  return (GetType () == FULL_WIFI_MAC_CTL_BACKRESP) ? true : false;
}


uint16_t
FullWifiMacHeader::GetRawDuration (void) const
{
  return m_duration;
}
Time
FullWifiMacHeader::GetDuration (void) const
{
  return MicroSeconds (m_duration);
}
uint16_t
FullWifiMacHeader::GetSequenceControl (void) const
{
  return (m_seqSeq << 4) | m_seqFrag;
}
uint16_t
FullWifiMacHeader::GetSequenceNumber (void) const
{
  return m_seqSeq;
}
uint16_t
FullWifiMacHeader::GetFragmentNumber (void) const
{
  return m_seqFrag;
}
bool
FullWifiMacHeader::IsRetry (void) const
{
  return (m_ctrlRetry == 1);
}
bool
FullWifiMacHeader::IsMoreFragments (void) const
{
  return (m_ctrlMoreFrag == 1);
}
bool
FullWifiMacHeader::IsQosBlockAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 3);
}
bool
FullWifiMacHeader::IsQosNoAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 1);
}
bool
FullWifiMacHeader::IsQosAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 0);
}
bool
FullWifiMacHeader::IsQosEosp (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosEosp == 1);
}
bool
FullWifiMacHeader::IsQosAmsdu (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_amsduPresent == 1);
}
uint8_t
FullWifiMacHeader::GetQosTid (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosTid;
}
enum FullWifiMacHeader::QosAckPolicy
FullWifiMacHeader::GetQosAckPolicy (void) const
{
  switch (m_qosAckPolicy)
    {
    case 0:
      return NORMAL_ACK;
      break;
    case 1:
      return NO_ACK;
      break;
    case 2:
      return NO_EXPLICIT_ACK;
      break;
    case 3:
      return BLOCK_ACK;
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum QosAckPolicy)-1;
}

uint8_t
FullWifiMacHeader::GetQosTxopLimit (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosStuff;
}

uint16_t
FullWifiMacHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= (m_ctrlType << 2) & (0x3 << 2);
  val |= (m_ctrlSubtype << 4) & (0xf << 4);
  val |= (m_ctrlToDs << 8) & (0x1 << 8);
  val |= (m_ctrlFromDs << 9) & (0x1 << 9);
  val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
  val |= (m_ctrlRetry << 11) & (0x1 << 11);
  val |= (m_ctrlMoreData << 13) & (0x1 << 13);
  val |= (m_ctrlWep << 14) & (0x1 << 14);
  val |= (m_ctrlOrder << 15) & (0x1 << 15);
  return val;
}

uint16_t
FullWifiMacHeader::GetQosControl (void) const
{
  uint16_t val = 0;
  val |= m_qosTid;
  val |= m_qosEosp << 4;
  val |= m_qosAckPolicy << 5;
  val |= m_amsduPresent << 7;
  val |= m_qosStuff << 8;
  return val;
}

void
FullWifiMacHeader::SetFrameControl (uint16_t ctrl)
{
  m_ctrlType = (ctrl >> 2) & 0x03;
  m_ctrlSubtype = (ctrl >> 4) & 0x0f;
  m_ctrlToDs = (ctrl >> 8) & 0x01;
  m_ctrlFromDs = (ctrl >> 9) & 0x01;
  m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
  m_ctrlRetry = (ctrl >> 11) & 0x01;
  m_ctrlMoreData = (ctrl >> 13) & 0x01;
  m_ctrlWep = (ctrl >> 14) & 0x01;
  m_ctrlOrder = (ctrl >> 15) & 0x01;
}
void
FullWifiMacHeader::SetSequenceControl (uint16_t seq)
{
  m_seqFrag = seq & 0x0f;
  m_seqSeq = (seq >> 4) & 0x0fff;
}
void
FullWifiMacHeader::SetQosControl (uint16_t qos)
{
  m_qosTid = qos & 0x000f;
  m_qosEosp = (qos >> 4) & 0x0001;
  m_qosAckPolicy = (qos >> 5) & 0x0003;
  m_amsduPresent = (qos >> 7) & 0x0001;
  m_qosStuff = (qos >> 8) & 0x00ff;
}

uint32_t
FullWifiMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          size = 2 + 2 + 6;
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          size = 2 + 2 + 6 + 6;
          break;
        }
      break;
    case TYPE_BUSY:
    case TYPE_DATA:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          size += 6;
        }
      if (m_ctrlSubtype & 0x08)
        {
          size += 2;
        }
      break;
    }
  return size;
}
const char *
FullWifiMacHeader::GetTypeString (void) const
{
#define FOO(x) \
case FULL_WIFI_MAC_ ## x: \
  return # x; \
  break;

  switch (GetType ())
    {
      FOO (CTL_RTS);
      FOO (CTL_CTS);
      FOO (CTL_ACK);
      FOO (CTL_BACKREQ);
      FOO (CTL_BACKRESP);

      FOO (MGT_BEACON);
      FOO (MGT_ASSOCIATION_REQUEST);
      FOO (MGT_ASSOCIATION_RESPONSE);
      FOO (MGT_DISASSOCIATION);
      FOO (MGT_REASSOCIATION_REQUEST);
      FOO (MGT_REASSOCIATION_RESPONSE);
      FOO (MGT_PROBE_REQUEST);
      FOO (MGT_PROBE_RESPONSE);
      FOO (MGT_AUTHENTICATION);
      FOO (MGT_DEAUTHENTICATION);
      FOO (MGT_ACTION);
      FOO (MGT_ACTION_NO_ACK);
      FOO (MGT_MULTIHOP_ACTION);

      FOO (DATA);
      FOO (DATA_CFACK);
      FOO (DATA_CFPOLL);
      FOO (DATA_CFACK_CFPOLL);
      FOO (DATA_NULL);
      FOO (DATA_NULL_CFACK);
      FOO (DATA_NULL_CFPOLL);
      FOO (DATA_NULL_CFACK_CFPOLL);
      FOO (QOSDATA);
      FOO (QOSDATA_CFACK);
      FOO (QOSDATA_CFPOLL);
      FOO (QOSDATA_CFACK_CFPOLL);
      FOO (QOSDATA_NULL);
      FOO (QOSDATA_NULL_CFPOLL);
      FOO (QOSDATA_NULL_CFACK_CFPOLL);
      FOO (BUSY_TONE);
      FOO (FORWARD_DATA);
      FOO (RETURN_DATA);
    default:
      return "ERROR";
    }
#undef FOO
  // needed to make gcc 4.0.1 ppc darwin happy.
  return "BIG_ERROR";
}

TypeId
FullWifiMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiMacHeader")
    .SetParent<Header> ()
    .AddConstructor<FullWifiMacHeader> ()
  ;
  return tid;
}

TypeId
FullWifiMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullWifiMacHeader::PrintFrameControl (std::ostream &os) const
{
  os << "ToDS=" << std::hex << (int) m_ctrlToDs << ", FromDS=" << std::hex << (int) m_ctrlFromDs
     << ", MoreFrag=" <<  std::hex << (int) m_ctrlMoreFrag << ", Retry=" << std::hex << (int) m_ctrlRetry
     << ", MoreData=" <<  std::hex << (int) m_ctrlMoreData << std::dec
  ;
}

void
FullWifiMacHeader::Print (std::ostream &os) const
{
  os << GetTypeString () << " ";
  switch (GetType ())
    {
    case FULL_WIFI_MAC_CTL_RTS:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2;
      break;
    case FULL_WIFI_MAC_CTL_CTS:
    case FULL_WIFI_MAC_CTL_ACK:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1;
      break;
    case FULL_WIFI_MAC_CTL_BACKREQ:
      break;
    case FULL_WIFI_MAC_CTL_BACKRESP:
      break;

    case FULL_WIFI_MAC_MGT_BEACON:
    case FULL_WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case FULL_WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case FULL_WIFI_MAC_MGT_DISASSOCIATION:
    case FULL_WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case FULL_WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
    case FULL_WIFI_MAC_MGT_PROBE_REQUEST:
    case FULL_WIFI_MAC_MGT_PROBE_RESPONSE:
    case FULL_WIFI_MAC_MGT_AUTHENTICATION:
    case FULL_WIFI_MAC_MGT_DEAUTHENTICATION:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2
         << ", BSSID=" << m_addr3 << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case FULL_WIFI_MAC_MGT_ACTION:
    case FULL_WIFI_MAC_MGT_ACTION_NO_ACK:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << "DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
    case FULL_WIFI_MAC_MGT_MULTIHOP_ACTION:
      os << " Duration/ID=" << m_duration << "us"
         << "RA=" << m_addr1 << ", TA=" << m_addr2 << ", DA=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
    case FULL_WIFI_MAC_DATA:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us";
      if (!m_ctrlToDs && !m_ctrlFromDs)
        {
          os << "DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3;
        }
      else if (!m_ctrlToDs && m_ctrlFromDs)
        {
          os << "DA=" << m_addr1 << ", SA=" << m_addr3 << ", BSSID=" << m_addr2;
        }
      else if (m_ctrlToDs && !m_ctrlFromDs)
        {
          os << "DA=" << m_addr3 << ", SA=" << m_addr2 << ", BSSID=" << m_addr1;
        }
      else if (m_ctrlToDs && m_ctrlFromDs)
        {
          os << "DA=" << m_addr3 << ", SA=" << m_addr4 << ", RA=" << m_addr1 << ", TA=" << m_addr2;
        }
      else
        {
          NS_FATAL_ERROR ("Impossible ToDs and FromDs flags combination");
        }
      os << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case FULL_WIFI_MAC_DATA_CFACK:
    case FULL_WIFI_MAC_DATA_CFPOLL:
    case FULL_WIFI_MAC_DATA_CFACK_CFPOLL:
    case FULL_WIFI_MAC_DATA_NULL:
    case FULL_WIFI_MAC_DATA_NULL_CFACK:
    case FULL_WIFI_MAC_DATA_NULL_CFPOLL:
    case FULL_WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA:
    case FULL_WIFI_MAC_QOSDATA_CFACK:
    case FULL_WIFI_MAC_QOSDATA_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_NULL:
    case FULL_WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case FULL_WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      break;
    case FULL_WIFI_MAC_BUSY_TONE:
    case FULL_WIFI_MAC_RETURN_DATA:
    case FULL_WIFI_MAC_FORWARD_DATA:
      break;
    }
}
uint32_t
FullWifiMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}
void
FullWifiMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  WriteTo (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      i.WriteHtolsbU16 (GetSequenceControl ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          WriteTo (i, m_addr2);
          break;
        default:
          //NOTREACHED
          NS_ASSERT (false);
          break;
        }
      break;
    case TYPE_BUSY:
    case TYPE_DATA:
      {
        WriteTo (i, m_addr2);
        WriteTo (i, m_addr3);
        i.WriteHtolsbU16 (GetSequenceControl ());
        if (m_ctrlToDs && m_ctrlFromDs)
          {
            WriteTo (i, m_addr4);
          }
        if (m_ctrlSubtype & 0x08)
          {
            i.WriteHtolsbU16 (GetQosControl ());
          }
      } break;
    default:
      //NOTREACHED
      NS_ASSERT (false);
      break;
    }
}
uint32_t
FullWifiMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  SetFrameControl (frame_control);
  m_duration = i.ReadLsbtohU16 ();
  ReadFrom (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          ReadFrom (i, m_addr2);
          break;
        }
      break;
    case TYPE_BUSY:
    case TYPE_DATA:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          ReadFrom (i, m_addr4);
        }
      if (m_ctrlSubtype & 0x08)
        {
          SetQosControl (i.ReadLsbtohU16 ());
        }
      break;
    }
  return i.GetDistanceFrom (start);
}

} // namespace ns3
