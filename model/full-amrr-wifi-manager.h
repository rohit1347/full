/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2003,2007 INRIA
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
#ifndef FULL_AMRR_WIFI_MANAGER_H
#define FULL_AMRR_WIFI_MANAGER_H

#include "full-wifi-remote-station-manager.h"
#include "ns3/nstime.h"

namespace ns3 {

struct FullAmrrWifiRemoteStation;

/**
 * \brief AMRR Rate control algorithm
 * \ingroup wifi
 *
 * This class implements the AMRR rate control algorithm which
 * was initially described in <i>IEEE 802.11 Rate Adaptation:
 * A Practical Approach</i>, by M. Lacage, M.H. Manshaei, and
 * T. Turletti.
 */
class FullAmrrWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);

  FullAmrrWifiManager ();

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
  virtual bool IsLowLatency (void) const;

  void UpdateRetry (FullAmrrWifiRemoteStation *station);
  void UpdateMode (FullAmrrWifiRemoteStation *station);
  void ResetCnt (FullAmrrWifiRemoteStation *station);
  void IncreaseRate (FullAmrrWifiRemoteStation *station);
  void DecreaseRate (FullAmrrWifiRemoteStation *station);
  bool IsMinRate (FullAmrrWifiRemoteStation *station) const;
  bool IsMaxRate (FullAmrrWifiRemoteStation *station) const;
  bool IsSuccess (FullAmrrWifiRemoteStation *station) const;
  bool IsFailure (FullAmrrWifiRemoteStation *station) const;
  bool IsEnough (FullAmrrWifiRemoteStation *station) const;

  Time m_updatePeriod;
  double m_failureRatio;
  double m_successRatio;
  uint32_t m_maxSuccessThreshold;
  uint32_t m_minSuccessThreshold;
};

} // namespace ns3

#endif /* FULL_AMRR_WIFI_MANAGER_H */
