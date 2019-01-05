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
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/pointer.h"

#include "full-edca-txop-n.h"
#include "full-mac-low.h"
#include "full-dcf-manager.h"
#include "full-mac-tx-middle.h"
#include "full-wifi-mac-trailer.h"
#include "full-wifi-mac.h"
#include "full-random-stream.h"
#include "full-wifi-mac-queue.h"
#include "full-msdu-aggregator.h"
#include "full-mgt-headers.h"
#include "full-qos-blocked-destinations.h"

NS_LOG_COMPONENT_DEFINE ("FullEdcaTxopN");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

class FullEdcaTxopN::FullDcf : public FullDcfState
{
public:
  FullDcf (FullEdcaTxopN * txop)
    : m_txop (txop)
  {
  }
private:
  virtual void DoNotifyAccessGranted (void)
  {
    m_txop->NotifyAccessGranted ();
  }
  virtual void DoNotifyInternalCollision (void)
  {
    m_txop->NotifyInternalCollision ();
  }
  virtual void DoNotifyCollision (void)
  {
    m_txop->NotifyCollision ();
  }
  virtual void DoNotifyChannelSwitching (void)
  {
    m_txop->NotifyChannelSwitching ();
  }
  virtual void DoNotifyRxStartNow (Time duration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble)
  {
      //do nothing, this function is used for full duplex transmission
      //currently only nqos module is working
  }
  FullEdcaTxopN *m_txop;
};

class FullEdcaTxopN::FullTransmissionListener : public FullMacLowTransmissionListener
{
public:
  FullTransmissionListener (FullEdcaTxopN * txop)
    : FullMacLowTransmissionListener (),
      m_txop (txop) {
  }

  virtual ~FullTransmissionListener () {}

  virtual void GotCts (double snr, FullWifiMode txMode)
  {
    m_txop->GotCts (snr, txMode);
  }
  virtual void MissedCts (void)
  {
    m_txop->MissedCts ();
  }
  virtual void GotAck (double snr, FullWifiMode txMode)
  {
    m_txop->GotAck (snr, txMode);
  }
  virtual void MissedAck (void)
  {
    m_txop->MissedAck ();
  }
  virtual void GotBlockAck (const FullCtrlBAckResponseHeader *blockAck, Mac48Address source)
  {
    m_txop->GotBlockAck (blockAck, source);
  }
  virtual void MissedBlockAck (void)
  {
    m_txop->MissedBlockAck ();
  }
  virtual void StartNext (void)
  {
    m_txop->StartNext ();
  }
  virtual void Cancel (void)
  {
    m_txop->Cancel ();
  }
  virtual void EndTxNoAck (void)
  {
    m_txop->EndTxNoAck ();
  }

private:
  FullEdcaTxopN *m_txop;
};

class FullEdcaTxopN::FullBlockAckEventListener : public FullMacLowBlockAckEventListener
{
public:
  FullBlockAckEventListener (FullEdcaTxopN * txop)
    : FullMacLowBlockAckEventListener (),
      m_txop (txop) {
  }
  virtual ~FullBlockAckEventListener () {}

  virtual void BlockAckInactivityTimeout (Mac48Address address, uint8_t tid)
  {
    m_txop->SendDelbaFrame (address, tid, false);
  }

private:
  FullEdcaTxopN *m_txop;
};

NS_OBJECT_ENSURE_REGISTERED (FullEdcaTxopN);

TypeId
FullEdcaTxopN::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullEdcaTxopN")
    .SetParent (ns3::FullDcf::GetTypeId ())
    .AddConstructor<FullEdcaTxopN> ()
    .AddAttribute ("BlockAckThreshold", "If number of packets in this queue reaches this value,\
                                         block ack mechanism is used. If this value is 0, block ack is never used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FullEdcaTxopN::SetBlockAckThreshold,
                                         &FullEdcaTxopN::GetBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BlockAckInactivityTimeout", "Represents max time (blocks of 1024 micro seconds) allowed for block ack\
                                                 inactivity. If this value isn't equal to 0 a timer start after that a\
                                                 block ack setup is completed and will be reset every time that a block\
                                                 ack frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FullEdcaTxopN::SetBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Queue", "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&FullEdcaTxopN::GetQueue),
                   MakePointerChecker<FullWifiMacQueue> ())
  ;
  return tid;
}

