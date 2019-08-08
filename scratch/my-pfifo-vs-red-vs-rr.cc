/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: John Abraham <john.abraham@gatech.edu>
 * Modified by:   Pasquale Imputato <p.imputato@gmail.com>
 *
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PFIFO_VS_RED_VS_RR");

class SimStats {
public:
  SimStats();
  void takeStats();
  void printStatsEachFlow();
  float getSumThroughput();
  float getFairnessIndex();

private:
  std::map<std::string, float> downFlowThroughput;
  float sumThroughput;

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
};

SimStats::SimStats()
  : sumThroughput (0)
{
  monitor = flowmon.InstallAll();
}


void SimStats::takeStats() {
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier =
    DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  // 10 * 256^3 + 2 * 256^2 + 1 * 256^1 + 0
  static const uint32_t ADDR_10_2_0_0 = 167903232;
  // 255 * 256^0 + 255 * 256^1
  static const uint32_t ADDR_10_2_255_255 = ADDR_10_2_0_0 + 65535;

  for(auto i = stats.begin(); i != stats.end(); i++) {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

    uint32_t duration = i->second.timeLastRxPacket.GetSeconds()
                        - i->second.timeFirstRxPacket.GetSeconds();
    // uint32_t duration = 14;
    uint64_t rxBytes = i->second.rxBytes;

    if(t.sourceAddress.Get() > ADDR_10_2_0_0 &&
      t.sourceAddress.Get() < ADDR_10_2_255_255) {
      std::string src_ip3 = std::to_string((t.sourceAddress.Get() % 65536 - 1) / 256);
      std::string dst_ip3 = std::to_string((t.destinationAddress.Get() % 65536 - 1) / 256);
      std::string label = "10.2." + src_ip3 + ".1->10.1." + dst_ip3 + ".1";
      float throughput = rxBytes * 8.0 / duration / (1024.0 * 1024.0);
      downFlowThroughput[label] = throughput;
      sumThroughput += throughput;
    }
  }
}

void SimStats::printStatsEachFlow() {
  std::cout << "-----------------------\n";
  for(const auto f : downFlowThroughput) {
      std::cout << "( " << f.first << " ) Troughput: " << f.second << " Mbps\n";
  }
  std::cout << "-----------------------\n";
}

float SimStats::getSumThroughput() {
  return sumThroughput;
}

float SimStats::getFairnessIndex() {
  double a = 0.0, b = 0.0;
  for(const auto f : downFlowThroughput) {
    a += f.second;
    b += f.second * f.second;
  }
  uint16_t n = downFlowThroughput.size();
  return (a * a) / ((n) * b);
}

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 5;
  uint32_t    maxPackets = 100;
  uint32_t    modeBytes  = 0;
  uint32_t    queueDiscLimitPackets = 1000;
  double      minTh = 50;
  double      maxTh = 80;
  uint32_t    pktSize = 512;
  std::string appDataRate = "10Mbps";
  std::string queueDiscType = "PfifoFast";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "1Mbps";
  std::string bottleNeckLinkDelay = "50ms";
  uint16_t    rrQueueNum = 10;

  CommandLine cmd;
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("queueDiscType", "Set QueueDisc type to PfifoFast or RED", queueDiscType);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set QueueDisc mode to Packets <0> or bytes <1>", modeBytes);

  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);

  cmd.AddValue ("rrQueueNum", "Number of RR queue", rrQueueNum);
  cmd.Parse (argc,argv);

  if ((queueDiscType != "RED") && (queueDiscType != "PfifoFast") && (queueDiscType != "RR"))
    {
      NS_ABORT_MSG ("Invalid queue disc type: Use --queueDiscType=RED or PfifoFast or RR");
    }

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  Config::SetDefault ("ns3::QueueBase::MaxSize",
                      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, maxPackets)));

  if (!modeBytes)
  {
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                        QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    Config::SetDefault ("ns3::PfifoFastQueueDisc::MaxSize",
                        QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    Config::SetDefault ("ns3::RRQueueDisc::MaxSize",
                        QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
  }
  else
  {
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                        QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
    minTh *= pktSize;
    maxTh *= pktSize;
  }

  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));

  Config::SetDefault ("ns3::RRQueueDisc::QueueNum", UintegerValue (rrQueueNum));

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));

  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  // Install Stack
  InternetStackHelper stack;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
     stack.Install (d.GetLeft (i));
    }
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
     stack.Install (d.GetRight (i));
    }

  stack.Install (d.GetLeft ());
  stack.Install (d.GetRight ());
  TrafficControlHelper tchBottleneck;

  if (queueDiscType == "PfifoFast")
    {
      tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
    }
  else if (queueDiscType == "RED")
    {
      tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
    }
  else if (queueDiscType == "RR")
    {
      tchBottleneck.SetRootQueueDisc ("ns3::RRQueueDisc");
    }
  else
    {
      NS_LOG_ERROR ("queueDiscType must be RED/PfifoFast/RR: " << queueDiscType);
    }

  tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
  tchBottleneck.Install (d.GetRight ()->GetDevice (0));

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetLeft (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (30.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      // Create an on/off app sending packets to the left side
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (15.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  SimStats simStats;

  Simulator::Stop (Seconds(30));
  Simulator::Run ();
  Simulator::Destroy ();

  simStats.takeStats();
  float sum = simStats.getSumThroughput();
  float fairnessIndex = simStats.getFairnessIndex();
  uint64_t linkCapacity = DataRate (bottleNeckLinkBw).GetBitRate();

  simStats.printStatsEachFlow();

  std::cout << "             Queue Type: " << queueDiscType << std::endl;
  std::cout << "       Total throughput: " << sum << " Mbps\n";
  std::cout << "Bottleneck Link Utility: " << sum / (linkCapacity / 1000.0 / 1000.0) << std::endl;
  std::cout << "         Fairness index: " << fairnessIndex << std::endl;

  return 0;
}
