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
#ifndef FULL_INTERFERENCE_HELPER_H
#define FULL_INTERFERENCE_HELPER_H

#include <stdint.h>
#include <vector>
#include <list>
#include "full-wifi-mode.h"
#include "full-wifi-preamble.h"
#include "full-wifi-phy-standard.h"
#include "ns3/nstime.h"
#include "ns3/simple-ref-count.h"

namespace ns3 {

class FullErrorRateModel;

/**
 * \ingroup wifi
 * \brief handles interference calculations
 */
class FullInterferenceHelper
{
public:
  class Event : public SimpleRefCount<FullInterferenceHelper::Event>
  {
public:
    Event (uint32_t size, FullWifiMode payloadMode,
           enum FullWifiPreamble preamble,
           Time duration, double rxPower);
    ~Event ();

    Time GetDuration (void) const;
    Time GetStartTime (void) const;
    Time GetEndTime (void) const;
    double GetRxPowerW (void) const;
    uint32_t GetSize (void) const;
    FullWifiMode GetPayloadMode (void) const;
    enum FullWifiPreamble GetPreambleType (void) const;
private:
    uint32_t m_size;
    FullWifiMode m_payloadMode;
    enum FullWifiPreamble m_preamble;
    Time m_startTime;
    Time m_endTime;
    double m_rxPowerW;
  };
  struct SnrPer
  {
    double snr;
    double per;
  };

  FullInterferenceHelper ();
  ~FullInterferenceHelper ();

  void SetNoiseFigure (double value);
  void SetErrorRateModel (Ptr<FullErrorRateModel> rate);

  double GetNoiseFigure (void) const;
  Ptr<FullErrorRateModel> GetErrorRateModel (void) const;

  double GetFirstPower (void) const;

  /**
   * \param energyW the minimum energy (W) requested
   * \returns the expected amount of time the observed
   *          energy on the medium will be higher than
   *          the requested threshold.
   */
  Time GetEnergyDuration (double energyW);

  double GetCurrentEnergyPower (void);

  Ptr<FullInterferenceHelper::Event> Add (uint32_t size, FullWifiMode payloadMode,
                                      enum FullWifiPreamble preamble,
                                      Time duration, double rxPower);

  struct FullInterferenceHelper::SnrPer CalculateSnrPer (Ptr<FullInterferenceHelper::Event> event);
  void NotifyRxStart ();
  void NotifyRxEnd ();
  void EraseEvents (void);
private:
  class NiChange
  {
public:
    NiChange (Time time, double delta);
    Time GetTime (void) const;
    double GetDelta (void) const;
    bool operator < (const NiChange& o) const;
private:
    Time m_time;
    double m_delta;
  };
  typedef std::vector <NiChange> NiChanges;
  typedef std::list<Ptr<Event> > Events;

  FullInterferenceHelper (const FullInterferenceHelper &o);
  FullInterferenceHelper &operator = (const FullInterferenceHelper &o);
  void AppendEvent (Ptr<Event> event);
  double CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const;
  double CalculateSnr (double signal, double noiseInterference, FullWifiMode mode) const;
  double CalculateChunkSuccessRate (double snir, Time delay, FullWifiMode mode) const;
  double CalculatePer (Ptr<const Event> event, NiChanges *ni) const;

  double m_noiseFigure; /**< noise figure (linear) */
  Ptr<FullErrorRateModel> m_errorRateModel;
  /// Experimental: needed for energy duration calculation
  NiChanges m_niChanges;
  double m_firstPower;
  bool m_rxing;
  /// Returns an iterator to the first nichange, which is later than moment
  NiChanges::iterator GetPosition (Time moment);
  void AddNiChangeEvent (NiChange change);
};

} // namespace ns3

#endif /* FULL_INTERFERENCE_HELPER_H */
