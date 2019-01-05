/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#include "full-regular-wifi-mac.h"

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

#include "full-mac-rx-middle.h"
#include "full-mac-tx-middle.h"
#include "full-mac-low.h"
#include "full-dcf.h"
#include "full-dcf-manager.h"
#include "full-wifi-phy.h"

#include "full-msdu-aggregator.h"

NS_LOG_COMPONENT_DEFINE ("FullRegularWifiMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullRegularWifiMac);

FullRegularWifiMac::FullRegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
  m_rxMiddle = new FullMacRxMiddle ();
  m_rxMiddle->SetForwardCallback (MakeCallback (&FullRegularWifiMac::Receive, this));

  m_txMiddle = new FullMacTxMiddle ();

  m_low = CreateObject<FullMacLow> ();
  m_low->SetRxCallback (MakeCallback (&FullMacRxMiddle::Receive, m_rxMiddle));

  m_dcfManager = new FullDcfManager ();
  m_dcfManager->SetupLowListener (m_low);

  m_dca = CreateObject<FullDcaTxop> ();
  m_dca->SetLow (m_low);
  m_dca->SetManager (m_dcfManager);
  m_dca->SetTxOkCallback (MakeCallback (&FullRegularWifiMac::TxOk, this));
  m_dca->SetTxFailedCallback (MakeCallback (&FullRegularWifiMac::TxFailed, this));
  m_dca->SetAckTimeoutCallback (MakeCallback (&FullRegularWifiMac::AckTimeout, this));
  m_dca->SetSendPacketCallback (MakeCallback (&FullRegularWifiMac::SendPacket, this));

  // Construct the EDCAFs. The ordering is important - highest
  // priority (see Table 9-1 in IEEE 802.11-2007) must be created
  // first.
  SetupEdcaQueue (AC_VO);
  SetupEdcaQueue (AC_VI);
  SetupEdcaQueue (AC_BE);
  SetupEdcaQueue (AC_BK);
}

FullRegularWifiMac::~FullRegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
FullRegularWifiMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  m_dca->Initialize ();

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Initialize ();
    }
}

void
FullRegularWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_rxMiddle;
  m_rxMiddle = NULL;

  delete m_txMiddle;
  m_txMiddle = NULL;

  delete m_dcfManager;
  m_dcfManager = NULL;

  m_low->Dispose ();
  m_low = NULL;

  m_phy = NULL;
  m_stationManager = NULL;

  m_dca->Dispose ();
  m_dca = NULL;

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second = NULL;
    }
}

void
FullRegularWifiMac::SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_stationManager = stationManager;
  m_low->SetWifiRemoteStationManager (stationManager);

  m_dca->SetWifiRemoteStationManager (stationManager);

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetWifiRemoteStationManager (stationManager);
    }
}

Ptr<FullWifiRemoteStationManager>
FullRegularWifiMac::GetWifiRemoteStationManager () const
{
  return m_stationManager;
}

void
FullRegularWifiMac::SetupEdcaQueue (enum AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);

  // Our caller shouldn't be attempting to setup a queue that is
  // already configured.
  NS_ASSERT (m_edca.find (ac) == m_edca.end ());

  Ptr<FullEdcaTxopN> edca = CreateObject<FullEdcaTxopN> ();
  edca->SetLow (m_low);
  edca->SetManager (m_dcfManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&FullRegularWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&FullRegularWifiMac::TxFailed, this));
  edca->SetAccessCategory (ac);
  edca->CompleteConfig ();
  m_edca.insert (std::make_pair (ac, edca));
}

void
FullRegularWifiMac::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetTypeOfStation (type);
    }
}

Ptr<FullDcaTxop>
FullRegularWifiMac::GetDcaTxop () const
{
  return m_dca;
}

Ptr<FullEdcaTxopN>
FullRegularWifiMac::GetVOQueue () const
{
  return m_edca.find (AC_VO)->second;
}

Ptr<FullEdcaTxopN>
FullRegularWifiMac::GetVIQueue () const
{
  return m_edca.find (AC_VI)->second;
}

Ptr<FullEdcaTxopN>
FullRegularWifiMac::GetBEQueue () const
{
  return m_edca.find (AC_BE)->second;
}

