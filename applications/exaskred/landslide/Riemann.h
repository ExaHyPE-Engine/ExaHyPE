// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Flux.h"
#include "NonconservativeProduct.h"

#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::landslide {
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

  template <class Shortcuts, int NumberOfUnknowns, int Order, class T>
  static inline GPU_CALLABLE_METHOD void aderdgRiemannSolver(
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR,
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const double                                 t,
    const double                                 dt,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const int                                    direction,
    const bool                                   isBoundaryFace,
    const int                                    faceIndex,
    const double* const NOALIAS                  quadratureWeights
  );
} // namespace applications::exahype2::exaskred::landslide

#include "Riemann.cpph"
