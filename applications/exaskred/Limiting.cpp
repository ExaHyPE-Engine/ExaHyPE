// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Limiting.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    static constexpr int hv = hu + 1;
    static constexpr int b  = 3;
  };

  template <typename T>
  void instantiateLimitingFunctions() {
    constexpr int               NumberOfUnknowns           = 4;
    constexpr int               NumberOfAuxiliaryVariables = 0;
    constexpr int               NumberOfStates             = 1;
    const T                     state[NumberOfUnknowns]    = {static_cast<T>(1.0), T{}, T{}, static_cast<T>(-1.0)};
    [[maybe_unused]] const auto statistics                 = applications::exahype2::exaskred::limiting::
      collectCellStatistics<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(state, NumberOfStates);
    [[maybe_unused]] const auto wet =
      applications::exahype2::exaskred::limiting::isWetCell<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        state,
        NumberOfStates,
        0.0
      );
    [[maybe_unused]] const auto bounded = applications::exahype2::exaskred::limiting::
      hasBoundedDepthVariation<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        state,
        NumberOfStates,
        0.0,
        1.0
      );
    [[maybe_unused]] const auto admissible = applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(state, 0.0);
  }

  template void instantiateLimitingFunctions<float>();
  template void instantiateLimitingFunctions<double>();
  template void instantiateLimitingFunctions<long double>();
} // namespace
