/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/double.h"

#include "full-mac-low.h"
#include "full-wifi-phy.h"
#include "full-wifi-mac-trailer.h"
#include "full-qos-utils.h"
#include "full-edca-txop-n.h"

NS_LOG_COMPONENT_DEFINE ("FullMacLow");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[mac=" << m_self << "] "


namespace ns3 {

class FullSnrTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  void Set (double snr);
  double Get (void) const;
private:
  double m_snr;
};

TypeId
FullSnrTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullSnrTag")
    .SetParent<Tag> ()
    .AddConstructor<FullSnrTag> ()
    .AddAttribute ("Snr", "The snr of the last packet received",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&FullSnrTag::Get),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}
TypeId
FullSnrTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
FullSnrTag::GetSerializedSize (void) const
{
  return sizeof (double);
}
void
FullSnrTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_snr);
}
void
FullSnrTag::Deserialize (TagBuffer i)
{
  m_snr = i.ReadDouble ();
}
void
FullSnrTag::Print (std::ostream &os) const
{
  os << "Snr=" << m_snr;
}
void
FullSnrTag::Set (double snr)
{
  m_snr = snr;
}
double
FullSnrTag::Get (void) const
{
  return m_snr;
}


FullMacLowTransmissionListener::FullMacLowTransmissionListener ()
{
}
FullMacLowTransmissionListener::~FullMacLowTransmissionListener ()
{
}
void
FullMacLowTransmissionListener::GotBlockAck (const FullCtrlBAckResponseHeader *blockAck,
                                         Mac48Address source)
{
}
void
FullMacLowTransmissionListener::MissedBlockAck (void)
{
}
FullMacLowDcfListener::FullMacLowDcfListener ()
{
}
FullMacLowDcfListener::~FullMacLowDcfListener ()
{
}

FullMacLowBlockAckEventListener::FullMacLowBlockAckEventListener ()
{
}
FullMacLowBlockAckEventListener::~FullMacLowBlockAckEventListener ()
{
}

FullMacLowTransmissionParameters::FullMacLowTransmissionParameters ()
  : m_nextSize (0),
    m_waitAck (ACK_NONE),
    m_sendRts (false),
    m_overrideDurationId (Seconds (0))
{
}
void
FullMacLowTransmissionParameters::EnableNextData (uint32_t size)
{
  m_nextSize = size;
}
void
FullMacLowTransmissionParameters::DisableNextData (void)
{
  m_nextSize = 0;
}
void
FullMacLowTransmissionParameters::EnableOverrideDurationId (Time durationId)
{
  m_overrideDurationId = durationId;
}
void
FullMacLowTransmissionParameters::DisableOverrideDurationId (void)
{
  m_overrideDurationId = Seconds (0);
}
void
FullMacLowTransmissionParameters::EnableSuperFastAck (void)
{
  m_waitAck = ACK_SUPER_FAST;
}
void
FullMacLowTransmissionParameters::EnableBasicBlockAck (void)
{
  m_waitAck = BLOCK_ACK_BASIC;
}
void
FullMacLowTransmissionParameters::EnableCompressedBlockAck (void)
{
  m_waitAck = BLOCK_ACK_COMPRESSED;
}
void
FullMacLowTransmissionParameters::EnableMultiTidBlockAck (void)
{
  m_waitAck = BLOCK_ACK_MULTI_TID;
}
void
FullMacLowTransmissionParameters::EnableFastAck (void)
{
  m_waitAck = ACK_FAST;
}
void
FullMacLowTransmissionParameters::EnableAck (void)
{
  m_waitAck = ACK_NORMAL;
}
void
FullMacLowTransmissionParameters::DisableAck (void)
{
  m_waitAck = ACK_NONE;
}
void
FullMacLowTransmissionParameters::EnableRts (void)
{
  m_sendRts = true;
}
void
FullMacLowTransmissionParameters::DisableRts (void)
{
  m_sendRts = false;
}
bool
FullMacLowTransmissionParameters::MustWaitAck (void) const
{
  return (m_waitAck != ACK_NONE);
}
bool
FullMacLowTransmissionParameters::MustWaitNormalAck (void) const
{
  return (m_waitAck == ACK_NORMAL);
}
bool
FullMacLowTransmissionParameters::MustWaitFastAck (void) const
{
  return (m_waitAck == ACK_FAST);
}
bool
FullMacLowTransmissionParameters::MustWaitSuperFastAck (void) const
{
  return (m_waitAck == ACK_SUPER_FAST);
}
bool
FullMacLowTransmissionParameters::MustWaitBasicBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_BASIC) ? true : false;
}
bool
FullMacLowTransmissionParameters::MustWaitCompressedBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_COMPRESSED) ? true : false;
}
bool
FullMacLowTransmissionParameters::MustWaitMultiTidBlockAck (void) const
{
  return (m_waitAck == BLOCK_ACK_MULTI_TID) ? true : false;
}
bool
FullMacLowTransmissionParameters::MustSendRts (void) const
{
  return m_sendRts;
}
bool
FullMacLowTransmissionParameters::HasDurationId (void) const
{
  return (m_overrideDurationId != Seconds (0));
}
Time
FullMacLowTransmissionParameters::GetDurationId (void) const
{
  NS_ASSERT (m_overrideDurationId != Seconds (0));
  return m_overrideDurationId;
}
bool
FullMacLowTransmissionParameters::HasNextPacket (void) const
{
  return (m_nextSize != 0);
}
uint32_t
FullMacLowTransmissionParameters::GetNextPacketSize (void) const
{
  NS_ASSERT (HasNextPacket ());
  return m_nextSize;
}

std::ostream &operator << (std::ostream &os, const FullMacLowTransmissionParameters &params)
{
  os << "["
  << "send rts=" << params.m_sendRts << ", "
  << "next size=" << params.m_nextSize << ", "
  << "dur=" << params.m_overrideDurationId << ", "
  << "ack=";
  switch (params.m_waitAck)
    {
    case FullMacLowTransmissionParameters::ACK_NONE:
      os << "none";
      break;
    case FullMacLowTransmissionParameters::ACK_NORMAL:
      os << "normal";
      break;
    case FullMacLowTransmissionParameters::ACK_FAST:
      os << "fast";
      break;
    case FullMacLowTransmissionParameters::ACK_SUPER_FAST:
      os << "super-fast";
      break;
    case FullMacLowTransmissionParameters::BLOCK_ACK_BASIC:
      os << "basic-block-ack";
      break;
    case FullMacLowTransmissionParameters::BLOCK_ACK_COMPRESSED:
      os << "compressed-block-ack";
      break;
    case FullMacLowTransmissionParameters::BLOCK_ACK_MULTI_TID:
      os << "multi-tid-block-ack";
      break;
    }
  os << "]";
  return os;
}


/***************************************************************
 *         Listener for PHY events. Forwards to MacLow
 ***************************************************************/


class PhyMacLowListener : public ns3::FullWifiPhyListener
{
public:
  PhyMacLowListener (ns3::FullMacLow *macLow)
    : m_macLow (macLow)
  {
  }
  virtual ~PhyMacLowListener ()
  {
  }
  virtual void NotifyRxStart (Time duration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble)
  {
  }
  virtual void NotifyRxEndOk (void)
  {
  }
  virtual void NotifyRxEndError (void)
  {
  }
  virtual void NotifyTxStart (Time duration)
  {
  }
  virtual void NotifyMaybeCcaBusyStart (Time duration)
  {
  }
  virtual void NotifySwitchingStart (Time duration)
  {
    m_macLow->NotifySwitchingStartNow (duration);
  }
private:
  ns3::FullMacLow *m_macLow;
};