FullEdcaTxopN::FullEdcaTxopN ()
  : m_manager (0),
    m_currentPacket (0),
    m_aggregator (0),
    m_blockAckType (COMPRESSED_BLOCK_ACK)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new FullEdcaTxopN::FullTransmissionListener (this);
  m_blockAckListener = new FullEdcaTxopN::FullBlockAckEventListener (this);
  m_dcf = new FullEdcaTxopN::FullDcf (this);
  m_queue = CreateObject<FullWifiMacQueue> ();
  m_rng = new FullRealRandomStream ();
  m_qosBlockedDestinations = new FullQosBlockedDestinations ();
  m_baManager = new FullBlockAckManager ();
  m_baManager->SetQueue (m_queue);
  m_baManager->SetBlockAckType (m_blockAckType);
  m_baManager->SetBlockDestinationCallback (MakeCallback (&FullQosBlockedDestinations::Block, m_qosBlockedDestinations));
  m_baManager->SetUnblockDestinationCallback (MakeCallback (&FullQosBlockedDestinations::Unblock, m_qosBlockedDestinations));
  m_baManager->SetMaxPacketDelay (m_queue->GetMaxDelay ());
}

FullEdcaTxopN::~FullEdcaTxopN ()
{
  NS_LOG_FUNCTION (this);
}

void
FullEdcaTxopN::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_dcf;
  delete m_rng;
  delete m_qosBlockedDestinations;
  delete m_baManager;
  delete m_blockAckListener;
  m_transmissionListener = 0;
  m_dcf = 0;
  m_rng = 0;
  m_qosBlockedDestinations = 0;
  m_baManager = 0;
  m_blockAckListener = 0;
  m_txMiddle = 0;
  m_aggregator = 0;
}

void
FullEdcaTxopN::SetManager (FullDcfManager *manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_manager = manager;
  m_manager->Add (m_dcf);
}

void
FullEdcaTxopN::SetTxOkCallback (TxOk callback)
{
  m_txOkCallback = callback;
}

void
FullEdcaTxopN::SetTxFailedCallback (TxFailed callback)
{
  m_txFailedCallback = callback;
}

void
FullEdcaTxopN::SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
}
void
FullEdcaTxopN::SetTypeOfStation (enum TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  m_typeOfStation = type;
}

enum TypeOfStation
FullEdcaTxopN::GetTypeOfStation (void) const
{
  return m_typeOfStation;
}

Ptr<FullWifiMacQueue >
FullEdcaTxopN::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
FullEdcaTxopN::SetMinCw (uint32_t minCw)
{
  NS_LOG_FUNCTION (this << minCw);
  m_dcf->SetCwMin (minCw);
}

void
FullEdcaTxopN::SetMaxCw (uint32_t maxCw)
{
  NS_LOG_FUNCTION (this << maxCw);
  m_dcf->SetCwMax (maxCw);
}

void
FullEdcaTxopN::SetAifsn (uint32_t aifsn)
{
  NS_LOG_FUNCTION (this << aifsn);
  m_dcf->SetAifsn (aifsn);
}

uint32_t
FullEdcaTxopN::GetMinCw (void) const
{
  return m_dcf->GetCwMin ();
}

uint32_t
FullEdcaTxopN::GetMaxCw (void) const
{
  return m_dcf->GetCwMax ();
}

uint32_t
FullEdcaTxopN::GetAifsn (void) const
{
  return m_dcf->GetAifsn ();
}

void
FullEdcaTxopN::SetTxMiddle (FullMacTxMiddle *txMiddle)
{
  m_txMiddle = txMiddle;
}

Ptr<FullMacLow>
FullEdcaTxopN::Low (void)
{
  return m_low;
}

