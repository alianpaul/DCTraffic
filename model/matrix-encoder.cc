
#include "matrix-encoder.h"
#include "openflow-switch-net-device.h"
#include "flow-hash.h"
#include "flow-field.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MatrixEncoder");
  
NS_OBJECT_ENSURE_REGISTERED(MatrixEncoder);

std::ostream&
operator<<(std::ostream& os, const MtxFlow& mtxflow)
{
  
  os << mtxflow.m_flow << " idx ";

  for(size_t i = 0; i < mtxflow.m_countTableIDXs.size(); ++i)
    {
      os << mtxflow.m_countTableIDXs[i] << " ";
    }

  return os;
}


MatrixEncoder::MatrixEncoder() : m_packetReceived(0)
{
  //initialize the hash seeds
  m_blockSeed = std::rand() % 10;
  for(size_t i = 0; i < MTX_NUM_IDX; ++i)
    {
      unsigned seed = std::rand() % 10;
      while(find(m_idxSeeds.begin(), m_idxSeeds.end(), seed) != m_idxSeeds.end())
	{
	  seed = std::rand() % 10;
	}
      m_idxSeeds.push_back(seed);
    }

  //intialize the blocks
  m_mtxBlocks.resize(MTX_NUM_BLOCK, MtxBlock());
  for(size_t i = 0; i < MTX_NUM_BLOCK; ++i)
    {
      m_mtxBlocks[i].m_countTable.resize(MTX_COUNT_TABLE_SIZE_IN_BLOCK);
    }
}
  
MatrixEncoder::~MatrixEncoder()
{}

void
MatrixEncoder::SetOFSwtch(Ptr<NetDevice> OFswtch, int id)
{
  NS_LOG_FUNCTION(this);
  m_id = id;
  OFswtch->SetPromiscReceiveCallback(MakeCallback(&MatrixEncoder::ReceiveFromOpenFlowSwtch, this));
}

bool
MatrixEncoder::ReceiveFromOpenFlowSwtch(Ptr<NetDevice> ofswtch,
					Ptr<const Packet> constPacket,
					uint16_t protocol,
					const Address& src, const Address& dst,
					NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION("MtxEncoder ID " << m_id << " receive\n");
  Ptr<Packet> packet    = constPacket->Copy();
  FlowField   flow      = FlowFieldFromPacket (packet, protocol);
  uint32_t    byte      = constPacket->GetSize();
  
  bool isNew                           = UpdateFlowFilter(flow);
  uint16_t blockIdx                    = GetBlockIdx(flow);
  std::vector<uint16_t> countTableIdxs = GetCountTableIdx(flow);

  //Update
  UpdateMtxBlock (flow, isNew, byte, blockIdx, countTableIdxs);

  //Update
  UpdateRealFlowCounter (flow, byte);

  ++m_packetReceived;
  if(m_packetReceived % 1000 == 0)
    std::cout << "MtxEncoder "    << m_id 
	      << " received packets " << m_packetReceived << std::endl;
  
  return true;
}

void
MatrixEncoder::Clear()
{
  NS_LOG_INFO("MtxEncoder ID " << m_id << " reset");

  m_mtxBlocks.clear();
  m_mtxBlocks.resize(MTX_NUM_BLOCK);
  for(size_t i = 0; i < MTX_NUM_BLOCK; ++i)
    {
      m_mtxBlocks[i].m_countTable.resize(MTX_COUNT_TABLE_SIZE_IN_BLOCK);
    }

  m_mtxFlowFilter.reset();
  m_realFlowCounter.clear();
  m_packetReceived = 0;
}

void
MatrixEncoder::UpdateMtxBlock(const FlowField& flow, bool isNew, uint32_t byte,
			      uint16_t blockIdx,
			      std::vector<uint16_t> countTableIdxs)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(blockIdx < MTX_NUM_BLOCK);
  MtxBlock& mtxBlock = m_mtxBlocks[blockIdx];
  
  //Update flow vector
  if(isNew)
    {
      //NS_LOG_INFO("New flow" << flow);
      mtxBlock.m_flowTable.push_back( MtxFlow(flow, countTableIdxs) );
    }

  //Update count table
  for(size_t i = 0; i < countTableIdxs.size(); ++i)
    {
      PckByteFlowCnt& field = mtxBlock.m_countTable[ countTableIdxs[i] ];
      field.m_packetCnt += 1;
      field.m_byteCnt   += byte;
      if(isNew) field.m_flowCnt += 1;
    }
}

