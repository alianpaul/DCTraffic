
#ifndef QUEUE_CONTOLLER_H
#define QUEUE_CONTOLLER_H

#include <map>

#include "ns3/object.h"

namespace ns3 {

class QueueConfig;

class QueueController : public Object
{

public:

  /* Sort the decoded flow set, pick out the elephant flow rate and calculate its 
   * drop rate. Set the swithc's QueueConfig content.
   */
  void SetQueueConfig(int swID /*Decoded flow set*/);

private:
  std::map<int, Ptr<QueueConfig> > m_queueConfigMap;
  
};

}


#endif
