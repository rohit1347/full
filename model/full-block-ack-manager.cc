/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/simulator.h"
#include "ns3/fatal-error.h"

#include "full-block-ack-manager.h"
#include "full-mgt-headers.h"
#include "full-ctrl-headers.h"
#include "full-wifi-mac-header.h"
#include "full-edca-txop-n.h"
#include "full-mac-low.h"
#include "full-wifi-mac-queue.h"
#include "full-mac-tx-middle.h"

NS_LOG_COMPONENT_DEFINE ("FullBlockAckManager");

namespace ns3 {

FullBlockAckManager::Item::Item ()
{
}

FullBlockAckManager::Item::Item (Ptr<const Packet> packet, const FullWifiMacHeader &hdr, Time tStamp)
  : packet (packet),
    hdr (hdr),
    timestamp (tStamp)
{
}

Bar::Bar ()
{
}

Bar::Bar (Ptr<const Packet> bar, Mac48Address recipient, uint8_t tid, bool immediate)
  : bar (bar),
    recipient (recipient),
    tid (tid),
    immediate (immediate)
{
}

FullBlockAckManager::FullBlockAckManager ()
{
}

FullBlockAckManager::~FullBlockAckManager ()
{
  m_queue = 0;
  m_agreements.clear ();
  m_retryPackets.clear ();
}

bool
FullBlockAckManager::ExistsAgreement (Mac48Address recipient, uint8_t tid) const
{
  return (m_agreements.find (std::make_pair (recipient, tid)) != m_agreements.end ());
}

bool
FullBlockAckManager::ExistsAgreementInState (Mac48Address recipient, uint8_t tid,
                                         enum FullOriginatorBlockAckAgreement::State state) const
{
  AgreementsCI it;
  it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      switch (state)
        {
        case FullOriginatorBlockAckAgreement::INACTIVE:
          return it->second.first.IsInactive ();
        case FullOriginatorBlockAckAgreement::ESTABLISHED:
          return it->second.first.IsEstablished ();
        case FullOriginatorBlockAckAgreement::PENDING:
          return it->second.first.IsPending ();
        case FullOriginatorBlockAckAgreement::UNSUCCESSFUL:
          return it->second.first.IsUnsuccessful ();
        default:
          NS_FATAL_ERROR ("Invalid state for block ack agreement");
        }
    }
  return false;
}

void
FullBlockAckManager::CreateAgreement (const FullMgtAddBaRequestHeader *reqHdr, Mac48Address recipient)
{
  std::pair<Mac48Address, uint8_t> key (recipient, reqHdr->GetTid ());
  FullOriginatorBlockAckAgreement agreement (recipient, reqHdr->GetTid ());
  agreement.SetStartingSequence (reqHdr->GetStartingSequence ());
  /* for now we assume that originator doesn't use this field. Use of this field
     is mandatory only for recipient */
  agreement.SetBufferSize (0);
  agreement.SetTimeout (reqHdr->GetTimeout ());
  agreement.SetAmsduSupport (reqHdr->IsAmsduSupported ());
  if (reqHdr->IsImmediateBlockAck ())
    {
      agreement.SetImmediateBlockAck ();
    }
  else
    {
      agreement.SetDelayedBlockAck ();
    }
  agreement.SetState (FullOriginatorBlockAckAgreement::PENDING);
  PacketQueue queue (0);
  std::pair<FullOriginatorBlockAckAgreement, PacketQueue> value (agreement, queue);
  m_agreements.insert (std::make_pair (key, value));
  m_blockPackets (recipient, reqHdr->GetTid ());
}

void
FullBlockAckManager::DestroyAgreement (Mac48Address recipient, uint8_t tid)
{
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      for (std::list<PacketQueueI>::iterator i = m_retryPackets.begin (); i != m_retryPackets.end ();)
        {
          if ((*i)->hdr.GetAddr1 () == recipient && (*i)->hdr.GetQosTid () == tid)
            {
              i = m_retryPackets.erase (i);
            }
          else
            {
              i++;
            }
        }
      m_agreements.erase (it);
      //remove scheduled bar
      for (std::list<Bar>::iterator i = m_bars.begin (); i != m_bars.end ();)
        {
          if (i->recipient == recipient && i->tid == tid)
            {
              i = m_bars.erase (i);
            }
          else
            {
              i++;
            }
        }
    }
}

