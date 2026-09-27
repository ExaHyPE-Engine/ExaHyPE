// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  /** A solver whose initial condition fills every node of a cell in one call sets this constant. */
  template <typename SolverType>
  concept InitialisesPatches = requires {
    { SolverType::InitialisesPatches } -> std::convertible_to<bool>;
  } and static_cast<bool>(SolverType::InitialisesPatches);
} // namespace exahype2::terms
