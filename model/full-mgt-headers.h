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
#ifndef FULL_MGT_HEADERS_H
#define FULL_MGT_HEADERS_H

#include <stdint.h>

#include "ns3/header.h"
#include "full-status-code.h"
#include "full-capability-information.h"
#include "full-supported-rates.h"
#include "full-ssid.h"

namespace ns3 {

/**
 * \ingroup wifi
 * Implement the header for management frames of type association request.
 */
class FullMgtAssocRequestHeader : public Header
{
public:
  FullMgtAssocRequestHeader ();
  ~FullMgtAssocRequestHeader ();

  void SetSsid (FullSsid ssid);
  void SetSupportedRates (FullSupportedRates rates);
  void SetListenInterval (uint16_t interval);

  FullSsid GetSsid (void) const;
  FullSupportedRates GetSupportedRates (void) const;
  uint16_t GetListenInterval (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  FullSsid m_ssid;
  FullSupportedRates m_rates;
  FullCapabilityInformation m_capability;
  uint16_t m_listenInterval;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type association response.
 */
class FullMgtAssocResponseHeader : public Header
{
public:
  FullMgtAssocResponseHeader ();
  ~FullMgtAssocResponseHeader ();

  FullStatusCode GetStatusCode (void);
  FullSupportedRates GetSupportedRates (void);

  void SetSupportedRates (FullSupportedRates rates);
  void SetStatusCode (FullStatusCode code);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  FullSupportedRates m_rates;
  FullCapabilityInformation m_capability;
  FullStatusCode m_code;
  uint16_t m_aid;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type probe request.
 */
class FullMgtProbeRequestHeader : public Header
{
public:
  ~FullMgtProbeRequestHeader ();

  void SetSsid (FullSsid ssid);
  void SetSupportedRates (FullSupportedRates rates);
  FullSsid GetSsid (void) const;
  FullSupportedRates GetSupportedRates (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
private:
  FullSsid m_ssid;
  FullSupportedRates m_rates;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type probe response.
 */
class FullMgtProbeResponseHeader : public Header
{
public:
  FullMgtProbeResponseHeader ();
  ~FullMgtProbeResponseHeader ();

  FullSsid GetSsid (void) const;
  uint64_t GetBeaconIntervalUs (void) const;
  FullSupportedRates GetSupportedRates (void) const;

  void SetSsid (FullSsid ssid);
  void SetBeaconIntervalUs (uint64_t us);
  void SetSupportedRates (FullSupportedRates rates);
  uint64_t GetTimestamp ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint64_t m_timestamp;
  FullSsid m_ssid;
  uint64_t m_beaconInterval;
  FullSupportedRates m_rates;
  FullCapabilityInformation m_capability;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type beacon.
 */
class FullMgtBeaconHeader : public FullMgtProbeResponseHeader
{
};

/****************************
*     Action frames
*****************************/

/**
 * \ingroup wifi
 *
 * See IEEE 802.11 chapter 7.3.1.11
 * Header format: | category: 1 | action value: 1 |
 *
 */
class FullWifiActionHeader : public Header
{
public:
  FullWifiActionHeader ();
  ~FullWifiActionHeader ();

  /* Compatible with open80211s implementation */
  enum CategoryValue //table 7-24 staring from 4
  {
    BLOCK_ACK = 3,
    MESH_PEERING_MGT = 30,
    MESH_LINK_METRIC = 31,
    MESH_PATH_SELECTION = 32,
    MESH_INTERWORKING = 33,
    MESH_RESOURCE_COORDINATION = 34,
    MESH_PROXY_FORWARDING = 35,
  };
  /* Compatible with open80211s implementation */
  enum PeerLinkMgtActionValue
  {
    PEER_LINK_OPEN = 0,
    PEER_LINK_CONFIRM = 1,
    PEER_LINK_CLOSE = 2,
  };
  enum LinkMetricActionValue
  {
    LINK_METRIC_REQUEST = 0,
    LINK_METRIC_REPORT,
  };
  /* Compatible with open80211s implementation */
  enum PathSelectionActionValue
  {
    PATH_SELECTION = 0,
  };
  enum InterworkActionValue
  {
    PORTAL_ANNOUNCEMENT = 0,
  };
  enum ResourceCoordinationActionValue
  {
    CONGESTION_CONTROL_NOTIFICATION = 0,
    MDA_SETUP_REQUEST,
    MDA_SETUP_REPLY,
    MDAOP_ADVERTISMENT_REQUEST,
    MDAOP_ADVERTISMENTS,
    MDAOP_SET_TEARDOWN,
    BEACON_TIMING_REQUEST,
    BEACON_TIMING_RESPONSE,
    TBTT_ADJUSTMENT_REQUEST,
    MESH_CHANNEL_SWITCH_ANNOUNCEMENT,
  };
  enum BlockAckActionValue
  {
    BLOCK_ACK_ADDBA_REQUEST = 0,
    BLOCK_ACK_ADDBA_RESPONSE = 1,
    BLOCK_ACK_DELBA = 2
  };
  typedef union
  {
    enum PeerLinkMgtActionValue peerLink;
    enum LinkMetricActionValue linkMetrtic;
    enum PathSelectionActionValue pathSelection;
    enum InterworkActionValue interwork;
    enum ResourceCoordinationActionValue resourceCoordination;
    enum BlockAckActionValue blockAck;
  } ActionValue;
  void   SetAction (enum CategoryValue type,ActionValue action);

  CategoryValue GetCategory ();
  ActionValue GetAction ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId () const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize () const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
private:
  uint8_t m_category;
  uint8_t m_actionValue;
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type add block ack request.
 */
class FullMgtAddBaRequestHeader : public Header
{
public:
  FullMgtAddBaRequestHeader ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDelayedBlockAck ();
  void SetImmediateBlockAck ();
  void SetTid (uint8_t tid);
  void SetTimeout (uint16_t timeout);
  void SetBufferSize (uint16_t size);
  void SetStartingSequence (uint16_t seq);
  void SetAmsduSupport (bool supported);

  uint16_t GetStartingSequence (void) const;
  uint8_t GetTid (void) const;
  bool IsImmediateBlockAck (void) const;
  uint16_t GetTimeout (void) const;
  uint16_t GetBufferSize (void) const;
  bool IsAmsduSupported (void) const;

private:
  uint16_t GetParameterSet (void) const;
  void SetParameterSet (uint16_t params);
  uint16_t GetStartingSequenceControl (void) const;
  void SetStartingSequenceControl (uint16_t seqControl);

  uint8_t m_dialogToken; /* Not used for now */
  uint8_t m_amsduSupport;
  uint8_t m_policy;
  uint8_t m_tid;
  uint16_t m_bufferSize;
  uint16_t m_timeoutValue;
  uint16_t m_startingSeq;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type add block ack response.
 */
class FullMgtAddBaResponseHeader : public Header
{
public:
  FullMgtAddBaResponseHeader ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDelayedBlockAck ();
  void SetImmediateBlockAck ();
  void SetTid (uint8_t tid);
  void SetTimeout (uint16_t timeout);
  void SetBufferSize (uint16_t size);
  void SetStatusCode (FullStatusCode code);
  void SetAmsduSupport (bool supported);

  FullStatusCode GetStatusCode (void) const;
  uint8_t GetTid (void) const;
  bool IsImmediateBlockAck (void) const;
  uint16_t GetTimeout (void) const;
  uint16_t GetBufferSize (void) const;
  bool IsAmsduSupported (void) const;

private:
  uint16_t GetParameterSet (void) const;
  void SetParameterSet (uint16_t params);

  uint8_t m_dialogToken; /* Not used for now */
  FullStatusCode m_code;
  uint8_t m_amsduSupport;
  uint8_t m_policy;
  uint8_t m_tid;
  uint16_t m_bufferSize;
  uint16_t m_timeoutValue;
};


/**
 * \ingroup wifi
 * Implement the header for management frames of type del block ack.
 */
class FullMgtDelBaHeader : public Header
{
public:
  FullMgtDelBaHeader ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  bool IsByOriginator (void) const;
  uint8_t GetTid (void) const;
  void SetTid (uint8_t);
  void SetByOriginator (void);
  void SetByRecipient (void);

private:
  uint16_t GetParameterSet (void) const;
  void SetParameterSet (uint16_t params);

  uint16_t m_initiator;
  uint16_t m_tid;
  /* Not used for now.
     Always set to 1: "Unspecified reason" */
  uint16_t m_reasonCode;
};

} // namespace ns3

#endif /* FULL_MGT_HEADERS_H */
