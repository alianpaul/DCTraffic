#include <fstream>
#include <algorithm>
#include <sstream>

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/system-thread.h"

#include "flow-decoder.h"
#include "flow-encoder.h"
#include "flow-field.h"
#include "LSXR/lsqrDense.h"
#include "LSXR/lsmrDense.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlowDecoder");
  
FlowDecoder::FlowDecoder (Ptr<DCTopology> topo)
  : m_topo (topo)
{
}

FlowDecoder::~FlowDecoder ()
{
}
  
void
FlowDecoder::DecodeFlows ()
{
  NS_LOG_FUNCTION(Simulator::Now().GetSeconds());

  //prepare the output file.
  StatInit();

  OutputOriginalCounter();
  OutputRealFlows();
  
  /*  1. Flow decode in this frame    */
  unsigned pass = 0;
  while( FlowAllDecode() )
    {
      NS_LOG_INFO("Pass: " << pass++);
      FlowSet_t::const_iterator itFlow;
      for(itFlow = m_passNewFlows.begin();
	  itFlow != m_passNewFlows.end(); ++itFlow)
	{
	  //Hacking: the node id == the last Ipv4 address section - 1
	  int           from = ((*itFlow).ipv4srcip & 0xff) - 1;
	  int           to   = ((*itFlow).ipv4dstip & 0xff) - 1;
	  Graph::Path_t path = m_topo->GetPath(from, to);

	  DecodeFlowOnPath (path, *itFlow);
	}
    }
  
  /*   2.Counter Decode in this frame    */
  CounterAllDecode();
  
  /*   3.Output the decoded info        */
  NS_LOG_INFO("Saving the decoded data");
  OutputDecodeInfo();

  /*   4. Clear all the infos(Decoder and Encoder) in this decoding frame */
  NS_LOG_INFO("Clear FlowRadar Status");
  Clear();
  for (unsigned ith = 0; ith < m_encoders.size(); ++ith)
    {
      m_encoders[ith]->Clear();
    }

  // Schedule next decode.
  if(Simulator::Now().GetSeconds() + PERIOD < END_TIME)
    {
      NS_LOG_INFO("Waiting to decode the next period flow");
      Simulator::Schedule (Seconds(PERIOD), &FlowDecoder::DecodeFlows, this);
    }
  else
    {
      NS_LOG_INFO("Stop Decoding");
    }

}

void
FlowDecoder::OutputOriginalCounter()
{
 
 
 SWStat_t::const_iterator itsw;
 for(itsw = m_swStat.begin();
     itsw != m_swStat.end(); ++itsw)
   {
     int               swID     = itsw->first;
     const Stat_t     &swStat   = itsw->second;
     std::ofstream    &file     = *(swStat.pSaveFile);
     Ptr<FlowEncoder>  target   = GetEncoderByID(swID);

     //Output Original Counters
     file << "counters" << std::endl;
     FlowEncoder::CountTable_t::const_iterator itc;
     const FlowEncoder::CountTable_t& counterTable = target->GetCountTable();
     for(itc = counterTable.begin(); itc != counterTable.end(); ++itc)
       {
	 file << itc->packet_cnt << std::endl;
       }
   }
  
}

void
FlowDecoder::OutputRealFlows()
{

 
 SWStat_t::const_iterator itsw;
 for(itsw = m_swStat.begin();
     itsw != m_swStat.end(); ++itsw)
   {
     int               swID     = itsw->first;
     Ptr<FlowEncoder>  target   = GetEncoderByID(swID);

     //Output Real Flow Size
     std::string rffilename = "sw-";
     std::stringstream ss;
     ss << rffilename << swID << "-t-"<<Simulator::Now().GetSeconds()<<"-real-flow.txt";
     ss >> rffilename;
     std::ofstream rffile(rffilename.c_str());
     FlowEncoder::FlowInfo_t::const_iterator itf; 
     const FlowEncoder::FlowInfo_t& realFlows = target->GetRealFlowCounter();
     for(itf = realFlows.begin(); itf != realFlows.end(); ++itf)
       {
	 rffile << itf->first << " " << itf->second << std::endl;
       }
   }
}

