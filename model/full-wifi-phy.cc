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

#include "full-wifi-phy.h"
#include "full-wifi-mode.h"
#include "full-wifi-channel.h"
#include "full-wifi-preamble.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("FullWifiPhy");

namespace ns3 {

/****************************************************************
 *       This destructor is needed.
 ****************************************************************/

FullWifiPhyListener::~FullWifiPhyListener ()
{
}

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED (FullWifiPhy);

TypeId
FullWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiPhy")
    .SetParent<Object> ()
    .AddTraceSource ("PhyTxBegin",
                     "Trace source indicating a packet has begun transmitting over the channel medium",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyTxBeginTrace))
    .AddTraceSource ("PhyTxEnd",
                     "Trace source indicating a packet has been completely transmitted over the channel. NOTE: the only official WifiPhy implementation available to this date (YansWifiPhy) never fires this trace source.",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyTxEndTrace))
    .AddTraceSource ("PhyTxDrop",
                     "Trace source indicating a packet has been dropped by the device during transmission",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyTxDropTrace))
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet has begun being received from the channel medium by the device",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyRxBeginTrace))
    .AddTraceSource ("PhyRxEnd",
                     "Trace source indicating a packet has been completely received from the channel medium by the device",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyRxEndTrace))
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped by the device during reception",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyRxDropTrace))
    .AddTraceSource ("MonitorSnifferRx",
                     "Trace source simulating a wifi device in monitor mode sniffing all received frames",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyMonitorSniffRxTrace))
    .AddTraceSource ("MonitorSnifferTx",
                     "Trace source simulating the capability of a wifi device in monitor mode to sniff all frames being transmitted",
                     MakeTraceSourceAccessor (&FullWifiPhy::m_phyMonitorSniffTxTrace))
  ;
  return tid;
}

FullWifiPhy::FullWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

FullWifiPhy::~FullWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}


FullWifiMode
FullWifiPhy::GetPlcpHeaderMode (FullWifiMode payloadMode, FullWifiPreamble preamble)
{
  switch (payloadMode.GetModulationClass ())
    {
    case FULL_WIFI_MOD_CLASS_OFDM:
      {
        switch (payloadMode.GetBandwidth ())
          {
          case 5000000:
            return FullWifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
          case 10000000:
            return FullWifiPhy::GetOfdmRate3MbpsBW10MHz ();
          default:
            // IEEE Std 802.11-2007, 17.3.2
            // actually this is only the first part of the PlcpHeader,
            // because the last 16 bits of the PlcpHeader are using the
            // same mode of the payload
            return FullWifiPhy::GetOfdmRate6Mbps ();
          }
      }

    case FULL_WIFI_MOD_CLASS_ERP_OFDM:
      return FullWifiPhy::GetErpOfdmRate6Mbps ();

    case FULL_WIFI_MOD_CLASS_DSSS:
      if (preamble == FULL_WIFI_PREAMBLE_LONG)
        {
          // IEEE Std 802.11-2007, sections 15.2.3 and 18.2.2.1
          return FullWifiPhy::GetDsssRate1Mbps ();
        }
      else  //  WIFI_PREAMBLE_SHORT
        {
          // IEEE Std 802.11-2007, section 18.2.2.2
          return FullWifiPhy::GetDsssRate2Mbps ();
        }

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return FullWifiMode ();
    }
}

