#ifndef MATRIX_DECODER_H
#define MATRIX_DECODER_H

#include "ns3/object.h"
#include "matrix-encoder.h"
#include "flow-field.h"
#include <string>

namespace ns3 {

class QueueController;  

class MatrixDecoder : public Object
{
  
public:

  /*
  */
  typedef Callback<void, int, const FlowInfo_t&> DecodedCallback_t;

  MatrixDecoder();
  virtual ~MatrixDecoder();

  void AddEncoder (Ptr<MatrixEncoder> mtxEncoder);
  void DecodeFlows ();
  
  void Init ();

  void SetDecodedCallback(DecodedCallback_t cb);
 

private:
  void MtxDecode(Ptr<MatrixEncoder> target);

  /*Output the flow data, real flows or measured online flows
   */
  void OutputFlowData(std::string type, int swID, double time, const FlowInfo_t& flows);
  
  /*Not defined, use OutputFlowData instead*/
  void OutputRealFlows(Ptr<MatrixEncoder> target);

  /*Output the flow vector and countTable
   */
  void OutputFlowSet(Ptr<MatrixEncoder> target);

private:
  DecodedCallback_t                 m_decodedCallback; 
  //After we decoded a switch, we send the decoded flow(measuredFlows) to queue controller through this callback 
  std::vector<Ptr<MatrixEncoder> >  m_encoders;  
};
  
}

#endif
