/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005,2006 INRIA
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

#include "full-aarfcd-wifi-manager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include <algorithm>

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

NS_LOG_COMPONENT_DEFINE ("FullAarfcd");

namespace ns3 {

struct FullAarfcdWifiRemoteStation : public FullWifiRemoteStation
{
  uint32_t m_timer;
  uint32_t m_success;
  uint32_t m_failed;
  bool m_recovery;
  bool m_justModifyRate;
  uint32_t m_retry;

  uint32_t m_successThreshold;
  uint32_t m_timerTimeout;

  uint32_t m_rate;
  bool m_rtsOn;
  uint32_t m_rtsWnd;
  uint32_t m_rtsCounter;
  bool m_haveASuccess;
};

NS_OBJECT_ENSURE_REGISTERED (FullAarfcdWifiManager);

TypeId
FullAarfcdWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullAarfcdWifiManager")
    .SetParent<FullWifiRemoteStationManager> ()
    .AddConstructor<FullAarfcdWifiManager> ()
    .AddAttribute ("SuccessK", "Multiplication factor for the success threshold in the AARF algorithm.",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&FullAarfcdWifiManager::m_successK),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TimerK",
                   "Multiplication factor for the timer threshold in the AARF algorithm.",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&FullAarfcdWifiManager::m_timerK),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxSuccessThreshold",
                   "Maximum value of the success threshold in the AARF algorithm.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&FullAarfcdWifiManager::m_maxSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinTimerThreshold",
                   "The minimum value for the 'timer' threshold in the AARF algorithm.",
                   UintegerValue (15),
                   MakeUintegerAccessor (&FullAarfcdWifiManager::m_minTimerThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinSuccessThreshold",
                   "The minimum value for the success threshold in the AARF algorithm.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&FullAarfcdWifiManager::m_minSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinRtsWnd",
                   "Minimum value for Rts window of Aarf-CD",
                   UintegerValue (1),
                   MakeUintegerAccessor (&FullAarfcdWifiManager::m_minRtsWnd),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxRtsWnd",
                   "Maximum value for Rts window of Aarf-CD",
                   UintegerValue (40),
                   MakeUintegerAccessor (&FullAarfcdWifiManager::m_maxRtsWnd),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TurnOffRtsAfterRateDecrease",
                   "If true the RTS mechanism will be turned off when the rate will be decreased",
                   BooleanValue (true),
                   MakeBooleanAccessor (&FullAarfcdWifiManager::m_turnOffRtsAfterRateDecrease),
                   MakeBooleanChecker ())
    .AddAttribute ("TurnOnRtsAfterRateIncrease",
                   "If true the RTS mechanism will be turned on when the rate will be increased",
                   BooleanValue (true),
                   MakeBooleanAccessor (&FullAarfcdWifiManager::m_turnOnRtsAfterRateIncrease),
                   MakeBooleanChecker ())
  ;
  return tid;
}
FullAarfcdWifiManager::FullAarfcdWifiManager ()
  : FullWifiRemoteStationManager ()
{
}
FullAarfcdWifiManager::~FullAarfcdWifiManager ()
{
}
FullWifiRemoteStation *
FullAarfcdWifiManager::DoCreateStation (void) const
{
  FullAarfcdWifiRemoteStation *station = new FullAarfcdWifiRemoteStation ();

  // aarf fields below
  station->m_successThreshold = m_minSuccessThreshold;
  station->m_timerTimeout = m_minTimerThreshold;
  station->m_rate = 0;
  station->m_success = 0;
  station->m_failed = 0;
  station->m_recovery = false;
  station->m_retry = 0;
  station->m_timer = 0;

  // aarf-cd specific fields below
  station->m_rtsOn = false;
  station->m_rtsWnd = m_minRtsWnd;
  station->m_rtsCounter = 0;
  station->m_justModifyRate = true;
  station->m_haveASuccess = false;

  return station;
}

void
FullAarfcdWifiManager::DoReportRtsFailed (FullWifiRemoteStation *station)
{
}
/**
 * It is important to realize that "recovery" mode starts after failure of
 * the first transmission after a rate increase and ends at the first successful
 * transmission. Specifically, recovery mode transcends retransmissions boundaries.
 * Fundamentally, ARF handles each data transmission independently, whether it
 * is the initial transmission of a packet or the retransmission of a packet.
 * The fundamental reason for this is that there is a backoff between each data
 * transmission, be it an initial transmission or a retransmission.
 */
