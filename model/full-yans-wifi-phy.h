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

#ifndef FULL_YANS_WIFI_PHY_H
#define FULL_YANS_WIFI_PHY_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "full-wifi-phy.h"
#include "full-wifi-mode.h"
#include "full-wifi-preamble.h"
#include "full-wifi-phy-standard.h"
#include "full-interference-helper.h"


namespace ns3 {

class FullYansWifiChannel;
class FullWifiPhyStateHelper;


/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a model of 802.11a. The model
 * implemented here is based on the model described
 * in "Yet Another Network Simulator",
 * (http://cutebugs.net/files/wns2-yans.pdf).
 *
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::PropagationLossModel
 * and ns3::PropagationDelayModel classes, both of which are
 * members of the ns3::YansWifiChannel class.
 */
class FullYansWifiPhy : public FullWifiPhy
{
public:
  static TypeId GetTypeId (void);

  FullYansWifiPhy ();
  virtual ~FullYansWifiPhy ();

  void SetChannel (Ptr<FullYansWifiChannel> channel);

  /**
   * \brief Set channel number.
   *
   * Channel center frequency = Channel starting frequency + 5 MHz * (nch - 1)
   *
   * where Starting channel frequency is standard-dependent, see SetStandard()
   * as defined in IEEE 802.11-2007 17.3.8.3.2.
   *
   * YansWifiPhy can switch among different channels. Basically, YansWifiPhy
   * has a private attribute m_channelNumber that identifies the channel the
   * PHY operates on. Channel switching cannot interrupt an ongoing transmission.
   * When PHY is in TX state, the channel switching is postponed until the end
   * of the current transmission. When the PHY is in RX state, the channel
   * switching causes the drop of the synchronized packet.
   */
  void SetChannelNumber (uint16_t id);
  /// Return current channel number, see SetChannelNumber()
  uint16_t GetChannelNumber () const;
  /// Return current center channel frequency in MHz, see SetChannelNumber()
  double GetChannelFrequencyMhz () const;

  void StartReceivePacket (Ptr<Packet> packet,
                           double rxPowerDbm,
                           FullWifiMode mode,
                           FullWifiPreamble preamble);

  void SetRxNoiseFigure (double noiseFigureDb);
  void SetTxPowerStart (double start);
  void SetTxPowerEnd (double end);
  void SetNTxPower (uint32_t n);
  void SetTxGain (double gain);
  void SetRxGain (double gain);
  void SetEdThreshold (double threshold);
  void SetCcaMode1Threshold (double threshold);
  void SetErrorRateModel (Ptr<FullErrorRateModel> rate);
  void SetDevice (Ptr<Object> device);
  void SetMobility (Ptr<Object> mobility);
  double GetRxNoiseFigure (void) const;
  double GetTxGain (void) const;
  double GetRxGain (void) const;
  double GetEdThreshold (void) const;
  double GetCcaMode1Threshold (void) const;
  Ptr<FullErrorRateModel> GetErrorRateModel (void) const;
  Ptr<Object> GetDevice (void) const;
  Ptr<Object> GetMobility (void);

  void SetEnableCaptureEffect (bool cap);
  bool GetEnableCaptureEffect (void) const;
  void SetCaptureEffectThreshold (double th);
  double GetCaptureEffectThreshold (void) const;




  virtual double GetTxPowerStart (void) const;
  virtual double GetTxPowerEnd (void) const;
  virtual uint32_t GetNTxPower (void) const;
  virtual void SetReceiveOkCallback (FullWifiPhy::RxOkCallback callback);
  virtual void SetReceiveErrorCallback (FullWifiPhy::RxErrorCallback callback);
  virtual void SendPacket (Ptr<const Packet> packet, FullWifiMode mode, enum FullWifiPreamble preamble, uint8_t txPowerLevel);
  virtual void RegisterListener (FullWifiPhyListener *listener);
  virtual bool IsStateCcaBusy (void);
  virtual bool IsRxStateIdle (void);
  virtual bool IsTxStateIdle (void);
  virtual bool IsRxStateBusy (void);
  virtual bool IsTxStateBusy (void);
  virtual bool IsStateRx (void);
  virtual bool IsStateTx (void);
  virtual bool IsStateSwitching (void);
  virtual Time GetRxStateDuration (void);
  virtual Time GetTxStateDuration (void);
  virtual Time GetTxDelayUntilIdle (void);
  virtual Time GetRxDelayUntilIdle (void);
  virtual Time GetLastRxStartTime (void) const;
  virtual uint32_t GetNModes (void) const;
  virtual FullWifiMode GetMode (uint32_t mode) const;
  virtual double CalculateSnr (FullWifiMode txMode, double ber) const;
  virtual Ptr<FullWifiChannel> GetChannel (void) const;
  virtual void ConfigureStandard (enum FullWifiPhyStandard standard);

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
  FullYansWifiPhy (const FullYansWifiPhy &o);
  virtual void DoDispose (void);
  void Configure80211a (void);
  void Configure80211b (void);
  void Configure80211g (void);
  void Configure80211_10Mhz (void);
  void Configure80211_5Mhz ();
  void ConfigureHolland (void);
  void Configure80211p_CCH (void);
  void Configure80211p_SCH (void);
  double GetEdThresholdW (void) const;
  double DbmToW (double dbm) const;
  double DbToRatio (double db) const;
  double WToDbm (double w) const;
  double RatioToDb (double ratio) const;
  double GetPowerDbm (uint8_t power) const;
  void EndReceive (Ptr<Packet> packet, Ptr<FullInterferenceHelper::Event> event);

private:
  double   m_edThresholdW;
  double   m_ccaMode1ThresholdW;
  double   m_txGainDb;
  double   m_rxGainDb;
  double   m_txPowerBaseDbm;
  double   m_txPowerEndDbm;
  uint32_t m_nTxPower;

  Ptr<FullYansWifiChannel> m_channel;
  uint16_t m_channelNumber;
  Ptr<Object> m_device;
  Ptr<Object> m_mobility;

  /**
   * This vector holds the set of transmission modes that this
   * WifiPhy(-derived class) can support. In conversation we call this
   * the DeviceRateSet (not a term you'll find in the standard), and
   * it is a superset of standard-defined parameters such as the
   * OperationalRateSet, and the BSSBasicRateSet (which, themselves,
   * have a superset/subset relationship).
   *
   * Mandatory rates relevant to this WifiPhy can be found by
   * iterating over this vector looking for WifiMode objects for which
   * WifiMode::IsMandatory() is true.
   *
   * A quick note is appropriate here (well, here is as good a place
   * as any I can find)...
   *
   * In the standard there is no text that explicitly precludes
   * production of a device that does not support some rates that are
   * mandatory (according to the standard) for PHYs that the device
   * happens to fully or partially support.
   *
   * This approach is taken by some devices which choose to only support,
   * for example, 6 and 9 Mbps ERP-OFDM rates for cost and power
   * consumption reasons (i.e., these devices don't need to be designed
   * for and waste current on the increased linearity requirement of
   * higher-order constellations when 6 and 9 Mbps more than meet their
   * data requirements). The wording of the standard allows such devices
   * to have an OperationalRateSet which includes 6 and 9 Mbps ERP-OFDM
   * rates, despite 12 and 24 Mbps being "mandatory" rates for the
   * ERP-OFDM PHY.
   *
   * Now this doesn't actually have any impact on code, yet. It is,
   * however, something that we need to keep in mind for the
   * future. Basically, the key point is that we can't be making
   * assumptions like "the Operational Rate Set will contain all the
   * mandatory rates".
   */
  FullWifiModeList m_deviceRateSet;

  EventId m_endRxEvent;
  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_random;
  /// Standard-dependent center frequency of 0-th channel, MHz
  double m_channelStartingFrequency;
  Ptr<FullWifiPhyStateHelper> m_receiveState;
  Ptr<FullWifiPhyStateHelper> m_sendState;
  FullInterferenceHelper m_interference;
  Time m_channelSwitchDelay;

  //enable the capture effect
  bool m_enableCaptureEffect;
  //the threshold for which the new arrived packet's power should be
  // higher than the current receiving packet
  double m_captureEffectThreshold;

};

} // namespace ns3


#endif /* FULL_YANS_WIFI_PHY_H */
