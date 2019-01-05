/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/config.h"
#include "full-athstats-helper.h"
#include <iomanip>
#include <iostream>
#include <fstream>


NS_LOG_COMPONENT_DEFINE ("FullAthstats");

namespace ns3 {


FullAthstatsHelper::FullAthstatsHelper ()
  : m_interval (Seconds (1.0))
{
}

void
FullAthstatsHelper::EnableAthstats (std::string filename,  uint32_t nodeid, uint32_t deviceid)
{
  Ptr<FullAthstatsWifiTraceSink> athstats = CreateObject<FullAthstatsWifiTraceSink> ();
  std::ostringstream oss;
  oss << filename
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  athstats->Open (oss.str ());

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid;
  std::string devicepath = oss.str ();

  Config::Connect (devicepath + "/Mac/MacTx", MakeCallback (&FullAthstatsWifiTraceSink::DevTxTrace, athstats));
  Config::Connect (devicepath + "/Mac/MacRx", MakeCallback (&FullAthstatsWifiTraceSink::DevRxTrace, athstats));

  Config::Connect (devicepath + "/RemoteStationManager/TxRtsFailed", MakeCallback (&FullAthstatsWifiTraceSink::TxRtsFailedTrace, athstats));
  Config::Connect (devicepath + "/RemoteStationManager/MacTxDataFailed", MakeCallback (&FullAthstatsWifiTraceSink::TxDataFailedTrace, athstats));
  Config::Connect (devicepath + "/RemoteStationManager/MacTxFinalRtsFailed", MakeCallback (&FullAthstatsWifiTraceSink::TxFinalRtsFailedTrace, athstats));
  Config::Connect (devicepath + "/RemoteStationManager/MacTxFinalDataFailed", MakeCallback (&FullAthstatsWifiTraceSink::TxFinalDataFailedTrace, athstats));

  Config::Connect (devicepath + "/Phy/State/RxOk", MakeCallback (&FullAthstatsWifiTraceSink::PhyRxOkTrace, athstats));
  Config::Connect (devicepath + "/Phy/State/RxError", MakeCallback (&FullAthstatsWifiTraceSink::PhyRxErrorTrace, athstats));
  Config::Connect (devicepath + "/Phy/State/Tx", MakeCallback (&FullAthstatsWifiTraceSink::PhyTxTrace, athstats));
  Config::Connect (devicepath + "/Phy/State/State", MakeCallback (&FullAthstatsWifiTraceSink::PhyStateTrace, athstats));
}

void
FullAthstatsHelper::EnableAthstats (std::string filename, Ptr<NetDevice> nd)
{
  EnableAthstats (filename, nd->GetNode ()->GetId (), nd->GetIfIndex ());
}


void
FullAthstatsHelper::EnableAthstats (std::string filename, NetDeviceContainer d)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnableAthstats (filename, dev->GetNode ()->GetId (), dev->GetIfIndex ());
    }
}


void
FullAthstatsHelper::EnableAthstats (std::string filename, NodeContainer n)
{
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
          devs.Add (node->GetDevice (j));
        }
    }
  EnableAthstats (filename, devs);
}





NS_OBJECT_ENSURE_REGISTERED (FullAthstatsWifiTraceSink);

TypeId
FullAthstatsWifiTraceSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullAthstatsWifiTraceSink")
    .SetParent<Object> ()
    .AddConstructor<FullAthstatsWifiTraceSink> ()
    .AddAttribute ("Interval",
                   "Time interval between reports",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&FullAthstatsWifiTraceSink::m_interval),
                   MakeTimeChecker ())
  ;
  return tid;
}

FullAthstatsWifiTraceSink::FullAthstatsWifiTraceSink ()
  :   m_txCount (0),
    m_rxCount (0),
    m_shortRetryCount (0),
    m_longRetryCount (0),
    m_exceededRetryCount (0),
    m_phyRxOkCount (0),
    m_phyRxErrorCount (0),
    m_phyTxCount (0),
    m_writer (0)
{
  Simulator::ScheduleNow (&FullAthstatsWifiTraceSink::WriteStats, this);
}

FullAthstatsWifiTraceSink::~FullAthstatsWifiTraceSink ()
{
  NS_LOG_FUNCTION (this);

  if (m_writer != 0)
    {
      NS_LOG_LOGIC ("m_writer nonzero " << m_writer);
      if (m_writer->is_open ())
        {
          NS_LOG_LOGIC ("m_writer open.  Closing " << m_writer);
          m_writer->close ();
        }

      NS_LOG_LOGIC ("Deleting writer " << m_writer);
      delete m_writer;

      NS_LOG_LOGIC ("m_writer = 0");
      m_writer = 0;
    }
  else
    {
      NS_LOG_LOGIC ("m_writer == 0");
    }
}

