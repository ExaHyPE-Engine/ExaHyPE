// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::riemann {
  template <class Shortcuts, int NumberOfUnknowns, int Order, class T>
  static inline GPU_CALLABLE_METHOD void wellBalancedRusanov(
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR
  );
} // namespace applications::exahype2::exaskred::riemann

#include "WellBalancedRusanov.cpph"