FullMacLow::FullMacLow ()
  : m_normalAckTimeoutEvent (),
    m_fastAckTimeoutEvent (),
    m_superFastAckTimeoutEvent (),
    m_fastAckFailedTimeoutEvent (),
    m_blockAckTimeoutEvent (),
    m_ctsTimeoutEvent (),
    m_sendCtsEvent (),
    m_sendAckEvent (),
    m_sendDataEvent (),
    m_waitSifsEvent (),
    m_endTxNoAckEvent (),
    m_currentPacket (0),
    m_listener (0)
{
  NS_LOG_FUNCTION (this);
  m_lastNavDuration = Seconds (0);
  m_lastNavStart = Seconds (0);
  m_promisc = false;
}

FullMacLow::~FullMacLow ()
{
  NS_LOG_FUNCTION (this);
}

void
FullMacLow::SetupPhyMacLowListener (Ptr<FullWifiPhy> phy)
{
  m_phyMacLowListener = new PhyMacLowListener (this);
  phy->RegisterListener (m_phyMacLowListener);
}


void
FullMacLow::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_normalAckTimeoutEvent.Cancel ();
  m_fastAckTimeoutEvent.Cancel ();
  m_superFastAckTimeoutEvent.Cancel ();
  m_fastAckFailedTimeoutEvent.Cancel ();
  m_blockAckTimeoutEvent.Cancel ();
  m_ctsTimeoutEvent.Cancel ();
  m_sendCtsEvent.Cancel ();
  m_sendAckEvent.Cancel ();
  m_sendDataEvent.Cancel ();
  m_waitSifsEvent.Cancel ();
  m_endTxNoAckEvent.Cancel ();
  m_phy = 0;
  m_stationManager = 0;
  delete m_phyMacLowListener;
  m_phyMacLowListener = 0;
}

void
FullMacLow::CancelAllEvents (void)
{
  NS_LOG_FUNCTION (this);
  bool oneRunning = false;
  if (m_normalAckTimeoutEvent.IsRunning ())
    {
      m_normalAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_fastAckTimeoutEvent.IsRunning ())
    {
      m_fastAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_superFastAckTimeoutEvent.IsRunning ())
    {
      m_superFastAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_fastAckFailedTimeoutEvent.IsRunning ())
    {
      m_fastAckFailedTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_blockAckTimeoutEvent.IsRunning ())
    {
      m_blockAckTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_ctsTimeoutEvent.IsRunning ())
    {
      m_ctsTimeoutEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendCtsEvent.IsRunning ())
    {
      m_sendCtsEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendAckEvent.IsRunning ())
    {
      m_sendAckEvent.Cancel ();
      oneRunning = true;
    }
  if (m_sendDataEvent.IsRunning ())
    {
      m_sendDataEvent.Cancel ();
      oneRunning = true;
    }
  if (m_waitSifsEvent.IsRunning ())
    {
      m_waitSifsEvent.Cancel ();
      oneRunning = true;
    }
  if (m_endTxNoAckEvent.IsRunning ()) 
    {
      m_endTxNoAckEvent.Cancel ();
      oneRunning = true;
    }
  if (oneRunning && m_listener != 0)
    {
      m_listener->Cancel ();
      m_listener = 0;
    }
}

void
FullMacLow::SetPhy (Ptr<FullWifiPhy> phy)
{
  m_phy = phy;
  m_phy->SetReceiveOkCallback (MakeCallback (&FullMacLow::ReceiveOk, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&FullMacLow::ReceiveError, this));
  SetupPhyMacLowListener (phy);
}
void
FullMacLow::SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> manager)
{
  m_stationManager = manager;
}

void
FullMacLow::SetAddress (Mac48Address ad)
{
  m_self = ad;
}
void
FullMacLow::SetAckTimeout (Time ackTimeout)
{
  m_ackTimeout = ackTimeout;
}
void
FullMacLow::SetBasicBlockAckTimeout (Time blockAckTimeout)
{
  m_basicBlockAckTimeout = blockAckTimeout;
}
void
FullMacLow::SetCompressedBlockAckTimeout (Time blockAckTimeout)
{
  m_compressedBlockAckTimeout = blockAckTimeout;
}
void
FullMacLow::SetCtsTimeout (Time ctsTimeout)
{
  m_ctsTimeout = ctsTimeout;
}
void
FullMacLow::SetSifs (Time sifs)
{
  m_sifs = sifs;
}
void
FullMacLow::SetSlotTime (Time slotTime)
{
  m_slotTime = slotTime;
}
void
FullMacLow::SetPifs (Time pifs)
{
  m_pifs = pifs;
}
void
FullMacLow::SetBssid (Mac48Address bssid)
{
  m_bssid = bssid;
}
void
FullMacLow::SetPromisc (void)
{
  m_promisc = true;
}

Ptr<FullWifiPhy>
FullMacLow::GetPhy(void) const
{
  return m_phy;
}

Mac48Address
FullMacLow::GetAddress (void) const
{
  return m_self;
}
Time
FullMacLow::GetAckTimeout (void) const
{
  return m_ackTimeout;
}
Time
FullMacLow::GetBasicBlockAckTimeout () const
{
  return m_basicBlockAckTimeout;
}
Time
FullMacLow::GetCompressedBlockAckTimeout () const
{
  return m_compressedBlockAckTimeout;
}
Time
FullMacLow::GetCtsTimeout (void) const
{
  return m_ctsTimeout;
}
Time
FullMacLow::GetSifs (void) const
{
  return m_sifs;
}
Time
FullMacLow::GetSlotTime (void) const
{
  return m_slotTime;
}
Time
FullMacLow::GetPifs (void) const
{
  return m_pifs;
}
Mac48Address
FullMacLow::GetBssid (void) const
{
  return m_bssid;
}

bool
FullMacLow::IsWaitingAckTimeout (void) const
{
	if (m_normalAckTimeoutEvent.IsRunning ()
	               || m_fastAckTimeoutEvent.IsRunning ()
	               || m_superFastAckTimeoutEvent.IsRunning ())
	{
		return true;
	}
	else
	{
		return false;
	}
}


void
FullMacLow::UpdateDuplexEnd (Time end)
{
  if (end > m_duplexEnd)
    {
      Time txDuration = end - Simulator::Now ();
      m_duplexEnd = end;

      //reset the ack timeout events if there is any
      if (m_normalAckTimeoutEvent.IsRunning ())
        {
          Time timerDelay = txDuration + GetAckTimeout ();
          m_normalAckTimeoutEvent.Cancel ();
          m_normalAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::NormalAckTimeout, this);
        }
      if (m_fastAckTimeoutEvent.IsRunning ())
        {
          Time timerDelay = txDuration + GetPifs ();
          m_fastAckTimeoutEvent.Cancel ();
          m_fastAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::FastAckTimeout, this);
        }
      if (m_superFastAckTimeoutEvent.IsRunning ())
        {
          Time timerDelay = txDuration + GetPifs ();
          m_superFastAckTimeoutEvent.Cancel ();
          m_superFastAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::SuperFastAckTimeout, this);
        }
    }
  NS_LOG_FUNCTION("Set duplexEnd to " << m_duplexEnd.GetSeconds ());

}
Time
FullMacLow::GetDuplexEnd (void) const
{
  return m_duplexEnd;
}

