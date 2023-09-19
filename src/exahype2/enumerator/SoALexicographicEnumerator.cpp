#include "SoALexicographicEnumerator.h"

#if !defined(GPUOffloadingOMP)
std::string exahype2::enumerator::SoALexicographicEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(SoA-lex"
      << ",#patches=" << _numberOfPatches
      << ",#vols=" << _numberOfVolumesPerAxisInPatch
      << ",#halo=" << _haloSize
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
