// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasInitialCondition = requires(
    Precision* const                              Q,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::initialCondition(Q, x, h, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
