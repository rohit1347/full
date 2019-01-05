/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#include "full-wifi-helper.h"
#include "ns3/full-wifi-net-device.h"
#include "ns3/full-wifi-mac.h"
#include "ns3/full-regular-wifi-mac.h"
#include "ns3/full-dca-txop.h"
#include "ns3/full-edca-txop-n.h"
#include "ns3/full-minstrel-wifi-manager.h"
#include "ns3/full-wifi-phy.h"
#include "ns3/full-wifi-remote-station-manager.h"
#include "ns3/full-wifi-channel.h"
#include "ns3/full-yans-wifi-channel.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("FullWifiHelper");

namespace ns3 {

FullWifiPhyHelper::~FullWifiPhyHelper ()
{
}

FullWifiMacHelper::~FullWifiMacHelper ()
{
}

FullWifiHelper::FullWifiHelper ()
  : m_standard (FULL_WIFI_PHY_STANDARD_80211a)
{
}

FullWifiHelper
FullWifiHelper::Default (void)
{
  FullWifiHelper helper;
//  helper.SetRemoteStationManager ("ns3::FullArfWifiManager");
  helper.SetRemoteStationManager ("ns3::FullAarfWifiManager");
  return helper;
}

void
FullWifiHelper::SetRemoteStationManager (std::string type,
                                     std::string n0, const AttributeValue &v0,
                                     std::string n1, const AttributeValue &v1,
                                     std::string n2, const AttributeValue &v2,
                                     std::string n3, const AttributeValue &v3,
                                     std::string n4, const AttributeValue &v4,
                                     std::string n5, const AttributeValue &v5,
                                     std::string n6, const AttributeValue &v6,
                                     std::string n7, const AttributeValue &v7)
{
  m_stationManager = ObjectFactory ();
  m_stationManager.SetTypeId (type);
  m_stationManager.Set (n0, v0);
  m_stationManager.Set (n1, v1);
  m_stationManager.Set (n2, v2);
  m_stationManager.Set (n3, v3);
  m_stationManager.Set (n4, v4);
  m_stationManager.Set (n5, v5);
  m_stationManager.Set (n6, v6);
  m_stationManager.Set (n7, v7);
}

void
FullWifiHelper::SetStandard (enum FullWifiPhyStandard standard)
{
  m_standard = standard;
}

NetDeviceContainer
FullWifiHelper::Install (const FullWifiPhyHelper &phyHelper,
                     const FullWifiMacHelper &macHelper, NodeContainer c) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<FullWifiNetDevice> device = CreateObject<FullWifiNetDevice> ();
      Ptr<FullWifiRemoteStationManager> manager = m_stationManager.Create<FullWifiRemoteStationManager> ();
      Ptr<FullWifiMac> mac = macHelper.Create ();
      Ptr<FullWifiPhy> phy = phyHelper.Create (node, device);
      mac->SetAddress (Mac48Address::Allocate ());
      mac->ConfigureStandard (m_standard);
      phy->ConfigureStandard (m_standard);
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetRemoteStationManager (manager);
      node->AddDevice (device);
      devices.Add (device);
      NS_LOG_DEBUG ("node=" << node << ", mob=" << node->GetObject<MobilityModel> ());
    }
  return devices;
}

NetDeviceContainer
FullWifiHelper::Install (const FullWifiPhyHelper &phy,
                     const FullWifiMacHelper &mac, Ptr<Node> node) const
{
  return Install (phy, mac, NodeContainer (node));
}

NetDeviceContainer
FullWifiHelper::Install (const FullWifiPhyHelper &phy,
                     const FullWifiMacHelper &mac, std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (phy, mac, NodeContainer (node));
}

