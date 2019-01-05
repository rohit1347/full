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
#ifndef FULL_EDCA_TXOP_N_H
#define FULL_EDCA_TXOP_N_H

#include "ns3/object.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"

#include "full-wifi-mode.h"
#include "full-wifi-mac-header.h"
#include "full-wifi-remote-station-manager.h"
#include "full-qos-utils.h"
#include "full-dcf.h"
#include "full-ctrl-headers.h"
#include "full-block-ack-manager.h"

#include <map>
#include <list>

namespace ns3 {

class FullDcfState;
class FullDcfManager;
class FullMacLow;
class FullMacTxMiddle;
class FullWifiMac;
class WifiMacParameters;
class FullWifiMacQueue;
class FullRandomStream;
class FullQosBlockedDestinations;
class FullMsduAggregator;
class FullMgtAddBaResponseHeader;
class FullBlockAckManager;
class FullMgtDelBaHeader;


#ifndef TYPE_OF_STATION
#define TYPE_OF_STATION
enum TypeOfStation
{
  STA,
  AP,
  ADHOC_STA,
  MESH
};
#endif /* TYPE_OF_STATION */

/**
 * \ingroup wifi
 * This queue contains packets for a particular access class.
 * possibles access classes are:
 *
 *   -AC_VO : voice, tid = 6,7         ^
 *   -AC_VI : video, tid = 4,5         |
 *   -AC_BE : best-effort, tid = 0,3   |  priority
 *   -AC_BK : background, tid = 1,2    |
 *
 * For more details see section 9.1.3.1 in 802.11 standard.
 */
class FullEdcaTxopN : public FullDcf
{
public:
  typedef Callback <void, const FullWifiMacHeader&> TxOk;
  typedef Callback <void, const FullWifiMacHeader&> TxFailed;

  static TypeId GetTypeId (void);
  FullEdcaTxopN ();
  virtual ~FullEdcaTxopN ();
  void DoDispose ();

  void SetLow (Ptr<FullMacLow> low);
  void SetTxMiddle (FullMacTxMiddle *txMiddle);
  void SetManager (FullDcfManager *manager);
  void SetTxOkCallback (TxOk callback);
  void SetTxFailedCallback (TxFailed callback);
  void SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> remoteManager);
  void SetTypeOfStation (enum TypeOfStation type);
  enum TypeOfStation GetTypeOfStation (void) const;

  Ptr<FullWifiMacQueue > GetQueue () const;
  virtual void SetMinCw (uint32_t minCw);
  virtual void SetMaxCw (uint32_t maxCw);
  virtual void SetAifsn (uint32_t aifsn);
  virtual uint32_t GetMinCw (void) const;
  virtual uint32_t GetMaxCw (void) const;
  virtual uint32_t GetAifsn (void) const;

  Ptr<FullMacLow> Low (void);
  Ptr<FullMsduAggregator> GetMsduAggregator (void) const;

  /* dcf notifications forwarded here */
  bool NeedsAccess (void) const;
  void NotifyAccessGranted (void);
  void NotifyInternalCollision (void);
  void NotifyCollision (void);
  /**
  * When a channel switching occurs, enqueued packets are removed.
  */
  void NotifyChannelSwitching (void);

  /*event handlers*/
  void GotCts (double snr, FullWifiMode txMode);
  void MissedCts (void);
  void GotAck (double snr, FullWifiMode txMode);
  void GotBlockAck (const FullCtrlBAckResponseHeader *blockAck, Mac48Address recipient);
  void MissedBlockAck (void);
  void GotAddBaResponse (const FullMgtAddBaResponseHeader *respHdr, Mac48Address recipient);
  void GotDelBaFrame (const FullMgtDelBaHeader *delBaHdr, Mac48Address recipient);
  void MissedAck (void);
  void StartNext (void);
  void Cancel (void);
  void EndTxNoAck (void);

