#include "matrix-decoder.h"
#include "matrix-encoder.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>
#include <utility>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MatrixDecoder");
NS_OBJECT_ENSURE_REGISTERED(MatrixDecoder);
  
MatrixDecoder::MatrixDecoder()
{}

MatrixDecoder::~MatrixDecoder()
{}

void
MatrixDecoder::AddEncoder(Ptr<MatrixEncoder> mtxEncoder)
{
  NS_LOG_FUNCTION(this);
  m_encoders.push_back(mtxEncoder);
}

void
MatrixDecoder::DecodeFlows()
{
  NS_LOG_FUNCTION(Simulator::Now().GetSeconds());

  //1. Decode flows, if it is offline decode, just output the enooded data.
  for(size_t i = 0; i < m_encoders.size(); ++i)
    {
      MtxDecode(m_encoders[i]);
    }
  
  //2. Clear all the counters
  for(size_t i = 0; i < m_encoders.size(); ++i)
    {
      m_encoders[i]->Clear();
    }

  //3. Schedule next decode event
  if(Simulator::Now().GetSeconds() + MTX_PERIOD < MTX_END_TIME)
    {
      NS_LOG_INFO("Waiting to decode the next period flow");
      Simulator::Schedule (Seconds(MTX_PERIOD), &MatrixDecoder::DecodeFlows, this);
    }
  else
    {
      NS_LOG_INFO("Stop Decoding");
    }
}


void
MatrixDecoder::MtxDecode(Ptr<MatrixEncoder> target)
{
  if (IS_OFFLINE_DECODE) 
    {
      //Output CounteTable and FlowVector to files to decode offline
      NS_LOG_LOGIC("Offline Decoding");
      OutputFlowSet(target);
    }
  else 
    {
      
      //Use online cplex lib to decode
      NS_LOG_LOGIC("Online Decoding");

      FlowInfoVec_t<PckByteCnt> measuredFlowPckByteInfo; 
      DecodeFlowInfoAt(target, measuredFlowPckByteInfo);
      
      OutputDecodedFlows(target->GetID(), measuredFlowPckByteInfo);      
      //Notify queue controller to update the queue config according to the measured flow.
      if(!m_decodedCallback.IsNull())
	{
	  if(measuredFlowPckByteInfo.size() > 0)
	    m_decodedCallback(target->GetID(), measuredFlowPckByteInfo);
	  else 
	    {
	      NS_LOG_INFO("No flow measured");
	    }
	}
      else
	{
	  NS_LOG_INFO("Queue Controller Callback not set");
	}
     
    }  

  //Output real flows to files
  OutputRealFlows(target);
}

/*Helper function to form the equations
 */
void FormEquations(const MtxBlock& block, 
		   std::vector<std::vector<uint16_t> >& cntIdToFlowId,
		   std::vector<uint32_t>& pckCnt,
		   std::vector<uint32_t>& byteCnt);
/*Helper function to solve the equations
 *defined in solver/cplex-solve.cc
 */
void CplexSolveEquations(const std::vector<std::vector<uint16_t> >& cntIdToVarId,
			 const std::vector<uint32_t>& cnt,
			 std::vector<uint32_t>& var);
void
MatrixDecoder::DecodeFlowInfoAt(Ptr<MatrixEncoder> target, FlowInfoVec_t<PckByteCnt>& flowPckByteInfo)
{
  const std::vector<MtxBlock>& mtxBlocks = target->GetMtxBlocks();
  //For each block, we form the equation and solve, this part can be parallelized 
  for(size_t ib = 0; ib < MTX_NUM_BLOCK; ++ib)
    {
      const MtxBlock& block   = mtxBlocks[ib];
      int             flowCnt = block.m_flowTable.size();  //varCnt
      int             eqCnt   = block.m_countTable.size(); //Equations Cnt

      std::vector<std::vector<uint16_t> > cntIdToFlowId(eqCnt, std::vector<uint16_t>());
      std::vector<uint32_t>               pckCnt(eqCnt, 0);
      std::vector<uint32_t>               byteCnt(eqCnt, 0);
      FormEquations(block, cntIdToFlowId, pckCnt, byteCnt);

      std::vector<uint32_t> pckSize(flowCnt, 0);   //flow pckSize
      CplexSolveEquations(cntIdToFlowId, pckCnt, pckSize);
      std::vector<uint32_t> byteSize(flowCnt, 0);  //flow byteSize
      CplexSolveEquations(cntIdToFlowId, byteCnt, byteSize);

      for(size_t iFlow = 0; iFlow < block.m_flowTable.size(); ++iFlow)
	{
	  const FlowField& flow = block.m_flowTable[iFlow].m_flow;
	  PckByteCnt       pb(pckSize[iFlow], byteSize[iFlow]);
	  flowPckByteInfo.push_back( std::make_pair(flow, pb) );
	}
    }
}

