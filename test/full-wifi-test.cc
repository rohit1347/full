/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *               2010      NICTA
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
 *         Quincy Tse <quincy.tse@nicta.com.au> (Case for Bug 991)
 */

#include "ns3/full-wifi-net-device.h"
#include "ns3/full-yans-wifi-channel.h"
#include "ns3/full-adhoc-wifi-mac.h"
#include "ns3/full-yans-wifi-phy.h"
#include "ns3/full-arf-wifi-manager.h"
//#include "ns3/propagation-delay-model.h"
//#include "ns3/propagation-loss-model.h"
#include "ns3/full-error-rate-model.h"
#include "ns3/full-yans-error-rate-model.h"
//#include "ns3/constant-position-mobility-model.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/object-factory.h"
#include "ns3/full-dca-txop.h"
#include "ns3/full-mac-rx-middle.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/full-duplex-library.h"

namespace ns3 {

class FullWifiTest : public TestCase
{
public:
  FullWifiTest ();

  virtual void DoRun (void);
private:
  void RunOne (void);
  void CreateOne (Vector pos, Ptr<FullYansWifiChannel> channel);
  void SendOnePacket (Ptr<FullWifiNetDevice> dev);

  ObjectFactory m_manager;
  ObjectFactory m_mac;
  ObjectFactory m_propDelay;
};

FullWifiTest::FullWifiTest ()
  : TestCase ("Wifi")
{
}

void
FullWifiTest::SendOnePacket (Ptr<FullWifiNetDevice> dev)
{
  Ptr<Packet> p = Create<Packet> ();
  dev->Send (p, dev->GetBroadcast (), 1);
}

void
FullWifiTest::CreateOne (Vector pos, Ptr<FullYansWifiChannel> channel)
{
  Ptr<Node> node = CreateObject<Node> ();
  Ptr<FullWifiNetDevice> dev = CreateObject<FullWifiNetDevice> ();

  Ptr<FullWifiMac> mac = m_mac.Create<FullWifiMac> ();
  mac->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);
  Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<FullYansWifiPhy> phy = CreateObject<FullYansWifiPhy> ();
  Ptr<FullErrorRateModel> error = CreateObject<FullYansErrorRateModel> ();
  phy->SetErrorRateModel (error);
  phy->SetChannel (channel);
  phy->SetDevice (dev);
  phy->SetMobility (node);
  phy->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);
  Ptr<FullWifiRemoteStationManager> manager = m_manager.Create<FullWifiRemoteStationManager> ();

  mobility->SetPosition (pos);
  node->AggregateObject (mobility);
  mac->SetAddress (Mac48Address::Allocate ());
  dev->SetMac (mac);
  dev->SetPhy (phy);
  dev->SetRemoteStationManager (manager);
  node->AddDevice (dev);

  Simulator::Schedule (Seconds (1.0), &FullWifiTest::SendOnePacket, this, dev);
}

void
FullWifiTest::RunOne (void)
{
  Ptr<FullYansWifiChannel> channel = CreateObject<FullYansWifiChannel> ();
  Ptr<PropagationDelayModel> propDelay = m_propDelay.Create<PropagationDelayModel> ();
  Ptr<PropagationLossModel> propLoss = CreateObject<RandomPropagationLossModel> ();
  channel->SetPropagationDelayModel (propDelay);
  channel->SetPropagationLossModel (propLoss);

  CreateOne (Vector (0.0, 0.0, 0.0), channel);
  CreateOne (Vector (5.0, 0.0, 0.0), channel);
  CreateOne (Vector (5.0, 0.0, 0.0), channel);

  Simulator::Run ();
  Simulator::Destroy ();

  Simulator::Stop (Seconds (10.0));
}

