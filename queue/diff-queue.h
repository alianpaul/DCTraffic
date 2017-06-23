
#ifndef DIFF_QUEUE_H
#define DIFF_QUEUE_H

#include <queue>
#include <boost/unordered_map.hpp>
#include "ns3/queue.h"
#include "ns3/flow-field.h"

namespace ns3 {

class MiceQueue;
class ElephantQueue;

class DiffQueue : public Queue {

public:
  static TypeId GetTypeId (void);
  
  DiffQueue ();
  virtual ~DiffQueue();

  void      SetMiceMaxPackets(uint32_t maxPackets);
  uint32_t  GetMiceMaxPackets() const;
  void      SetElephantMaxPackets(uint32_t maxPackets);
  uint32_t  GetElephantMaxPackets() const;

  struct ElephantFlowEntry
  {
    ElephantFlowEntry() : dropRate(0) {}
    double dropRate; 
  };

  typedef boost::unordered_map<FlowField, ElephantFlowEntry, FlowFieldBoostHash> ElephantFlowInfo;

private:
  virtual bool DoEnqueue (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoDequeue ();
  virtual Ptr<const QueueItem> DoPeek() const;

  Ptr<MiceQueue>      m_miceQueue;
  uint32_t            m_miceMaxPackets;
  uint32_t            m_miceMaxBytes;

  Ptr<ElephantQueue>  m_elephantQueue;
  uint32_t            m_elephantMaxPackets;
  uint32_t            m_elephantMaxBytes;
  
  ElephantFlowInfo    m_elephantFlowInfo; 
  //use this data structure to check wheather the flow is a mice/elephant.
  
};

}

#endif
