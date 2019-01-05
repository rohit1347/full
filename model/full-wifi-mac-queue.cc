/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
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

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "full-wifi-mac-queue.h"
#include "full-qos-blocked-destinations.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullWifiMacQueue);

FullWifiMacQueue::FullItem::FullItem (Ptr<const Packet> packet,
                          const FullWifiMacHeader &hdr,
                          Time tstamp)
  : packet (packet),
    hdr (hdr),
    tstamp (tstamp)
{
}

TypeId
FullWifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiMacQueue")
    .SetParent<Object> ()
    .AddConstructor<FullWifiMacQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (400),
                   MakeUintegerAccessor (&FullWifiMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (10.0)),
                   MakeTimeAccessor (&FullWifiMacQueue::m_maxDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

FullWifiMacQueue::FullWifiMacQueue ()
  : m_size (0)
{
}

FullWifiMacQueue::~FullWifiMacQueue ()
{
  Flush ();
}

void
FullWifiMacQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void
FullWifiMacQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t
FullWifiMacQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

Time
FullWifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

void
FullWifiMacQueue::Enqueue (Ptr<const Packet> packet, const FullWifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_back (FullItem (packet, hdr, now));
  m_size++;
}

void
FullWifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end ();)
    {
      if (i->tstamp + m_maxDelay > now)
        {
          i++;
        }
      else
        {
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet>
FullWifiMacQueue::Dequeue (FullWifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      FullItem i = m_queue.front ();
      m_queue.pop_front ();
      m_size--;
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
FullWifiMacQueue::Peek (FullWifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      FullItem i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
FullWifiMacQueue::DequeueByTidAndAddress (FullWifiMacHeader *hdr, uint8_t tid,
                                      FullWifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      NS_ASSERT (type <= 4);
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  packet = it->packet;
                  *hdr = it->hdr;
                  m_queue.erase (it);
                  m_size--;
                  break;
                }
            }
        }
    }
  return packet;
}

Ptr<const Packet>
FullWifiMacQueue::PeekByTidAndAddress (FullWifiMacHeader *hdr, uint8_t tid,
                                   FullWifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      NS_ASSERT (type <= 4);
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  *hdr = it->hdr;
                  return it->packet;
                }
            }
        }
    }
  return 0;
}

bool
FullWifiMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

uint32_t
FullWifiMacQueue::GetSize (void)
{
  return m_size;
}

void
FullWifiMacQueue::Flush (void)
{
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}

Mac48Address
FullWifiMacQueue::GetAddressForPacket (enum FullWifiMacHeader::AddressType type, PacketQueueI it)
{
  if (type == FullWifiMacHeader::ADDR1)
    {
      return it->hdr.GetAddr1 ();
    }
  if (type == FullWifiMacHeader::ADDR2)
    {
      return it->hdr.GetAddr2 ();
    }
  if (type == FullWifiMacHeader::ADDR3)
    {
      return it->hdr.GetAddr3 ();
    }
  return 0;
}

bool
FullWifiMacQueue::Remove (Ptr<const Packet> packet)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

void
FullWifiMacQueue::PushFront (Ptr<const Packet> packet, const FullWifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (FullItem (packet, hdr, now));
  m_size++;
}

Ptr<const Packet>
FullWifiMacQueue::DequeueFirstAvailable (FullWifiMacHeader *hdr, Mac48Address src)
{
  PacketQueueI it = m_queue.begin ();
  Ptr<const Packet> packet = 0;
  for (; it != m_queue.end (); it++)
    {
      if (it->hdr.GetAddr1 () == src)
        {
          *hdr = it->hdr;
          packet = it->packet;
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return 0;
}

Ptr<const Packet>
FullWifiMacQueue::PeekFirstAvailable (FullWifiMacHeader *hdr, Mac48Address src)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->hdr.GetAddr1 () == src)
        {
          *hdr = it->hdr;
          return it->packet;
        }
    }
  return 0;
}

uint32_t
FullWifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, FullWifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      NS_ASSERT (type <= 4);
      for (it = m_queue.begin (); it != m_queue.end (); it++)
        {
          if (GetAddressForPacket (type, it) == addr)
            {
              if (it->hdr.IsQosData () && it->hdr.GetQosTid () == tid)
                {
                  nPackets++;
                }
            }
        }
    }
  return nPackets;
}

Ptr<const Packet>
FullWifiMacQueue::DequeueFirstAvailable (FullWifiMacHeader *hdr, Time &timestamp,
                                     const FullQosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          packet = it->packet;
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return packet;
}

Ptr<const Packet>
FullWifiMacQueue::PeekFirstAvailable (FullWifiMacHeader *hdr, Time &timestamp,
                                  const FullQosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          return it->packet;
        }
    }
  return 0;
}

} // namespace ns3