void
FullEdcaTxopN::SetLow (Ptr<FullMacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

bool
FullEdcaTxopN::NeedsAccess (void) const
{
  return !m_queue->IsEmpty () || m_currentPacket != 0 || m_baManager->HasPackets ();
}

void
FullEdcaTxopN::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0)
    {
      if (m_queue->IsEmpty () && !m_baManager->HasPackets ())
        {
          NS_LOG_DEBUG ("queue is empty");
          return;
        }
      if (m_baManager->HasBar (m_currentBar))
        {
          SendBlockAckRequest (m_currentBar);
          return;
        }
      /* check if packets need retransmission are stored in BlockAckManager */
      m_currentPacket = m_baManager->GetNextPacket (m_currentHdr);
      if (m_currentPacket == 0)
        {
          if (m_queue->PeekFirstAvailable (&m_currentHdr, m_currentPacketTimestamp, m_qosBlockedDestinations) == 0)
            {
              NS_LOG_DEBUG ("no available packets in the queue");
              return;
            }
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && m_blockAckThreshold > 0
              && !m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
              && SetupBlockAckIfNeeded ())
            {
              return;
            }
          m_currentPacket = m_queue->DequeueFirstAvailable (&m_currentHdr, m_currentPacketTimestamp, m_qosBlockedDestinations);
          NS_ASSERT (m_currentPacket != 0);

          uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
          m_currentHdr.SetSequenceNumber (sequence);
          m_currentHdr.SetFragmentNumber (0);
          m_currentHdr.SetNoMoreFragments ();
          m_currentHdr.SetNoRetry ();
          m_fragmentNumber = 0;
          NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () <<
                        ", to=" << m_currentHdr.GetAddr1 () <<
                        ", seq=" << m_currentHdr.GetSequenceControl ());
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ())
            {
              VerifyBlockAck ();
            }
        }
    }
  FullMacLowTransmissionParameters params;
  params.DisableOverrideDurationId ();
  if (m_currentHdr.GetAddr1 ().IsGroup ())
    {
      params.DisableRts ();
      params.DisableAck ();
      params.DisableNextData ();
      m_low->StartTransmission (m_currentPacket,
                                &m_currentHdr,
                                params,
                                m_transmissionListener);

      NS_LOG_DEBUG ("tx broadcast");
    }
  else if (m_currentHdr.GetType () == FULL_WIFI_MAC_CTL_BACKREQ)
    {
      SendBlockAckRequest (m_currentBar);
    }
  else
    {
      if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
        {
          params.DisableAck ();
        }
      else
        {
          params.EnableAck ();
        }
      if (NeedFragmentation () && ((m_currentHdr.IsQosData ()
                                    && !m_currentHdr.IsQosAmsdu ())
                                   || m_currentHdr.IsData ())
          && (m_blockAckThreshold == 0
              || m_blockAckType == BASIC_BLOCK_ACK))
        {
          //With COMPRESSED_BLOCK_ACK fragmentation must be avoided.
          params.DisableRts ();
          FullWifiMacHeader hdr;
          Ptr<Packet> fragment = GetFragmentPacket (&hdr);
          if (IsLastFragment ())
            {
              NS_LOG_DEBUG ("fragmenting last fragment size=" << fragment->GetSize ());
              params.DisableNextData ();
            }
          else
            {
              NS_LOG_DEBUG ("fragmenting size=" << fragment->GetSize ());
              params.EnableNextData (GetNextFragmentSize ());
            }
          m_low->StartTransmission (fragment, &hdr, params,
                                    m_transmissionListener);
        }
      else
        {
          FullWifiMacHeader peekedHdr;
          if (m_currentHdr.IsQosData ()
              && m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                               FullWifiMacHeader::ADDR1, m_currentHdr.GetAddr1 ())
              && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && m_aggregator != 0 && !m_currentHdr.IsRetry ())
            {
              /* here is performed aggregation */
              Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
              m_aggregator->Aggregate (m_currentPacket, currentAggregatedPacket,
                                       MapSrcAddressForAggregation (peekedHdr),
                                       MapDestAddressForAggregation (peekedHdr));
              bool aggregated = false;
              bool isAmsdu = false;
              Ptr<const Packet> peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                                             FullWifiMacHeader::ADDR1,
                                                                             m_currentHdr.GetAddr1 ());
              while (peekedPacket != 0)
                {
                  aggregated = m_aggregator->Aggregate (peekedPacket, currentAggregatedPacket,
                                                        MapSrcAddressForAggregation (peekedHdr),
                                                        MapDestAddressForAggregation (peekedHdr));
                  if (aggregated)
                    {
                      isAmsdu = true;
                      m_queue->Remove (peekedPacket);
                    }
                  else
                    {
                      break;
                    }
                  peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                               FullWifiMacHeader::ADDR1, m_currentHdr.GetAddr1 ());
                }
              if (isAmsdu)
                {
                  m_currentHdr.SetQosAmsdu ();
                  m_currentHdr.SetAddr3 (m_low->GetBssid ());
                  m_currentPacket = currentAggregatedPacket;
                  currentAggregatedPacket = 0;
                  NS_LOG_DEBUG ("tx unicast A-MSDU");
                }
            }
          if (NeedRts ())
            {
              params.EnableRts ();
              NS_LOG_DEBUG ("tx unicast rts");
            }
          else
            {
              params.DisableRts ();
              NS_LOG_DEBUG ("tx unicast");
            }
          params.DisableNextData ();
          m_low->StartTransmission (m_currentPacket, &m_currentHdr,
                                    params, m_transmissionListener);
          CompleteTx ();
        }
    }
}

