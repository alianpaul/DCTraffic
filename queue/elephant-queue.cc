#include "elephant-queue.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ElephantQueue");
NS_OBJECT_ENSURE_REGISTERED(ElephantQueue);

TypeId ElephantQueue::GetTypeId(void)
{

  static TypeId tid = TypeId ("ns3::ElephantQueue")
    .SetParent<Queue> ()
    .SetGroupName ("Openflow")
    .AddConstructor<ElephantQueue> ()
  ;

  return tid;
} 

ElephantQueue::ElephantQueue () :
  Queue (),
  m_packets()
{
  NS_LOG_FUNCTION(this);
}

ElephantQueue::~ElephantQueue ()
{
  NS_LOG_FUNCTION(this);
} 

bool
ElephantQueue::DoEnqueue (Ptr<QueueItem> item)
{
  NS_LOG_FUNCTION (this << item);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  NS_LOG_INFO ("Max Capacity: " << GetMaxPackets() << " Current Size: " << GetNPackets() );
  m_packets.push (item);

  return true;
}

Ptr<QueueItem>
ElephantQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  NS_LOG_INFO ("Max Capacity: " << GetMaxPackets() << " Current Size: " << GetNPackets() );
  Ptr<QueueItem> item = m_packets.front ();
  m_packets.pop ();

  return item;
} 

Ptr<const QueueItem>
ElephantQueue::DoPeek (void) const
{

  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  return m_packets.front ();
}

}
