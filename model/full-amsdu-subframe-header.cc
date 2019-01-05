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
#include "full-amsdu-subframe-header.h"
#include "ns3/address-utils.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullAmsduSubframeHeader);

TypeId
FullAmsduSubframeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FullAmsduSubframeHeader")
    .SetParent<Header> ()
    .AddConstructor<FullAmsduSubframeHeader> ()
  ;
  return tid;
}

TypeId
FullAmsduSubframeHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

FullAmsduSubframeHeader::FullAmsduSubframeHeader ()
  : m_length (0)
{
}

FullAmsduSubframeHeader::~FullAmsduSubframeHeader ()
{
}

uint32_t
FullAmsduSubframeHeader::GetSerializedSize () const
{
  return (6 + 6 + 2);
}

void
FullAmsduSubframeHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_da);
  WriteTo (i, m_sa);
  i.WriteHtolsbU16 (m_length);
}

uint32_t
FullAmsduSubframeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  ReadFrom (i, m_da);
  ReadFrom (i, m_sa);
  m_length = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
FullAmsduSubframeHeader::Print (std::ostream &os) const
{
  os << "DA = " << m_da << ", SA = " << m_sa << ", length = " << m_length;
}

void
FullAmsduSubframeHeader::SetDestinationAddr (Mac48Address to)
{
  m_da = to;
}

void
FullAmsduSubframeHeader::SetSourceAddr (Mac48Address from)
{
  m_sa = from;
}

void
FullAmsduSubframeHeader::SetLength (uint16_t length)
{
  m_length = length;
}

Mac48Address
FullAmsduSubframeHeader::GetDestinationAddr (void) const
{
  return m_da;
}

Mac48Address
FullAmsduSubframeHeader::GetSourceAddr (void) const
{
  return m_sa;
}

uint16_t
FullAmsduSubframeHeader::GetLength (void) const
{
  return m_length;
}

} // namespace ns3
