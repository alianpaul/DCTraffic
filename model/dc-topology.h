/*
 * Build the topology, Also work as a container to store the created nodes
 * , csmaNetDevices, openswitchNetDevices, Edges   
 */

#ifndef DC_TOPOLOGY_H
#define DC_TOPOLOGY_H

#include <iosfwd>
#include <vector>

#include "ns3/object.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-interface-container.h"

#include "graph-algo.h"

namespace ns3 {

enum TraceMode
{
  PCAP,
  ASCII,
  NOTRACE
};

enum MeasureMode
{
  FLOW_RADAR,
  MTX_RADAR,
  NORADAR
};

enum QueueMode
{
  DROP_TAIL_QUEUE,
  DIFF_QUEUE
};

class FlowDecoder;
class MatrixDecoder;
namespace ofi
{ 
  class EasyController;
}
class OpenFlowSwitchNetDevice;
  
class DCTopology : public Object {
  
public:
  
  
  //==========================Public Interface=================
  
  static TypeId GetTypeId( void );

  DCTopology(const char* filename, TraceMode traceType,
	     MeasureMode radarType, QueueMode queueType);
  virtual ~DCTopology();

    
  /* Build the Nodes, NetDevices according to the Topofile
   * OpenFlowNetDevices and Controller are excluded.
   * @filename: topo file name
   */
  void BuildTopo (const char* filename, TraceMode traceType,
		  MeasureMode radarType, QueueMode queueType);

  Graph::Path_t                GetPath    (int from, int to) const;
  unsigned                     GetNumHost () const;
  unsigned                     GetNumSW   () const;
  Ipv4Address                  GetHostIPAddr    (int hostID) const;
  std::vector<Ipv4Address>     GetAllHostIPAddr ()           const;
  Address                      GetHostMacAddr   (int hostID) const;
  Ptr<Node>                    GetHostNode      (int hostID) const;
  Ptr<NetDevice>               GetHostNetDevice (int hostID) const;
  //TODO: GetHostID need a more concrete version.
  int                          GetHostID  (uint32_t ipv4Addr) const; 
  Ptr<OpenFlowSwitchNetDevice> GetOFSwtch (int SWID) const;
  Ptr<Node>                    GetSWNode  (int SWID) const;

private:
  DCTopology(const DCTopology&);
  DCTopology& operator=(const DCTopology&);

  /* Easy contoller config the flow table on openflow switches.
   * Schedule the Flow Radar Decoding process.
   * Mute the host net device receive callback(We don't want any packet
   * generated automatically by up layer protocol. All packet is generated
   * manually).  
   */
  void Init(MeasureMode radarType);
  
  /*Create ns3 Nodes in the NodeContainer according to the Topofile.
   *Also intall internet stack on all nodes
   *Called by BuildTopo
   *file: input topo file
   */
  void CreateNodes (std::ifstream& file);

  /*Create ns3 port NetDevices on the nodes(host and swtiches)
   *Create the adjcent list of the topo;
   *Called by BuildTopo
   */
  void CreateNetDevices (std::ifstream& file, TraceMode traceType);

  /*Create ns3 OpenFlowNetDevices(openflowswitches) with the provided
   *switch nodes and swtich port netdevices
   *Create the easy controller.
   *Called by BuildTopo
   */
  void CreateOFSwitches ();

  /* Create FlowRadar
   * Install a flow encoder on each openflow switch.
   *   Call flow encoder's SetOFSwtch function register the openflow swtich's
   *   promisc receive call back function.
   * Register each flow encoder to a central flow decoder.
   */
  void CreateFlowRadar ();

  /*
   * Another flow measurement.
   */
  void CreateMatrixRadar();
  
  /* Set IP addresses of the hosts and openflow switches
   * and also set ARP cache permantly, because this simulation is focused on
   * ethernet IP packetets. We don't want any ethernet ARP broadcast packets flow 
   * in the network. In this way, it's easy to set flow tables of the switch.
   */
  void SetIPAddrAndArp ();

  /* Set the queue used by the netdevice. DropTail or DiffQueue
   */
  void SetSWNetdeviceQueue (QueueMode queueType);
    
  int                             m_numHost; 
  NodeContainer                   m_hostNodes;
  NetDeviceContainer              m_hostDevices;
  Ipv4InterfaceContainer          m_hostIPInterface;    //host ip interfaces
  
  int                             m_numSw;
  NodeContainer                   m_switchNodes;
  NetDeviceContainer              m_OFSwtchDevices;
  Ipv4InterfaceContainer          m_OFSwtchIPInterface; //OFSW ip interfaces
  std::vector<NetDeviceContainer> m_switchPortDevices;

  Ptr<FlowDecoder>                m_flowRadar;
  Ptr<MatrixDecoder>              m_matrixRadar;
  Ptr<ofi::EasyController>        m_easyController;

  Graph                           m_graph;             //Store All path info
};

  
}

#endif