void
FullMacLow::SetRxCallback (Callback<void,Ptr<Packet>,const FullWifiMacHeader *> callback)
{
  m_rxCallback = callback;
}
void
FullMacLow::RegisterDcfListener (FullMacLowDcfListener *listener)
{
  m_dcfListeners.push_back (listener);
}


void
FullMacLow::StartTransmission (Ptr<const Packet> packet,
                           const FullWifiMacHeader* hdr,
                           FullMacLowTransmissionParameters params,
                           FullMacLowTransmissionListener *listener)
{
  NS_LOG_FUNCTION (this << packet << hdr << params << listener);
  /* m_currentPacket is not NULL because someone started
   * a transmission and was interrupted before one of:
   *   - ctsTimeout
   *   - sendDataAfterCTS
   * expired. This means that one of these timers is still
   * running. They are all cancelled below anyway by the
   * call to CancelAllEvents (because of at least one
   * of these two timer) which will trigger a call to the
   * previous listener's cancel method.
   *
   * This typically happens because the high-priority
   * QapScheduler has taken access to the channel from
   * one of the Edca of the QAP.
   */
  m_currentPacket = packet->Copy ();
  m_currentHdr = *hdr;
  CancelAllEvents ();
  m_listener = listener;
  m_txParams = params;

  //NS_ASSERT (m_phy->IsStateIdle ());

  NS_LOG_DEBUG ("startTx size=" << GetSize (m_currentPacket, &m_currentHdr) <<
                ", to=" << m_currentHdr.GetAddr1 () << ", listener=" << m_listener);

  if (m_txParams.MustSendRts ())
    {
      SendRtsForPacket ();
    }
  else
    {
      SendDataPacket ();
    }

  /* When this method completes, we have taken ownership of the medium. */
  NS_ASSERT (m_phy->IsStateTx ());
}

void
FullMacLow::ReceiveError (Ptr<const Packet> packet, double rxSnr)
{
  NS_LOG_FUNCTION (this << packet << rxSnr);
  NS_LOG_DEBUG ("rx failed ");
  if (m_txParams.MustWaitFastAck ())
    {
      NS_ASSERT (m_fastAckFailedTimeoutEvent.IsExpired ());
      m_fastAckFailedTimeoutEvent = Simulator::Schedule (GetSifs (),
                                                         &FullMacLow::FastAckFailedTimeout, this);
    }
  return;
}

void
FullMacLow::NotifySwitchingStartNow (Time duration)
{
  NS_LOG_DEBUG ("switching channel. Cancelling MAC pending events");
  m_stationManager->Reset ();
  CancelAllEvents ();
  if (m_navCounterResetCtsMissed.IsRunning ())
    {
      m_navCounterResetCtsMissed.Cancel ();
    }
  m_lastNavStart = Simulator::Now ();
  m_lastNavDuration = Seconds (0);
  m_currentPacket = 0;
  m_listener = 0;
}

