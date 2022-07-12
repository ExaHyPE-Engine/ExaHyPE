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
  if (Dimensions==2) {
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
      base *= (2*_haloSize);
    }
    else {
      base *= _numberOfDoFsPerAxisInPatch;
    }
  }
  return result * (_unknowns+_numberOfAuxiliaryVariables) + unknown;
}