void
FullWifiHelper::EnableLogComponents (void)
{
  LogComponentEnable ("FullAarfcd", LOG_LEVEL_ALL);
  LogComponentEnable ("FullAdhocWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("FullAmrrWifiRemoteStation", LOG_LEVEL_ALL);
  LogComponentEnable ("FullApWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("Fullns3::ArfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("FullCara", LOG_LEVEL_ALL);
  LogComponentEnable ("FullDcaTxop", LOG_LEVEL_ALL);
  LogComponentEnable ("FullDcfManager", LOG_LEVEL_ALL);
  LogComponentEnable ("FullDsssErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullEdcaTxopN", LOG_LEVEL_ALL);
  LogComponentEnable ("FullInterferenceHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("FullJakes", LOG_LEVEL_ALL);
  LogComponentEnable ("FullMacLow", LOG_LEVEL_ALL);
  LogComponentEnable ("FullMacRxMiddle", LOG_LEVEL_ALL);
  LogComponentEnable ("FullMsduAggregator", LOG_LEVEL_ALL);
  LogComponentEnable ("FullMsduStandardAggregator", LOG_LEVEL_ALL);
  LogComponentEnable ("FullNistErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullOnoeWifiRemoteStation", LOG_LEVEL_ALL);
  LogComponentEnable ("FullPropagationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullRegularWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("FullRraaWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("FullStaWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("FullSupportedRates", LOG_LEVEL_ALL);
  LogComponentEnable ("FullWifiChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullWifiPhyStateHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("FullWifiPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("FullWifiRemoteStationManager", LOG_LEVEL_ALL);
  LogComponentEnable ("FullYansErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullYansWifiChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("FullYansWifiPhy", LOG_LEVEL_ALL);
}

int64_t
FullWifiHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<FullWifiNetDevice> wifi = DynamicCast<FullWifiNetDevice> (netDevice);
      if (wifi)
        {
          // Handle any random numbers in the PHY objects.
          currentStream += wifi->GetPhy ()->AssignStreams (currentStream);

          // Handle any random numbers in the station managers.
          Ptr<FullWifiRemoteStationManager> manager = wifi->GetRemoteStationManager ();
          Ptr<FullMinstrelWifiManager> minstrel = DynamicCast<FullMinstrelWifiManager> (manager);
          if (minstrel)
            {
              currentStream += minstrel->AssignStreams (currentStream);
            }

          // Handle any random numbers in the MAC objects.
          Ptr<FullWifiMac> mac = wifi->GetMac ();
          Ptr<FullRegularWifiMac> rmac = DynamicCast<FullRegularWifiMac> (mac);
          if (rmac)
            {
              PointerValue ptr;
              rmac->GetAttribute ("DcaTxop", ptr);
              Ptr<FullDcaTxop> dcaTxop = ptr.Get<FullDcaTxop> ();
              currentStream += dcaTxop->AssignStreams (currentStream);

              rmac->GetAttribute ("VO_EdcaTxopN", ptr);
              Ptr<FullEdcaTxopN> vo_edcaTxopN = ptr.Get<FullEdcaTxopN> ();
              currentStream += vo_edcaTxopN->AssignStreams (currentStream);

              rmac->GetAttribute ("VI_EdcaTxopN", ptr);
              Ptr<FullEdcaTxopN> vi_edcaTxopN = ptr.Get<FullEdcaTxopN> ();
              currentStream += vi_edcaTxopN->AssignStreams (currentStream);

              rmac->GetAttribute ("BE_EdcaTxopN", ptr);
              Ptr<FullEdcaTxopN> be_edcaTxopN = ptr.Get<FullEdcaTxopN> ();
              currentStream += be_edcaTxopN->AssignStreams (currentStream);

              rmac->GetAttribute ("BK_EdcaTxopN", ptr);
              Ptr<FullEdcaTxopN> bk_edcaTxopN = ptr.Get<FullEdcaTxopN> ();
              currentStream += bk_edcaTxopN->AssignStreams (currentStream);
            }
        }
    }
  return (currentStream - stream);
}

} // namespace ns3