void
FlowDecoder::OutputDecodeInfo()
{
  
  SWStat_t::const_iterator itsw;
  for(itsw = m_swStat.begin();
      itsw != m_swStat.end(); ++itsw)
    {
      int               swID     = itsw->first;
      const Stat_t     &swStat   = itsw->second;
      std::ofstream    &file     = *(swStat.pSaveFile);
      Ptr<FlowEncoder>  target   = GetEncoderByID(swID);
      if(!file)
	{
	  NS_LOG_ERROR("Decoded flow file is not open");
	}

      file << "flows" <<std::endl;
      //file <<"Time: "         << Simulator::Now().GetSeconds() << std::endl;
      file <<"IsAllDecoded: " << swStat.IsAllDecoded           << std::endl;
      file <<"FlowCnt: "      << swStat.numFlow                << std::endl;
      
      // Output All decoded flows
      // srcip dstip port srcport dstport hashpos1 hashpos2 ...
      //
      const FlowInfo_t &flowInfo = itsw->second.decodedFlowInfo;
      FlowInfo_t::const_iterator itFlow;
      for(itFlow = flowInfo.begin(); itFlow != flowInfo.end(); ++itFlow)
	{
	  file << itFlow->first; // << itFlow->second << std::endl;
	  std::vector<uint32_t> idxs =  target->GetCountTableIdx( itFlow->first );
	  for(size_t i = 0; i < idxs.size(); ++i)
	    {
	      file << " " << idxs[i];
	    }
	  file << std::endl;
	}
    }
  
}
  
bool
FlowDecoder::FlowAllDecode()
{
  
  m_passNewFlows.clear();
  for (unsigned ith = 0; ith < m_encoders.size(); ++ith)
    {
      FlowSingleDecode (m_encoders[ith]);
    }

  return ( m_passNewFlows.empty() ? false : true);
}
  
void
FlowDecoder::Init()
{
  
  for(size_t ith = 0; ith < NUM_THREAD; ++ith)
    {
      Ptr<SystemThread> workerThread
	= Create<SystemThread>( MakeCallback(&FlowDecoder::WorkerThread, this) );
      m_workerThreads.push_back (workerThread);
    }
  
  Simulator::Schedule (Seconds(PERIOD), &FlowDecoder::DecodeFlows, this);
}

void
FlowDecoder::AddEncoder (Ptr<FlowEncoder> encoder)
{
  m_encoders.push_back(encoder);
}


Ptr<FlowEncoder>
FlowDecoder::GetEncoderByID (int swID)
{
  Ptr<FlowEncoder> e;
  for(unsigned ith = 0; ith < m_encoders.size(); ++ith)
    {
      e = m_encoders[ith];
      if( e->GetID() == swID)
	break;
    }

  return e;
}

void 
FlowDecoder::StatInit()
{
  for(unsigned ith = 0; ith < m_encoders.size(); ++ith)
    {
      const int swID = m_encoders[ith]->GetID();
      
      //m_curSWFlowInfo[swID] = FlowInfo_t();
      
      std::string filename = "sw-";
      std::string time     = "-t-";
      std::stringstream ss;
      ss << filename << swID << time << Simulator::Now().GetSeconds() << ".txt";
      filename.clear();
      ss >> filename;

      m_swStat[swID] = Stat_t();
      
      if( !(*(m_swStat[swID].pSaveFile
	      = new std::ofstream(filename.c_str()))))
	{
	  NS_LOG_ERROR("Switch save file open failed");
	  return;
	}
      
    }
}
  
//algorithm find_if predicate
bool IsFlowCountOne(const FlowEncoder::CountTableEntry& e)
{
  return (e.flow_cnt == (uint8_t)1);
}
void
FlowDecoder::FlowSingleDecode(Ptr<FlowEncoder> target)
{

  int swID = target->GetID();

  FlowEncoder::CountTable_t&          countTable   = target->GetCountTable();
  FlowEncoder::CountTable_t::iterator itEntry;
  while( (itEntry = std::find_if(countTable.begin(), countTable.end(),
				 IsFlowCountOne))
	!= countTable.end() )
    {
      //Finded a pure cell
      FlowField flow      = (*itEntry).GetFlow();
      
      uint32_t  packetCnt = (*itEntry).packet_cnt;
      //NS_LOG_INFO ("Flow: "<< flow );
      
      //m_curSWFlowInfo[swID][flow] = packetCnt;
      m_swStat[swID].decodedFlowInfo[flow] = packetCnt;

      /* If it's a new flow doesn't collected among switches before,
       * add to m_passNewFlows
       */
      if (m_passNewFlows.find(flow) == m_passNewFlows.end())
	{
	  m_passNewFlows.insert(flow);
	}

      target->ClearFlowInCountTable(flow);
      
    }  
}

