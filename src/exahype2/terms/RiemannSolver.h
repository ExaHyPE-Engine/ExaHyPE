// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  /**
   * The Riemann solver writes the left and right fluxes of a face and returns the largest wave speed
   * on it. A finite-volume solver passes one point of the face, a DG solver every face node with the
   * fluxes of both sides in FL and FR. On a domain boundary faceIndex names the face from the inner cell,
   * elsewhere it equals normal.
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