void
FullMacLow::ReceiveOk (Ptr<Packet> packet, double rxSnr, FullWifiMode txMode, FullWifiPreamble preamble)
{
  NS_LOG_FUNCTION (this << packet << rxSnr << txMode << preamble);
  /* A packet is received from the PHY.
   * When we have handled this packet,
   * we handle any packet present in the
   * packet queue.
   */
  FullWifiMacHeader hdr;
  packet->RemoveHeader (hdr);

  // the send ack event should be delay by this much
  Time delay = m_duplexEnd > Simulator::Now () ? m_duplexEnd - Simulator::Now ():Seconds (0);

  bool isPrevNavZero = IsNavZero ();
  NS_LOG_DEBUG ("duration/id=" << hdr.GetDuration ());
  NotifyNav (hdr, txMode, preamble);
  if (hdr.IsRts ())
    {
      /* see section 9.2.5.7 802.11-1999
       * A STA that is addressed by an RTS frame shall transmit a CTS frame after a SIFS
       * period if the NAV at the STA receiving the RTS frame indicates that the medium is
       * idle. If the NAV at the STA receiving the RTS indicates the medium is not idle,
       * that STA shall not respond to the RTS frame.
       */
      if (isPrevNavZero
          && hdr.GetAddr1 () == m_self)
        {
          NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", schedule CTS");
          NS_ASSERT (m_sendCtsEvent.IsExpired ());
          m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
                                        rxSnr, txMode);
          m_sendCtsEvent = Simulator::Schedule (GetSifs (),
                                                &FullMacLow::SendCtsAfterRts, this,
                                                hdr.GetAddr2 (),
                                                hdr.GetDuration (),
                                                txMode,
                                                rxSnr);
        }
      else
        {
          NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", cannot schedule CTS");
        }
    }
  else if (hdr.IsCts ()
           && hdr.GetAddr1 () == m_self
           && m_ctsTimeoutEvent.IsRunning ()
           && m_currentPacket != 0)
    {
      NS_LOG_DEBUG ("receive cts from=" << m_currentHdr.GetAddr1 ());
      FullSnrTag tag;
      packet->RemovePacketTag (tag);
      m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    rxSnr, txMode);
      m_stationManager->ReportRtsOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                     rxSnr, txMode, tag.Get ());

      m_ctsTimeoutEvent.Cancel ();
      NotifyCtsTimeoutResetNow ();
      m_listener->GotCts (rxSnr, txMode);
      NS_ASSERT (m_sendDataEvent.IsExpired ());
      m_sendDataEvent = Simulator::Schedule (GetSifs (),
                                             &FullMacLow::SendDataAfterCts, this,
                                             hdr.GetAddr1 (),
                                             hdr.GetDuration (),
                                             txMode);
    }
  else if (hdr.IsAck ()
           && hdr.GetAddr1 () == m_self
           && (m_normalAckTimeoutEvent.IsRunning ()
               || m_fastAckTimeoutEvent.IsRunning ()
               || m_superFastAckTimeoutEvent.IsRunning ())
           && m_txParams.MustWaitAck ())
    {
      NS_LOG_DEBUG ("receive ack from=" << m_currentHdr.GetAddr1 ());
      FullSnrTag tag;
      packet->RemovePacketTag (tag);
      m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    rxSnr, txMode);
      m_stationManager->ReportDataOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                      rxSnr, txMode, tag.Get ());
      bool gotAck = false;
      if (m_txParams.MustWaitNormalAck ()
          && m_normalAckTimeoutEvent.IsRunning ())
        {
          m_normalAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
      if (m_txParams.MustWaitFastAck ()
          && m_fastAckTimeoutEvent.IsRunning ())
        {
          m_fastAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
      if (gotAck)
        {
          m_listener->GotAck (rxSnr, txMode);
        }
      if (m_txParams.HasNextPacket ())
        {
          m_waitSifsEvent = Simulator::Schedule (GetSifs (),
                                                 &FullMacLow::WaitSifsAfterEndTx, this);
        }
    }
  else if (hdr.IsBlockAck () && hdr.GetAddr1 () == m_self
           && (m_txParams.MustWaitBasicBlockAck () || m_txParams.MustWaitCompressedBlockAck ())
           && m_blockAckTimeoutEvent.IsRunning ())
    {
      NS_LOG_DEBUG ("got block ack from " << hdr.GetAddr2 ());
      FullCtrlBAckResponseHeader blockAck;
      packet->RemoveHeader (blockAck);
      m_blockAckTimeoutEvent.Cancel ();
      m_listener->GotBlockAck (&blockAck, hdr.GetAddr2 ());
    }
  else if (hdr.IsBlockAckReq () && hdr.GetAddr1 () == m_self)
    {
      FullCtrlBAckRequestHeader blockAckReq;
      packet->RemoveHeader (blockAckReq);
      if (!blockAckReq.IsMultiTid ())
        {
          uint8_t tid = blockAckReq.GetTidInfo ();
          AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), tid));
          if (it != m_bAckAgreements.end ())
            {
              //Update block ack cache
              BlockAckCachesI i = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), tid));
              NS_ASSERT (i != m_bAckCaches.end ());
              (*i).second.UpdateWithBlockAckReq (blockAckReq.GetStartingSequence ());

              NS_ASSERT (m_sendAckEvent.IsExpired ());
              /* See section 11.5.3 in IEEE802.11 for mean of this timer */
              ResetBlockAckInactivityTimerIfNeeded (it->second.first);
              if ((*it).second.first.IsImmediateBlockAck ())
                {
                  NS_LOG_DEBUG ("rx blockAckRequest/sendImmediateBlockAck from=" << hdr.GetAddr2 ());
                  m_sendAckEvent = Simulator::Schedule (delay + GetSifs (),
                                                        &FullMacLow::SendBlockAckAfterBlockAckRequest, this,
                                                        blockAckReq,
                                                        hdr.GetAddr2 (),
                                                        hdr.GetDuration (),
                                                        txMode);
                }
              else
                {
                  NS_FATAL_ERROR ("Delayed block ack not supported.");
                }
            }
          else
            {
              NS_LOG_DEBUG ("There's not a valid agreement for this block ack request.");
            }
        }
      else
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
    }
  else if (hdr.IsCtl ())
    {
      NS_LOG_DEBUG ("rx drop " << hdr.GetTypeString ());
    }
  else if (hdr.GetAddr1 () == m_self)
    {
      m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
                                    rxSnr, txMode);

      if (hdr.IsQosData () && StoreMpduIfNeeded (packet, hdr))
        {
          /* From section 9.10.4 in IEEE802.11:
             Upon the receipt of a QoS data frame from the originator for which
             the Block Ack agreement exists, the recipient shall buffer the MSDU
             regardless of the value of the Ack Policy subfield within the
             QoS Control field of the QoS data frame. */
          if (hdr.IsQosAck ())
            {
              AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
              RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequence (),
                                                            hdr.GetAddr2 (), hdr.GetQosTid ());
              RxCompleteBufferedPacketsUntilFirstLost (hdr.GetAddr2 (), hdr.GetQosTid ());
              NS_ASSERT (m_sendAckEvent.IsExpired ());
              m_sendAckEvent = Simulator::Schedule (delay + GetSifs (),
                                                    &FullMacLow::SendAckAfterData, this,
                                                    hdr.GetAddr2 (),
                                                    hdr.GetDuration (),
                                                    txMode,
                                                    rxSnr);
            }
          else if (hdr.IsQosBlockAck ())
            {
              AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
              /* See section 11.5.3 in IEEE802.11 for mean of this timer */
              ResetBlockAckInactivityTimerIfNeeded (it->second.first);
            }
          return;
        }
      else if (hdr.IsQosData () && hdr.IsQosBlockAck ())
        {
          /* This happens if a packet with ack policy Block Ack is received and a block ack
             agreement for that packet doesn't exist.

             From section 11.5.3 in IEEE802.11e:
             When a recipient does not have an active Block ack for a TID, but receives
             data MPDUs with the Ack Policy subfield set to Block Ack, it shall discard
             them and shall send a DELBA frame using the normal access
             mechanisms. */
          AcIndex ac = QosUtilsMapTidToAc (hdr.GetQosTid ());
          m_edcaListeners[ac]->BlockAckInactivityTimeout (hdr.GetAddr2 (), hdr.GetQosTid ());
          return;
        }
      else if (hdr.IsBusyTone ())
        { //do nothing when a busytone signal is received
          NS_LOG_DEBUG ("rx busy tone from=" << hdr.GetAddr2 () );
          return;
        }
      else if (hdr.IsQosData () && hdr.IsQosNoAck ())
        {
          NS_LOG_DEBUG ("rx unicast/noAck from=" << hdr.GetAddr2 ());
        }
      else if (hdr.IsData () || hdr.IsMgt ())
        {
          NS_LOG_DEBUG ("rx unicast/sendAck from=" << hdr.GetAddr2 ());
          NS_ASSERT (m_sendAckEvent.IsExpired ());
          m_sendAckEvent = Simulator::Schedule (delay + GetSifs (),
                                                &FullMacLow::SendAckAfterData, this,
                                                hdr.GetAddr2 (),
                                                hdr.GetDuration (),
                                                txMode,
                                                rxSnr);
        }
      goto rxPacket;
    }
  else if (hdr.GetAddr1 ().IsGroup ())
    {
      if (hdr.IsData () || hdr.IsMgt ())
        {
          NS_LOG_DEBUG ("rx group from=" << hdr.GetAddr2 ());
          goto rxPacket;
        }
      else
        {
          // DROP
        }
    }
  else if (m_promisc)
    {
      NS_ASSERT (hdr.GetAddr1 () != m_self);
      if (hdr.IsData ())
        {
          goto rxPacket;
        }
    }
  else
    {
      //NS_LOG_DEBUG_VERBOSE ("rx not-for-me from %d", GetSource (packet));
    }
  return;
rxPacket:
  FullWifiMacTrailer fcs;
  packet->RemoveTrailer (fcs);
  m_rxCallback (packet, &hdr);
  return;
}