void
FullBlockAckManager::UpdateAgreement (const FullMgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  uint8_t tid = respHdr->GetTid ();
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      FullOriginatorBlockAckAgreement& agreement = it->second.first;
      agreement.SetBufferSize (respHdr->GetBufferSize () + 1);
      agreement.SetTimeout (respHdr->GetTimeout ());
      agreement.SetAmsduSupport (respHdr->IsAmsduSupported ());
      if (respHdr->IsImmediateBlockAck ())
        {
          agreement.SetImmediateBlockAck ();
        }
      else
        {
          agreement.SetDelayedBlockAck ();
        }
      agreement.SetState (FullOriginatorBlockAckAgreement::ESTABLISHED);
      if (agreement.GetTimeout () != 0)
        {
          Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());
          agreement.m_inactivityEvent = Simulator::Schedule (timeout,
                                                             &FullBlockAckManager::InactivityTimeout,
                                                             this,
                                                             recipient, tid);
        }
    }
  m_unblockPackets (recipient, tid);
}

void
FullBlockAckManager::StorePacket (Ptr<const Packet> packet, const FullWifiMacHeader &hdr, Time tStamp)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (hdr.IsQosData ());

  uint8_t tid = hdr.GetQosTid ();
  Mac48Address recipient = hdr.GetAddr1 ();

  Item item (packet, hdr, tStamp);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  it->second.second.push_back (item);
}

Ptr<const Packet>
FullBlockAckManager::GetNextPacket (FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  Ptr<const Packet> packet = 0;
  if (m_retryPackets.size () > 0)
    {
      CleanupBuffers ();
      PacketQueueI queueIt = m_retryPackets.front ();
      m_retryPackets.pop_front ();
      packet = queueIt->packet;
      hdr = queueIt->hdr;
      hdr.SetRetry ();
      NS_LOG_INFO ("Retry packet seq=" << hdr.GetSequenceNumber ());
      uint8_t tid = hdr.GetQosTid ();
      Mac48Address recipient = hdr.GetAddr1 ();

      if (ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::ESTABLISHED)
          || SwitchToBlockAckIfNeeded (recipient, tid, hdr.GetSequenceNumber ()))
        {
          hdr.SetQosAckPolicy (FullWifiMacHeader::BLOCK_ACK);
        }
      else
        {
          /* From section 9.10.3 in IEEE802.11e standard:
           * In order to improve efficiency, originators using the Block Ack facility
           * may send MPDU frames with the Ack Policy subfield in QoS control frames
           * set to Normal Ack if only a few MPDUs are available for transmission.[...]
           * When there are sufficient number of MPDUs, the originator may switch back to
           * the use of Block Ack.
           */
          hdr.SetQosAckPolicy (FullWifiMacHeader::NORMAL_ACK);
          AgreementsI i = m_agreements.find (std::make_pair (recipient, tid));
          i->second.second.erase (queueIt);
        }
    }
  return packet;
}

bool
FullBlockAckManager::HasBar (struct Bar &bar)
{
  if (m_bars.size () > 0)
    {
      bar = m_bars.front ();
      m_bars.pop_front ();
      return true;
    }
  return false;
}

bool
FullBlockAckManager::HasPackets (void) const
{
  return (m_retryPackets.size () > 0 || m_bars.size () > 0);
}

uint32_t
FullBlockAckManager::GetNBufferedPackets (Mac48Address recipient, uint8_t tid) const
{
  uint32_t nPackets = 0;
  if (ExistsAgreement (recipient, tid))
    {
      AgreementsCI it = m_agreements.find (std::make_pair (recipient, tid));
      PacketQueueCI queueIt = (*it).second.second.begin ();
      uint16_t currentSeq = 0;
      while (queueIt != (*it).second.second.end ())
        {
          currentSeq = (*queueIt).hdr.GetSequenceNumber ();
          nPackets++;
          /* a fragmented packet must be counted as one packet */
          while (queueIt != (*it).second.second.end () && (*queueIt).hdr.GetSequenceNumber () == currentSeq)
            {
              queueIt++;
            }
        }
      return nPackets;
    }
  return 0;
}

uint32_t
FullBlockAckManager::GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
{
  uint32_t nPackets = 0;
  uint16_t currentSeq = 0;
  if (ExistsAgreement (recipient, tid))
    {
      std::list<PacketQueueI>::const_iterator it = m_retryPackets.begin ();
      while (it != m_retryPackets.end ())
        {
          if ((*it)->hdr.GetAddr1 () == recipient && (*it)->hdr.GetQosTid () == tid)
            {
              currentSeq = (*it)->hdr.GetSequenceNumber ();
              nPackets++;
              /* a fragmented packet must be counted as one packet */
              while (it != m_retryPackets.end () && (*it)->hdr.GetSequenceNumber () == currentSeq)
                {
                  it++;
                }
            }
        }
    }
  return nPackets;
}

void
FullBlockAckManager::SetBlockAckThreshold (uint8_t nPackets)
{
  m_blockAckThreshold = nPackets;
}

