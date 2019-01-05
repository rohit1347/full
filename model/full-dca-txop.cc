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

#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "full-dca-txop.h"
#include "full-dcf-manager.h"
#include "full-mac-low.h"
#include "full-wifi-mac-queue.h"
#include "full-mac-tx-middle.h"
#include "full-wifi-mac-trailer.h"
#include "full-wifi-mac.h"
#include "full-random-stream.h"

NS_LOG_COMPONENT_DEFINE ("FullDcaTxop");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

class FullDcaTxop::FullDcf : public FullDcfState
{
public:
  FullDcf (FullDcaTxop * txop)
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
    m_txop->NotifyRxStartNow (duration, packet, txMode, preamble);
  }
  FullDcaTxop *m_txop;
};

class FullDcaTxop::FullTransmissionListener : public FullMacLowTransmissionListener
{
public:
  FullTransmissionListener (FullDcaTxop * txop)
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
  FullDcaTxop *m_txop;
};



void
ForwardMap::RemoveItem (Mac48Address add)
{
  std::list<ForwardItem>::iterator it;
  it = m_forwardQueue.begin();
  for ( it = m_forwardQueue.begin (); it != m_forwardQueue.end (); it++)
    {
      if (add == (*it).add)
        {
          m_forwardQueue.erase (it);
        }
    }
}

ForwardItem*
ForwardMap::GetItem (Mac48Address add)
{
  std::list<ForwardItem>::iterator it;
  for ( it = m_forwardQueue.begin (); it != m_forwardQueue.end (); it++)
    {
      if (add == (*it).add)
        {
          return &(*it);
        }
    }
  return 0;
}

void
ForwardMap::SortQueue ()
{
//  std::sort(m_forwardQueue.begin(), m_forwardQueue.end(), sort_pred());
  m_forwardQueue.sort(sort_pred());
}

void
ForwardMap::UpdateItem (Mac48Address add, double p)
{
  std::list<ForwardItem>::iterator it;
  for ( it = m_forwardQueue.begin (); it != m_forwardQueue.end (); it++)
    {
      if (add == (*it).add)
        {
          (*it).priority = p;
        }
    }
  SortQueue ();
}


ForwardMap*
ForwardQueue::GetForwardMap (Mac48Address add)
    {
        std::list<ForwardMap>::iterator it;
        for ( it = m_forwardQueue.begin (); it != m_forwardQueue.end (); it++)
          {
            if (add == (*it).GetTransmitterAddress ())
              {
                return &(*it);
              }
          }
        return 0;
    }

NS_OBJECT_ENSURE_REGISTERED (FullDcaTxop);

TypeId
FullDcaTxop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullDcaTxop")
    .SetParent (ns3::FullDcf::GetTypeId ())
    .AddConstructor<FullDcaTxop> ()
    .AddAttribute ("Queue", "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&FullDcaTxop::GetQueue),
                   MakePointerChecker<FullWifiMacQueue> ())
  ;
  return tid;
}

FullDcaTxop::FullDcaTxop ()
  : m_manager (0),
    m_currentPacket (0)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new FullDcaTxop::FullTransmissionListener (this);
  m_dcf = new FullDcaTxop::FullDcf (this);
  m_queue = CreateObject<FullWifiMacQueue> ();
  m_rng = new FullRealRandomStream ();
  m_txMiddle = new FullMacTxMiddle ();

  m_enableBusyTone = false;
  m_enableReturnPacket = false;
  m_enableForward = false;

  m_forwardQueue = new ForwardQueue();

}

FullDcaTxop::~FullDcaTxop ()
{
  NS_LOG_FUNCTION (this);
}

void
FullDcaTxop::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_dcf;
  delete m_rng;
  delete m_txMiddle;
  m_transmissionListener = 0;
  m_dcf = 0;
  m_rng = 0;
  m_txMiddle = 0;
}

void
FullDcaTxop::SetManager (FullDcfManager *manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_manager = manager;
  m_manager->Add (m_dcf);
}

void
FullDcaTxop::SetLow (Ptr<FullMacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}
void
FullDcaTxop::SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
}
void
FullDcaTxop::SetTxOkCallback (TxOk callback)
{
  m_txOkCallback = callback;
}
void
FullDcaTxop::SetTxFailedCallback (TxFailed callback)
{
  m_txFailedCallback = callback;
}

void
FullDcaTxop::SetAckTimeoutCallback (TxFailed callback)
{
  m_ackTimeoutCallback = callback;
}