uint32_t
FullMacLow::GetAckSize (void) const
{
  FullWifiMacHeader ack;
  ack.SetType (FULL_WIFI_MAC_CTL_ACK);
  return ack.GetSize () + 4;
}
uint32_t
FullMacLow::GetBlockAckSize (enum BlockAckType type) const
{
  FullWifiMacHeader hdr;
  hdr.SetType (FULL_WIFI_MAC_CTL_BACKRESP);
  FullCtrlBAckResponseHeader blockAck;
  if (type == BASIC_BLOCK_ACK)
    {
      blockAck.SetType (BASIC_BLOCK_ACK);
    }
  else if (type == COMPRESSED_BLOCK_ACK)
    {
      blockAck.SetType (COMPRESSED_BLOCK_ACK);
    }
  else if (type == MULTI_TID_BLOCK_ACK)
    {
      //Not implemented
      NS_ASSERT (false);
    }
  return hdr.GetSize () + blockAck.GetSerializedSize () + 4;
}
uint32_t
FullMacLow::GetRtsSize (void) const
{
  FullWifiMacHeader rts;
  rts.SetType (FULL_WIFI_MAC_CTL_RTS);
  return rts.GetSize () + 4;
}
Time
FullMacLow::GetAckDuration (Mac48Address to, FullWifiMode dataTxMode) const
{
  FullWifiMode ackMode = GetAckTxModeForData (to, dataTxMode);
  return m_phy->CalculateTxDuration (GetAckSize (), ackMode, FULL_WIFI_PREAMBLE_LONG);
}
Time
FullMacLow::GetBlockAckDuration (Mac48Address to, FullWifiMode blockAckReqTxMode, enum BlockAckType type) const
{
  /*
   * For immediate BlockAck we should transmit the frame with the same WifiMode
   * as the BlockAckReq.
   *
   * from section 9.6 in IEEE802.11e:
   * The BlockAck control frame shall be sent at the same rate and modulation class as
   * the BlockAckReq frame if it is sent in response to a BlockAckReq frame.
   */
  return m_phy->CalculateTxDuration (GetBlockAckSize (type), blockAckReqTxMode, FULL_WIFI_PREAMBLE_LONG);
}
Time
FullMacLow::GetCtsDuration (Mac48Address to, FullWifiMode rtsTxMode) const
{
  FullWifiMode ctsMode = GetCtsTxModeForRts (to, rtsTxMode);
  return m_phy->CalculateTxDuration (GetCtsSize (), ctsMode, FULL_WIFI_PREAMBLE_LONG);
}
uint32_t
FullMacLow::GetCtsSize (void) const
{
  FullWifiMacHeader cts;
  cts.SetType (FULL_WIFI_MAC_CTL_CTS);
  return cts.GetSize () + 4;
}
uint32_t
FullMacLow::GetSize (Ptr<const Packet> packet, const FullWifiMacHeader *hdr) const
{
  FullWifiMacTrailer fcs;
  return packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
}

FullWifiMode
FullMacLow::GetRtsTxMode (Ptr<const Packet> packet, const FullWifiMacHeader *hdr) const
{
  Mac48Address to = hdr->GetAddr1 ();
  return m_stationManager->GetRtsMode (to, hdr, packet);
}
FullWifiMode
FullMacLow::GetDataTxMode (Ptr<const Packet> packet, const FullWifiMacHeader *hdr) const
{
  Mac48Address to = hdr->GetAddr1 ();
  FullWifiMacTrailer fcs;
  uint32_t size =  packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
  return m_stationManager->GetDataMode (to, hdr, packet, size);
}

FullWifiMode
FullMacLow::GetCtsTxModeForRts (Mac48Address to, FullWifiMode rtsTxMode) const
{
  return m_stationManager->GetCtsMode (to, rtsTxMode);
}
FullWifiMode
FullMacLow::GetAckTxModeForData (Mac48Address to, FullWifiMode dataTxMode) const
{
  return m_stationManager->GetAckMode (to, dataTxMode);
}


Time
FullMacLow::CalculateOverallTxTime (Ptr<const Packet> packet,
                                const FullWifiMacHeader* hdr,
                                const FullMacLowTransmissionParameters& params) const
{
  Time txTime = Seconds (0);
  FullWifiMode rtsMode = GetRtsTxMode (packet, hdr);
  FullWifiMode dataMode = GetDataTxMode (packet, hdr);
  if (params.MustSendRts ())
    {
      txTime += m_phy->CalculateTxDuration (GetRtsSize (), rtsMode, FULL_WIFI_PREAMBLE_LONG);
      txTime += GetCtsDuration (hdr->GetAddr1 (), rtsMode);
      txTime += Time (GetSifs () * 2);
    }
  uint32_t dataSize = GetSize (packet, hdr);
  txTime += m_phy->CalculateTxDuration (dataSize, dataMode, FULL_WIFI_PREAMBLE_LONG);
  if (params.MustWaitAck ())
    {
      txTime += GetSifs ();
      txTime += GetAckDuration (hdr->GetAddr1 (), dataMode);
    }
  return txTime;
}

Time
FullMacLow::CalculateTransmissionTime (Ptr<const Packet> packet,
                                   const FullWifiMacHeader* hdr,
                                   const FullMacLowTransmissionParameters& params) const
{
  Time txTime = CalculateOverallTxTime (packet, hdr, params);
  if (params.HasNextPacket ())
    {
      FullWifiMode dataMode = GetDataTxMode (packet, hdr);
      txTime += GetSifs ();
      txTime += m_phy->CalculateTxDuration (params.GetNextPacketSize (), dataMode, FULL_WIFI_PREAMBLE_LONG);
    }
  return txTime;
}

void
FullMacLow::NotifyNav (const FullWifiMacHeader &hdr, FullWifiMode txMode, FullWifiPreamble preamble)
{
  NS_ASSERT (m_lastNavStart <= Simulator::Now ());
  Time duration = hdr.GetDuration ();

  if (hdr.IsCfpoll ()
      && hdr.GetAddr2 () == m_bssid)
    {
      // see section 9.3.2.2 802.11-1999
      DoNavResetNow (duration);
      return;
    }
  // XXX Note that we should also handle CF_END specially here
  // but we don't for now because we do not generate them.
  else if (hdr.GetAddr1 () != m_self)
    {
      // see section 9.2.5.4 802.11-1999
      bool navUpdated = DoNavStartNow (duration);
      if (hdr.IsRts () && navUpdated)
        {
          /**
           * A STA that used information from an RTS frame as the most recent basis to update its NAV setting
           * is permitted to reset its NAV if no PHY-RXSTART.indication is detected from the PHY during a
           * period with a duration of (2 * aSIFSTime) + (CTS_Time) + (2 * aSlotTime) starting at the
           * PHY-RXEND.indication corresponding to the detection of the RTS frame. The “CTS_Time” shall
           * be calculated using the length of the CTS frame and the data rate at which the RTS frame
           * used for the most recent NAV update was received.
           */
          FullWifiMacHeader cts;
          cts.SetType (FULL_WIFI_MAC_CTL_CTS);
          Time navCounterResetCtsMissedDelay =
            m_phy->CalculateTxDuration (cts.GetSerializedSize (), txMode, preamble) +
            Time (2 * GetSifs ()) + Time (2 * GetSlotTime ());
          m_navCounterResetCtsMissed = Simulator::Schedule (navCounterResetCtsMissedDelay,
                                                            &FullMacLow::NavCounterResetCtsMissed, this,
                                                            Simulator::Now ());
        }
    }
}

void
FullMacLow::NavCounterResetCtsMissed (Time rtsEndRxTime)
{
  if (m_phy->GetLastRxStartTime () > rtsEndRxTime)
    {
      DoNavResetNow (Seconds (0.0));
    }
}

void
FullMacLow::DoNavResetNow (Time duration)
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->NavReset (duration);
    }
  m_lastNavStart = Simulator::Now ();
  m_lastNavStart = duration;
}
bool
FullMacLow::DoNavStartNow (Time duration)
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->NavStart (duration);
    }
  Time newNavEnd = Simulator::Now () + duration;
  Time oldNavEnd = m_lastNavStart + m_lastNavDuration;
  if (newNavEnd > oldNavEnd)
    {
      m_lastNavStart = Simulator::Now ();
      m_lastNavDuration = duration;
      return true;
    }
  return false;
}
void
FullMacLow::NotifyAckTimeoutStartNow (Time duration)
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->AckTimeoutStart (duration);
    }
}
void
FullMacLow::NotifyAckTimeoutResetNow ()
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->AckTimeoutReset ();
    }
}
void
FullMacLow::NotifyCtsTimeoutStartNow (Time duration)
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->CtsTimeoutStart (duration);
    }
}
void
FullMacLow::NotifyCtsTimeoutResetNow ()
{
  for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
    {
      (*i)->CtsTimeoutReset ();
    }
}

