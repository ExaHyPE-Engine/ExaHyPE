// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "LandslideLimiting.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    static constexpr int hv = hu + 1;
    static constexpr int z  = 3;
  };

  template <typename T>
  void instantiateLandslideLimitingFunctions() {
    constexpr int NumberOfUnknowns           = 4;
    constexpr int NumberOfAuxiliaryVariables = 0;
    constexpr applications::exahype2::exaskred::landslide::Physics
                                                TestPhysics{9.81, 0.1, 0.47, 0.005, 1100.0, 1025.0, 0.0, 0.01, 1.0};
    const T                                     states[9 * NumberOfUnknowns] = {};
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    [[maybe_unused]] const auto admissibleState = applications::exahype2::exaskred::landslide::
      isAdmissibleGranularLayerState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(states, TestPhysics);
    [[maybe_unused]] const auto admissibleCell = applications::exahype2::exaskred::landslide::
      isAdmissibleGranularLayerCell<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        states,
        9,
        cellSize,
        TestPhysics,
        16.0
      );
    [[maybe_unused]] const auto admissibleCurvature = applications::exahype2::exaskred::landslide::
      hasAdmissibleGranularLayerHeightCurvature2D<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        states,
        3,
        TestPhysics,
        0.3
      );
    T observables[3] = {};
    applications::exahype2::exaskred::landslide::mapGranularLayerDmpObservables<Shortcuts>(observables, states);
    [[maybe_unused]] const auto relaxation =
      applications::exahype2::exaskred::landslide::granularLayerDmpRelaxationParameter(0, 0.0, TestPhysics);
  }

  template void instantiateLandslideLimitingFunctions<float>();
  template void instantiateLandslideLimitingFunctions<double>();
} // namespace
