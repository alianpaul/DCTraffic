
#include "diff-queue.h"
#include "mice-queue.h"
#include "elephant-queue.h"
#include "queue-config.h"
#include "ns3/log.h"
#include "ns3/ipv4-l3-protocol.h"

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
		  UintegerValue(1),
		  MakeUintegerAccessor (&DiffQueue::SetMiceWeight,
					&DiffQueue::GetMiceWeight),
		  MakeUintegerChecker<uint16_t> ())
    .AddAttribute("TotalWeight",
		  "The total queue weight of mice and elephant",
		  UintegerValue(1),
		  MakeUintegerAccessor (&DiffQueue::SetTotalWeight,
					&DiffQueue::GetTotalWeight),
		  MakeUintegerChecker<uint16_t> ())
    .AddConstructor<DiffQueue> ()
  ;

  return tid;
}

void 
DiffQueue::Init()
{
  //If the target max is bigger than the number of packets in the queue,
  //ns-3 will abort.
  m_miceQueue->SetMaxPackets(m_miceMaxPackets);
  m_elephantQueue->SetMaxPackets(m_elephantMaxPackets);
}

DiffQueue::DiffQueue() : m_currentQueue(0)
{
   NS_LOG_FUNCTION(this);
   m_miceQueue     = Create<MiceQueue>();
   m_elephantQueue = Create<ElephantQueue>();
}

DiffQueue::~DiffQueue()
{
}

void 
DiffQueue::SetMiceMaxPackets(uint32_t maxPackets)
{
  NS_LOG_FUNCTION(this << maxPackets);
  m_miceMaxPackets = maxPackets;
}

uint32_t 
DiffQueue::GetMiceMaxPackets() const
{
  return m_miceMaxPackets;
}

void
DiffQueue::SetElephantMaxPackets(uint32_t maxPackets)
{ 
  NS_LOG_FUNCTION(this << maxPackets);
  m_elephantMaxPackets = maxPackets;
}

uint32_t
DiffQueue::GetElephantMaxPackets() const
{
  return m_elephantMaxPackets;
}

void
DiffQueue::SetQueueConfig(Ptr<QueueConfig> qc)
{
  m_queueConfig = qc;
}

void
DiffQueue::SetMiceWeight(uint16_t miceW)
{
  m_miceWeight  = miceW;
}

uint16_t
DiffQueue::GetMiceWeight() const
{
  return m_miceWeight;
}

void
DiffQueue::SetTotalWeight(uint16_t totalW)
{
  m_totalWeight = totalW;
}

uint16_t
DiffQueue::GetTotalWeight() const
{
  return m_totalWeight;
}

bool 
DiffQueue::DoEnqueue(Ptr<QueueItem> item)
{
  Ptr<Packet> packet = item->GetPacket();
  FlowField   flow   = FlowFieldFromPacket(packet, Ipv4L3Protocol::PROT_NUMBER); 
  //Attention:
  //FlowFieldFromPacket was originally used by FlowEncoder::ReceiveFromOpenFlowSwtch callback, the
  //the protocol parameter was filled by this callback. But here we manually set the protocol argument to IPv4
  QueueConfig::ElephantFlowInfo_t& elephantFlowInfo = m_queueConfig->GetElephantFlowInfo();
  QueueConfig::ElephantFlowInfo_t::const_iterator it = elephantFlowInfo.find(flow);
  if(it == elephantFlowInfo.cend())
    {
      NS_LOG_INFO("mice flow packet");
      return m_miceQueue->Enqueue(item);
    }
  else
    {
      NS_LOG_INFO("elephant flow packet");
      return m_elephantQueue->Enqueue(item);
    }
  
}

Ptr<QueueItem>
DiffQueue::DoDequeue()
{

  NS_LOG_FUNCTION(this);
  Ptr<QueueItem> item = 0;
  if(m_currentQueue < m_miceWeight)
    {
      NS_LOG_LOGIC("Dequeue mice queue");
      item = m_miceQueue->Dequeue();
      if(item == 0)
	{
	  //mice queue is actually empty, so we dequeue elephant queue
	  item = m_elephantQueue->Dequeue();
	}
    }
  else 
    {
      NS_LOG_LOGIC("Dequeue elephant queue");
      item = m_elephantQueue->Dequeue();
      if (item == 0)
	{
	  //elephant queue is empty.
	  item = m_miceQueue->Dequeue();
	}
    }
  m_currentQueue = ( ++m_currentQueue ) % m_totalWeight;
  return item;
}

Ptr<const QueueItem>
DiffQueue::DoPeek () const
{
  return 0;
}

}
