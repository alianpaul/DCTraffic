#ifndef MATRIX_DECODER_H
#define MATRIX_DECODER_H

#include "ns3/object.h"
#include "matrix-encoder.h"
#include <string>

namespace ns3 {
  
class MatrixDecoder : public Object
{
  
public:
  MatrixDecoder();
  virtual ~MatrixDecoder();

  void AddEncoder (Ptr<MatrixEncoder> mtxEncoder);
  void DecodeFlows ();
  
  void Init ();

private:
  void MtxDecode(Ptr<MatrixEncoder> target);

  /*Output the flow data, real flows or measured online flows
   */
  void OutputFlowData(std::string type, int swID, double time, const MatrixEncoder::FlowInfo_t& flows);
  
  /*Not defined, use OutputFlowData instead*/
  void OutputRealFlows(Ptr<MatrixEncoder> target);

  /*Output the flow vector and countTable
   */
  void OutputFlowSet(Ptr<MatrixEncoder> target);

  std::vector<Ptr<MatrixEncoder> > m_encoders;  
};
  
}

#endif
