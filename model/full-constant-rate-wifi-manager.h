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
#ifndef FULL_CONSTANT_RATE_WIFI_MANAGER_H
#define FULL_CONSTANT_RATE_WIFI_MANAGER_H

#include <stdint.h>

#include "full-wifi-remote-station-manager.h"

namespace ns3 {

/**
 * \ingroup wifi
 * \brief use constant rates for data and control transmissions
 *
 * This class uses always the same transmission rate for every
 * packet sent.
 */
class FullConstantRateWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);
  FullConstantRateWifiManager ();
  virtual ~FullConstantRateWifiManager ();

private:
  // overriden from base class
  virtual FullWifiRemoteStation* DoCreateStation (void) const;
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
  virtual bool IsLowLatency (void) const;

  FullWifiMode m_dataMode;
  FullWifiMode m_ctlMode;
};

} // namespace ns3



#endif /* FULL_CONSTANT_RATE_WIFI_MANAGER_H */