/*Helper function to form the equations
 */
void FormEquations(const MtxBlock& block, 
		   std::vector<std::vector<uint16_t> >& cntIdToFlowId,
		   std::vector<uint32_t>& pckCnt,
		   std::vector<uint32_t>& byteCnt)
{
  //Loop through the flow vector to form cntIdToFlowId
  for(size_t flowId = 0; flowId < block.m_flowTable.size(); ++flowId)
    {
      const MtxFlow& field = block.m_flowTable[flowId];
      for(size_t iIdx = 0; iIdx < field.m_countTableIDXs.size(); ++iIdx)
	{
	  size_t cntId = field.m_countTableIDXs[iIdx];
	  cntIdToFlowId[cntId].push_back(flowId);
	}
    }
  
  //Loop through the countTable to form pckCnt and byteCnt
  for(size_t cntId = 0; cntId < block.m_countTable.size(); ++cntId)
    {
      pckCnt[cntId]  = block.m_countTable[cntId].m_packetCnt;
      byteCnt[cntId] = block.m_countTable[cntId].m_byteCnt;
    }
}

void
MatrixDecoder::OutputFlowSet(Ptr<MatrixEncoder> target)
{
  NS_LOG_INFO("Decode at swtch " << target->GetID());

  //output file prefix and postfix
  std::stringstream ss;
  ss << "s-" << target->GetID() << "-t-" << Simulator::Now().GetSeconds();
  std::string filenamePre; ss >> filenamePre;
    
  const std::vector<MtxBlock>& mtxBlocks = target->GetMtxBlocks();
  for(size_t bi = 0 ; bi < mtxBlocks.size(); ++bi)
    {
      std::stringstream ssb; ssb << "-b-" << bi << ".txt";
      std::string filenamePost; ssb >> filenamePost;
      std::string filename = filenamePre + filenamePost; 
      //NS_LOG_INFO( filename );
      
      std::ofstream     file(filename.c_str());
      NS_ASSERT(file.is_open());
	
      const MtxBlock& block = mtxBlocks[bi];

      //1.Output all the flows.
      file << "flows " << block.m_flowTable.size() << std::endl;
      for(size_t fi = 0; fi < block.m_flowTable.size(); ++fi)
	{
	  //NS_LOG_INFO(mtxflow);
	  file << block.m_flowTable[fi] << std::endl; 
	}

      //2.Output counter table
      file << "counters" << std::endl;
      for(size_t ci = 0; ci < block.m_countTable.size(); ++ci)
	{
	  file << block.m_countTable[ci] << std::endl;
	}
    }

}

 
void
MatrixDecoder::OutputRealFlows(Ptr<MatrixEncoder> target)
{
  NS_LOG_INFO("Output real flows at sw " << target->GetID());

  NS_LOG_INFO("MtxEncoder " << target->GetID() << " Packets Receved "
	      << target->GetTotalPacketsReceived());
  
  std::stringstream ss;
  ss << "sw-" << target->GetID() << "-t-" << Simulator::Now().GetSeconds()
     << "-real-flow.txt";
  std::string filename; ss >> filename;

  std::ofstream file(filename.c_str());
  NS_ASSERT( file.is_open() );
  
  const FlowInfoHashMap_t<PckByteCnt>& realFlows = target->GetRealFlowCounter();
  for(FlowInfoHashMap_t<PckByteCnt>::const_iterator it = realFlows.begin();
      it != realFlows.end();
      ++it )
    {
      file << it->first << " " << it->second << std::endl;
    }

  file.close(); 
}


void 
MatrixDecoder::OutputDecodedFlows(int swID, const FlowInfoVec_t<PckByteCnt>& flows)
{
  NS_LOG_INFO("Output decoded flows " << " at sw " << swID); 
  std::stringstream ss;
  ss << "sw-" << swID << "-t-" << Simulator::Now().GetSeconds() << "-measured-flow.txt";
  std::string filename; ss >> filename;

  std::ofstream file(filename.c_str());
  NS_ASSERT( file.is_open() );

  for(FlowInfoVec_t<PckByteCnt>::const_iterator it = flows.begin();
      it != flows.end();
      ++it )
    {
      file << it->first << " " << it->second << std::endl;
    }
  file.close();
}
  
void
MatrixDecoder::Init()
{
  
  NS_LOG_FUNCTION(this);
  Simulator::Schedule (Seconds(MTX_PERIOD), &MatrixDecoder::DecodeFlows, this);
}

void
MatrixDecoder::SetDecodedCallback(DecodedCallback_t cb)
{
  NS_LOG_INFO("Set decoded cb");
  m_decodedCallback = cb;
}
  
}
