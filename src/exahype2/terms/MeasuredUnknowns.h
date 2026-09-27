// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/terms/AnalyticalSolution.h"

namespace exahype2::terms {
  /** A solver that names the unknowns of its error measurement carries an analytical solution. */
  template <typename SolverType>
  concept HasMeasuredUnknowns = exahype2::terms::HasAnalyticalSolution<SolverType> and requires {
    { SolverType::MeasuredUnknowns.size() } -> std::convertible_to<std::size_t>;
  };
} // namespace exahype2::terms
