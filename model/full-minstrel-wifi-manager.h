/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Duy Nguyen
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
 * Author: Duy Nguyen <duy@soe.ucsc.edu>
 */



#ifndef FULL_MINSTREL_WIFI_MANAGER_H
#define FULL_MINSTREL_WIFI_MANAGER_H

#include "full-wifi-remote-station-manager.h"
#include "full-wifi-mode.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

struct FullMinstrelWifiRemoteStation;

/**
 * A struct to contain all information related to a data rate
 */
struct FullRateInfo
{
  /**
   * Perfect transmission time calculation, or frame calculation
   * Given a bit rate and a packet length n bytes
   */
  Time perfectTxTime;


  uint32_t retryCount;  ///< retry limit
  uint32_t adjustedRetryCount;  ///< adjust the retry limit for this rate
  uint32_t numRateAttempt;  ///< how many number of attempts so far
  uint32_t numRateSuccess;    ///< number of successful pkts
  uint32_t prob;  ///< (# pkts success )/(# total pkts)

  /**
   * EWMA calculation
   * ewma_prob =[prob *(100 - ewma_level) + (ewma_prob_old * ewma_level)]/100
   */
  uint32_t ewmaProb;

  uint32_t prevNumRateAttempt;  ///< from last rate
  uint32_t prevNumRateSuccess;  ///< from last rate
  uint64_t successHist;  ///< aggregate of all successes
  uint64_t attemptHist;  ///< aggregate of all attempts
  uint32_t throughput;  ///< throughput of a rate
};

/**
 * Data structure for a Minstrel Rate table
 * A vector of a struct RateInfo
 */
typedef std::vector<struct FullRateInfo> FullMinstrelRate;

/**
 * Data structure for a Sample Rate table
 * A vector of a vector uint32_t
 */
typedef std::vector<std::vector<uint32_t> > FullSampleRate;


/**
 * \author Duy Nguyen
 * \brief Implementation of Minstrel Rate Control Algorithm
 * \ingroup wifi
 *
 * Porting Minstrel from Madwifi and Linux Kernel
 * http://linuxwireless.org/en/developers/Documentation/mac80211/RateControl/minstrel
 */
class FullMinstrelWifiManager : public FullWifiRemoteStationManager
{

public:
  static TypeId GetTypeId (void);
  FullMinstrelWifiManager ();
  virtual ~FullMinstrelWifiManager ();

  virtual void SetupPhy (Ptr<FullWifiPhy> phy);

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

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

  /// for estimating the TxTime of a packet with a given mode
  Time GetCalcTxTime (FullWifiMode mode) const;
  void AddCalcTxTime (FullWifiMode mode, Time t);

  /// update the number of retries and reset accordingly
  void UpdateRetry (FullMinstrelWifiRemoteStation *station);

  /// getting the next sample from Sample Table
  uint32_t GetNextSample (FullMinstrelWifiRemoteStation *station);

  /// find a rate to use from Minstrel Table
  uint32_t FindRate (FullMinstrelWifiRemoteStation *station);

  /// updating the Minstrel Table every 1/10 seconds
  void UpdateStats (FullMinstrelWifiRemoteStation *station);

  /// initialize Minstrel Table
  void RateInit (FullMinstrelWifiRemoteStation *station);

  /// initialize Sample Table
  void InitSampleTable (FullMinstrelWifiRemoteStation *station);

  /// printing Sample Table
  void PrintSampleTable (FullMinstrelWifiRemoteStation *station);

  /// printing Minstrel Table
  void PrintTable (FullMinstrelWifiRemoteStation *station);

  void CheckInit (FullMinstrelWifiRemoteStation *station);  ///< check for initializations


  typedef std::vector<std::pair<Time,FullWifiMode> > TxTime;
  FullMinstrelRate m_minstrelTable;  ///< minstrel table
  FullSampleRate m_sampleTable;  ///< sample table


  TxTime m_calcTxTime;  ///< to hold all the calculated TxTime for all modes
  Time m_updateStats;  ///< how frequent do we calculate the stats(1/10 seconds)
  double m_lookAroundRate;  ///< the % to try other rates than our current rate
  double m_ewmaLevel;  ///< exponential weighted moving average
  uint32_t m_segmentSize;  ///< largest allowable segment size
  uint32_t m_sampleCol;  ///< number of sample columns
  uint32_t m_pktLen;  ///< packet length used  for calculate mode TxTime
  uint32_t m_nsupported;  ///< modes supported

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

} // namespace ns3

#endif /* FULL_MINSTREL_WIFI_MANAGER_H */
