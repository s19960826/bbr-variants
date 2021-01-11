/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/csma-send-device.h"
#include "ns3/csma-send-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"

#include "ns3/trace-helper.h"
#include "csma-send-helper.h"
#include "ns3/multi-queue.h"


#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaSendHelper");

CsmaSendHelper::CsmaSendHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  //m_queueFactory.SetTypeId ("ns3::MultiQueue<Packet>");
  m_deviceFactory.SetTypeId ("ns3::CsmaSendDevice");
  m_channelFactory.SetTypeId ("ns3::CsmaSendChannel");
}

void 
CsmaSendHelper::SetQueue (std::string type,
                      std::string n1, const AttributeValue &v1,
                      std::string n2, const AttributeValue &v2,
                      std::string n3, const AttributeValue &v3,
                      std::string n4, const AttributeValue &v4)
{
  QueueBase::AppendItemTypeIfNotPresent (type, "Packet");

  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
CsmaSendHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
CsmaSendHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

//set controller
void
CsmaSendHelper::SetHelpController(Ptr<ControlDecider> controller)
{
  help_controller = controller;
}

void 
CsmaSendHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type CsmaNetDevice.
  //
  Ptr<CsmaSendDevice> device = nd->GetObject<CsmaSendDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("CsmaSendHelper::EnablePcapInternal(): Device " << device << " not of type ns3::CsmaNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, 
                                                     PcapHelper::DLT_EN10MB);
  if (promiscuous)
    {
      pcapHelper.HookDefaultSink<CsmaSendDevice> (device, "PromiscSniffer", file);
    }
  else
    {
      pcapHelper.HookDefaultSink<CsmaSendDevice> (device, "Sniffer", file);
    }
}

void 
CsmaSendHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type CsmaNetDevice.
  //
  Ptr<CsmaSendDevice> device = nd->GetObject<CsmaSendDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("CsmaSendHelper::EnableAsciiInternal(): Device " << device << " not of type ns3::CsmaNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to 
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create 
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy 
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<CsmaSendDevice> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue<Packet> > queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet> > (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet> > (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet> > (queue, "Dequeue", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for 
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the 
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static 
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::CsmaSendDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::CsmaSendDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::CsmaSendDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::CsmaSendDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer
CsmaSendHelper::Install (Ptr<Node> node) const
{
  Ptr<CsmaSendChannel> channel = m_channelFactory.Create ()->GetObject<CsmaSendChannel> ();
  return Install (node, channel);
}

NetDeviceContainer
CsmaSendHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}

NetDeviceContainer
CsmaSendHelper::Install (Ptr<Node> node, Ptr<CsmaSendChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
CsmaSendHelper::Install (Ptr<Node> node, std::string channelName) const
{
  Ptr<CsmaSendChannel> channel = Names::Find<CsmaSendChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
CsmaSendHelper::Install (std::string nodeName, Ptr<CsmaSendChannel> channel) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
CsmaSendHelper::Install (std::string nodeName, std::string channelName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Ptr<CsmaSendChannel> channel = Names::Find<CsmaSendChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer 
CsmaSendHelper::Install (const NodeContainer &c) const
{
  Ptr<CsmaSendChannel> channel = m_channelFactory.Create ()->GetObject<CsmaSendChannel> ();

  return Install (c, channel);
}

NetDeviceContainer 
CsmaSendHelper::Install (const NodeContainer &c, Ptr<CsmaSendChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

NetDeviceContainer 
CsmaSendHelper::Install (const NodeContainer &c, std::string channelName) const
{
  Ptr<CsmaSendChannel> channel = Names::Find<CsmaSendChannel> (channelName);
  return Install (c, channel);
}

int64_t
CsmaSendHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<CsmaSendDevice> csma = DynamicCast<CsmaSendDevice> (netDevice);
      if (csma)
        {
          currentStream += csma->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}

Ptr<NetDevice>
CsmaSendHelper::InstallPriv (Ptr<Node> node, Ptr<CsmaSendChannel> channel) const
{
  Ptr<CsmaSendDevice> device = m_deviceFactory.Create<CsmaSendDevice> ();
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
  Ptr<Queue<Packet> > queue = m_queueFactory.Create<Queue<Packet> > ();
  device->SetQueue (queue);
  device->Attach (channel);
  // Aggregate a NetDeviceQueueInterface object
  Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface> ();
  ndqi->GetTxQueue (0)->ConnectQueueTraces (queue);
  device->AggregateObject (ndqi);
  device->SetDevController(help_controller);

  return device;
}

} // namespace ns3