void
FullAthstatsWifiTraceSink::ResetCounters ()
{
  m_txCount = 0;
  m_rxCount = 0;
  m_shortRetryCount = 0;
  m_longRetryCount = 0;
  m_exceededRetryCount = 0;
  m_phyRxOkCount = 0;
  m_phyRxErrorCount = 0;
  m_phyTxCount = 0;
}

void
FullAthstatsWifiTraceSink::DevTxTrace (std::string context, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << context << p);
  ++m_txCount;
}

void
FullAthstatsWifiTraceSink::DevRxTrace (std::string context, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << context << p);
  ++m_rxCount;
}


void
FullAthstatsWifiTraceSink::TxRtsFailedTrace (std::string context, Mac48Address address)
{
  NS_LOG_FUNCTION (this << context << address);
  ++m_shortRetryCount;
}

void
FullAthstatsWifiTraceSink::TxDataFailedTrace (std::string context, Mac48Address address)
{
  NS_LOG_FUNCTION (this << context << address);
  ++m_longRetryCount;
}

void
FullAthstatsWifiTraceSink::TxFinalRtsFailedTrace (std::string context, Mac48Address address)
{
  NS_LOG_FUNCTION (this << context << address);
  ++m_exceededRetryCount;
}

void
FullAthstatsWifiTraceSink::TxFinalDataFailedTrace (std::string context, Mac48Address address)
{
  NS_LOG_FUNCTION (this << context << address);
  ++m_exceededRetryCount;
}



void
FullAthstatsWifiTraceSink::PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, FullWifiMode mode, enum FullWifiPreamble preamble)
{
  NS_LOG_FUNCTION (this << context << packet << " mode=" << mode << " snr=" << snr );
  ++m_phyRxOkCount;
}

void
FullAthstatsWifiTraceSink::PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr)
{
  NS_LOG_FUNCTION (this << context << packet << " snr=" << snr );
  ++m_phyRxErrorCount;
}

void
FullAthstatsWifiTraceSink::PhyTxTrace (std::string context, Ptr<const Packet> packet, FullWifiMode mode, FullWifiPreamble preamble, uint8_t txPower)
{
  NS_LOG_FUNCTION (this << context << packet << "PHYTX mode=" << mode );
  ++m_phyTxCount;
}


void
FullAthstatsWifiTraceSink::PhyStateTrace (std::string context, Time start, Time duration, enum FullWifiPhy::State state)
{
  NS_LOG_FUNCTION (this << context << start << duration << state);

}



void
FullAthstatsWifiTraceSink::Open (std::string const &name)
{
  NS_LOG_FUNCTION (this << name);
  NS_ABORT_MSG_UNLESS (m_writer == 0, "AthstatsWifiTraceSink::Open (): m_writer already allocated (std::ofstream leak detected)");

  m_writer = new std::ofstream ();
  NS_ABORT_MSG_UNLESS (m_writer, "AthstatsWifiTraceSink::Open (): Cannot allocate m_writer");

  NS_LOG_LOGIC ("Created writer " << m_writer);

  m_writer->open (name.c_str (), std::ios_base::binary | std::ios_base::out);
  NS_ABORT_MSG_IF (m_writer->fail (), "AthstatsWifiTraceSink::Open (): m_writer->open (" << name.c_str () << ") failed");

  NS_ASSERT_MSG (m_writer->is_open (), "AthstatsWifiTraceSink::Open (): m_writer not open");

  NS_LOG_LOGIC ("Writer opened successfully");
}


void
FullAthstatsWifiTraceSink::WriteStats ()
{
  NS_ABORT_MSG_UNLESS (this, "function called with null this pointer, now=" << Now () );
  // the comments below refer to how each value maps to madwifi's athstats
  // I know C strings are ugly but that's the quickest way to use exactly the same format as in madwifi
  char str[200];
  snprintf (str, 200, "%8u %8u %7u %7u %7u %6u %6u %6u %7u %4u %3uM\n",
            (unsigned int) m_txCount, // /proc/net/dev transmitted packets to which we should subract mgmt frames
            (unsigned int) m_rxCount, // /proc/net/dev received packets but subracts mgmt frames from it
            (unsigned int) 0,        // ast_tx_altrate,
            (unsigned int) m_shortRetryCount,    // ast_tx_shortretry,
            (unsigned int) m_longRetryCount,     // ast_tx_longretry,
            (unsigned int) m_exceededRetryCount, // ast_tx_xretries,
            (unsigned int) m_phyRxErrorCount,    // ast_rx_crcerr,
            (unsigned int) 0,        // ast_rx_badcrypt,
            (unsigned int) 0,        // ast_rx_phyerr,
            (unsigned int) 0,        // ast_rx_rssi,
            (unsigned int) 0         // rate
            );

  if (m_writer)
    {

      *m_writer << str;

      ResetCounters ();
      Simulator::Schedule (m_interval, &FullAthstatsWifiTraceSink::WriteStats, this);
    }
}




} // namespace ns3


