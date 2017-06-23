#ifndef FLOW_ENCODER_H
#define FLOW_ENCODER_H

#include <iosfwd>
#include <bitset>

#include "ns3/object.h"
#include "ns3/net-device.h"

#include "flow-radar-config.h"
#include "flow-field.h"

#include <boost/unordered_map.hpp>

namespace ns3 {

  
class FlowEncoder : public Object
{
public:

  
  struct CountTableEntry
  {
    uint32_t xor_srcip;
    uint32_t xor_dstip;
    uint16_t xor_srcport;
    uint16_t xor_dstport;
    uint8_t  xor_prot;
    
    uint8_t  flow_cnt;
    uint32_t packet_cnt;

    CountTableEntry(): xor_srcip(0), xor_dstip(0), xor_srcport(0), xor_dstport(0),
		       xor_prot(0), flow_cnt(0), packet_cnt(0)
    {}

    inline void XORFlow(const FlowField& flow)
    {
      xor_srcip   ^= flow.ipv4srcip;
      xor_dstip   ^= flow.ipv4dstip;
      xor_srcport ^= flow.srcport;
      xor_dstport ^= flow.dstport;
      xor_prot    ^= flow.ipv4prot;
    }

    inline FlowField GetFlow()
    {
      NS_ASSERT(flow_cnt == 1);

      FlowField flow;
      
      flow.ipv4srcip = xor_srcip;
      flow.ipv4dstip = xor_dstip;
      flow.srcport   = xor_srcport;
      flow.dstport   = xor_dstport;
      flow.ipv4prot  = xor_prot;

      return flow;
    }
  };

  typedef std::vector<CountTableEntry>  CountTable_t;
  /* for real flow */
  typedef boost::unordered_map<FlowField, uint16_t, FlowFieldBoostHash> FlowInfo_t;
  
  /*Initialize the flow filter and count table
   */
  FlowEncoder ();
  virtual ~FlowEncoder ();

  /* Set the openflow switch's promisc receive callback to this flow encoder' 
   * recive function;
   */
  void                SetOFSwtch (Ptr<NetDevice> OFswtch, int id);
  
  int                 GetID();

  CountTable_t&       GetCountTable();

  const FlowInfo_t&   GetRealFlowCounter() { return m_realFlowCounter; }

  bool                ContainsFlow (const FlowField& flow);

  /*
   */
  void                ClearFlowInCountTable(const FlowField& flow);
  
  /* Clear flow filter and count table.
   */
  void                Clear();

  /* Calculate the count table idxs;
   */
  std::vector<uint32_t> GetCountTableIdx(const FlowField& flow);

  
  /* The call back function for openflow switch net device.
   * When openflow swtich net device receive a new packet(it's ReceiveFromDevice
   * function be called.), it will use it's m_promiscRxCallback which match to 
   * this function.
   */
  bool ReceiveFromOpenFlowSwtch (Ptr<NetDevice> ofswtch,
				 Ptr<const Packet> constPacket, uint16_t protocol,
				 const Address& src, const Address& dst,
				 NetDevice::PacketType packetType);
private:

  /* Extract the 5 tuple flow infro from packet to struct FlowField
   */
  FlowField    FlowFieldFromPacket(Ptr<Packet> packet, uint16_t protocol) const;

  /* Check the flow with the flow filter, if the flow is new, and update the 
   * flow filter and return true;
   * else return false;
   */
  bool      UpdateFlowFilter(const FlowField& flow);

  /* Update the flow according count table.
   * If the flow is new, we xor the flow fields,
   * else, we only update the packet count.
   */
  void      UpdateCountTable(const FlowField& flow, bool isNew);

  /* m_realFlowCounter stores the real flow size.
   */
  void      UpdateRealFlowCounter(const FlowField& flow);
  
  /* Calculate the flow filter idxs;
   */
  static std::vector<uint32_t> GetFlowFilterIdx(const FlowField& flow);

  
  typedef std::bitset<FLOW_FILTER_SIZE> FlowFilter_t;
  
  int                     m_id;             //id of the switch node
  FlowFilter_t            m_flowFilter;     //bit  
  CountTable_t            m_countTable;     //count table
  FlowInfo_t              m_realFlowCounter;
  std::vector<unsigned>   m_seeds;          //CounterTable hash seeds
  static unsigned         m_nextSeed;       //global next seed to add.
  uint64_t                m_packetReceived; //
};

 
}

#endif
