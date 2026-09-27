// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasDiffusiveSourceTerm = requires(
    const Precision* const                       Q,
    const Precision* const                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    Precision* const                             S
  ) {
    { SolverType::diffusiveSourceTerm(Q, deltaQ, x, h, t, dt, S) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
