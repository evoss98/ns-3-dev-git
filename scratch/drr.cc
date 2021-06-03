/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Stanford University
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
 * Author: Serhat Arslan <sarslan@stanford.edu>
 * Modified by: Ang Li & Erin Voss
 *
 * Deficit Round Robin Topology
 *
 * Single host single hop:
 *
 *    h1-----------------s0-----------------h2
 *
 * Multi host single hop:
 *
 *    h1-----------------s0-----------------h3
 *                        |
 *                        |
 *                        |
 *                       h2
 *
 * Multi host multi hop:
 *
 *    h1-----------------s0-----------------s1--------------h3
 *                                           |
 *                                           |
 *                                           |
 *                                          h2
 *
 *
 *  Usage (e.g.):
 *    sudo ./waf --run 'drr'
 *    sudo NS_LOG="DrrQueueDisc" ./waf --run scratch/drr.cc
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/drr-queue-disc.h"
#include "ns3/sfq-queue-disc.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DRRExample");

static double TRACE_START_TIME = 0.05;

/* Tracing is one of the most valuable features of a simulation environment.
 * It means we can get to see evolution of any value / state we are interested
 * throughout the simulation. Basically, in NS-3, you set some tracing options
 * for pre-defined TraceSources and provide a function that defines what to do
 * when the traced value changes.
 * The helper functions below are used to trace particular values throughout
 * the simulation. Make sure to take a look at how they are used in the main
 * function. You can learn more about tracing at
 * https://www.nsnam.org/docs/tutorial/html/tracing.html If you are going to
 * work with NS-3 in the future, you will definitely need to read this page.
 */

