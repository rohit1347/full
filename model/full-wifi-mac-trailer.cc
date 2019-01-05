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
#include "full-wifi-mac-trailer.h"
#include "ns3/assert.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullWifiMacTrailer);

FullWifiMacTrailer::FullWifiMacTrailer ()
{
}

FullWifiMacTrailer::~FullWifiMacTrailer ()
{
}

TypeId
FullWifiMacTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiMacTrailer")
    .SetParent<Trailer> ()
    .AddConstructor<FullWifiMacTrailer> ()
  ;
  return tid;
}
TypeId
FullWifiMacTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
FullWifiMacTrailer::Print (std::ostream &os) const
{
}
uint32_t
FullWifiMacTrailer::GetSerializedSize (void) const
{
  return FULL_WIFI_MAC_FCS_LENGTH;
}
void
FullWifiMacTrailer::Serialize (Buffer::Iterator start) const
{
  start.Prev (FULL_WIFI_MAC_FCS_LENGTH);
  start.WriteU32 (0);
}
uint32_t
FullWifiMacTrailer::Deserialize (Buffer::Iterator start)
{
  return FULL_WIFI_MAC_FCS_LENGTH;
}

} // namespace ns3
