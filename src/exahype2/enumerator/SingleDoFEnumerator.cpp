// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SingleDoFEnumerator.h"

#if defined(GPUOffloadingOff)
std::string exahype2::enumerator::SingleDoFEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(single-dof"
      << ",#unknowns=" << _unknowns << ",#aux=" << _numberOfAuxiliaryVariables << ")";

#if PeanoDebug > 0
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
