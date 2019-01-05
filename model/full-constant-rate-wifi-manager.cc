/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005 INRIA
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

#include "full-constant-rate-wifi-manager.h"

#include "ns3/string.h"
#include "ns3/assert.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullConstantRateWifiManager);

TypeId
FullConstantRateWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullConstantRateWifiManager")
    .SetParent<FullWifiRemoteStationManager> ()
    .AddConstructor<FullConstantRateWifiManager> ()
    .AddAttribute ("DataMode", "The transmission mode to use for every data packet transmission",
                   StringValue ("OfdmRate6Mbps"),
                   MakeFullWifiModeAccessor (&FullConstantRateWifiManager::m_dataMode),
                   MakeFullWifiModeChecker ())
    .AddAttribute ("ControlMode", "The transmission mode to use for every control packet transmission.",
                   StringValue ("OfdmRate6Mbps"),
                   MakeFullWifiModeAccessor (&FullConstantRateWifiManager::m_ctlMode),
                   MakeFullWifiModeChecker ())
  ;
  return tid;
}

FullConstantRateWifiManager::FullConstantRateWifiManager ()
{
}
FullConstantRateWifiManager::~FullConstantRateWifiManager ()
{
}


FullWifiRemoteStation *
FullConstantRateWifiManager::DoCreateStation (void) const
{
  FullWifiRemoteStation *station = new FullWifiRemoteStation ();
  return station;
}


void
FullConstantRateWifiManager::DoReportRxOk (FullWifiRemoteStation *station,
                                       double rxSnr, FullWifiMode txMode)
{
}
void
FullConstantRateWifiManager::DoReportRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullConstantRateWifiManager::DoReportDataFailed (FullWifiRemoteStation *station)
{
}
void
FullConstantRateWifiManager::DoReportRtsOk (FullWifiRemoteStation *st,
                                        double ctsSnr, FullWifiMode ctsMode, double rtsSnr)
{
}
void
FullConstantRateWifiManager::DoReportDataOk (FullWifiRemoteStation *st,
                                         double ackSnr, FullWifiMode ackMode, double dataSnr)
{
}
void
FullConstantRateWifiManager::DoReportFinalRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullConstantRateWifiManager::DoReportFinalDataFailed (FullWifiRemoteStation *station)
{
}

FullWifiMode
FullConstantRateWifiManager::DoGetDataMode (FullWifiRemoteStation *st, uint32_t size)
{
  return m_dataMode;
}
FullWifiMode
FullConstantRateWifiManager::DoGetRtsMode (FullWifiRemoteStation *st)
{
  return m_ctlMode;
}

bool
FullConstantRateWifiManager::IsLowLatency (void) const
{
  return true;
}

} // namespace ns3
