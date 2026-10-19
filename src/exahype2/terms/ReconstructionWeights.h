// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <concepts>

namespace exahype2::terms {
  template <typename SolverType>
  concept HasReconstructionWeights = requires(const int entry) {
    { SolverType::reconstructionWeightsOf(entry) } -> std::same_as<int>;
  };
} // namespace exahype2::terms
