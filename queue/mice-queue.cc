
#include "mice-queue.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MiceQueue");
NS_OBJECT_ENSURE_REGISTERED(MiceQueue);

TypeId MiceQueue::GetTypeId(void)
{

  static TypeId tid = TypeId ("ns3::MiceQueue")
    .SetParent<Queue> ()
    .SetGroupName ("Openflow")
    .AddConstructor<MiceQueue> ()
  ;

  return tid;
} 

MiceQueue::MiceQueue () :
  Queue (),
  m_packets()
{
  NS_LOG_FUNCTION(this);
}

MiceQueue::~MiceQueue ()
{
  NS_LOG_FUNCTION(this);
} 

bool
MiceQueue::DoEnqueue (Ptr<QueueItem> item)
{
  NS_LOG_FUNCTION (this << item);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  NS_LOG_INFO ("Max Capacity: " << GetMaxPackets() << " Current Size: " << GetNPackets() );
  m_packets.push (item);

  return true;
}

Ptr<QueueItem>
MiceQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  NS_LOG_INFO ("Max Capacity: " << GetMaxPackets() << " Current Size: " << GetNPackets() );
  Ptr<QueueItem> item = m_packets.front ();
  m_packets.pop ();

  return item;
} 

Ptr<const QueueItem>
MiceQueue::DoPeek (void) const
{

  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_packets.size() == GetNPackets ());

  return m_packets.front ();
}

}
