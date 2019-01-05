/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
#include "full-mgt-headers.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"

namespace ns3 {

/***********************************************************
 *          Probe Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtProbeRequestHeader);

FullMgtProbeRequestHeader::~FullMgtProbeRequestHeader ()
{
}

void
FullMgtProbeRequestHeader::SetSsid (FullSsid ssid)
{
  m_ssid = ssid;
}
FullSsid
FullMgtProbeRequestHeader::GetSsid (void) const
{
  return m_ssid;
}
void
FullMgtProbeRequestHeader::SetSupportedRates (FullSupportedRates rates)
{
  m_rates = rates;
}

FullSupportedRates
FullMgtProbeRequestHeader::GetSupportedRates (void) const
{
  return m_rates;
}
uint32_t
FullMgtProbeRequestHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += m_ssid.GetSerializedSize ();
  size += m_rates.GetSerializedSize ();
  size += m_rates.extended.GetSerializedSize ();
  return size;
}
TypeId
FullMgtProbeRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtProbeRequestHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtProbeRequestHeader> ()
  ;
  return tid;
}
TypeId
FullMgtProbeRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
FullMgtProbeRequestHeader::Print (std::ostream &os) const
{
  os << "ssid=" << m_ssid << ", "
     << "rates=" << m_rates;
}
void
FullMgtProbeRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i = m_ssid.Serialize (i);
  i = m_rates.Serialize (i);
  i = m_rates.extended.Serialize (i);
}
uint32_t
FullMgtProbeRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i = m_ssid.Deserialize (i);
  i = m_rates.Deserialize (i);
  i = m_rates.extended.DeserializeIfPresent (i);
  return i.GetDistanceFrom (start);
}


/***********************************************************
 *          Probe Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtProbeResponseHeader);

FullMgtProbeResponseHeader::FullMgtProbeResponseHeader ()
{
}
FullMgtProbeResponseHeader::~FullMgtProbeResponseHeader ()
{
}
uint64_t
FullMgtProbeResponseHeader::GetTimestamp ()
{
  return m_timestamp;
}
FullSsid
FullMgtProbeResponseHeader::GetSsid (void) const
{
  return m_ssid;
}
uint64_t
FullMgtProbeResponseHeader::GetBeaconIntervalUs (void) const
{
  return m_beaconInterval;
}
FullSupportedRates
FullMgtProbeResponseHeader::GetSupportedRates (void) const
{
  return m_rates;
}

void
FullMgtProbeResponseHeader::SetSsid (FullSsid ssid)
{
  m_ssid = ssid;
}
void
FullMgtProbeResponseHeader::SetBeaconIntervalUs (uint64_t us)
{
  m_beaconInterval = us;
}
void
FullMgtProbeResponseHeader::SetSupportedRates (FullSupportedRates rates)
{
  m_rates = rates;
}
TypeId
FullMgtProbeResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtProbeResponseHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtProbeResponseHeader> ()
  ;
  return tid;
}
TypeId
FullMgtProbeResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
FullMgtProbeResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 8; // timestamp
  size += 2; // beacon interval
  size += m_capability.GetSerializedSize ();
  size += m_ssid.GetSerializedSize ();
  size += m_rates.GetSerializedSize ();
  //size += 3; // ds parameter set
  size += m_rates.extended.GetSerializedSize ();
  // xxx
  return size;
}
void
FullMgtProbeResponseHeader::Print (std::ostream &os) const
{
  os << "ssid=" << m_ssid << ", "
     << "rates=" << m_rates;
}
void
FullMgtProbeResponseHeader::Serialize (Buffer::Iterator start) const
{
  // timestamp
  // beacon interval
  // capability information
  // ssid
  // supported rates
  // fh parameter set
  // ds parameter set
  // cf parameter set
  // ibss parameter set
  //XXX
  Buffer::Iterator i = start;
  i.WriteHtolsbU64 (Simulator::Now ().GetMicroSeconds ());
  i.WriteHtolsbU16 (m_beaconInterval / 1024);
  i = m_capability.Serialize (i);
  i = m_ssid.Serialize (i);
  i = m_rates.Serialize (i);
  //i.WriteU8 (0, 3); // ds parameter set.
  i = m_rates.extended.Serialize (i);
}
uint32_t
FullMgtProbeResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_timestamp = i.ReadLsbtohU64 ();
  m_beaconInterval = i.ReadLsbtohU16 ();
  m_beaconInterval *= 1024;
  i = m_capability.Deserialize (i);
  i = m_ssid.Deserialize (i);
  i = m_rates.Deserialize (i);
  //i.Next (3); // ds parameter set
  i = m_rates.extended.DeserializeIfPresent (i);
  return i.GetDistanceFrom (start);
}

/***********************************************************
 *          Assoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtAssocRequestHeader);

FullMgtAssocRequestHeader::FullMgtAssocRequestHeader ()
  : m_listenInterval (0)
{
}
FullMgtAssocRequestHeader::~FullMgtAssocRequestHeader ()
{
}

void
FullMgtAssocRequestHeader::SetSsid (FullSsid ssid)
{
  m_ssid = ssid;
}
void
FullMgtAssocRequestHeader::SetSupportedRates (FullSupportedRates rates)
{
  m_rates = rates;
}
void
FullMgtAssocRequestHeader::SetListenInterval (uint16_t interval)
{
  m_listenInterval = interval;
}
FullSsid
FullMgtAssocRequestHeader::GetSsid (void) const
{
  return m_ssid;
}
FullSupportedRates
FullMgtAssocRequestHeader::GetSupportedRates (void) const
{
  return m_rates;
}
uint16_t
FullMgtAssocRequestHeader::GetListenInterval (void) const
{
  return m_listenInterval;
}

TypeId
FullMgtAssocRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtAssocRequestHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtAssocRequestHeader> ()
  ;
  return tid;
}
TypeId
FullMgtAssocRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
FullMgtAssocRequestHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += m_capability.GetSerializedSize ();
  size += 2;
  size += m_ssid.GetSerializedSize ();
  size += m_rates.GetSerializedSize ();
  size += m_rates.extended.GetSerializedSize ();
  return size;
}
void
FullMgtAssocRequestHeader::Print (std::ostream &os) const
{
  os << "ssid=" << m_ssid << ", "
     << "rates=" << m_rates;
}
void
FullMgtAssocRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i = m_capability.Serialize (i);
  i.WriteHtolsbU16 (m_listenInterval);
  i = m_ssid.Serialize (i);
  i = m_rates.Serialize (i);
  i = m_rates.extended.Serialize (i);
}
uint32_t
FullMgtAssocRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i = m_capability.Deserialize (i);
  m_listenInterval = i.ReadLsbtohU16 ();
  i = m_ssid.Deserialize (i);
  i = m_rates.Deserialize (i);
  i = m_rates.extended.DeserializeIfPresent (i);
  return i.GetDistanceFrom (start);
}

/***********************************************************
 *          Assoc Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtAssocResponseHeader);

FullMgtAssocResponseHeader::FullMgtAssocResponseHeader ()
  : m_aid (0)
{
}
FullMgtAssocResponseHeader::~FullMgtAssocResponseHeader ()
{
}

FullStatusCode
FullMgtAssocResponseHeader::GetStatusCode (void)
{
  return m_code;
}
FullSupportedRates
FullMgtAssocResponseHeader::GetSupportedRates (void)
{
  return m_rates;
}
void
FullMgtAssocResponseHeader::SetStatusCode (FullStatusCode code)
{
  m_code = code;
}
void
FullMgtAssocResponseHeader::SetSupportedRates (FullSupportedRates rates)
{
  m_rates = rates;
}

TypeId
FullMgtAssocResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtAssocResponseHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtAssocResponseHeader> ()
  ;
  return tid;
}
TypeId
FullMgtAssocResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
FullMgtAssocResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += m_capability.GetSerializedSize ();
  size += m_code.GetSerializedSize ();
  size += 2; // aid
  size += m_rates.GetSerializedSize ();
  size += m_rates.extended.GetSerializedSize ();
  return size;
}

void
FullMgtAssocResponseHeader::Print (std::ostream &os) const
{
  os << "status code=" << m_code << ", "
     << "rates=" << m_rates;
}
void
FullMgtAssocResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i = m_capability.Serialize (i);
  i = m_code.Serialize (i);
  i.WriteHtolsbU16 (m_aid);
  i = m_rates.Serialize (i);
  i = m_rates.extended.Serialize (i);
}
uint32_t
FullMgtAssocResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i = m_capability.Deserialize (i);
  i = m_code.Deserialize (i);
  m_aid = i.ReadLsbtohU16 ();
  i = m_rates.Deserialize (i);
  i = m_rates.extended.DeserializeIfPresent (i);
  return i.GetDistanceFrom (start);
}
/**********************************************************
 *   ActionFrame
 **********************************************************/
FullWifiActionHeader::FullWifiActionHeader ()
{
}
FullWifiActionHeader::~FullWifiActionHeader ()
{
}
void
FullWifiActionHeader::SetAction (FullWifiActionHeader::CategoryValue type,
                             FullWifiActionHeader::ActionValue action)
{
  m_category = type;

  switch (type)
    {
    case BLOCK_ACK:
      {
        m_actionValue = action.blockAck;
        break;
      }
    case MESH_PEERING_MGT:
      {
        m_actionValue = action.peerLink;
        break;
      }
    case MESH_PATH_SELECTION:
      {
        m_actionValue = action.pathSelection;
        break;
      }
    case MESH_LINK_METRIC:
    case MESH_INTERWORKING:
    case MESH_RESOURCE_COORDINATION:
    case MESH_PROXY_FORWARDING:
      break;
    }
}
FullWifiActionHeader::CategoryValue
FullWifiActionHeader::GetCategory ()
{
  switch (m_category)
    {
    case BLOCK_ACK:
      return BLOCK_ACK;
    case MESH_PEERING_MGT:
      return MESH_PEERING_MGT;
    case MESH_LINK_METRIC:
      return MESH_LINK_METRIC;
    case MESH_PATH_SELECTION:
      return MESH_PATH_SELECTION;
    case MESH_INTERWORKING:
      return MESH_INTERWORKING;
    case MESH_RESOURCE_COORDINATION:
      return MESH_RESOURCE_COORDINATION;
    case MESH_PROXY_FORWARDING:
      return MESH_PROXY_FORWARDING;
    default:
      NS_FATAL_ERROR ("Unknown action value");
      return MESH_PEERING_MGT;
    }
}
FullWifiActionHeader::ActionValue
FullWifiActionHeader::GetAction ()
{
  ActionValue retval;
  retval.peerLink = PEER_LINK_OPEN; // Needs to be initialized to something to quiet valgrind in default cases
  switch (m_category)
    {
    case BLOCK_ACK:
      switch (m_actionValue)
        {
        case BLOCK_ACK_ADDBA_REQUEST:
          retval.blockAck = BLOCK_ACK_ADDBA_REQUEST;
          return retval;
        case BLOCK_ACK_ADDBA_RESPONSE:
          retval.blockAck = BLOCK_ACK_ADDBA_RESPONSE;
          return retval;
        case BLOCK_ACK_DELBA:
          retval.blockAck = BLOCK_ACK_DELBA;
          return retval;
        }
    case MESH_PEERING_MGT:
      switch (m_actionValue)
        {
        case PEER_LINK_OPEN:
          retval.peerLink = PEER_LINK_OPEN;
          return retval;
        case PEER_LINK_CONFIRM:
          retval.peerLink = PEER_LINK_CONFIRM;
          return retval;
        case PEER_LINK_CLOSE:
          retval.peerLink = PEER_LINK_CLOSE;
          return retval;
        default:
          NS_FATAL_ERROR ("Unknown mesh peering management action code");
          retval.peerLink = PEER_LINK_OPEN; /* quiet compiler */
          return retval;
        }
    case MESH_PATH_SELECTION:
      switch (m_actionValue)
        {
        case PATH_SELECTION:
          retval.pathSelection = PATH_SELECTION;
          return retval;
        default:
          NS_FATAL_ERROR ("Unknown mesh path selection action code");
          retval.peerLink = PEER_LINK_OPEN; /* quiet compiler */
          return retval;
        }
    case MESH_LINK_METRIC:
    // not yet supported
    case MESH_INTERWORKING:
    // not yet supported
    case MESH_RESOURCE_COORDINATION:
    // not yet supported
    default:
      NS_FATAL_ERROR ("Unsupported mesh action");
      retval.peerLink = PEER_LINK_OPEN; /* quiet compiler */
      return retval;
    }
}
TypeId
FullWifiActionHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FullWifiActionHeader")
    .SetParent<Header> ()
    .AddConstructor<FullWifiActionHeader> ();
  return tid;
}
TypeId
FullWifiActionHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}
void
FullWifiActionHeader::Print (std::ostream &os) const
{
}
uint32_t
FullWifiActionHeader::GetSerializedSize () const
{
  return 2;
}
void
FullWifiActionHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_category);
  start.WriteU8 (m_actionValue);
}
uint32_t
FullWifiActionHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_category = i.ReadU8 ();
  m_actionValue = i.ReadU8 ();
  return i.GetDistanceFrom (start);
}

