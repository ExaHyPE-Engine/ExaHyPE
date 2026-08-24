// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "ShallowWater.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    static constexpr int hv = hu + 1;
    static constexpr int b  = 3;
  };

  template <typename T>
  void instantiateShallowWater() {
    constexpr int NumberOfUnknowns         = 4;
    constexpr int Normal                   = 0;
    const T       Q[NumberOfUnknowns]      = {static_cast<T>(1.0), T{}, T{}, static_cast<T>(-1.0)};
    const T       deltaQ[NumberOfUnknowns] = {T{}, T{}, T{}, T{}};
    T             output[NumberOfUnknowns] = {T{}, T{}, T{}, T{}};
    const tarch::la::Vector<DIMENSIONS, double> x{};
    const tarch::la::Vector<DIMENSIONS, double> h(1.0);

    [[maybe_unused]] const auto velocity =
      applications::exahype2::exaskred::shallowwater::regularisedVelocity(Q[Shortcuts::h], Q[Shortcuts::hu], 1.0e-5);
    applications::exahype2::exaskred::shallowwater::flux<Shortcuts, NumberOfUnknowns>(
      Q,
      x,
      h,
      0.0,
      0.0,
      Normal,
      1.0e-5,
      output
    );
    [[maybe_unused]] const auto eigenvalue =
      applications::exahype2::exaskred::shallowwater::maxEigenvalue<Shortcuts>(Q, x, h, 0.0, 0.0, Normal, 9.81, 1.0e-5);
    applications::exahype2::exaskred::shallowwater::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
      Q,
      deltaQ,
      x,
      h,
      0.0,
      0.0,
      Normal,
      9.81,
      1.0e-5,
      output
    );
  }

  template void instantiateShallowWater<float>();
  template void instantiateShallowWater<double>();
  template void instantiateShallowWater<long double>();
} // namespace
