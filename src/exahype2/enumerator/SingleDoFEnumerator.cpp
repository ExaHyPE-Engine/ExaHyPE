#include "SingleDoFEnumerator.h"


#if !defined(GPUOffloadingOMP)
std::string exahype2::enumerator::SingleDoFEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(single-volume"
      << ",#unknowns=" << _unknowns
      << ",#aux=" << _numberOfAuxiliaryVariables
      << ")";

  #if PeanoDebug>0
  msg << ", index-sequence=["
      << "," << (*this)(0,{0,0},0)
      << "," << (*this)(1,{0,0},0)
      << ",..."
      << "," << (*this)(0,{1,0},0)
      << "," << (*this)(0,{2,0},0)
      << ",..."
      << "," << (*this)(0,{0,1},0)
      << "," << (*this)(0,{0,2},0)
      << ",..."
      << "," << (*this)(0,{0,0},1)
      << "," << (*this)(0,{0,0},2)
      << ",..."
      << "," << this->size()
      << "]"
      ;
  #endif

  return msg.str();
}
#endif

