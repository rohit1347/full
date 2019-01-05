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
#include "full-interference-helper.h"
#include "full-wifi-phy.h"
#include "full-error-rate-model.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("FullInterferenceHelper");

namespace ns3 {

/****************************************************************
 *       Phy event class
 ****************************************************************/

FullInterferenceHelper::Event::Event (uint32_t size, FullWifiMode payloadMode,
                                  enum FullWifiPreamble preamble,
                                  Time duration, double rxPower)
  : m_size (size),
    m_payloadMode (payloadMode),
    m_preamble (preamble),
    m_startTime (Simulator::Now ()),
    m_endTime (m_startTime + duration),
    m_rxPowerW (rxPower)
{
}
FullInterferenceHelper::Event::~Event ()
{
}

Time
FullInterferenceHelper::Event::GetDuration (void) const
{
  return m_endTime - m_startTime;
}
Time
FullInterferenceHelper::Event::GetStartTime (void) const
{
  return m_startTime;
}
Time
FullInterferenceHelper::Event::GetEndTime (void) const
{
  return m_endTime;
}
double
FullInterferenceHelper::Event::GetRxPowerW (void) const
{
  return m_rxPowerW;
}
uint32_t
FullInterferenceHelper::Event::GetSize (void) const
{
  return m_size;
}
FullWifiMode
FullInterferenceHelper::Event::GetPayloadMode (void) const
{
  return m_payloadMode;
}
enum FullWifiPreamble
FullInterferenceHelper::Event::GetPreambleType (void) const
{
  return m_preamble;
}

/****************************************************************
 *       Class which records SNIR change events for a
 *       short period of time.
 ****************************************************************/

FullInterferenceHelper::NiChange::NiChange (Time time, double delta)
  : m_time (time),
    m_delta (delta)
{
}
Time
FullInterferenceHelper::NiChange::GetTime (void) const
{
  return m_time;
}
double
FullInterferenceHelper::NiChange::GetDelta (void) const
{
  return m_delta;
}
bool
FullInterferenceHelper::NiChange::operator < (const FullInterferenceHelper::NiChange& o) const
{
  return (m_time < o.m_time);
}

/****************************************************************
 *       The actual InterferenceHelper
 ****************************************************************/

FullInterferenceHelper::FullInterferenceHelper ()
  : m_errorRateModel (0),
    m_firstPower (0.0),
    m_rxing (false)
{
}
FullInterferenceHelper::~FullInterferenceHelper ()
{
  EraseEvents ();
  m_errorRateModel = 0;
}

Ptr<FullInterferenceHelper::Event>
FullInterferenceHelper::Add (uint32_t size, FullWifiMode payloadMode,
                         enum FullWifiPreamble preamble,
                         Time duration, double rxPowerW)
{
  Ptr<FullInterferenceHelper::Event> event;

  event = Create<FullInterferenceHelper::Event> (size,
                                             payloadMode,
                                             preamble,
                                             duration,
                                             rxPowerW);
  AppendEvent (event);
  return event;
}


void
FullInterferenceHelper::SetNoiseFigure (double value)
{
  m_noiseFigure = value;
}

double
FullInterferenceHelper::GetNoiseFigure (void) const
{
  return m_noiseFigure;
}

void
FullInterferenceHelper::SetErrorRateModel (Ptr<FullErrorRateModel> rate)
{
  m_errorRateModel = rate;
}

Ptr<FullErrorRateModel>
FullInterferenceHelper::GetErrorRateModel (void) const
{
  return m_errorRateModel;
}

double
FullInterferenceHelper::GetFirstPower (void) const
{
  return m_firstPower;
}

double
FullInterferenceHelper::GetCurrentEnergyPower (void)
{
  Time now = Simulator::Now ();
  double noiseInterferenceW = 0.0;
  noiseInterferenceW = m_firstPower;
  NiChanges::iterator nowIterator = GetPosition (now);
  for (NiChanges::const_iterator i = m_niChanges.begin (); i != nowIterator; i++)
    {
      noiseInterferenceW += i->GetDelta ();
    }
  return noiseInterferenceW;
}

Time
FullInterferenceHelper::GetEnergyDuration (double energyW)
{
  Time now = Simulator::Now ();
  double noiseInterferenceW = 0.0;
  Time end = now;
  noiseInterferenceW = m_firstPower;
  for (NiChanges::const_iterator i = m_niChanges.begin (); i != m_niChanges.end (); i++)
    {
      noiseInterferenceW += i->GetDelta ();
      end = i->GetTime ();
      if (end < now)
        {
          continue;
        }
      if (noiseInterferenceW < energyW)
        {
          break;
        }
    }
  return end > now ? end - now : MicroSeconds (0);
}

void
FullInterferenceHelper::AppendEvent (Ptr<FullInterferenceHelper::Event> event)
{
  Time now = Simulator::Now ();
  if (!m_rxing)
    {
      NiChanges::iterator nowIterator = GetPosition (now);
      for (NiChanges::iterator i = m_niChanges.begin (); i != nowIterator; i++)
        {
          m_firstPower += i->GetDelta ();
        }
      m_niChanges.erase (m_niChanges.begin (), nowIterator);
      m_niChanges.insert (m_niChanges.begin (), NiChange (event->GetStartTime (), event->GetRxPowerW ()));
    }
  else
    {
      AddNiChangeEvent (NiChange (event->GetStartTime (), event->GetRxPowerW ()));
    }
  AddNiChangeEvent (NiChange (event->GetEndTime (), -event->GetRxPowerW ()));

}


double
FullInterferenceHelper::CalculateSnr (double signal, double noiseInterference, FullWifiMode mode) const
{
  // thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  // Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * mode.GetBandwidth ();
  // receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = m_noiseFigure * Nt;
  double noise = noiseFloor + noiseInterference;
  double snr = signal / noise;
  return snr;
}

double
FullInterferenceHelper::CalculateNoiseInterferenceW (Ptr<FullInterferenceHelper::Event> event, NiChanges *ni) const
{
  double noiseInterference = m_firstPower;
  NS_ASSERT (m_rxing);
  for (NiChanges::const_iterator i = m_niChanges.begin () + 1; i != m_niChanges.end (); i++)
    {
      if ((event->GetEndTime () == i->GetTime ()) && event->GetRxPowerW () == -i->GetDelta ())
        {
          break;
        }
      ni->push_back (*i);
    }
  ni->insert (ni->begin (), NiChange (event->GetStartTime (), noiseInterference));
  ni->push_back (NiChange (event->GetEndTime (), 0));
  return noiseInterference;
}

double
FullInterferenceHelper::CalculateChunkSuccessRate (double snir, Time duration, FullWifiMode mode) const
{
  if (duration == NanoSeconds (0))
    {
      return 1.0;
    }
  uint32_t rate = mode.GetPhyRate ();
  uint64_t nbits = (uint64_t)(rate * duration.GetSeconds ());
  double csr = m_errorRateModel->GetChunkSuccessRate (mode, snir, (uint32_t)nbits);
  return csr;
}

double
FullInterferenceHelper::CalculatePer (Ptr<const FullInterferenceHelper::Event> event, NiChanges *ni) const
{
  double psr = 1.0; /* Packet Success Rate */
  NiChanges::iterator j = ni->begin ();
  Time previous = (*j).GetTime ();
  FullWifiMode payloadMode = event->GetPayloadMode ();
  FullWifiPreamble preamble = event->GetPreambleType ();
  FullWifiMode headerMode = FullWifiPhy::GetPlcpHeaderMode (payloadMode, preamble);
  Time plcpHeaderStart = (*j).GetTime () + MicroSeconds (FullWifiPhy::GetPlcpPreambleDurationMicroSeconds (payloadMode, preamble));
  Time plcpPayloadStart = plcpHeaderStart + MicroSeconds (FullWifiPhy::GetPlcpHeaderDurationMicroSeconds (payloadMode, preamble));
  double noiseInterferenceW = (*j).GetDelta ();
  double powerW = event->GetRxPowerW ();

  j++;
  while (ni->end () != j)
    {
      Time current = (*j).GetTime ();
      NS_ASSERT (current >= previous);

      if (previous >= plcpPayloadStart)
        {
          psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                          noiseInterferenceW,
                                                          payloadMode),
                                            current - previous,
                                            payloadMode);
        }
      else if (previous >= plcpHeaderStart)
        {
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              headerMode),
                                                plcpPayloadStart - previous,
                                                headerMode);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              payloadMode),
                                                current - plcpPayloadStart,
                                                payloadMode);
            }
          else
            {
              NS_ASSERT (current >= plcpHeaderStart);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              headerMode),
                                                current - previous,
                                                headerMode);
            }
        }
      else
        {
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              headerMode),
                                                plcpPayloadStart - plcpHeaderStart,
                                                headerMode);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              payloadMode),
                                                current - plcpPayloadStart,
                                                payloadMode);
            }
          else if (current >= plcpHeaderStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              headerMode),
                                                current - plcpHeaderStart,
                                                headerMode);
            }
        }

      noiseInterferenceW += (*j).GetDelta ();
      previous = (*j).GetTime ();
      j++;
    }

  double per = 1 - psr;
  return per;
}


struct FullInterferenceHelper::SnrPer
FullInterferenceHelper::CalculateSnrPer (Ptr<FullInterferenceHelper::Event> event)
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetPayloadMode ());

  /* calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculatePer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

void
FullInterferenceHelper::EraseEvents (void)
{
  m_niChanges.clear ();
  m_rxing = false;
  m_firstPower = 0.0;
}
FullInterferenceHelper::NiChanges::iterator
FullInterferenceHelper::GetPosition (Time moment)
{
  return std::upper_bound (m_niChanges.begin (), m_niChanges.end (), NiChange (moment, 0));

}
void
FullInterferenceHelper::AddNiChangeEvent (NiChange change)
{
  m_niChanges.insert (GetPosition (change.GetTime ()), change);
}
void
FullInterferenceHelper::NotifyRxStart ()
{
  m_rxing = true;
}
void
FullInterferenceHelper::NotifyRxEnd ()
{
  m_rxing = false;
}
} // namespace ns3
