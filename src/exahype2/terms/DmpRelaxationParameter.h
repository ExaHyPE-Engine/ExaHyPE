// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasDmpRelaxationParameter = requires(const int observable, const Precision specifiedRelaxationParameter) {
    { SolverType::dmpRelaxationParameter(observable, specifiedRelaxationParameter) } -> std::same_as<Precision>;
  };
} // namespace exahype2::terms
