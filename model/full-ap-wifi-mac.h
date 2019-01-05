/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
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
#ifndef FULL_AP_WIFI_MAC_H
#define FULL_AP_WIFI_MAC_H

#include "full-regular-wifi-mac.h"

#include "full-amsdu-subframe-header.h"
#include "full-supported-rates.h"

namespace ns3 {

/**
 * \brief Wi-Fi AP state machine
 * \ingroup wifi
 *
 * Handle association, dis-association and authentication,
 * of STAs within an infrastructure BSS.
 */
class FullApWifiMac : public FullRegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  FullApWifiMac ();
  virtual ~FullApWifiMac ();

  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<FullWifiRemoteStationManager> stationManager);

  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp);

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   * \param from the address from which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.  The extra parameter "from" allows
   * this device to operate in a bridged mode, forwarding received
   * frames without altering the source address.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from);
  virtual bool SupportsSendFrom (void) const;

  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
   * \param interval the interval between two beacon transmissions.
   */
  void SetBeaconInterval (Time interval);
  /**
   * \returns the interval between two beacon transmissions.
   */
  Time GetBeaconInterval (void) const;
  /**
   * Start beacon transmission immediately.
   */
  void StartBeaconing (void);

private:
  virtual void Receive (Ptr<Packet> packet, const FullWifiMacHeader *hdr);
  virtual void TxOk (const FullWifiMacHeader &hdr);
  virtual void TxFailed (const FullWifiMacHeader &hdr);

  /**
   * This method is called to de-aggregate an A-MSDU and forward the
   * constituent packets up the stack. We override the WifiMac version
   * here because, as an AP, we also need to think about redistributing
   * to other associated STAs.
   *
   * \param aggregatedPacket the Packet containing the A-MSDU.
   * \param hdr a pointer to the MAC header for \c aggregatedPacket.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket,
                                           const FullWifiMacHeader *hdr);

  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);
  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to, uint8_t tid);
  void SendProbeResp (Mac48Address to);
  void SendAssocResp (Mac48Address to, bool success);
  void SendOneBeacon (void);
  FullSupportedRates GetSupportedRates (void) const;
  void SetBeaconGeneration (bool enable);
  bool GetBeaconGeneration (void) const;
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  Ptr<FullDcaTxop> m_beaconDca;
  Time m_beaconInterval;
  bool m_enableBeaconGeneration;
  EventId m_beaconEvent;
};

} // namespace ns3

#endif /* FULL_AP_WIFI_MAC_H */
