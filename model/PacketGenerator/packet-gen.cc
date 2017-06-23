
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "ns3/log.h"
#include "ns3/csma-net-device.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/arp-cache.h"

#include "packet-gen.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PacketGenerator");

NS_OBJECT_ENSURE_REGISTERED(PacketGenerator);

PacketGenerator::PacketGenerator(int                             hostid,  
				 Ptr<NetDevice>                  netdev,
				 Ptr<Node>                       node,
				 const Address&                  macAddr,
				 const Ipv4Address&              ipv4Addr,
				 const std::vector<Ipv4Address>& allAddr,
				 const char*                     filename)
  : m_hostID(hostid), m_macAddr(macAddr), m_ipv4Addr(ipv4Addr),
    m_device(netdev), m_node(node),  m_lastTime(0.0f), m_allIPAddr(allAddr)
{
  NS_LOG_FUNCTION( hostid );
  
  m_file.open (filename);
  if(m_file.fail())
    NS_FATAL_ERROR("packet file can not open");
  std::string line;
  std::getline(m_file, line); //Read out the table header.
}

PacketGenerator::~PacketGenerator()
{
}

void
PacketGenerator::StartGenerator(float endTime)
{
  m_endTime = endTime;
  ScheduleNextTx();
}

void 
PacketGenerator::ScheduleNextTx()
{
  //1. Read one packet information from you pcap file
  //   If no more packetm, just return to stop.
  std::string line;
  if(std::getline(m_file, line))
    {

      //2. Read and transform packet info
      std::istringstream issl(line);

      unsigned packetID;
      float    time;
      std::string   ipsrc,ipdst;
      std::string   l4prot;
      uint16_t portsrc, portdst;
      uint32_t size;
      std::string   trash; //for escaping the content

      issl >> packetID >> time;
      if(time > m_endTime) //Turn off PakcetGenerator
	return;
			     
      issl >> ipsrc >> ipdst >> l4prot;
      if(l4prot == "TCP" || l4prot == "UDP")
	{
	  issl >> size;
	  issl >> portsrc;
	  issl >> trash;
	  issl >> portdst;
	}
      else
	{
	  size = 0;
	  portsrc = 0;
	  portdst = 0;
	}
      
      uint8_t protType;
      if(l4prot == "TCP")
	{
	  protType = TcpL4Protocol::PROT_NUMBER;
	  size = 512;
	  NS_ASSERT(size >= 0);
	  if(size > 1500) size = 1500;
	}
      else if(l4prot == "UDP")
	{
	  protType = UdpL4Protocol::PROT_NUMBER;
	  size = 512;
	  NS_ASSERT(size >= 0);
	  if(size > 1500) size = 1500;     //MTU 1500
	}
      else
	{
	  protType = -1;
	  size = 0;
	}
      
      Ipv4Address ipNewDst = GetNewIPAddress(ipdst);
      float       delay    = time - m_lastTime;
      delay = delay < 0.0 ? 0.0 : delay;
      m_lastTime           = time;
      
      //3. Schedule the transmit event according to the time
      //   Simulator::Schedule(time, &SendPacket, this, ...)
      //   ...means other arguments like packet size, packet type, port etc.
      Simulator::Schedule (Seconds(delay), &PacketGenerator::SendPacket, this,
			   ipNewDst, portsrc, portdst, protType, size);
    }

}

void
PacketGenerator::SendPacket(Ipv4Address ipdst,
			    uint16_t    portsrc,
			    uint16_t    portdst,
			    uint8_t     type,
			    uint32_t    size)
{

  if(type == TcpL4Protocol::PROT_NUMBER || type == UdpL4Protocol::PROT_NUMBER)
    {
      Ptr<Packet> pkt = Create<Packet>(size);
      Send(pkt, ipdst, portsrc, portdst, type);
    }

  ScheduleNextTx();
}
  
void
PacketGenerator::Send( Ptr<Packet>        packet,
		       const Ipv4Address& ipdst,
		       uint16_t           portsrc,
		       uint16_t           portdst,
		       uint8_t            type)
{
  // 1.Add Transport Layer header
  if(type == UdpL4Protocol::PROT_NUMBER)
    {
      UdpHeader udpHeader;

      if( Node::ChecksumEnabled() )
	{
	  udpHeader.EnableChecksums ();
	  udpHeader.InitializeChecksum(m_ipv4Addr,
				       ipdst,
				       UdpL4Protocol::PROT_NUMBER);
	}
      udpHeader.SetDestinationPort (portdst);
      udpHeader.SetSourcePort      (portsrc);
      packet->AddHeader(udpHeader);
    }
  else if(type == TcpL4Protocol::PROT_NUMBER)
    {
      TcpHeader tcpHeader;

      tcpHeader.SetDestinationPort (portdst);
      tcpHeader.SetSourcePort      (portsrc);
      packet->AddHeader(tcpHeader);  
    }
  else
    {
      NS_FATAL_ERROR("Transport Layer Protocol Unknown");
      return;
    }

  Ptr<Ipv4>           ipv4     = m_node->GetObject<Ipv4> ();
  Ptr<Ipv4L3Protocol> ipv4Impl = DynamicCast<Ipv4L3Protocol, Ipv4>(ipv4);
  
  //2. Add IP Layer header
  Ipv4Header          ipHeader = ipv4Impl->BuildHeader(m_ipv4Addr, ipdst,
						       type,
						       packet->GetSize(),
						       32, //ttl
						       0, //tos
						       false); //mayFragment
  packet->AddHeader(ipHeader);
  Address hwDst = GetHardwareDstAddr(ipv4Impl, ipdst);
  //3. Send the Packet through net device
  m_device->Send(packet, hwDst, Ipv4L3Protocol::PROT_NUMBER);
}

Address
PacketGenerator::GetHardwareDstAddr(Ptr<Ipv4L3Protocol> ipv4Impl,
				    const Ipv4Address& ipdst)
{

  if (ipdst.IsBroadcast())
    {
      return m_device->GetBroadcast();
    }
  else if(ipdst.IsMulticast())
    {
      return m_device->GetMulticast(ipdst);
    }
  else
    {
      int32_t interface = ipv4Impl->GetInterfaceForDevice(m_device);
      NS_ASSERT( interface >= 0 );
      Ptr<Ipv4Interface> outInterface = ipv4Impl->GetInterface (interface);
      Ptr<ArpCache>      arpCache     = outInterface->GetArpCache();
      ArpCache::Entry *entry = arpCache->Lookup(ipdst);

      //We set the arp cache to be permanent, it should exist.
      if(entry != 0)
	{
	  return entry->GetMacAddress();
	}
    }
  
  NS_FATAL_ERROR("Arp cache error");
  return Address();
  
}


Ipv4Address
PacketGenerator::GetNewIPAddress(const std::string& ipdstStr)
{
  
  
  if(ipdstStr == "Broadcast")
    {
      return Ipv4Address("255.255.255.255");
    }

  uint32_t    rawAddr = Ipv4Address(ipdstStr.c_str()).Get();
  boost::unordered_map<uint32_t, unsigned>::iterator it = m_ipDict.find(rawAddr);
  if(it == m_ipDict.end())
    {
      int ridx;
      while((ridx = std::rand() % m_allIPAddr.size())
	    == m_hostID)
	{
	}
      //std::cout << "ridx: " << ridx << std::endl;
	  
      m_ipDict[rawAddr] = ridx;
      return m_allIPAddr[ridx];
    }
  else
    {
      return m_allIPAddr[it->second];
    }
    
}

  
}
