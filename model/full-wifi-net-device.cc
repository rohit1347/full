/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
#include "full-wifi-net-device.h"
#include "full-wifi-mac.h"
#include "full-wifi-phy.h"
#include "full-wifi-remote-station-manager.h"
#include "full-wifi-channel.h"
#include "ns3/llc-snap-header.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("FullWifiNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullWifiNetDevice);

TypeId
FullWifiNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<FullWifiNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH),
                   MakeUintegerAccessor (&FullWifiNetDevice::SetMtu,
                                         &FullWifiNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> (1,MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH))
    .AddAttribute ("Channel", "The channel attached to this device",
                   PointerValue (),
                   MakePointerAccessor (&FullWifiNetDevice::DoGetChannel),
                   MakePointerChecker<FullWifiChannel> ())
    .AddAttribute ("Phy", "The PHY layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&FullWifiNetDevice::GetPhy,
                                        &FullWifiNetDevice::SetPhy),
                   MakePointerChecker<FullWifiPhy> ())
    .AddAttribute ("Mac", "The MAC layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&FullWifiNetDevice::GetMac,
                                        &FullWifiNetDevice::SetMac),
                   MakePointerChecker<FullWifiMac> ())
    .AddAttribute ("RemoteStationManager", "The station manager attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&FullWifiNetDevice::SetRemoteStationManager,
                                        &FullWifiNetDevice::GetRemoteStationManager),
                   MakePointerChecker<FullWifiRemoteStationManager> ())
  ;
  return tid;
}

FullWifiNetDevice::FullWifiNetDevice ()
  : m_configComplete (false)
{
  NS_LOG_FUNCTION_NOARGS ();
}
FullWifiNetDevice::~FullWifiNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
FullWifiNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = 0;
  m_mac->Dispose ();
  m_phy->Dispose ();
  m_stationManager->Dispose ();
  m_mac = 0;
  m_phy = 0;
  m_stationManager = 0;
  // chain up.
  NetDevice::DoDispose ();
}

void
FullWifiNetDevice::DoInitialize (void)
{
  m_phy->Initialize ();
  m_mac->Initialize ();
  m_stationManager->Initialize ();
  NetDevice::DoInitialize ();
}

void
FullWifiNetDevice::CompleteConfig (void)
{
  if (m_mac == 0
      || m_phy == 0
      || m_stationManager == 0
      || m_node == 0
      || m_configComplete)
    {
      return;
    }
  m_mac->SetWifiRemoteStationManager (m_stationManager);
  m_mac->SetWifiPhy (m_phy);
  m_mac->SetForwardUpCallback (MakeCallback (&FullWifiNetDevice::ForwardUp, this));
  m_mac->SetLinkUpCallback (MakeCallback (&FullWifiNetDevice::LinkUp, this));
  m_mac->SetLinkDownCallback (MakeCallback (&FullWifiNetDevice::LinkDown, this));
  m_stationManager->SetupPhy (m_phy);
  m_configComplete = true;
}

void
FullWifiNetDevice::SetMac (Ptr<FullWifiMac> mac)
{
  m_mac = mac;
  CompleteConfig ();
}
void
FullWifiNetDevice::SetPhy (Ptr<FullWifiPhy> phy)
{
  m_phy = phy;
  CompleteConfig ();
}
void
FullWifiNetDevice::SetRemoteStationManager (Ptr<FullWifiRemoteStationManager> manager)
{
  m_stationManager = manager;
  CompleteConfig ();
}
Ptr<FullWifiMac>
FullWifiNetDevice::GetMac (void) const
{
  return m_mac;
}
Ptr<FullWifiPhy>
FullWifiNetDevice::GetPhy (void) const
{
  return m_phy;
}
Ptr<FullWifiRemoteStationManager>
FullWifiNetDevice::GetRemoteStationManager (void) const
{
  return m_stationManager;
}

