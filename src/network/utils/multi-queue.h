/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */

#ifndef MULTI_QUEUE_H
#define MULTI_QUEUE_H

#include "ns3/queue.h"

namespace ns3 {

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
template <typename Item>
class MultiQueue : public Queue<Item>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief MultiQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  MultiQueue ();

  virtual ~MultiQueue ();

  virtual bool Enqueue (Ptr<Item> item);
  virtual Ptr<Item> Dequeue (void);
  virtual Ptr<Item> Remove (void);
  virtual Ptr<const Item> Peek (void) const;

private:
  using Queue<Item>::begin;
  using Queue<Item>::end;
  using Queue<Item>::DoEnqueue;
  using Queue<Item>::DoDequeue;
  using Queue<Item>::DoRemove;
  using Queue<Item>::DoPeek;

  NS_LOG_TEMPLATE_DECLARE;     //!< redefinition of the log component
};


/**
 * Implementation of the templates declared above.
 */

template <typename Item>
TypeId
MultiQueue<Item>::GetTypeId (void)
{
  static TypeId tid = TypeId (("ns3::MultiQueue<" + GetTypeParamName<MultiQueue<Item> > () + ">").c_str ())
    .SetParent<Queue<Item> > ()
    .SetGroupName ("Network")
    .template AddConstructor<MultiQueue<Item> > ()
  ;
  return tid;
}

template <typename Item>
MultiQueue<Item>::MultiQueue () :
  Queue<Item> (),
  NS_LOG_TEMPLATE_DEFINE ("MultiQueue")
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
MultiQueue<Item>::~MultiQueue ()
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
bool
MultiQueue<Item>::Enqueue (Ptr<Item> item)
{
  NS_LOG_FUNCTION (this << item);

  //return DoEnqueue (end (), item);
  return DoEnqueue (item);
}

template <typename Item>
Ptr<Item>
MultiQueue<Item>::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  //Ptr<Item> item = DoDequeue (begin ());
  Ptr<Item> item = DoDequeue ();

  NS_LOG_LOGIC ("Popped " << item);

  return item;
}

template <typename Item>
Ptr<Item>
MultiQueue<Item>::Remove (void)
{
  NS_LOG_FUNCTION (this);

  //Ptr<Item> item = DoRemove (begin ());
  Ptr<Item> item = DoRemove ();

  NS_LOG_LOGIC ("Removed " << item);

  return item;
}

template <typename Item>
Ptr<const Item>
MultiQueue<Item>::Peek (void) const
{
  NS_LOG_FUNCTION (this);

  //return DoPeek (begin ());
  return DoPeek ();
}

// The following explicit template instantiation declarations prevent all the
// translation units including this header file to implicitly instantiate the
// MultiQueue<Packet> class and the MultiQueue<QueueDiscItem> class. The
// unique instances of these classes are explicitly created through the macros
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (MultiQueue,Packet) and
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (MultiQueue,QueueDiscItem), which are included
// in drop-tail-queue.cc
extern template class MultiQueue<Packet>;
extern template class MultiQueue<QueueDiscItem>;

} // namespace ns3

#endif /* DROPTAIL_H */
