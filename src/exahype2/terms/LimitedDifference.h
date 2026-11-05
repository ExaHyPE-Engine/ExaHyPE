// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasLimitedDifference = requires(
    const Precision* const                        QL,
    const Precision* const                        Q,
    const Precision* const                        QR,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const int                                     normal,
    Precision* const                              limitedDifferences,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::limitedDifference(QL, Q, QR, x, h, normal, limitedDifferences, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
