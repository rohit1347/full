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

#include "full-yans-wifi-phy.h"
#include "full-wifi-mac-header.h"
#include "full-yans-wifi-channel.h"
#include "full-wifi-mode.h"
#include "full-wifi-preamble.h"
#include "full-wifi-phy-state-helper.h"
#include "full-error-rate-model.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/pointer.h"
#include "ns3/net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("FullYansWifiPhy");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullYansWifiPhy);

TypeId
FullYansWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullYansWifiPhy")
    .SetParent<FullWifiPhy> ()
    .AddConstructor<FullYansWifiPhy> ()
    .AddAttribute ("EnergyDetectionThreshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to detect the signal.",
                   DoubleValue (-96.0),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetEdThreshold,
                                       &FullYansWifiPhy::GetEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaMode1Threshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to declare CCA BUSY state",
                   DoubleValue (-99.0),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetCcaMode1Threshold,
                                       &FullYansWifiPhy::GetCcaMode1Threshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxGain",
                   "Transmission gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetTxGain,
                                       &FullYansWifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxGain",
                   "Reception gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetRxGain,
                                       &FullYansWifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
   .AddAttribute ("CaptureEffectThreshold",
                  "capture effect threshold in dB",
                  DoubleValue (0),
                  MakeDoubleAccessor (&FullYansWifiPhy::SetCaptureEffectThreshold,
                                      &FullYansWifiPhy::GetCaptureEffectThreshold),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerStart and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&FullYansWifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TxPowerEnd",
                   "Maximum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetTxPowerEnd,
                                       &FullYansWifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerStart",
                   "Minimum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetTxPowerStart,
                                       &FullYansWifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\"."
                   " For",
                   DoubleValue (7),
                   MakeDoubleAccessor (&FullYansWifiPhy::SetRxNoiseFigure,
                                       &FullYansWifiPhy::GetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReceiveState", "The receive state of the PHY layer",
                   PointerValue (),
                   MakePointerAccessor (&FullYansWifiPhy::m_receiveState),
                   MakePointerChecker<FullWifiPhyStateHelper> ())
     .AddAttribute ("SendState", "The send state of the PHY layer",
                    PointerValue (),
                    MakePointerAccessor (&FullYansWifiPhy::m_sendState),
                    MakePointerChecker<FullWifiPhyStateHelper> ())
    .AddAttribute ("ChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies. NOTE: Unused now.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&FullYansWifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
     .AddAttribute ("EnableCaptureEffect",
                    "This Boolean attribute is set to enable capture effect",
                    BooleanValue (false),
                    MakeBooleanAccessor (&FullYansWifiPhy::SetEnableCaptureEffect,
                                         &FullYansWifiPhy::GetEnableCaptureEffect),
                    MakeBooleanChecker ())
    .AddAttribute ("ChannelNumber",
                   "Channel center frequency = Channel starting frequency + 5 MHz * (nch - 1)",
                   UintegerValue (1),
                   MakeUintegerAccessor (&FullYansWifiPhy::SetChannelNumber,
                                         &FullYansWifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint16_t> ())

  ;
  return tid;
}

FullYansWifiPhy::FullYansWifiPhy ()
  :  m_channelNumber (1),
    m_endRxEvent (),
    m_channelStartingFrequency (0)
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_receiveState = CreateObject<FullWifiPhyStateHelper> ();
  m_sendState = CreateObject<FullWifiPhyStateHelper> ();
}

FullYansWifiPhy::~FullYansWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
FullYansWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_deviceRateSet.clear ();
  m_device = 0;
  m_mobility = 0;
  m_receiveState = 0;
  m_sendState = 0;
}

void
FullYansWifiPhy::ConfigureStandard (enum FullWifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard)
    {
    case FULL_WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211_10MHZ:
      Configure80211_10Mhz ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211_5MHZ:
      Configure80211_5Mhz ();
      break;
    case FULL_WIFI_PHY_STANDARD_holland:
      ConfigureHolland ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211p_CCH:
      Configure80211p_CCH ();
      break;
    case FULL_WIFI_PHY_STANDARD_80211p_SCH:
      Configure80211p_SCH ();
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}


void
FullYansWifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
}
void
FullYansWifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}
void
FullYansWifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}
void
FullYansWifiPhy::SetNTxPower (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nTxPower = n;
}
void
FullYansWifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}
void
FullYansWifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}
void
FullYansWifiPhy::SetEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_edThresholdW = DbmToW (threshold);
}
void
FullYansWifiPhy::SetCcaMode1Threshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaMode1ThresholdW = DbmToW (threshold);
}
void
FullYansWifiPhy::SetErrorRateModel (Ptr<FullErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
}
void
FullYansWifiPhy::SetDevice (Ptr<Object> device)
{
  m_device = device;
}
void
FullYansWifiPhy::SetMobility (Ptr<Object> mobility)
{
  m_mobility = mobility;
}

