
#include <algorithm>

#include "diff-queue.h"
#include "queue-controller.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"

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
}

QueueController::~QueueController()
{
}

void 
QueueController::SetSWDiffQueueNum(int swID, int numDiffQ) 
{ 
  //NS_LOG_INFO("SWID " << swID << " DiffQNum " << numDiffQ << " Set");
  int isw = swID - m_numHost; 
  m_swDiffQueue[isw].resize(numDiffQ, Ptr<DiffQueue>()); 
};

void 
QueueController::RegisterDiffQueue(int swID, int diffQID, Ptr<DiffQueue> diffQ)
{
  //NS_LOG_INFO("SWID " << swID << " QID " << diffQID << " Registered");
  int isw = swID - m_numHost;
  NS_ASSERT( !m_swDiffQueue[isw][diffQID] ); 
  m_swDiffQueue[isw][diffQID] = diffQ;
}

 
bool IsIPPredicate(const Ipv4Address& ip, const QueueController::RouteEntry_t e)
{
  return e.first == ip;
}

void
QueueController::AddRouteTableEntry(int swID, Ipv4Address ipDstAddr, int swOutPort)
{
  //NS_LOG_INFO("SWID " << swID <<" DstIP " <<ipDstAddr << " OutPort " <<swOutPort);
  int isw = swID - m_numHost; 
  RouteTable_t& rt  = m_swRouteTable[isw];
  RouteTable_t::iterator it = std::find_if(rt.begin(), 
					   rt.end(), 
					   std::bind1st(std::ptr_fun(&IsIPPredicate), ipDstAddr));
  if(it != rt.end())
    {
      NS_ASSERT(it->second == swOutPort);
      return;
    }
  rt.push_back( RouteEntry_t(ipDstAddr, swOutPort) );
}

void
QueueController::ReceiveDecodedFlow(int swID, const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo)
{
  NS_LOG_INFO("SWID " << swID << " receive decoded flow info");

  int swIdx = swID - m_numHost;

  const RouteTable_t&                 routeTable = m_swRouteTable[swIdx];
  const std::vector<Ptr<DiffQueue> >& diffQueues = m_swDiffQueue[swIdx];

  std::vector<FlowStat> queuesFlowStat; //Flow Statistics for each queue
  queuesFlowStat.resize(diffQueues.size());

  ComputeFlowStatistics(flowPckByteInfo, routeTable, queuesFlowStat); //Compute flow statistics for each queue.

  //Print out the route table to check out
  NS_LOG_INFO("SW " << swID << " Route Table\n" << routeTable);
  
  ConfigQueuesOnSwtch(queuesFlowStat, diffQueues);  //configure each queue according to the flow statistics
  
}

void
QueueController::ConfigQueuesOnSwtch(const std::vector<FlowStat>& queuesFlowStat, 
				     const std::vector<Ptr<DiffQueue> >& queues)
{
  
  for(size_t i = 0; i < queues.size(); ++i)
    {
      const FlowStat& flowStat  = queuesFlowStat[i];    
      Ptr<DiffQueue>  diffQueue = queues[i];

      //Set the ElephantFlowInfo of the diffQueue
      diffQueue->ClearElephantFlowInfo();
      for(size_t ie = 0; ie < flowStat.m_elephantCnt; ++ie)
	{
	  const FlowField&  eflow = flowStat.m_sortedFlowPckByteInfo[ie].first;
	  //const PckByteCnt& ePB   = flowStat.m_sortedFlowPckByteInfo[ie].second;
	  
	  //TODO: Calculate the drop rate
	  float droprate = 0.f;
	  diffQueue->SetElephantFlowInfo( eflow, droprate );
	}

      //Set the elephant mice maxPackets of the diffQueue
      uint64_t totalP = flowStat.m_micePckTotalCnt + flowStat.m_elephantPckTotalCnt;
      float micePercent = (float) flowStat.m_micePckTotalCnt / (float) totalP;
      //TODO:
      float miceExpand  = 1.2;
      micePercent *= miceExpand;

      uint32_t queueTotalMaxPackets    = diffQueue->GetMaxPackets();
      uint32_t queueMiceMaxPackets     = queueTotalMaxPackets * micePercent;
      uint32_t queueElephantMaxPackets = queueTotalMaxPackets - queueMiceMaxPackets;

      NS_ASSERT(queueMiceMaxPackets >= 0);
      NS_ASSERT(queueElephantMaxPackets >= 0);

      diffQueue->SetMiceMaxPackets( queueMiceMaxPackets );
      diffQueue->SetElephantMaxPackets( queueElephantMaxPackets );

      NS_LOG_INFO("Queue " << i << " FlowStat\n" << flowStat);       
      NS_LOG_INFO("new mice: " << queueMiceMaxPackets);
      NS_LOG_INFO("new elep: " << queueElephantMaxPackets);

      
    }
}


