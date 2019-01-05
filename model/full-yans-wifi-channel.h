/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */
#ifndef FULL_YANS_WIFI_CHANNEL_H
#define FULL_YANS_WIFI_CHANNEL_H

#include <vector>
#include <stdint.h>
#include "ns3/packet.h"
#include "full-wifi-channel.h"
#include "full-wifi-mode.h"
#include "full-wifi-preamble.h"

namespace ns3 {

class NetDevice;
class PropagationLossModel;
class PropagationDelayModel;
class FullYansWifiPhy;

/**
 * \brief A Yans wifi channel
 * \ingroup wifi
 *
 * This wifi channel implements the propagation model described in
 * "Yet Another Network Simulator", (http://cutebugs.net/files/wns2-yans.pdf).
 *
 * This class is expected to be used in tandem with the ns3::YansWifiPhy
 * class and contains a ns3::PropagationLossModel and a ns3::PropagationDelayModel.
 * By default, no propagation models are set so, it is the caller's responsability
 * to set them before using the channel.
 */
class FullYansWifiChannel : public FullWifiChannel
{
public:
  static TypeId GetTypeId (void);

  FullYansWifiChannel ();
  virtual ~FullYansWifiChannel ();

  // inherited from Channel.
  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

  void Add (Ptr<FullYansWifiPhy> phy);

  /**
   * \param loss the new propagation loss model.
   */
  void SetPropagationLossModel (Ptr<PropagationLossModel> loss);
  /**
   * \param delay the new propagation delay model.
   */
  void SetPropagationDelayModel (Ptr<PropagationDelayModel> delay);

  /**
   * \param sender the device from which the packet is originating.
   * \param packet the packet to send
   * \param txPowerDbm the tx power associated to the packet
   * \param wifiMode the tx mode associated to the packet
   * \param preamble the preamble associated to the packet
   *
   * This method should not be invoked by normal users. It is
   * currently invoked only from WifiPhy::Send. YansWifiChannel
   * delivers packets only between PHYs with the same m_channelNumber,
   * e.g. PHYs that are operating on the same channel.
   */
  void Send (Ptr<FullYansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm,
             FullWifiMode wifiMode, FullWifiPreamble preamble) const;

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

private:
  FullYansWifiChannel& operator = (const FullYansWifiChannel &);
  FullYansWifiChannel (const FullYansWifiChannel &);

  typedef std::vector<Ptr<FullYansWifiPhy> > PhyList;
  void Receive (std::size_t i, Ptr<Packet> packet, double rxPowerDbm,
                FullWifiMode txMode, FullWifiPreamble preamble) const;


  PhyList m_phyList;
  Ptr<PropagationLossModel> m_loss;
  Ptr<PropagationDelayModel> m_delay;
};

} // namespace ns3


#endif /* FULL_YANS_WIFI_CHANNEL_H */