uint32_t
FullWifiPhy::GetPlcpHeaderDurationMicroSeconds (FullWifiMode payloadMode, FullWifiPreamble preamble)
{
  switch (payloadMode.GetModulationClass ())
    {
    case FULL_WIFI_MOD_CLASS_OFDM:
      {
        switch (payloadMode.GetBandwidth ())
          {
          case 20000000:
          default:
            // IEEE Std 802.11-2007, section 17.3.3 and figure 17-4
            // also section 17.3.2.3, table 17-4
            // We return the duration of the SIGNAL field only, since the
            // SERVICE field (which strictly speaking belongs to the PLCP
            // header, see section 17.3.2 and figure 17-1) is sent using the
            // payload mode.
            return 4;
          case 10000000:
            // IEEE Std 802.11-2007, section 17.3.2.3, table 17-4
            return 8;
          case 5000000:
            // IEEE Std 802.11-2007, section 17.3.2.3, table 17-4
            return 16;
          }
      }

    case FULL_WIFI_MOD_CLASS_ERP_OFDM:
      return 16;

    case FULL_WIFI_MOD_CLASS_DSSS:
      if (preamble == FULL_WIFI_PREAMBLE_SHORT)
        {
          // IEEE Std 802.11-2007, section 18.2.2.2 and figure 18-2
          return 24;
        }
      else // WIFI_PREAMBLE_LONG
        {
          // IEEE Std 802.11-2007, sections 18.2.2.1 and figure 18-1
          return 48;
        }

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return 0;
    }
}

uint32_t
FullWifiPhy::GetPlcpPreambleDurationMicroSeconds (FullWifiMode payloadMode, FullWifiPreamble preamble)
{
  switch (payloadMode.GetModulationClass ())
    {
    case FULL_WIFI_MOD_CLASS_OFDM:
      {
        switch (payloadMode.GetBandwidth ())
          {
          case 20000000:
          default:
            // IEEE Std 802.11-2007, section 17.3.3,  figure 17-4
            // also section 17.3.2.3, table 17-4
            return 16;
          case 10000000:
            // IEEE Std 802.11-2007, section 17.3.3, table 17-4
            // also section 17.3.2.3, table 17-4
            return 32;
          case 5000000:
            // IEEE Std 802.11-2007, section 17.3.3
            // also section 17.3.2.3, table 17-4
            return 64;
          }
      }

    case FULL_WIFI_MOD_CLASS_ERP_OFDM:
      return 4;

    case FULL_WIFI_MOD_CLASS_DSSS:
      if (preamble == FULL_WIFI_PREAMBLE_SHORT)
        {
          // IEEE Std 802.11-2007, section 18.2.2.2 and figure 18-2
          return 72;
        }
      else // WIFI_PREAMBLE_LONG
        {
          // IEEE Std 802.11-2007, sections 18.2.2.1 and figure 18-1
          return 144;
        }

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return 0;
    }
}

uint32_t
FullWifiPhy::GetPayloadDurationMicroSeconds (uint32_t size, FullWifiMode payloadMode)
{
  NS_LOG_FUNCTION (size << payloadMode);

  switch (payloadMode.GetModulationClass ())
    {
    case FULL_WIFI_MOD_CLASS_OFDM:
    case FULL_WIFI_MOD_CLASS_ERP_OFDM:
      {
        // IEEE Std 802.11-2007, section 17.3.2.3, table 17-4
        // corresponds to T_{SYM} in the table
        uint32_t symbolDurationUs;

        switch (payloadMode.GetBandwidth ())
          {
          case 20000000:
          default:
            symbolDurationUs = 4;
            break;
          case 10000000:
            symbolDurationUs = 8;
            break;
          case 5000000:
            symbolDurationUs = 16;
            break;
          }

        // IEEE Std 802.11-2007, section 17.3.2.2, table 17-3
        // corresponds to N_{DBPS} in the table
        double numDataBitsPerSymbol = payloadMode.GetDataRate ()  * symbolDurationUs / 1e6;

        // IEEE Std 802.11-2007, section 17.3.5.3, equation (17-11)
        uint32_t numSymbols = lrint (std::ceil ((16 + size * 8.0 + 6.0) / numDataBitsPerSymbol));

        // Add signal extension for ERP PHY
        if (payloadMode.GetModulationClass () == FULL_WIFI_MOD_CLASS_ERP_OFDM)
          {
            return numSymbols * symbolDurationUs + 6;
          }
        else
          {
            return numSymbols * symbolDurationUs;
          }
      }

    case FULL_WIFI_MOD_CLASS_DSSS:
      // IEEE Std 802.11-2007, section 18.2.3.5
      NS_LOG_LOGIC (" size=" << size
                             << " mode=" << payloadMode
                             << " rate=" << payloadMode.GetDataRate () );
      return lrint (std::ceil ((size * 8.0) / (payloadMode.GetDataRate () / 1.0e6)));

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return 0;
    }
}

