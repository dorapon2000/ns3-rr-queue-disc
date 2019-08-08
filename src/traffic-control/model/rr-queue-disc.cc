/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "rr-queue-disc.h"
#include <algorithm>
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RRQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (RRQueueDisc);

TypeId RRQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RRQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<RRQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("1000p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("QueueNum",
                   "The number of Queue",
                   UintegerValue (10),
                   MakeUintegerAccessor (&RRQueueDisc::m_queueNum),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

RRQueueDisc::RRQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS),
    m_queueNum (10),
    m_rrIndex (0)
{
  NS_LOG_FUNCTION (this);
}

RRQueueDisc::~RRQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

int16_t
RRQueueDisc::findIndex(uint32_t value)
{
  auto iter = std::find(m_queueHashList.begin(), m_queueHashList.end(), value);
  size_t index = std::distance(m_queueHashList.begin(), iter);

  if(index == m_queueHashList.size())
    {
      return -1;
    }

  return index;
}

int16_t
RRQueueDisc::GetQueueIndex (uint32_t flowHash)
{
  NS_LOG_FUNCTION (this << flowHash);

  int16_t index = findIndex(flowHash);

  if (index == -1)
    {
      if (m_queueHashList.size() >= m_queueNum)
        {
          return -1;
        }

      m_queueHashList.push_back(flowHash);

      return findIndex(flowHash);
    }

  return index;
}

bool
RRQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      return false;
    }

  uint32_t flowHash = item->Hash(0);
  int16_t queueIndex = GetQueueIndex (flowHash);

  if (queueIndex == -1)
    {
      NS_LOG_LOGIC ("Too many flows -- dropping pkt");
      DropBeforeEnqueue (item, TOO_MANY_FLOWS_DROP);
      return false;
    }

  bool retval = GetInternalQueue (queueIndex)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval;
}

Ptr<QueueDiscItem>
RRQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  uint16_t lastRrIndex = m_rrIndex;
  while (true)
    {
      m_rrIndex = (m_rrIndex + 1) % m_queueNum;
      Ptr<QueueDiscItem> item = GetInternalQueue (m_rrIndex)->Dequeue ();

      if (item)
        {
          return item;
        }

      if (lastRrIndex == m_rrIndex)
        {
          break;
        }
    }

  NS_LOG_LOGIC ("Queue empty");
  return 0;
}

Ptr<const QueueDiscItem>
RRQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  uint16_t lastRrIndex = m_rrIndex;
  while (true)
    {
      m_rrIndex = (m_rrIndex + 1) % m_queueNum;
      Ptr<const QueueDiscItem> item = GetInternalQueue (m_rrIndex)->Peek ();

      if (item)
        {
          m_rrIndex = lastRrIndex;
          return item;
        }

      if (lastRrIndex == m_rrIndex)
        {
          break;
        }
    }

  m_rrIndex = lastRrIndex;

  NS_LOG_LOGIC ("Queue empty");
  return 0;
}

bool
RRQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("RRQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("RRQueueDisc needs no packet filter");
      return false;
    }

  if (m_queueNum == 0)
    {
      NS_LOG_ERROR ("queueNum must be larger than 0");
      return false;
    }

  if (GetNInternalQueues () < m_queueNum)
    {
      // add a DropTail queue
      while (GetNInternalQueues () < m_queueNum)
      {
        AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
      }
    }

  return true;
}

void
RRQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3