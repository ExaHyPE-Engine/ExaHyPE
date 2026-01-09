// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  /**
   * Implementation of a LLF-Riemann Solver with Hydrostatic Reconstruction for
   *  - well-balancedness
   *  - preserve the positivity of the material height h
   *
   * Based on the following papers:
   *  - "Second-order finite volume with hydrostatic reconstruction for tsunami simulation"
   *  - "Wetting and Drying Procedures for Shallow Water Simulations" from the book "The Mathematics of Marine
   * Modelling"
   *  - "A FAST AND STABLE WELL-BALANCED SCHEME WITH HYDROSTATIC RECONSTRUCTION FOR SHALLOW WATER FLOWS"
   */
  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD double llfRiemannSolver(
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

#include "LLFRiemannSolver.cpph"
