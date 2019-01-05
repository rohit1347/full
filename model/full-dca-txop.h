/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 */

#ifndef FULL_DCA_TXOP_H
#define FULL_DCA_TXOP_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/full-wifi-mac-header.h"
#include "ns3/full-wifi-mode.h"
#include "ns3/full-wifi-remote-station-manager.h"
#include "ns3/full-dcf.h"
#include "ns3/full-wifi-preamble.h"
#include "ns3/event-id.h"

#include <algorithm>


namespace ns3 {

class FullDcfState;
class FullDcfManager;
class FullWifiMacQueue;
class FullMacLow;
class FullWifiMacParameters;
class FullMacTxMiddle;
class FullRandomStream;
class FullMacStation;
class FullMacStations;


struct ForwardItem
{
  ForwardItem (Mac48Address add, double p)
    :add (add), priority (p) {};

  Mac48Address add;
  double priority;

  bool operator == (const ForwardItem &p1)
  {
     if(p1.add == add && p1.priority == priority)
       return true;
     else
       return false;
  }
};

struct sort_pred {
  bool operator()(const ForwardItem &left, const ForwardItem &right) {
    return left.priority < right.priority;
  }
};

class ForwardMap
{
public:
  ForwardMap () {};
  ForwardMap (Mac48Address tx)
    : m_primaryTransmitter (tx) {};
  ForwardMap (Mac48Address tx, std::list<ForwardItem> queue)
    : m_primaryTransmitter (tx), m_forwardQueue (queue) {};

  void AddItem (ForwardItem newItem) { m_forwardQueue.push_back (newItem); SortQueue ();}
  void SetQueue (std::list<ForwardItem> queue) { m_forwardQueue = queue; }
  void SortQueue ();
  void SetTransmitterAddress (Mac48Address add) { m_primaryTransmitter = add; }
  uint32_t Size () { return m_forwardQueue.size (); }
  Mac48Address GetTransmitterAddress (void) const { return m_primaryTransmitter; }
  std::list<ForwardItem> GetQueue (void) const { return m_forwardQueue; }

  void RemoveItem (Mac48Address add);
  ForwardItem* GetItem (Mac48Address add) ;
  void UpdateItem (Mac48Address add, double p);

private:
  Mac48Address m_primaryTransmitter;
  std::list<ForwardItem> m_forwardQueue;

};



//store the forwarding policies for different transmitter in one list
class ForwardQueue :public Object
{
public:
  ForwardQueue () {};
  ForwardQueue (std::list<ForwardMap> queue)
    : m_forwardQueue (queue) {};

  void AddForwardMap (ForwardMap newItem) { m_forwardQueue.push_back (newItem); }
  std::list<ForwardMap> GetQueue (void) const { return m_forwardQueue; }
  uint32_t Size () { return m_forwardQueue.size (); }

  ForwardMap *GetForwardMap (Mac48Address add) ;

private:
  Mac48Address m_primaryTransmitter;
  std::list<ForwardMap> m_forwardQueue;
};


/**
 * \brief handle packet fragmentation and retransmissions.
 * \ingroup wifi
 *
 * This class implements the packet fragmentation and
 * retransmission policy. It uses the ns3::MacLow and ns3::DcfManager
 * helper classes to respectively send packets and decide when
 * to send them. Packets are stored in a ns3::WifiMacQueue until
 * they can be sent.
 *
 * The policy currently implemented uses a simple fragmentation
 * threshold: any packet bigger than this threshold is fragmented
 * in fragments whose size is smaller than the threshold.
 *
 * The retransmission policy is also very simple: every packet is
 * retransmitted until it is either successfully transmitted or
 * it has been retransmitted up until the ssrc or slrc thresholds.
 *
 * The rts/cts policy is similar to the fragmentation policy: when
 * a packet is bigger than a threshold, the rts/cts protocol is used.
 */
class FullDcaTxop : public FullDcf
{
public:
  static TypeId GetTypeId (void);

  typedef Callback <void, const FullWifiMacHeader&> TxOk;
  typedef Callback <void, const FullWifiMacHeader&> TxFailed;

  FullDcaTxop ();
  ~FullDcaTxop ();