void
FullWifiTest::DoRun (void)
{
  m_mac.SetTypeId ("ns3::AdhocWifiMac");
  m_propDelay.SetTypeId ("ns3::ConstantSpeedPropagationDelayModel");

  m_manager.SetTypeId ("ns3::ArfWifiManager");
  RunOne ();
  m_manager.SetTypeId ("ns3::AarfWifiManager");
  RunOne ();
  m_manager.SetTypeId ("ns3::ConstantRateWifiManager");
  RunOne ();
  m_manager.SetTypeId ("ns3::OnoeWifiManager");
  RunOne ();
  m_manager.SetTypeId ("ns3::AmrrWifiManager");
  RunOne ();
  m_manager.SetTypeId ("ns3::IdealWifiManager");
  RunOne ();

  m_mac.SetTypeId ("ns3::AdhocWifiMac");
  RunOne ();
  m_mac.SetTypeId ("ns3::ApWifiMac");
  RunOne ();
  m_mac.SetTypeId ("ns3::StaWifiMac");
  RunOne ();


  m_propDelay.SetTypeId ("ns3::RandomPropagationDelayModel");
  m_mac.SetTypeId ("ns3::AdhocWifiMac");
  RunOne ();
  Simulator::Destroy ();
}

//-----------------------------------------------------------------------------
class FullQosUtilsIsOldPacketTest : public TestCase
{
public:
  FullQosUtilsIsOldPacketTest () : TestCase ("QosUtilsIsOldPacket")
  {
  }
  virtual void DoRun (void)
  {
    // startingSeq=0, seqNum=2047
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (0, 2047), false, "2047 is new in comparison to 0");
    // startingSeq=0, seqNum=2048
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (0, 2048), true, "2048 is old in comparison to 0");
    // startingSeq=2048, seqNum=0
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (2048, 0), true, "0 is old in comparison to 2048");
    // startingSeq=4095, seqNum=0
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (4095, 0), false, "0 is new in comparison to 4095");
    // startingSeq=0, seqNum=4095
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (0, 4095), true, "4095 is old in comparison to 0");
    // startingSeq=4095 seqNum=2047
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (4095, 2047), true, "2047 is old in comparison to 4095");
    // startingSeq=2048 seqNum=4095
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (2048, 4095), false, "4095 is new in comparison to 2048");
    // startingSeq=2049 seqNum=0
    NS_TEST_EXPECT_MSG_EQ (QosUtilsIsOldPacket (2049, 0), false, "0 is new in comparison to 2049");
  }
};

//-----------------------------------------------------------------------------
class FullInterferenceHelperSequenceTest : public TestCase
{
public:
  FullInterferenceHelperSequenceTest ();

  virtual void DoRun (void);
private:
  Ptr<Node> CreateOne (Vector pos, Ptr<FullYansWifiChannel> channel);
  void SendOnePacket (Ptr<FullWifiNetDevice> dev);
  void SwitchCh (Ptr<FullWifiNetDevice> dev);

  ObjectFactory m_manager;
  ObjectFactory m_mac;
  ObjectFactory m_propDelay;
};

FullInterferenceHelperSequenceTest::FullInterferenceHelperSequenceTest ()
  : TestCase ("InterferenceHelperSequence")
{
}

void
FullInterferenceHelperSequenceTest::SendOnePacket (Ptr<FullWifiNetDevice> dev)
{
  Ptr<Packet> p = Create<Packet> (9999);
  dev->Send (p, dev->GetBroadcast (), 1);
}

void
FullInterferenceHelperSequenceTest::SwitchCh (Ptr<FullWifiNetDevice> dev)
{
  Ptr<FullWifiPhy> p = dev->GetPhy ();
  p->SetChannelNumber (1);
}

Ptr<Node>
FullInterferenceHelperSequenceTest::CreateOne (Vector pos, Ptr<FullYansWifiChannel> channel)
{
  Ptr<Node> node = CreateObject<Node> ();
  Ptr<FullWifiNetDevice> dev = CreateObject<FullWifiNetDevice> ();

  Ptr<FullWifiMac> mac = m_mac.Create<FullWifiMac> ();
  mac->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);
  Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<FullYansWifiPhy> phy = CreateObject<FullYansWifiPhy> ();
  Ptr<FullErrorRateModel> error = CreateObject<FullYansErrorRateModel> ();
  phy->SetErrorRateModel (error);
  phy->SetChannel (channel);
  phy->SetDevice (dev);
  phy->SetMobility (node);
  phy->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);
  Ptr<FullWifiRemoteStationManager> manager = m_manager.Create<FullWifiRemoteStationManager> ();

  mobility->SetPosition (pos);
  node->AggregateObject (mobility);
  mac->SetAddress (Mac48Address::Allocate ());
  dev->SetMac (mac);
  dev->SetPhy (phy);
  dev->SetRemoteStationManager (manager);
  node->AddDevice (dev);

  return node;
}

