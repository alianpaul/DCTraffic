
#include "diff-queue.h"

#include "ns3/log.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ethernet-header.h"

#include <cstdlib>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DiffQueue");
NS_OBJECT_ENSURE_REGISTERED(DiffQueue);

TypeId DiffQueue::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::DiffQueue")
    .SetParent<Queue> ()
    .SetGroupName("Openflow")
    .AddAttribute("MiceMaxPackets",
		  "The max number of packets in mice queue",
		  UintegerValue(100),
		  MakeUintegerAccessor (&DiffQueue::SetMiceMaxPackets,
					&DiffQueue::GetMiceMaxPackets),
		  MakeUintegerChecker<uint32_t> ())
    .AddAttribute("ElephantMaxPackets",
		  "The max number of packets in elephant queue",
		  UintegerValue(100),
		  MakeUintegerAccessor (&DiffQueue::SetElephantMaxPackets,
					&DiffQueue::GetElephantMaxPackets),
		  MakeUintegerChecker<uint32_t> ())
    .AddAttribute("MiceWeight",
		  "The mice queue weight of scheduling",
		  UintegerValue(5),
		  MakeUintegerAccessor (&DiffQueue::SetMiceWeight,
					&DiffQueue::GetMiceWeight),
		  MakeUintegerChecker<uint16_t> ())
    .AddAttribute("TotalWeight",
		  "The total queue weight of mice and elephant",
		  UintegerValue(10),
		  MakeUintegerAccessor (&DiffQueue::SetTotalWeight,
					&DiffQueue::GetTotalWeight),
		  MakeUintegerChecker<uint16_t> ())
    .AddConstructor<DiffQueue> ()
  ;

  return tid;
}

DiffQueue::DiffQueue() 
  :m_miceNPcks(0), m_miceNBytes(0),
   m_elephantNPcks(0), m_elephantNBytes(0),
   m_currentQueue(0)
{
   NS_LOG_FUNCTION(this);
}

DiffQueue::~DiffQueue()
{
}

bool 
DiffQueue::DoEnqueue(Ptr<QueueItem> item)
{
  Ptr<Packet> packet = item->GetPacket()->Copy(); //dont change original packet
  /* Remove Ethernet header first, other wise the FlowFieldFromPacket will not work
   */
  EthernetHeader header(false);
  packet->RemoveHeader(header);
  //Attention:
  //FlowFieldFromPacket was originally used by FlowEncoder::ReceiveFromOpenFlowSwtch callback, the
  //the protocol parameter was filled by this callback. But here we manually set the protocol argument to IPv4
  FlowField   flow   = FlowFieldFromPacket(packet, Ipv4L3Protocol::PROT_NUMBER);
  NS_LOG_INFO("SW " << m_swID << " port " << m_portID  <<" receive a packet from flow: " << flow);

  /*
  NS_LOG_INFO("E flow:");
  for(ElephantFlowInfo_t::const_iterator ie = m_elephantFlowInfo.begin();
      ie != m_elephantFlowInfo.end();
      ++ie)
    {
      NS_LOG_INFO(ie->first << " " << ie->second);
    }
  */

  ElephantFlowInfo_t::const_iterator ie = m_elephantFlowInfo.find(flow);
  if(ie == m_elephantFlowInfo.end())
    {
      NS_LOG_INFO("Try enqueue mice");
      if(m_miceNPcks >= m_miceMaxPackets)
	{
	  NS_LOG_INFO("The mice queue is full, drop");
	  Drop(item->GetPacket());
	  return false;
	}
      m_miceQueue.push(item);
      m_miceNPcks  += 1;
      m_miceNBytes += item->GetPacketSize();
      return true;
    }
  else
    {
      NS_LOG_INFO("Try enqueue elephant");
      if(m_elephantNPcks >= m_elephantMaxPackets)
	{
	  //The queue is full
	  NS_LOG_INFO("The elephant queue is full, drop");
	  Drop(item->GetPacket());
	  return false;
	}
      else if(m_elephantNPcks >= m_elephantMaxPackets * 0.8)
	{
	  //The queue is almost full, do active queue management. drop the packet randomly
	  //according to the drop rate of the flow
	  NS_LOG_INFO("The elephant queue is almost full, AQM");
	  float bar      = (float) (std::rand()) / (float) RAND_MAX;
	  float dropRate = ie->second; 
	  if(bar < dropRate)
	    {
	      NS_LOG_INFO("AQM drop");
	      Drop(item->GetPacket());
	      return false;
	    }
	}

      //TODO: Use the drop rate to decide whether to drop the elephant flow packets
      m_elephantQueue.push(item);
      m_elephantNPcks  += 1;
      m_elephantNBytes += item->GetPacketSize();
      return true;
    }
  
}

Ptr<QueueItem>
DiffQueue::DoDequeue()
{

  /*The steal must be sucess. Because if there is no packets in both queue.
   * DoDequeue action will not happen.
   */
  Ptr<QueueItem> item = 0;

  if( (m_currentQueue < m_miceWeight && !m_miceQueue.empty()) || 
      (m_currentQueue >= m_miceWeight && m_elephantQueue.empty()))
    {
      if(m_currentQueue < m_miceWeight) NS_LOG_INFO("Mice Turn, Mice dequeue");
      else NS_LOG_INFO("Elep Turn, Mice steal");

      NS_ASSERT(!m_miceQueue.empty());

      item = m_miceQueue.front();
	  
      m_miceNBytes -= item->GetPacketSize();
      m_miceNPcks  -= 1;
      m_miceQueue.pop();
    }
  else
    {
      if(m_currentQueue >= m_miceWeight) NS_LOG_INFO("Elep Turn, Elep dequeue");
      else NS_LOG_INFO("Mice Turn, Elep steal");
      
      NS_ASSERT(!m_elephantQueue.empty());
      
      item = m_elephantQueue.front();

      m_elephantNBytes -= item->GetPacketSize();
      m_elephantNPcks  -= 1;
      m_elephantQueue.pop();
    }

  m_currentQueue = ( ++m_currentQueue ) % m_totalWeight;
  return item;
}

Ptr<const QueueItem>
DiffQueue::DoPeek () const
{
  return 0;
}

void DiffQueue::PrintElephantFlowInfo() const
{
  std::cout << "elep flow drop rate" << std::endl;
  ElephantFlowInfo_t::const_iterator it = m_elephantFlowInfo.begin();
  for(;it != m_elephantFlowInfo.end(); ++it)
    {
      std::cout << it->first << " drop rate " << it->second << std::endl;
    }
}

}
