/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 *         Vitor M. Eichemberger <vitor.marge@gmail.com>
 *
 * Creating a chain of N  OpenFlow 1.3 switches and a single controller CTRL.
 * Traffic flows from host H0 to host H1.
 *
 *     H0                               H1
 *     |                                 |
 * ----------   ----------           ----------
 * |  Sw 0  |---|  Sw 1  |--- ... ---| Sw N-1 |
 * ----------   ----------           ----------
 *     :            :           :         :
 *     ...................... . . . .......
 *                       :
 *                      CTRL
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ofswitch13-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/rng-seed-manager.h"

#include <vector>

using namespace ns3;

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

NS_LOG_COMPONENT_DEFINE ("DceCampusOFSwitch13");

enum CONN_TYPE {
	CSMA,
	P2P
} CONN_TYPE;

enum APP_TYPE {
	SS,
	FWM,
	NMBFS,
	NMUCS,
	FWCM,
	FWS,
	NSBFS,
	NSUCS,
	FWCS
};

static const char *apps[] =
{
		"ss",
		"fwm",
		"nm-bfs",
		"nm-ucs",
		"fwcm",
		"fws",
		"ns-bfs",
		"ns-ucs",
		"fwcs"
};

int
main (int argc, char *argv[])
{
  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  size_t nCampuses = 1;
  size_t nClientsPer = 8;
  size_t connType = CSMA;
  bool verbose = false;
  bool trace = false;
  uint32_t app = 0;
  bool realController = false;
  size_t numFlows = 0;
  uint32_t run = 0;

  CommandLine cmd;
  cmd.AddValue ("campuses", "Number of campuses", nCampuses);
  cmd.AddValue ("connType", "Type of connection between controller and switch", connType);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("trace", "Tracing traffic to files", trace);
  cmd.AddValue ("app", "Which application to use", app);
  cmd.AddValue ("realController", "Use external controller through TAP", realController);
  cmd.AddValue ("numFlows", "Number of flows (x4) to transmit", numFlows);
  cmd.AddValue ("run", "Adjust the run value", run);
  cmd.Parse (argc, argv);

  std::cout << apps[app] << "\t" << realController << "\t" << numFlows << "\t";
  RngSeedManager::SetRun(run);

  if (verbose)
    {
      LogComponentEnable ("DceCampusOFSwitch13", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
    }

  // Enabling Checksum computations
  if (realController)
  {
	  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  }
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  // Create controller first so its ID is 0 (i.e. will use files-0 space)
  NodeContainer of13ControllerNodes;
  of13ControllerNodes.Create(nCampuses);

  // Installing the tcp/ip stack into hosts
  InternetStackHelper internet;

  std::vector<NodeContainer> net1Servers;
  std::vector<Ipv4InterfaceContainer> net1Interfaces;
  std::vector<NodeContainer> net2Clients;
  std::vector<Ipv4InterfaceContainer> net22Interfaces;
  std::vector<Ipv4InterfaceContainer> net23Interfaces;
  std::vector<Ipv4InterfaceContainer> net24Interfaces;
  std::vector<Ipv4InterfaceContainer> net25Interfaces;
  std::vector<Ipv4InterfaceContainer> net26aInterfaces;
  std::vector<Ipv4InterfaceContainer> net26bInterfaces;
  std::vector<Ipv4InterfaceContainer> net26cInterfaces;
  std::vector<NodeContainer> net3Clients;
  std::vector<Ipv4InterfaceContainer> net30aInterfaces;
  std::vector<Ipv4InterfaceContainer> net30bInterfaces;
  std::vector<Ipv4InterfaceContainer> net32Interfaces;
  std::vector<Ipv4InterfaceContainer> net33aInterfaces;
  std::vector<Ipv4InterfaceContainer> net33bInterfaces;
  std::vector<NodeContainer> net0Switches;
  std::vector<NodeContainer> net1Switches;
  std::vector<NodeContainer> net2Switches;
  std::vector<NodeContainer> net3Switches;
  std::vector<NodeContainer> net4Switch;
  std::vector<NodeContainer> net5Switch;
  for (uint32_t i = 0; i < nCampuses; ++i)
  {
	  // Create the host nodes
	  NodeContainer net1s;
	  net1s.Create(4);
	  net1Servers.push_back(net1s);
	  internet.Install(net1Servers[i]);
	  net1Interfaces.push_back(Ipv4InterfaceContainer());

	  NodeContainer net2c;
	  net2c.Create(7*nClientsPer);
	  net2Clients.push_back(net2c);
	  internet.Install(net2Clients[i]);
	  net22Interfaces.push_back(Ipv4InterfaceContainer());
	  net23Interfaces.push_back(Ipv4InterfaceContainer());
	  net24Interfaces.push_back(Ipv4InterfaceContainer());
	  net25Interfaces.push_back(Ipv4InterfaceContainer());
	  net26aInterfaces.push_back(Ipv4InterfaceContainer());
	  net26bInterfaces.push_back(Ipv4InterfaceContainer());
	  net26cInterfaces.push_back(Ipv4InterfaceContainer());

	  NodeContainer net3c;
	  net3c.Create(5*nClientsPer);
	  net3Clients.push_back(net3c);
	  internet.Install(net3Clients[i]);
	  net30aInterfaces.push_back(Ipv4InterfaceContainer());
	  net30bInterfaces.push_back(Ipv4InterfaceContainer());
	  net32Interfaces.push_back(Ipv4InterfaceContainer());
	  net33aInterfaces.push_back(Ipv4InterfaceContainer());
	  net33bInterfaces.push_back(Ipv4InterfaceContainer());

	  // Create the switch nodes; stack is installed in ofswitch13 API
	  NodeContainer net0Sw;
	  net0Sw.Create(3);
	  net0Switches.push_back(net0Sw);

	  NodeContainer net1Sw;
	  net1Sw.Create(2);
	  net1Switches.push_back(net1Sw);

	  NodeContainer net2Sw;
	  net2Sw.Create(7);
	  net2Switches.push_back(net2Sw);

	  NodeContainer net3Sw;
	  net3Sw.Create(5);
	  net3Switches.push_back(net3Sw);

	  NodeContainer net4Sw;
	  net4Sw.Create(1);
	  net4Switch.push_back(net4Sw);

	  NodeContainer net5Sw;
	  net5Sw.Create(1);
	  net5Switch.push_back(net5Sw);
  }

  // Configure the CsmaHelper
  CsmaHelper csma_1gb5ms;
  csma_1gb5ms.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csma_1gb5ms.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
  CsmaHelper csma_2gb200ms;
  csma_2gb200ms.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csma_2gb200ms.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
  CsmaHelper csma_100mb1ms;
  csma_100mb1ms.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csma_100mb1ms.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  // Initialize IPv4 host helpers
  Ipv4AddressHelper ipv4switches;
  Ipv4InterfaceContainer internetIpIfaces;
  ipv4switches.SetBase ("192.168.0.0", "255.255.0.0");

  DceManagerHelper dceManager;
  dceManager.Install (of13ControllerNodes, 100);
  ApplicationContainer apps; // Holds DCE controller apps

  std::vector< Ptr<OFSwitch13Helper> > of13Helpers;
  for (uint32_t z = 0; z < nCampuses; ++z)
  {
	  // Set up controller
	  of13Helpers.push_back(CreateObject<OFSwitch13Helper> ());
	  if (realController)
	  {
		  of13Helpers[z]->SetAttribute ("ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDCSMA));
	  }
	  else
	  {
		  of13Helpers[z]->SetAttribute ("ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));
	  }
	  of13Helpers[z]->InstallExternalController (of13ControllerNodes.Get(z));

      //std::cout << "Creating Campus Network " << z << ":" << std::endl;
      // Create Net0
      //std::cout << "  SubNet [ 0 ]" << std::endl;
      NetDeviceContainer ndc0[3];
      NetDeviceContainer of13SwitchPorts0 [3];
      for (uint32_t i = 0; i < 3; ++i)
      {
          of13SwitchPorts0[i] = NetDeviceContainer ();
      }
      // Connect the three switches in a ring
      for (uint32_t i = 0; i < 3; ++i)
      {
    	  NodeContainer tmp0Sw;
    	  tmp0Sw.Add(net0Switches[z].Get(i));
    	  tmp0Sw.Add(net0Switches[z].Get((i+1)%3));
    	  ndc0[i] = csma_1gb5ms.Install(tmp0Sw);

          of13SwitchPorts0[i].Add(ndc0[i].Get(0));
          of13SwitchPorts0[(i+1)%3].Add(ndc0[i].Get(1));
      }

      // Create Net1
      //std::cout << "  SubNet [ 1 ]" << std::endl;
      NetDeviceContainer ndc1[5];
      NetDeviceContainer of13SwitchPorts1 [2];
      for (uint32_t i = 0; i < 2; ++i)
      {
    	  of13SwitchPorts1[i] = NetDeviceContainer();
      }
      // Connect 2 servers to 1 switch
      for (uint32_t i = 0; i < 4; ++i)
      {
    	  NodeContainer tmp1;
    	  int swIndex = floor(i/2);
    	  tmp1.Add(net1Switches[z].Get(swIndex));
    	  tmp1.Add(net1Servers[z].Get(i));
    	  ndc1[i] = csma_1gb5ms.Install(tmp1);

    	  of13SwitchPorts1[swIndex].Add(ndc1[i].Get(0));
    	  net1Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc1[i].Get(1))));
    	  //std::cout << swIndex << " " << net1Interfaces[z].GetAddress(i,0) << std::endl;
      }
      // Connect the 2 switches together
      NodeContainer tmp11;
      tmp11.Add(net1Servers[z].Get(0));
      tmp11.Add(net1Servers[z].Get(1));
	  ndc1[4] = csma_1gb5ms.Install(tmp11);
	  of13SwitchPorts1[0].Add(ndc1[4].Get(0));
	  of13SwitchPorts1[1].Add(ndc1[4].Get(1));

	  // Connect Net0 to Net1
	  NodeContainer net0_1;
	  net0_1.Add(net0Switches[z].Get(2));
	  net0_1.Add(net1Switches[z].Get(0));
	  NetDeviceContainer ndc0_1;
	  ndc0_1 = csma_1gb5ms.Install(net0_1);
	  of13SwitchPorts0[2].Add(ndc0_1.Get(0));
	  of13SwitchPorts1[0].Add(ndc0_1.Get(1));

	  // Connect Net0 to Net4
      NetDeviceContainer of13SwitchPorts4 = NetDeviceContainer ();
	  NodeContainer net0_4;
	  net0_4.Add(net0Switches[z].Get(1));
	  net0_4.Add(net4Switch[z].Get(0));
	  NetDeviceContainer ndc0_4;
	  ndc0_4 = csma_1gb5ms.Install(net0_4);
	  of13SwitchPorts0[1].Add(ndc0_4.Get(0));
	  of13SwitchPorts4.Add(ndc0_4.Get(1));

	  // Connect Net0 to Net5
      NetDeviceContainer of13SwitchPorts5 = NetDeviceContainer ();
	  NodeContainer net0_5;
	  net0_5.Add(net0Switches[z].Get(1));
	  net0_5.Add(net5Switch[z].Get(0));
	  NetDeviceContainer ndc0_5;
	  ndc0_5 = csma_1gb5ms.Install(net0_5);
	  of13SwitchPorts0[1].Add(ndc0_5.Get(0));
	  of13SwitchPorts5.Add(ndc0_5.Get(1));

	  // Connect Net4 to Net5
	  NodeContainer net4_5;
	  net4_5.Add(net4Switch[z].Get(0));
	  net4_5.Add(net5Switch[z].Get(0));
	  NetDeviceContainer ndc4_5;
	  ndc4_5 = csma_1gb5ms.Install(net4_5);
	  of13SwitchPorts4.Add(ndc4_5.Get(0));
	  of13SwitchPorts5.Add(ndc4_5.Get(1));

	  // Create Net2
      //std::cout << "  SubNet [ 2 ]" << std::endl;
      NetDeviceContainer of13SwitchPorts2 [7];
      for (uint32_t i = 0; i < 7; ++i)
      {
    	  of13SwitchPorts2[i] = NetDeviceContainer();
      }

	  // Connect Net4 to Net2, switch 0
	  NodeContainer net4_20;
	  net4_20.Add(net4Switch[z].Get(0));
	  net4_20.Add(net2Switches[z].Get(0));
	  NetDeviceContainer ndc4_20;
	  ndc4_20 = csma_1gb5ms.Install(net4_20);
	  of13SwitchPorts4.Add(ndc4_20.Get(0));
	  of13SwitchPorts2[0].Add(ndc4_20.Get(1));

	  // Connect Net4 to Net2, switch 1
	  NodeContainer net4_21;
	  net4_21.Add(net4Switch[z].Get(0));
	  net4_21.Add(net2Switches[z].Get(1));
	  NetDeviceContainer ndc4_21;
	  ndc4_21 = csma_1gb5ms.Install(net4_21);
	  of13SwitchPorts4.Add(ndc4_21.Get(0));
	  of13SwitchPorts2[1].Add(ndc4_21.Get(1));

	  // Connect Net2 switches 0 and 1
	  NodeContainer net2_01;
	  net2_01.Add(net2Switches[z].Get(0));
	  net2_01.Add(net2Switches[z].Get(1));
	  NetDeviceContainer ndc2_01;
	  ndc2_01 = csma_1gb5ms.Install(net2_01);
	  of13SwitchPorts2[0].Add(ndc2_01.Get(0));
	  of13SwitchPorts2[1].Add(ndc2_01.Get(1));

	  // Connect Net2 switches 0 and 2
	  NodeContainer net2_02;
	  net2_02.Add(net2Switches[z].Get(0));
	  net2_02.Add(net2Switches[z].Get(2));
	  NetDeviceContainer ndc2_02;
	  ndc2_02 = csma_1gb5ms.Install(net2_02);
	  of13SwitchPorts2[0].Add(ndc2_02.Get(0));
	  of13SwitchPorts2[2].Add(ndc2_02.Get(1));

	  // Connect Net2 switches 1 and 3
	  NodeContainer net2_13;
	  net2_13.Add(net2Switches[z].Get(1));
	  net2_13.Add(net2Switches[z].Get(3));
	  NetDeviceContainer ndc2_13;
	  ndc2_13 = csma_1gb5ms.Install(net2_13);
	  of13SwitchPorts2[1].Add(ndc2_13.Get(0));
	  of13SwitchPorts2[3].Add(ndc2_13.Get(1));

	  // Connect Net2 switches 2 and 3
	  NodeContainer net2_23;
	  net2_23.Add(net2Switches[z].Get(2));
	  net2_23.Add(net2Switches[z].Get(3));
	  NetDeviceContainer ndc2_23;
	  ndc2_23 = csma_1gb5ms.Install(net2_23);
	  of13SwitchPorts2[2].Add(ndc2_23.Get(0));
	  of13SwitchPorts2[3].Add(ndc2_23.Get(1));

	  // Connect Net2 switches 2 and 4
	  NodeContainer net2_24;
	  net2_24.Add(net2Switches[z].Get(2));
	  net2_24.Add(net2Switches[z].Get(4));
	  NetDeviceContainer ndc2_24;
	  ndc2_24 = csma_1gb5ms.Install(net2_24);
	  of13SwitchPorts2[2].Add(ndc2_24.Get(0));
	  of13SwitchPorts2[4].Add(ndc2_24.Get(1));

	  // Connect Net2 switches 3 and 5
	  NodeContainer net2_35;
	  net2_35.Add(net2Switches[z].Get(3));
	  net2_35.Add(net2Switches[z].Get(5));
	  NetDeviceContainer ndc2_35;
	  ndc2_35 = csma_1gb5ms.Install(net2_35);
	  of13SwitchPorts2[3].Add(ndc2_35.Get(0));
	  of13SwitchPorts2[5].Add(ndc2_35.Get(1));

	  // Connect Net2 switches 5 and 6
	  NodeContainer net2_56;
	  net2_56.Add(net2Switches[z].Get(5));
	  net2_56.Add(net2Switches[z].Get(6));
	  NetDeviceContainer ndc2_56;
	  ndc2_56 = csma_1gb5ms.Install(net2_56);
	  of13SwitchPorts2[5].Add(ndc2_56.Get(0));
	  of13SwitchPorts2[6].Add(ndc2_56.Get(1));

	  // Connect Net2 switch 2 with 42 clients (slots 0-41)
	  // Connect Net2 switch 3 with 42 clients (slots 42-83)
	  // Connect Net2 switch 4 with 42 clients (slots 84-125)
	  // Connect Net2 switch 5 with 42 clients (slots 126-167)
	  for (uint32_t i = 0; i < nClientsPer; ++i)
	  {
		  NodeContainer net2_2c;
		  net2_2c.Add(net2Switches[z].Get(2));
		  net2_2c.Add(net2Clients[z].Get(i));
		  NetDeviceContainer ndc2_2c;
		  ndc2_2c = csma_100mb1ms.Install(net2_2c);
		  of13SwitchPorts2[2].Add(ndc2_2c.Get(0));
    	  net22Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_2c.Get(1))));

		  NodeContainer net2_3c;
		  net2_3c.Add(net2Switches[z].Get(3));
		  net2_3c.Add(net2Clients[z].Get(i+nClientsPer));
		  NetDeviceContainer ndc2_3c;
		  ndc2_3c = csma_100mb1ms.Install(net2_3c);
		  of13SwitchPorts2[3].Add(ndc2_3c.Get(0));
    	  net23Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_3c.Get(1))));

		  NodeContainer net2_4c;
		  net2_4c.Add(net2Switches[z].Get(4));
		  net2_4c.Add(net2Clients[z].Get(i+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc2_4c;
		  ndc2_4c = csma_100mb1ms.Install(net2_4c);
		  of13SwitchPorts2[4].Add(ndc2_4c.Get(0));
    	  net24Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_4c.Get(1))));

		  NodeContainer net2_5c;
		  net2_5c.Add(net2Switches[z].Get(5));
		  net2_5c.Add(net2Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc2_5c;
		  ndc2_5c = csma_100mb1ms.Install(net2_5c);
		  of13SwitchPorts2[5].Add(ndc2_5c.Get(0));
    	  net25Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_5c.Get(1))));

		  NodeContainer net2_6ac;
		  net2_6ac.Add(net2Switches[z].Get(6));
		  net2_6ac.Add(net2Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc2_6ac;
		  ndc2_6ac = csma_100mb1ms.Install(net2_6ac);
		  of13SwitchPorts2[6].Add(ndc2_6ac.Get(0));
    	  net26aInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_6ac.Get(1))));

		  NodeContainer net2_6bc;
		  net2_6bc.Add(net2Switches[z].Get(6));
		  net2_6bc.Add(net2Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc2_6bc;
		  ndc2_6bc = csma_100mb1ms.Install(net2_6bc);
		  of13SwitchPorts2[6].Add(ndc2_6bc.Get(0));
    	  net26bInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_6bc.Get(1))));

		  NodeContainer net2_6cc;
		  net2_6cc.Add(net2Switches[z].Get(6));
		  net2_6cc.Add(net2Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc2_6cc;
		  ndc2_6cc = csma_100mb1ms.Install(net2_6cc);
		  of13SwitchPorts2[6].Add(ndc2_6cc.Get(0));
    	  net26cInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc2_6cc.Get(1))));
	  }

	  // Create Net3
      //std::cout << "  SubNet [ 3 ]" << std::endl;
      NetDeviceContainer of13SwitchPorts3 [4];
      for (uint32_t i = 0; i < 4; ++i)
      {
    	  of13SwitchPorts3[i] = NetDeviceContainer();
      }

	  // Connect Net5 to Net3, switch 0
	  NodeContainer net5_30;
	  net5_30.Add(net5Switch[z].Get(0));
	  net5_30.Add(net3Switches[z].Get(0));
	  NetDeviceContainer ndc5_30;
	  ndc5_30 = csma_1gb5ms.Install(net5_30);
	  of13SwitchPorts5.Add(ndc5_30.Get(0));
	  of13SwitchPorts3[0].Add(ndc5_30.Get(1));

	  // Connect Net5 to Net3, switch 1
	  NodeContainer net5_31;
	  net5_31.Add(net5Switch[z].Get(0));
	  net5_31.Add(net3Switches[z].Get(1));
	  NetDeviceContainer ndc5_31;
	  ndc5_31 = csma_1gb5ms.Install(net5_31);
	  of13SwitchPorts5.Add(ndc5_31.Get(0));
	  of13SwitchPorts3[1].Add(ndc5_31.Get(1));

	  // Connect Net3 switches 0 and 1
	  NodeContainer net3_01;
	  net3_01.Add(net3Switches[z].Get(0));
	  net3_01.Add(net3Switches[z].Get(1));
	  NetDeviceContainer ndc3_01;
	  ndc3_01 = csma_1gb5ms.Install(net3_01);
	  of13SwitchPorts3[0].Add(ndc3_01.Get(0));
	  of13SwitchPorts3[1].Add(ndc3_01.Get(1));

	  // Connect Net3 switches 1 and 2
	  NodeContainer net3_12;
	  net3_12.Add(net3Switches[z].Get(1));
	  net3_12.Add(net3Switches[z].Get(2));
	  NetDeviceContainer ndc3_12;
	  ndc3_12 = csma_1gb5ms.Install(net3_12);
	  of13SwitchPorts3[1].Add(ndc3_12.Get(0));
	  of13SwitchPorts3[2].Add(ndc3_12.Get(1));

	  // Connect Net3 switches 1 and 3
	  NodeContainer net3_13;
	  net3_13.Add(net3Switches[z].Get(1));
	  net3_13.Add(net3Switches[z].Get(3));
	  NetDeviceContainer ndc3_13;
	  ndc3_13 = csma_1gb5ms.Install(net3_13);
	  of13SwitchPorts3[1].Add(ndc3_13.Get(0));
	  of13SwitchPorts3[3].Add(ndc3_13.Get(1));

	  // Connect Net3 switches 2 and 3
	  NodeContainer net3_23;
	  net3_23.Add(net3Switches[z].Get(2));
	  net3_23.Add(net3Switches[z].Get(3));
	  NetDeviceContainer ndc3_23;
	  ndc3_23 = csma_1gb5ms.Install(net3_23);
	  of13SwitchPorts3[2].Add(ndc3_23.Get(0));
	  of13SwitchPorts3[3].Add(ndc3_23.Get(1));

	  // Connect Net3 switch 0 with 42 clients (slots 0-41)
	  // Connect Net3 switch 0 with 42 clients (slots 42-83)
	  // Connect Net3 switch 2 with 42 clients (slots 84-125)
	  // Connect Net3 switch 3 with 42 clients (slots 126-167)
	  // Connect Net3 switch 3 with 42 clients
	  for (uint32_t i = 0; i < nClientsPer; ++i)
	  {
		  NodeContainer net3_0ac;
		  net3_0ac.Add(net3Switches[z].Get(0));
		  net3_0ac.Add(net3Clients[z].Get(i));
		  NetDeviceContainer ndc3_0ac;
		  ndc3_0ac = csma_100mb1ms.Install(net3_0ac);
		  of13SwitchPorts3[0].Add(ndc3_0ac.Get(0));
    	  net30aInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc3_0ac.Get(1))));

		  NodeContainer net3_0bc;
		  net3_0bc.Add(net3Switches[z].Get(0));
		  net3_0bc.Add(net3Clients[z].Get(i+nClientsPer));
		  NetDeviceContainer ndc3_0bc;
		  ndc3_0bc = csma_100mb1ms.Install(net3_0bc);
		  of13SwitchPorts3[0].Add(ndc3_0bc.Get(0));
    	  net30bInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc3_0bc.Get(1))));

		  NodeContainer net3_2c;
		  net3_2c.Add(net3Switches[z].Get(2));
		  net3_2c.Add(net3Clients[z].Get(i+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc3_2c;
		  ndc3_2c = csma_100mb1ms.Install(net3_2c);
		  of13SwitchPorts3[2].Add(ndc3_2c.Get(0));
    	  net32Interfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc3_2c.Get(1))));

		  NodeContainer net3_3ac;
		  net3_3ac.Add(net3Switches[z].Get(3));
		  net3_3ac.Add(net3Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc3_3ac;
		  ndc3_3ac = csma_100mb1ms.Install(net3_3ac);
		  of13SwitchPorts3[3].Add(ndc3_3ac.Get(0));
    	  net33aInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc3_3ac.Get(1))));

		  NodeContainer net3_3bc;
		  net3_3bc.Add(net3Switches[z].Get(3));
		  net3_3bc.Add(net3Clients[z].Get(i+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
		  NetDeviceContainer ndc3_3bc;
		  ndc3_3bc = csma_100mb1ms.Install(net3_3bc);
		  of13SwitchPorts3[3].Add(ndc3_3bc.Get(0));
    	  net33bInterfaces[z].Add(ipv4switches.Assign (NetDeviceContainer(ndc3_3bc.Get(1))));
	  }
	  //std::cout << std::endl;

      // Install ports on all switches
	  //std::cout << "Installing OpenFlow switches on Net 0... ";
      for (uint32_t i = 0; i < 3; ++i)
      {
    	  of13Helpers[z]->InstallSwitch (net0Switches[z].Get(i), of13SwitchPorts0 [i]);
      }
      //std::cout << "1... ";
      for (uint32_t i = 0; i < 2; ++i)
      {
    	  of13Helpers[z]->InstallSwitch (net1Switches[z].Get(i), of13SwitchPorts1 [i]);
      }
      //std::cout << "2... ";
      for (uint32_t i = 0; i < 7; ++i)
      {
    	  of13Helpers[z]->InstallSwitch (net2Switches[z].Get(i), of13SwitchPorts2 [i]);
      }
      //std::cout << "3... ";
      for (uint32_t i = 0; i < 4; ++i)
      {
    	  of13Helpers[z]->InstallSwitch (net3Switches[z].Get(i), of13SwitchPorts3 [i]);
      }
      //std::cout << "4... ";
	  of13Helpers[z]->InstallSwitch (net4Switch[z].Get(0), of13SwitchPorts4);
      //std::cout << "5... ";
	  of13Helpers[z]->InstallSwitch (net5Switch[z].Get(0), of13SwitchPorts5);
      //std::cout << std::endl;

      // Enable datapath logs
      if (verbose)
      {
    	  of13Helpers[z]->EnableDatapathLogs ("all");
      }
      // Enable pcap traces
      if (trace)
      {
    	  of13Helpers[z]->EnableOpenFlowPcap ();
      }

      // Set up controller node application
      if (realController)
      {
    	  // TapBridge to local machine
    	  // The default configuration expects a controller on you local machine at port 6653
    	  TapBridgeHelper tapBridge;
    	  tapBridge.SetAttribute ("Mode", StringValue ("ConfigureLocal"));
    	  std::stringstream ss;
    	  for (uint32_t tapIdx = 0; tapIdx < of13Helpers[z]->m_ctrlDevs.GetN(); ++tapIdx)
    	  {
    		  ss.str("");
    		  ss << "ctrl" << tapIdx;
        	  tapBridge.Install (of13ControllerNodes.Get(z),
        			  of13Helpers[z]->m_ctrlDevs.Get(tapIdx),
        			  StringValue (ss.str()));
    	  }
      }
      else
      {
          DceApplicationHelper dce;

          dce.SetStackSize (1<<20);
          dce.SetBinary ("python2-dce");
          dce.ResetArguments ();
          dce.ResetEnvironment ();
          dce.AddEnvironment ("PATH", "/:/python2.7:/pox:/ryu");
          dce.AddEnvironment ("PYTHONHOME", "/:/python2.7:/pox:/ryu");
          dce.AddEnvironment ("PYTHONPATH", "/:/python2.7:/pox:/ryu");
          if (verbose)
          {
        	  dce.AddArgument ("-v");
          }
          dce.AddArgument ("ryu-manager");
          if (verbose)
          {
        	  dce.AddArgument ("--verbose");
          }      switch (app)
          {
          case SS:
        	  dce.AddArgument("ryu/app/simple_switch_13.py");
        	  break;
          case FWS:
        	  dce.AddArgument("ryu/app/fw_simple.py");
        	  break;
          case FWM:
        	  dce.AddArgument("ryu/app/fw_mpls.py");
        	  break;
          case NSBFS:
        	  dce.AddArgument("ryu/app/nix_simple_bfs.py");
        	  break;
          case NSUCS:
        	  dce.AddArgument("ryu/app/nix_simple_ucs.py");
        	  break;
          case NMBFS:
        	  dce.AddArgument("ryu/app/nix_mpls_bfs.py");
        	  break;
          case NMUCS:
        	  dce.AddArgument("ryu/app/nix_mpls_ucs.py");
        	  break;
          case FWCS:
        	  dce.AddArgument("ryu/app/fw_cuda_simple.py");
        	  break;
          case FWCM:
        	  dce.AddArgument("ryu/app/fw_cuda_mpls.py");
        	  break;
          default:
        	  NS_LOG_ERROR ("Invalid controller application");
          }

          apps.Add (dce.Install (of13ControllerNodes.Get(z)));
      }
  }
  apps.Start (Seconds (0.0));

  // Create Traffic Flows
  //std::cout << "Creating UDP Traffic Flows:" << std::endl;
  Config::SetDefault ("ns3::OnOffApplication::OnTime",
                      StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  Config::SetDefault ("ns3::OnOffApplication::OffTime",
                      StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",
		  	  	  	  DataRateValue(DataRate("100kb/s")));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize",
		  	  	  	  UintegerValue(1400));

  ApplicationContainer clientApps;
  ApplicationContainer sinkApps;
  Ptr<NormalRandomVariable> nrng = CreateObject<NormalRandomVariable> ();
  double startTime1 = 0.0;
  double startTime2 = 0.0;
  double startTime3 = 0.0;
  if (numFlows > 0)
  {
	  for (size_t i = 0; i < 4; ++i)
	  {
		  OnOffHelper client1 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress1 (InetSocketAddress (net1Interfaces.at(0).GetAddress(0,0), 45000+i));
		  client1.SetAttribute ("Remote", remoteAddress1);

		  ApplicationContainer clientApp1;
		  clientApp1.Add (client1.Install (net2Clients.at(0).Get(i)));
		  startTime1 = nrng->GetValue(40.2022,12.7569);
		  std::cout << startTime1 << "\t";
		  clientApp1.Start (Seconds (startTime1));
		  clientApps.Add(clientApp1);

		  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+i));
		  sinkApps.Add(sinkHelper1.Install (net1Servers.at(0).Get(0)));

		  OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress2 (InetSocketAddress (net1Interfaces.at(0).GetAddress(1,0), 45000+i));
		  client2.SetAttribute ("Remote", remoteAddress2);

		  ApplicationContainer clientApp2;
		  clientApp2.Add (client2.Install (net2Clients.at(0).Get(i+nClientsPer)));
		  startTime1 = nrng->GetValue(45.1488,15.8784);
		  std::cout << startTime1 << "\t";
		  clientApp2.Start (Seconds (startTime1));
		  clientApps.Add(clientApp2);

		  PacketSinkHelper sinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+i));
		  sinkApps.Add(sinkHelper2.Install (net1Servers.at(0).Get(1)));



		  OnOffHelper client3 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress3 (InetSocketAddress (net1Interfaces.at(0).GetAddress(2,0), 45000+i));
		  client3.SetAttribute ("Remote", remoteAddress3);

		  ApplicationContainer clientApp3;
		  clientApp3.Add (client3.Install (net3Clients.at(0).Get(i)));
		  startTime1 = nrng->GetValue(50.3700,12.4275);
		  std::cout << startTime1 << "\t";
		  clientApp3.Start (Seconds (startTime1));
		  clientApps.Add(clientApp3);

		  PacketSinkHelper sinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+i));
		  sinkApps.Add(sinkHelper3.Install (net1Servers.at(0).Get(2)));

		  OnOffHelper client4 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress4 (InetSocketAddress (net1Interfaces.at(0).GetAddress(3,0), 45000+i));
		  client4.SetAttribute ("Remote", remoteAddress4);

		  ApplicationContainer clientApp4;
		  clientApp4.Add (client4.Install (net3Clients.at(0).Get(i+nClientsPer)));
		  startTime1 = nrng->GetValue(52.0771,12.1562);
		  std::cout << startTime1 << "\t";
		  clientApp4.Start (Seconds (startTime1));
		  clientApps.Add(clientApp4);

		  PacketSinkHelper sinkHelper4 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+i));
		  sinkApps.Add(sinkHelper4.Install (net1Servers.at(0).Get(3)));
	  }
	  startTime3 = startTime1; // In case fewer flows
  }
  if (numFlows > 1)
  {
	  for (size_t i = 0; i < 4; ++i)
	  {
		  OnOffHelper client1 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress1 (InetSocketAddress (net1Interfaces.at(0).GetAddress(0,0), 45000+100+i));
		  client1.SetAttribute ("Remote", remoteAddress1);

		  ApplicationContainer clientApp1;
		  clientApp1.Add (client1.Install (net2Clients.at(0).Get(i+2*nClientsPer)));
		  startTime2 = nrng->GetValue(60.9917,8.8969);
		  std::cout << startTime2 << "\t";
		  clientApp1.Start (Seconds (startTime2));
		  clientApps.Add(clientApp1);

		  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+100+i));
		  sinkApps.Add(sinkHelper1.Install (net1Servers.at(0).Get(0)));

		  OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress2 (InetSocketAddress (net1Interfaces.at(0).GetAddress(1,0), 45000+100+i));
		  client2.SetAttribute ("Remote", remoteAddress2);

		  ApplicationContainer clientApp2;
		  clientApp2.Add (client2.Install (net2Clients.at(0).Get(i+3*nClientsPer)));
		  startTime2 = nrng->GetValue(62.2988,8.7971);
		  std::cout << startTime2 << "\t";
		  clientApp2.Start (Seconds (startTime2));
		  clientApps.Add(clientApp2);

		  PacketSinkHelper sinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+100+i));
		  sinkApps.Add(sinkHelper2.Install (net1Servers.at(0).Get(1)));



		  OnOffHelper client3 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress3 (InetSocketAddress (net1Interfaces.at(0).GetAddress(2,0), 45000+100+i));
		  client3.SetAttribute ("Remote", remoteAddress3);

		  ApplicationContainer clientApp3;
		  clientApp3.Add (client3.Install (net3Clients.at(0).Get(i+2*nClientsPer)));
		  startTime2 = nrng->GetValue(63.6185,8.9161);
		  std::cout << startTime2 << "\t";
		  clientApp3.Start (Seconds (startTime2));
		  clientApps.Add(clientApp3);

		  PacketSinkHelper sinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+100+i));
		  sinkApps.Add(sinkHelper3.Install (net1Servers.at(0).Get(2)));

		  OnOffHelper client4 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress4 (InetSocketAddress (net1Interfaces.at(0).GetAddress(3,0), 45000+100+i));
		  client4.SetAttribute ("Remote", remoteAddress4);

		  ApplicationContainer clientApp4;
		  clientApp4.Add (client4.Install (net3Clients.at(0).Get(i+3*nClientsPer)));
		  startTime2 = nrng->GetValue(66.8806,10.3544);
		  std::cout << startTime2 << "\t";
		  clientApp4.Start (Seconds (startTime2));
		  clientApps.Add(clientApp4);

		  PacketSinkHelper sinkHelper4 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+100+i));
		  sinkApps.Add(sinkHelper4.Install (net1Servers.at(0).Get(3)));
	  }
	  startTime3 = startTime2; // In case fewer flows
  }
  if (numFlows > 2)
  {
	  for (size_t i = 0; i < 4; ++i)
	  {
		  OnOffHelper client1 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress1 (InetSocketAddress (net1Interfaces.at(0).GetAddress(0,0), 45000+200+i));
		  client1.SetAttribute ("Remote", remoteAddress1);

		  ApplicationContainer clientApp1;
		  clientApp1.Add (client1.Install (net2Clients.at(0).Get(i+4*nClientsPer)));
		  startTime3 = nrng->GetValue(76.4604,8.3340);
		  std::cout << startTime3 << "\t";
		  clientApp1.Start (Seconds (startTime3));
		  clientApps.Add(clientApp1);

		  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+200+i));
		  sinkApps.Add(sinkHelper1.Install (net1Servers.at(0).Get(0)));

		  OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress2 (InetSocketAddress (net1Interfaces.at(0).GetAddress(1,0), 45000+200+i));
		  client2.SetAttribute ("Remote", remoteAddress2);

		  ApplicationContainer clientApp2;
		  clientApp2.Add (client2.Install (net2Clients.at(0).Get(i+5*nClientsPer)));
		  startTime3 = nrng->GetValue(77.6292,8.3340);
		  std::cout << startTime3 << "\t";
		  clientApp2.Start (Seconds (startTime3));
		  clientApps.Add(clientApp2);

		  PacketSinkHelper sinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+200+i));
		  sinkApps.Add(sinkHelper2.Install (net1Servers.at(0).Get(1)));



		  OnOffHelper client3 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress3 (InetSocketAddress (net1Interfaces.at(0).GetAddress(2,0), 45000+200+i));
		  client3.SetAttribute ("Remote", remoteAddress3);

		  ApplicationContainer clientApp3;
		  clientApp3.Add (client3.Install (net3Clients.at(0).Get(i+4*nClientsPer)));
		  startTime3 = nrng->GetValue(78.4683,8.4330);
		  std::cout << startTime3 << "\t";
		  clientApp3.Start (Seconds (startTime3));
		  clientApps.Add(clientApp3);

		  PacketSinkHelper sinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+200+i));
		  sinkApps.Add(sinkHelper3.Install (net1Servers.at(0).Get(2)));

		  OnOffHelper client4 ("ns3::UdpSocketFactory", Address ());
		  AddressValue remoteAddress4 (InetSocketAddress (net1Interfaces.at(0).GetAddress(3,0), 45000+200+i));
		  client4.SetAttribute ("Remote", remoteAddress4);

		  ApplicationContainer clientApp4;
		  clientApp4.Add (client4.Install (net2Clients.at(0).Get(i+6*nClientsPer)));
		  startTime3 = nrng->GetValue(80.1033,8.4256);
		  std::cout << startTime3 << "\t";
		  clientApp4.Start (Seconds (startTime3));
		  clientApps.Add(clientApp4);

		  PacketSinkHelper sinkHelper4 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 45000+200+i));
		  sinkApps.Add(sinkHelper4.Install (net1Servers.at(0).Get(3)));
	  }
	  startTime3 = startTime2; // In case fewer flows
  }
  if (numFlows > 0)
  {
	  sinkApps.Start (Seconds (0));
  }

  V4PingHelper v4ping(net1Interfaces.at(0).GetAddress(3,0));
  v4ping.SetAttribute("Verbose", BooleanValue(true));
  v4ping.SetAttribute("Size", UintegerValue(1422));
  v4ping.SetAttribute("Count", UintegerValue(2));
  ApplicationContainer pingApp1_0 = v4ping.Install(net1Servers.at(0).Get(0));
  pingApp1_0.Start(Seconds(startTime3+20.0));
  ApplicationContainer pingApp2_9 = v4ping.Install(net2Clients.at(0).Get(0+nClientsPer+nClientsPer+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
  pingApp2_9.Start(Seconds(startTime3+22.0));
  v4ping.SetAttribute("Stopper", BooleanValue(true));
  ApplicationContainer pingApp3_5 = v4ping.Install(net3Clients.at(0).Get(0+nClientsPer+nClientsPer+nClientsPer+nClientsPer));
  pingApp3_5.Start(Seconds(startTime3+30.0));

  //std::cout << "Running simulator..." << std::endl;
  Simulator::Stop(Seconds(startTime3+60.0));
  Simulator::Run ();
  if (!realController)
  {
	  Ptr<DceApplication> controller = DynamicCast<DceApplication> (of13ControllerNodes.Get(0)->GetApplication(0));
	  controller->StopExternally();
  }

  // Transmitted bytes
  uint32_t sentBytes = 0;
  uint32_t recvBytes = 0;
  for (size_t i = 0; i < clientApps.GetN(); ++i)
  {
	  Ptr<OnOffApplication> source = DynamicCast<OnOffApplication> (clientApps.Get (i));
	  sentBytes += source->m_totBytes;
  }
  for (size_t i = 0; i < sinkApps.GetN(); ++i)
  {
	  Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApps.Get (i));
	  recvBytes += sink->GetTotalRx();
  }
  TIMER_NOW(t1);
  double d1 = TIMER_DIFF (t1, t0);
  std::cout << recvBytes << "\t" << (sentBytes == 0 ? 0 : 100 * (sentBytes - recvBytes) / sentBytes) << "%\t"
		  << "\t" << recvBytes * 8.0 / Simulator::Now().GetSeconds() << "\t" << Simulator::Now().GetSeconds()
		  << "\t" << d1 << std::endl;

  Simulator::Destroy ();
}

