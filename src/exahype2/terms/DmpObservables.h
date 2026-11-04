// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasDmpObservables = requires(Precision* const observables, const Precision* const Q) {
    { SolverType::mapDmpObservables(observables, Q) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
