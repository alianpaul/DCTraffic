#ifndef FLOW_FIELD_H
#define FLOW_FIELD_H

#include <iostream>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
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

typedef boost::unordered_map<FlowField, uint16_t, FlowFieldBoostHash> FlowInfo_t; //Store the flow-size

std::ostream& operator<<(std::ostream& os, const FlowField& flow);

bool operator==(FlowField const& f1, FlowField const& f2);

FlowField FlowFieldFromPacket(Ptr<Packet> packet, uint16_t protocol);

}

#endif
