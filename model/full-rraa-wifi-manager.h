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
#ifndef FULL_RRAA_WIFI_MANAGER_H
#define FULL_RRAA_WIFI_MANAGER_H

#include "ns3/nstime.h"
#include "full-wifi-remote-station-manager.h"

namespace ns3 {

struct FullRraaWifiRemoteStation;

/**
 * \brief Robust Rate Adaptation Algorithm
 * \ingroup wifi
 *
 * This is an implementation of RRAA as described in
 * "Robust rate adaptation for 802.11 wireless networks"
 * by "Starsky H. Y. Wong", "Hao Yang", "Songwu Lu", and,
 * "Vaduvur Bharghavan" published in Mobicom 06.
 */
class FullRraaWifiManager : public FullWifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);

  FullRraaWifiManager ();
  virtual ~FullRraaWifiManager ();

private:
  struct ThresholdsItem
  {
    uint32_t datarate;
    double pori;
    double pmtl;
    uint32_t ewnd;
  };

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
  virtual bool DoNeedRts (FullWifiRemoteStation *st,
                          Ptr<const Packet> packet, bool normally);
  virtual bool IsLowLatency (void) const;

  uint32_t GetMaxRate (FullRraaWifiRemoteStation *station);
  uint32_t GetMinRate (FullRraaWifiRemoteStation *station);
  void CheckTimeout (FullRraaWifiRemoteStation *station);
  void RunBasicAlgorithm (FullRraaWifiRemoteStation *station);
  void ARts (FullRraaWifiRemoteStation *station);
  void ResetCountersBasic (FullRraaWifiRemoteStation *station);
  struct ThresholdsItem GetThresholds (FullWifiMode mode) const;
  struct ThresholdsItem GetThresholds (FullRraaWifiRemoteStation *station, uint32_t rate) const;

  bool m_basic;
  Time m_timeout;
  uint32_t m_ewndfor54;
  uint32_t m_ewndfor48;
  uint32_t m_ewndfor36;
  uint32_t m_ewndfor24;
  uint32_t m_ewndfor18;
  uint32_t m_ewndfor12;
  uint32_t m_ewndfor9;
  uint32_t m_ewndfor6;
  double m_porifor48;
  double m_porifor36;
  double m_porifor24;
  double m_porifor18;
  double m_porifor12;
  double m_porifor9;
  double m_porifor6;
  double m_pmtlfor54;
  double m_pmtlfor48;
  double m_pmtlfor36;
  double m_pmtlfor24;
  double m_pmtlfor18;
  double m_pmtlfor12;
  double m_pmtlfor9;
};

} // namespace ns3

#endif /* FULL_RRAA_WIFI_MANAGER_H */