void
FullDcaTxop::SetSendPacketCallback (TxFailed callback)
{
	m_sendPacketCallback = callback;
}

Ptr<FullWifiMacQueue >
FullDcaTxop::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
FullDcaTxop::SetEnableBusyTone (bool busy)
{
  m_enableBusyTone = busy;
}
void
FullDcaTxop::SetEnableReturnPacket (bool re)
{
  m_enableReturnPacket = re;
}
void
FullDcaTxop::SetEnableForward (bool forward)
{
  m_enableForward = forward;
}
void
FullDcaTxop::SetForwardQueue (Ptr<ForwardQueue> forwardQueue)
{
  m_forwardQueue = forwardQueue;
}
bool
FullDcaTxop::GetEnableBusyTone (void) const
{
  return m_enableBusyTone;
}
bool
FullDcaTxop::GetEnableReturnPacket (void) const
{
  return m_enableReturnPacket;
}
bool
FullDcaTxop::GetEnableForward (void) const
{
  return m_enableForward;
}
Ptr<ForwardQueue>
FullDcaTxop::GetForwardQueue (void) const
{
  return m_forwardQueue;
}

void
FullDcaTxop::SetMinCw (uint32_t minCw)
{
  NS_LOG_FUNCTION (this << minCw);
  m_dcf->SetCwMin (minCw);
}
void
FullDcaTxop::SetMaxCw (uint32_t maxCw)
{
  NS_LOG_FUNCTION (this << maxCw);
  m_dcf->SetCwMax (maxCw);
}
void
FullDcaTxop::SetAifsn (uint32_t aifsn)
{
  NS_LOG_FUNCTION (this << aifsn);
  m_dcf->SetAifsn (aifsn);
}
uint32_t
FullDcaTxop::GetMinCw (void) const
{
  return m_dcf->GetCwMin ();
}
uint32_t
FullDcaTxop::GetMaxCw (void) const
{
  return m_dcf->GetCwMax ();
}
uint32_t
FullDcaTxop::GetAifsn (void) const
{
  return m_dcf->GetAifsn ();
}

void
FullDcaTxop::Queue (Ptr<const Packet> packet, const FullWifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  FullWifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr,
                                     packet, fullPacketSize);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

int64_t
FullDcaTxop::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rng->AssignStreams (stream);
  return 1;
}

void
FullDcaTxop::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty ())
      && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
FullDcaTxop::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && !m_queue->IsEmpty ()
      && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}


Ptr<FullMacLow>
FullDcaTxop::Low (void)
{
  return m_low;
}

bool
FullDcaTxop::NeedRts (Ptr<const Packet> packet, const FullWifiMacHeader *header)
{
  return m_stationManager->NeedRts (header->GetAddr1 (), header,
                                    packet);
}

void
FullDcaTxop::DoInitialize ()
{
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  ns3::FullDcf::DoInitialize ();
}
bool
FullDcaTxop::NeedRtsRetransmission (void)
{
  return m_stationManager->NeedRtsRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                  m_currentPacket);
}

bool
FullDcaTxop::NeedDataRetransmission (void)
{
  return m_stationManager->NeedDataRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket);
}
bool
FullDcaTxop::NeedFragmentation (void)
{
  return m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket);
}

void
FullDcaTxop::NextFragment (void)
{
  m_fragmentNumber++;
}

uint32_t
FullDcaTxop::GetFragmentSize (void)
{
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber);
}
bool
FullDcaTxop::IsLastFragment (void)
{
  return m_stationManager->IsLastFragment (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                           m_currentPacket, m_fragmentNumber);
}

uint32_t
FullDcaTxop::GetNextFragmentSize (void)
{
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber + 1);
}

uint32_t
FullDcaTxop::GetFragmentOffset (void)
{
  return m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket, m_fragmentNumber);
}

Ptr<Packet>
FullDcaTxop::GetFragmentPacket (FullWifiMacHeader *hdr)
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

