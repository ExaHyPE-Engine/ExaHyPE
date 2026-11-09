// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasDmpObservables = requires(
    Precision* const                              observables,
    const Precision* const                        Q,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::mapDmpObservables(observables, Q, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
