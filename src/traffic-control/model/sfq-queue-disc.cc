/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
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
 * Authors: Aditya Katapadi Kamath <akamath1997@gmail.com>
 *          A Tarun Karthik <tarunkarthik999@gmail.com>
 *          Anuj Revankar <anujrevankar@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

 //Code taken from: https://github.com/AKKamath/NS3-SFQ-Implementation/blob/master/src/traffic-control/model/sfq-queue-disc.cc

#include "ns3/log.h"
#include "ns3/string.h"
#include "sfq-queue-disc.h"
#include "ns3/queue.h"
#include "ns3/random-variable-stream.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/simulator.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SfqQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (SfqFlow);

TypeId SfqFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqFlow")
    .SetParent<QueueDiscClass> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SfqFlow> ()
  ;
  return tid;
}

SfqFlow::SfqFlow ()
  : m_allot (0),
    m_status (SFQ_EMPTY_SLOT)
{
  NS_LOG_FUNCTION (this);
}

SfqFlow::~SfqFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
SfqFlow::SetAllot (uint32_t allot)
{
  NS_LOG_FUNCTION (this << allot);
  m_allot = allot;
}

int32_t
SfqFlow::GetAllot (void) const
{
  NS_LOG_FUNCTION (this);
  return m_allot;
}

void
SfqFlow::IncreaseAllot (int32_t allot)
{
  NS_LOG_FUNCTION (this << allot);
  m_allot += allot;
}

void
SfqFlow::SetStatus (FlowStatus status)
{
  NS_LOG_FUNCTION (this);
  m_status = status;
}

SfqFlow::FlowStatus
SfqFlow::GetStatus (void) const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}

NS_OBJECT_ENSURE_REGISTERED (SfqQueueDisc);

TypeId SfqQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SfqQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("10240p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("Flows",
                   "The number of queues into which the incoming packets are classified",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SfqQueueDisc::m_flows),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PerturbationTime",
                   "The time duration after which salt used as an additional input to the hash function is changed",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&SfqQueueDisc::m_perturbTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

SfqQueueDisc::SfqQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS)
{
  NS_LOG_FUNCTION (this);
}

SfqQueueDisc::~SfqQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
SfqQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  uint32_t h = item->Hash () % m_flows;

  Ptr<SfqFlow> flow;
  if (m_flowsIndices.find (h) == m_flowsIndices.end ())
    {
      NS_LOG_DEBUG ("Creating a new flow queue with index " << h);
      flow = m_flowFactory.Create<SfqFlow> ();
      Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
      qd->Initialize ();
      flow->SetQueueDisc (qd);
      AddQueueDiscClass (flow);

      m_flowsIndices[h] = GetNQueueDiscClasses () - 1;
    }
  else
    {
      flow = StaticCast<SfqFlow> (GetQueueDiscClass (m_flowsIndices[h]));
    }

  flow->GetQueueDisc ()->Enqueue (item);

  if (m_currentQueueSize >= GetMaxSize ().GetValue ()) {
      DropFromLongestQueue();
  }
  m_currentQueueSize += 1;

  NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h] << "; current queue size " << m_currentQueueSize);
  if (flow->GetStatus () == SfqFlow::SFQ_EMPTY_SLOT)
    {
      flow->SetStatus (SfqFlow::SFQ_IN_USE);
      m_flowList.push_back (flow);
    }
  return true;
}

uint32_t
SfqQueueDisc::DropFromLongestQueue (void)
{
    uint32_t maxBacklog = 0, index = 0;
    Ptr<QueueDisc> qd;

    /* Queue is full! Find the fat flow and drop packet(s) from it */
    for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
        qd = GetQueueDiscClass (i)->GetQueueDisc ();
        uint32_t bytes = qd->GetNBytes ();
        if (bytes > maxBacklog)
        {
            maxBacklog = bytes;
            index = i;
        }
    }   

    // drop just one packet from the longest queue
    qd = GetQueueDiscClass (index)->GetQueueDisc ();
    Ptr<QueueDiscItem> item;
    item = qd->GetInternalQueue (0)->Dequeue ();
    DropAfterDequeue (item, OVERLIMIT_DROP);
    m_currentQueueSize -= 1;

    NS_LOG_DEBUG ("Packet dropped from flow index " << index << "; current queue size " << m_currentQueueSize);
    return index;
}

Ptr<QueueDiscItem>
SfqQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<SfqFlow> flow;
  Ptr<QueueDiscItem> item;

  do
    {
      flow = m_flowList.front ();
      if (!flow) {
        return 0;
      }
      item = flow->GetQueueDisc ()->Dequeue ();
      if (!item)
        {
          NS_LOG_DEBUG ("Could not get a packet from the selected flow queue" << flow);
          flow->SetStatus (SfqFlow::SFQ_EMPTY_SLOT);
          m_flowList.pop_front ();
        }
      else
        {
          NS_LOG_DEBUG ("Dequeued packet from flow " << flow);
          m_flowList.pop_front ();
          m_flowList.push_back (flow);
        }
    }
  while (item == 0);

  m_currentQueueSize -= 1;
  return item;
}

bool
SfqQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("SfqQueueDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("SfqQueueDisc cannot have internal queues");
      return false;
    }
  return true;
}

void
SfqQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  m_currentQueueSize = 0;

  m_flowFactory.SetTypeId ("ns3::SfqFlow");
  m_queueDiscFactory.SetTypeId ("ns3::FifoQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 1000000)));
}


} // namespace ns3