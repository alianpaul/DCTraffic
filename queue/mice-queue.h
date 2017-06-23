
#ifndef MICE_QUEUE_H
#define MICE_QUEUE_H

#include <queue>
#include "ns3/queue.h"

namespace ns3 {

class MiceQueue : public Queue
{

public:
  static TypeId GetTypeId ();
  
  MiceQueue ();
  virtual ~MiceQueue ();

private:
  virtual bool DoEnqueue (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoDequeue (void);
  virtual Ptr<const QueueItem> DoPeek (void) const;

  std::queue<Ptr<QueueItem> > m_packets;

}; 

}

#endif