void
FullBlockAckManager::NotifyGotBlockAck (const FullCtrlBAckResponseHeader *blockAck, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this);
  uint16_t sequenceFirstLost = 0;
  if (!blockAck->IsMultiTid ())
    {
      uint8_t tid = blockAck->GetTidInfo ();
      if (ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::ESTABLISHED))
        {
          bool foundFirstLost = false;
          AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
          PacketQueueI queueEnd = it->second.second.end ();

          if (it->second.first.m_inactivityEvent.IsRunning ())
            {
              /* Upon reception of a block ack frame, the inactivity timer at the
                 originator must be reset.
                 For more details see section 11.5.3 in IEEE802.11e standard */
              it->second.first.m_inactivityEvent.Cancel ();
              Time timeout = MicroSeconds (1024 * it->second.first.GetTimeout ());
              it->second.first.m_inactivityEvent = Simulator::Schedule (timeout,
                                                                        &FullBlockAckManager::InactivityTimeout,
                                                                        this,
                                                                        recipient, tid);
            }
          if (blockAck->IsBasic ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd;)
                {
                  if (blockAck->IsFragmentReceived ((*queueIt).hdr.GetSequenceNumber (),
                                                    (*queueIt).hdr.GetFragmentNumber ()))
                    {
                      queueIt = it->second.second.erase (queueIt);
                    }
                  else
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          sequenceFirstLost = (*queueIt).hdr.GetSequenceNumber ();
                          (*it).second.first.SetStartingSequence (sequenceFirstLost);
                        }
                      m_retryPackets.push_back (queueIt);
                      queueIt++;
                    }
                }
            }
          else if (blockAck->IsCompressed ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd;)
                {
                  if (blockAck->IsPacketReceived ((*queueIt).hdr.GetSequenceNumber ()))
                    {
                      uint16_t currentSeq = (*queueIt).hdr.GetSequenceNumber ();
                      while (queueIt != queueEnd
                             && (*queueIt).hdr.GetSequenceNumber () == currentSeq)
                        {
                          queueIt = it->second.second.erase (queueIt);
                        }
                    }
                  else
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          sequenceFirstLost = (*queueIt).hdr.GetSequenceNumber ();
                          (*it).second.first.SetStartingSequence (sequenceFirstLost);
                        }
                      m_retryPackets.push_back (queueIt);
                      queueIt++;
                    }
                }
            }
          uint16_t newSeq = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
          if ((foundFirstLost && !SwitchToBlockAckIfNeeded (recipient, tid, sequenceFirstLost))
              || (!foundFirstLost && !SwitchToBlockAckIfNeeded (recipient, tid, newSeq)))
            {
              it->second.first.SetState (FullOriginatorBlockAckAgreement::INACTIVE);
            }
        }
    }
  else
    {
      //NOT SUPPORTED FOR NOW
      NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
    }
}

void
FullBlockAckManager::SetBlockAckType (enum BlockAckType bAckType)
{
  m_blockAckType = bAckType;
}

Ptr<Packet>
FullBlockAckManager::ScheduleBlockAckReqIfNeeded (Mac48Address recipient, uint8_t tid)
{
  /* This method checks if a BlockAckRequest frame should be send to the recipient station.
     Number of packets under block ack is specified in OriginatorBlockAckAgreement object but sometimes
     this number could be incorrect. In fact is possible that a block ack agreement exists for n
     packets but some of these packets are dropped due to MSDU lifetime expiration.
   */
  NS_LOG_FUNCTION (this);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  if ((*it).second.first.IsBlockAckRequestNeeded ()
      || (GetNRetryNeededPackets (recipient, tid) == 0
          && m_queue->GetNPacketsByTidAndAddress (tid, FullWifiMacHeader::ADDR1, recipient) == 0))
    {
      FullOriginatorBlockAckAgreement &agreement = (*it).second.first;
      agreement.CompleteExchange ();

      FullCtrlBAckRequestHeader reqHdr;
      if (m_blockAckType == BASIC_BLOCK_ACK || m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          reqHdr.SetType (m_blockAckType);
          reqHdr.SetTidInfo (agreement.GetTid ());
          reqHdr.SetStartingSequence (agreement.GetStartingSequence ());
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Invalid block ack type.");
        }
      Ptr<Packet> bar = Create<Packet> ();
      bar->AddHeader (reqHdr);
      return bar;
    }
  return 0;
}

void
FullBlockAckManager::InactivityTimeout (Mac48Address recipient, uint8_t tid)
{
  m_blockAckInactivityTimeout (recipient, tid, true);
}

void
FullBlockAckManager::NotifyAgreementEstablished (Mac48Address recipient, uint8_t tid, uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  it->second.first.SetState (FullOriginatorBlockAckAgreement::ESTABLISHED);
  it->second.first.SetStartingSequence (startingSeq);
}

void
FullBlockAckManager::NotifyAgreementUnsuccessful (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (it != m_agreements.end ())
    {
      it->second.first.SetState (FullOriginatorBlockAckAgreement::UNSUCCESSFUL);
    }
}