void
FullYansWifiPhy::SetEnableCaptureEffect (bool cap)
{
  m_enableCaptureEffect = cap;
}
void
FullYansWifiPhy::SetCaptureEffectThreshold (double th)
{
  m_captureEffectThreshold = th;
}
double
FullYansWifiPhy::GetCaptureEffectThreshold (void) const
{
  return m_captureEffectThreshold;
}

bool
FullYansWifiPhy::GetEnableCaptureEffect (void) const
{
  return m_enableCaptureEffect;
}

double
FullYansWifiPhy::GetRxNoiseFigure (void) const
{
  return RatioToDb (m_interference.GetNoiseFigure ());
}
double
FullYansWifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}
double
FullYansWifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}
double
FullYansWifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}
double
FullYansWifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

double
FullYansWifiPhy::GetEdThreshold (void) const
{
  return WToDbm (m_edThresholdW);
}

double
FullYansWifiPhy::GetCcaMode1Threshold (void) const
{
  return WToDbm (m_ccaMode1ThresholdW);
}

Ptr<FullErrorRateModel>
FullYansWifiPhy::GetErrorRateModel (void) const
{
  return m_interference.GetErrorRateModel ();
}
Ptr<Object>
FullYansWifiPhy::GetDevice (void) const
{
  return m_device;
}
Ptr<Object>
FullYansWifiPhy::GetMobility (void)
{
  return m_mobility;
}

double
FullYansWifiPhy::CalculateSnr (FullWifiMode txMode, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txMode, ber);
}

Ptr<FullWifiChannel>
FullYansWifiPhy::GetChannel (void) const
{
  return m_channel;
}
void
FullYansWifiPhy::SetChannel (Ptr<FullYansWifiChannel> channel)
{
  m_channel = channel;
  m_channel->Add (this);
}

void
FullYansWifiPhy::SetChannelNumber (uint16_t nch)
{
  if (Simulator::Now () == Seconds (0))
    {
      // this is not channel switch, this is initialization
      NS_LOG_DEBUG ("start at channel " << nch);
      m_channelNumber = nch;
      return;
    }

  NS_ASSERT (!IsStateSwitching ());

  if (m_sendState->GetState () == FullYansWifiPhy::TX)
    {
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetTxDelayUntilIdle (), &FullYansWifiPhy::SetChannelNumber, this, nch);
      return;
    }

  switch (m_receiveState->GetState ())
    {
    case FullYansWifiPhy::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endRxEvent.Cancel ();
      goto switchChannel;
      break;
//    case FullYansWifiPhy::TX:
//      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
//      Simulator::Schedule (GetDelayUntilIdle (), &FullYansWifiPhy::SetChannelNumber, this, nch);
//      break;
    case FullYansWifiPhy::CCA_BUSY:
    case FullYansWifiPhy::IDLE:
      goto switchChannel;
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return;

switchChannel:

  NS_LOG_DEBUG ("switching channel " << m_channelNumber << " -> " << nch);
  m_receiveState->SwitchToChannelSwitching (m_channelSwitchDelay);
  m_sendState->SwitchToChannelSwitching (m_channelSwitchDelay);
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  m_channelNumber = nch;
}

