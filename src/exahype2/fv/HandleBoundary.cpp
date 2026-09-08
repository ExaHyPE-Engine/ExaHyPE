// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundary.h"

namespace {
  template <int NumberOfData, typename StoragePrecision>
  struct StubSolver final {
    static GPU_CALLABLE_METHOD void boundaryConditions(
      [[maybe_unused]] const StoragePrecision* NOALIAS const        insideState,
      StoragePrecision* NOALIAS const                               outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        outsideState[unknown] = static_cast<StoragePrecision>(0);
      }
    }
  };

  // Instantiating every halo width here makes a template error surface when the
  // core library is built.
  template <int HaloSize, typename StoragePrecision>
  [[maybe_unused]] void instantiateHandleBoundary() {
    constexpr int NumberOfVolumesPerAxis     = 8;
    constexpr int NumberOfUnknowns           = 5;
    constexpr int NumberOfAuxiliaryVariables = 0;
#if DIMENSIONS == 2
    constexpr int NumberOfFaceVolumes = 2 * HaloSize * NumberOfVolumesPerAxis;
#else
    constexpr int NumberOfFaceVolumes = 2 * HaloSize * NumberOfVolumesPerAxis * NumberOfVolumesPerAxis;
#endif

    StoragePrecision                            scratch[NumberOfFaceVolumes * NumberOfUnknowns] = {};
    const tarch::la::Vector<DIMENSIONS, double> patchCentre                                     = {};
    const tarch::la::Vector<DIMENSIONS, double> patchSize                                       = {};

    using Solver = StubSolver<NumberOfUnknowns + NumberOfAuxiliaryVariables, StoragePrecision>;
    Solver solver{};

    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      exahype2::fv::handleBoundary<
        Solver,
        NumberOfVolumesPerAxis,
        HaloSize,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        StoragePrecision>(solver, patchCentre, patchSize, 0.0, faceNumber, scratch);
    }
  }

  [[maybe_unused]] void instantiateHandleBoundary() {
    instantiateHandleBoundary<1, float>();
    instantiateHandleBoundary<1, double>();
    instantiateHandleBoundary<1, long double>();
    instantiateHandleBoundary<2, float>();
    instantiateHandleBoundary<2, double>();
    instantiateHandleBoundary<2, long double>();
  }
} // namespace
