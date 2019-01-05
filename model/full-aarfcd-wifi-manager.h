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
#ifndef FULL_AARFCD_WIFI_MANAGER_H
#define FULL_AARFCD_WIFI_MANAGER_H

#include "full-wifi-remote-station-manager.h"

namespace ns3 {

struct FullAarfcdWifiRemoteStation;

/**
 * \brief an implementation of the AARF-CD algorithm
 * \ingroup wifi
 *
 * This algorithm was first described in "Efficient Collision Detection for Auto Rate Fallback Algorithm".
 * The implementation available here was done by Federico Maguolo for a very early development
 * version of ns-3. Federico died before merging this work in ns-3 itself so his code was ported
 * to ns-3 later without his supervision.
 */
class FullAarfcdWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);
  FullAarfcdWifiManager ();
  virtual ~FullAarfcdWifiManager ();

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

  void CheckRts (FullAarfcdWifiRemoteStation *station);
  void IncreaseRtsWnd (FullAarfcdWifiRemoteStation *station);
  void ResetRtsWnd (FullAarfcdWifiRemoteStation *station);
  void TurnOffRts (FullAarfcdWifiRemoteStation *station);
  void TurnOnRts (FullAarfcdWifiRemoteStation *station);

  // aarf fields below
  uint32_t m_minTimerThreshold;
  uint32_t m_minSuccessThreshold;
  double m_successK;
  uint32_t m_maxSuccessThreshold;
  double m_timerK;

  // aarf-cd fields below
  uint32_t m_minRtsWnd;
  uint32_t m_maxRtsWnd;
  bool m_rtsFailsAsDataFails;
  bool m_turnOffRtsAfterRateDecrease;
  bool m_turnOnRtsAfterRateIncrease;
};

} // namespace ns3

#endif /* FULL_AARFCD_WIFI_MANAGER_H */
