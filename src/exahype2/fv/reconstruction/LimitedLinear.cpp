// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimitedLinear.h"

namespace {
  template <int NumberOfUnknowns, typename ComputePrecision>
  struct StubPdeTerms {
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

  template <int NumberOfUnknowns, typename ComputePrecision>
  struct DefaultSolver final : StubPdeTerms<NumberOfUnknowns, ComputePrecision> {};

  template <int NumberOfUnknowns, typename ComputePrecision>
  struct CustomSolver final : StubPdeTerms<NumberOfUnknowns, ComputePrecision> {
    static GPU_CALLABLE_METHOD ComputePrecision limitedDifference(
      const ComputePrecision* NOALIAS const        leftState,
      const ComputePrecision* NOALIAS const        centreState,
      const ComputePrecision* NOALIAS const        rightState,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const int                                    normal,
      const int                                    unknown
    ) {
      return static_cast<ComputePrecision>(0);
    }

    static GPU_CALLABLE_METHOD void reconstruct(
      const ComputePrecision* NOALIAS const        leftState,
      const ComputePrecision* NOALIAS const        centreState,
      const ComputePrecision* NOALIAS const        rightState,
      const ComputePrecision* NOALIAS const        limitedDifferences,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const int                                    normal,
      const int                                    unknown,
      ComputePrecision&                            minusState,
      ComputePrecision&                            plusState
    ) {
      minusState = centreState[unknown];
      plusState  = centreState[unknown];
    }
  };

  // Taking these addresses makes the core library compile every public branch.
  template <typename StoragePrecision, typename ComputePrecision>
  void instantiateLimitedLinearReconstructionTemplates() {
    constexpr int NumberOfVolumesPerAxis = 8;
    constexpr int NumberOfUnknowns       = 5;
    constexpr int NumberOfData           = 6;
    using Reconstruction                 = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;
    using Default                        = DefaultSolver<NumberOfUnknowns, ComputePrecision>;
    using Custom                         = CustomSolver<NumberOfUnknowns, ComputePrecision>;

    [[maybe_unused]] auto* neighbour =
      &Reconstruction::neighbourAlongNormal<NumberOfVolumesPerAxis, NumberOfData, StoragePrecision>;
    [[maybe_unused]] auto* defaultEvolution =
      &Reconstruction::
        evolveFaceStates<Default, NumberOfUnknowns, NumberOfData, true, true, true, StoragePrecision, ComputePrecision>;
    [[maybe_unused]] auto* customEvolution =
      &Reconstruction::
        evolveFaceStates<Custom, NumberOfUnknowns, NumberOfData, true, true, true, StoragePrecision, ComputePrecision>;
    [[maybe_unused]] auto* rusanov =
      &Reconstruction::rusanovFaceFlux<Default, NumberOfUnknowns, true, ComputePrecision>;
    [[maybe_unused]] auto* rusanovWithoutFlux =
      &Reconstruction::rusanovFaceFlux<Default, NumberOfUnknowns, false, ComputePrecision>;
    [[maybe_unused]] auto* genericUpdate = &Reconstruction::accumulateFluxDivergence<
      Default,
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
      Custom,
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

  [[maybe_unused]] void instantiateLimitedLinearReconstructionTemplates() {
    instantiateLimitedLinearReconstructionTemplates<float, float>();
    instantiateLimitedLinearReconstructionTemplates<float, double>();
    instantiateLimitedLinearReconstructionTemplates<double, float>();
    instantiateLimitedLinearReconstructionTemplates<double, double>();
    instantiateLimitedLinearReconstructionTemplates<long double, long double>();
  }
} // namespace