/***************************************************
*                 ADDBARequest
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtAddBaRequestHeader);

FullMgtAddBaRequestHeader::FullMgtAddBaRequestHeader ()
  : m_dialogToken (1),
    m_amsduSupport (1),
    m_bufferSize (0)
{
}

TypeId
FullMgtAddBaRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtAddBaRequestHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtAddBaRequestHeader> ();
  return tid;
}

TypeId
FullMgtAddBaRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullMgtAddBaRequestHeader::Print (std::ostream &os) const
{
}

uint32_t
FullMgtAddBaRequestHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog token
  size += 2; //Block ack parameter set
  size += 2; //Block ack timeout value
  size += 2; //Starting sequence control
  return size;
}

void
FullMgtAddBaRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (GetParameterSet ());
  i.WriteHtolsbU16 (m_timeoutValue);
  i.WriteHtolsbU16 (GetStartingSequenceControl ());
}

uint32_t
FullMgtAddBaRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  SetParameterSet (i.ReadLsbtohU16 ());
  m_timeoutValue = i.ReadLsbtohU16 ();
  SetStartingSequenceControl (i.ReadLsbtohU16 ());
  return i.GetDistanceFrom (start);
}

void
FullMgtAddBaRequestHeader::SetDelayedBlockAck ()
{
  m_policy = 0;
}

void
FullMgtAddBaRequestHeader::SetImmediateBlockAck ()
{
  m_policy = 1;
}

void
FullMgtAddBaRequestHeader::SetTid (uint8_t tid)
{
  NS_ASSERT (tid < 16);
  m_tid = tid;
}

void
FullMgtAddBaRequestHeader::SetTimeout (uint16_t timeout)
{
  m_timeoutValue = timeout;
}

void
FullMgtAddBaRequestHeader::SetBufferSize (uint16_t size)
{
  m_bufferSize = size;
}

void
FullMgtAddBaRequestHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

void
FullMgtAddBaRequestHeader::SetAmsduSupport (bool supported)
{
  m_amsduSupport = supported;
}

uint8_t
FullMgtAddBaRequestHeader::GetTid (void) const
{
  return m_tid;
}

bool
FullMgtAddBaRequestHeader::IsImmediateBlockAck (void) const
{
  return (m_policy == 1) ? true : false;
}

uint16_t
FullMgtAddBaRequestHeader::GetTimeout (void) const
{
  return m_timeoutValue;
}

uint16_t
FullMgtAddBaRequestHeader::GetBufferSize (void) const
{
  return m_bufferSize;
}

bool
FullMgtAddBaRequestHeader::IsAmsduSupported (void) const
{
  return (m_amsduSupport == 1) ? true : false;
}

uint16_t
FullMgtAddBaRequestHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

uint16_t
FullMgtAddBaRequestHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
FullMgtAddBaRequestHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

uint16_t
FullMgtAddBaRequestHeader::GetParameterSet (void) const
{
  uint16_t res = 0;
  res |= m_amsduSupport;
  res |= m_policy << 1;
  res |= m_tid << 2;
  res |= m_bufferSize << 6;
  return res;
}

void
FullMgtAddBaRequestHeader::SetParameterSet (uint16_t params)
{
  m_amsduSupport = (params) & 0x01;
  m_policy = (params >> 1) & 0x01;
  m_tid = (params >> 2) & 0x0f;
  m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
*                 ADDBAResponse
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtAddBaResponseHeader);

FullMgtAddBaResponseHeader::FullMgtAddBaResponseHeader ()
  : m_dialogToken (1),
    m_amsduSupport (1),
    m_bufferSize (0)
{
}

TypeId
FullMgtAddBaResponseHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FullMgtAddBaResponseHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtAddBaResponseHeader> ()
  ;
  return tid;
}

TypeId
FullMgtAddBaResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullMgtAddBaResponseHeader::Print (std::ostream &os) const
{
  os << "status code=" << m_code;
}

uint32_t
FullMgtAddBaResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog token
  size += m_code.GetSerializedSize (); //Status code
  size += 2; //Block ack parameter set
  size += 2; //Block ack timeout value
  return size;
}

void
FullMgtAddBaResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i = m_code.Serialize (i);
  i.WriteHtolsbU16 (GetParameterSet ());
  i.WriteHtolsbU16 (m_timeoutValue);
}

uint32_t
FullMgtAddBaResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  i = m_code.Deserialize (i);
  SetParameterSet (i.ReadLsbtohU16 ());
  m_timeoutValue = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
FullMgtAddBaResponseHeader::SetDelayedBlockAck ()
{
  m_policy = 0;
}

void
FullMgtAddBaResponseHeader::SetImmediateBlockAck ()
{
  m_policy = 1;
}

void
FullMgtAddBaResponseHeader::SetTid (uint8_t tid)
{
  NS_ASSERT (tid < 16);
  m_tid = tid;
}

void
FullMgtAddBaResponseHeader::SetTimeout (uint16_t timeout)
{
  m_timeoutValue = timeout;
}

void
FullMgtAddBaResponseHeader::SetBufferSize (uint16_t size)
{
  m_bufferSize = size;
}

void
FullMgtAddBaResponseHeader::SetStatusCode (FullStatusCode code)
{
  m_code = code;
}

void
FullMgtAddBaResponseHeader::SetAmsduSupport (bool supported)
{
  m_amsduSupport = supported;
}

FullStatusCode
FullMgtAddBaResponseHeader::GetStatusCode (void) const
{
  return m_code;
}

uint8_t
FullMgtAddBaResponseHeader::GetTid (void) const
{
  return m_tid;
}

bool
FullMgtAddBaResponseHeader::IsImmediateBlockAck (void) const
{
  return (m_policy == 1) ? true : false;
}

uint16_t
FullMgtAddBaResponseHeader::GetTimeout (void) const
{
  return m_timeoutValue;
}

uint16_t
FullMgtAddBaResponseHeader::GetBufferSize (void) const
{
  return m_bufferSize;
}

bool
FullMgtAddBaResponseHeader::IsAmsduSupported (void) const
{
  return (m_amsduSupport == 1) ? true : false;
}

uint16_t
FullMgtAddBaResponseHeader::GetParameterSet (void) const
{
  uint16_t res = 0;
  res |= m_amsduSupport;
  res |= m_policy << 1;
  res |= m_tid << 2;
  res |= m_bufferSize << 6;
  return res;
}

void
FullMgtAddBaResponseHeader::SetParameterSet (uint16_t params)
{
  m_amsduSupport = (params) & 0x01;
  m_policy = (params >> 1) & 0x01;
  m_tid = (params >> 2) & 0x0f;
  m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
*                     DelBa
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullMgtDelBaHeader);

FullMgtDelBaHeader::FullMgtDelBaHeader ()
  : m_reasonCode (1)
{
}

TypeId
FullMgtDelBaHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullMgtDelBaHeader")
    .SetParent<Header> ()
    .AddConstructor<FullMgtDelBaHeader> ()
  ;
  return tid;
}

TypeId
FullMgtDelBaHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullMgtDelBaHeader::Print (std::ostream &os) const
{
}

uint32_t
FullMgtDelBaHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //DelBa parameter set
  size += 2; //Reason code
  return size;
}

void
FullMgtDelBaHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetParameterSet ());
  i.WriteHtolsbU16 (m_reasonCode);
}

uint32_t
FullMgtDelBaHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetParameterSet (i.ReadLsbtohU16 ());
  m_reasonCode = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

bool
FullMgtDelBaHeader::IsByOriginator (void) const
{
  return (m_initiator == 1) ? true : false;
}

uint8_t
FullMgtDelBaHeader::GetTid (void) const
{
  NS_ASSERT (m_tid < 16);
  uint8_t tid = static_cast<uint8_t> (m_tid);
  return tid;
}

void
FullMgtDelBaHeader::SetByOriginator (void)
{
  m_initiator = 1;
}

void
FullMgtDelBaHeader::SetByRecipient (void)
{
  m_initiator = 0;
}

void
FullMgtDelBaHeader::SetTid (uint8_t tid)
{
  NS_ASSERT (tid < 16);
  m_tid = static_cast<uint16_t> (tid);
}

uint16_t
FullMgtDelBaHeader::GetParameterSet (void) const
{
  uint16_t res = 0;
  res |= m_initiator << 11;
  res |= m_tid << 12;
  return res;
}

void
FullMgtDelBaHeader::SetParameterSet (uint16_t params)
{
  m_initiator = (params >> 11) & 0x01;
  m_tid = (params >> 12) & 0x0f;
}

} // namespace ns3
