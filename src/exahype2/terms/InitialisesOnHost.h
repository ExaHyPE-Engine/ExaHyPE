// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType>
  concept InitialisesOnHost = requires {
    { SolverType::InitialisesOnHost } -> std::convertible_to<bool>;
  } and static_cast<bool>(SolverType::InitialisesOnHost);
} // namespace exahype2::terms
