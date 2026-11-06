// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, int NumberOfPointSources>
  concept HasPointSourceLocations = requires(
    double                                        location[NumberOfPointSources][DIMENSIONS],
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::initPointSourceLocations(location, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
