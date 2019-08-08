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

#ifndef RR_QUEUE_DISC_H
#define RR_QUEUE_DISC_H

#include "ns3/queue-disc.h"

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * Queue disc implementing the RR (Round Robin) policy.
 *
 */
class RRQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief RRQueueDisc constructor
   *
   * Creates a queue with a depth of 1000 packets by default
   */
  RRQueueDisc ();

  virtual ~RRQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded
  static constexpr const char* TOO_MANY_FLOWS_DROP = "The num of flows is over limit";  //!< Packet dropped due to too many flows

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);
  /**
   * \brief Find index of value in m_rrIndex
   * Ref to http://yamayatakeshi.jp/c11%E3%81%A3%E3%81%BD%E3%81%8Fstdvector%E3%81%A7index%E3%82%92find%E3%81%97%E3%81%9F%E3%81%84/
   * @param value value you search
   * @return index of value in m_rrIndex
   */
  int16_t findIndex(uint32_t value);
  /**
   * \brief Get the index of internal queue corresponding to the flow hash
   * @param flowHash
   * @return Index of internal queue
   */
  int16_t GetQueueIndex (uint32_t flowHash);
  uint16_t m_queueNum; //!< Number of queue
  uint16_t m_rrIndex; //!< Index for round robin
  std::vector<uint32_t> m_queueHashList; //!< List of queueHash
};

} // namespace ns3

#endif /* RR_QUEUE_DISC_H */