
#include "diff-queue.h"
#include "mice-queue.h"
#include "elephant-queue.h"
#include "ns3/log.h"


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
    .AddConstructor<DiffQueue> ()
  ;

  return tid;
}

DiffQueue::DiffQueue() : m_miceMaxPackets(0)
		       , m_miceMaxBytes(0)
		       , m_elephantMaxPackets(0)
		       , m_elephantMaxBytes(0)
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
  m_miceQueue->SetMaxPackets(maxPackets);
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
  m_elephantQueue->SetMaxPackets(maxPackets);
  m_elephantMaxPackets = maxPackets;
}

uint32_t
DiffQueue::GetElephantMaxPackets() const
{
  return m_elephantMaxPackets;
}

bool 
DiffQueue::DoEnqueue(Ptr<QueueItem> item)
{
  return m_miceQueue->Enqueue(item);
}

Ptr<QueueItem>
DiffQueue::DoDequeue()
{
  return m_miceQueue->Dequeue();
}

Ptr<const QueueItem>
DiffQueue::DoPeek () const
{
  return m_miceQueue->Peek();
}

}
