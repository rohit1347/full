/*
 * library.cc
 *
 *  Created on: Jul 16, 2012
 *      Author: sam
 */



#include "full-duplex-library.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FullDuplexLibrary");


struct sort_pred {
  bool operator()(const std::pair<Ptr<Node>, double> &left, const std::pair<Ptr<Node>, double> &right) {
    return left.second < right.second;
  }
};


Vector
GetPosition (Ptr<Node> node)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  return mobility->GetPosition ();
}

double
CalculateSnr(Ptr<Node> src, Ptr<Node> dst, Ptr<PropagationLossModel> lossModel, double txPower){
  Ptr<MobilityModel> senderMobility = src->GetObject<MobilityModel> ();
  Ptr<MobilityModel> recvMobility = dst->GetObject<MobilityModel> ();
  double rxPower = lossModel->CalcRxPower (txPower, senderMobility, recvMobility);
  return rxPower;
}



double
GetDistance (Ptr<Node> node1, Ptr<Node> node2)
{
  Vector pos1 = GetPosition (node1);
  Vector pos2 = GetPosition (node2);
  return sqrt (pow (abs (pos1.x - pos2.x), 2) + pow (abs (pos1.y - pos2.y), 2));
}


void
CreateStream (Ptr<Node> src, Ptr<Node> dest, Time start, Time stop, double cbrInterval, uint32_t packetSize)
{

  Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel> ();
//  std::cout << "distance: " << GetDistance (src, dest) <<"  snr between ap and client: " << CalculateSnr(src,dest,lossModel,22) <<"\n";


  Vector srcPos = GetPosition (src);
  Vector destPos = GetPosition (dest);

  Ptr<Ipv4> ipv4src = src->GetObject<Ipv4>();
  Ptr<Ipv4> ipv4dest = dest->GetObject<Ipv4>();

  Ipv4InterfaceAddress iaddrsrc = ipv4src->GetAddress (1,0);
  Ipv4InterfaceAddress iaddrdest = ipv4dest->GetAddress (1,0);

  Ipv4Address ipv4Addrsrc = iaddrsrc.GetLocal ();
  Ipv4Address ipv4Addrdest = iaddrdest.GetLocal ();


  NS_LOG_INFO ("App: Src ip " << ipv4Addrsrc
                               << " pos (" << srcPos.x << "," << srcPos.y << ")\n"
                               << "    Dest ip " << ipv4Addrdest
                               << " pos (" << destPos.x << "," << destPos.y << ")"
                               << " dist " << GetDistance (src, dest));

  //cast to void, to suppress variable set but not
  //used compiler warning in optimized builds
  (void) srcPos;
  (void) destPos;
  (void) ipv4Addrsrc;

  ////   Install applications: two CBR streams each saturating the channel
    UdpServerHelper server (9);
    server.SetAttribute ("StartTime", TimeValue (start - Seconds(0.1)));
    server.Install (dest);

  UdpClientHelper client1 (ipv4Addrdest, 9);
  // client1.SetAttribute ("PacketSize", RandomVariableValue (ConstantRandomVariable (packetSize)));
  ConstantRandomVariable cons;
  client1.SetAttribute ("PacketSize", DoubleValue(cons.GetValue(packetSize)));
  //client1.SetAttribute ("PacketSize", (const AttributeValue)0);
  client1.SetAttribute ("StartTime", TimeValue (start));
  client1.SetAttribute ("StopTime", TimeValue (stop));
  client1.SetAttribute ("MaxPackets", UintegerValue (0));
  // client1.SetAttribute ("Interval",  RandomVariableValue (ExponentialRandomVariable(cbrInterval,.1)));
  ExponentialRandomVariable exp;
  client1.SetAttribute ("Interval",  DoubleValue(exp.GetValue(cbrInterval,.1)));
  client1.Install (src);
}

