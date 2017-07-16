
#include <ctime>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <bitset>
#include <cfloat>
#include <iomanip>

#include "app-gen.h"
#include "dc-topology.h"
#include "PacketGenerator/packet-gen.h"

#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"


NS_LOG_COMPONENT_DEFINE("AppGen");

namespace ns3{

  AppGen::AppGen(Ptr<DCTopology> topo) : m_topo(topo), m_flowNotGeneratedCnt(0)
{
}

AppGen::~AppGen()
{
}

void
AppGen::SetPacketTrace(const char* filename, float endTime)
{
  for(unsigned hostID = 0; hostID < m_topo->GetNumHost(); ++hostID)
    {
      Ptr<PacketGenerator> pg
	= CreateObject<PacketGenerator>(hostID,
					m_topo->GetHostNetDevice(hostID),
					m_topo->GetHostNode(hostID),
					m_topo->GetHostMacAddr(hostID),
					m_topo->GetHostIPAddr(hostID),
					m_topo->GetAllHostIPAddr(),
					filename);
      pg->StartGenerator(endTime);
      m_packetGenerators.push_back(pg);
    }
}


void
AppGen::GetTwoDiffHost(int& from, int& to)
{
  int numHost = m_topo->GetNumHost();
  
  from = std::rand() % numHost;
  to   = std::rand() % numHost;
  while(from == to)
    {
      to  = std::rand() % numHost;
    }
}

void
AppGen::DoGenFlowsAtSource(int sourceID, int flowCnt,
			  float bandwidth, float endTime)
{
  NS_LOG_FUNCTION(this);

  int elephantFlowCnt = flowCnt * 0.2;
  int mouseFlowCnt     = flowCnt - elephantFlowCnt;
  NS_LOG_INFO("Gen Flow at host " << sourceID);
  NS_LOG_INFO("Elephant Flow Cnt " << elephantFlowCnt);
  NS_LOG_INFO("Mice Flow Cnt " << mouseFlowCnt);

  float elephantFlowAverBD = bandwidth * 0.8 * 0.24/ elephantFlowCnt;
  float mouseFlowAverBD    = bandwidth * 0.2 * 0.24/ mouseFlowCnt;

  elephantFlowAverBD *= 1000 * 1000; //Transform in to kbps
  mouseFlowAverBD    *= 1000 * 1000;

  NS_LOG_INFO("ElephantFlowAverBD " << elephantFlowAverBD);
  NS_LOG_INFO("MouseFlowAverBD "    << mouseFlowAverBD);

  Ptr<ExponentialRandomVariable> elephantFlowBDRNG
    = CreateObject<ExponentialRandomVariable> ();
  elephantFlowBDRNG->SetAttribute("Mean", DoubleValue(elephantFlowAverBD) );
  elephantFlowBDRNG->SetAttribute("Bound", DoubleValue(elephantFlowAverBD * 10) );

  Ptr<ExponentialRandomVariable> mouseFlowBDRNG
    = CreateObject<ExponentialRandomVariable> ();
  mouseFlowBDRNG->SetAttribute("Mean", DoubleValue(mouseFlowAverBD) );
  mouseFlowBDRNG->SetAttribute("Bound", DoubleValue(mouseFlowAverBD * 10) );

  float timeEPS   = FLT_EPSILON;
  //float timeEPS = 0;
  float startTime = 0.0;
  m_port          = 65535;
  
  NS_LOG_INFO("Generate Elephant flows");
  for(int i = 0; i < elephantFlowCnt; ++i)
    {
      int dstID = GetDiffHostRandomly(sourceID);
      double bd = elephantFlowBDRNG->GetValue();
      DoGenFlow(sourceID, dstID, m_port--, bd, startTime += timeEPS, endTime);
    }

  NS_LOG_INFO("Generate Mouse flows");
  for(int i = 0; i < mouseFlowCnt; ++i)
    {
      int dstID = GetDiffHostRandomly(sourceID);
      double bd = mouseFlowBDRNG->GetValue();
      DoGenFlow(sourceID, dstID, m_port--, bd, startTime += timeEPS, endTime);
    }
}

void
AppGen::DoGenFlow(int src, int dst, int dstport, float bd,
		  float startTime, float endTime)
{
  NS_LOG_INFO("src " << src << " dst " << dst);
  std::stringstream essbd;
  essbd << std::fixed << std::setprecision(5) << bd << "kb/s";
  std::string estrbd;
  essbd >> estrbd;
  NS_LOG_INFO (estrbd);
  NS_LOG_INFO (dstport);

  float bdinbit = bd * 1000;
  float firstPacketTime = 512 * 8 / bdinbit;
  if(startTime + firstPacketTime > endTime)
    {
      ++m_flowNotGeneratedCnt;
      return;
    }

  Ptr<Node>   clientNode = m_topo->GetHostNode(src);
  Ptr<Node>   serverNode = m_topo->GetHostNode(dst);
  Ipv4Address serverAddr = m_topo->GetHostIPAddr(dst);

  OnOffHelper onoff("ns3::UdpSocketFactory",
		    Address(InetSocketAddress(serverAddr, dstport)));
  onoff.SetConstantRate( DataRate(estrbd.c_str()) );
  ApplicationContainer app = onoff.Install (clientNode);

  app.Start(Seconds(startTime));
  app.Stop(Seconds(endTime));

  PacketSinkHelper sink("ns3::UdpSocketFactory",
			Address(InetSocketAddress(Ipv4Address::GetAny(),
						  dstport)));
  app = sink.Install(serverNode);
  app.Start(Seconds(startTime));
}

int
AppGen::GetDiffHostRandomly(int from)
{
  int numHost = m_topo->GetNumHost();
  int to = std::rand() % numHost;
  while(from == to)
    {
      to  = std::rand() % numHost;
    }

  return to;
}
  
/* DateRate: in Gbps
 */   
void
AppGen::GenElephantMouseFlow(int flowCnt, float bandwidth, float endTime)
{
  NS_LOG_FUNCTION(this);
  int numHost         = m_topo->GetNumHost();
  int flowCntAtSource = flowCnt / numHost; 
  for(int sourceID = 0; sourceID < numHost; ++sourceID)
    {
      DoGenFlowsAtSource(sourceID, flowCntAtSource, bandwidth, endTime);
    }

  std::cout << "Flows not generated " << m_flowNotGeneratedCnt << std::endl;
  
}
  
void
AppGen::GenRandomUDPFlow (int flowCnt)
{
  /*
  std::time_t     rawtime;
  std::time (&rawtime);
  struct std::tm* timeinfo = std::localtime(&rawtime);
  char            buffer[80];
  std::strftime( buffer, sizeof(buffer), "%d-%m-%Y", timeinfo);
  std::string     filename (buffer);
  */
  std::string     filename = "gen-flow.txt";
  std::ofstream   file;
  const int packetSize   = 512;
  //const int maxPacketCnt = 10;
  
  file.open(filename.c_str());

  if(!file)
    {
      NS_LOG_ERROR("Gen flow file open failed");
    }

  //std::bitset<65535> preports;
  int port = 65535;
  int numHost = m_topo->GetNumHost();
  float startTime = 0.1;
  float stepTime = 0.1;
  int   stepFlow = 100;

  int smallAverageFlowSize = 1;
  int bigAverageFlowSize   = 30;
  int smallFlowSizeRange   = 5;
  int bigFlowSizeRange     = 20;
  
  for (int ith = 0; ith < flowCnt; ++ith)
    {
      
      int src  = std::rand() % numHost;
      int dst  = std::rand() % numHost;
      while( src == dst )
	{
	  dst  = std::rand() % numHost;
	}
      
      --port;

      int packetCnt = 20;
      if( (float(std::rand())/float(RAND_MAX)) < 0.8 )
	{
	  //small flow
	  packetCnt =
	    smallAverageFlowSize
	    +
	    smallFlowSizeRange * (float(std::rand())/float(RAND_MAX));
	}
      else
	{
	  //big flow
	  packetCnt =
	    bigAverageFlowSize
	    +
	    bigFlowSizeRange * (float(std::rand())/float(RAND_MAX));
	}
      
      unsigned    maxBytes = packetSize * packetCnt;
      Ptr<Node>   clientNode = m_topo->GetHostNode(src);
      Ptr<Node>   serverNode = m_topo->GetHostNode(dst);
      Ipv4Address serverAddr = m_topo->GetHostIPAddr(dst);
      Ipv4Address clientAddr = m_topo->GetHostIPAddr(src);
      
      //client
      OnOffHelper onoff("ns3::UdpSocketFactory",
			Address(InetSocketAddress(serverAddr, port)));
      onoff.SetAttribute("MaxBytes", UintegerValue(maxBytes));
      onoff.SetConstantRate(DataRate("500kb/s"));
      ApplicationContainer app = onoff.Install (clientNode);

      if(ith % stepFlow == 0)
	{
	  startTime += stepTime;
	}
      
      app.Start(Seconds(startTime));
      app.Stop(Seconds(100.0));

      //server
      PacketSinkHelper sink("ns3::UdpSocketFactory",
			    Address(InetSocketAddress(Ipv4Address::GetAny(),
						      port)));
      app = sink.Install(serverNode);
      app.Start(Seconds(0.0));
	
      file << clientAddr << " " << serverAddr << " " << "UDP "<< port << " " << packetCnt << std::endl;
    }

  file.close();
}
  
void
AppGen::GenTCPFlow (int src, int dst, uint16_t port, uint32_t maxBytes,
		   float startTime)
{
 
}

void
AppGen::GenUDPFlow (int src, int dst, uint16_t port, uint32_t maxBytes,
		  float startTime, float stopTime)
{
  Ptr<Node>   clientNode = m_topo->GetHostNode(src);
  
  Ptr<Node>   serverNode = m_topo->GetHostNode(dst);
  Ipv4Address serverAddr = m_topo->GetHostIPAddr(dst);

  //client
  OnOffHelper onoff("ns3::UdpSocketFactory",
			  Address(InetSocketAddress(serverAddr, port)));
  //onoff.SetAttribute("MaxBytes", UintegerValue(maxBytes));
  //onoff.SetAttribute("PacketSize", UintegerValue(1024));
  //onoff.SetAttribute("DataRate",DataRateValue(DataRate("1000kb/s")));
  onoff.SetConstantRate(DataRate("1000kb/s"), 1024);
  ApplicationContainer app = onoff.Install (clientNode);
  app.Start(Seconds(startTime));
  app.Stop(Seconds(stopTime));

  //server
  PacketSinkHelper sink("ns3::UdpSocketFactory",
			Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
  app = sink.Install(serverNode);
  app.Start(Seconds(0.0));

}

}