void
FullInterferenceHelperSequenceTest::DoRun (void)
{
  m_mac.SetTypeId ("ns3::AdhocWifiMac");
  m_propDelay.SetTypeId ("ns3::ConstantSpeedPropagationDelayModel");
  m_manager.SetTypeId ("ns3::ConstantRateWifiManager");

  Ptr<FullYansWifiChannel> channel = CreateObject<FullYansWifiChannel> ();
  Ptr<PropagationDelayModel> propDelay = m_propDelay.Create<PropagationDelayModel> ();
  Ptr<MatrixPropagationLossModel> propLoss = CreateObject<MatrixPropagationLossModel> ();
  channel->SetPropagationDelayModel (propDelay);
  channel->SetPropagationLossModel (propLoss);

  Ptr<Node> rxOnly = CreateOne (Vector (0.0, 0.0, 0.0), channel);
  Ptr<Node> senderA = CreateOne (Vector (5.0, 0.0, 0.0), channel);
  Ptr<Node> senderB = CreateOne (Vector (-5.0, 0.0, 0.0), channel);

  propLoss->SetLoss (senderB->GetObject<MobilityModel> (), rxOnly->GetObject<MobilityModel> (), 0, true);
  propLoss->SetDefaultLoss (999);

  Simulator::Schedule (Seconds (1.0),
                       &FullInterferenceHelperSequenceTest::SendOnePacket, this,
                       DynamicCast<FullWifiNetDevice> (senderB->GetDevice (0)));

  Simulator::Schedule (Seconds (1.0000001),
                       &FullInterferenceHelperSequenceTest::SwitchCh, this,
                       DynamicCast<FullWifiNetDevice> (rxOnly->GetDevice (0)));

  Simulator::Schedule (Seconds (5.0),
                       &FullInterferenceHelperSequenceTest::SendOnePacket, this,
                       DynamicCast<FullWifiNetDevice> (senderA->GetDevice (0)));

  Simulator::Schedule (Seconds (7.0),
                       &FullInterferenceHelperSequenceTest::SendOnePacket, this,
                       DynamicCast<FullWifiNetDevice> (senderB->GetDevice (0)));

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();

  Simulator::Destroy ();
}

//-----------------------------------------------------------------------------
/**
 * Make sure that when multiple broadcast packets are queued on the same
 * device in a short succession no virtual collision occurs 
 *
 * The observed behavior is that the first frame will be sent immediately,
 * and the frames are spaced by (backoff + SIFS + AIFS) time intervals
 * (where backoff is a random number of slot sizes up to maximum CW)
 * 
 * The following test case should _not_ generate virtual collision for the second frame.
 * The seed and run numbers were pick such that the second frame gets backoff = 0.
 *
 *                      frame 1, frame 2
 *                      arrive                SIFS + AIFS
 *                      V                    <-----------> 
 * time  |--------------|-------------------|-------------|----------->
 *       0              1s                  1.001408s     1.001442s
 *                      ^                   ^             ^
 *                      start TX            finish TX     start TX
 *                      frame 1             frame 1       frame 2
 *                      ^
 *                      frame 2
 *                      backoff = 0
 *
 * The buggy behavior was that frame 2 experiences a virtual collision and re-selects the
 * backoff again. As a result, the _actual_ backoff experience by frame 2 is less likely to be 0
 * since that would require two successions of 0 backoff (one that generates the virtual collision and
 * one after the virtual collision).
 */

class FullBug555TestCase : public TestCase
{
public:

  FullBug555TestCase ();

  virtual void DoRun (void);

private:

  void SendOnePacket (Ptr<FullWifiNetDevice> dev);

  ObjectFactory m_manager;
  ObjectFactory m_mac; 
  ObjectFactory m_propDelay;

  Time m_firstTransmissionTime;
  Time m_secondTransmissionTime;
  unsigned int m_numSentPackets;

  void NotifyPhyTxBegin (Ptr<const Packet> p);
};

FullBug555TestCase::FullBug555TestCase ()
  : TestCase ("Test case for Bug 555")
{
}

void 
FullBug555TestCase::NotifyPhyTxBegin (Ptr<const Packet> p)
{
  if (m_numSentPackets == 0)
    {
      NS_ASSERT_MSG (Simulator::Now() == Time (Seconds (1)), "Packet 0 not transmitted at 1 second");
      m_numSentPackets++;
      m_firstTransmissionTime = Simulator::Now ();
    }
  else if (m_numSentPackets == 1)
    {
      m_secondTransmissionTime = Simulator::Now ();
    }
}