  void RestartAccessIfNeeded (void);
  void StartAccessIfNeeded (void);
  bool NeedRts (void);
  bool NeedRtsRetransmission (void);
  bool NeedDataRetransmission (void);
  bool NeedFragmentation (void) const;
  uint32_t GetNextFragmentSize (void);
  uint32_t GetFragmentSize (void);
  uint32_t GetFragmentOffset (void);
  bool IsLastFragment (void) const;
  void NextFragment (void);
  Ptr<Packet> GetFragmentPacket (FullWifiMacHeader *hdr);

  void SetAccessCategory (enum AcIndex ac);
  void Queue (Ptr<const Packet> packet, const FullWifiMacHeader &hdr);
  void SetMsduAggregator (Ptr<FullMsduAggregator> aggr);
  void PushFront (Ptr<const Packet> packet, const FullWifiMacHeader &hdr);
  void CompleteConfig (void);
  void SetBlockAckThreshold (uint8_t threshold);
  uint8_t GetBlockAckThreshold (void) const;
  void SetBlockAckInactivityTimeout (uint16_t timeout);
  void SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator);

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

private:
  // void DoStart ();
  void DoInitialize();
  /**
   * This functions are used only to correctly set addresses in a-msdu subframe.
   * If aggregating sta is a STA (in an infrastructured network):
   *   SA = Address2
   *   DA = Address3
   * If aggregating sta is an AP
   *   SA = Address3
   *   DA = Address1
   */
  Mac48Address MapSrcAddressForAggregation (const FullWifiMacHeader &hdr);
  Mac48Address MapDestAddressForAggregation (const FullWifiMacHeader &hdr);
  FullEdcaTxopN &operator = (const FullEdcaTxopN &);
  FullEdcaTxopN (const FullEdcaTxopN &);

  /* If number of packets in the queue reaches m_blockAckThreshold value, an ADDBARequest frame
   * is sent to destination in order to setup a block ack.
   */
  bool SetupBlockAckIfNeeded ();
  /* Sends an ADDBARequest to establish a block ack agreement with sta
   * addressed by <i>recipient</i> for tid <i>tid</i>.
   */
  void SendAddBaRequest (Mac48Address recipient, uint8_t tid, uint16_t startSeq,
                         uint16_t timeout, bool immediateBAck);
  /* After that all packets, for which a block ack agreement was established, have been
   * transmitted, we have to send a block ack request.
   */
  void SendBlockAckRequest (const struct Bar &bar);
  /* For now is typically invoked to complete transmission of a packets sent with ack policy
   * Block Ack: the packet is buffered and dcf is reset.
   */
  void CompleteTx (void);
  /* Verifies if dequeued packet has to be transmitted with ack policy Block Ack. This happens
   * if an established block ack agreement exists with the receiver.
   */
  void VerifyBlockAck (void);

  AcIndex m_ac;
  class FullDcf;
  class FullTransmissionListener;
  class FullBlockAckEventListener;
  friend class FullDcf;
  friend class FullTransmissionListener;
  FullDcf *m_dcf;
  FullDcfManager *m_manager;
  Ptr<FullWifiMacQueue> m_queue;
  TxOk m_txOkCallback;
  TxFailed m_txFailedCallback;
  Ptr<FullMacLow> m_low;
  FullMacTxMiddle *m_txMiddle;
  FullTransmissionListener *m_transmissionListener;
  FullBlockAckEventListener *m_blockAckListener;
  FullRandomStream *m_rng;
  Ptr<FullWifiRemoteStationManager> m_stationManager;
  uint8_t m_fragmentNumber;

  /* current packet could be a simple MSDU or, if an aggregator for this queue is
     present, could be an A-MSDU.
   */
  Ptr<const Packet> m_currentPacket;

  FullWifiMacHeader m_currentHdr;
  Ptr<FullMsduAggregator> m_aggregator;
  TypeOfStation m_typeOfStation;
  FullQosBlockedDestinations *m_qosBlockedDestinations;
  FullBlockAckManager *m_baManager;
  /*
   * Represents the minimum number of packets for use of block ack.
   */
  uint8_t m_blockAckThreshold;
  enum BlockAckType m_blockAckType;
  Time m_currentPacketTimestamp;
  uint16_t m_blockAckInactivityTimeout;
  struct Bar m_currentBar;
};

}  // namespace ns3

#endif /* FULL_EDCA_TXOP_N_H */
