#ifndef FLOW_DECODER_H
#define FLOW_DECODER_H

#include <iosfwd> 

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "ns3/object.h"
#include "ns3/system-condition.h"
#include "ns3/system-mutex.h"

#include "flow-field.h"
#include "dc-topology.h"
#include "graph-algo.h"
#include "work-queue.h"

namespace ns3
{

class FlowEncoder;
class SystemThread;

class FlowDecoder : public Object
{
public:
  
  FlowDecoder (Ptr<DCTopology> topo);
  virtual ~FlowDecoder ();

  void AddEncoder (Ptr<FlowEncoder> encoder);
  void DecodeFlows ();

  /* Schedule the decode event and Init containers
   */
  void Init ();
  
private:
  FlowDecoder(const FlowDecoder&);
  FlowDecoder& operator=(const FlowDecoder&);

  /*the decoded flows, key: FlowField, value: packet cnt*/
  typedef boost::unordered_map<FlowField, uint32_t, FlowFieldBoostHash>  FlowInfo_t;
  
  struct Stat_t
  {
    std::ofstream* pSaveFile;     //save decoded flow data in this file
    bool           IsAllDecoded;  //Is all flow decoded,(all flow_cnt == 0)
    unsigned       numFlow;       //Num of decoded flows
    FlowInfo_t     decodedFlowInfo;
    
    Stat_t() : pSaveFile(NULL), IsAllDecoded(true), numFlow(0)
    {}
    
  };


  typedef std::map<int, Stat_t>                               SWStat_t;

  typedef boost::unordered_set<FlowField, FlowFieldBoostHash> FlowSet_t;

  /* Get encoder by swID
   */
  Ptr<FlowEncoder>  GetEncoderByID(int swID);

  /*Initialize each decoding switch's status in the current decoding frame.*/
  void StatInit();

  /* Decode flow set pass through this single swtch.
   */
  void FlowSingleDecode    (Ptr<FlowEncoder> target);

  /* Do flow single decode on all swtch, if no new flow decoded(m_passNewFlows
   * is empty), return false;else return true.
   */
  bool FlowAllDecode();
  
  /* Update the swtch' m_curSWFlowInfo on the path
   */
  void DecodeFlowOnPath    (const Graph::Path_t& path, const FlowField& flow);

  void CounterAllDecode ();
  
  /* Decode flow packet count.
   * return the CounterSingleDecode info: iters taken, flows solved.
   */
  std::string CounterSingleDecode (Ptr<FlowEncoder> target);

  /* Construct linear equations for CounterSingleDecode
   * We must scan the whole count table to construct the linear equations of
   * this swtch,at the same time of scanning,We can know about whether all
   * flows of switch is decoded out.So we can update swtch status. 
   * e.g. num of flows decoded, is all flow decoded.
   * return false if not flow is decoded out.
   */
  bool ConstructLinearEquations (double* A[], double b[],
				 unsigned m, unsigned n,
				 Ptr<FlowEncoder> target);

  /* Clear Decoder's Info in this decoding fame.
   */
  void Clear();


  /* Worker Thread. Do the CounterDecode job
   */
  void WorkerThread();

  /*Output*/
  void OutputOriginalCounter();
  /*Output the real flows, directly measured at encoder*/
  void OutputRealFlows();
  /*Output the flows decoded by flowradar */
  void OutputDecodeInfo();
  
  std::vector<Ptr<FlowEncoder> >  m_encoders;

  /* Flow decoded on single swtches in this frame
  SWFlowInfo_t                    m_curSWFlowInfo;
  */

  /* Swtch status in this frame, include the result output file, the decoded flows.
   */
  SWStat_t                        m_swStat;
  /* The new decoded in a pass(single decode on all sw)
   */
  FlowSet_t                       m_passNewFlows;

  
  Ptr<DCTopology>                 m_topo;

  /* mutex protected work queue for multi-threads
   */
  WorkQueue<Ptr<FlowEncoder> >    m_workQueue;

  /* Worker threads
   */
  std::vector<Ptr<SystemThread> > m_workerThreads;
  /* Worker threads LOG synchronize mutex;
   */
  SystemMutex                     m_workerOutputMutex;
  /* m_numDoneWorkers mutex;
   */
  SystemMutex                     m_workerDoneMutex;
  size_t                          m_numDoneWorkers;
  /* Condition variable, wait for all workers
   */
  SystemCondition                 m_allWorkersDone;
  
  
};

}

#endif
