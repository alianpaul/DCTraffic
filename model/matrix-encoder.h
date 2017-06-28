#ifndef MATRIX_ENCODER_H
#define MATRIX_ENCODER_H

#include "ns3/object.h"
#include "ns3/net-device.h"

#include "matrix-radar-config.h"
#include "flow-field.h"

#include <vector>
#include <bitset>
#include <iosfwd>

#include <boost/unordered_map.hpp>

namespace ns3
{

struct MtxFlowField : public FlowField
{
  MtxFlowField(const FlowField& flow, std::vector<uint16_t> idxs)
    : FlowField(flow), m_countTableIDXs(idxs)
  {}
  
  std::vector<uint16_t>  m_countTableIDXs; //a flow map to MTX_COUNT_IDXS entries. 
};
  
std::ostream&
operator<<(std::ostream& os, const MtxFlowField& block);
  
struct MtxBlock
{
  std::vector<MtxFlowField>  m_flowTable;
  std::vector<uint16_t>      m_countTable;
};
    
class MatrixEncoder : public Object
{
public:

  typedef boost::unordered_map<FlowField, uint16_t, FlowFieldBoostHash> FlowInfo_t;

  MatrixEncoder();
  virtual ~MatrixEncoder();

  /* Install Mtx Radar on the openflow switch.
   */
  void SetOFSwtch(Ptr<NetDevice> OFswtch, int id);

  /* The call back function for openflow switch net device.
   * When openflow swtich net device receive a new packet(it's ReceiveFromDevice
   * function be called.), it will use it's m_promiscRxCallback which match to 
   * this function.
   */
  bool ReceiveFromOpenFlowSwtch (Ptr<NetDevice> ofswtch,
				 Ptr<const Packet> constPacket, uint16_t protocol,
				 const Address& src, const Address& dst,
				 NetDevice::PacketType packetType);

  /* Clear the record flows info
   */
  void Clear();

  int                          GetID()       { return m_id; }
  const std::vector<MtxBlock>& GetMtxBlocks() { return m_mtxBlocks; }
  const FlowInfo_t&            GetRealFlowCounter() { return m_realFlowCounter; }
  uint64_t                     GetTotalPacketsReceived() {return m_packetReceived;}
  
private:

  void      UpdateMtxBlock(const FlowField& flow, bool isNew,
			   uint16_t blockIdx,
			   std::vector<uint16_t> countTableIdxs);

  /* m_realFlowCounter stores the real flow size.
   */
  void      UpdateRealFlowCounter(const FlowField& flow);
  
  bool      UpdateFlowFilter(const FlowField& flow);

  std::vector<uint32_t> GetFlowFilterIdx(const FlowField& flow);
  uint16_t              GetBlockIdx(const FlowField& flow);
  std::vector<uint16_t> GetCountTableIdx(const FlowField& flow);
  
  typedef std::bitset<MTX_FLOW_FILTER_SIZE> FlowFilter_t;
  
  int                       m_id;         //id of the switch node
  unsigned                  m_blockSeed;  //seed to choose a group
  std::vector<unsigned>     m_idxSeeds;   //seed to choose idx in a group

  std::vector<MtxBlock>     m_mtxBlocks;  //mtx blocks, we have MTX_COUNT_SUBTABLEs
  FlowFilter_t              m_mtxFlowFilter;
  FlowInfo_t                m_realFlowCounter;
  uint64_t                  m_packetReceived;
};
  
}

#endif
