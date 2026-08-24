// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Constant.h"

namespace {
  template <int NumberOfUnknowns, typename ComputePrecision>
  struct StubSolver final {
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

    static GPU_CALLABLE_METHOD void diffusiveSourceTerm(
      const ComputePrecision* NOALIAS const        state,
      const ComputePrecision* NOALIAS const        stateGradient,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      ComputePrecision* NOALIAS const              diffusiveSources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        diffusiveSources[unknown] = static_cast<ComputePrecision>(0);
      }
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
  void instantiateConstantReconstructionTemplates() {
    constexpr int NumberOfVolumesPerAxis = 8;
    constexpr int NumberOfUnknowns       = 5;
    using Solver                         = StubSolver<NumberOfUnknowns, ComputePrecision>;

    [[maybe_unused]] auto* rusanov = &exahype2::fv::reconstruction::Constant::accumulateFluxDivergence<
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
      StoragePrecision,
      ComputePrecision>;
    [[maybe_unused]] auto* nonconservativeProduct = &exahype2::fv::reconstruction::Constant::accumulateFluxDivergence<
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      false,
      true,
      false,
      false,
      false,
      false,
      StoragePrecision,
      ComputePrecision>;
    [[maybe_unused]] auto* diffusiveRusanov = &exahype2::fv::reconstruction::Constant::accumulateFluxDivergence<
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
      StoragePrecision,
      ComputePrecision>;
    [[maybe_unused]] auto* customRiemann = &exahype2::fv::reconstruction::Constant::accumulateFluxDivergence<
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      false,
      false,
      false,
      true,
      true,
      true,
      StoragePrecision,
      ComputePrecision>;
  }

  [[maybe_unused]] void instantiateConstantReconstructionTemplates() {
    instantiateConstantReconstructionTemplates<float, float>();
    instantiateConstantReconstructionTemplates<float, double>();
    instantiateConstantReconstructionTemplates<double, float>();
    instantiateConstantReconstructionTemplates<double, double>();
    instantiateConstantReconstructionTemplates<long double, long double>();
  }
} // namespace