void
QueueController::ComputeFlowStatistics(const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo, 
				       const RouteTable_t& routeTable, 
				       std::vector<FlowStat>& queuesFlowStat)
{

  //Divide flows into different queues.
  for(FlowInfoVec_t<PckByteCnt>::const_iterator it = flowPckByteInfo.begin();
      it != flowPckByteInfo.end();
      ++it)
    {
      const Ipv4Address& ipDst = Ipv4Address((it->first).ipv4dstip);

      //Using the route table to find the according queue it belongs to.
      size_t i = 0;
      for(; i < routeTable.size(); ++i)
	{
	  if(routeTable[i].first == ipDst) break;
	}
      NS_ASSERT(i < routeTable.size()); //ensure we found an entry
      int queueId = routeTable[i].second;

      queuesFlowStat[queueId].m_sortedFlowPckByteInfo.push_back(*it);
    }

  //Compute each queues flow statistics
  for(size_t i = 0; i < queuesFlowStat.size(); ++i)
    {
      FlowStat& flowStat = queuesFlowStat[i];
      std::sort(flowStat.m_sortedFlowPckByteInfo.begin(),
		flowStat.m_sortedFlowPckByteInfo.end(),
		PckByteCntByteGreater());
      
      flowStat.m_totalFlowCnt = flowStat.m_sortedFlowPckByteInfo.size();
      flowStat.m_elephantCnt  = flowStat.m_totalFlowCnt * 0.5; //TODO: How big is the threshold
      flowStat.m_miceCnt      = flowStat.m_totalFlowCnt - flowStat.m_elephantCnt;

      size_t i = 0;
      for(; i < flowStat.m_elephantCnt; ++i)
	{
	  const PckByteCnt& pb = flowStat.m_sortedFlowPckByteInfo[i].second;
	  flowStat.m_elephantPckTotalCnt  += pb.m_packetCnt;
	  flowStat.m_elephantByteTotalCnt += pb.m_byteCnt; 
	}
      
      for(; i < flowStat.m_totalFlowCnt; ++i)
	{
	  const PckByteCnt& pb = flowStat.m_sortedFlowPckByteInfo[i].second;
	  flowStat.m_micePckTotalCnt  += pb.m_packetCnt;
	  flowStat.m_miceByteTotalCnt += pb.m_byteCnt; 
	}

    }

}

std::ostream&
operator<<(std::ostream& os, const FlowStat& stat)
{
  os << "----------------Flow stat---------------\n";
  os << "total flow cnt:    " << stat.m_totalFlowCnt << "\n";
  os << "elephant flow cnt: " << stat.m_elephantCnt << "\n";
  os << "elephant byte cnt: " << stat.m_elephantByteTotalCnt << "\n";
  os << "elephant pck cnt:  " << stat.m_elephantPckTotalCnt << "\n";
  os << "mice flow cnt:     " << stat.m_miceCnt << "\n";
  os << "mice byte cnt:     " << stat.m_miceByteTotalCnt << "\n";
  os << "mice pck cnt:      " << stat.m_micePckTotalCnt << "\n";
  os << "E:\n";
  for(size_t i = 0; i < stat.m_elephantCnt; ++i)
    {
      os << stat.m_sortedFlowPckByteInfo[i].first << " " << stat.m_sortedFlowPckByteInfo[i].second << "\n";
    }
  os << "M:\n";
  for(size_t i = stat.m_elephantCnt; i < stat.m_miceCnt + stat.m_elephantCnt; ++i)
    {
      os << stat.m_sortedFlowPckByteInfo[i].first << " " << stat.m_sortedFlowPckByteInfo[i].second << "\n";
    }
  os << "----------------Flow End-----------------";
  return os;
}

std::ostream& 
operator<<(std::ostream& os, const QueueController::RouteTable_t& rt)
{
  for(size_t i = 0; i < rt.size(); ++i)
    {
      os << "Dst " << rt[i].first << " Port " << rt[i].second << "\n"; 
    }
  
  return os;
}

}