Ptr<FullEdcaTxopN>
FullRegularWifiMac::GetBKQueue () const
{
  return m_edca.find (AC_BK)->second;
}

void
FullRegularWifiMac::SetWifiPhy (Ptr<FullWifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
  m_dcfManager->SetupPhyListener (phy);
  m_low->SetPhy (phy);
}

Ptr<FullWifiPhy>
FullRegularWifiMac::GetWifiPhy () const
{
  return m_phy;
}

void
FullRegularWifiMac::SetForwardUpCallback (ForwardUpCallback upCallback)
{
  NS_LOG_FUNCTION (this);
  m_forwardUp = upCallback;
}

void
FullRegularWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = linkUp;
}

void
FullRegularWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  m_linkDown = linkDown;
}

void
FullRegularWifiMac::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_qosSupported = enable;
}

bool
FullRegularWifiMac::GetQosSupported () const
{
  return m_qosSupported;
}

void
FullRegularWifiMac::SetSlot (Time slotTime)
{
  NS_LOG_FUNCTION (this << slotTime);
  m_dcfManager->SetSlot (slotTime);
  m_low->SetSlotTime (slotTime);
}

Time
FullRegularWifiMac::GetSlot (void) const
{
  return m_low->GetSlotTime ();
}

void
FullRegularWifiMac::SetSifs (Time sifs)
{
  NS_LOG_FUNCTION (this << sifs);
  m_dcfManager->SetSifs (sifs);
  m_low->SetSifs (sifs);
}

Time
FullRegularWifiMac::GetSifs (void) const
{
  return m_low->GetSifs ();
}

void
FullRegularWifiMac::SetEifsNoDifs (Time eifsNoDifs)
{
  NS_LOG_FUNCTION (this << eifsNoDifs);
  m_dcfManager->SetEifsNoDifs (eifsNoDifs);
}

Time
FullRegularWifiMac::GetEifsNoDifs (void) const
{
  return m_dcfManager->GetEifsNoDifs ();
}

void
FullRegularWifiMac::SetPifs (Time pifs)
{
  NS_LOG_FUNCTION (this << pifs);
  m_low->SetPifs (pifs);
}

Time
FullRegularWifiMac::GetPifs (void) const
{
  return m_low->GetPifs ();
}

void
FullRegularWifiMac::SetAckTimeout (Time ackTimeout)
{
  NS_LOG_FUNCTION (this << ackTimeout);
  m_low->SetAckTimeout (ackTimeout);
}

Time
FullRegularWifiMac::GetAckTimeout (void) const
{
  return m_low->GetAckTimeout ();
}

void
FullRegularWifiMac::SetCtsTimeout (Time ctsTimeout)
{
  NS_LOG_FUNCTION (this << ctsTimeout);
  m_low->SetCtsTimeout (ctsTimeout);
}

Time
FullRegularWifiMac::GetCtsTimeout (void) const
{
  return m_low->GetCtsTimeout ();
}

void
FullRegularWifiMac::SetBasicBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetBasicBlockAckTimeout (blockAckTimeout);
}

Time
FullRegularWifiMac::GetBasicBlockAckTimeout (void) const
{
  return m_low->GetBasicBlockAckTimeout ();
}

void
FullRegularWifiMac::SetCompressedBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetCompressedBlockAckTimeout (blockAckTimeout);
}

Time
FullRegularWifiMac::GetCompressedBlockAckTimeout (void) const
{
  return m_low->GetCompressedBlockAckTimeout ();
}

void
FullRegularWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_low->SetAddress (address);
}

Mac48Address
FullRegularWifiMac::GetAddress (void) const
{
  return m_low->GetAddress ();
}

void
FullRegularWifiMac::SetSsid (FullSsid ssid)
{
  NS_LOG_FUNCTION (this << ssid);
  m_ssid = ssid;
}

FullSsid
FullRegularWifiMac::GetSsid (void) const
{
  return m_ssid;
}

void
FullRegularWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << bssid);
  m_low->SetBssid (bssid);
}

Mac48Address
FullRegularWifiMac::GetBssid (void) const
{
  return m_low->GetBssid ();
}

void
FullRegularWifiMac::SetPromisc (void)
{
  m_low->SetPromisc ();
}


