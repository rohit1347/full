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

#include "full-amrr-wifi-manager.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

NS_LOG_COMPONENT_DEFINE ("FullAmrrWifiRemoteStation");

namespace ns3 {

struct FullAmrrWifiRemoteStation : public FullWifiRemoteStation
{
  Time m_nextModeUpdate;
  uint32_t m_tx_ok;
  uint32_t m_tx_err;
  uint32_t m_tx_retr;
  uint32_t m_retry;
  uint32_t m_txrate;
  uint32_t m_successThreshold;
  uint32_t m_success;
  bool m_recovery;
};


NS_OBJECT_ENSURE_REGISTERED (FullAmrrWifiManager);

TypeId
FullAmrrWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullAmrrWifiManager")
    .SetParent<FullWifiRemoteStationManager> ()
    .AddConstructor<FullAmrrWifiManager> ()
    .AddAttribute ("UpdatePeriod",
                   "The interval between decisions about rate control changes",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&FullAmrrWifiManager::m_updatePeriod),
                   MakeTimeChecker ())
    .AddAttribute ("FailureRatio",
                   "Ratio of minimum erroneous transmissions needed to switch to a lower rate",
                   DoubleValue (1.0 / 3.0),
                   MakeDoubleAccessor (&FullAmrrWifiManager::m_failureRatio),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("SuccessRatio",
                   "Ratio of maximum erroneous transmissions needed to switch to a higher rate",
                   DoubleValue (1.0 / 10.0),
                   MakeDoubleAccessor (&FullAmrrWifiManager::m_successRatio),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("MaxSuccessThreshold",
                   "Maximum number of consecutive success periods needed to switch to a higher rate",
                   UintegerValue (10),
                   MakeUintegerAccessor (&FullAmrrWifiManager::m_maxSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinSuccessThreshold",
                   "Minimum number of consecutive success periods needed to switch to a higher rate",
                   UintegerValue (1),
                   MakeUintegerAccessor (&FullAmrrWifiManager::m_minSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

FullAmrrWifiManager::FullAmrrWifiManager ()
{
}

FullWifiRemoteStation *
FullAmrrWifiManager::DoCreateStation (void) const
{
  FullAmrrWifiRemoteStation *station = new FullAmrrWifiRemoteStation ();
  station->m_nextModeUpdate = Simulator::Now () + m_updatePeriod;
  station->m_tx_ok = 0;
  station->m_tx_err = 0;
  station->m_tx_retr = 0;
  station->m_retry = 0;
  station->m_txrate = 0;
  station->m_successThreshold = m_minSuccessThreshold;
  station->m_success = 0;
  station->m_recovery = false;
  return station;
}


void
FullAmrrWifiManager::DoReportRxOk (FullWifiRemoteStation *station,
                               double rxSnr, FullWifiMode txMode)
{
}
void
FullAmrrWifiManager::DoReportRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullAmrrWifiManager::DoReportDataFailed (FullWifiRemoteStation *st)
{
  FullAmrrWifiRemoteStation *station = (FullAmrrWifiRemoteStation *)st;
  station->m_retry++;
  station->m_tx_retr++;
}
void
FullAmrrWifiManager::DoReportRtsOk (FullWifiRemoteStation *st,
                                double ctsSnr, FullWifiMode ctsMode, double rtsSnr)
{
}
void
FullAmrrWifiManager::DoReportDataOk (FullWifiRemoteStation *st,
                                 double ackSnr, FullWifiMode ackMode, double dataSnr)
{
  FullAmrrWifiRemoteStation *station = (FullAmrrWifiRemoteStation *)st;
  station->m_retry = 0;
  station->m_tx_ok++;
}
void
FullAmrrWifiManager::DoReportFinalRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullAmrrWifiManager::DoReportFinalDataFailed (FullWifiRemoteStation *st)
{
  FullAmrrWifiRemoteStation *station = (FullAmrrWifiRemoteStation *)st;
  station->m_retry = 0;
  station->m_tx_err++;
}
bool
FullAmrrWifiManager::IsMinRate (FullAmrrWifiRemoteStation *station) const
{
  return (station->m_txrate == 0);
}
bool
FullAmrrWifiManager::IsMaxRate (FullAmrrWifiRemoteStation *station) const
{
  NS_ASSERT (station->m_txrate + 1 <= GetNSupported (station));
  return (station->m_txrate + 1 == GetNSupported (station));
}
bool
FullAmrrWifiManager::IsSuccess (FullAmrrWifiRemoteStation *station) const
{
  return (station->m_tx_retr + station->m_tx_err) < station->m_tx_ok * m_successRatio;
}
bool
FullAmrrWifiManager::IsFailure (FullAmrrWifiRemoteStation *station) const
{
  return (station->m_tx_retr + station->m_tx_err) > station->m_tx_ok * m_failureRatio;
}
bool
FullAmrrWifiManager::IsEnough (FullAmrrWifiRemoteStation *station) const
{
  return (station->m_tx_retr + station->m_tx_err + station->m_tx_ok) > 10;
}
void
FullAmrrWifiManager::ResetCnt (FullAmrrWifiRemoteStation *station)
{
  station->m_tx_ok = 0;
  station->m_tx_err = 0;
  station->m_tx_retr = 0;
}
void
FullAmrrWifiManager::IncreaseRate (FullAmrrWifiRemoteStation *station)
{
  station->m_txrate++;
  NS_ASSERT (station->m_txrate < GetNSupported (station));
}
void
FullAmrrWifiManager::DecreaseRate (FullAmrrWifiRemoteStation *station)
{
  station->m_txrate--;
}

void
FullAmrrWifiManager::UpdateMode (FullAmrrWifiRemoteStation *station)
{
  if (Simulator::Now () < station->m_nextModeUpdate)
    {
      return;
    }
  station->m_nextModeUpdate = Simulator::Now () + m_updatePeriod;
  NS_LOG_DEBUG ("Update");

  bool needChange = false;

  if (IsSuccess (station) && IsEnough (station))
    {
      station->m_success++;
      NS_LOG_DEBUG ("++ success=" << station->m_success << " successThreshold=" << station->m_successThreshold <<
                    " tx_ok=" << station->m_tx_ok << " tx_err=" << station->m_tx_err << " tx_retr=" << station->m_tx_retr <<
                    " rate=" << station->m_txrate << " n-supported-rates=" << GetNSupported (station));
      if (station->m_success >= station->m_successThreshold
          && !IsMaxRate (station))
        {
          station->m_recovery = true;
          station->m_success = 0;
          IncreaseRate (station);
          needChange = true;
        }
      else
        {
          station->m_recovery = false;
        }
    }
  else if (IsFailure (station))
    {
      station->m_success = 0;
      NS_LOG_DEBUG ("-- success=" << station->m_success << " successThreshold=" << station->m_successThreshold <<
                    " tx_ok=" << station->m_tx_ok << " tx_err=" << station->m_tx_err << " tx_retr=" << station->m_tx_retr <<
                    " rate=" << station->m_txrate << " n-supported-rates=" << GetNSupported (station));
      if (!IsMinRate (station))
        {
          if (station->m_recovery)
            {
              station->m_successThreshold *= 2;
              station->m_successThreshold = std::min (station->m_successThreshold,
                                                      m_maxSuccessThreshold);
            }
          else
            {
              station->m_successThreshold = m_minSuccessThreshold;
            }
          station->m_recovery = false;
          DecreaseRate (station);
          needChange = true;
        }
      else
        {
          station->m_recovery = false;
        }
    }
  if (IsEnough (station) || needChange)
    {
      NS_LOG_DEBUG ("Reset");
      ResetCnt (station);
    }
}
FullWifiMode
FullAmrrWifiManager::DoGetDataMode (FullWifiRemoteStation *st, uint32_t size)
{
  FullAmrrWifiRemoteStation *station = (FullAmrrWifiRemoteStation *)st;
  UpdateMode (station);
  NS_ASSERT (station->m_txrate < GetNSupported (station));
  uint32_t rateIndex;
  if (station->m_retry < 1)
    {
      rateIndex = station->m_txrate;
    }
  else if (station->m_retry < 2)
    {
      if (station->m_txrate > 0)
        {
          rateIndex = station->m_txrate - 1;
        }
      else
        {
          rateIndex = station->m_txrate;
        }
    }
  else if (station->m_retry < 3)
    {
      if (station->m_txrate > 1)
        {
          rateIndex = station->m_txrate - 2;
        }
      else
        {
          rateIndex = station->m_txrate;
        }
    }
  else
    {
      if (station->m_txrate > 2)
        {
          rateIndex = station->m_txrate - 3;
        }
      else
        {
          rateIndex = station->m_txrate;
        }
    }

  return GetSupported (station, rateIndex);
}
FullWifiMode
FullAmrrWifiManager::DoGetRtsMode (FullWifiRemoteStation *st)
{
  FullAmrrWifiRemoteStation *station = (FullAmrrWifiRemoteStation *)st;
  UpdateMode (station);
  // XXX: can we implement something smarter ?
  return GetSupported (station, 0);
}


bool
FullAmrrWifiManager::IsLowLatency (void) const
{
  return true;
}

} // namespace ns3
