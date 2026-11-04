// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  /**
   * solveRiemannProblem() writes the fluxes of both sides of a face into FL and FR and returns the
   * largest wave speed. A finite-volume solver passes one point of the face and a DG solver passes
   * every face node. faceIndex is the face number of the inner cell on a domain boundary and equals
   * normal elsewhere.
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
