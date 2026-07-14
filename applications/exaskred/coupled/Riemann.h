// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Flux.h"
#include "NonconservativeProduct.h"

#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::coupled {
  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class T>
  static inline GPU_CALLABLE_METHOD double solveRiemannProblem(
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
} // namespace applications::exahype2::exaskred::coupled

#include "Riemann.cpph"