void
FullMacLow::ForwardDown (Ptr<const Packet> packet, const FullWifiMacHeader* hdr,
                     FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this << packet << hdr << txMode);
  NS_LOG_DEBUG ("send " << hdr->GetTypeString () <<
                ", to=" << hdr->GetAddr1 () <<
                ", size=" << packet->GetSize () <<
                ", mode=" << txMode <<
                ", duration=" << hdr->GetDuration () <<
                ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);
  m_phy->SendPacket (packet, txMode, FULL_WIFI_PREAMBLE_LONG, 22);

  //FIXME: should only update on sending a data packet
  //move this to send data and dataaftercts
//  UpdateDuplexEnd (Simulator::Now () +
//      m_phy->CalculateTxDuration (packet->GetSize (), txMode, FULL_WIFI_PREAMBLE_LONG));
//  Time duration =
//      m_phy->CalculateTxDuration (packet->GetSize (), txMode, FULL_WIFI_PREAMBLE_LONG);
  //std::cout <<" packet duration : " << duration.GetMicroSeconds () << std::endl;
}

void
FullMacLow::CtsTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("cts timeout");
  // XXX: should check that there was no rx start before now.
  // we should restart a new cts timeout now until the expected
  // end of rx if there was a rx start before now.
  m_stationManager->ReportRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  m_currentPacket = 0;
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  listener->MissedCts ();
}
void
FullMacLow::NormalAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("normal ack timeout");
  // XXX: should check that there was no rx start before now.
  // we should restart a new ack timeout now until the expected
  // end of rx if there was a rx start before now.
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  listener->MissedAck ();
}
void
FullMacLow::FastAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  if (m_phy->IsRxStateIdle ())
    {
      NS_LOG_DEBUG ("fast Ack idle missed");
      listener->MissedAck ();
    }
  else
    {
      NS_LOG_DEBUG ("fast Ack ok");
    }
}
void
FullMacLow::BlockAckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("block ack timeout");

  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  listener->MissedBlockAck ();
}
void
FullMacLow::SuperFastAckTimeout ()
{
  NS_LOG_FUNCTION (this);
  m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  if (m_phy->IsRxStateIdle ())
    {
      NS_LOG_DEBUG ("super fast Ack failed");
      listener->MissedAck ();
    }
  else
    {
      NS_LOG_DEBUG ("super fast Ack ok");
      listener->GotAck (0.0, FullWifiMode ());
    }
}

void
FullMacLow::SendRtsForPacket (void)
{
  NS_LOG_FUNCTION (this);
  /* send an RTS for this packet. */
  FullWifiMacHeader rts;
  rts.SetType (FULL_WIFI_MAC_CTL_RTS);
  rts.SetDsNotFrom ();
  rts.SetDsNotTo ();
  rts.SetNoRetry ();
  rts.SetNoMoreFragments ();
  rts.SetAddr1 (m_currentHdr.GetAddr1 ());
  rts.SetAddr2 (m_self);
  FullWifiMode rtsTxMode = GetRtsTxMode (m_currentPacket, &m_currentHdr);
  Time duration = Seconds (0);
  if (m_txParams.HasDurationId ())
    {
      duration += m_txParams.GetDurationId ();
    }
  else
    {
      FullWifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
      duration += GetSifs ();
      duration += GetCtsDuration (m_currentHdr.GetAddr1 (), rtsTxMode);
      duration += GetSifs ();
      duration += m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr),
                                              dataTxMode, FULL_WIFI_PREAMBLE_LONG);
      duration += GetSifs ();
      duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
    }
  rts.SetDuration (duration);

  Time txDuration = m_phy->CalculateTxDuration (GetRtsSize (), rtsTxMode, FULL_WIFI_PREAMBLE_LONG);
  Time timerDelay = txDuration + GetCtsTimeout ();

  NS_ASSERT (m_ctsTimeoutEvent.IsExpired ());
  NotifyCtsTimeoutStartNow (timerDelay);
  m_ctsTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::CtsTimeout, this);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (rts);
  FullWifiMacTrailer fcs;
  packet->AddTrailer (fcs);

  ForwardDown (packet, &rts, rtsTxMode);
}

void
FullMacLow::StartDataTxTimers (void)
{
  FullWifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
  Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr), dataTxMode, FULL_WIFI_PREAMBLE_LONG);
  if (m_txParams.MustWaitNormalAck ())
    {
      Time timerDelay = txDuration + GetAckTimeout ();
      NS_ASSERT (m_normalAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_normalAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::NormalAckTimeout, this);
    }
  else if (m_txParams.MustWaitFastAck ())
    {
      Time timerDelay = txDuration + GetPifs ();
      NS_ASSERT (m_fastAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_fastAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::FastAckTimeout, this);
    }
  else if (m_txParams.MustWaitSuperFastAck ())
    {
      Time timerDelay = txDuration + GetPifs ();
      NS_ASSERT (m_superFastAckTimeoutEvent.IsExpired ());
      NotifyAckTimeoutStartNow (timerDelay);
      m_superFastAckTimeoutEvent = Simulator::Schedule (timerDelay,
                                                        &FullMacLow::SuperFastAckTimeout, this);
    }
  else if (m_txParams.MustWaitBasicBlockAck ())
    {
      Time timerDelay = txDuration + GetBasicBlockAckTimeout ();
      NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
      m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::BlockAckTimeout, this);
    }
  else if (m_txParams.MustWaitCompressedBlockAck ())
    {
      Time timerDelay = txDuration + GetCompressedBlockAckTimeout ();
      NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
      m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &FullMacLow::BlockAckTimeout, this);
    }
  else if (m_txParams.HasNextPacket ())
    {
      Time delay = txDuration + GetSifs ();
      NS_ASSERT (m_waitSifsEvent.IsExpired ());
      m_waitSifsEvent = Simulator::Schedule (delay, &FullMacLow::WaitSifsAfterEndTx, this);
    }
  else
    {
      // since we do not expect any timer to be triggered.
      //do nothing if busy tone packet is transmitted
      if (m_currentHdr.GetType () != FULL_WIFI_MAC_BUSY_TONE)
        {
          Simulator::Schedule(txDuration, &FullMacLow::EndTxNoAck, this);
        }
    }
}

void
FullMacLow::SendDataPacket (void)
{
  NS_LOG_FUNCTION (this);
  /* send this packet directly. No RTS is needed. */
  StartDataTxTimers ();

  FullWifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
  Time duration = Seconds (0.0);
  if (m_txParams.HasDurationId ())
    {
      duration += m_txParams.GetDurationId ();
    }
  else
    {
      if (m_txParams.MustWaitBasicBlockAck ())
        {
          duration += GetSifs ();
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), dataTxMode, BASIC_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitCompressedBlockAck ())
        {
          duration += GetSifs ();
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), dataTxMode, COMPRESSED_BLOCK_ACK);
        }
      else if (m_txParams.MustWaitAck ())
        {
          duration += GetSifs ();
          duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
        }
      if (m_txParams.HasNextPacket ())
        {
          duration += GetSifs ();
          duration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (),
                                                  dataTxMode, FULL_WIFI_PREAMBLE_LONG);
          if (m_txParams.MustWaitAck ())
            {
              duration += GetSifs ();
              duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
            }
        }
    }
  if (m_currentHdr.GetType () != FULL_WIFI_MAC_BUSY_TONE)
    {
      m_currentHdr.SetDuration (duration);
    }

  m_currentPacket->AddHeader (m_currentHdr);
  FullWifiMacTrailer fcs;
  m_currentPacket->AddTrailer (fcs);

  ForwardDown (m_currentPacket, &m_currentHdr, dataTxMode);

  UpdateDuplexEnd (Simulator::Now () +
      m_phy->CalculateTxDuration (m_currentPacket->GetSize (), dataTxMode, FULL_WIFI_PREAMBLE_LONG));
