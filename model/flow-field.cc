
#include <iostream>

#include "ns3/log.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-address.h"

#include "flow-field.h"

namespace ns3
{

std::ostream&
operator<<(std::ostream& os, const FlowField& flow)
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
     << srcport << " " << dstport;

  return os;
}
  
bool operator==(FlowField const& f1, FlowField const& f2)
{
  return f1.ipv4srcip == f2.ipv4srcip
      && f1.ipv4dstip == f2.ipv4dstip
      && f1.srcport   == f2.srcport
      && f1.dstport   == f2.dstport
      && f1.ipv4prot  == f2.ipv4prot;
}


  
}