void
FullRegularWifiMac::SetEnableBusyTone (bool busy)
{
  m_dca->SetEnableBusyTone (busy);
}
void
FullRegularWifiMac::SetEnableReturnPacket (bool re)
{
  m_dca->SetEnableReturnPacket (re);
}
void
FullRegularWifiMac::SetEnableForward (bool forward)
{
  m_dca->SetEnableForward (forward);
}
void
FullRegularWifiMac::SetForwardQueue (Ptr<ForwardQueue> forwardQueue)
{
  m_dca->SetForwardQueue (forwardQueue);
}
bool
FullRegularWifiMac::GetEnableBusyTone (void) const
{
  return m_dca->GetEnableBusyTone ();
}
bool
FullRegularWifiMac::GetEnableReturnPacket (void) const
{
  return m_dca->GetEnableReturnPacket ();
}
bool
FullRegularWifiMac::GetEnableForward (void) const
{
  return m_dca->GetEnableForward ();
}
Ptr<ForwardQueue>
FullRegularWifiMac::GetForwardQueue (void) const
{
  return m_dca->GetForwardQueue ();
}



void
FullRegularWifiMac::Enqueue (Ptr<const Packet> packet,
                         Mac48Address to, Mac48Address from)
{
  // We expect RegularWifiMac subclasses which do support forwarding (e.g.,
  // AP) to override this method. Therefore, we throw a fatal error if
  // someone tries to invoke this method on a class which has not done
  // this.
  NS_FATAL_ERROR ("This MAC entity (" << this << ", " << GetAddress ()
                                      << ") does not support Enqueue() with from address");
}

bool
FullRegularWifiMac::SupportsSendFrom (void) const
{
  return false;
}

void
FullRegularWifiMac::ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from);
  m_forwardUp (packet, from, to);
}

void
FullRegularWifiMac::Receive (Ptr<Packet> packet, const FullWifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);

  Mac48Address to = hdr->GetAddr1 ();
  Mac48Address from = hdr->GetAddr2 ();

  // We don't know how to deal with any frame that is not addressed to
  // us (and odds are there is nothing sensible we could do anyway),
  // so we ignore such frames.
  //
  // The derived class may also do some such filtering, but it doesn't
  // hurt to have it here too as a backstop.
  if (to != GetAddress ())
    {
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      // There is currently only any reason for Management Action
      // frames to be flying about if we are a QoS STA.
      NS_ASSERT (m_qosSupported);

      FullWifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case FullWifiActionHeader::BLOCK_ACK:

          switch (actionHdr.GetAction ().blockAck)
            {
            case FullWifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
              {
                FullMgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader (reqHdr);

                // We've received an ADDBA Request. Our policy here is
                // to automatically accept it, so we get the ADDBA
                // Response on it's way immediately.
                SendAddBaResponse (&reqHdr, from);
                // This frame is now completely dealt with, so we're done.
                return;
              }

            case FullWifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
              {
                FullMgtAddBaResponseHeader respHdr;
                packet->RemoveHeader (respHdr);

                // We've received an ADDBA Response. We assume that it
                // indicates success after an ADDBA Request we have
                // sent (we could, in principle, check this, but it
                // seems a waste given the level of the current model)
                // and act by locally establishing the agreement on
                // the appropriate queue.
                AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
                m_edca[ac]->GotAddBaResponse (&respHdr, from);
                // This frame is now completely dealt with, so we're done.
                return;
              }

            case FullWifiActionHeader::BLOCK_ACK_DELBA:
              {
                FullMgtDelBaHeader delBaHdr;
                packet->RemoveHeader (delBaHdr);

                if (delBaHdr.IsByOriginator ())
                  {
                    // This DELBA frame was sent by the originator, so
                    // this means that an ingoing established
                    // agreement exists in MacLow and we need to
                    // destroy it.
                    m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                  }
                else
                  {
                    // We must have been the originator. We need to
                    // tell the correct queue that the agreement has
                    // been torn down
                    AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                    m_edca[ac]->GotDelBaFrame (&delBaHdr, from);
                  }
                // This frame is now completely dealt with, so we're done.
                return;
              }

            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
            }

        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
        }
    }
  NS_FATAL_ERROR ("Don't know how to handle frame (type=" << hdr->GetType ());
}

