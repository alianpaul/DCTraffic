
#ifndef QUEUE_CONFIG_H
#define QUEUE_CONFIG_H

#include "ns3/object.h"
#include "ns3/flow-field.h"
#include <iostream>

namespace ns3 {

/* We use this class to configure the queues on a switch.
 * Also, it contains the elephant flows table 
 */
class QueueConfig : public Object 
{

public:
  static TypeId GetTypeId (void);
  
  QueueConfig(int swID);
  virtual ~QueueConfig();

  void Clear();

  void AddElephantFlowInfo(const FlowField& flow /*We may set each elephant flow's drop rate*/);
  
  bool IsElephant(const FlowField& flow /*we may get the drop rate*/);

private:
  int                       m_swID;           //Which sw the queueConfig is installed on.
  FlowInfoHashMap_t<float>  m_elephantFlows;  
};

}

#endif