void FullEdcaTxopN::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}

void
FullEdcaTxopN::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
FullEdcaTxopN::GotCts (double snr, FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  NS_LOG_DEBUG ("got cts");
}

void
FullEdcaTxopN::MissedCts (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed cts");
  if (!NeedRtsRetransmission ())
    {
      NS_LOG_DEBUG ("Cts Fail");
      m_stationManager->ReportFinalRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      // to reset the dcf.
      m_currentPacket = 0;
      m_dcf->ResetCw ();
    }
  else
    {
      m_dcf->UpdateFailedCw ();
    }
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
FullEdcaTxopN::NotifyChannelSwitching (void)
{
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
FullEdcaTxopN::Queue (Ptr<const Packet> packet, const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  FullWifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr,
                                     packet, fullPacketSize);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

void
FullEdcaTxopN::GotAck (double snr, FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  if (!NeedFragmentation ()
      || IsLastFragment ()
      || m_currentHdr.IsQosAmsdu ())
    {
      NS_LOG_DEBUG ("got ack. tx done.");
      if (!m_txOkCallback.IsNull ())
        {
          m_txOkCallback (m_currentHdr);
        }

      if (m_currentHdr.IsAction ())
        {
          FullWifiActionHeader actionHdr;
          Ptr<Packet> p = m_currentPacket->Copy ();
          p->RemoveHeader (actionHdr);
          if (actionHdr.GetCategory () == FullWifiActionHeader::BLOCK_ACK
              && actionHdr.GetAction ().blockAck == FullWifiActionHeader::BLOCK_ACK_DELBA)
            {
              FullMgtDelBaHeader delBa;
              p->PeekHeader (delBa);
              if (delBa.IsByOriginator ())
                {
                  m_baManager->TearDownBlockAck (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
              else
                {
                  m_low->DestroyBlockAckAgreement (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
            }
        }
      m_currentPacket = 0;

      m_dcf->ResetCw ();
      m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
      RestartAccessIfNeeded ();
    }
  else
    {
      NS_LOG_DEBUG ("got ack. tx not done, size=" << m_currentPacket->GetSize ());
    }
}

void
FullEdcaTxopN::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission ())
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      // to reset the dcf.
      m_currentPacket = 0;
      m_dcf->ResetCw ();
    }
  else
    {
      NS_LOG_DEBUG ("Retransmit");
      m_currentHdr.SetRetry ();
      m_dcf->UpdateFailedCw ();
    }
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
FullEdcaTxopN::MissedBlockAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed block ack");
  //should i report this to station addressed by ADDR1?
  NS_LOG_DEBUG ("Retransmit block ack request");
  m_currentHdr.SetRetry ();
  m_dcf->UpdateFailedCw ();

  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

Ptr<FullMsduAggregator>
FullEdcaTxopN::GetMsduAggregator (void) const
{
  return m_aggregator;
}

void
FullEdcaTxopN::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty () || m_baManager->HasPackets ())
      && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
FullEdcaTxopN::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && (!m_queue->IsEmpty () || m_baManager->HasPackets ())
      && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

bool
FullEdcaTxopN::NeedRts (void)
{
  return m_stationManager->NeedRts (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    m_currentPacket);
}

bool
FullEdcaTxopN::NeedRtsRetransmission (void)
{
  return m_stationManager->NeedRtsRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                  m_currentPacket);
}

bool
FullEdcaTxopN::NeedDataRetransmission (void)
{
  return m_stationManager->NeedDataRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket);
}

void
FullEdcaTxopN::NextFragment (void)
{
  m_fragmentNumber++;
}

void
FullEdcaTxopN::StartNext (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("start next packet fragment");
  /* this callback is used only for fragments. */
  NextFragment ();
  FullWifiMacHeader hdr;
  Ptr<Packet> fragment = GetFragmentPacket (&hdr);
  FullMacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableOverrideDurationId ();
  if (IsLastFragment ())
    {
      params.DisableNextData ();
    }
  else
    {
      params.EnableNextData (GetNextFragmentSize ());
    }
  Low ()->StartTransmission (fragment, &hdr, params, m_transmissionListener);
}

void
FullEdcaTxopN::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
}

void
FullEdcaTxopN::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  StartAccessIfNeeded ();
}