uint16_t
FullYansWifiPhy::GetChannelNumber () const
{
  return m_channelNumber;
}

double
FullYansWifiPhy::GetChannelFrequencyMhz () const
{
  return m_channelStartingFrequency + 5 * GetChannelNumber ();
}

void
FullYansWifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_receiveState->SetReceiveOkCallback (callback);
}
void
FullYansWifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_receiveState->SetReceiveErrorCallback (callback);
}
void
FullYansWifiPhy::StartReceivePacket (Ptr<Packet> packet,
                                 double rxPowerDbm,
                                 FullWifiMode txMode,
                                 enum FullWifiPreamble preamble)
{
//  NS_LOG_FUNCTION (this << packet << rxPowerDbm << txMode << preamble);
  rxPowerDbm += m_rxGainDb;
  double rxPowerW = DbmToW (rxPowerDbm);
  Time rxDuration = CalculateTxDuration (packet->GetSize (), txMode, preamble);
  FullWifiMacHeader header;
  packet->PeekHeader (header);
  if (header.GetType () == FULL_WIFI_MAC_BUSY_TONE)
    {
      rxDuration = header.GetDuration ();
    }
  Time endRx = Simulator::Now () + rxDuration;
  double capRxW = DbmToW (rxPowerDbm - m_captureEffectThreshold);
  double noiseInterferenceW;

    NS_LOG_FUNCTION (this << packet << rxPowerDbm << txMode << "rxpower:"<<capRxW);

  Ptr<FullInterferenceHelper::Event> event;
//  event = m_interference.Add (packet->GetSize (),
//                              txMode,
//                              preamble,
//                              rxDuration,
//                              rxPowerW);

  switch (m_receiveState->GetState ())
    {
    case FullYansWifiPhy::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet);
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_receiveState->GetDelayUntilIdle ())
        {
          // that packet will be noise _after_ the completion of the
          // channel switching.
          event = m_interference.Add (packet->GetSize (),
                                      txMode,
                                      preamble,
                                      rxDuration,
                                      rxPowerW);
          goto maybeCcaBusy;
        }
      break;
    case FullYansWifiPhy::RX:    //FIXME: change this method to allow capture effect
      //capture effect enabled
    	NS_LOG_INFO("slefjw");
      noiseInterferenceW = m_interference.GetCurrentEnergyPower ();
      if ( m_enableCaptureEffect && (noiseInterferenceW < capRxW))
        {
          NS_LOG_DEBUG ("stronger signal captured:" << rxPowerW << " > " << m_interference.GetFirstPower ());
          m_receiveState->SwitchFromRxEndError(packet, 0);
          NotifyRxDrop (packet);
          m_interference.NotifyRxEnd ();
          m_endRxEvent.Cancel ();
          goto receivePacket;
        }

      NS_LOG_DEBUG ("drop packet because already in Rx (power=" <<
                    rxPowerW << "W)" << m_interference.GetFirstPower ());
      event = m_interference.Add (packet->GetSize (),
                                  txMode,
                                  preamble,
                                  rxDuration,
                                  rxPowerW);

      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_receiveState->GetDelayUntilIdle ())
        {
          // that packet will be noise _after_ the reception of the
          // currently-received packet.
          goto maybeCcaBusy;
        }
      break;
