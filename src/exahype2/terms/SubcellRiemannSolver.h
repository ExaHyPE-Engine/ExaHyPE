// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  /**
   * The Riemann solver of the finite-volume subcells of a limited DG cell takes the arguments of
   * HasRiemannSolver for one point of a subcell face.
   */
  template <typename SolverType, typename Precision>
  concept HasSubcellRiemannSolver = requires(
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
      SolverType::solveSubcellRiemannProblem(QL, QR, x, h, t, dt, normal, isBoundaryFace, faceIndex, FL, FR)
    } -> std::same_as<double>;
  };
} // namespace exahype2::terms