bool
FullEdcaTxopN::NeedFragmentation (void) const
{
  return m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket);
}

uint32_t
FullEdcaTxopN::GetFragmentSize (void)
{
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber);
}

uint32_t
FullEdcaTxopN::GetNextFragmentSize (void)
{
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber + 1);
}

uint32_t
FullEdcaTxopN::GetFragmentOffset (void)
{
  return m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket, m_fragmentNumber);
}


bool
FullEdcaTxopN::IsLastFragment (void) const
{
  return m_stationManager->IsLastFragment (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                           m_currentPacket, m_fragmentNumber);
}

Ptr<Packet>
FullEdcaTxopN::GetFragmentPacket (FullWifiMacHeader *hdr)
{
  *hdr = m_currentHdr;
  hdr->SetFragmentNumber (m_fragmentNumber);
  uint32_t startOffset = GetFragmentOffset ();
  Ptr<Packet> fragment;
  if (IsLastFragment ())
    {
      hdr->SetNoMoreFragments ();
    }
  else
    {
      hdr->SetMoreFragments ();
    }
  fragment = m_currentPacket->CreateFragment (startOffset,
                                              GetFragmentSize ());
  return fragment;
}

void
FullEdcaTxopN::SetAccessCategory (enum AcIndex ac)
{
  m_ac = ac;
}

Mac48Address
FullEdcaTxopN::MapSrcAddressForAggregation (const FullWifiMacHeader &hdr)
{
  Mac48Address retval;
  if (m_typeOfStation == STA || m_typeOfStation == ADHOC_STA)
    {
      retval = hdr.GetAddr2 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

Mac48Address
FullEdcaTxopN::MapDestAddressForAggregation (const FullWifiMacHeader &hdr)
{
  Mac48Address retval;
  if (m_typeOfStation == AP || m_typeOfStation == ADHOC_STA)
    {
      retval = hdr.GetAddr1 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

void
FullEdcaTxopN::SetMsduAggregator (Ptr<FullMsduAggregator> aggr)
{
  m_aggregator = aggr;
}

void
FullEdcaTxopN::PushFront (Ptr<const Packet> packet, const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  FullWifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr,
                                     packet, fullPacketSize);
  m_queue->PushFront (packet, hdr);
  StartAccessIfNeeded ();
}

void
FullEdcaTxopN::GotAddBaResponse (const FullMgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("received ADDBA response from " << recipient);
  uint8_t tid = respHdr->GetTid ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::PENDING))
    {
      if (respHdr->GetStatusCode ().IsSuccess ())
        {
          NS_LOG_DEBUG ("block ack agreement established with " << recipient);
          m_baManager->UpdateAgreement (respHdr, recipient);
        }
      else
        {
          NS_LOG_DEBUG ("discard ADDBA response" << recipient);
          m_baManager->NotifyAgreementUnsuccessful (recipient, tid);
        }
    }
  RestartAccessIfNeeded ();
}

void
FullEdcaTxopN::GotDelBaFrame (const FullMgtDelBaHeader *delBaHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("received DELBA frame from=" << recipient);
  m_baManager->TearDownBlockAck (recipient, delBaHdr->GetTid ());
}

void
FullEdcaTxopN::GotBlockAck (const FullCtrlBAckResponseHeader *blockAck, Mac48Address recipient)
{
  NS_LOG_DEBUG ("got block ack from=" << recipient);
  m_baManager->NotifyGotBlockAck (blockAck, recipient);
  m_currentPacket = 0;
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
FullEdcaTxopN::VerifyBlockAck (void)
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();
  uint16_t sequence = m_currentHdr.GetSequenceNumber ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::INACTIVE))
    {
      m_baManager->SwitchToBlockAckIfNeeded (recipient, tid, sequence);
    }
  if (m_baManager->ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::ESTABLISHED))
    {
      m_currentHdr.SetQosAckPolicy (FullWifiMacHeader::BLOCK_ACK);
    }
}

void
FullEdcaTxopN::CompleteTx (void)
{
  if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
    {
      if (!m_currentHdr.IsRetry ())
        {
          m_baManager->StorePacket (m_currentPacket, m_currentHdr, m_currentPacketTimestamp);
        }
      m_baManager->NotifyMpduTransmission (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid (),
                                           m_txMiddle->GetNextSeqNumberByTidAndAddress (m_currentHdr.GetQosTid (),
                                                                                        m_currentHdr.GetAddr1 ()));
      //we are not waiting for an ack: transmission is completed
      m_currentPacket = 0;
      m_dcf->ResetCw ();
      m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
      StartAccessIfNeeded ();
    }
}

