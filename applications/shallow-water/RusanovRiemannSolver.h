// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FrictionLaws.h"
#include "PDE.h"

namespace applications::exahype2::ShallowWater {
  template <class T, class Shortcuts, class Quadrature, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, int Order>
  static inline GPU_CALLABLE_METHOD void rusanovRiemannSolverADERDG(
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

  template <class T, class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  static inline GPU_CALLABLE_METHOD double rusanovRiemannSolverFV(
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
} // namespace applications::exahype2::ShallowWater

#include "RusanovRiemannSolver.cpph"
