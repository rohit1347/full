/*
 * library.cc
 *
 *  Created on: Jul 16, 2012
 *      Author: sam
 */



#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
// #include "ns3/tools-module.h"
// #include "ns3/stats-module.h"
#include "ns3/average.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"

#include <vector>
#include <fstream>
#include <string>
#include <sstream>

using namespace ns3;

#ifndef FULL_DUPLEX_LIBRARY_H
#define FULL_DUPLEX_LIBRARY_H


Vector
GetPosition (Ptr<Node> node);

double
CalculateSnr(Ptr<Node> src, Ptr<Node> dst, Ptr<PropagationLossModel> lossModel, double txPower);

//void
//SetDuplexConflictMap (NodeContainer nodes,
//    std::vector<std::pair<Ptr<Node>,Ptr<Node> > > flowList,
//    Ptr<PropagationLossModel> lossModel,
//    double txPower);
//
//void
//SetSwConflictMap (NodeContainer nodes,
//    std::vector<std::pair<Ptr<Node>, Ptr<Node> > > flowList,
//    Ptr<PropagationLossModel> lossModel,
//    double txPower);

double
GetDistance (Ptr<Node> node1, Ptr<Node> node2);

void
ClockSeconds (double interval);

void
CreateStream (Ptr<Node> src, Ptr<Node> dest, Time start, Time stop, double cbrInterval, std::size_t packetSize);

ApplicationContainer
SetupPacketReceive (Ptr<Node> node);
void
CreateTcpStream (Ptr<Node> src, Ptr<Node> dest, Time start, Time stop, double cbrInterval, uint32_t packetSize);


std::vector<std::pair<Ptr<Node>, double> >
GetDistanceList (NodeContainer nodes, Ptr<Node> node);
std::string ReportLegend ();


class NodeLog : public Object
{
public:
  NodeLog ()
  {
    id = 0;
    enqueue = 0;
    sendDataSuccess = 0;
    sendDataFail = 0;
    sendData = 0;
    ackTimeout = 0;
    sendSecondaryPacket = 0;
    sendExposedPacket = 0;
    sendSignature = 0;
    sendPayload = 0;
    sendAck = 0;
    sendPacket = 0;
    sendFill = 0;
    bytesSent = 0;
    exposedAckTimeout = 0;

    failedAck = 0;
    exposedBytes = 0;
    primaryBytes = 0;
    secondaryBytes = 0;
    primaryAckTimeout = 0;
    delay = 0;
    receivedPackets = 0;
  }
  uint32_t id;
  uint32_t enqueue;
  uint32_t sendDataSuccess;
  uint32_t sendDataFail;
  uint32_t ackTimeout;
  uint32_t exposedAckTimeout;
  uint32_t sendSecondaryPacket;
  uint32_t sendExposedPacket;
  uint32_t sendSignature;
  uint32_t sendPayload;
  uint32_t sendAck;
  uint32_t sendData;
  uint32_t sendPacket;
  uint32_t sendFill;
  uint32_t bytesSent;

  uint32_t failedAck;
  uint32_t exposedBytes;
  uint32_t primaryBytes;
  uint32_t secondaryBytes;
  uint32_t primaryAckTimeout;

  double delay;
  uint32_t receivedPackets;

  std::string Print (Time duration, bool pretty);
};




class NodeLogList : public Object
{
public:
  std::vector<Ptr<NodeLog> > nodeLogList;
  Ptr<NodeLog> operator[] (uint16_t i) { return nodeLogList[i]; }

  void Create (uint32_t numNodes)
  {
    for (uint32_t i = 0; i < numNodes; i++)
      {
        Ptr<NodeLog> node = CreateObject<NodeLog> ();
        node->id = i;
        // conveniently, this gives the node the desired list index of its id
        nodeLogList.push_back (node);
      }
  }

  std::string Report (Time duration, bool pretty);
  std::string ReportThroughput (Time duration);
  double ReportDelay();

};

class DuplexExperiment : public Object
{
public:
  DuplexExperiment ()
  {
    stopTime = Seconds(5);
    startTime = Seconds(1.0);
    numNodes = 40;
    numStreams = 40;

    dim = 100;
    spread = .4;
    apSpread = .2;
    stationSpread = 30;
    txPower = 22; // in dBmW
    streamsPerNode = 1;
    numNodesPerAp = 5;
    numAps = 30;
    maxDistance = 100;
    cbrInterval = .0001;

    packetSize = 500;
    loadPositions = false;
    fullDuplex = false;
    captureEffect = false;
    returnPacket = false;
    secondaryPacket = false;
    busytone = false;
    phyMode  = "OfdmRate6Mbps";

    uplinkRate = "6Mbps";
    downlinkRate = "6Mbps";

    std::stringstream ss;
    ss<<"runs/pos_nodes"<<(int)numNodesPerAp<<"_aps_"<<(int)numAps;
    positionFileName = ss.str ();
    std::stringstream ss_log;
    ss_log<<"runs/log_nodes"<<(int)numNodesPerAp<<"_aps_"<<(int)numAps;
    std::stringstream ss_flow;
    ss_flow<<"runs/flow_nodes"<<(int)numNodesPerAp<<"_aps_"<<(int)numAps;
    flowFileName = ss_flow.str ();
//    std::cout << logFileName << "  file Name\n";
    legendFileName = "runs/legend";
    topoFileName = "runs/snr.txt";
    protocol = "udp";
  };

  Time stopTime;
  Time startTime;

  uint16_t numNodes;
  uint16_t numStreams;
  uint16_t numNodesPerAp;
  uint16_t numAps;

  double dim;
  double spread;
  double txPower; // in dBmW
  double apSpread;
  double stationSpread;
  double maxDistance;
  double cbrInterval;
  double streamsPerNode;
  double exposedThreshold;
  double minProbability;

  bool loadPositions;
  bool fullDuplex;
  bool captureEffect;
  bool returnPacket;
  bool secondaryPacket;
  bool busytone;
  std::string phyMode;
  uint32_t packetSize;

  std::string uplinkRate ;
  std::string downlinkRate ;


  std::string positionFileName;
  std::string logFileName;
  std::string legendFileName;
  std::string topoFileName;
  std::string flowFileName;

  std::string protocol;

  // Trace connection functions
  NodeLogList nodeLogList;

};

CommandLine CreateCommandLine(Ptr<DuplexExperiment> d);

#endif  /*  FULL_DUPLEX_LIBRARY  */