bool
FullEdcaTxopN::SetupBlockAckIfNeeded ()
{
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();

  uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, FullWifiMacHeader::ADDR1, recipient);

  if (packets >= m_blockAckThreshold)
    {
      /* Block ack setup */
      uint16_t startingSequence = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      SendAddBaRequest (recipient, tid, startingSequence, m_blockAckInactivityTimeout, true);
      return true;
    }
  return false;
}

void
FullEdcaTxopN::SendBlockAckRequest (const struct Bar &bar)
{
  NS_LOG_FUNCTION (this);
  FullWifiMacHeader hdr;
  hdr.SetType (FULL_WIFI_MAC_CTL_BACKREQ);
  hdr.SetAddr1 (bar.recipient);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetBssid ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  m_currentPacket = bar.bar;
  m_currentHdr = hdr;

  FullMacLowTransmissionParameters params;
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();
  if (bar.immediate)
    {
      if (m_blockAckType == BASIC_BLOCK_ACK)
        {
          params.EnableBasicBlockAck ();
        }
      else if (m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          params.EnableCompressedBlockAck ();
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported");
        }
    }
  else
    {
      //Delayed block ack
      params.EnableAck ();
    }
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
}

void
FullEdcaTxopN::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
  m_low->RegisterBlockAckListenerForAc (m_ac, m_blockAckListener);
  m_baManager->SetBlockAckInactivityCallback (MakeCallback (&FullEdcaTxopN::SendDelbaFrame, this));
}

void
FullEdcaTxopN::SetBlockAckThreshold (uint8_t threshold)
{
  m_blockAckThreshold = threshold;
  m_baManager->SetBlockAckThreshold (threshold);
}

void
FullEdcaTxopN::SetBlockAckInactivityTimeout (uint16_t timeout)
{
  m_blockAckInactivityTimeout = timeout;
}

uint8_t
FullEdcaTxopN::GetBlockAckThreshold (void) const
{
  return m_blockAckThreshold;
}

void
FullEdcaTxopN::SendAddBaRequest (Mac48Address dest, uint8_t tid, uint16_t startSeq,
                             uint16_t timeout, bool immediateBAck)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("sent ADDBA request to " << dest);
  FullWifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (dest);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  FullWifiActionHeader actionHdr;
  FullWifiActionHeader::ActionValue action;
  action.blockAck = FullWifiActionHeader::BLOCK_ACK_ADDBA_REQUEST;
  actionHdr.SetAction (FullWifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  /*Setting ADDBARequest header*/
  FullMgtAddBaRequestHeader reqHdr;
  reqHdr.SetAmsduSupport (true);
  if (immediateBAck)
    {
      reqHdr.SetImmediateBlockAck ();
    }
  else
    {
      reqHdr.SetDelayedBlockAck ();
    }
  reqHdr.SetTid (tid);
  /* For now we don't use buffer size field in the ADDBA request frame. The recipient
   * will choose how many packets it can receive under block ack.
   */
  reqHdr.SetBufferSize (0);
  reqHdr.SetTimeout (timeout);
  reqHdr.SetStartingSequence (startSeq);

  m_baManager->CreateAgreement (&reqHdr, dest);

  packet->AddHeader (reqHdr);
  packet->AddHeader (actionHdr);

  m_currentPacket = packet;
  m_currentHdr = hdr;

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  FullMacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();

  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params,
                            m_transmissionListener);
}

void
FullEdcaTxopN::SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator)
{
  FullWifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (addr);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  FullMgtDelBaHeader delbaHdr;
  delbaHdr.SetTid (tid);
  if (byOriginator)
    {
      delbaHdr.SetByOriginator ();
    }
  else
    {
      delbaHdr.SetByRecipient ();
    }

  FullWifiActionHeader actionHdr;
  FullWifiActionHeader::ActionValue action;
  action.blockAck = FullWifiActionHeader::BLOCK_ACK_DELBA;
  actionHdr.SetAction (FullWifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (delbaHdr);
  packet->AddHeader (actionHdr);

  PushFront (packet, hdr);
}

int64_t
FullEdcaTxopN::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rng->AssignStreams (stream);
  return 1;
}

void
FullEdcaTxopN::DoInitialize ()
{
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  ns3::FullDcf::DoInitialize ();
}
} // namespace ns3