Time
FullWifiPhy::CalculateTxDuration (uint32_t size, FullWifiMode payloadMode, FullWifiPreamble preamble)
{
  uint32_t duration = GetPlcpPreambleDurationMicroSeconds (payloadMode, preamble)
    + GetPlcpHeaderDurationMicroSeconds (payloadMode, preamble)
    + GetPayloadDurationMicroSeconds (size, payloadMode);
  return MicroSeconds (duration);
}


void
FullWifiPhy::NotifyTxBegin (Ptr<const Packet> packet)
{
  m_phyTxBeginTrace (packet);
}

void
FullWifiPhy::NotifyTxEnd (Ptr<const Packet> packet)
{
  m_phyTxEndTrace (packet);
}

void
FullWifiPhy::NotifyTxDrop (Ptr<const Packet> packet)
{
  m_phyTxDropTrace (packet);
}

void
FullWifiPhy::NotifyRxBegin (Ptr<const Packet> packet)
{
  m_phyRxBeginTrace (packet);
}

void
FullWifiPhy::NotifyRxEnd (Ptr<const Packet> packet)
{
  m_phyRxEndTrace (packet);
}

void
FullWifiPhy::NotifyRxDrop (Ptr<const Packet> packet)
{
  m_phyRxDropTrace (packet);
}

void
FullWifiPhy::NotifyMonitorSniffRx (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm)
{
  m_phyMonitorSniffRxTrace (packet, channelFreqMhz, channelNumber, rate, isShortPreamble, signalDbm, noiseDbm);
}

void
FullWifiPhy::NotifyMonitorSniffTx (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble)
{
  m_phyMonitorSniffTxTrace (packet, channelFreqMhz, channelNumber, rate, isShortPreamble);
}


/**
 * Clause 15 rates (DSSS)
 */