void
FullRegularWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket,
                                            const FullWifiMacHeader *hdr)
{
  FullMsduAggregator::DeaggregatedMsdus packets =
    FullMsduAggregator::Deaggregate (aggregatedPacket);

  for (FullMsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                 (*i).second.GetDestinationAddr ());
    }
}

void
FullRegularWifiMac::SendAddBaResponse (const FullMgtAddBaRequestHeader *reqHdr,
                                   Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  FullWifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  FullMgtAddBaResponseHeader respHdr;
  FullStatusCode code;
  code.SetSuccess ();
  respHdr.SetStatusCode (code);
  //Here a control about queues type?
  respHdr.SetAmsduSupport (reqHdr->IsAmsduSupported ());

  if (reqHdr->IsImmediateBlockAck ())
    {
      respHdr.SetImmediateBlockAck ();
    }
  else
    {
      respHdr.SetDelayedBlockAck ();
    }
  respHdr.SetTid (reqHdr->GetTid ());
  // For now there's not no control about limit of reception. We
  // assume that receiver has no limit on reception. However we assume
  // that a receiver sets a bufferSize in order to satisfy next
  // equation: (bufferSize + 1) % 16 = 0 So if a recipient is able to
  // buffer a packet, it should be also able to buffer all possible
  // packet's fragments. See section 7.3.1.14 in IEEE802.11e for more
  // details.
  respHdr.SetBufferSize (1023);
  respHdr.SetTimeout (reqHdr->GetTimeout ());

  FullWifiActionHeader actionHdr;
  FullWifiActionHeader::ActionValue action;
  action.blockAck = FullWifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
  actionHdr.SetAction (FullWifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (respHdr);
  packet->AddHeader (actionHdr);

  // We need to notify our MacLow object as it will have to buffer all
  // correctly received packets for this Block Ack session
  m_low->CreateBlockAckAgreement (&respHdr, originator,
                                  reqHdr->GetStartingSequence ());

  // It is unclear which queue this frame should go into. For now we
  // bung it into the queue corresponding to the TID for which we are
  // establishing an agreement, and push it to the head.
  m_edca[QosUtilsMapTidToAc (reqHdr->GetTid ())]->PushFront (packet, hdr);
}

TypeId
FullRegularWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullRegularWifiMac")
    .SetParent<FullWifiMac> ()
    .AddAttribute ("QosSupported",
                   "This Boolean attribute is set to enable 802.11e/WMM-style QoS support at this STA",
                   BooleanValue (false),
                   MakeBooleanAccessor (&FullRegularWifiMac::SetQosSupported,
                                        &FullRegularWifiMac::GetQosSupported),
                   MakeBooleanChecker ())
   .AddAttribute ("EnableBusyTone",
                  "This Boolean attribute is set to enable sending busytone to protect transmission",
                  BooleanValue (false),
                  MakeBooleanAccessor (&FullRegularWifiMac::SetEnableBusyTone,
                                       &FullRegularWifiMac::GetEnableBusyTone),
                  MakeBooleanChecker ())
    .AddAttribute ("EnableReturnPacket",
                   "This Boolean attribute is set to enable sending return packet if needed",
                   BooleanValue (false),
                   MakeBooleanAccessor (&FullRegularWifiMac::SetEnableReturnPacket,
                                        &FullRegularWifiMac::GetEnableReturnPacket),
                   MakeBooleanChecker ())
   .AddAttribute ("EnableForward",
                  "This Boolean attribute is set to enable forwarding packet if possible",
                  BooleanValue (false),
                  MakeBooleanAccessor (&FullRegularWifiMac::SetEnableForward,
                                       &FullRegularWifiMac::GetEnableForward),
                  MakeBooleanChecker ())
   .AddAttribute ("ForwardQueue",
                  "This queue stores the dst of the return packet or the foward address",
                  PointerValue (),
                  MakePointerAccessor (&FullRegularWifiMac::SetForwardQueue,
                                       &FullRegularWifiMac::GetForwardQueue),
                  MakePointerChecker<ForwardQueue> ())
    .AddAttribute ("DcaTxop", "The DcaTxop object",
                   PointerValue (),
                   MakePointerAccessor (&FullRegularWifiMac::GetDcaTxop),
                   MakePointerChecker<FullDcaTxop> ())
    .AddAttribute ("VO_EdcaTxopN",
                   "Queue that manages packets belonging to AC_VO access class",
                   PointerValue (),
                   MakePointerAccessor (&FullRegularWifiMac::GetVOQueue),
                   MakePointerChecker<FullEdcaTxopN> ())
    .AddAttribute ("VI_EdcaTxopN",
                   "Queue that manages packets belonging to AC_VI access class",
                   PointerValue (),
                   MakePointerAccessor (&FullRegularWifiMac::GetVIQueue),
                   MakePointerChecker<FullEdcaTxopN> ())
    .AddAttribute ("BE_EdcaTxopN",
                   "Queue that manages packets belonging to AC_BE access class",
                   PointerValue (),
                   MakePointerAccessor (&FullRegularWifiMac::GetBEQueue),
                   MakePointerChecker<FullEdcaTxopN> ())
    .AddAttribute ("BK_EdcaTxopN",
                   "Queue that manages packets belonging to AC_BK access class",
                   PointerValue (),
                   MakePointerAccessor (&FullRegularWifiMac::GetBKQueue),
                   MakePointerChecker<FullEdcaTxopN> ())
    .AddTraceSource ( "TxOkHeader",
                      "The header of successfully transmitted packet",
                      MakeTraceSourceAccessor (&FullRegularWifiMac::m_txOkCallback))
    .AddTraceSource ("TxErrHeader",
                     "The header of unsuccessfully transmitted packet",
                     MakeTraceSourceAccessor (&FullRegularWifiMac::m_txErrCallback))
    .AddTraceSource ("AckTimeout",
                      "Trace for AckTimeout",
                     MakeTraceSourceAccessor (&FullRegularWifiMac::m_ackTimeoutCallback))
	 .AddTraceSource ("SendPacket",
					   "Trace for SendPacket",
					  MakeTraceSourceAccessor (&FullRegularWifiMac::m_sendPacketCallback))
  ;

  return tid;
}

