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