FullWifiMode
FullWifiPhy::GetDsssRate1Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("DsssRate1Mbps",
        FULL_WIFI_MOD_CLASS_DSSS,
                                     true,
                                     22000000, 1000000,
                                     FULL_WIFI_CODE_RATE_UNDEFINED,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetDsssRate2Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("DsssRate2Mbps",
        FULL_WIFI_MOD_CLASS_DSSS,
                                     true,
                                     22000000, 2000000,
                                     FULL_WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}


/**
 * Clause 18 rates (HR/DSSS)
 */
FullWifiMode
FullWifiPhy::GetDsssRate5_5Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("DsssRate5_5Mbps",
        FULL_WIFI_MOD_CLASS_DSSS,
                                     true,
                                     22000000, 5500000,
                                     FULL_WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetDsssRate11Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("DsssRate11Mbps",
        FULL_WIFI_MOD_CLASS_DSSS,
                                     true,
                                     22000000, 11000000,
                                     FULL_WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}


/**
 * Clause 19.5 rates (ERP-OFDM)
 */
FullWifiMode
FullWifiPhy::GetErpOfdmRate6Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate6Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     20000000, 6000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate9Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate9Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     20000000, 9000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate12Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate12Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     20000000, 12000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate18Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate18Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     20000000, 18000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate24Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate24Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     20000000, 24000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate36Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate36Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     20000000, 36000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate48Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate48Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     20000000, 48000000,
                                     FULL_WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

FullWifiMode
FullWifiPhy::GetErpOfdmRate54Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("ErpOfdmRate54Mbps",
        FULL_WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     20000000, 54000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


/**
 * Clause 17 rates (OFDM)
 */
FullWifiMode
FullWifiPhy::GetOfdmRate6Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate6Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     20000000, 6000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate9Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate9Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     20000000, 9000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate12Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate12Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     20000000, 12000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate18Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate18Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     20000000, 18000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate24Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate24Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     20000000, 24000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate36Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate36Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     20000000, 36000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate48Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate48Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     20000000, 48000000,
                                     FULL_WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate54Mbps ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate54Mbps",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     20000000, 54000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

/* 10 MHz channel rates */
FullWifiMode
FullWifiPhy::GetOfdmRate3MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     10000000, 3000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate4_5MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     10000000, 4500000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate6MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     10000000, 6000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate9MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     10000000, 9000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate12MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     10000000, 12000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate18MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate18MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     10000000, 18000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate24MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate24MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     10000000, 24000000,
                                     FULL_WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate27MbpsBW10MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate27MbpsBW10MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     10000000, 27000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

/* 5 MHz channel rates */
FullWifiMode
FullWifiPhy::GetOfdmRate1_5MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate1_5MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     5000000, 1500000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate2_25MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate2_25MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     5000000, 2250000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate3MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     5000000, 3000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate4_5MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     5000000, 4500000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate6MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     true,
                                     5000000, 6000000,
                                     FULL_WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate9MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     5000000, 9000000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate12MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     5000000, 12000000,
                                     FULL_WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

FullWifiMode
FullWifiPhy::GetOfdmRate13_5MbpsBW5MHz ()
{
  static FullWifiMode mode =
    FullWifiModeFactory::CreateWifiMode ("OfdmRate13_5MbpsBW5MHz",
        FULL_WIFI_MOD_CLASS_OFDM,
                                     false,
                                     5000000, 13500000,
                                     FULL_WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

std::ostream& operator<< (std::ostream& os, enum FullWifiPhy::State state)
{
  switch (state)
    {
    case FullWifiPhy::IDLE:
      return (os << "IDLE");
    case FullWifiPhy::CCA_BUSY:
      return (os << "CCA_BUSY");
    case FullWifiPhy::TX:
      return (os << "TX");
    case FullWifiPhy::RX:
      return (os << "RX");
    case FullWifiPhy::SWITCHING:
      return (os << "SWITCHING");
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state");
      return (os << "INVALID");
    }
}

} // namespace ns3

namespace {

static class Constructor
{
public:
  Constructor ()
  {
    ns3::FullWifiPhy::GetDsssRate1Mbps ();
    ns3::FullWifiPhy::GetDsssRate2Mbps ();
    ns3::FullWifiPhy::GetDsssRate5_5Mbps ();
    ns3::FullWifiPhy::GetDsssRate11Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate6Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate9Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate12Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate18Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate24Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate36Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate48Mbps ();
    ns3::FullWifiPhy::GetErpOfdmRate54Mbps ();
    ns3::FullWifiPhy::GetOfdmRate6Mbps ();
    ns3::FullWifiPhy::GetOfdmRate9Mbps ();
    ns3::FullWifiPhy::GetOfdmRate12Mbps ();
    ns3::FullWifiPhy::GetOfdmRate18Mbps ();
    ns3::FullWifiPhy::GetOfdmRate24Mbps ();
    ns3::FullWifiPhy::GetOfdmRate36Mbps ();
    ns3::FullWifiPhy::GetOfdmRate48Mbps ();
    ns3::FullWifiPhy::GetOfdmRate54Mbps ();
    ns3::FullWifiPhy::GetOfdmRate3MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate4_5MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate6MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate9MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate12MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate18MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate24MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate27MbpsBW10MHz ();
    ns3::FullWifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate2_25MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate3MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate4_5MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate6MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate9MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate12MbpsBW5MHz ();
    ns3::FullWifiPhy::GetOfdmRate13_5MbpsBW5MHz ();
  }
} g_constructor;
}