void
FullAarfcdWifiManager::DoReportDataFailed (FullWifiRemoteStation *st)
{

  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *)st;
  station->m_timer++;
  station->m_failed++;
  station->m_retry++;
  station->m_success = 0;

  if (!station->m_rtsOn)
    {
      TurnOnRts (station);
      if (!station->m_justModifyRate && !station->m_haveASuccess)
        {
          IncreaseRtsWnd (station);
        }
      else
        {
          ResetRtsWnd (station);
        }
      station->m_rtsCounter = station->m_rtsWnd;
      if (station->m_retry >= 2)
        {
          station->m_timer = 0;
        }
    }
  else if (station->m_recovery)
    {
      NS_ASSERT (station->m_retry >= 1);
      station->m_justModifyRate = false;
      station->m_rtsCounter = station->m_rtsWnd;
      if (station->m_retry == 1)
        {
          // need recovery fallback
          if (m_turnOffRtsAfterRateDecrease)
            {
              TurnOffRts (station);
            }
          station->m_justModifyRate = true;
          station->m_successThreshold = (int)(Min (station->m_successThreshold * m_successK,
                                                   m_maxSuccessThreshold));
          station->m_timerTimeout = (int)(Max (station->m_timerTimeout * m_timerK,
                                               m_minSuccessThreshold));
          if (station->m_rate != 0)
            {
              station->m_rate--;
            }
        }
      station->m_timer = 0;
    }
  else
    {
      NS_ASSERT (station->m_retry >= 1);
      station->m_justModifyRate = false;
      station->m_rtsCounter = station->m_rtsWnd;
      if (((station->m_retry - 1) % 2) == 1)
        {
          // need normal fallback
          if (m_turnOffRtsAfterRateDecrease)
            {
              TurnOffRts (station);
            }
          station->m_justModifyRate = true;
          station->m_timerTimeout = m_minTimerThreshold;
          station->m_successThreshold = m_minSuccessThreshold;
          if (station->m_rate != 0)
            {
              station->m_rate--;
            }
        }
      if (station->m_retry >= 2)
        {
          station->m_timer = 0;
        }
    }
  CheckRts (station);
}
void
FullAarfcdWifiManager::DoReportRxOk (FullWifiRemoteStation *station,
                                 double rxSnr, FullWifiMode txMode)
{
}
void
FullAarfcdWifiManager::DoReportRtsOk (FullWifiRemoteStation *st,
                                  double ctsSnr, FullWifiMode ctsMode, double rtsSnr)
{
  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *) st;
  NS_LOG_DEBUG ("station=" << station << " rts ok");
  station->m_rtsCounter--;
}
void
FullAarfcdWifiManager::DoReportDataOk (FullWifiRemoteStation *st,
                                   double ackSnr, FullWifiMode ackMode, double dataSnr)
{
  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *) st;
  station->m_timer++;
  station->m_success++;
  station->m_failed = 0;
  station->m_recovery = false;
  station->m_retry = 0;
  station->m_justModifyRate = false;
  station->m_haveASuccess = true;
  NS_LOG_DEBUG ("station=" << station << " data ok success=" << station->m_success << ", timer=" << station->m_timer);
  if ((station->m_success == station->m_successThreshold
       || station->m_timer == station->m_timerTimeout)
      && (station->m_rate < (GetNSupported (station) - 1)))
    {
      NS_LOG_DEBUG ("station=" << station << " inc rate");
      station->m_rate++;
      station->m_timer = 0;
      station->m_success = 0;
      station->m_recovery = true;
      station->m_justModifyRate = true;
      if (m_turnOnRtsAfterRateIncrease)
        {
          TurnOnRts (station);
          ResetRtsWnd (station);
          station->m_rtsCounter = station->m_rtsWnd;
        }
    }
  CheckRts (station);
}
void
FullAarfcdWifiManager::DoReportFinalRtsFailed (FullWifiRemoteStation *station)
{
}
void
FullAarfcdWifiManager::DoReportFinalDataFailed (FullWifiRemoteStation *station)
{
}

FullWifiMode
FullAarfcdWifiManager::DoGetDataMode (FullWifiRemoteStation *st, uint32_t size)
{
  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *) st;
  return GetSupported (station, station->m_rate);
}
FullWifiMode
FullAarfcdWifiManager::DoGetRtsMode (FullWifiRemoteStation *st)
{
  // XXX: we could/should implement the Aarf algorithm for
  // RTS only by picking a single rate within the BasicRateSet.
  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *) st;
  return GetSupported (station, 0);
}

bool
FullAarfcdWifiManager::DoNeedRts (FullWifiRemoteStation *st,
                              Ptr<const Packet> packet, bool normally)
{
  FullAarfcdWifiRemoteStation *station = (FullAarfcdWifiRemoteStation *) st;
  NS_LOG_INFO ("" << station << " rate=" << station->m_rate << " rts=" << (station->m_rtsOn ? "RTS" : "BASIC") <<
               " rtsCounter=" << station->m_rtsCounter);
  return station->m_rtsOn;
}

bool
FullAarfcdWifiManager::IsLowLatency (void) const
{
  return true;
}

void
FullAarfcdWifiManager::CheckRts (FullAarfcdWifiRemoteStation *station)
{
  if (station->m_rtsCounter == 0 && station->m_rtsOn)
    {
      TurnOffRts (station);
    }
}

void
FullAarfcdWifiManager::TurnOffRts (FullAarfcdWifiRemoteStation *station)
{
  station->m_rtsOn = false;
  station->m_haveASuccess = false;
}

void
FullAarfcdWifiManager::TurnOnRts (FullAarfcdWifiRemoteStation *station)
{
  station->m_rtsOn = true;
}

void
FullAarfcdWifiManager::IncreaseRtsWnd (FullAarfcdWifiRemoteStation *station)
{
  if (station->m_rtsWnd == m_maxRtsWnd)
    {
      return;
    }

  station->m_rtsWnd *= 2;
  if (station->m_rtsWnd > m_maxRtsWnd)
    {
      station->m_rtsWnd = m_maxRtsWnd;
    }
}

void
FullAarfcdWifiManager::ResetRtsWnd (FullAarfcdWifiRemoteStation *station)
{
  station->m_rtsWnd = m_minRtsWnd;
}


} // namespace ns3
