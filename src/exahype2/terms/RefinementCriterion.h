// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasRefinementCriterion = requires(
    const Precision* const                        Q,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const double                                  t,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::refinementCriterion(Q, x, h, t, parameters) } -> std::same_as<exahype2::RefinementCommand>;
  };
} // namespace exahype2::terms
