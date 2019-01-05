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
 * Author: Federico Maguolo <maguolof@dei.unipd.it>
 */
#ifndef FULL_CARA_WIFI_MANAGER_H
#define FULL_CARA_WIFI_MANAGER_H

#include "full-wifi-remote-station-manager.h"

namespace ns3 {

/**
 * \brief implement the CARA rate control algorithm
 * \ingroup wifi
 *
 * Implement the CARA algorithm from:
 * J. Kim, S. Kim, S. Choi, and D. Qiao.
 * "CARA: Collision-Aware Rate Adaptation for IEEE 802.11 WLANs."
 *
 * Originally implemented by Federico Maguolo for a very early
 * prototype version of ns-3.
 */
class FullCaraWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);
  FullCaraWifiManager ();
  virtual ~FullCaraWifiManager ();

private:
  // overriden from base class
  virtual FullWifiRemoteStation * DoCreateStation (void) const;
  virtual void DoReportRxOk (FullWifiRemoteStation *station,
                             double rxSnr, FullWifiMode txMode);
  virtual void DoReportRtsFailed (FullWifiRemoteStation *station);
  virtual void DoReportDataFailed (FullWifiRemoteStation *station);
  virtual void DoReportRtsOk (FullWifiRemoteStation *station,
                              double ctsSnr, FullWifiMode ctsMode, double rtsSnr);
  virtual void DoReportDataOk (FullWifiRemoteStation *station,
                               double ackSnr, FullWifiMode ackMode, double dataSnr);
  virtual void DoReportFinalRtsFailed (FullWifiRemoteStation *station);
  virtual void DoReportFinalDataFailed (FullWifiRemoteStation *station);
  virtual FullWifiMode DoGetDataMode (FullWifiRemoteStation *station, uint32_t size);
  virtual FullWifiMode DoGetRtsMode (FullWifiRemoteStation *station);
  virtual bool DoNeedRts (FullWifiRemoteStation *station,
                          Ptr<const Packet> packet, bool normally);
  virtual bool IsLowLatency (void) const;

  uint32_t m_timerTimeout;
  uint32_t m_successThreshold;
  uint32_t m_failureThreshold;
  uint32_t m_probeThreshold;
};

} // namespace ns3

#endif /* FULL_CARA_WIFI_MANAGER_H */
