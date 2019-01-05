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
#ifndef FULL_IDEAL_WIFI_MANAGER_H
#define FULL_IDEAL_WIFI_MANAGER_H

#include <stdint.h>
#include <vector>
#include "full-wifi-mode.h"
#include "full-wifi-remote-station-manager.h"

namespace ns3 {

/**
 * \brief Ideal rate control algorithm
 * \ingroup wifi
 *
 * This class implements an 'ideal' rate control algorithm
 * similar to RBAR in spirit (see <i>A rate-adaptive MAC
 * protocol for multihop wireless networks</i> by G. Holland,
 * N. Vaidya, and P. Bahl.): every station keeps track of the
 * snr of every packet received and sends back this snr to the
 * original transmitter by an out-of-band mechanism. Each
 * transmitter keeps track of the last snr sent back by a receiver
 * and uses it to pick a transmission mode based on a set
 * of snr thresholds built from a target ber and transmission
 * mode-specific snr/ber curves.
 */
class FullIdealWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);
  FullIdealWifiManager ();
  virtual ~FullIdealWifiManager ();

  virtual void SetupPhy (Ptr<FullWifiPhy> phy);

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

  // return the min snr needed to successfully transmit
  // data with this mode at the specified ber.
  double GetSnrThreshold (FullWifiMode mode) const;
  void AddModeSnrThreshold (FullWifiMode mode, double ber);

  typedef std::vector<std::pair<double,FullWifiMode> > Thresholds;

  double m_ber;
  Thresholds m_thresholds;
  double m_minSnr;
  double m_maxSnr;
};

} // namespace ns3

#endif /* FULL_IDEAL_WIFI_MANAGER_H */