void
FlowDecoder::DecodeFlowOnPath (const Graph::Path_t& path, const FlowField& flow)
{
  
  unsigned lst = path.size() - 1;
  for(unsigned ith = 0; ith < lst; ++ith)
    {
      int          swID          = path[ith].dst;
      //FlowInfo_t&  swDcdFlowInfo = m_curSWFlowInfo.at (swID);
      FlowInfo_t& swDcdFlowInfo = m_swStat[swID].decodedFlowInfo;
      
      
      FlowInfo_t::iterator itFlow;
      if ( (itFlow = swDcdFlowInfo.find (flow)) == swDcdFlowInfo.end() )
	{
	  /*This switch doesn't decoded the flow before, but the flow must go 
	   *through this switch. Check the sw's flowfilter to make sure that 
	   *the flow go through this sw. If so. Update the swtich's flow set 
	   *and CountTable.
	   */

	  //NS_LOG_INFO (swID<<" doesn't decode this before");
	  
	  Ptr<FlowEncoder>  swEncoder = GetEncoderByID (swID);
	  if ( swEncoder->ContainsFlow(flow) )
	    {
	      //NS_LOG_INFO(swID<<" flow filter matched, add flow.");
	      swDcdFlowInfo [flow] = 0;
	      swEncoder->ClearFlowInCountTable (flow);
	    }
	  else
	    {
	      //NS_LOG_INFO(swID<<" flow filter not matched");
	    }
  
	}
      
    }
}

void
FlowDecoder::WorkerThread()
{
  Ptr<FlowEncoder> onEncoder;
  while( m_workQueue.TryGetWork(onEncoder) )
    {
      std::string info = CounterSingleDecode(onEncoder);
      {
	CriticalSection cs(m_workerOutputMutex);
	NS_LOG_INFO(info.c_str());
      }
    }

  //No more works
  {
    CriticalSection cs(m_workerDoneMutex);
    m_numDoneWorkers++;
  }

  if(m_numDoneWorkers == NUM_THREAD)
    {
      //All worker threads finish their work
      //Synchronize the flow decoder to wake up.
      m_allWorkersDone.SetCondition (true);
      m_allWorkersDone.Broadcast();
    }
}
  
std::string
FlowDecoder::CounterSingleDecode (Ptr<FlowEncoder> target)
{

  std::ostringstream oss;
  
  int            swID   = target->GetID();
  const unsigned m      = COUNT_TABLE_SIZE; //Row
  //const unsigned n      = m_curSWFlowInfo.at(swID).size(); //Col
  const unsigned n      = m_swStat[swID].decodedFlowInfo.size(); 

  oss << "Counter Decode at " << swID << "\n"
      << "Flows to cal: " << n << "\n";
  
  if(n == 0)
    {
      oss << "No flow to calculate!\n";
      return oss.str();
    }
 
  //Contruct and Solve the linear equations with lsqr
  //double* A[m]; //Proxy
  //double  b[m];
  double* A[1];
  double  b[1]; /*disable the lsqr solver*/
    
  //double* AA = new double[m*n];
  double* AA = new double[10];

  /*
  for(unsigned i = 0; i < m; ++i)
    {
      A[i] = &(AA[i*n]);
      for(unsigned j = 0; j < n; ++j)
	{
	  A[i][j] = 0.0;
	}
    }
  */

  if( !ConstructLinearEquations (A, b, m, n, target) )
    {
      //Not All flow is decoded, The solve must be wrong.
      oss << "Not All flow is decoded out, cal will be wrong!\n";
      delete [] AA;
      return oss.str();
    }

  /*
  lsqrDense solver;
  //lsmrDense solver;
  const double eps = 1e-5;
  solver.SetEpsilon( eps );
  solver.SetDamp( 0.0 );
  solver.SetMaximumNumberOfIterations( 100 );
  solver.SetToleranceA( 1e-6 );
  solver.SetToleranceB( 1e-6 );
  solver.SetUpperLimitOnConditional( 1.0 / ( 10 * sqrt( eps ) ) );
  //solver.SetStandardErrorEstimatesFlag( true );
  //solver.SetStandardErrorEstimatesFlag( true );
  //double se[n];
  //solver.SetStandardErrorEstimates( se );
  solver.SetMatrix(A);
  double x[n];
  for(unsigned jth = 0; jth < n; ++jth)
    {
      x[jth] = 0.0; //at least 1packet, ps, no use, solver will clear it into 0...
    }
  
  
  solver.Solve(m, n, b, x);

  oss << "Stopped because " << solver.GetStoppingReason() << " : " << solver.GetStoppingReasonMessage() << "\n";
  oss << "Used " << solver.GetNumberOfIterationsPerformed() << " Iters" << "\n";
  
  //for(unsigned jth = 0; jth < n; ++jth)
    //{
      //NS_LOG_INFO(x[jth]);
    //}
  

  //Fill the m_curSWFlowInfo packet info
  FlowInfo_t &swDcdFlowInfo = m_curSWFlowInfo.at(swID);
  
  FlowInfo_t::iterator itFlow;
  unsigned jth;
  for(jth = 0, itFlow = swDcdFlowInfo.begin();
      itFlow != swDcdFlowInfo.end();
      ++jth, ++itFlow)
    { 
      itFlow->second = x[jth] + 0.5; //fill the packet cnt; + 0.5 for round;
    }
    
  delete []  AA;
  */
  return oss.str();
}

