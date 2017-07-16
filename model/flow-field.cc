#include "flow-field.h"

#include <iostream>

#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlowField");

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

std::ostream& 
operator<<(std::ostream& os, const PckByteFlowCnt& pbf)
{
  os << "PckCnt " << pbf.m_packetCnt << " ByteCnt " << pbf.m_byteCnt << " FlowCnt " << pbf.m_flowCnt;
  return os;
}

std::ostream& operator<<(std::ostream& os, const PckByteCnt& pb)
{
  os << "PckCnt " << pb.m_packetCnt << " ByteCnt " << pb.m_byteCnt;
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

FlowField FlowFieldFromPacket(Ptr<Packet> packet, uint16_t protocol)
{
  NS_LOG_INFO("Extract flow field");
    
  FlowField flow;
  if(protocol == Ipv4L3Protocol::PROT_NUMBER)
    {
      
      Ipv4Header ipHd;
      if( packet->PeekHeader(ipHd) )
	{
	  NS_LOG_INFO("IP header detected");
	  
	  flow.ipv4srcip = ipHd.GetSource().Get();
	  flow.ipv4dstip = ipHd.GetDestination().Get();
	  flow.ipv4prot  = ipHd.GetProtocol ();
	  packet->RemoveHeader (ipHd);

	  if( flow.ipv4prot == TcpL4Protocol::PROT_NUMBER)
	    {
	      TcpHeader tcpHd;
	      if( packet->PeekHeader(tcpHd) )
		{
		  NS_LOG_INFO("TCP header detected");
		  
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
		  NS_LOG_INFO("UDP header detected");
		 
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
      else 
	{
	  NS_LOG_INFO("Peek failed, remove mac layer header first");
	  exit(0);
	}
    }
  else
    {
      NS_LOG_INFO("packet is not an ip packet");
    }

  NS_LOG_INFO("Extract Result: " << flow);
  return flow;
}
  
}
