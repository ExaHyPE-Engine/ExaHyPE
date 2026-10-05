// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasPostprocessUpdatedPatch = requires(
    Precision* const                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt
  ) {
    { SolverType::postprocessUpdatedPatch(Q, x, h, t, dt) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