static void
QueueOccupancyTracer (Ptr<OutputStreamWrapper> stream,
                     uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Queue Disc size from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void
UdpReceiverTracer (Ptr<OutputStreamWrapper> stream,
                   const Ptr< const Packet > packet,
                   const Address &srcAddress,
                   const Address &destAddress)
{
  InetSocketAddress socketAdd = InetSocketAddress::ConvertFrom (srcAddress);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << ","
                        << socketAdd.GetPort () << ","
                        << packet->GetSize () << ","
                        << socketAdd.GetIpv4 () << std::endl;
}

static void
TraceUDPPacketReceived (Ptr<OutputStreamWrapper> udpReceivedStream, std::string nodeNumberStr)
{
  // Use the correct TraceSource in order to trace
  // the packet received by the UDP server.
  //
  /* Note how the path is constructed for configuring the TraceSource. NS-3
   * keeps a hierarchical list of all available modules created for the
   * simulation
   */
  Config::ConnectWithoutContext ("/NodeList/" + nodeNumberStr + "/ApplicationList/0/$ns3::UdpServer/RxWithAddresses",
                                 MakeBoundCallback (&UdpReceiverTracer, udpReceivedStream));
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;

  int flowPerHost = 20;
  int illBehavedFlowNumber = 10;
  
  std::string normalFlowInterval = "50ms"; // 20 packets/s
  std::string illBehavedFlowInterval = "16.7ms"; // 60 packets/s

  int time = 200; // run simulation for x seconds

  std::string queueDisc = "drr";
  uint32_t quantum = 500;
  bool multiHop = false;
  bool multiHost = false;
  CommandLine cmd (__FILE__);
  cmd.AddValue ("queueDisc", "The queue disc to use", queueDisc);
  cmd.AddValue ("quantum", "The quantum for the drr queue disc", quantum);
  cmd.AddValue ("multiHop", "Whether the simulation uses a multi-hop topology", multiHop);
  cmd.AddValue ("multiHost", "Whether the simulation uses 2 sending hosts (instead of 1)", multiHost);
  cmd.Parse (argc, argv);

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("DRRExample", LOG_LEVEL_DEBUG);

  NS_LOG_DEBUG("Simulation with queueDisc:" << queueDisc <<
               " flowPerHost=" << flowPerHost << " illBehavedFlowNumber=" << illBehavedFlowNumber <<
               " normalFlowInterval=" << normalFlowInterval << " illBehavedFlowInterval=" <<
               illBehavedFlowInterval << " time=" << time << " quantum=" << quantum
               << " multiHop=" << multiHop << " multiHost=" << multiHost);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  std::string dir = "outputs/drr/";

  std::string qStreamName = dir + queueDisc + "_" + std::to_string(quantum) + (!multiHost ? "" : "_multihost") + (!multiHop ? "" : "_multihop") + "_q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  std::string udpReceiverStreamName = dir + queueDisc + "_" + std::to_string(quantum) + (!multiHost ? "" : "_multihost") + (!multiHop ? "" : "_multihop") + "_receivedPacket.tr";
  Ptr<OutputStreamWrapper> udpReceiverStream;
  udpReceiverStream = asciiTraceHelper.CreateFileStream (udpReceiverStreamName);

  /* In order to run simulations in NS-3, you need to set up your network all
   * the way from the physical layer to the application layer. But don't worry!
   * NS-3 has very nice classes to help you out at every layer of the network.
   */

  /******** Create Nodes ********/
  /* Nodes are the used for end-hosts, switches etc. */
  NS_LOG_DEBUG("Creating Nodes...");

  if (!multiHop && !multiHost) {

    NodeContainer nodes;
    // create 3 nodes.
    nodes.Create (3);

    Ptr<Node> h1 = nodes.Get (0);
    Ptr<Node> s0 = nodes.Get (1);
    Ptr<Node> h2 = nodes.Get (2);

    /******** Create Channels ********/
    /* Channels are used to connect different nodes in the network. There are
     * different types of channels one can use to simulate different environments
     * such as ethernet or wireless. In our case, we would like to simulate a
     * cable that directly connects two nodes. These cables have particular
     * properties (ie. propagation delay and link rate) and they need to be set
     * before installing them on nodes.
     */
    NS_LOG_DEBUG("Configuring Channels...");

    PointToPointHelper hostLink;
    hostLink.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    hostLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    bottleneckLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    /******** Create NetDevices ********/
    NS_LOG_DEBUG("Creating NetDevices...");

    /* When channels are installed in between nodes, NS-3 creates NetDevices for
     * the associated nodes that simulate the Network Interface Cards connecting
     * the channel and the node.
     */
    // install the links created above in between correct nodes.
    NetDeviceContainer h1s0_NetDevices = hostLink.Install (h1, s0);
    NetDeviceContainer s0h2_NetDevices = bottleneckLink.Install (s0, h2);

    /******** Install Internet Stack ********/
    NS_LOG_DEBUG("Installing Internet Stack...");

    InternetStackHelper stack;
    stack.InstallAll ();

    // use the correct attribute name to set the size of the bottleneck queue.
    TrafficControlHelper tchPfifo;
    if (queueDisc == "drr") {
      tchPfifo.SetRootQueueDisc ("ns3::DrrQueueDisc", "Quantum", UintegerValue (quantum), "MaxQueueSize", UintegerValue(500), "Flows", UintegerValue(500));
    } else if (queueDisc == "sfq") {
      tchPfifo.SetRootQueueDisc ("ns3::SfqQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")), "Flows", UintegerValue(500));
    } else {
      tchPfifo.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")));
    }

    QueueDiscContainer s0h2_QueueDiscs = tchPfifo.Install (s0h2_NetDevices);
    /* Trace Bottleneck Queue Occupancy */
    s0h2_QueueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
                              MakeBoundCallback (&QueueOccupancyTracer, qStream));

    /* Set IP addresses of the nodes in the network */
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer h1s0_interfaces = address.Assign (h1s0_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer s0h2_interfaces = address.Assign (s0h2_NetDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /******** Setting up the Application ********/
    NS_LOG_DEBUG("Setting up the Application...");


    /* The receiver application */
    uint16_t receiverPort = 5001;

    // Install the receiver application on the correct host.
    UdpServerHelper receiverHelper (receiverPort);
    ApplicationContainer receiverApp = receiverHelper.Install (h2);
    receiverApp.Start (Seconds (0.0));
    receiverApp.Stop (Seconds ((double)time));


    /* The sender Application */
    for (int i = 1; i <= flowPerHost; i++) {
      UdpClientHelper sendHelper (s0h2_interfaces.GetAddress (1), receiverPort);

      sendHelper.SetAttribute ("MaxPackets", UintegerValue (100000));
      if (i == illBehavedFlowNumber) {
        sendHelper.SetAttribute ("Interval", StringValue (illBehavedFlowInterval));
      } else {
        sendHelper.SetAttribute ("Interval", StringValue (normalFlowInterval));
      }

      // NOTE: packet header is 28 bytes
      // so this results in total packet size x + 28 bytes
      // Use random bits between 12 (min size) and 560 bytes (roughly 4500 bits)
      uint32_t packetSize = 560;
      // NS_LOG_DEBUG("Flow " << i << " has random packet size between 12 and " << packetSize
      //   << " bytes, expected bits/s = " << (packetSize + 28) * 8 * (i == illBehavedFlowNumber ? 60 : 20));
      sendHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

      // Install the source application on the correct host.
      ApplicationContainer sourceApp = sendHelper.Install (h1);
      sourceApp.Start (Seconds (0.0));
      sourceApp.Stop (Seconds ((double)time));
    }
  } else if (!multiHop && multiHost){
    // Single-hop multi-host topology
    NodeContainer nodes;
    // create 4 nodes.
    nodes.Create (4);

    Ptr<Node> h1 = nodes.Get (0);
    Ptr<Node> s0 = nodes.Get (1);
    Ptr<Node> h2 = nodes.Get (2);
    Ptr<Node> h3 = nodes.Get (3);

    /******** Create Channels ********/
    /* Channels are used to connect different nodes in the network. There are
     * different types of channels one can use to simulate different environments
     * such as ethernet or wireless. In our case, we would like to simulate a
     * cable that directly connects two nodes. These cables have particular
     * properties (ie. propagation delay and link rate) and they need to be set
     * before installing them on nodes.
     */
    NS_LOG_DEBUG("Configuring Channels...");

    PointToPointHelper hostLink;
    hostLink.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    hostLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper hostLink2;
    hostLink2.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    hostLink2.SetChannelAttribute ("Delay", StringValue ("1ms"));
    hostLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    bottleneckLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    /******** Create NetDevices ********/
    NS_LOG_DEBUG("Creating NetDevices...");

    /* When channels are installed in between nodes, NS-3 creates NetDevices for
     * the associated nodes that simulate the Network Interface Cards connecting
     * the channel and the node.
     */
    // install the links created above in between correct nodes.
    NetDeviceContainer h1s0_NetDevices = hostLink.Install (h1, s0);
    NetDeviceContainer h2s0_NetDevices = hostLink2.Install (h2, s0);
    NetDeviceContainer s0h3_NetDevices = bottleneckLink.Install (s0, h3);

    /******** Install Internet Stack ********/
    NS_LOG_DEBUG("Installing Internet Stack...");

    InternetStackHelper stack;
    stack.InstallAll ();

    // use the correct attribute name to set the size of the bottleneck queue.
    TrafficControlHelper tchPfifo;
    if (queueDisc == "drr") {
      tchPfifo.SetRootQueueDisc ("ns3::DrrQueueDisc", "Quantum", UintegerValue (quantum), "MaxQueueSize", UintegerValue(500), "Flows", UintegerValue(1000));
    } else if (queueDisc == "sfq") {
      tchPfifo.SetRootQueueDisc ("ns3::SfqQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")), "Flows", UintegerValue(1000));
    } else {
      tchPfifo.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")));
    }

    QueueDiscContainer s0h3_QueueDiscs = tchPfifo.Install (s0h3_NetDevices);
    /* Trace Bottleneck Queue Occupancy */
    s0h3_QueueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
                              MakeBoundCallback (&QueueOccupancyTracer, qStream));

    /* Set IP addresses of the nodes in the network */
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer h1s0_interfaces = address.Assign (h1s0_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer h2s1_interfaces = address.Assign (h2s0_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer s0h3_interfaces = address.Assign (s0h3_NetDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /******** Setting up the Application ********/
    NS_LOG_DEBUG("Setting up the Application...");


    /* The receiver application */
    uint16_t receiverPort = 5001;

    // Install the receiver application on the correct host.
    UdpServerHelper receiverHelper (receiverPort);
    ApplicationContainer receiverApp = receiverHelper.Install (h3);
    receiverApp.Start (Seconds (0.0));
    receiverApp.Stop (Seconds ((double)time));


    /* The sender Application */
    for (int i = 1; i <= flowPerHost; i++) {
      UdpClientHelper sendHelper (s0h3_interfaces.GetAddress (1), receiverPort);

      sendHelper.SetAttribute ("MaxPackets", UintegerValue (100000));
      if (i == illBehavedFlowNumber) {
        sendHelper.SetAttribute ("Interval", StringValue (illBehavedFlowInterval));
      } else {
        sendHelper.SetAttribute ("Interval", StringValue (normalFlowInterval));
      }

      // NOTE: packet header is 28 bytes
      // so this results in total packet size x + 28 bytes
      // Use random bits between 12 (min size) and 560 bytes (roughly 4500 bits)
      uint32_t packetSize = 560;
      // NS_LOG_DEBUG("Flow " << i << " has random packet size between 12 and " << packetSize
      //   << " bytes, expected bits/s = " << (packetSize + 28) * 8 * (i == illBehavedFlowNumber ? 60 : 20));
      sendHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

      // Install the source application on the correct host.
      ApplicationContainer sourceApp = sendHelper.Install (h1);
      sourceApp.Start (Seconds (0.0));
      sourceApp.Stop (Seconds ((double)time));

      ApplicationContainer sourceApp2 = sendHelper.Install (h2);
      sourceApp2.Start (Seconds (0.025));
      sourceApp2.Stop (Seconds ((double)time));
    }
  } else if (multiHop && multiHost){
    // Multi-hop multi-host topology
    NodeContainer nodes;
    // create 5 nodes.
    nodes.Create (5);

    Ptr<Node> h1 = nodes.Get (0);
    Ptr<Node> s0 = nodes.Get (1);
    Ptr<Node> h2 = nodes.Get (2);
    Ptr<Node> s1 = nodes.Get (3);
    Ptr<Node> h3 = nodes.Get (4);

    /******** Create Channels ********/
    /* Channels are used to connect different nodes in the network. There are
     * different types of channels one can use to simulate different environments
     * such as ethernet or wireless. In our case, we would like to simulate a
     * cable that directly connects two nodes. These cables have particular
     * properties (ie. propagation delay and link rate) and they need to be set
     * before installing them on nodes.
     */
    NS_LOG_DEBUG("Configuring Channels...");

    PointToPointHelper hostLink;
    hostLink.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    hostLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper hostLink2;
    hostLink2.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    hostLink2.SetChannelAttribute ("Delay", StringValue ("1ms"));
    hostLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    bottleneckLink.SetChannelAttribute ("Delay", StringValue ("1ms"));
    bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    PointToPointHelper bottleneckLink2;
    bottleneckLink2.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    bottleneckLink2.SetChannelAttribute ("Delay", StringValue ("1ms"));
    bottleneckLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    /******** Create NetDevices ********/
    NS_LOG_DEBUG("Creating NetDevices...");

    /* When channels are installed in between nodes, NS-3 creates NetDevices for
     * the associated nodes that simulate the Network Interface Cards connecting
     * the channel and the node.
     */
    // install the links created above in between correct nodes.
    NetDeviceContainer h1s0_NetDevices = hostLink.Install (h1, s0);
    NetDeviceContainer h2s1_NetDevices = hostLink2.Install (h2, s1);
    NetDeviceContainer s0s1_NetDevices = bottleneckLink.Install (s0, s1);
    NetDeviceContainer s1h3_NetDevices = bottleneckLink2.Install (s1, h3);

    /******** Install Internet Stack ********/
    NS_LOG_DEBUG("Installing Internet Stack...");

    InternetStackHelper stack;
    stack.InstallAll ();

    // use the correct attribute name to set the size of the bottleneck queue.
    TrafficControlHelper tchPfifo;
    if (queueDisc == "drr") {
      tchPfifo.SetRootQueueDisc ("ns3::DrrQueueDisc", "Quantum", UintegerValue (quantum), "MaxQueueSize", UintegerValue(500), "Flows", UintegerValue(1000));
    } else if (queueDisc == "sfq") {
      tchPfifo.SetRootQueueDisc ("ns3::SfqQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")), "Flows", UintegerValue(1000));
    } else {
      tchPfifo.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue (QueueSize ("500p")));
    }

    QueueDiscContainer s1h3_QueueDiscs = tchPfifo.Install (s1h3_NetDevices);
    tchPfifo.Install (s0s1_NetDevices);

    /* Trace Bottleneck Queue Occupancy */
    s1h3_QueueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
                              MakeBoundCallback (&QueueOccupancyTracer, qStream));

    /* Set IP addresses of the nodes in the network */
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer h1s0_interfaces = address.Assign (h1s0_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer h2s1_interfaces = address.Assign (h2s1_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer s0s1_interfaces = address.Assign (s0s1_NetDevices);
    address.NewNetwork ();
    Ipv4InterfaceContainer s1h3_interfaces = address.Assign (s1h3_NetDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /******** Setting up the Application ********/
    NS_LOG_DEBUG("Setting up the Application...");


    /* The receiver application */
    uint16_t receiverPort = 5001;

    // Install the receiver application on the correct host.
    UdpServerHelper receiverHelper (receiverPort);
    ApplicationContainer receiverApp = receiverHelper.Install (h3);
    receiverApp.Start (Seconds (0.0));
    receiverApp.Stop (Seconds ((double)time));


    /* The sender Application */
    for (int i = 1; i <= flowPerHost; i++) {
      UdpClientHelper sendHelper (s1h3_interfaces.GetAddress (1), receiverPort);

      sendHelper.SetAttribute ("MaxPackets", UintegerValue (100000));
      if (i == illBehavedFlowNumber) {
        sendHelper.SetAttribute ("Interval", StringValue (illBehavedFlowInterval));
      } else {
        sendHelper.SetAttribute ("Interval", StringValue (normalFlowInterval));
      }

      // NOTE: packet header is 28 bytes
      // so this results in total packet size x + 28 bytes
      // Use random bits between 12 (min size) and 560 bytes (roughly 4500 bits)
      uint32_t packetSize = 560;
      // NS_LOG_DEBUG("Flow " << i << " has random packet size between 12 and " << packetSize
      //   << " bytes, expected bits/s = " << (packetSize + 28) * 8 * (i == illBehavedFlowNumber ? 60 : 20));
      sendHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

      // Install the source application on the correct host.
      ApplicationContainer sourceApp = sendHelper.Install (h1);
      sourceApp.Start (Seconds (0.0));
      sourceApp.Stop (Seconds ((double)time));

      ApplicationContainer sourceApp2 = sendHelper.Install (h2);
      sourceApp2.Start (Seconds (0.025));
      sourceApp2.Stop (Seconds ((double)time));
    }
  }

  //Flow Monitor, using for measuring delays
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();


  /* Start tracing the RTT after the connection is established */
  int portNum = 2;
  portNum += multiHop ? 1 : 0;
  portNum += multiHost ? 1 : 0;
  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceUDPPacketReceived, udpReceiverStream, std::to_string(portNum));

  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  Simulator::Run ();

  std::string flowFileName = dir + queueDisc + "_" + std::to_string(quantum) + (!multiHost ? "" : "_multihost") + (!multiHop ? "" : "_multihop") + ".xml";

  flowMonitor->SerializeToXmlFile(flowFileName, true, true);
  Simulator::Destroy ();
  return 0;
}
