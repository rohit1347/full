/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
#include "full-capability-information.h"

namespace ns3 {

FullCapabilityInformation::FullCapabilityInformation ()
  : m_capability (0)
{
}

void
FullCapabilityInformation::SetEss (void)
{
  Set (0);
  Clear (1);
}
void
FullCapabilityInformation::SetIbss (void)
{
  Clear (0);
  Set (1);
}

bool
FullCapabilityInformation::IsEss (void) const
{
  return Is (0);
}
bool
FullCapabilityInformation::IsIbss (void) const
{
  return Is (1);
}

void
FullCapabilityInformation::Set (uint8_t n)
{
  uint32_t mask = 1 << n;
  m_capability |= mask;
}

void
FullCapabilityInformation::Clear (uint8_t n)
{
  uint32_t mask = 1 << n;
  m_capability &= ~mask;
}

bool
FullCapabilityInformation::Is (uint8_t n) const
{
  uint32_t mask = 1 << n;
  return (m_capability & mask) == mask;
}


uint32_t
FullCapabilityInformation::GetSerializedSize (void) const
{
  return 2;
}
Buffer::Iterator
FullCapabilityInformation::Serialize (Buffer::Iterator start) const
{
  start.WriteHtolsbU16 (m_capability);
  return start;
}
Buffer::Iterator
FullCapabilityInformation::Deserialize (Buffer::Iterator start)
{
  m_capability = start.ReadLsbtohU16 ();
  return start;
}


} // namespace ns3
