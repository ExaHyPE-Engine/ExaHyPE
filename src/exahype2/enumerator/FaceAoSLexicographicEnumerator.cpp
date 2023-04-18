#include "FaceAoSLexicographicEnumerator.h"


#if !defined(GPUOffloadingOMP)
std::string exahype2::enumerator::FaceAoSLexicographicEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(FaceAoS-lex"
      << ",#face-no=" << _faceNumber
      << ",#dofs-per-axis=" << _numberOfDoFsPerAxisInPatch
      << ",#halo=" << _haloSize
      << ",#unknowns=" << _unknowns
      << ",#aux=" << _numberOfAuxiliaryVariables
      << ")";

  return msg.str();
}
#endif


int exahype2::enumerator::FaceAoSLexicographicEnumerator::size() const {
  if constexpr (Dimensions==2) {
    return (_numberOfDoFsPerAxisInPatch)
        * (2*_haloSize)
        * (_unknowns+_numberOfAuxiliaryVariables);
  }
  else {
    return (_numberOfDoFsPerAxisInPatch*_numberOfDoFsPerAxisInPatch)
        * (2*_haloSize)
        * (_unknowns+_numberOfAuxiliaryVariables);
  }
}


int exahype2::enumerator::FaceAoSLexicographicEnumerator::operator()(
  const tarch::la::Vector<Dimensions,int>&  dofIndex,
  int unknown
) const {
  int base   = 1;
  int result = 0;
  for (int d=0; d<Dimensions; d++) {
    result += dofIndex(d) * base;
    if (d==_faceNumber % Dimensions) {
      #if defined(GPUOffloadingOMP)
      assertion1(dofIndex(d)>=0,dofIndex);
      assertion1(dofIndex(d)<2*_haloSize,dofIndex);
      #else
      assertion2(dofIndex(d)>=0,toString(),dofIndex);
      assertion2(dofIndex(d)<2*_haloSize,toString(),dofIndex);
      #endif
      base *= (2*_haloSize);
    }
    else {
      #if defined(GPUOffloadingOMP)
      assertion1(dofIndex(d)>=0,dofIndex);
      assertion1(dofIndex(d)<_numberOfDoFsPerAxisInPatch,dofIndex);
      #else
      assertion2(dofIndex(d)>=0,toString(),dofIndex);
      assertion2(dofIndex(d)<_numberOfDoFsPerAxisInPatch,toString(),dofIndex);
      #endif
      base *= _numberOfDoFsPerAxisInPatch;
    }
  }
  return result * (_unknowns+_numberOfAuxiliaryVariables) + unknown;
}