std::vector<std::pair<Ptr<Node>, double> >
GetDistanceList (NodeContainer nodes, Ptr<Node> node)
{
  std::vector<std::pair<Ptr<Node>, double> > distanceList;
    // make a list of all the other nodes, with distance
    for (NodeContainer::Iterator it = nodes.Begin (); it != nodes.End (); ++it)
      {
        if (*it != node)
          distanceList.push_back(std::make_pair (*it, GetDistance (node, *it)));
      }
    // sort the new node list by distance
    std::sort(distanceList.begin(), distanceList.end(), sort_pred());
    return distanceList;
}

ApplicationContainer
SetupPacketReceive (Ptr<Node> node)
{
  int port = 9;
  PacketSinkHelper  sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = sinkHelper.Install (node);
  return sinkApp;

}

void
CreateTcpStream (Ptr<Node> src, Ptr<Node> dest, Time start, Time stop, double cbrInterval, uint32_t packetSize)
{
          int port = 9;
          Vector serverPos = GetPosition (src);
          Vector clientPos = GetPosition (dest);

          Ptr<Ipv4> ipv4Server = src->GetObject<Ipv4>();
          Ptr<Ipv4> ipv4Client = dest->GetObject<Ipv4>();

          Ipv4InterfaceAddress iaddrServer = ipv4Server->GetAddress (1,0);
          Ipv4InterfaceAddress iaddrClient = ipv4Client->GetAddress (1,0);

          Ipv4Address ipv4AddrServer = iaddrServer.GetLocal ();
          Ipv4Address ipv4AddrClient = iaddrClient.GetLocal ();

          NS_LOG_DEBUG ("Set up Server Device " <<  (src->GetDevice (0))->GetAddress ()
                                                << " with ip " << ipv4AddrServer
                                                << " position (" << serverPos.x << "," << serverPos.y << "," << serverPos.z << ")");

          NS_LOG_DEBUG ("Set up Client Device " <<  (dest->GetDevice (0))->GetAddress ()
                                                << " with ip " << ipv4AddrClient
                                                << " position (" << clientPos.x << "," << clientPos.y << "," << clientPos.z << ")"
                                                << "\n");

          //cast serverPos,clientPos,iaddrClient to void, to suppress variable set but not
          //used compiler warning in optimized builds
          (void) serverPos;
          (void) clientPos;
          (void) ipv4AddrClient;

          // Equipping the source  node with OnOff Application used for sending
          OnOffHelper onoff ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address ("10.0.0.1"), port)));
          onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
          onoff.SetAttribute ("OffTime",StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
          onoff.SetAttribute ("DataRate", DataRateValue (DataRate (10000000)));
          onoff.SetAttribute ("PacketSize", UintegerValue (packetSize));
          onoff.SetAttribute ("Remote", AddressValue (InetSocketAddress (ipv4AddrClient, port)));

          ApplicationContainer apps = onoff.Install (src);
          apps.Start (Seconds(1));
          apps.Stop ((stop));


//        //create a udp application to warm up the simulation
//          OnOffHelper onoffUdp ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address ("10.0.0.1"), 10)));
//          onoffUdp.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1)));
//          onoffUdp.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
//          onoffUdp.SetAttribute ("DataRate", DataRateValue (DataRate (10000000)));
//          onoffUdp.SetAttribute ("PacketSize", UintegerValue (1500));
//          onoffUdp.SetAttribute ("Remote", AddressValue (InetSocketAddress (ipv4AddrServer, port)));
//          ApplicationContainer appsUdp = onoffUdp.Install (dest);
//          appsUdp.Start (start);
//          appsUdp.Stop (Seconds(3));

          ApplicationContainer sinkApp = SetupPacketReceive (dest);
          sinkApp.Start((start - Seconds(0.01)));
          sinkApp.Stop((stop + Seconds(0.01)));

}



