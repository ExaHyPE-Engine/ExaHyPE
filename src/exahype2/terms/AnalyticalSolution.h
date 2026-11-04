// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType>
  concept HasAnalyticalSolution = requires(
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const double                                  t,
    const double                                  dt,
    const double* const                           Q,
    double* const                                 solution,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::analyticalSolution(x, t, dt, Q, solution, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
