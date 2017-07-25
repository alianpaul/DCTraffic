
#ifndef QUEUE_CONTOLLER_H
#define QUEUE_CONTOLLER_H

#include <map>
#include <vector>
#include <utility>
#include <iosfwd>

#include "ns3/object.h"
#include "ns3/flow-field.h"  

namespace ns3 {

class Ipv4Address;
class DiffQueue;

/* FlowStat is the mice elephant flow info that each queue should have(because these info is measured
 * before enqueue.).
 */

struct FlowStat  
{
  /*TODO: may compute other statistics*/
  FlowStat() : m_elephantCnt(0), m_miceCnt(0),
	       m_elephantPckTotalCnt(0), m_elephantByteTotalCnt(0),
	       m_micePckTotalCnt(0), m_miceByteTotalCnt(0)
  {}
  
  FlowInfoVec_t<PckByteCnt> m_sortedFlowPckByteInfo;
  uint32_t m_totalFlowCnt;
  uint32_t m_elephantCnt;  //elephant flows
  uint32_t m_miceCnt;      //mice flows
  uint64_t m_elephantPckTotalCnt;
  uint64_t m_elephantByteTotalCnt;
  uint64_t m_micePckTotalCnt;
  uint64_t m_miceByteTotalCnt;
};

std::ostream& operator<<(std::ostream& os, const FlowStat& stat);

class QueueController : public Object
{
public:
  static TypeId GetTypeId (void);

  QueueController(int h, int sw);
  virtual ~QueueController();

  /* Set the sw's diff queue count,(same with it' net device count)
   */
  void SetSWDiffQueueNum(int swID, int numDiffQ);

  void RegisterDiffQueue(int swID, int diffQID, Ptr<DiffQueue> diffQ);
  
  void AddRouteTableEntry(int swID, Ipv4Address ipDstAddr, int swOutPort);

  /* Callback function call by matrix decoder
   */
  void ReceiveDecodedFlow(int swID, const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo);

  
private:
  typedef std::pair<Ipv4Address, int>  RouteEntry_t;
  typedef std::vector<RouteEntry_t>    RouteTable_t;

  friend std::ostream& operator<<(std::ostream& os, const RouteTable_t& rt);
  friend bool IsIPPredicate(const Ipv4Address& ip, const RouteEntry_t e);

  /* Use computed flow statistics to config the diffqueues 
   * on the specified switch.
   */
  void ConfigQueuesOnSwtch(const std::vector<FlowStat>& queuesFlowStat, 
			   const std::vector<Ptr<DiffQueue> >& queues);
  /* Compute flow statistics for each queue on the swtch.
   * We divide the flowPckByteInfo into different group according to the routeTable.
   * Each group matches a a queue(port) of a switch
   */
  void ComputeFlowStatistics(const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo,
			     const RouteTable_t& routeTable, 
			     std::vector<FlowStat>& queuesFlowStat);

  int                                         m_numHost;
  int                                         m_numSw;
  std::vector<RouteTable_t>                   m_swRouteTable; 
  //the idx of swID is (swID - m_numHost), it stores the sw's route table(dst ip addr - port to output)
  std::vector<std::vector<Ptr<DiffQueue> > >  m_swDiffQueue;
  //the idx of swID is (swID - m_numHost), it stores the sw's queue, we use the port number to index the queue
};

std::ostream& operator<<(std::ostream& os, const QueueController::RouteTable_t& rt);

}


#endif
