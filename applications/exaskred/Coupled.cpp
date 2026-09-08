// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Coupled.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    [[maybe_unused]] static constexpr int hv = hu + 1;
    static constexpr int lsh  = 3;
    static constexpr int lshu = 4;
    [[maybe_unused]] static constexpr int lshv = lshu + 1;
    static constexpr int z = 6;
  };

  template <typename T>
  void instantiateCoupledFunctions() {
    constexpr int    NumberOfUnknowns           = 7;
    constexpr int    NumberOfAuxiliaryVariables = 0;
    constexpr int    Order                      = 1;
    constexpr int    Normal                     = 0;
    constexpr double MinimumWaterHeight         = 1.0e-3;
    constexpr applications::exahype2::exaskred::landslide::Physics
            TestPhysics{9.81, 0.1, 0.47, 0.005, 2000.0, 1000.0, 0.0, 0.01, 1.0};
    const T state[NumberOfUnknowns] =
      {static_cast<T>(1.0), T{}, T{}, static_cast<T>(1.0), T{}, T{}, static_cast<T>(-2.0)};
    const T faceStates[(Order + 1) * NumberOfUnknowns] = {
      static_cast<T>(1.0),
      T{},
      T{},
      static_cast<T>(1.0),
      T{},
      T{},
      static_cast<T>(-2.0),
      static_cast<T>(1.0),
      T{},
      T{},
      static_cast<T>(1.0),
      T{},
      T{},
      static_cast<T>(-2.0)
    };
    const T                                     gradient[2 * NumberOfUnknowns]                = {};
    T                                           output[NumberOfUnknowns]                      = {};
    T                                           interfaceFluxLeft[NumberOfUnknowns]           = {};
    T                                           interfaceFluxRight[NumberOfUnknowns]          = {};
    T                                           faceFluxLeft[(Order + 1) * NumberOfUnknowns]  = {};
    T                                           faceFluxRight[(Order + 1) * NumberOfUnknowns] = {};
    const tarch::la::Vector<DIMENSIONS, double> x{};
    const tarch::la::Vector<DIMENSIONS, double> h(1.0);

    applications::exahype2::exaskred::coupled::
      sanitiseDryLayers<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        state,
        output,
        TestPhysics,
        MinimumWaterHeight
      );
    applications::exahype2::exaskred::coupled::flux<Shortcuts, NumberOfUnknowns>(
      state,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      MinimumWaterHeight,
      output
    );
    [[maybe_unused]] const auto eigenvalue = applications::exahype2::exaskred::coupled::maxEigenvalue<
      Shortcuts>(state, x, h, 0.0, 0.0, Normal, TestPhysics, MinimumWaterHeight, 1.0);
    applications::exahype2::exaskred::coupled::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
      state,
      gradient,
      x,
      h,
      0.0,
      0.0,
      Normal,
      TestPhysics,
      MinimumWaterHeight,
      output
    );
    applications::exahype2::exaskred::coupled::diffusiveSourceTerm<
      Shortcuts,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables>(state, gradient, x, h, 0.0, 1.0, TestPhysics, MinimumWaterHeight, output);
    applications::exahype2::exaskred::coupled::
      riemannSolver<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, Order>(
        faceStates,
        faceStates,
        x,
        h,
        0.0,
        1.0,
        Normal,
        TestPhysics,
        MinimumWaterHeight,
        faceFluxLeft,
        faceFluxRight
      );
    [[maybe_unused]] const auto waveSpeed = applications::exahype2::exaskred::coupled::
      solveRiemannProblem<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        state,
        state,
        x,
        h,
        0.0,
        1.0,
        Normal,
        TestPhysics,
        MinimumWaterHeight,
        interfaceFluxLeft,
        interfaceFluxRight
      );
  }

  template void instantiateCoupledFunctions<float>();
  template void instantiateCoupledFunctions<double>();
} // namespace