void
FullRegularWifiMac::FinishConfigureStandard (enum FullWifiPhyStandard standard)
{
  uint32_t cwmin;
  uint32_t cwmax;

  switch (standard)
    {
    case FULL_WIFI_PHY_STANDARD_80211p_CCH:
    case FULL_WIFI_PHY_STANDARD_80211p_SCH:
      cwmin = 15;
      cwmax = 511;
      break;

    case FULL_WIFI_PHY_STANDARD_holland:
    case FULL_WIFI_PHY_STANDARD_80211a:
    case FULL_WIFI_PHY_STANDARD_80211g:
    case FULL_WIFI_PHY_STANDARD_80211_10MHZ:
    case FULL_WIFI_PHY_STANDARD_80211_5MHZ:
      cwmin = 15;
      cwmax = 1023;
      break;

    case FULL_WIFI_PHY_STANDARD_80211b:
      cwmin = 31;
      cwmax = 1023;
      break;

    default:
      NS_FATAL_ERROR ("Unsupported WifiPhyStandard in RegularWifiMac::FinishConfigureStandard ()");
    }

  // The special value of AC_BE_NQOS which exists in the Access
  // Category enumeration allows us to configure plain old DCF.
  ConfigureDcf (m_dca, cwmin, cwmax, AC_BE_NQOS);

  // Now we configure the EDCA functions
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      // Special configuration for 802.11p CCH
      if (standard == FULL_WIFI_PHY_STANDARD_80211p_CCH)
        {
          ConfigureCCHDcf (i->second, cwmin, cwmax, i->first);
        }
      else
        {
          ConfigureDcf (i->second, cwmin, cwmax, i->first);
        }
    }
}

void
FullRegularWifiMac::TxOk (const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txOkCallback (hdr);
}

void
FullRegularWifiMac::TxFailed (const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txErrCallback (hdr);
}

void
FullRegularWifiMac::AckTimeout (const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_ackTimeoutCallback (hdr);
}

void
FullRegularWifiMac::SendPacket (const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_sendPacketCallback (hdr);
}

} // namespace ns3
