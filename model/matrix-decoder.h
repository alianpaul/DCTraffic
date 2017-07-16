#ifndef MATRIX_DECODER_H
#define MATRIX_DECODER_H

#include "ns3/object.h"
#include "flow-field.h"
#include <string>

namespace ns3 {

class QueueController;  
class MatrixEncoder;

class MatrixDecoder : public Object
{
  
public:

  typedef Callback<void, int, const FlowInfoVec_t<PckByteCnt>& > DecodedCallback_t;

  MatrixDecoder();
  virtual ~MatrixDecoder();

  void AddEncoder (Ptr<MatrixEncoder> mtxEncoder);

  void DecodeFlows ();
  
  void Init ();

  void SetDecodedCallback(DecodedCallback_t cb);
 

private:

  void MtxDecode(Ptr<MatrixEncoder> target);
  /* Form and solve the flow equations,
   * Use the cplex lib.
   * return flowPckByteCnt
   */
  void DecodeFlowInfoAt(Ptr<MatrixEncoder> target, FlowInfoVec_t<PckByteCnt>& flowPckByteInfo);

  /*Output the online decoded flow data(pck cnt and byte cnt)
   */
  void OutputDecodedFlows(int swID, const FlowInfoVec_t<PckByteCnt>& flows);
  
  /*Output real flows for checking
   */
  void OutputRealFlows(Ptr<MatrixEncoder> target);
  
  /*Output the flow vector and countTable for offline decoding
   */
  void OutputFlowSet(Ptr<MatrixEncoder> target);


private:
  DecodedCallback_t                 m_decodedCallback; 
  //After we decoded a switch, we send the decoded flow(measuredFlows) to queue controller through this callback 
  std::vector<Ptr<MatrixEncoder> >  m_encoders;  
};
  
}

#endif