bool
FullDcaTxop::NeedsAccess (void) const
{
  return !m_queue->IsEmpty () || m_currentPacket != 0;
}
void
FullDcaTxop::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0)
    {
      if (m_queue->IsEmpty ())
        {
          NS_LOG_DEBUG ("queue empty");
          return;
        }
      m_currentPacket = m_queue->Dequeue (&m_currentHdr);
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
    }
  FullMacLowTransmissionParameters params;
  params.DisableOverrideDurationId ();

  m_sendPacketCallback (m_currentHdr);

  if (m_currentHdr.GetAddr1 ().IsGroup ())
    {
      params.DisableRts ();
      params.DisableAck ();
      params.DisableNextData ();
      Low ()->StartTransmission (m_currentPacket,
                                 &m_currentHdr,
                                 params,
                                 m_transmissionListener);
      NS_LOG_DEBUG ("tx broadcast");
    }
  else
    {
      params.EnableAck ();

      if (NeedFragmentation ())
        {
          FullWifiMacHeader hdr;
          Ptr<Packet> fragment = GetFragmentPacket (&hdr);
          if (NeedRts (fragment, &hdr))
            {
              params.EnableRts ();
            }
          else
            {
              params.DisableRts ();
            }
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
          Low ()->StartTransmission (fragment, &hdr, params,
                                     m_transmissionListener);
        }
      else
        {
          if (NeedRts (m_currentPacket, &m_currentHdr))
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
          Low ()->StartTransmission (m_currentPacket, &m_currentHdr,
                                     params, m_transmissionListener);
        }
    }
}

void
FullDcaTxop::NotifyRxStartNow (Time duration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble)
{
        NS_LOG_FUNCTION (this);

        FullWifiMacHeader receiveHdr;
        packet->PeekHeader (receiveHdr);
        NS_LOG_INFO("srouce: " << receiveHdr.GetAddr2 () <<"  dst: " << receiveHdr.GetAddr1 ());

//        if ( receiveHdr.GetAddr2 () == Mac48Address ("00:00:00:00:00:00")) //ack packets
//          {
//            NS_LOG_DEBUG("DELETEM ME.");
//          }


        if (m_returnEvent.IsRunning ())
          {
            m_returnEvent.Cancel ();
          }
        //check PHY tx state, do nothing if the phy is tx busy
        if ( m_low->GetPhy ()->IsTxStateBusy () || m_low->IsWaitingAckTimeout ())
          {
        	//FIXME: send a busytone signal to fill in the gap
        	if(m_currentHdr.GetAddr1 () == receiveHdr.GetAddr2 ())
        	{   //change the acktimeouts
        		m_low->UpdateDuplexEnd (Simulator::Now  () + duration);
        	}
            return;
          }

        //check to see if this packet is for me or not
        //check the packet type to make sure that it is a data packet
        if (receiveHdr.GetType () == FULL_WIFI_MAC_DATA
            && receiveHdr.GetAddr1 () == m_low->GetAddress ()
            )
          {
            m_low->UpdateDuplexEnd (Simulator::Now  () + duration);
            // send the other transmissions until the MAC header is received
            Time delay = m_low->GetPhy ()->CalculateTxDuration (receiveHdr.GetSize (), txMode, preamble);//MicroSeconds(1);//
            FullWifiMacHeader hdr;
//            Time delay = Seconds(0);
            //current packet is not empty and is for the transmitter
            if (m_currentPacket != 0)
              {
//                if (m_currentHdr.GetAddr1 () == receiveHdr.GetAddr2 () && m_enableReturnPacket)
//                  {
//                    m_dcf->ResetBackoffSlots ();
//                    m_returnEvent = Simulator::Schedule (delay, &FullDcaTxop::NotifyAccessGranted, this);
//                    NS_LOG_INFO("has return packet");
//                    return;
//                  }
                m_queue->PushFront (m_currentPacket, m_currentHdr);
                m_currentPacket = 0;
              }
//            else
//              {
                //current packet is empty, try to select a packet from the queue
                //it could be a return packet or a forward packet depends
                packet = CheckForForwardPacket (&hdr, receiveHdr.GetAddr2 ());
                if (packet != 0 && (m_enableForward || m_enableReturnPacket))
                  {
                    m_queue->PushFront (packet, hdr);
                    m_dcf->ResetBackoffSlots ();
                    //FIXME: send a busytone signal to fill in the gap
                    //FIXME: the return and forward packet should be marked so that the secondary receiver will
                    //not send anything
                    m_returnEvent = Simulator::Schedule (delay, &FullDcaTxop::NotifyAccessGranted, this);
                    NS_LOG_INFO("has return packet");
                    return;
                  }

//              }

            if (m_enableBusyTone)
              { //send busy tone until the mac header is decoded
                if (duration > delay)
                  {
                    NS_LOG_INFO("send busytone");
                    m_returnEvent = Simulator::Schedule (delay, &FullDcaTxop::SendBusyTone, this, duration - delay, receiveHdr.GetAddr2 ());
                  }
              }
          }

}

