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

#include "full-ideal-wifi-manager.h"
#include "full-wifi-phy.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include <cmath>

namespace ns3 {

struct FullIdealWifiRemoteStation : public FullWifiRemoteStation
{
  double m_lastSnr;
};

NS_OBJECT_ENSURE_REGISTERED (FullIdealWifiManager);

TypeId
FullIdealWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullIdealWifiManager")
    .SetParent<FullWifiRemoteStationManager> ()
    .AddConstructor<FullIdealWifiManager> ()
    .AddAttribute ("BerThreshold",
                   "The maximum Bit Error Rate acceptable at any transmission mode",
                   DoubleValue (10e-6),
                   MakeDoubleAccessor (&FullIdealWifiManager::m_ber),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

FullIdealWifiManager::FullIdealWifiManager ()
{
}
FullIdealWifiManager::~FullIdealWifiManager ()
{
}

void
FullIdealWifiManager::SetupPhy (Ptr<FullWifiPhy> phy)
{
  uint32_t nModes = phy->GetNModes ();
  for (uint32_t i = 0; i < nModes; i++)
    {
      FullWifiMode mode = phy->GetMode (i);
      AddModeSnrThreshold (mode, phy->CalculateSnr (mode, m_ber));
    }

  FullWifiRemoteStationManager::SetupPhy (phy);
}

double
FullIdealWifiManager::GetSnrThreshold (FullWifiMode mode) const
{
  for (Thresholds::const_iterator i = m_thresholds.begin (); i != m_thresholds.end (); i++)
    {
      if (mode == i->second)
        {
          return i->first;
        }
    }
  NS_ASSERT (false);
  return 0.0;
}

void
FullIdealWifiManager::AddModeSnrThreshold (FullWifiMode mode, double snr)
{
  m_thresholds.push_back (std::make_pair (snr,mode));
}

FullWifiRemoteStation *
FullIdealWifiManager::DoCreateStation (void) const
{
  FullIdealWifiRemoteStation *station = new FullIdealWifiRemoteStation ();
  station->m_lastSnr = 0.0;
  return station;
}


void
FullIdealWifiManager::DoReportRxOk (FullWifiRemoteStation *station,
                                double rxSnr, FullWifiMode txMode)
{
}
void
FullIdealWifiManager::DoReportRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullIdealWifiManager::DoReportDataFailed (FullWifiRemoteStation *station)
{
}
void
FullIdealWifiManager::DoReportRtsOk (FullWifiRemoteStation *st,
                                 double ctsSnr, FullWifiMode ctsMode, double rtsSnr)
{
  FullIdealWifiRemoteStation *station = (FullIdealWifiRemoteStation *)st;
  station->m_lastSnr = rtsSnr;
}
void
FullIdealWifiManager::DoReportDataOk (FullWifiRemoteStation *st,
                                  double ackSnr, FullWifiMode ackMode, double dataSnr)
{
  FullIdealWifiRemoteStation *station = (FullIdealWifiRemoteStation *)st;
  station->m_lastSnr = dataSnr;
}
void
FullIdealWifiManager::DoReportFinalRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullIdealWifiManager::DoReportFinalDataFailed (FullWifiRemoteStation *station)
{
}

FullWifiMode
FullIdealWifiManager::DoGetDataMode (FullWifiRemoteStation *st, uint32_t size)
{
  FullIdealWifiRemoteStation *station = (FullIdealWifiRemoteStation *)st;
  // We search within the Supported rate set the mode with the
  // highest snr threshold possible which is smaller than m_lastSnr
  // to ensure correct packet delivery.
  double maxThreshold = 0.0;
  FullWifiMode maxMode = GetDefaultMode ();
  for (uint32_t i = 0; i < GetNSupported (station); i++)
    {
      FullWifiMode mode = GetSupported (station, i);
      double threshold = GetSnrThreshold (mode);
      if (threshold > maxThreshold
          && threshold < station->m_lastSnr)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return maxMode;
}
FullWifiMode
FullIdealWifiManager::DoGetRtsMode (FullWifiRemoteStation *st)
{
  FullIdealWifiRemoteStation *station = (FullIdealWifiRemoteStation *)st;
  // We search within the Basic rate set the mode with the highest
  // snr threshold possible which is smaller than m_lastSnr to
  // ensure correct packet delivery.
  double maxThreshold = 0.0;
  FullWifiMode maxMode = GetDefaultMode ();
  for (uint32_t i = 0; i < GetNBasicModes (); i++)
    {
      FullWifiMode mode = GetBasicMode (i);
      double threshold = GetSnrThreshold (mode);
      if (threshold > maxThreshold
          && threshold < station->m_lastSnr)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return maxMode;
}

bool
FullIdealWifiManager::IsLowLatency (void) const
{
  return true;
}

} // namespace ns3
