// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasBoundaryConditions = requires(
    const Precision* const                        inside,
    Precision* const                              outside,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const double                                  t,
    const int                                     normal,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::boundaryConditions(inside, outside, x, h, t, normal, parameters) } -> std::same_as<void>;
  };

  template <typename SolverType>
  concept IsPeriodicAlongEveryAxis = SolverType::PeriodicBoundaries[0] and SolverType::PeriodicBoundaries[1]
    and SolverType::PeriodicBoundaries[DIMENSIONS - 1];

  template <typename SolverType, typename Precision>
  concept HasBoundaryConditionsWhereNeeded =
    HasBoundaryConditions<SolverType, Precision> or IsPeriodicAlongEveryAxis<SolverType>;
} // namespace exahype2::terms
