
#ifndef DIFF_QUEUE_H
#define DIFF_QUEUE_H

#include <queue>

#include "ns3/queue.h"
#include "ns3/flow-field.h"

namespace ns3 {

class DiffQueue : public Queue {

public:
  static TypeId GetTypeId (void);
  
  DiffQueue ();
  virtual ~DiffQueue();
 
  void      SetMiceMaxPackets(uint32_t maxPackets) {m_miceMaxPackets = maxPackets;}
  uint32_t  GetMiceMaxPackets() const { return m_miceMaxPackets;}
  void      SetElephantMaxPackets(uint32_t maxPackets) {m_elephantMaxPackets = maxPackets;}
  uint32_t  GetElephantMaxPackets() const {return m_elephantMaxPackets;}

  void      SetMiceWeight(uint16_t miceW) { m_miceWeight = miceW;}
  uint16_t  GetMiceWeight() const {return m_miceWeight;}
  void      SetTotalWeight(uint16_t totalW) { m_totalWeight = totalW;}
  uint16_t  GetTotalWeight() const {return m_totalWeight;}

  void      SetPortID(int portID) {m_portID = portID;};
  int       GetPortID() const {return m_portID;};
  void      SetSWID(int swID) {m_swID = swID;};
  int       GetSWID() const {return m_swID;};
 
  void      ClearElephantFlowInfo() { m_elephantFlowInfo.clear();}
  void      SetElephantFlowInfo(const FlowField& ef, float droprate) { m_elephantFlowInfo[ef] = droprate;}

private:

  virtual bool DoEnqueue (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoDequeue ();
  virtual Ptr<const QueueItem> DoPeek() const;

  typedef FlowInfoHashMap_t<float> ElephantFlowInfo_t; //currently we only store the drop rate.
  //Elephant flow info, use it to check if a flow is a elephant flow, if it is,
  //Drop the packet according to the drop rate.
  ElephantFlowInfo_t           m_elephantFlowInfo;

  std::queue<Ptr<QueueItem> >  m_miceQueue;
  std::queue<Ptr<QueueItem> >  m_elephantQueue;

  uint32_t             m_miceMaxPackets;
  uint64_t             m_miceMaxBytes;
  uint32_t             m_miceNPcks;     //current packets in the queue
  uint64_t             m_miceNBytes;    //current bytes in the queue
  

  uint32_t             m_elephantMaxPackets;
  uint64_t             m_elephantMaxBytes;
  uint32_t             m_elephantNPcks;  //current elephant packets in the queue
  uint64_t             m_elephantNBytes; //current elephant bytes in the queue
   
  uint16_t             m_miceWeight;
  uint16_t             m_totalWeight;
  uint16_t             m_currentQueue;

  int                  m_swID;
  int                  m_portID;

};

}

#endif