//    case FullYansWifiPhy::TX:
//      NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
//                    rxPowerW << "W)");
//      NotifyRxDrop (packet);
//      if (endRx > Simulator::Now () + m_receiveState->GetDelayUntilIdle ())
//        {
//          // that packet will be noise _after_ the transmission of the
//          // currently-transmitted packet.
//          goto maybeCcaBusy;
//        }
//      break;
    case FullYansWifiPhy::CCA_BUSY:
    case FullYansWifiPhy::IDLE:
      receivePacket:
      event = m_interference.Add (packet->GetSize (),
                                  txMode,
                                  preamble,
                                  rxDuration,
                                  rxPowerW);

      if (rxPowerW > m_edThresholdW)
        {
          NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
          // sync to signal
          m_receiveState->SwitchToRx (rxDuration, packet, txMode, preamble);
          NS_ASSERT (m_endRxEvent.IsExpired ());
          NotifyRxBegin (packet);
          m_interference.NotifyRxStart ();
          m_endRxEvent = Simulator::Schedule (rxDuration, &FullYansWifiPhy::EndReceive, this,
                                              packet,
                                              event);

          if ( m_enableCaptureEffect )
            {
               goto maybeCcaBusy;  // in case of capture effect, the new packet maybe a short one
            }
        }
      else
        {
          NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                        rxPowerW << "<" << m_edThresholdW << ")");
          NotifyRxDrop (packet);
          goto maybeCcaBusy;
        }
      break;

    default:
      NS_ASSERT (false);
      break;
    }

  return;

maybeCcaBusy:
  // We are here because we have received the first bit of a packet and we are
  // not going to be able to synchronize on it
  // In this model, CCA becomes busy when the aggregation of all signals as
  // tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_receiveState->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
FullYansWifiPhy::SendPacket (Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble, uint8_t txPower)
{
  NS_LOG_FUNCTION (this << packet << txMode << preamble << (uint32_t)txPower);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsability of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_sendState->IsStateTx () && !m_sendState->IsStateSwitching ());

  Time txDuration = CalculateTxDuration (packet->GetSize (), txMode, preamble);
  FullWifiMacHeader header;
  packet->PeekHeader (header);
  if (header.GetType () == FULL_WIFI_MAC_BUSY_TONE)
    {
      txDuration = header.GetDuration ();
    }
//  if (m_receiveState->IsStateRx ())
//    {
//      m_endRxEvent.Cancel ();
//      m_interference.NotifyRxEnd ();
//    }
  NotifyTxBegin (packet);
  uint32_t dataRate500KbpsUnits = txMode.GetDataRate () / 500000;
  bool isShortPreamble = (FULL_WIFI_PREAMBLE_SHORT == preamble);
  NotifyMonitorSniffTx (packet, (uint16_t)GetChannelFrequencyMhz (), GetChannelNumber (), dataRate500KbpsUnits, isShortPreamble);
  m_sendState->SwitchToTx (txDuration, packet, txMode, preamble, txPower);
  m_channel->Send (this, packet, GetPowerDbm (txPower) + m_txGainDb, txMode, preamble);
}

uint32_t
FullYansWifiPhy::GetNModes (void) const
{
  return m_deviceRateSet.size ();
}
FullWifiMode
FullYansWifiPhy::GetMode (uint32_t mode) const
{
  return m_deviceRateSet[mode];
}
uint32_t
FullYansWifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
FullYansWifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz

  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate54Mbps ());
}


void
FullYansWifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 2407; // 2.407 GHz

  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate11Mbps ());
}

void
FullYansWifiPhy::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 2407; // 2.407 GHz

  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate6Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate9Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetDsssRate11Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate12Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate18Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate24Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate36Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate48Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetErpOfdmRate54Mbps ());
}

void
FullYansWifiPhy::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
FullYansWifiPhy::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate3MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate9MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12MbpsBW5MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
}

void
FullYansWifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate54Mbps ());
}

