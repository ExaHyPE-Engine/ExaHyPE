// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FiniteVolume.h"

#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/timestepping/SingleStep.h"

namespace {
  // This stateless stub lets the core library instantiate every PDE-term branch.
  template <int NumberOfUnknowns, typename ComputePrecision>
  struct StubSolver final {
    static GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxes[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        averageState,
      [[maybe_unused]] const ComputePrecision* NOALIAS const        stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               result
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        result[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD void sourceTerm(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               sources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sources[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD ComputePrecision maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      return static_cast<ComputePrecision>(1);
    }

    static GPU_CALLABLE_METHOD void diffusiveSourceTerm(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const ComputePrecision* NOALIAS const        stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               diffusiveSources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        diffusiveSources[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD ComputePrecision solveRiemannProblem(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        leftState,
      [[maybe_unused]] const ComputePrecision* NOALIAS const        rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               leftGoingFlux,
      ComputePrecision* NOALIAS const                               rightGoingFlux
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        leftGoingFlux[unknown]  = static_cast<ComputePrecision>(0);
        rightGoingFlux[unknown] = static_cast<ComputePrecision>(0);
      }
      return static_cast<ComputePrecision>(1);
    }
  };

  // Instantiating every branch here makes a template error surface when the core
  // library is built.
  template <typename StoragePrecision, typename ComputePrecision>
  [[maybe_unused]] double instantiateFiniteVolume() {
    constexpr int NumberOfVolumesPerAxis = 8;
    constexpr int NumberOfUnknowns       = 5;

    StoragePrecision                            inputStateStorage[1]           = {};
    StoragePrecision                            outputStateStorage[1]          = {};
    const StoragePrecision*                     faceInputPatches[TWO_TIMES_D]  = {};
    StoragePrecision*                           faceOutputPatches[TWO_TIMES_D] = {};
    const tarch::la::Vector<DIMENSIONS, double> patchCentre                    = {};
    const tarch::la::Vector<DIMENSIONS, double> patchSize                      = {};

    using Solver = StubSolver<NumberOfUnknowns, ComputePrecision>;
    auto result  = 0.0;

    // Constant reconstruction with a generic Rusanov flux and a final-state
    // eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      true,
      false,
      false,
      false,
      false,
      false,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Constant reconstruction with a generic Rusanov flux, non-conservative
    // product, source and a final-state eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      true,
      true,
      true,
      false,
      false,
      false,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Constant reconstruction with a non-conservative product only.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      false,
      true,
      false,
      false,
      false,
      false,
      false,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Constant reconstruction with a generic Rusanov flux, non-conservative
    // product, diffusive source and a final-state eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      true,
      true,
      false,
      true,
      false,
      false,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Constant reconstruction with a custom Riemann solver, non-conservative
    // product, diffusive source, returned wave speeds and a final-state
    // eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      true,
      true,
      false,
      true,
      true,
      true,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Limited-linear MUSCL-Hancock reconstruction with a generic Rusanov flux,
    // source and a final-state eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::VanAlbada>,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      true,
      false,
      true,
      false,
      false,
      false,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Limited-linear MUSCL-Hancock reconstruction with a custom Riemann solver,
    // returned wave speeds and a final-state eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      true,
      false,
      false,
      false,
      true,
      true,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    // Limited-linear MUSCL-Hancock reconstruction with a generic Rusanov flux,
    // non-conservative product and a final-state eigenvalue.
    result += exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      true,
      true,
      false,
      false,
      false,
      true,
      true,
      StoragePrecision,
      ComputePrecision>(
      inputStateStorage,
      outputStateStorage,
      faceInputPatches,
      faceOutputPatches,
      patchCentre,
      patchSize,
      0.0,
      0.0
    );

    return result;
  }

  [[maybe_unused]] void instantiateFiniteVolume() {
    instantiateFiniteVolume<float, float>();
    instantiateFiniteVolume<float, double>();
    instantiateFiniteVolume<double, float>();
    instantiateFiniteVolume<double, double>();
    instantiateFiniteVolume<long double, long double>();
  }
} // namespace
