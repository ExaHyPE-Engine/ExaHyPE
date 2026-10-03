// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  /**
   * solveRiemannProblem() writes the fluxes of both sides of a face into FL and FR and returns the
   * largest wave speed. A finite-volume solver passes one point of the face, a DG solver every face
   * node. On a domain boundary faceIndex is the face number of the inner cell, elsewhere it equals normal.
   */
  template <typename SolverType, typename Precision>
  concept HasRiemannSolver = requires(
    const Precision* const                       QL,
    const Precision* const                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const bool                                   isBoundaryFace,
    const int                                    faceIndex,
    Precision* const                             FL,
    Precision* const                             FR
  ) {
    {
      SolverType::solveRiemannProblem(QL, QR, x, h, t, dt, normal, isBoundaryFace, faceIndex, FL, FR)
    } -> std::same_as<double>;
  };
} // namespace exahype2::terms
