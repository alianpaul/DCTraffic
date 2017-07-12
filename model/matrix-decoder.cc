#include "matrix-decoder.h"
#include "flow-field.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>



namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MatrixDecoder");
NS_OBJECT_ENSURE_REGISTERED(MatrixDecoder);

/*Call the cplex lib to solve the function, defined in solver/cplex-solver.cc*/
int CplexSolve(Ptr<MatrixEncoder> target, FlowInfo_t& flows);
  
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
      FlowInfo_t measuredFlows;
      int status = CplexSolve(target, measuredFlows);
      NS_ASSERT(!status); //assert that all block is successfully decoded
      
      
      OutputFlowData("measured",
		     target->GetID(),
		     Simulator::Now().GetSeconds(),
		     measuredFlows);

      //Notify queue controller to update the queue config according to the measured flow.
      if(!m_decodedCallback.IsNull())
	{
	  if(measuredFlows.size() > 0)
	    m_decodedCallback(target->GetID(), measuredFlows);
	}
      else
	{
	  NS_LOG_INFO("Queue Controller Callback not set");
	}
    }  

  //Output real flows to files
  //OutputRealFlows(m_encoders[i]);
  OutputFlowData("real",
		 target->GetID(), 
		 Simulator::Now().GetSeconds(), 
		 target->GetRealFlowCounter());
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

/** disabled, use OutputFlowData instead 
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
*/

void 
MatrixDecoder::OutputFlowData(std::string type, int swID, double time, const FlowInfo_t& flows)
{
  NS_LOG_INFO("Output " << type << " flows " << " at sw " << swID); 
  std::stringstream ss;
  ss << "sw-" << swID << "-t-" << time <<"-"<< type <<"-flow.txt";
  std::string filename; ss >> filename;

  std::ofstream file(filename.c_str());
  NS_ASSERT( file.is_open() );

  for(FlowInfo_t::const_iterator it = flows.begin();
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
