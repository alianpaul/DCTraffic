#include "matrix-decoder.h"
#include "matrix-encoder.h"
#include "flow-field.h"
#include "LSXR/lsqrDense.h"
#include "LSXR/lsmrDense.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>

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

  //1. Output CounteTable and FlowVector to files
  for(size_t i = 0; i < m_encoders.size(); ++i)
    {
      MtxDecode(m_encoders[i]);
    }

  //1.1 Output real flows to files
  for(size_t i = 0; i < m_encoders.size(); ++i)
    {
      OutputRealFlows(m_encoders[i]);
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
	  const MtxFlowField& mtxflow = block.m_flowTable[fi];
	  //NS_LOG_INFO(mtxflow);
	  file << mtxflow << std::endl; 
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
  NS_LOG_INFO("Output real flows " << target->GetID());

  NS_LOG_INFO("MtxEncoder " << target->GetID() << " Packets Receved "
	      << target->GetTotalPacketsReceived());
  
  std::stringstream ss;
  ss << "sw-" << target->GetID() << "-t-" << Simulator::Now().GetSeconds()
     << "-real-flow.txt";
  std::string filename; ss >> filename;

  std::ofstream file(filename.c_str());
  NS_ASSERT( file.is_open() );
  
  const MatrixEncoder::FlowInfo_t& realFlows = target->GetRealFlowCounter();
  for(MatrixEncoder::FlowInfo_t::const_iterator it = realFlows.begin();
      it != realFlows.end();
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
  
}
