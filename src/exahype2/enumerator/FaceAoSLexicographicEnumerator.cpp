// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceAoSLexicographicEnumerator.h"

#if defined(GPUOffloadingOff)
std::string exahype2::enumerator::FaceAoSLexicographicEnumerator::toString() const {
  std::ostringstream msg;
  msg
    << "(FaceAoS-lex"
    << ",#face-number=" << _faceNumber
    << ",#number-of-dofs-per-axis=" << _numberOfDoFsPerAxisInCell
    << ",#halo-size=" << _haloSize
    << ",#number-of-unknowns=" << _numberOfUnknowns
    << ",#number-of-auxiliary-variables=" << _numberOfAuxiliaryVariables
    << ")";

  return msg.str();
}
#endif
