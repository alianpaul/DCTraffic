
#include "queue-config.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("QueueConfig");
NS_OBJECT_ENSURE_REGISTERED(QueueConfig);

TypeId QueueConfig::GetTypeId()
{

  static TypeId tid = TypeId("ns3::QueueConfig")
    .SetParent<Object>()
    .SetGroupName("Openflow")
  ;
    
  return tid;
}

QueueConfig::QueueConfig(int swID) : m_swID(swID)
{}

QueueConfig::~QueueConfig()
{}



}
