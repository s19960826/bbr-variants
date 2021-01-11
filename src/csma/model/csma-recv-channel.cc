/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Emmanuelle Laprise
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#include "csma-recv-channel.h"
#include "csma-recv-device.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaRecvChannel");

NS_OBJECT_ENSURE_REGISTERED (CsmaRecvChannel);

TypeId
CsmaRecvChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CsmaRecvChannel")
    .SetParent<Channel> ()
    .SetGroupName ("Csma")
    .AddConstructor<CsmaRecvChannel> ()
    .AddAttribute ("DataRate", 
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (0xffffffff)),
                   MakeDataRateAccessor (&CsmaRecvChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CsmaRecvChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

CsmaRecvChannel::CsmaRecvChannel ()
  :
    Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = IDLE;
  m_deviceList.clear ();
}

CsmaRecvChannel::~CsmaRecvChannel ()
{
  NS_LOG_FUNCTION (this);
  m_deviceList.clear ();
}

int32_t
CsmaRecvChannel::Attach (Ptr<CsmaRecvDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  CsmaRecvRec rec (device);

  m_deviceList.push_back (rec);
  return (m_deviceList.size () - 1);
}

bool
CsmaRecvChannel::Reattach (Ptr<CsmaRecvDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<CsmaRecvRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end ( ); it++)
    {
      if (it->devicePtr == device) 
        {
          if (!it->active) 
            {
              it->active = true;
              return true;
            } 
          else 
            {
              return false;
            }
        }
    }
  return false;
}

bool
CsmaRecvChannel::Reattach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  if (deviceId < m_deviceList.size ())
    {
      return false;
    }

  if (m_deviceList[deviceId].active)
    {
      return false;
    } 
  else 
    {
      m_deviceList[deviceId].active = true;
      return true;
    }
}

bool
CsmaRecvChannel::Detach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  if (deviceId < m_deviceList.size ())
    {
      if (!m_deviceList[deviceId].active)
        {
          NS_LOG_WARN ("CsmaRecvChannel::Detach(): Device is already detached (" << deviceId << ")");
          return false;
        }

      m_deviceList[deviceId].active = false;

      if ((m_state == TRANSMITTING) && (m_currentSrc == deviceId))
        {
          NS_LOG_WARN ("CsmaRecvChannel::Detach(): Device is currently" << "transmitting (" << deviceId << ")");
        }

      return true;
    } 
  else 
    {
      return false;
    }
}

bool
CsmaRecvChannel::Detach (Ptr<CsmaRecvDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<CsmaRecvRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if ((it->devicePtr == device) && (it->active)) 
        {
          it->active = false;
          return true;
        }
    }
  return false;
}

bool
CsmaRecvChannel::TransmitStart (Ptr<const Packet> p, uint32_t srcId)
{
  NS_LOG_FUNCTION (this << p << srcId);
  NS_LOG_INFO ("UID is " << p->GetUid () << ")");

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("CsmaRecvChannel::TransmitStart(): State is not IDLE");
      return false;
    }

  if (!IsActive (srcId))
    {
      NS_LOG_ERROR ("CsmaRecvChannel::TransmitStart(): Seclected source is not currently attached to network");
      return false;
    }

  NS_LOG_LOGIC ("switch to TRANSMITTING");
  m_currentPkt = p->Copy ();
  m_currentSrc = srcId;
  m_state = TRANSMITTING;
  return true;
}

bool
CsmaRecvChannel::IsActive (uint32_t deviceId)
{
  return (m_deviceList[deviceId].active);
}

bool
CsmaRecvChannel::TransmitEnd ()
{
  NS_LOG_FUNCTION (this << m_currentPkt << m_currentSrc);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == TRANSMITTING);
  m_state = PROPAGATING;

  bool retVal = true;

  if (!IsActive (m_currentSrc))
    {
      NS_LOG_ERROR ("CsmaRecvChannel::TransmitEnd(): Seclected source was detached before the end of the transmission");
      retVal = false;
    }

  NS_LOG_LOGIC ("Schedule event in " << m_delay.GetSeconds () << " sec");


  NS_LOG_LOGIC ("Receive");

  std::vector<CsmaRecvRec>::iterator it;
  uint32_t devId = 0;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++)
    {
      if (it->IsActive ())
        {
          // schedule reception events
          Simulator::ScheduleWithContext (it->devicePtr->GetNode ()->GetId (),
                                          m_delay,
                                          &CsmaRecvDevice::Receive, it->devicePtr,
                                          m_currentPkt->Copy (), m_deviceList[m_currentSrc].devicePtr);
        }
      devId++;
    }

  // also schedule for the tx side to go back to IDLE
  Simulator::Schedule (m_delay, &CsmaRecvChannel::PropagationCompleteEvent,
                       this);
  return retVal;
}

void
CsmaRecvChannel::PropagationCompleteEvent ()
{
  NS_LOG_FUNCTION (this << m_currentPkt);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == PROPAGATING);
  m_state = IDLE;
}

uint32_t
CsmaRecvChannel::GetNumActDevices (void)
{
  int numActDevices = 0;
  std::vector<CsmaRecvRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if (it->active)
        {
          numActDevices++;
        }
    }
  return numActDevices;
}

std::size_t
CsmaRecvChannel::GetNDevices (void) const
{
  return m_deviceList.size ();
}

Ptr<CsmaRecvDevice>
CsmaRecvChannel::GetCsmaDevice (std::size_t i) const
{
  return m_deviceList[i].devicePtr;
}

int32_t
CsmaRecvChannel::GetDeviceNum (Ptr<CsmaRecvDevice> device)
{
  std::vector<CsmaRecvRec>::iterator it;
  int i = 0;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if (it->devicePtr == device)
        {
          if (it->active) 
            {
              return i;
            } 
          else 
            {
              return -2;
            }
        }
      i++;
    }
  return -1;
}

bool
CsmaRecvChannel::IsBusy (void)
{
  if (m_state == IDLE) 
    {
      return false;
    } 
  else 
    {
      return true;
    }
}

DataRate
CsmaRecvChannel::GetDataRate (void)
{
  return m_bps;
}

Time
CsmaRecvChannel::GetDelay (void)
{
  return m_delay;
}

WireState
CsmaRecvChannel::GetState (void)
{
  return m_state;
}

Ptr<NetDevice>
CsmaRecvChannel::GetDevice (std::size_t i) const
{
  return GetCsmaDevice (i);
}

CsmaRecvRec::CsmaRecvRec ()
{
  active = false;
}

CsmaRecvRec::CsmaRecvRec (Ptr<CsmaRecvDevice> device)
{
  devicePtr = device; 
  active = true;
}

CsmaRecvRec::CsmaRecvRec (CsmaRecvRec const &deviceRec)
{
  devicePtr = deviceRec.devicePtr;
  active = deviceRec.active;
}

bool
CsmaRecvRec::IsActive () 
{
  return active;
}

} // namespace ns3
