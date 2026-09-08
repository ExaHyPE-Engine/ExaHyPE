// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "AugmentedState.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    [[maybe_unused]] static constexpr int hv = hu + 1;
    static constexpr int b = 3;
  };

  template <typename T>
  void instantiateAugmentedState() {
    constexpr int NumberOfUnknowns             = 4;
    const T       leftState[NumberOfUnknowns]  = {static_cast<T>(1.0), T{}, T{}, static_cast<T>(-1.0)};
    const T       rightState[NumberOfUnknowns] = {static_cast<T>(1.0), T{}, T{}, static_cast<T>(-1.0)};
    T             leftFlux[NumberOfUnknowns]{};
    T             rightFlux[NumberOfUnknowns]{};
    const tarch::la::Vector<DIMENSIONS, double> x{};
    const tarch::la::Vector<DIMENSIONS, double> h(1.0);
    [[maybe_unused]] const auto                 maximumEigenvalue =
      applications::exahype2::exaskred::riemann::augmentedState<Shortcuts, Shortcuts::b, true>(
        leftState,
        rightState,
        x,
        h,
        0.0,
        0.1,
        0,
        9.81,
        1.0e-5,
        leftFlux,
        rightFlux
      );
  }

  template void instantiateAugmentedState<float>();
  template void instantiateAugmentedState<double>();
} // namespace
