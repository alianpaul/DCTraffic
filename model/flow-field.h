#ifndef FLOW_FIELD_H
#define FLOW_FIELD_H

#include <iostream>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <vector>
#include "ns3/ptr.h"

namespace ns3
{

class Packet;

struct FlowField
{
  uint32_t ipv4srcip;
  uint32_t ipv4dstip;
  uint16_t srcport;
  uint16_t dstport;
  uint8_t  ipv4prot;

  FlowField():ipv4srcip(0), ipv4dstip(0), srcport(0), dstport(0), ipv4prot(0)
  {}
 
};

struct FlowFieldBoostHash
  : std::unary_function<FlowField, std::size_t>
{
  std::size_t operator()(FlowField const& f) const
  {
    std::size_t seed = 0;
    boost::hash_combine(seed, f.ipv4srcip);
    boost::hash_combine(seed, f.ipv4dstip);
    boost::hash_combine(seed, f.srcport);
    boost::hash_combine(seed, f.dstport);
    boost::hash_combine(seed, f.ipv4prot);
    return seed;
  }
};

//typedef boost::unordered_map<FlowField, uint16_t, FlowFieldBoostHash> FlowInfo_t; //Store the flow-size

/*Current compiler do not support c++11 template alias
 *So we just define a new template
 */
template<class INFO>
class FlowInfoHashMap_t : public boost::unordered_map<FlowField, INFO, FlowFieldBoostHash> {};

template<class INFO>
class FlowInfoVec_t : public std::vector<std::pair<FlowField, INFO> > {};

struct PckByteFlowCnt
{
  PckByteFlowCnt() : m_packetCnt(0), m_byteCnt(0), m_flowCnt(0)
  {}
  PckByteFlowCnt(size_t p, size_t b, size_t f) : m_packetCnt(p), m_byteCnt(b), m_flowCnt(f)
  {}
  uint16_t m_packetCnt;
  uint32_t m_byteCnt;
  uint16_t m_flowCnt;
};

struct PckByteCnt
{
  PckByteCnt() : m_packetCnt(0), m_byteCnt(0)
  {}
  PckByteCnt(size_t p, size_t b) : m_packetCnt(p), m_byteCnt(b)
  {} 
  uint16_t m_packetCnt;
  uint32_t m_byteCnt;
};

struct PckByteCntByteGreater
{
  bool operator()(const std::pair<FlowField, PckByteCnt>& x1, 
		  const std::pair<FlowField, PckByteCnt>& x2)
  {
    return x1.second.m_byteCnt > x2.second.m_byteCnt;
  }
};

struct PckByteCntPckGreater
{

  bool operator()(const std::pair<FlowField, PckByteCnt>& x1, 
		  const std::pair<FlowField, PckByteCnt>& x2)
  {
    return x1.second.m_packetCnt > x2.second.m_packetCnt;
  }
};

std::ostream& operator<<(std::ostream& os, const FlowField& flow);
std::ostream& operator<<(std::ostream& os, const PckByteFlowCnt& pbf);
std::ostream& operator<<(std::ostream& os, const PckByteCnt& pb);

bool operator==(FlowField const& f1, FlowField const& f2);

FlowField FlowFieldFromPacket(Ptr<Packet> packet, uint16_t protocol);

}

#endif
