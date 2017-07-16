
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

void
QueueController::ReceiveDecodedFlow(int swID, const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo)
{
  NS_LOG_INFO("SWID " << swID << " receive decoded flow info");
  
  //Compute flow statistics
  FlowStat swtchFlowStat;
  ComputeFlowStatistics(swID, flowPckByteInfo, swtchFlowStat);

  NS_LOG_INFO(swtchFlowStat);

  //Config the queue 
  ConfigQueuesOnSwtch(swID, swtchFlowStat);
}

void
QueueController::ConfigQueuesOnSwtch(int swID, const FlowStat& stat)
{
  NS_LOG_INFO("QController config qc at sw " << swID);

  Ptr<QueueConfig> qc = m_swQueueConfig[swID - m_numHost];
  if(stat.m_elephantCnt > 0)
    {
      //update queue config' elephant flow set
      const FlowInfoVec_t<PckByteCnt>& rflowinfo = stat.m_sortedFlowPckByteInfo;
      qc->Clear();

      for(size_t i = 0; i < stat.m_elephantCnt; ++i)
	{
	  qc->AddElephantFlowInfo( rflowinfo[i].first );
	}
    }

}

void
QueueController::ComputeFlowStatistics(int swID, const FlowInfoVec_t<PckByteCnt>& flowPckByteInfo, 
				       FlowStat& stat)
{

  uint64_t flowCnt = flowPckByteInfo.size();
  stat.m_elephantCnt = flowCnt * 0.5; //TODO: now just for testing 
  stat.m_miceCnt     = flowCnt - stat.m_elephantCnt;

  stat.m_sortedFlowPckByteInfo = flowPckByteInfo;
  FlowInfoVec_t<PckByteCnt>& rFlowPckByte =  stat.m_sortedFlowPckByteInfo;  
  std::sort(rFlowPckByte.begin(), 
	    rFlowPckByte.end(), 
	    PckByteCntByteGreater());
  
  //Collect elephant flow statistic
  for(size_t i = 0; i < stat.m_elephantCnt; ++i)
    {
      stat.m_elephantPckTotalCnt  += rFlowPckByte[i].second.m_packetCnt;
      stat.m_elephantByteTotalCnt += rFlowPckByte[i].second.m_byteCnt;
    }

  //Collect mice flow statistic
  for(size_t i = stat.m_elephantCnt; i < flowCnt; ++i)
    {
      stat.m_micePckTotalCnt  += rFlowPckByte[i].second.m_packetCnt;
      stat.m_miceByteTotalCnt += rFlowPckByte[i].second.m_byteCnt;
    }
  
  
}

std::ostream&
operator<<(std::ostream& os, const FlowStat& stat)
{
  os << "----------------Flow stat---------------\n";
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
  os << "------------------End-----------------";
  return os;
}



}

