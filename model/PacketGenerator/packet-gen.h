#ifndef PACKET_GEN_H
#define PACKET_GEN_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"

#include <boost/unordered_map.hpp>
#include <fstream>

namespace ns3{

class NetDevice;
class Packet;
class Node;
class Ipv4L3Protocol;
  
class PacketGenerator : public Object
{

public:
  PacketGenerator(int                             hostid,  
		  Ptr<NetDevice>                  netdev,
		  Ptr<Node>                       node,
		  const Address&                  macAddr,
		  const Ipv4Address&              ipv4Addr,
		  const std::vector<Ipv4Address>& allAddr,
		  const char*                     filename);

  ~PacketGenerator();
  
  /* TODO: Start transmit.
   */
  void StartGenerator(float endTime);

  void Send( Ptr<Packet>        packet,
	     const Ipv4Address& ipdst,
	     uint16_t           portsrc,
	     uint16_t           portdst,
	     uint8_t            type);

private:

  /* TODO: Schedule the next SendPacket event
   */
  void ScheduleNextTx();

  /* TODO:
   */
  void SendPacket(Ipv4Address ipdst,
		  uint16_t    portsrc,
		  uint16_t    portdst,
		  uint8_t     type,
		  uint32_t    size);
  
  /* Find the hardware destination address in the arp cache
   */
  Address GetHardwareDstAddr(Ptr<Ipv4L3Protocol> ipv4Impl,
			     const Ipv4Address& ipdst);

  /* Transform the ipdst into the data center host ip;
   */
  Ipv4Address GetNewIPAddress(const std::string& ipdst);
  
  int                      m_hostID;       //ID of the host that the pack gen is on 
  Address                  m_macAddr;      //mac addr of the host's net device
  Ipv4Address              m_ipv4Addr;     //ip of the host 
  Ptr<NetDevice>           m_device;       //host' net device
  Ptr<Node>                m_node;         //host' node
  float                    m_lastTime;     //lastSendTime;
  float                    m_endTime;
  
  std::vector<Ipv4Address>                 m_allIPAddr;  //other host ip addr in DC.
  boost::unordered_map<uint32_t, unsigned> m_ipDict; //ip dict
  std::ifstream                            m_file;         //packet file
};

}


#endif