std::string
NodeLogList::Report (Time duration, bool pretty)
{
  Ptr<NodeLog> overall = CreateObject<NodeLog> ();
  std::stringstream out;
  Ptr<NodeLog> curNode;

  for (std::vector<Ptr<NodeLog> >::iterator it = nodeLogList.begin ();
      it != nodeLogList.end (); ++it)
    {
      curNode = *it;
      out << curNode->Print (duration, pretty) << "\n";
      if (pretty)
        {
          overall->bytesSent += curNode->bytesSent;
          overall->ackTimeout += curNode->ackTimeout;
          overall->sendSecondaryPacket += curNode->sendSecondaryPacket;
          overall->sendExposedPacket += curNode->sendExposedPacket;
          overall->sendSignature += curNode->sendSignature;
          overall->sendDataSuccess += curNode->sendDataSuccess;
          overall->sendDataFail += curNode->sendDataFail;
          overall->sendPayload += curNode->sendPayload;
          overall->sendAck += curNode->sendAck;
          overall->sendPacket += curNode->sendPacket;
          overall->sendFill += curNode->sendFill;
          overall->enqueue += curNode->enqueue;
          overall->exposedAckTimeout += curNode->exposedAckTimeout;
          overall->failedAck += curNode->failedAck;
          overall->primaryAckTimeout += curNode->primaryAckTimeout;
          overall->primaryBytes += curNode->primaryBytes;
          overall->secondaryBytes += curNode->secondaryBytes;
          overall->exposedBytes += curNode->exposedBytes;
          overall->delay += curNode->delay;
        }
    }
  if (pretty)
    {
      out << "Total:\n";
      out << overall->Print (duration, true) << "\n";
    }
  return out.str ();
}


std::string
NodeLogList::ReportThroughput (Time duration)
{
  uint32_t bytes = 0;
  std::stringstream out;

  for (std::vector<Ptr<NodeLog> >::iterator it = nodeLogList.begin ();
      it != nodeLogList.end (); ++it)
    {
      bytes += (*it)->bytesSent;
//       bytes += (*it)->receivedPackets;
    }
  out << ((float)bytes)*8.0e-6/duration.GetSeconds ();
//  out << bytes;
  return out.str ();
}

double
NodeLogList::ReportDelay ()
{
  double aggregateDelay = 0;
  int sendPackets = 0;
  for (std::vector<Ptr<NodeLog> >::iterator it = nodeLogList.begin ();
      it != nodeLogList.end (); ++it)
    {
      aggregateDelay += (*it)->delay;
    }
  for (std::vector<Ptr<NodeLog> >::iterator it = nodeLogList.begin ();
      it != nodeLogList.end (); ++it)
    {
      sendPackets += (*it)->receivedPackets;
    }
  return aggregateDelay/sendPackets;
}

std::string ReportLegend ()
{
  std::stringstream out;
  // set up for pretty = false
  out << "id, "              //1
      << "enqueue, "          //2
      << "tp, "               //3
      << "sDataSuccess, "     //4
      << "sDataFail, "        //5
      << "ackTimeout, "       //6
      << "sData, "            //7
      << "sSecondaryPacket, "  //8
      << "sExposedPacket, "    //9
      << "sSignature, "       //10
      << "sPayload, "           //11
      << "sAck, "               //12
      << "sPacket, "           //13
      << "sFill, "              //14
      << "exposedAckTimeout, "    //15
      << "failedAck, "            //16
      << "primaryAckTimeout, "        //17
      << "primaryTp, "              //18
      << "SecondaryTp, "           //19
      << "ExposedTp, "             //20
      << "delay ";                //21

  return out.str ();
}


