// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AoSLexicographicEnumerator.h"

#if !defined(WITH_GPU)
std::string exahype2::enumerator::AoSLexicographicEnumerator::toString() const {
  std::ostringstream msg;
  msg
    << "(AoS-lex"
    << ",#number-of-cells=" << _numberOfCells
    << ",#number-of-dofs=" << _numberOfDoFsPerAxisInCell
    << ",#halo-size=" << _haloSize
    << ",#number-of-unknowns=" << _numberOfUnknowns
    << ",#number-of-auxiliary-variables=" << _numberOfAuxiliaryVariables
    << ")";

#if PEANO_DEBUG > 0
  msg
    << ", index-sequence=["
    << "," << (*this)(0, {0, 0}, 0) << "," << (*this)(1, {0, 0}, 0) << ",..."
    << "," << (*this)(0, {1, 0}, 0) << "," << (*this)(0, {2, 0}, 0) << ",..."
    << "," << (*this)(0, {0, 1}, 0) << "," << (*this)(0, {0, 2}, 0) << ",..."
    << "," << (*this)(0, {0, 0}, 1) << "," << (*this)(0, {0, 0}, 2) << ",..."
    << "," << this->size() << "]";
#endif

  return msg.str();
}
#endif
