// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  /**
   * solveSubcellRiemannProblem() takes the arguments of solveRiemannProblem() for one point of a
   * subcell face of a limited DG cell.
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
