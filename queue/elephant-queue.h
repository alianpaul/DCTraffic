
#ifndef ELEPHANT_QUEUE_H
#define ELEPHANT_QUEUE_H

#include <queue>
#include "ns3/queue.h"

namespace ns3 {

class ElephantQueue : public Queue
{
public:
  static TypeId GetTypeId ();

  ElephantQueue();
  virtual ~ElephantQueue();

private:
  virtual bool DoEnqueue (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoDequeue (void);
  virtual Ptr<const QueueItem> DoPeek (void) const;

  std::queue<Ptr<QueueItem> > m_packets;

};

}

#endif