//  Time duration =
//      m_phy->CalculateTxDuration (m_currentPacket->GetSize (), dataTxMode, FULL_WIFI_PREAMBLE_LONG);


  m_currentPacket = 0;
}

bool
FullMacLow::IsNavZero (void) const
{
  if (m_lastNavStart + m_lastNavDuration < Simulator::Now ())
    {
      return true;
    }
  else
    {
      return false;
    }
}

void
FullMacLow::SendCtsAfterRts (Mac48Address source, Time duration, FullWifiMode rtsTxMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << source << duration << rtsTxMode << rtsSnr);
  /* send a CTS when you receive a RTS
   * right after SIFS.
   */
  FullWifiMode ctsTxMode = GetCtsTxModeForRts (source, rtsTxMode);
  FullWifiMacHeader cts;
  cts.SetType (FULL_WIFI_MAC_CTL_CTS);
  cts.SetDsNotFrom ();
  cts.SetDsNotTo ();
  cts.SetNoMoreFragments ();
  cts.SetNoRetry ();
  cts.SetAddr1 (source);
  duration -= GetCtsDuration (source, rtsTxMode);
  duration -= GetSifs ();
  NS_ASSERT (duration >= MicroSeconds (0));
  cts.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  FullWifiMacTrailer fcs;
  packet->AddTrailer (fcs);

  FullSnrTag tag;
  tag.Set (rtsSnr);
  packet->AddPacketTag (tag);

  ForwardDown (packet, &cts, ctsTxMode);
}

void
FullMacLow::SendDataAfterCts (Mac48Address source, Time duration, FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this);
  /* send the third step in a
   * RTS/CTS/DATA/ACK hanshake
   */
  NS_ASSERT (m_currentPacket != 0);
  StartDataTxTimers ();

  FullWifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
  Time newDuration = Seconds (0);
  newDuration += GetSifs ();
  newDuration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
  Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr),
                                                dataTxMode, FULL_WIFI_PREAMBLE_LONG);
  duration -= txDuration;
  duration -= GetSifs ();

  duration = std::max (duration, newDuration);
  NS_ASSERT (duration >= MicroSeconds (0));
  m_currentHdr.SetDuration (duration);

  m_currentPacket->AddHeader (m_currentHdr);
  FullWifiMacTrailer fcs;
  m_currentPacket->AddTrailer (fcs);

  UpdateDuplexEnd (Simulator::Now () +
      m_phy->CalculateTxDuration (m_currentPacket->GetSize (), dataTxMode, FULL_WIFI_PREAMBLE_LONG));
//  Time duration =
//      m_phy->CalculateTxDuration (m_currentPacket->GetSize (), dataTxMode, FULL_WIFI_PREAMBLE_LONG);

  ForwardDown (m_currentPacket, &m_currentHdr, dataTxMode);
  m_currentPacket = 0;
}

void
FullMacLow::WaitSifsAfterEndTx (void)
{
  m_listener->StartNext ();
}

void 
FullMacLow::EndTxNoAck (void)
{
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  listener->EndTxNoAck ();
}

void
FullMacLow::FastAckFailedTimeout (void)
{
  NS_LOG_FUNCTION (this);
  FullMacLowTransmissionListener *listener = m_listener;
  m_listener = 0;
  listener->MissedAck ();
  NS_LOG_DEBUG ("fast Ack busy but missed");
}

void
FullMacLow::SendAckAfterData (Mac48Address source, Time duration, FullWifiMode dataTxMode, double dataSnr)
{
  NS_LOG_FUNCTION (this);
  /* send an ACK when you receive
   * a packet after SIFS.
   */
  FullWifiMode ackTxMode = GetAckTxModeForData (source, dataTxMode);
  FullWifiMacHeader ack;
  ack.SetType (FULL_WIFI_MAC_CTL_ACK);
  ack.SetDsNotFrom ();
  ack.SetDsNotTo ();
  ack.SetNoRetry ();
  ack.SetNoMoreFragments ();
  ack.SetAddr1 (source);
  duration -= GetAckDuration (source, dataTxMode);
  duration -= GetSifs ();
  NS_ASSERT (duration >= MicroSeconds (0));
  ack.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (ack);
  FullWifiMacTrailer fcs;
  packet->AddTrailer (fcs);

  FullSnrTag tag;
  tag.Set (dataSnr);
  packet->AddPacketTag (tag);

  ForwardDown (packet, &ack, ackTxMode);
}

bool
FullMacLow::StoreMpduIfNeeded (Ptr<Packet> packet, FullWifiMacHeader hdr)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
  if (it != m_bAckAgreements.end ())
    {
      FullWifiMacTrailer fcs;
      packet->RemoveTrailer (fcs);
      BufferedPacket bufferedPacket (packet, hdr);

      uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
      uint16_t mappedSeqControl = QosUtilsMapSeqControlToUniqueInteger (hdr.GetSequenceControl (), endSequence);

      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end ()
           && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceControl (), endSequence) < mappedSeqControl; i++)
        {
          ;
        }
      (*it).second.second.insert (i, bufferedPacket);

      //Update block ack cache
      BlockAckCachesI j = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
      NS_ASSERT (j != m_bAckCaches.end ());
      (*j).second.UpdateWithMpdu (&hdr);

      return true;
    }
  return false;
}

void
FullMacLow::CreateBlockAckAgreement (const FullMgtAddBaResponseHeader *respHdr, Mac48Address originator,
                                 uint16_t startingSeq)
{
  uint8_t tid = respHdr->GetTid ();
  FullBlockAckAgreement agreement (originator, tid);
  if (respHdr->IsImmediateBlockAck ())
    {
      agreement.SetImmediateBlockAck ();
    }
  else
    {
      agreement.SetDelayedBlockAck ();
    }
  agreement.SetAmsduSupport (respHdr->IsAmsduSupported ());
  agreement.SetBufferSize (respHdr->GetBufferSize () + 1);
  agreement.SetTimeout (respHdr->GetTimeout ());
  agreement.SetStartingSequence (startingSeq);

  std::list<BufferedPacket> buffer (0);
  AgreementKey key (originator, respHdr->GetTid ());
  AgreementValue value (agreement, buffer);
  m_bAckAgreements.insert (std::make_pair (key, value));

  FullBlockAckCache cache;
  cache.Init (startingSeq, respHdr->GetBufferSize () + 1);
  m_bAckCaches.insert (std::make_pair (key, cache));

  if (respHdr->GetTimeout () != 0)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, respHdr->GetTid ()));
      Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());

      AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());

      it->second.first.m_inactivityEvent = Simulator::Schedule (timeout,
                                                                &FullMacLowBlockAckEventListener::BlockAckInactivityTimeout,
                                                                m_edcaListeners[ac],
                                                                originator, tid);
    }
}

