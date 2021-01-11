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

#include "csma-send-channel.h"
#include "csma-send-device.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaSendChannel");

NS_OBJECT_ENSURE_REGISTERED (CsmaSendChannel);

TypeId
CsmaSendChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CsmaSendChannel")
    .SetParent<Channel> ()
    .SetGroupName ("Csma")
    .AddConstructor<CsmaSendChannel> ()
    .AddAttribute ("DataRate", 
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (0xffffffff)),
                   MakeDataRateAccessor (&CsmaSendChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CsmaSendChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

CsmaSendChannel::CsmaSendChannel ()
  :
    Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = IDLE;
  m_deviceList.clear ();
}

CsmaSendChannel::~CsmaSendChannel ()
{
  NS_LOG_FUNCTION (this);
  m_deviceList.clear ();
}

int32_t
CsmaSendChannel::Attach (Ptr<CsmaSendDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  CsmaSendRec rec (device);

  m_deviceList.push_back (rec);
  return (m_deviceList.size () - 1);
}

bool
CsmaSendChannel::Reattach (Ptr<CsmaSendDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<CsmaSendRec>::iterator it;
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
CsmaSendChannel::Reattach (uint32_t deviceId)
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
CsmaSendChannel::Detach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  if (deviceId < m_deviceList.size ())
    {
      if (!m_deviceList[deviceId].active)
        {
          NS_LOG_WARN ("CsmaSendChannel::Detach(): Device is already detached (" << deviceId << ")");
          return false;
        }

      m_deviceList[deviceId].active = false;

      if ((m_state == TRANSMITTING) && (m_currentSrc == deviceId))
        {
          NS_LOG_WARN ("CsmaSendChannel::Detach(): Device is currently" << "transmitting (" << deviceId << ")");
        }

      return true;
    } 
  else 
    {
      return false;
    }
}

bool
CsmaSendChannel::Detach (Ptr<CsmaSendDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<CsmaSendRec>::iterator it;
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
CsmaSendChannel::TransmitStart (Ptr<const Packet> p, uint32_t srcId)
{
  NS_LOG_FUNCTION (this << p << srcId);
  NS_LOG_INFO ("UID is " << p->GetUid () << ")");

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("CsmaSendChannel::TransmitStart(): State is not IDLE");
      return false;
    }

  if (!IsActive (srcId))
    {
      NS_LOG_ERROR ("CsmaSendChannel::TransmitStart(): Seclected source is not currently attached to network");
      return false;
    }

  NS_LOG_LOGIC ("switch to TRANSMITTING");
  m_currentPkt = p->Copy ();
  m_currentSrc = srcId;
  m_state = TRANSMITTING;
  return true;
}

bool
CsmaSendChannel::IsActive (uint32_t deviceId)
{
  return (m_deviceList[deviceId].active);
}

bool
CsmaSendChannel::TransmitEnd ()
{
  NS_LOG_FUNCTION (this << m_currentPkt << m_currentSrc);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == TRANSMITTING);
  m_state = PROPAGATING;

  bool retVal = true;

  if (!IsActive (m_currentSrc))
    {
      NS_LOG_ERROR ("CsmaSendChannel::TransmitEnd(): Seclected source was detached before the end of the transmission");
      retVal = false;
    }

  NS_LOG_LOGIC ("Schedule event in " << m_delay.GetSeconds () << " sec");


  NS_LOG_LOGIC ("Receive");

  std::vector<CsmaSendRec>::iterator it;
  uint32_t devId = 0;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++)
    {
      if (it->IsActive ())
        {
          // schedule reception events
          Simulator::ScheduleWithContext (it->devicePtr->GetNode ()->GetId (),
                                          m_delay,
                                          &CsmaSendDevice::Receive, it->devicePtr,
                                          m_currentPkt->Copy (), m_deviceList[m_currentSrc].devicePtr);
        }
      devId++;
    }

  // also schedule for the tx side to go back to IDLE
  Simulator::Schedule (m_delay, &CsmaSendChannel::PropagationCompleteEvent,
                       this);
  return retVal;
}

void
CsmaSendChannel::PropagationCompleteEvent ()
{
  NS_LOG_FUNCTION (this << m_currentPkt);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == PROPAGATING);
  m_state = IDLE;
}

uint32_t
CsmaSendChannel::GetNumActDevices (void)
{
  int numActDevices = 0;
  std::vector<CsmaSendRec>::iterator it;
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
CsmaSendChannel::GetNDevices (void) const
{
  return m_deviceList.size ();
}

Ptr<CsmaSendDevice>
CsmaSendChannel::GetCsmaDevice (std::size_t i) const
{
  return m_deviceList[i].devicePtr;
}

int32_t
CsmaSendChannel::GetDeviceNum (Ptr<CsmaSendDevice> device)
{
  std::vector<CsmaSendRec>::iterator it;
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
CsmaSendChannel::IsBusy (void)
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
CsmaSendChannel::GetDataRate (void)
{
  return m_bps;
}

Time
CsmaSendChannel::GetDelay (void)
{
  return m_delay;
}

WireState
CsmaSendChannel::GetState (void)
{
  return m_state;
}

Ptr<NetDevice>
CsmaSendChannel::GetDevice (std::size_t i) const
{
  return GetCsmaDevice (i);
}

CsmaSendRec::CsmaSendRec ()
{
  active = false;
}

CsmaSendRec::CsmaSendRec (Ptr<CsmaSendDevice> device)
{
  devicePtr = device; 
  active = true;
}

CsmaSendRec::CsmaSendRec (CsmaSendRec const &deviceRec)
{
  devicePtr = deviceRec.devicePtr;
  active = deviceRec.active;
}

bool
CsmaSendRec::IsActive () 
{
  return active;
}

} // namespace ns3
