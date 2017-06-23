#ifndef APP_GEN_H
#define APP_GEN_H

#include "ns3/object.h"

namespace ns3{

class DCTopology;
class PacketGenerator;
  
class  AppGen : public Object
{
  
public:

  static const int BULK_APP   = 0; //TCP Bulk Application
  static const int ONOFF_APP  = 1; //UDP ON OFF Application
  
  AppGen(Ptr<DCTopology> topo);
  virtual ~AppGen();

  /*
  void GenFlow(int appType, int src, int dst, uint16_t port, uint32_t maxBytes,
	       float startTime, float stopTime);
  */
  
  /* Generate packets according to the packet trace
   */
  void SetPacketTrace(const char* filename, float endTime);

  void GenElephantMouseFlow(int flowCnt, float bandwidth, float endTime);
  
  void GenRandomUDPFlow (int flowCnt);

  void GenTCPFlow(int src, int dst, uint16_t port, uint32_t maxBytes,
		  float startTime);
  
  void GenUDPFlow(int src, int dst, uint16_t port, uint32_t maxBytes,
		  float startTime, float stopTime);

private:
  AppGen(const AppGen&);
  AppGen& operator=(const AppGen&);

  /*Used by GenElephantMouseFlow to generate elephant mouse flow at host*/
  void DoGenFlowsAtSource(int sourceID, int flowCnt,
			 float bandwidth, float endTime);

  void DoGenFlow(int src, int dst, int dstport, float bd,
		 float startTime, float endTime);

  void GetTwoDiffHost(int& from, int& to);

  int  GetDiffHostRandomly(int from);

  int                                 m_port;
  Ptr<DCTopology>                     m_topo;
  std::vector<Ptr<PacketGenerator> >  m_packetGenerators;
  int                                 m_flowNotGeneratedCnt;
};

}

#endif