void
FlowDecoder::CounterAllDecode ()
{
  NS_LOG_INFO("Counter Decode Start");
  //Add encoders to work queue;
  NS_ASSERT(m_workQueue.IsEmpty());
  for (unsigned ith = 0; ith < m_encoders.size(); ++ith)
    {
      m_workQueue.PutWork( m_encoders[ith] );
    }
  m_numDoneWorkers = 0;
  m_allWorkersDone.SetCondition(false);
  
  // Spawn the worker threads.
  for (unsigned ith = 0; ith < NUM_THREAD; ++ith)
    {
      m_workerThreads[ith]->Start();
    }
  //Wait for workers to finish their work
  m_allWorkersDone.Wait();

  //join all workers threads
  for(unsigned ith = 0; ith < NUM_THREAD; ++ith)
    {
      m_workerThreads[ith]->Join();
    }

}

bool
FlowDecoder::ConstructLinearEquations (double* A[], double b[],
				       unsigned m,  unsigned n,
				       Ptr<FlowEncoder> target)
{
 
  int                        swID           = target->GetID();
  //FlowInfo_t                &swDcdFlowInfo  = m_curSWFlowInfo.at (swID);
  FlowInfo_t                &swDcdFlowInfo  = m_swStat[swID].decodedFlowInfo;
  FlowEncoder::CountTable_t &swCountTable   = target->GetCountTable();
  Stat_t                    &swStat         = m_swStat.at(swID);

  //Update the swtch status,
  swStat.numFlow = swDcdFlowInfo.size();
  
  //No more flow will be inserted, so rehashing will not happen
  /*
  FlowInfo_t::const_iterator itFlow;
  unsigned col;
  for (col = 0, itFlow = swDcdFlowInfo.begin();
      itFlow != swDcdFlowInfo.end();
      ++col, ++itFlow)
    {
      const FlowField      &flow    = itFlow->first;
      std::vector<uint32_t> rowIdxs = target->GetCountTableIdx(flow);
      for (unsigned ith = 0; ith < rowIdxs.size(); ++ith)
	{
	  //A[rowIdxs[ith]][col] = 1.0;
	}
    }
  */
  //NS_ASSERT(col == n && m == COUNT_TABLE_SIZE);
  
  for(unsigned row = 0; row < m; ++row)
    {
      //b[row] = swCountTable[row].packet_cnt;
      
      //not all flow decoded out, Update the swtch status
      if(swStat.IsAllDecoded && swCountTable[row].flow_cnt > 0)
	{
	  swStat.IsAllDecoded = false;
	}
    }

  /*
  for(unsigned i = 0; i < m; ++i)
  {
    for(unsigned j = 0; j < n; ++j)
      {
	std::cout << A[i][j] << " ";
      }

    std::cout << "  " << b[i] << std::endl;
  }
  */

  return swStat.IsAllDecoded;  
}

void
FlowDecoder::Clear()
{
  
  SWStat_t::iterator itStat;
  for( itStat = m_swStat.begin(); itStat != m_swStat.end(); ++itStat )
    {
      (itStat->second).IsAllDecoded = true;
      (itStat->second).numFlow      = 0;
      (itStat->second).decodedFlowInfo.clear();
      (itStat->second).pSaveFile->close();
      delete (itStat->second).pSaveFile;
    }
}

}

