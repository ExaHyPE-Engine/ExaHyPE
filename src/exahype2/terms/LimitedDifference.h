// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasLimitedDifference = requires(
    const Precision* const                       QL,
    const Precision* const                       Q,
    const Precision* const                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const int                                    normal,
    const int                                    unknown
  ) {
    { SolverType::limitedDifference(QL, Q, QR, x, h, normal, unknown) } -> std::same_as<Precision>;
  };
} // namespace exahype2::terms
