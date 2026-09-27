// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasPointSource = requires(
    const Precision* const Q,
    const double* const    x,
    const double           t,
    const double           dt,
    Precision* const       forceVector,
    const int              n
  ) {
    { SolverType::pointSource(Q, x, t, dt, forceVector, n) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