void
FullMacLow::DestroyBlockAckAgreement (Mac48Address originator, uint8_t tid)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequence (), originator, tid);
      RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
      m_bAckAgreements.erase (it);

      BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
      NS_ASSERT (i != m_bAckCaches.end ());
      m_bAckCaches.erase (i);
    }
}

void
FullMacLow::RxCompleteBufferedPacketsWithSmallerSequence (uint16_t seq, Mac48Address originator, uint8_t tid)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
      uint16_t mappedStart = QosUtilsMapSeqControlToUniqueInteger (seq, endSequence);
      uint16_t guard = (*it).second.second.begin ()->second.GetSequenceControl () & 0xfff0;
      BufferedPacketI last = (*it).second.second.begin ();

      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end ()
           && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceNumber (), endSequence) < mappedStart;)
        {
          if (guard == (*i).second.GetSequenceControl ())
            {
              if (!(*i).second.IsMoreFragments ())
                {
                  while (last != i)
                    {
                      m_rxCallback ((*last).first, &(*last).second);
                      last++;
                    }
                  m_rxCallback ((*last).first, &(*last).second);
                  last++;
                  /* go to next packet */
                  while (i != (*it).second.second.end () && ((guard >> 4) & 0x0fff) == (*i).second.GetSequenceNumber ())
                    {
                      i++;
                    }
                  if (i != (*it).second.second.end ())
                    {
                      guard = (*i).second.GetSequenceControl () & 0xfff0;
                      last = i;
                    }
                }
              else
                {
                  guard++;
                }
            }
          else
            {
              /* go to next packet */
              while (i != (*it).second.second.end () && ((guard >> 4) & 0x0fff) == (*i).second.GetSequenceNumber ())
                {
                  i++;
                }
              if (i != (*it).second.second.end ())
                {
                  guard = (*i).second.GetSequenceControl () & 0xfff0;
                  last = i;
                }
            }
        }
      (*it).second.second.erase ((*it).second.second.begin (), i);
    }
}

void
FullMacLow::RxCompleteBufferedPacketsUntilFirstLost (Mac48Address originator, uint8_t tid)
{
  AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
  if (it != m_bAckAgreements.end ())
    {
      uint16_t startingSeqCtrl = ((*it).second.first.GetStartingSequence () << 4) & 0xfff0;
      uint16_t guard = startingSeqCtrl;

      BufferedPacketI lastComplete = (*it).second.second.begin ();
      BufferedPacketI i = (*it).second.second.begin ();
      for (; i != (*it).second.second.end () && guard == (*i).second.GetSequenceControl (); i++)
        {
          if (!(*i).second.IsMoreFragments ())
            {
              while (lastComplete != i)
                {
                  m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
                  lastComplete++;
                }
              m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
              lastComplete++;
            }
          guard = (*i).second.IsMoreFragments () ? (guard + 1) : ((guard + 16) & 0xfff0);
        }
      (*it).second.first.SetStartingSequence ((guard >> 4) & 0x0fff);
      /* All packets already forwarded to WifiMac must be removed from buffer:
      [begin (), lastComplete) */
      (*it).second.second.erase ((*it).second.second.begin (), lastComplete);
    }
}

void
FullMacLow::SendBlockAckResponse (const FullCtrlBAckResponseHeader* blockAck, Mac48Address originator, bool immediate,
                              Time duration, FullWifiMode blockAckReqTxMode)
{
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (*blockAck);

  FullWifiMacHeader hdr;
  hdr.SetType (FULL_WIFI_MAC_CTL_BACKRESP);
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  m_currentPacket = packet;
  m_currentHdr = hdr;
  if (immediate)
    {
      m_txParams.DisableAck ();
      duration -= GetSifs ();
      if (blockAck->IsBasic ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxMode, BASIC_BLOCK_ACK);
        }
      else if (blockAck->IsCompressed ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxMode, COMPRESSED_BLOCK_ACK);
        }
      else if (blockAck->IsMultiTid ())
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
    }
  else
    {
      m_txParams.EnableAck ();
      duration += GetSifs ();
      duration += GetAckDuration (originator, blockAckReqTxMode);
    }
  m_txParams.DisableNextData ();

  StartDataTxTimers ();

  NS_ASSERT (duration >= MicroSeconds (0));
  hdr.SetDuration (duration);
  //here should be present a control about immediate or delayed block ack
  //for now we assume immediate
  packet->AddHeader (hdr);
  FullWifiMacTrailer fcs;
  packet->AddTrailer (fcs);
  ForwardDown (packet, &hdr, blockAckReqTxMode);
  m_currentPacket = 0;
}

void
FullMacLow::SendBlockAckAfterBlockAckRequest (const FullCtrlBAckRequestHeader reqHdr, Mac48Address originator,
                                          Time duration, FullWifiMode blockAckReqTxMode)
{
  NS_LOG_FUNCTION (this);
  FullCtrlBAckResponseHeader blockAck;
  uint8_t tid;
  bool immediate = false;
  if (!reqHdr.IsMultiTid ())
    {
      tid = reqHdr.GetTidInfo ();
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
        {
          blockAck.SetStartingSequence (reqHdr.GetStartingSequence ());
          blockAck.SetTidInfo (tid);
          immediate = (*it).second.first.IsImmediateBlockAck ();
          if (reqHdr.IsBasic ())
            {
              blockAck.SetType (BASIC_BLOCK_ACK);
            }
          else if (reqHdr.IsCompressed ())
            {
              blockAck.SetType (COMPRESSED_BLOCK_ACK);
            }
          BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
          NS_ASSERT (i != m_bAckCaches.end ());
          (*i).second.FillBlockAckBitmap (&blockAck);

          /* All packets with smaller sequence than starting sequence control must be passed up to Wifimac
           * See 9.10.3 in IEEE8022.11e standard.
           */
          RxCompleteBufferedPacketsWithSmallerSequence (reqHdr.GetStartingSequence (), originator, tid);
          RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
        }
      else
        {
          NS_LOG_DEBUG ("there's not a valid block ack agreement with " << originator);
        }
    }
  else
    {
      NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
    }

  SendBlockAckResponse (&blockAck, originator, immediate, duration, blockAckReqTxMode);
}

void
FullMacLow::ResetBlockAckInactivityTimerIfNeeded (FullBlockAckAgreement &agreement)
{
  if (agreement.GetTimeout () != 0)
    {
      NS_ASSERT (agreement.m_inactivityEvent.IsRunning ());
      agreement.m_inactivityEvent.Cancel ();
      Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());

      AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());
      //std::map<AcIndex, MacLowTransmissionListener*>::iterator it = m_edcaListeners.find (ac);
      //NS_ASSERT (it != m_edcaListeners.end ());

      agreement.m_inactivityEvent = Simulator::Schedule (timeout,
                                                         &FullMacLowBlockAckEventListener::BlockAckInactivityTimeout,
                                                         m_edcaListeners[ac],
                                                         agreement.GetPeer (),
                                                         agreement.GetTid ());
    }
}

void
FullMacLow::RegisterBlockAckListenerForAc (enum AcIndex ac, FullMacLowBlockAckEventListener *listener)
{
  m_edcaListeners.insert (std::make_pair (ac, listener));
}

} // namespace ns3