void
FullBug555TestCase::SendOnePacket (Ptr<FullWifiNetDevice> dev)
{
  Ptr<Packet> p = Create<Packet> (1000);
  dev->Send (p, dev->GetBroadcast (), 1);
}

void
FullBug555TestCase::DoRun (void)
{
  m_mac.SetTypeId ("ns3::AdhocWifiMac");
  m_propDelay.SetTypeId ("ns3::ConstantSpeedPropagationDelayModel");
  m_manager.SetTypeId ("ns3::ConstantRateWifiManager");

  //The simulation with the following seed and run numbers expe
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (17);

  Ptr<FullYansWifiChannel> channel = CreateObject<FullYansWifiChannel> ();
  Ptr<PropagationDelayModel> propDelay = m_propDelay.Create<PropagationDelayModel> ();
  Ptr<PropagationLossModel> propLoss = CreateObject<RandomPropagationLossModel> ();
  channel->SetPropagationDelayModel (propDelay);
  channel->SetPropagationLossModel (propLoss);

  Ptr<Node> txNode = CreateObject<Node> ();
  Ptr<FullWifiNetDevice> txDev = CreateObject<FullWifiNetDevice> ();
  Ptr<FullWifiMac> txMac = m_mac.Create<FullWifiMac> ();
  txMac->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);

  Ptr<ConstantPositionMobilityModel> txMobility = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<FullYansWifiPhy> txPhy = CreateObject<FullYansWifiPhy> ();
  Ptr<FullErrorRateModel> txError = CreateObject<FullYansErrorRateModel> ();
  txPhy->SetErrorRateModel (txError);
  txPhy->SetChannel (channel);
  txPhy->SetDevice (txDev);
  txPhy->SetMobility (txNode);
  txPhy->ConfigureStandard (FULL_WIFI_PHY_STANDARD_80211a);

  txPhy->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback (&FullBug555TestCase::NotifyPhyTxBegin, this));

  txMobility->SetPosition (Vector (0.0, 0.0, 0.0));
  txNode->AggregateObject (txMobility);
  txMac->SetAddress (Mac48Address::Allocate ());
  txDev->SetMac (txMac);
  txDev->SetPhy (txPhy);
  txDev->SetRemoteStationManager (m_manager.Create<FullWifiRemoteStationManager> ());
  txNode->AddDevice (txDev);

  m_firstTransmissionTime = Seconds (0.0);
  m_secondTransmissionTime = Seconds (0.0);
  m_numSentPackets = 0;
  
  Simulator::Schedule (Seconds (1.0), &FullBug555TestCase::SendOnePacket, this, txDev);
  Simulator::Schedule (Seconds (1.0), &FullBug555TestCase::SendOnePacket, this, txDev);

  Simulator::Stop (Seconds (2.0));
  Simulator::Run ();
  Simulator::Destroy ();

  // First packet has 1408 us of transmit time.   Slot time is 9 us.  
  // Backoff is 0 slots.  SIFS is 16 us.  AIFS is 2 slots = 18 us.
  // Should send next packet at 1408 us + (0 * 9 us) + 16 us + 18 us
  // 1442 us after the first one.
  uint32_t expectedWait1 = 1408 + (0 * 9) + 16 + 18;
  Time expectedSecondTransmissionTime = MicroSeconds (expectedWait1) + Seconds (1.0);

  NS_TEST_ASSERT_MSG_EQ (m_secondTransmissionTime, expectedSecondTransmissionTime, "The second transmission time not correct!");
}

//-----------------------------------------------------------------------------

class FullWifiTestSuite : public TestSuite
{
public:
  FullWifiTestSuite ();
};

FullWifiTestSuite::FullWifiTestSuite ()
  : TestSuite ("devices-wifi", UNIT)
{
  AddTestCase (new FullWifiTest);
  AddTestCase (new FullQosUtilsIsOldPacketTest);
  AddTestCase (new FullInterferenceHelperSequenceTest); // Bug 991
  AddTestCase (new FullBug555TestCase); // Bug 555
}

static FullWifiTestSuite g_wifiTestSuite;

} // namespace ns3