std::string
NodeLog::Print (Time duration, bool pretty)
{
  std::stringstream out;
  out.precision(pretty ? 3 : 5);
  if (pretty)
    {
      out << "id: " << id << ", "
          << "enqueue: " << enqueue << ", "
          //    << "\tbytes: " << bytesSent << ", "
          << "\ttp Mbps: " << bytesSent*8.0/1e6/duration.GetSeconds () << ", "
          << "\tsDataSuccess: " << sendDataSuccess << ", "
          << "\tsDataFail: " << sendDataFail << ", "
          << "\tackTimeout: " << ackTimeout << ", "
          << "\tsData: " << sendData << ", "
          << "\tsSecondaryPacket: " << sendSecondaryPacket << ", "
          << "\tsExposedPacket: " << sendExposedPacket << ", "
          << "\tsSignature: " << sendSignature << ", "
          << "\tsPayload: " << sendPayload << ", "
          << "\tsAck: " << sendAck << ", "
          << "\tsPacket: " << sendPacket << ", "
          << "\tsFill: " << sendFill << ", "
          << "\texposedAckTimeout: " << exposedAckTimeout << ", "
          << "\tfailedAck: " << failedAck <<", "
          << "\tprimaryAckTimeout: " <<primaryAckTimeout <<", "
          << "\tprimary tp Mbps: " << primaryBytes*8.0/1e6/duration.GetSeconds () <<", "
          << "\tseondary tp Mbps: " <<secondaryBytes*8.0/1e6/duration.GetSeconds () <<", "
          << "\texposed tp Mbps: " << exposedBytes*8.0/1e6/duration.GetSeconds () <<", "
          << "\tdelay s" << delay;
    }
  else
    {
      out << id << ", "
          << enqueue << ", "
          //    << bytesSent << ", "
          << bytesSent*8.0/1e6/duration.GetSeconds () << ", "
          << sendDataSuccess << ", "
          << sendDataFail << ", "
          << ackTimeout << ", "
          << sendData << ", "
          << sendSecondaryPacket << ", "
          << sendExposedPacket << ", "
          << sendSignature << ", "
          << sendPayload << ", "
          << sendAck << ", "
          << sendPacket << ", "
          << sendFill << ", "
          << exposedAckTimeout << ", "
          << failedAck <<", "
          << primaryAckTimeout <<", "
          << primaryBytes <<", "
          << secondaryBytes <<", "
          << exposedBytes <<", "
          << delay;
    }

  return out.str ();
}

CommandLine
CreateCommandLine (Ptr<DuplexExperiment> d)
{
  CommandLine cmd;
  cmd.AddValue("stopTime","stopTime",d->stopTime);
  cmd.AddValue("startTime","startTime",d->startTime);

  cmd.AddValue("numNodes","numNodes",d->numNodes);
  cmd.AddValue("numStreams","numStreams",d->numStreams);
  cmd.AddValue("numNodesPerAp","numNodesPerAp",d->numNodesPerAp);
  cmd.AddValue("numAps","numAps",d->numAps);

  cmd.AddValue("dim","dim",d->dim);
  cmd.AddValue("spread","spread",d->spread);
  cmd.AddValue("txPower","txPower",d->txPower); // in dBmW
  cmd.AddValue("apSpread","apSpread",d->apSpread);
  cmd.AddValue("stationSpread","stationSpread",d->stationSpread);
  cmd.AddValue("maxDistance","maxDistance",d->maxDistance);
  cmd.AddValue("cbrInterval","cbrInterval",d->cbrInterval);
  cmd.AddValue("streamsPerNode","streamsPerNode",d->streamsPerNode);
  cmd.AddValue("exposedThreshold","exposedThreshold",d->exposedThreshold);
  cmd.AddValue("minProbability", "minProbability", d->minProbability);


  cmd.AddValue("loadPositions","loadPositions",d->loadPositions);
//  cmd.AddValue("duplexMode","duplexMode",d->duplexMode);
  cmd.AddValue ("returnPacket", "enable returnPacket (true) or not (false)", d->returnPacket);
  cmd.AddValue ("secondaryPacket", "enable forwarding packet (true) or not (false)", d->secondaryPacket);


  cmd.AddValue("positionFileName","positionFileName",d->positionFileName);
  cmd.AddValue("logFileName","logFileName",d->logFileName);
  cmd.AddValue("legendFileName","legendFileName",d->legendFileName);
  cmd.AddValue("protocol","upd or tcp protocol, default--udp",d->protocol);

  return cmd;
}

