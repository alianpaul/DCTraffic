
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
operator<<(std::ostream& os, const MtxFlowField& flow)
{
  Ipv4Address srcip (flow.ipv4srcip);
  Ipv4Address dstip (flow.ipv4dstip);
  std::string prot;
  if (flow.ipv4prot == TcpL4Protocol::PROT_NUMBER)
    prot = "TCP";
  else if (flow.ipv4prot == UdpL4Protocol::PROT_NUMBER)
    prot = "UDP";
  else
    {
      prot = "Unknown L4 prot";
      os << flow.ipv4prot;
    }
    
  uint16_t srcport = flow.srcport;
  uint16_t dstport = flow.dstport;

  os << srcip << " " << dstip << " " << prot << " "
     << srcport << " " << dstport << " ";

  for(size_t i = 0; i < flow.m_countTableIDXs.size(); ++i)
    {
      os << flow.m_countTableIDXs[i] << " ";
    }

  return os;
}

  MatrixEncoder::MatrixEncoder() : m_packetReceived(0)
{
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

  m_mtxBlocks.resize(MTX_NUM_BLOCK, MtxBlock());
  for(size_t i = 0; i < MTX_NUM_BLOCK; ++i)
    {
      m_mtxBlocks[i].m_countTable.resize(MTX_COUNT_TABLE_SIZE_IN_BLOCK, 0);
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
  
  bool isNew                           = UpdateFlowFilter(flow);
  uint16_t blockIdx                    = GetBlockIdx(flow);
  std::vector<uint16_t> countTableIdxs = GetCountTableIdx(flow);

  //Update
  UpdateMtxBlock (flow, isNew, blockIdx, countTableIdxs);

  //Update
  UpdateRealFlowCounter (flow);

  ++m_packetReceived;
  if(m_packetReceived % 1000 == 0)
    std::cout << "MtxEncoder "    << m_id << " received packets "
	      << m_packetReceived << std::endl;
  
  return true;
}

void
MatrixEncoder::Clear()
{
  NS_LOG_INFO("MtxEncoder ID " << m_id << " reset");

  m_mtxBlocks.clear();
  m_mtxBlocks.resize(MTX_NUM_BLOCK, MtxBlock());
  for(size_t i = 0; i < MTX_NUM_BLOCK; ++i)
    {
      m_mtxBlocks[i].m_countTable.resize(MTX_COUNT_TABLE_SIZE_IN_BLOCK, 0);
    }

  m_mtxFlowFilter.reset();
  m_realFlowCounter.clear();
  m_packetReceived = 0;
}

void
MatrixEncoder::UpdateMtxBlock(const FlowField& flow, bool isNew,
			      uint16_t blockIdx,
			      std::vector<uint16_t> countTableIdxs)
{

  NS_ASSERT(blockIdx < MTX_NUM_BLOCK);
  MtxBlock& mtxBlock = m_mtxBlocks[blockIdx];
  
  if(isNew)
    {
      //NS_LOG_INFO("New flow" << flow);
      mtxBlock.m_flowTable.push_back( MtxFlowField(flow, countTableIdxs) );
    }

  for(size_t i = 0; i < countTableIdxs.size(); ++i)
    {
      ++ mtxBlock.m_countTable[ countTableIdxs[i] ];
    }
}

void
MatrixEncoder::UpdateRealFlowCounter(const FlowField& flow)
{
  FlowInfo_t::iterator itFlow;
  if( (itFlow = m_realFlowCounter.find(flow)) == m_realFlowCounter.end() )
    {
      m_realFlowCounter[flow] = 1;
    }
  else
    {
      itFlow->second += 1;
    } 
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

FlowField
MatrixEncoder::FlowFieldFromPacket(Ptr<Packet> packet, uint16_t protocol) const
{
  //NS_LOG_INFO("Extract flow field");
    
  FlowField flow;
  if(protocol == Ipv4L3Protocol::PROT_NUMBER)
    {
      Ipv4Header ipHd;
      if( packet->PeekHeader(ipHd) )
	{
	  //NS_LOG_INFO("IP header detected");
	  
	  flow.ipv4srcip = ipHd.GetSource().Get();
	  flow.ipv4dstip = ipHd.GetDestination().Get();
	  flow.ipv4prot  = ipHd.GetProtocol ();
	  packet->RemoveHeader (ipHd);

	  if( flow.ipv4prot == TcpL4Protocol::PROT_NUMBER)
	    {
	      TcpHeader tcpHd;
	      if( packet->PeekHeader(tcpHd) )
		{
		  //NS_LOG_INFO("TCP header detected");
		  
		  flow.srcport = tcpHd.GetSourcePort ();
		  flow.dstport = tcpHd.GetDestinationPort ();
		  packet->RemoveHeader(tcpHd);

		}
	    }
	  else if( flow.ipv4prot == UdpL4Protocol::PROT_NUMBER )
	    {
	      UdpHeader udpHd;
	      if( packet->PeekHeader(udpHd))
		{
		  //NS_LOG_INFO("UDP header detected");
		 
		  flow.srcport = udpHd.GetSourcePort ();
		  flow.dstport = udpHd.GetDestinationPort ();
		  packet->RemoveHeader(udpHd);
		}
	    }
	  else
	    {
	      NS_LOG_INFO("layer 4 protocol can't extract: "<< unsigned(flow.ipv4prot));
	    }
	  
	}
    }
  else
    {
      NS_LOG_INFO("packet is not an ip packet");
    }

  return flow;
}

  
}
