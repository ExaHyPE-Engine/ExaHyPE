// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Landslide.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    static constexpr int hv = hu + 1;
    static constexpr int z  = 3;
  };

  template <typename T>
  void instantiateLandslideFunctions() {
    constexpr int NumberOfUnknowns           = 4;
    constexpr int NumberOfAuxiliaryVariables = 0;
    constexpr int Normal                     = 0;
    constexpr applications::exahype2::exaskred::landslide::Physics
            TestPhysics{9.81, 0.1, 0.47, 0.005, 1100.0, 1025.0, 0.0, 0.01, 1.0};
    const T state[NumberOfUnknowns]        = {static_cast<T>(1.0), T{}, T{}, static_cast<T>(-1.0)};
    const T gradient[2 * NumberOfUnknowns] = {};
    T       output[NumberOfUnknowns]       = {};
    const tarch::la::Vector<DIMENSIONS, double> x{};
    const tarch::la::Vector<DIMENSIONS, double> h(1.0);

    applications::exahype2::exaskred::landslide::
      sanitiseDryState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(state, output, TestPhysics);
    [[maybe_unused]] const auto faceGravity =
      applications::exahype2::exaskred::landslide::interfaceEffectiveGravity<Shortcuts>(state, state, TestPhysics);
    applications::exahype2::exaskred::landslide::flux<Shortcuts, NumberOfUnknowns>(
      state,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      output
    );
    applications::exahype2::exaskred::landslide::flux<Shortcuts, NumberOfUnknowns>(
      state,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      9.81,
      output
    );
    [[maybe_unused]] const auto fluxSpeed = applications::exahype2::exaskred::landslide::fluxEigenvalue<Shortcuts>(
      state,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics
    );
    [[maybe_unused]] const auto coreFluxSpeed = applications::exahype2::exaskred::landslide::fluxEigenvalue<
      Shortcuts>(state, x, h, 0.0, 0.0, Normal, TestPhysics, 9.81);
    [[maybe_unused]] const auto eigenvalue = applications::exahype2::exaskred::landslide::effectiveMaxEigenvalue<
      Shortcuts>(state, x, h, 0.0, 0.0, Normal, TestPhysics, 1.0);
    [[maybe_unused]] const auto coreEigenvalue = applications::exahype2::exaskred::landslide::effectiveMaxEigenvalue<
      Shortcuts>(state, x, h, 0.0, 0.0, Normal, TestPhysics, 9.81, true, 1.0);
    [[maybe_unused]] const auto dragSpeed =
      applications::exahype2::exaskred::landslide::waterFrictionDragSourceSpeed<Shortcuts>(state, TestPhysics, true);
    applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
      state,
      gradient,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      output
    );
    applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
      state,
      gradient,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      9.81,
      output
    );
    applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
      Shortcuts,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables>(state, gradient, x, h, 0.0, 1.0, TestPhysics, output);
    applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
      Shortcuts,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables>(state, gradient, x, h, 0.0, 1.0, TestPhysics, 9.81, true, output);
  }

  template void instantiateLandslideFunctions<float>();
  template void instantiateLandslideFunctions<double>();
} // namespace
