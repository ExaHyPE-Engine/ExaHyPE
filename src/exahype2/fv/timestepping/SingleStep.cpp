// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SingleStep.h"

namespace {
  struct StubReconstruction final {
    static constexpr int HaloSize = 1;

    template <
      typename Solver,
      int  NumberOfVolumesPerAxis,
      int  NumberOfUnknowns,
      int  NumberOfAuxiliaryVariables,
      bool EvaluateFlux,
      bool EvaluateNonconservativeProduct,
      bool EvaluateSource,
      bool EvaluateDiffusiveSource,
      bool EvaluateCustomRiemann,
      bool EvaluateMaxEigenvalue,
      typename StoragePrecision,
      typename ComputePrecision>
    static GPU_CALLABLE_METHOD void accumulateFluxDivergence(
      const StoragePrecision* NOALIAS const        inputPatch,
      const StoragePrecision* NOALIAS const        faceInputPatches[TWO_TIMES_D],
      const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
      const int                                    centreOffset,
      const StoragePrecision* NOALIAS const        centreState,
      const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
      const tarch::la::Vector<DIMENSIONS, double>& patchSize,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      StoragePrecision* NOALIAS const              outputState,
      double&                                      maxEigenvalueAfterTimeStep
    ) {}
  };

  template <int NumberOfUnknowns, typename ComputePrecision>
  struct StubSolver final {
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
  };

  // Taking these addresses makes the core library compile every public branch.
  template <typename StoragePrecision, typename ComputePrecision>
  void instantiateSingleStepTemplates() {
    constexpr int NumberOfVolumesPerAxis = 8;
    constexpr int NumberOfUnknowns       = 5;
    using Solver                         = StubSolver<NumberOfUnknowns, ComputePrecision>;

    [[maybe_unused]] auto* homogeneous = &exahype2::fv::timestepping::SingleStep::apply<
      StubReconstruction,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      0,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      StoragePrecision,
      ComputePrecision>;
    [[maybe_unused]] auto* sourceAndEigenvalue = &exahype2::fv::timestepping::SingleStep::apply<
      StubReconstruction,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      1,
      false,
      false,
      true,
      false,
      false,
      false,
      true,
      StoragePrecision,
      ComputePrecision>;
  }

  [[maybe_unused]] void instantiateSingleStepTemplates() {
    instantiateSingleStepTemplates<float, float>();
    instantiateSingleStepTemplates<float, double>();
    instantiateSingleStepTemplates<double, float>();
    instantiateSingleStepTemplates<double, double>();
    instantiateSingleStepTemplates<long double, long double>();
  }
} // namespace
