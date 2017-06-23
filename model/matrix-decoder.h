#ifndef MATRIX_DECODER_H
#define MATRIX_DECODER_H

#include "ns3/object.h"

namespace ns3 {

class MatrixEncoder;
  
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
  void OutputRealFlows(Ptr<MatrixEncoder> target);
  
  std::vector<Ptr<MatrixEncoder> > m_encoders;
  
};
  
}

#endif
