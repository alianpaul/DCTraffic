
#include <algorithm>

#include "diff-queue.h"
#include "queue-controller.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "queue-config.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("QueueController");
NS_OBJECT_ENSURE_REGISTERED(QueueController);


TypeId
QueueController::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::QueueController")
    .SetParent<Object>()
    .SetGroupName("Openflow");

  return tid;
}

QueueController::QueueController(int h, int sw) : m_numHost(h), m_numSw(sw)
{
  NS_LOG_FUNCTION(this<<"numHost" << m_numHost << "numSW" << m_numSw);
  m_swRouteTable.resize(sw);
  m_swDiffQueue.resize(sw);
  m_swQueueConfig.resize(sw);
}

QueueController::~QueueController()
{
}

void 
QueueController::SetSWDiffQueueNum(int swID, int numDiffQ) 
{ 
  NS_LOG_INFO("SWID " << swID << " DiffQNum " << numDiffQ << " Set");
  int isw = swID - m_numHost; 
  m_swDiffQueue[isw].resize(numDiffQ, Ptr<DiffQueue>()); 
};

void 
QueueController::RegisterDiffQueue(int swID, int diffQID, Ptr<DiffQueue> diffQ)
{
  NS_LOG_INFO("SWID " << swID << " QID " << diffQID << " Registered");
  int isw = swID - m_numHost;
  m_swDiffQueue[isw][diffQID] = diffQ;
}

void
QueueController::RegisterQueueConfig(int swID, Ptr<QueueConfig> queueConfig)
{
  NS_LOG_INFO("SWID " << swID << " queueConfig set");
  int isw = swID - m_numHost;
  m_swQueueConfig[isw] = queueConfig;
}

void
QueueController::AddRouteTableEntry(int swID, Ipv4Address ipDstAddr, int swOutPort)
{
  NS_LOG_INFO("SWID " << swID <<" DstIP " <<ipDstAddr << " OutPort " <<swOutPort);
  int isw = swID - m_numHost; 
  RouteTable_t& rt  = m_swRouteTable[isw]; 
  rt.push_back( RouteEntry_t(ipDstAddr, swOutPort) );
}

/* Private helper function, extract elephant flow info from the flows
 * after sort the flowinfo, the first threshold * flow.size() flows is the big flow 
 */
void ExtractElephantFlowInfo(const FlowInfo_t& flows,
			     float             threshold,         
			     std::vector<std::pair<FlowField, uint16_t> >& elephantFlowInfoVec, 
			     uint64_t& elephantTotalInfo, 
			     uint64_t& miceTotalInfo); 

void 
QueueController::ReceiveDecodedFlow(int swID, const FlowInfo_t& flowInfo)
{
  NS_LOG_INFO("Receive sw " << swID << " decoded flow ");
  /*TEST: print out the received flow
  for(FlowInfo_t::const_iterator it = flows.begin(); it != flows.end(); ++it)
    {
      std::cout << it->first << " " << it->second << std::endl;
    }
  */

  /*Compute flow statistics for queue config*/
  std::vector<std::pair<FlowField, uint16_t> > elephantFlowInfoVec;
  uint64_t elephantTotalInfo;
  uint64_t miceTotalInfo;
  ExtractElephantFlowInfo(flowInfo, 0.5, elephantFlowInfoVec, elephantTotalInfo, miceTotalInfo);
  
  /*Test: output elepantflows
  std::cout << "Elephant flows" << std::endl;
  for(size_t i = 0; i < elephantFlowInfoVec.size(); ++i)
    {
      std::cout << elephantFlowInfoVec[i].first << " info " << elephantFlowInfoVec[i].second << std::endl; 
    }
  */

  //TODO: 1 Calculate the queue information according to the extracted flow info.
  

  //TODO: 2 Dispatch the elephant flow info 
  UpdateQueueConfig(swID, elephantFlowInfoVec);
}

void
QueueController::UpdateQueueConfig(int swID, 
				   const std::vector<std::pair<FlowField, uint16_t> >& elephantFlowInfoVec)
{
  std::cout << "Update queue config at " << swID << std::endl;
  for(size_t i = 0; i < elephantFlowInfoVec.size(); ++i)
    {
      std::cout << elephantFlowInfoVec[i].first << " info " << elephantFlowInfoVec[i].second << std::endl; 
    }
}

struct FlowInfoGreater
{
  bool operator()(const std::pair<FlowField, uint16_t>& x1, const std::pair<FlowField, uint16_t>& x2)
  {
    return x1.second > x2.second;
  }
};

void ExtractElephantFlowInfo(const FlowInfo_t& flowInfo,
			     float             threshold,
			     std::vector<std::pair<FlowField, uint16_t> >& elephantFlowInfoVec, 
			     uint64_t& elephantTotalInfo, 
			     uint64_t& miceTotalInfo)
{

  //Current elephantFlowInfo contains all flow
  std::copy(flowInfo.begin(), flowInfo.end(), std::back_inserter(elephantFlowInfoVec));

  std::sort(elephantFlowInfoVec.begin(), elephantFlowInfoVec.end(), FlowInfoGreater());

  /*Test: print out the sorted flowinfo
  for(size_t i = 0; i < elephantFlowInfoVec.size(); ++i)
    {
      std::cout << elephantFlowInfoVec[i].first << " info " << elephantFlowInfoVec[i].second << std::endl; 
    }
  */
  
  uint64_t totalInfo = 0;
  for(size_t i = 0; i < elephantFlowInfoVec.size(); ++i)
    {
      totalInfo += elephantFlowInfoVec[i].second;
    }

  //Extract the elephantFlowInfo 
  int elephantEnd = threshold * flowInfo.size(); 
  elephantFlowInfoVec.resize(elephantEnd);
  elephantTotalInfo = 0;
  for (size_t i = 0; i < elephantFlowInfoVec.size(); ++i) 
    {
      elephantTotalInfo += elephantFlowInfoVec[i].second;
    }
  miceTotalInfo = totalInfo - elephantTotalInfo;
}


}

