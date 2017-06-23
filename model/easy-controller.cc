
#include <iostream>

#include "easy-controller.h"
#include "openflow-switch-net-device.h"
#include "dc-topology.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EasyController");
//NS_OBJECT_ENSURE_REGISTERED (EasyController);
namespace ofi {

TypeId
EasyController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ofi::EasyController")
    .SetParent<Controller> ()
    .SetGroupName("Openflow")
    .AddConstructor<EasyController> ()
    ;

  return tid;
}

void
EasyController::ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer)
{
  NS_LOG_ERROR("Receive packet can not route");
  return;
}
  
void
EasyController::SetTopo (Ptr<DCTopology> topo)
{
  NS_LOG_FUNCTION(this);
  m_topo = topo;
}

void
EasyController::SetDefaultFlowTable ()
{

  NS_LOG_FUNCTION(this);

  const unsigned          numHost  = m_topo->GetNumHost(); 

  /*
  for(unsigned i = 0; i < numNodes; ++i)
    {
      NS_LOG_LOGIC("id: "<<i);
      for(unsigned j = 0; j < adjList[i].size(); ++j)
	{
	  std::cout << adjList[i][j].from_port << " " <<
		       adjList[i][j].to_port << " " <<
	               adjList[i][j].id << std::endl;
	}

    }
  */
 
  for(unsigned from = 0; from < numHost; ++from)
    {
      for(unsigned to = 0; to < numHost; ++to)
	{
	  if(from == to) continue;
	  Graph::Path_t path = m_topo->GetPath(from, to);
	  
	  NS_LOG_INFO("Built path from " << from << " to " << to);
	    
	  SetFlowOnPath (path);
	}
    }
}

  
void
EasyController::SetFlowOnPath(const Graph::Path_t& path)
{
  int lst = path.size() - 1;
  //prepare the flow source host;
  int         srcHID     = path[0].src;
  Address     macSrcAddr = m_topo->GetHostMacAddr (srcHID);
  Ipv4Address ipSrcAddr  = m_topo->GetHostIPAddr  (srcHID);  


  //prepare the flow dst host
  int         dstHID     = path[lst].dst;
  Address     macDstAddr = m_topo->GetHostMacAddr (dstHID);
  Ipv4Address ipDstAddr  = m_topo->GetHostIPAddr  (dstHID);

  for(int ith = 0; ith < lst; ++ith)
    {
      const Graph::Edge_t& inEdge  = path[ith];
      const Graph::Edge_t& outEdge = path[ith + 1];

      NS_ASSERT(inEdge.dst == outEdge.src);
      
      int swID      = inEdge.dst;
      int swInPort  = inEdge.dpt;
      int swOutPort = outEdge.spt;

      Ptr<OpenFlowSwitchNetDevice> swtch = m_topo->GetOFSwtch (swID);
      
      ProactiveModFlow (swtch, OFPFC_ADD, swOutPort, swInPort,
			macSrcAddr, macDstAddr, ipSrcAddr, ipDstAddr,
			-1, -1); //tcp/udp dont consider

      NS_LOG_INFO("Swtch " << swID << " proactively add flow from "
		  << ipSrcAddr << "|" << macSrcAddr
		  << " to " << ipDstAddr << "|" << macDstAddr
		  << " in_port: " << swInPort << " out_port: " << swOutPort);
      
    }
}
  
void
EasyController::ProactiveModFlow (Ptr<OpenFlowSwitchNetDevice> swtch,
				  uint16_t command,
				  uint16_t out_port, uint16_t in_port,
				  Address mac_src, Address mac_dst,
				  Ipv4Address ip_src, Ipv4Address ip_dst,
				  uint16_t port_src, uint16_t port_dst)
{

  NS_ASSERT (m_switches.find(swtch) != m_switches.end());
  
  //Create the matching key:
  sw_flow_key key;
  key.wildcards = htonl(OFPFW_NW_PROTO); //flow wildcards, defined in openflow.h 
  
  FlowExtract(&key.flow, in_port, mac_src, mac_dst, ip_src, ip_dst, port_src, port_dst);

  //Create the output-to-port action
  ofp_action_output x[1];
  x[0].type = htons (OFPAT_OUTPUT);
  x[0].len  = htons (sizeof(ofp_action_output));
  x[0].port = out_port;

  //buffer_id does not exist
  uint32_t buffer_id = htonl (std::numeric_limits<uint32_t>::max());
  ofp_flow_mod * ofm = BuildFlow (key, buffer_id, command, x, sizeof(x), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
  
  SendToSwitch (swtch, ofm, ofm->header.length);  
}


void
EasyController::FlowExtract (struct flow * flow_in_key,
			     uint16_t in_port,
			     Address mac_src, Address mac_dst,
			     Ipv4Address ip_src, Ipv4Address ip_dst,
			     uint16_t port_src, uint16_t port_dst)
{

  memset(flow_in_key, 0, sizeof(*flow_in_key));


  //Input VLAN vlan disable openflow.h
  flow_in_key->dl_vlan = htons (OFP_VLAN_NONE);
  //Input switch port
  flow_in_key->in_port = htons (in_port);


  //Ethernet frame type: Ethernet IP (No Ethernet Arp, because we set the
  //host's cache permanently). So we need to set nw_src, nw_dst.
  flow_in_key->dl_type = htons (ETH_TYPE_IP);
  //Ethernet address.
  Mac48Address mac48_src = Mac48Address::ConvertFrom(mac_src);
  Mac48Address mac48_dst = Mac48Address::ConvertFrom(mac_dst);
  mac48_src.CopyTo (flow_in_key->dl_src);
  mac48_dst.CopyTo (flow_in_key->dl_dst);
  //MAH: start MPLS_INVALID_LABEL private\packet.h
  flow_in_key->mpls_label1 = htonl (MPLS_INVALID_LABEL);
  flow_in_key->mpls_label2 = htonl (MPLS_INVALID_LABEL);

  
  //IP source address
  flow_in_key->nw_src = htonl (ip_src.Get ());
  flow_in_key->nw_dst = htonl (ip_dst.Get ());
  //IP protocal
  //openflow/lib/flow.cc set nw_proto=0 to avoid
  //tricking other code into thinking that this packet has a L4 packet.
  flow_in_key->nw_proto = 0;

  //TCP UDP port
  flow_in_key->tp_src = htons (port_src);
  flow_in_key->tp_dst = htons (port_dst);

  return;
  
}

      
}

}