void
FullBlockAckManager::NotifyMpduTransmission (Mac48Address recipient, uint8_t tid, uint16_t nextSeqNumber)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> bar = 0;
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  uint16_t nextSeq;
  if (GetNRetryNeededPackets (recipient, tid) > 0)
    {
      nextSeq = GetSeqNumOfNextRetryPacket (recipient, tid);
    }
  else
    {
      nextSeq = nextSeqNumber;
    }
  it->second.first.NotifyMpduTransmission (nextSeq);
  bar = ScheduleBlockAckReqIfNeeded (recipient, tid);
  if (bar != 0)
    {
      Bar request (bar, recipient, tid, it->second.first.IsImmediateBlockAck ());
      m_bars.push_back (request);
    }
}

void
FullBlockAckManager::SetQueue (Ptr<FullWifiMacQueue> queue)
{
  m_queue = queue;
}

bool
FullBlockAckManager::SwitchToBlockAckIfNeeded (Mac48Address recipient, uint8_t tid, uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::PENDING));
  if (!ExistsAgreementInState (recipient, tid, FullOriginatorBlockAckAgreement::UNSUCCESSFUL) && ExistsAgreement (recipient, tid))
    {
      uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, FullWifiMacHeader::ADDR1, recipient) +
        GetNBufferedPackets (recipient, tid);
      if (packets >= m_blockAckThreshold)
        {
          NotifyAgreementEstablished (recipient, tid, startingSeq);
          return true;
        }
    }
  return false;
}

void
FullBlockAckManager::TearDownBlockAck (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this);
  DestroyAgreement (recipient, tid);
}

bool
FullBlockAckManager::HasOtherFragments (uint16_t sequenceNumber) const
{
  bool retVal = false;
  if (m_retryPackets.size () > 0)
    {
      Item next = *(m_retryPackets.front ());
      if (next.hdr.GetSequenceNumber () == sequenceNumber)
        {
          retVal = true;
        }
    }
  return retVal;
}

uint32_t
FullBlockAckManager::GetNextPacketSize (void) const
{
  uint32_t size = 0;
  if (m_retryPackets.size () > 0)
    {
      Item next = *(m_retryPackets.front ());
      size = next.packet->GetSize ();
    }
  return size;
}

void
FullBlockAckManager::CleanupBuffers (void)
{
  for (AgreementsI j = m_agreements.begin (); j != m_agreements.end (); j++)
    {
      if (j->second.second.empty ())
        {
          continue;
        }
      Time now = Simulator::Now ();
      PacketQueueI end = j->second.second.begin ();
      for (PacketQueueI i = j->second.second.begin (); i != j->second.second.end (); i++)
        {
          if (i->timestamp + m_maxDelay > now)
            {
              end = i;
              break;
            }
          else
            {
              /* remove retry packet iterator if it's present in retry queue */
              for (std::list<PacketQueueI>::iterator it = m_retryPackets.begin (); it != m_retryPackets.end (); it++)
                {
                  if ((*it)->hdr.GetAddr1 () == j->second.first.GetPeer ()
                      && (*it)->hdr.GetQosTid () == j->second.first.GetTid ()
                      && (*it)->hdr.GetSequenceNumber () == i->hdr.GetSequenceNumber ())
                    {
                      m_retryPackets.erase (it);
                    }
                }
            }
        }
      j->second.second.erase (j->second.second.begin (), end);
      j->second.first.SetStartingSequence (end->hdr.GetSequenceNumber ());
    }
}

void
FullBlockAckManager::SetMaxPacketDelay (Time maxDelay)
{
  NS_LOG_FUNCTION (this);
  m_maxDelay = maxDelay;
}

void
FullBlockAckManager::SetBlockAckInactivityCallback (Callback<void, Mac48Address, uint8_t, bool> callback)
{
  m_blockAckInactivityTimeout = callback;
}

void
FullBlockAckManager::SetBlockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback)
{
  m_blockPackets = callback;
}

void
FullBlockAckManager::SetUnblockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback)
{
  m_unblockPackets = callback;
}

void
FullBlockAckManager::SetTxMiddle (FullMacTxMiddle* txMiddle)
{
  m_txMiddle = txMiddle;
}

uint16_t
FullBlockAckManager::GetSeqNumOfNextRetryPacket (Mac48Address recipient, uint8_t tid) const
{
  std::list<PacketQueueI>::const_iterator it = m_retryPackets.begin ();
  while (it != m_retryPackets.end ())
    {
      if ((*it)->hdr.GetAddr1 () == recipient && (*it)->hdr.GetQosTid () == tid)
        {
          return (*it)->hdr.GetSequenceNumber ();
        }
    }
  return 4096;
}

} // namespace ns3