void
MatrixEncoder::UpdateRealFlowCounter(const FlowField& flow, uint32_t byte)
{
  /*We use the simplest way to check if the flow is new, we are NOT using flowfilter to check if 
   *the flow is new(because it might be wrong)
   */
  NS_LOG_FUNCTION(this);
  FlowInfoHashMap_t<PckByteCnt>::iterator itFlow;
  if( (itFlow = m_realFlowCounter.find(flow)) == m_realFlowCounter.end() )
    {
      m_realFlowCounter[flow] = PckByteCnt();
    }

  m_realFlowCounter[flow].m_packetCnt += 1;
  m_realFlowCounter[flow].m_byteCnt   += byte;
}

bool
MatrixEncoder::UpdateFlowFilter(const FlowField& flow)
{
  
  std::vector<uint32_t> filterIdxs = GetFlowFilterIdx(flow);

  bool isNew = false;
  for(unsigned ith = 0; ith < filterIdxs.size(); ++ith)
    {
      if( !m_mtxFlowFilter[filterIdxs[ith]] )
	{
	  //new flow
	  m_mtxFlowFilter[filterIdxs[ith]] = true;
	  isNew = true;
	}
    }
  return isNew;
}

std::vector<uint16_t>
MatrixEncoder::GetCountTableIdx(const FlowField& flow)
{
  std::vector<uint16_t> idxs;
  for(size_t i = 0; i < m_idxSeeds.size(); ++i)
    {
      char buf[13];
      memset(buf, 0, 13);
      memcpy(buf     , &(flow.ipv4srcip), 4);
      memcpy(buf + 4 , &(flow.ipv4dstip), 4);
      memcpy(buf + 8 , &(flow.srcport)  , 2);
      memcpy(buf + 10, &(flow.dstport)  , 2);
      memcpy(buf + 12, &(flow.ipv4prot) , 1);

      idxs.push_back( murmur3_32(buf, 13, m_idxSeeds[i])
		      % MTX_COUNT_TABLE_SIZE_IN_BLOCK );
    }

  return idxs;
}

uint16_t
MatrixEncoder::GetBlockIdx(const FlowField& flow)
{
  char buf[13];
  memset(buf, 0, 13);
  memcpy(buf     , &(flow.ipv4srcip), 4);
  memcpy(buf + 4 , &(flow.ipv4dstip), 4);
  memcpy(buf + 8 , &(flow.srcport)  , 2);
  memcpy(buf + 10, &(flow.dstport)  , 2);
  memcpy(buf + 12, &(flow.ipv4prot) , 1);

  return murmur3_32(buf, 13, m_blockSeed) % MTX_NUM_BLOCK;
}
  
std::vector<uint32_t>
MatrixEncoder::GetFlowFilterIdx(const FlowField& flow)
{
  std::vector<uint32_t> filterIdxs;
  
  char buf[13];
  for(unsigned ith = 0; ith < MTX_NUM_FLOW_HASH; ++ith)
    {

      memset(buf, 0, 13);
      memcpy(buf     , &(flow.ipv4srcip), 4);
      memcpy(buf + 4 , &(flow.ipv4dstip), 4);
      memcpy(buf + 8 , &(flow.srcport)  , 2);
      memcpy(buf + 10, &(flow.dstport)  , 2);
      memcpy(buf + 12, &(flow.ipv4prot) , 1);
      
      //ith is also work as a seed of hash function
      uint32_t idx = murmur3_32(buf, 13, ith);

      //according to the P4 modify_field_with_hash_based_offset
      //the idx value is generated by %size;
      idx %= MTX_FLOW_FILTER_SIZE;
      filterIdxs.push_back(idx);
    }

  return filterIdxs;
}
  
}
