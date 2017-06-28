
#ifndef DIFF_QUEUE_H
#define DIFF_QUEUE_H

#include <queue>
#include <boost/unordered_map.hpp>
#include "ns3/queue.h"
#include "ns3/flow-field.h"

namespace ns3 {

class MiceQueue;
class ElephantQueue;
class QueueConfig;

class DiffQueue : public Queue {

public:
  static TypeId GetTypeId (void);
  
  DiffQueue ();
  virtual ~DiffQueue();

  /* Init the queue settings according to the queue config data(m_queueConfig)
   */
  void Init();

  void      SetMiceMaxPackets(uint32_t maxPackets);
  uint32_t  GetMiceMaxPackets() const;
  void      SetElephantMaxPackets(uint32_t maxPackets);
  uint32_t  GetElephantMaxPackets() const;

  void      SetMiceWeight(uint16_t miceW);
  uint16_t  GetMiceWeight() const;
  void      SetTotalWeight(uint16_t totalW);
  uint16_t  GetTotalWeight() const;

  void      SetQueueConfig (Ptr<QueueConfig> qc);

private:
  virtual bool DoEnqueue (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoDequeue ();
  virtual Ptr<const QueueItem> DoPeek() const;

  Ptr<MiceQueue>       m_miceQueue;
  uint32_t             m_miceMaxPackets;
  uint32_t             m_miceMaxBytes;

  Ptr<ElephantQueue>   m_elephantQueue;
  uint32_t             m_elephantMaxPackets;
  uint32_t             m_elephantMaxBytes;
  
  Ptr<QueueConfig>     m_queueConfig;

  uint16_t             m_miceWeight;
  uint16_t             m_totalWeight;
  uint16_t             m_currentQueue;
};

}

#endif