void
FullDcaTxop::SendBusyTone (Time duration, Mac48Address dst)
{
  //create a fake packet and send it to the given address
  Ptr<Packet> busyPacket = Create<Packet> (0);
  FullWifiMacHeader hdr;
  hdr.SetType (FULL_WIFI_MAC_BUSY_TONE);
  hdr.SetAddr1 (dst);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&hdr);
  hdr.SetSequenceNumber (sequence);
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();
  hdr.SetDuration (duration);

  FullMacLowTransmissionParameters params;
  params.DisableOverrideDurationId ();
  params.DisableRts ();
  params.DisableAck ();
  params.DisableNextData ();
  Low ()->StartTransmission (busyPacket,
                                 &hdr,
                                 params,
                                 m_transmissionListener);
  NS_LOG_DEBUG ("tx busytone");

}

Ptr<const Packet>
FullDcaTxop::CheckForReturnPacket (FullWifiMacHeader *hdr, Mac48Address src)
{
  NS_LOG_FUNCTION (this);
  return m_queue->DequeueFirstAvailable (hdr, src);
}

Ptr<const Packet>
FullDcaTxop::CheckForForwardPacket (FullWifiMacHeader *hdr, Mac48Address src)
{
  NS_LOG_FUNCTION (this);
  Ptr<const Packet> packet;
  if (m_forwardQueue->Size () == 0)
    {
      if (m_enableForward && !m_enableReturnPacket)
        {
//          NS_ASSERT (false);
    	  return 0;
        }
    }

  NS_LOG_INFO("queue size:" << m_forwardQueue->Size () );

  if ((m_enableForward || m_enableReturnPacket))
    {
      if ((!m_enableForward || m_forwardQueue->Size () == 0) && m_enableReturnPacket)
        {
           packet = CheckForReturnPacket (hdr, src);
           if (packet != 0)
           {
        	   hdr->SetType (FULL_WIFI_MAC_RETURN_DATA);
           }
           return packet;
        }
      else
        {
          ForwardMap *map = m_forwardQueue->GetForwardMap (src);
          if (map == 0)  //forward map for the given src not exist
          {
        	  return 0;
          }
          std::list<ForwardItem> qu = map->GetQueue ();
          std::list<ForwardItem>::iterator it;
          for (it = qu.begin (); it != qu.end (); it++)
            {
              Ptr<const Packet> packet = CheckForReturnPacket (hdr, (*it).add);
              if (packet != 0)
                {
            	  hdr->SetType (FULL_WIFI_MAC_FORWARD_DATA);
                  return packet;
                }
            }
        }
    }
  return 0;

}

void
FullDcaTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}
void
FullDcaTxop::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("collision");
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
FullDcaTxop::NotifyChannelSwitching (void)
{
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
FullDcaTxop::GotCts (double snr, FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  NS_LOG_DEBUG ("got cts");
}
void
FullDcaTxop::MissedCts (void)
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
FullDcaTxop::GotAck (double snr, FullWifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  if (!NeedFragmentation ()
      || IsLastFragment ())
    {
      NS_LOG_DEBUG ("got ack. tx done.");
      if (!m_txOkCallback.IsNull ())
        {
          m_txOkCallback (m_currentHdr);
        }

      /* we are not fragmenting or we are done fragmenting
       * so we can get rid of that packet now.
       */
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
FullDcaTxop::MissedAck (void)
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
  m_ackTimeoutCallback (m_currentHdr);
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();

}
void
FullDcaTxop::StartNext (void)
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
FullDcaTxop::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
  /**
   * This happens in only one case: in an AP, you have two DcaTxop:
   *   - one is used exclusively for beacons and has a high priority.
   *   - the other is used for everything else and has a normal
   *     priority.
   *
   * If the normal queue tries to send a unicast data frame, but
   * if the tx fails (ack timeout), it starts a backoff. If the beacon
   * queue gets a tx oportunity during this backoff, it will trigger
   * a call to this Cancel function.
   *
   * Since we are already doing a backoff, we will get access to
   * the medium when we can, we have nothing to do here. We just
   * ignore the cancel event and wait until we are given again a
   * tx oportunity.
   *
   * Note that this is really non-trivial because each of these
   * frames is assigned a sequence number from the same sequence
   * counter (because this is a non-802.11e device) so, the scheme
   * described here fails to ensure in-order delivery of frames
   * at the receiving side. This, however, does not matter in
   * this case because we assume that the receiving side does not
   * update its <seq,ad> tupple for packets whose destination
   * address is a broadcast address.
   */
}

void
FullDcaTxop::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  StartAccessIfNeeded ();
}

} // namespace ns3
