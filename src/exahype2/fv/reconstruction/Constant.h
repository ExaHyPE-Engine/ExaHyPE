// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  struct Constant;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Piecewise-constant reconstruction. The scheme is the first-order Godunov scheme.
 */
struct exahype2::fv::reconstruction::Constant final {
  static constexpr int HaloSize = 1;

  template <
    typename SolverType,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    typename StoragePrecision,
    typename ComputePrecision
  >
  struct StateTerms final {
    ComputePrecision flux[DIMENSIONS][EvaluateFlux ? NumberOfUnknowns : 1]{};
    ComputePrecision maxEigenvalue[DIMENSIONS]{};

    INLINE_METHOD void evaluate(
      const StoragePrecision* NOALIAS               patch,
      const StoragePrecision* NOALIAS const         faces[peano4::FacesPerCell],
      const StoragePrecision* NOALIAS const         auxVarsfaces[peano4::FacesPerCell],
      const tarch::la::Vector<DIMENSIONS, int>&     volumeIndex,
      int                                           centreOffset,
      const tarch::la::Vector<DIMENSIONS, double>&  patchCentre,
      const tarch::la::Vector<DIMENSIONS, double>&  patchSize,
      const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
      double                                        timeStamp,
      double                                        timeStepSize,
      const typename SolverType::RuntimeParameters& parameters
    );
  };

  template <
    typename SolverType,
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
    typename ComputePrecision
  >
  using SweepTerms = std::conditional_t<
    EvaluateCustomRiemann,
    exahype2::fv::NoTermRing,
    exahype2::fv::TermRing<
      StateTerms<
        SolverType,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        EvaluateFlux,
        StoragePrecision,
        ComputePrecision
      >,
      peano4::power(NumberOfVolumesPerAxis, DIMENSIONS - 1)
    >
  >;

  template <
    typename SolverType,
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
    typename ComputePrecision
  >
  using TeamTerms = std::conditional_t<
    EvaluateCustomRiemann,
    exahype2::fv::NoTermRing,
    exahype2::fv::WaveSpeedsOfTeam<ComputePrecision>
  >;

  template <
    typename SolverType,
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
    typename ComputePrecision,
    typename Terms
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static void accumulateFluxDivergence(
    const StoragePrecision* NOALIAS               inputPatch,
    const StoragePrecision* NOALIAS const         faceInputPatches[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS const         faceAuxVarsPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&     volumeIndex,
    int                                           centreOffset,
    const StoragePrecision* NOALIAS               centreState,
    const tarch::la::Vector<DIMENSIONS, double>&  patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  patchSize,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    double                                        timeStamp,
    double                                        timeStepSize,
    int                                           boundaryFaces,
    StoragePrecision* NOALIAS                     outputState,
    double&                                       maxEigenvalueAfterTimeStep,
    const Terms&                                  terms,
    const typename SolverType::RuntimeParameters& parameters
  );

  template <typename SolverType, typename Terms, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision getMaxEigenvalue(
    const Terms&                                  terms,
    int                                           volume,
    const ComputePrecision* NOALIAS               state,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    double                                        timeStamp,
    double                                        timeStepSize,
    int                                           normal,
    const typename SolverType::RuntimeParameters& parameters
  );

private:
  template <typename SolverType, int NumberOfUnknowns, bool EvaluateFlux, typename Terms, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const ComputePrecision* getFluxAndMaxEigenvalue(
    const Terms&                                  terms,
    int                                           volume,
    const ComputePrecision* NOALIAS               state,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    double                                        timeStamp,
    double                                        timeStepSize,
    int                                           normal,
    ComputePrecision* NOALIAS                     fluxBuffer,
    ComputePrecision&                             maxEigenvalue,
    const typename SolverType::RuntimeParameters& parameters
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/Constant.cpph"