void
FullWifiNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}
uint32_t
FullWifiNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}
Ptr<Channel>
FullWifiNetDevice::GetChannel (void) const
{
  return m_phy->GetChannel ();
}
Ptr<FullWifiChannel>
FullWifiNetDevice::DoGetChannel (void) const
{
  return m_phy->GetChannel ();
}
void
FullWifiNetDevice::SetAddress (Address address)
{
  m_mac->SetAddress (Mac48Address::ConvertFrom (address));
}
Address
FullWifiNetDevice::GetAddress (void) const
{
  return m_mac->GetAddress ();
}
bool
FullWifiNetDevice::SetMtu (const uint16_t mtu)
{
  if (mtu > MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH)
    {
      return false;
    }
  m_mtu = mtu;
  return true;
}
uint16_t
FullWifiNetDevice::GetMtu (void) const
{
  return m_mtu;
}
bool
FullWifiNetDevice::IsLinkUp (void) const
{
  return m_phy != 0 && m_linkUp;
}
void
FullWifiNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChanges.ConnectWithoutContext (callback);
}
bool
FullWifiNetDevice::IsBroadcast (void) const
{
  return true;
}
Address
FullWifiNetDevice::GetBroadcast (void) const
{
  return Mac48Address::GetBroadcast ();
}
bool
FullWifiNetDevice::IsMulticast (void) const
{
  return true;
}
Address
FullWifiNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  return Mac48Address::GetMulticast (multicastGroup);
}
Address FullWifiNetDevice::GetMulticast (Ipv6Address addr) const
{
  return Mac48Address::GetMulticast (addr);
}
bool
FullWifiNetDevice::IsPointToPoint (void) const
{
  return false;
}
bool
FullWifiNetDevice::IsBridge (void) const
{
  return false;
}
bool
FullWifiNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_ASSERT (Mac48Address::IsMatchingType (dest));

  Mac48Address realTo = Mac48Address::ConvertFrom (dest);

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packet->AddHeader (llc);

  m_mac->NotifyTx (packet);
  m_mac->Enqueue (packet, realTo);
  return true;
}
Ptr<Node>
FullWifiNetDevice::GetNode (void) const
{
  return m_node;
}
void
FullWifiNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
  CompleteConfig ();
}
bool
FullWifiNetDevice::NeedsArp (void) const
{
  return true;
}
void
FullWifiNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_forwardUp = cb;
}

void
FullWifiNetDevice::ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  LlcSnapHeader llc;
  packet->RemoveHeader (llc);
  enum NetDevice::PacketType type;
  if (to.IsBroadcast ())
    {
      type = NetDevice::PACKET_BROADCAST;
    }
  else if (to.IsGroup ())
    {
      type = NetDevice::PACKET_MULTICAST;
    }
  else if (to == m_mac->GetAddress ())
    {
      type = NetDevice::PACKET_HOST;
    }
  else
    {
      type = NetDevice::PACKET_OTHERHOST;
    }

  if (type != NetDevice::PACKET_OTHERHOST)
    {
      m_mac->NotifyRx (packet);
      m_forwardUp (this, packet, llc.GetType (), from);
    }

  if (!m_promiscRx.IsNull ())
    {
      m_mac->NotifyPromiscRx (packet);
      m_promiscRx (this, packet, llc.GetType (), from, to, type);
    }
}

void
FullWifiNetDevice::LinkUp (void)
{
  m_linkUp = true;
  m_linkChanges ();
}
void
FullWifiNetDevice::LinkDown (void)
{
  m_linkUp = false;
  m_linkChanges ();
}

bool
FullWifiNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_ASSERT (Mac48Address::IsMatchingType (dest));
  NS_ASSERT (Mac48Address::IsMatchingType (source));

  Mac48Address realTo = Mac48Address::ConvertFrom (dest);
  Mac48Address realFrom = Mac48Address::ConvertFrom (source);

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packet->AddHeader (llc);

  m_mac->NotifyTx (packet);
  m_mac->Enqueue (packet, realTo, realFrom);

  return true;
}

void
FullWifiNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  m_promiscRx = cb;
  m_mac->SetPromisc();
}

bool
FullWifiNetDevice::SupportsSendFrom (void) const
{
  return m_mac->SupportsSendFrom ();
}

} // namespace ns3

