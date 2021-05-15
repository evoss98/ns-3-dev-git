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
 *    h1-----------------s0-----------------h2
 *    h3-----------------s0
 *    ...
 *    hN-----------------s0
 *
 *  Usage (e.g.):
 *    ./waf --run 'drr'
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

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " received packet from: "
                        << socketAdd.GetPort () << std::endl;
}

static void
TraceUDPPacketReceived (Ptr<OutputStreamWrapper> udpReceivedStream)
{
  // Use the correct TraceSource in order to trace
  // the packet received by the UDP server.
  //
  /* Note how the path is constructed for configuring the TraceSource. NS-3
   * keeps a hierarchical list of all available modules created for the
   * simulation
   */
  Config::ConnectWithoutContext ("/NodeList/2/ApplicationList/0/$ns3::UdpServer/RxWithAddresses",
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

  int packetSize = 25; // bytes = 100 bits. Try random between 0 and 4500 bits
  int time = 2000; // run simulation for x seconds

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("DRRExample", LOG_LEVEL_DEBUG);

  NS_LOG_DEBUG("DRR Simulation for:" <<
               " flowPerHost=" << flowPerHost << " illBehavedFlowNumber=" << illBehavedFlowNumber <<
               " normalFlowInterval=" << normalFlowInterval << " illBehavedFlowInterval=" <<
               illBehavedFlowInterval << " time=" << time << " packetSize=" << packetSize);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  std::string dir = "outputs/drr/";

  std::string qStreamName = dir + "q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  std::string udpReceiverStreamName = dir + "receivedPacket.tr";
  Ptr<OutputStreamWrapper> udpReceiverStream;
  udpReceiverStream = asciiTraceHelper.CreateFileStream (udpReceiverStreamName);

  /* In order to run simulations in NS-3, you need to set up your network all
   * the way from the physical layer to the application layer. But don't worry!
   * NS-3 has very nice classes to help you out at every layer of the network.
   */

  /******** Create Nodes ********/
  /* Nodes are the used for end-hosts, switches etc. */
  NS_LOG_DEBUG("Creating Nodes...");

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
  hostLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("40Kbps"));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
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
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc",
                             "MaxSize", StringValue("100p"));

  tchPfifo.Install (h1s0_NetDevices);
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
  for (int i = 0; i < flowPerHost; i++) {
    UdpClientHelper sendHelper (s0h2_interfaces.GetAddress (1), receiverPort);

    sendHelper.SetAttribute ("MaxPackets", UintegerValue (100000));
    if (i == illBehavedFlowNumber) {
      sendHelper.SetAttribute ("Interval", StringValue (illBehavedFlowInterval));
    } else {
      sendHelper.SetAttribute ("Interval", StringValue (normalFlowInterval));
    }
    sendHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

    // Install the source application on the correct host.
    ApplicationContainer sourceApp = sendHelper.Install (h1);
    sourceApp.Start (Seconds (0.0));
    sourceApp.Stop (Seconds ((double)time));
  }

  /* Start tracing the RTT after the connection is established */
  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceUDPPacketReceived, udpReceiverStream);

  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  // TODO: Up until now, you have only set up the simulation environment and
  //       described what kind of events you want NS3 to simulate. However
  //       you have actually not run the simulation yet. Complete the command
  //       below to run it.
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}