// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::meteotsunami {
  template <class Shortcuts, int NumberOfUnknowns, class T>
  static inline GPU_CALLABLE_METHOD void nonconservativeProduct(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             BTimesDeltaQ
  );
} // namespace applications::exahype2::exaskred::meteotsunami

#include "NonconservativeProduct.cpph"
