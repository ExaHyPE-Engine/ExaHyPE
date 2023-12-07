// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceAoSLexicographicEnumerator.h"

#if defined(GPUOffloadingOff)
std::string exahype2::enumerator::FaceAoSLexicographicEnumerator::toString() const {
  std::ostringstream msg;
  msg
    << "(FaceAoS-lex"
    << ",#face-no=" << _faceNumber << ",#dofs-per-axis=" << _numberOfDoFsPerAxisInCell << ",#halo=" << _haloSize << ",#unknowns=" << _unknowns
    << ",#aux=" << _numberOfAuxiliaryVariables << ")";

  return msg.str();
}
#endif
