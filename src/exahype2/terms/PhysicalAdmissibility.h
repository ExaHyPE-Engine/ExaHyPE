// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasPhysicalAdmissibility = requires(
    const Precision* const                       Q,
    const int                                    numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) {
    { SolverType::isPhysicallyAdmissible(Q, numberOfStates, x, h) } -> std::same_as<bool>;
  };
} // namespace exahype2::terms