void
FullYansWifiPhy::Configure80211p_CCH (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 802.11p works over the 5Ghz freq range

  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
FullYansWifiPhy::Configure80211p_SCH (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 802.11p works over the 5Ghz freq range

  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (FullWifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
FullYansWifiPhy::RegisterListener (FullWifiPhyListener *listener)
{
  m_receiveState->RegisterListener (listener);
  m_sendState->RegisterListener (listener);
}

bool
FullYansWifiPhy::IsStateCcaBusy (void)
{
  return m_receiveState->IsStateCcaBusy ();
}

bool
FullYansWifiPhy::IsRxStateIdle (void)
{
  return m_receiveState->IsStateIdle ();
}
bool
FullYansWifiPhy::IsTxStateIdle (void)
{
  return m_sendState->IsStateIdle ();
}
bool
FullYansWifiPhy::IsRxStateBusy (void)
{
  return m_receiveState->IsStateBusy ();
}
bool
FullYansWifiPhy::IsTxStateBusy (void)
{
  return m_sendState->IsStateBusy ();
}
bool
FullYansWifiPhy::IsStateRx (void)
{
  return m_receiveState->IsStateRx ();
}
bool
FullYansWifiPhy::IsStateTx (void)
{
  return m_sendState->IsStateTx ();
}
bool
FullYansWifiPhy::IsStateSwitching (void)
{
  return (m_receiveState->IsStateSwitching () || m_sendState->IsStateSwitching ());
}

Time
FullYansWifiPhy::GetRxStateDuration (void)
{
  return m_receiveState->GetStateDuration ();
}
Time
FullYansWifiPhy::GetTxStateDuration (void)
{
  return m_sendState->GetStateDuration ();
}
Time
FullYansWifiPhy::GetTxDelayUntilIdle (void)
{
  return m_sendState->GetDelayUntilIdle ();
}
Time
FullYansWifiPhy::GetRxDelayUntilIdle (void)
{
  return m_receiveState->GetDelayUntilIdle ();
}

Time
FullYansWifiPhy::GetLastRxStartTime (void) const
{
  return m_receiveState->GetLastRxStartTime ();
}

double
FullYansWifiPhy::DbToRatio (double dB) const
{
  double ratio = std::pow (10.0, dB / 10.0);
  return ratio;
}

double
FullYansWifiPhy::DbmToW (double dBm) const
{
  double mW = std::pow (10.0, dBm / 10.0);
  return mW / 1000.0;
}

double
FullYansWifiPhy::WToDbm (double w) const
{
  return 10.0 * std::log10 (w * 1000.0);
}

double
FullYansWifiPhy::RatioToDb (double ratio) const
{
  return 10.0 * std::log10 (ratio);
}

double
FullYansWifiPhy::GetEdThresholdW (void) const
{
  return m_edThresholdW;
}

double
FullYansWifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm, "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

void
FullYansWifiPhy::EndReceive (Ptr<Packet> packet, Ptr<FullInterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  struct FullInterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculateSnrPer (event);
  m_interference.NotifyRxEnd ();

  NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate ()) <<
                ", snr=" << snrPer.snr << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
  if (m_random->GetValue () > snrPer.per)
    {
      NotifyRxEnd (packet);
      uint32_t dataRate500KbpsUnits = event->GetPayloadMode ().GetDataRate () / 500000;
      bool isShortPreamble = (FULL_WIFI_PREAMBLE_SHORT == event->GetPreambleType ());
      double signalDbm = RatioToDb (event->GetRxPowerW ()) + 30;
      double noiseDbm = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
      NotifyMonitorSniffRx (packet, (uint16_t)GetChannelFrequencyMhz (), GetChannelNumber (), dataRate500KbpsUnits, isShortPreamble, signalDbm, noiseDbm);
      m_receiveState->SwitchFromRxEndOk (packet, snrPer.snr, event->GetPayloadMode (), event->GetPreambleType ());
    }
  else
    {
      /* failure. */
      NotifyRxDrop (packet);
      m_receiveState->SwitchFromRxEndError (packet, snrPer.snr);
    }
}

int64_t
FullYansWifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}
} // namespace ns3
