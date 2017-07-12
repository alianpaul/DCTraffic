
#ifndef QUEUE_CONTOLLER_H
#define QUEUE_CONTOLLER_H

#include <map>
#include <vector>
#include <utility>

#include "ns3/object.h"
#include "ns3/flow-field.h"  //Can't forward declare a typedef FlowInfo_t, so must include

namespace ns3 {

class QueueConfig;
class Ipv4Address;
class DiffQueue;

class QueueController : public Object
{

public:
  static TypeId GetTypeId (void);

  QueueController(int h, int sw);
  virtual ~QueueController();

  /* Set the sw's diff queue count,(same with it' net device count)
   */
  void SetSWDiffQueueNum(int swID, int numDiffQ);
  /*
   */
  void RegisterQueueConfig(int swID, Ptr<QueueConfig> queueConfig);

  void RegisterDiffQueue(int swID, int diffQID, Ptr<DiffQueue> diffQ);
  
  void AddRouteTableEntry(int swID, Ipv4Address ipDstAddr, int swOutPort);

  void UpdateQueueConfig(int swID,  const std::vector<std::pair<FlowField, uint16_t> >& elephantFlowInfoVec);

  /* Called by matrix decoder(through call back)
   */
  void ReceiveDecodedFlow(int swID, const FlowInfo_t& flows);

private:
  typedef std::pair<Ipv4Address, int>  RouteEntry_t;
  typedef std::vector<RouteEntry_t>    RouteTable_t;

  int                                         m_numHost;
  int                                         m_numSw;
  std::vector<RouteTable_t>                   m_swRouteTable; 
  //the idx of swID is (swID - m_numHost), it stores the sw's route table(dst ip addr - port to output)
  std::vector<std::vector<Ptr<DiffQueue> > >  m_swDiffQueue;
  //the idx of swID is (swID - m_numHost), it stores the sw's queue, we use the port number to index the queue;
  std::vector<Ptr<QueueConfig> >              m_swQueueConfig;
  //1 queue config per switch
};

}


#endif
