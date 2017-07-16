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

//The FlowVec
struct MtxFlow
{
  MtxFlow(const FlowField& flow, std::vector<uint16_t> idxs)
    : m_flow(flow), m_countTableIDXs(idxs)
  {}
  
  FlowField              m_flow;
  std::vector<uint16_t>  m_countTableIDXs; 
  //a flow map to MTX_COUNT_IDXS entries. we store the entries directly to avoid decoder recalculating  
};
std::ostream&
operator<<(std::ostream& os, const MtxFlow& mtxflow);



//Each block contains 1 FlowVec(stores the flows mapped to this block) and 1 counterTable
struct MtxBlock
{
  std::vector<MtxFlow>          m_flowTable;    //the flow vector
  std::vector<PckByteFlowCnt>   m_countTable;   //the counter table, the info is aggregated 
};

    
class MatrixEncoder : public Object
{
public:

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

  int                                   GetID()       { return m_id; }
  const std::vector<MtxBlock>&          GetMtxBlocks() { return m_mtxBlocks; }
  const FlowInfoHashMap_t<PckByteCnt>&  GetRealFlowCounter() { return m_realFlowCounter; }
  uint64_t                              GetTotalPacketsReceived() {return m_packetReceived;}
  
private:

  /* @flow: 5 tuple
   * @isNew: is this a new flow, if true, a this flow to flow vector, and increament the flowCnt field of counter
   * @byte: the size of the received packet
   * @blockIdx:
   * @countTableIdx:
   */
  void      UpdateMtxBlock(const FlowField& flow, bool isNew, uint32_t byte,
			   uint16_t blockIdx,
			   std::vector<uint16_t> countTableIdxs);

  /* m_realFlowCounter stores the real flow size.
   */
  void      UpdateRealFlowCounter(const FlowField& flow, uint32_t byte);
  
  bool      UpdateFlowFilter(const FlowField& flow);

  std::vector<uint32_t> GetFlowFilterIdx(const FlowField& flow);
  uint16_t              GetBlockIdx(const FlowField& flow);
  std::vector<uint16_t> GetCountTableIdx(const FlowField& flow);
  
  int                       m_id;         //id of the switch node
  unsigned                  m_blockSeed;  //seed to choose a group
  std::vector<unsigned>     m_idxSeeds;   //seed to choose idx in a group

  std::vector<MtxBlock>         m_mtxBlocks;  //mtx blocks, we have MTX_COUNT_SUBTABLEs
  typedef std::bitset<MTX_FLOW_FILTER_SIZE> FlowFilter_t;
  FlowFilter_t                  m_mtxFlowFilter;
  FlowInfoHashMap_t<PckByteCnt> m_realFlowCounter; //the info is flow's packet byte cnt 
  uint64_t                      m_packetReceived;
};
  
}

#endif