  void SetLow (Ptr<FullMacLow> low);
  void SetManager (FullDcfManager *manager);
  void SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> remoteManager);

  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxOk callback);
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed unsuccessfully.
   */
  void SetTxFailedCallback (TxFailed callback);

  void SetAckTimeoutCallback (TxFailed callback);
  void SetSendPacketCallback (TxFailed callback);

  Ptr<FullWifiMacQueue > GetQueue () const;
  virtual void SetMinCw (uint32_t minCw);
  virtual void SetMaxCw (uint32_t maxCw);
  virtual void SetAifsn (uint32_t aifsn);
  virtual uint32_t GetMinCw (void) const;
  virtual uint32_t GetMaxCw (void) const;
  virtual uint32_t GetAifsn (void) const;


  void SetEnableBusyTone (bool busy);
  void SetEnableReturnPacket (bool re);
  void SetEnableForward (bool forward);
  void SetForwardQueue (Ptr<ForwardQueue> forwardQueue);
  bool GetEnableBusyTone (void) const;
  bool GetEnableReturnPacket (void) const;
  bool GetEnableForward (void) const;
  Ptr<ForwardQueue> GetForwardQueue (void) const;
  void SendBusyTone (Time duration, Mac48Address dst);

  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  void Queue (Ptr<const Packet> packet, const FullWifiMacHeader &hdr);

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
  class FullTransmissionListener;
  class FullNavListener;
  class FullPhyListener;
  class FullDcf;
  friend class FullDcf;
  friend class FullTransmissionListener;

  FullDcaTxop &operator = (const FullDcaTxop &);
  FullDcaTxop (const FullDcaTxop &o);

  // Inherited from ns3::Object
  Ptr<FullMacLow> Low (void);
  void DoInitialize ();
  /* dcf notifications forwarded here */
  bool NeedsAccess (void) const;
  void NotifyAccessGranted (void);
  void NotifyInternalCollision (void);
  void NotifyCollision (void);
  void NotifyRxStartNow (Time duration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble);
  /**
  * When a channel switching occurs, enqueued packets are removed.
  */
  void NotifyChannelSwitching (void);
  /* event handlers */
  void GotCts (double snr, FullWifiMode txMode);
  void MissedCts (void);
  void GotAck (double snr, FullWifiMode txMode);
  void MissedAck (void);
  void StartNext (void);
  void Cancel (void);
  void EndTxNoAck (void);

  void RestartAccessIfNeeded (void);
  void StartAccessIfNeeded (void);
  bool NeedRts (Ptr<const Packet> packet, const FullWifiMacHeader *header);
  bool NeedRtsRetransmission (void);
  bool NeedDataRetransmission (void);
  bool NeedFragmentation (void);
  uint32_t GetNextFragmentSize (void);
  uint32_t GetFragmentSize (void);
  uint32_t GetFragmentOffset (void);
  bool IsLastFragment (void);
  void NextFragment (void);
  Ptr<Packet> GetFragmentPacket (FullWifiMacHeader *hdr);
  virtual void DoDispose (void);

  Ptr<const Packet> CheckForReturnPacket (FullWifiMacHeader *hdr, Mac48Address src);
  Ptr<const Packet> CheckForForwardPacket (FullWifiMacHeader *hdr, Mac48Address src);

  FullDcf *m_dcf;
  FullDcfManager *m_manager;
  TxOk m_txOkCallback;
  TxFailed m_txFailedCallback;
  TxFailed m_ackTimeoutCallback;
  TxFailed m_sendPacketCallback;
  Ptr<FullWifiMacQueue> m_queue;
  FullMacTxMiddle *m_txMiddle;
  Ptr <FullMacLow> m_low;
  Ptr<FullWifiRemoteStationManager> m_stationManager;
  FullTransmissionListener *m_transmissionListener;
  FullRandomStream *m_rng;

  bool m_accessOngoing;
  Ptr<const Packet> m_currentPacket;
  FullWifiMacHeader m_currentHdr;
  uint8_t m_fragmentNumber;

  bool m_enableBusyTone;
  bool m_enableReturnPacket;
  bool m_enableForward;
  // the return address, the forward address, a null address (or broadcast address) if busytone enabled
  // the double value is the priority for the address
  Ptr<ForwardQueue> m_forwardQueue;
  EventId m_returnEvent;

};

} // namespace ns3



#endif /* FULL_DCA_TXOP_H */
