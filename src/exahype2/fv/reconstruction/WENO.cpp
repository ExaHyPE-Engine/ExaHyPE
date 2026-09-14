// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "WENO.h"

namespace {
  template <int NumberOfUnknowns, typename ComputePrecision>
  struct StubSolver {
    static GPU_CALLABLE_METHOD void flux(
      const ComputePrecision* NOALIAS const        state,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const int                                    normal,
      ComputePrecision* NOALIAS const              fluxes
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxes[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD void nonconservativeProduct(
      const ComputePrecision* NOALIAS const        averageState,
      const ComputePrecision* NOALIAS const        stateJump,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const int                                    normal,
      ComputePrecision* NOALIAS const              result
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        result[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD void sourceTerm(
      const ComputePrecision* NOALIAS const        state,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      ComputePrecision* NOALIAS const              sources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sources[unknown] = static_cast<ComputePrecision>(0);
      }
    }

    static GPU_CALLABLE_METHOD ComputePrecision maxEigenvalue(
      const ComputePrecision* NOALIAS const        state,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const int                                    normal
    ) {
      return static_cast<ComputePrecision>(1);
    }

    static GPU_CALLABLE_METHOD ComputePrecision solveRiemannProblem(
      const ComputePrecision* NOALIAS const        leftState,
      const ComputePrecision* NOALIAS const        rightState,
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const int                                    normal,
      ComputePrecision* NOALIAS const              leftGoingFlux,
      ComputePrecision* NOALIAS const              rightGoingFlux
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        leftGoingFlux[unknown]  = static_cast<ComputePrecision>(0);
        rightGoingFlux[unknown] = static_cast<ComputePrecision>(0);
      }
      return static_cast<ComputePrecision>(1);
    }
  };

  // Taking these addresses makes the core library compile every public branch.
  template <typename StoragePrecision, typename ComputePrecision>
  void instantiateWENOReconstructionTemplates() {
    constexpr int NumberOfVolumesPerAxis = 8;
    constexpr int NumberOfUnknowns       = 5;
    constexpr int NumberOfData           = 6;
    using Reconstruction                 = exahype2::fv::reconstruction::WENO<exahype2::fv::weno::JiangShu>;
    using Solver                         = StubSolver<NumberOfUnknowns, ComputePrecision>;

    [[maybe_unused]] auto* faceState =
      &Reconstruction::reconstructFaceState<NumberOfData, StoragePrecision, ComputePrecision>;
    [[maybe_unused]] auto* evolution =
      &Reconstruction::evolveFaceStates<Solver, NumberOfUnknowns, NumberOfData, true, true, true, ComputePrecision>;
    [[maybe_unused]] auto* genericUpdate = &Reconstruction::accumulateFluxDivergence<
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
      StoragePrecision,
      ComputePrecision>;
    [[maybe_unused]] auto* customUpdate = &Reconstruction::accumulateFluxDivergence<
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      false,
      false,
      false,
      false,
      true,
      true,
      StoragePrecision,
      ComputePrecision>;
  }

  [[maybe_unused]] void instantiateWENOReconstructionTemplates() {
    instantiateWENOReconstructionTemplates<float, float>();
    instantiateWENOReconstructionTemplates<float, double>();
    instantiateWENOReconstructionTemplates<double, float>();
    instantiateWENOReconstructionTemplates<double, double>();
    instantiateWENOReconstructionTemplates<long double, long double>();
  }
} // namespace
