// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  /**
   * A solver whose initial condition runs on the host and uploads the cells afterwards sets this constant.
   */
  template <typename SolverType>
  concept InitialisesOnHost = requires {
    { SolverType::InitialisesOnHost } -> std::convertible_to<bool>;
  } and static_cast<bool>(SolverType::InitialisesOnHost);
} // namespace exahype2::terms
