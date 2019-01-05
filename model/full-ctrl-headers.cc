/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "ns3/fatal-error.h"

#include "full-ctrl-headers.h"

namespace ns3 {

/***********************************
 *       Block ack request
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (FullCtrlBAckRequestHeader);

FullCtrlBAckRequestHeader::FullCtrlBAckRequestHeader ()
  : m_barAckPolicy (false),
    m_multiTid (false),
    m_compressed (false)
{
}

FullCtrlBAckRequestHeader::~FullCtrlBAckRequestHeader ()
{
}

TypeId
FullCtrlBAckRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullCtrlBAckRequestHeader")
    .SetParent<Header> ()
    .AddConstructor<FullCtrlBAckRequestHeader> ()
  ;
  return tid;
}

TypeId
FullCtrlBAckRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullCtrlBAckRequestHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq;
}

uint32_t
FullCtrlBAckRequestHeader::GetSerializedSize () const
{
  uint32_t size = 0;
  size += 2; //Bar control
  if (!m_multiTid)
    {
      size += 2; //Starting sequence control
    }
  else
    {
      if (m_compressed)
        {
          size += (2 + 2) * (m_tidInfo + 1);  //Multi-tid block ack
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return size;
}

void
FullCtrlBAckRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBarControl ());
  if (!m_multiTid)
    {
      i.WriteHtolsbU16 (GetStartingSequenceControl ());
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

uint32_t
FullCtrlBAckRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBarControl (i.ReadLsbtohU16 ());
  if (!m_multiTid)
    {
      SetStartingSequenceControl (i.ReadLsbtohU16 ());
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i.GetDistanceFrom (start);
}

uint16_t
FullCtrlBAckRequestHeader::GetBarControl (void) const
{
  uint16_t res = 0;
  if (m_barAckPolicy)
    {
      res |= 0x1;
    }
  if (m_multiTid)
    {
      res |= (0x1 << 1);
    }
  if (m_compressed)
    {
      res |= (0x1 << 2);
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
FullCtrlBAckRequestHeader::SetBarControl (uint16_t bar)
{
  m_barAckPolicy = ((bar & 0x01) == 1) ? true : false;
  m_multiTid = (((bar >> 1) & 0x01) == 1) ? true : false;
  m_compressed = (((bar >> 2) & 0x01) == 1) ? true : false;
  m_tidInfo = (bar >> 12) & 0x0f;
}

uint16_t
FullCtrlBAckRequestHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
FullCtrlBAckRequestHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

void
FullCtrlBAckRequestHeader::SetHtImmediateAck (bool immediateAck)
{
  m_barAckPolicy = immediateAck;
}

void
FullCtrlBAckRequestHeader::SetType (enum BlockAckType type)
{
  switch (type)
    {
    case BASIC_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = false;
      break;
    case COMPRESSED_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = true;
      break;
    case MULTI_TID_BLOCK_ACK:
      m_multiTid = true;
      m_compressed = true;
      break;
    default:
      NS_FATAL_ERROR ("Invalid variant type");
      break;
    }
}

void
FullCtrlBAckRequestHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
FullCtrlBAckRequestHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
FullCtrlBAckRequestHeader::MustSendHtImmediateAck (void) const
{
  return m_barAckPolicy;
}

uint8_t
FullCtrlBAckRequestHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
FullCtrlBAckRequestHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
FullCtrlBAckRequestHeader::IsBasic (void) const
{
  return (!m_multiTid && !m_compressed) ? true : false;
}

bool
FullCtrlBAckRequestHeader::IsCompressed (void) const
{
  return (!m_multiTid && m_compressed) ? true : false;
}

bool
FullCtrlBAckRequestHeader::IsMultiTid (void) const
{
  return (m_multiTid && m_compressed) ? true : false;
}

/***********************************
 *       Block ack response
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (FullCtrlBAckResponseHeader);

FullCtrlBAckResponseHeader::FullCtrlBAckResponseHeader ()
  : m_baAckPolicy (false),
    m_multiTid (false),
    m_compressed (false)
{
  memset (&bitmap, 0, sizeof (bitmap));
}

FullCtrlBAckResponseHeader::~FullCtrlBAckResponseHeader ()
{
}

TypeId
FullCtrlBAckResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullCtrlBAckResponseHeader")
    .SetParent<Header> ()
    .AddConstructor<FullCtrlBAckResponseHeader> ()
  ;
  return tid;
}

TypeId
FullCtrlBAckResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullCtrlBAckResponseHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq;
}

uint32_t
FullCtrlBAckResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //Bar control
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          size += (2 + 128); //Basic block ack
        }
      else
        {
          size += (2 + 8); //Compressed block ack
        }
    }
  else
    {
      if (m_compressed)
        {
          size += (2 + 2 + 8) * (m_tidInfo + 1); //Multi-tid block ack
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return size;
}

void
FullCtrlBAckResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBaControl ());
  if (!m_multiTid)
    {
      i.WriteHtolsbU16 (GetStartingSequenceControl ());
      i = SerializeBitmap (i);
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

uint32_t
FullCtrlBAckResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBaControl (i.ReadLsbtohU16 ());
  if (!m_multiTid)
    {
      SetStartingSequenceControl (i.ReadLsbtohU16 ());
      i = DeserializeBitmap (i);
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i.GetDistanceFrom (start);
}

void
FullCtrlBAckResponseHeader::SetHtImmediateAck (bool immediateAck)
{
  m_baAckPolicy = immediateAck;
}

void
FullCtrlBAckResponseHeader::SetType (enum BlockAckType type)
{
  switch (type)
    {
    case BASIC_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = false;
      break;
    case COMPRESSED_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = true;
      break;
    case MULTI_TID_BLOCK_ACK:
      m_multiTid = true;
      m_compressed = true;
      break;
    default:
      NS_FATAL_ERROR ("Invalid variant type");
      break;
    }
}

void
FullCtrlBAckResponseHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
FullCtrlBAckResponseHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
FullCtrlBAckResponseHeader::MustSendHtImmediateAck (void) const
{
  return (m_baAckPolicy) ? true : false;
}

uint8_t
FullCtrlBAckResponseHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
FullCtrlBAckResponseHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
FullCtrlBAckResponseHeader::IsBasic (void) const
{
  return (!m_multiTid && !m_compressed) ? true : false;
}

bool
FullCtrlBAckResponseHeader::IsCompressed (void) const
{
  return (!m_multiTid && m_compressed) ? true : false;
}

bool
FullCtrlBAckResponseHeader::IsMultiTid (void) const
{
  return (m_multiTid && m_compressed) ? true : false;
}

uint16_t
FullCtrlBAckResponseHeader::GetBaControl (void) const
{
  uint16_t res = 0;
  if (m_baAckPolicy)
    {
      res |= 0x1;
    }
  if (m_multiTid)
    {
      res |= (0x1 << 1);
    }
  if (m_compressed)
    {
      res |= (0x1 << 2);
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
FullCtrlBAckResponseHeader::SetBaControl (uint16_t ba)
{
  m_baAckPolicy = ((ba & 0x01) == 1) ? true : false;
  m_multiTid = (((ba >> 1) & 0x01) == 1) ? true : false;
  m_compressed = (((ba >> 2) & 0x01) == 1) ? true : false;
  m_tidInfo = (ba >> 12) & 0x0f;
}

uint16_t
FullCtrlBAckResponseHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
FullCtrlBAckResponseHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

Buffer::Iterator
FullCtrlBAckResponseHeader::SerializeBitmap (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          for (uint32_t j = 0; j < 64; j++)
            {
              i.WriteHtolsbU16 (bitmap.m_bitmap[j]);
            }
        }
      else
        {
          i.WriteHtolsbU64 (bitmap.m_compressedBitmap);
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i;
}

Buffer::Iterator
FullCtrlBAckResponseHeader::DeserializeBitmap (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          for (uint32_t j = 0; j < 64; j++)
            {
              bitmap.m_bitmap[j] = i.ReadLsbtohU16 ();
            }
        }
      else
        {
          bitmap.m_compressedBitmap = i.ReadLsbtohU64 ();
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i;
}

void
FullCtrlBAckResponseHeader::SetReceivedPacket (uint16_t seq)
{
  if (!IsInBitmap (seq))
    {
      return;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          /* To set correctly basic block ack bitmap we need fragment number too.
             So if it's not specified, we consider packet not fragmented. */
          bitmap.m_bitmap[IndexInBitmap (seq)] |= 0x0001;
        }
      else
        {
          bitmap.m_compressedBitmap |= (uint64_t (0x0000000000000001) << IndexInBitmap (seq));
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

void
FullCtrlBAckResponseHeader::SetReceivedFragment (uint16_t seq, uint8_t frag)
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          bitmap.m_bitmap[IndexInBitmap (seq)] |= (0x0001 << frag);
        }
      else
        {
          /* We can ignore this...compressed block ack doesn't support
             acknowledgement of single fragments */
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

bool
FullCtrlBAckResponseHeader::IsPacketReceived (uint16_t seq) const
{
  if (!IsInBitmap (seq))
    {
      return false;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          /*It's impossible to say if an entire packet was correctly received. */
          return false;
        }
      else
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return false;
}

bool
FullCtrlBAckResponseHeader::IsFragmentReceived (uint16_t seq, uint8_t frag) const
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return false;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          return ((bitmap.m_bitmap[IndexInBitmap (seq)] & (0x0001 << frag)) != 0x0000) ? true : false;
        }
      else
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return false;
}

uint8_t
FullCtrlBAckResponseHeader::IndexInBitmap (uint16_t seq) const
{
  uint8_t index;
  if (seq >= m_startingSeq)
    {
      index = seq - m_startingSeq;
    }
  else
    {
      index = 4096 - m_startingSeq + seq;
    }
  NS_ASSERT (index <= 63);
  return index;
}

bool
FullCtrlBAckResponseHeader::IsInBitmap (uint16_t seq) const
{
  return (seq - m_startingSeq + 4096) % 4096 < 64;
}

const uint16_t*
FullCtrlBAckResponseHeader::GetBitmap (void) const
{
  return bitmap.m_bitmap;
}

uint64_t
FullCtrlBAckResponseHeader::GetCompressedBitmap (void) const
{
  return bitmap.m_compressedBitmap;
}

void
FullCtrlBAckResponseHeader::ResetBitmap (void)
{
  memset (&bitmap, 0, sizeof (bitmap));
}

}  // namespace ns3
