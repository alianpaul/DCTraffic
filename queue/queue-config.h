
#ifndef QUEUE_CONFIG_H
#define QUEUE_CONFIG_H

#include <boost/unordered_map.hpp>
#include "ns3/object.h"
#include "ns3/flow-field.h"

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


  struct ElephantFlowEntry
  {
    ElephantFlowEntry() : dropRate(0.0) {}
    double dropRate; 
  };
  typedef boost::unordered_map<FlowField, ElephantFlowEntry, FlowFieldBoostHash> ElephantFlowInfo_t;


  ElephantFlowInfo_t& GetElephantFlowInfo() {return m_elephantFlowInfo;}

private:
  int                  m_swID;
  //Which sw the queueConfig is installed on.
  ElephantFlowInfo_t   m_elephantFlowInfo; 
  //use this data structure to check wheather the flow is a mice/elephant, if the flow is an elephant, 
  //what is the drop rate.
};

}

#endif
