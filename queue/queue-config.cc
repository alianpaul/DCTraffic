
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

void 
QueueConfig::AddElephantFlowInfo(const FlowField& flow /*We may set each elephant flow's drop rate*/) 
{ 
  NS_LOG_INFO("SW " << m_swID << " queue config add new elephant flow " << flow);
  m_elephantFlows[flow] = 0.f;
}

bool
QueueConfig::IsElephant(const FlowField& flow)
{
  NS_LOG_INFO("Is " << flow << " at SW " << m_swID << " Elephant ");
  /*
  NS_LOG_INFO("Current Table");
  for(FlowInfoHashMap_t<float>::const_iterator it = m_elephantFlows.begin();
      it != m_elephantFlows.end();
      ++it)
    {
      NS_LOG_INFO(it->first);
    }
  */
  FlowInfoHashMap_t<float>::const_iterator it = m_elephantFlows.find(flow);
  if(it == m_elephantFlows.end())
    {
      NS_LOG_INFO("No");
      return false;
    }
  
  NS_LOG_INFO("Yes");
  return true;
}

void
QueueConfig::Clear()
{
  NS_LOG_INFO("SW " << m_swID << " elephant flow set clear");
  m_elephantFlows.clear();
}

QueueConfig::QueueConfig(int swID) : m_swID(swID)
{}

QueueConfig::~QueueConfig()
{}



}
